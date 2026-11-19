// Zhiji pendant face v3 — full GrokBot port (BSD-3, nasawz/GrokBot via
// Coiggahou2002/grokbot-web). All 25 expression rings from face_expr.h.
// Behavior mirrors src/engine.ts + src/data/states.ts: each FaceState has an
// expression pool re-sampled on a cadence; switching morphs every ring point;
// blink is a y-squash about the eye centroid. Dirty-rect repaint only.
#include "face_display.h"
#include "face_expr.h"
#include <Arduino_GFX_Library.h>

#define PIN_DC   4
#define PIN_CS   3
#define PIN_SCK  7
#define PIN_MOSI 9
#define PIN_RES  5
#define PIN_BLK  6

static Arduino_DataBus *bus;
static Arduino_GC9A01 *gfx;
static Arduino_Canvas *cv;  // full-frame buffer: one atomic SPI write per frame
static volatile FaceState state_ = FACE_IDLE;
static unsigned long state_since = 0;

static const float SCALE = 240.0f / 228.541f;
#define EYE_SCALE 1.0f  // enlarge ring about its own centroid (big-capsule look)
static const uint16_t C_EYE = 0xFFFF, C_DIM = 0x7BEF, C_ERR = 0xF800;

// state -> expression pool + cadences (from grokbot-web src/data/states.ts)
struct StateDef {
  uint8_t pool[6]; uint8_t n;
  uint16_t emin, emax;   // expression re-sample cadence ms
  uint16_t bmin, bmax;   // blink cadence ms, 0 = never
};
static const StateDef STATES[] = {
  /* IDLE     = idle      */ {{10,0,26,25,27,1}, 6, 6000, 11000, 4000, 9000},
  /* WORK     = working   */ {{ 7,16,11,10},     4, 1800, 3200,  2800, 5500},
  /* HAPPY    = laughing  */ {{ 2,11,17},        3, 1200, 2400,  2500, 5000},
  /* ERROR    = angry     */ {{ 7,16},           2, 2200, 3800,  3500, 7000},
  /* SLEEP    = sleeping  */ {{13,22,4},         3, 6000, 10000, 0,    0},
  /* THINK    = thinking  */ {{14,17,5},         3, 2000, 3600,  3500, 7000},
  /* EXCITE   = excited   */ {{ 2,17,3,11},      4, 1100, 2000,  2000, 4000},
  /* SURPRISE = surprised */ {{ 3,12},           2, 2500, 4000,  1800, 3500},
  /* CURIOUS  = curious   */ {{ 3,0,15,26},      4, 1800, 3200,  2500, 5500},
  /* CONFUSED = confused  */ {{14,5,23},         3, 2200, 3800,  2800, 5500},
  /* SAD      = sad       */ {{ 4,13,22},        3, 4000, 7000,  4000, 8000},
  /* PLAYFUL  = playful   */ {{ 2,17,11},        3, 1500, 3000,  2000, 4500},
  /* CELEBRATE= celebrate */ {{ 2,17,20},        3, 1400, 2600,  2200, 4500},
};

// morph state: current -> target over ~280ms smoothstep (engine.ts uses a
// spring; a fixed smoothstep is close enough at this size)
static float cur[2][96], tgt[2][96];
static float morph_t = 1.0f;
static uint8_t cur_expr = 0;
static unsigned long morph_start = 0;
#define MORPH_MS 450

static void copyRing(float *d, const float *s) { memcpy(d, s, 96 * sizeof(float)); }

static void selectExpr(uint8_t e, unsigned long now) {
  if (e == cur_expr && morph_t >= 1.0f) return;
  for (int k = 0; k < 2; k++)
    for (int i = 0; i < 96; i++)
      cur[k][i] = cur[k][i] + (tgt[k][i] - cur[k][i]) * morph_t; // snapshot displayed
  copyRing(tgt[0], EXPR_RINGS[e][0]);
  copyRing(tgt[1], EXPR_RINGS[e][1]);
  cur_expr = e;
  morph_t = 0;
  morph_start = now;
}

static void blendedRing(int k, float *out) {
  float s = morph_t * morph_t * (3.0f - 2.0f * morph_t); // smoothstep
  for (int i = 0; i < 96; i++) out[i] = cur[k][i] + (tgt[k][i] - cur[k][i]) * s;
}

static void ringCentroid(const float *r, float *cx, float *cy) {
  *cx = *cy = 0;
  for (int i = 0; i < 48; i++) { *cx += r[i*2]; *cy += r[i*2+1]; }
  *cx /= 48; *cy /= 48;
}

// project blended ring -> screen coords (lid = y-squash, dx/dy = gaze shift)
static void projectRing(const float *r, float cx, float cy, float lid,
                        float dx, float dy, float *px, float *py) {
  float scx = cx * SCALE + dx, scy = cy * SCALE + dy;
  for (int i = 0; i < 48; i++) {
    px[i] = scx + (r[i*2]   * SCALE + dx - scx) * EYE_SCALE;
    py[i] = scy + (r[i*2+1] * SCALE + dy - scy) * EYE_SCALE * (1.0f - lid);
  }
}

static void fillPoly(const float *px, const float *py, float cx, float cy, uint16_t col) {
  for (int i = 0; i < 48; i++) {
    int j = (i + 1) % 48;
    cv->fillTriangle((int)cx, (int)cy, (int)px[i], (int)py[i], (int)px[j], (int)py[j], col);
  }
}

static unsigned long cadence(uint16_t lo, uint16_t hi) {
  return lo + (esp_random() % (uint32_t)(hi - lo + 1));
}

static void faceTask(void *) {
  gfx->begin();
  gfx->fillScreen(0x0000);
  cv = new Arduino_Canvas(240, 240, gfx);
  cv->begin();
  cv->fillScreen(0x0000);
  pinMode(PIN_BLK, OUTPUT);
  digitalWrite(PIN_BLK, HIGH);
  uint8_t e0 = STATES[(int)state_].pool[0];
  copyRing(cur[0], EXPR_RINGS[e0][0]); copyRing(tgt[0], EXPR_RINGS[e0][0]);
  copyRing(cur[1], EXPR_RINGS[e0][1]); copyRing(tgt[1], EXPR_RINGS[e0][1]);
  cur_expr = e0;

  float oLx[48], oLy[48], oRx[48], oRy[48];
  bool drawn = false;
  int lookX = 0, lookY = 0, tgtX = 0, tgtY = 0;
  FaceState last_state = (FaceState)-1;
  unsigned long next_expr = millis() + 9000, next_blink = millis() + 4000,
                next_look = millis() + 4500;
  for (;;) {
    unsigned long now = millis();
    const StateDef &sd = STATES[(int)state_];
    if ((state_ == FACE_HAPPY || state_ == FACE_ERROR) && now - state_since > 4000)
      state_ = FACE_IDLE;

    // state switched -> rest on pool[0], re-seed cadences
    if (state_ != last_state) {
      last_state = state_;
      selectExpr(sd.pool[0], now);
      next_expr = now + cadence(sd.emin, sd.emax);
      next_blink = sd.bmax ? now + cadence(sd.bmin, sd.bmax) : (unsigned long)-1;
    }
    // auto-expression: re-sample the pool like engine.ts #expressionTimer
    if (now >= next_expr) {
      next_expr = now + cadence(sd.emin, sd.emax);
      uint8_t pick = sd.pool[esp_random() % sd.n];
      if (pick != cur_expr) selectExpr(pick, now); else next_expr = now + 800;
    }
    if (morph_t < 1.0f) morph_t = fminf(1.0f, (now - morph_start) / (float)MORPH_MS);

    // blink: when now >= next_blink run a 360ms timeline (120 close, 60 shut, 180 open)
    long bt = (long)now - (long)next_blink;
    float lid = 0;
    if (sd.bmax && bt >= 0) {
      if (bt < 120)      lid = bt / 120.0f * 0.95f;
      else if (bt < 180) lid = 0.95f;
      else if (bt < 360) lid = 0.95f * (1.0f - (bt - 180) / 180.0f);
      else next_blink = now + ((esp_random() % 100 < 15) ? 450 : cadence(sd.bmin, sd.bmax));
    }
    if (state_ == FACE_WORK) lid = max(lid, 0.15f);

    if (now > next_look) {
      next_look = now + 3200 + (esp_random() % 4000);
      tgtX = (int)(esp_random() % 29) - 14;
      tgtY = (int)(esp_random() % 11) - 5;
    }
    lookX += (tgtX - lookX) / 4;
    lookY += (tgtY - lookY) / 4;

    // repaint only while something is animating; frame is one atomic flush
    bool anim = morph_t < 1.0f || lid > 0 || lookX != tgtX || lookY != tgtY || !drawn;
    if (anim) {
      float bL[96], bR[96], nLx[48], nLy[48], nRx[48], nRy[48];
      blendedRing(0, bL); blendedRing(1, bR);
      float cxL, cyL, cxR, cyR;
      ringCentroid(bL, &cxL, &cyL); ringCentroid(bR, &cxR, &cyR);
      // face space is already centered on the screen (114.27 * SCALE = 120)
      float dx = lookX;
      uint16_t ec = (state_ == FACE_SLEEP) ? C_DIM : (state_ == FACE_ERROR ? C_ERR : C_EYE);
      projectRing(bL, cxL, cyL, lid, dx, lookY, nLx, nLy);
      projectRing(bR, cxR, cyR, lid, dx, lookY, nRx, nRy);
      cv->fillScreen(0x0000);
      float scyL = cyL * SCALE + lookY, scyR = cyR * SCALE + lookY;
      fillPoly(nLx, nLy, cxL * SCALE + dx, scyL, ec);
      fillPoly(nRx, nRy, cxR * SCALE + dx, scyR, ec);
      cv->flush();
      memcpy(oLx, nLx, sizeof(oLx)); memcpy(oLy, nLy, sizeof(oLy));
      memcpy(oRx, nRx, sizeof(oRx)); memcpy(oRy, nRy, sizeof(oRy));
      drawn = true;
    }
    vTaskDelay(pdMS_TO_TICKS(40));
  }
}

void face_begin() {
  bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, GFX_NOT_DEFINED, HSPI);
  gfx = new Arduino_GC9A01(bus, PIN_RES, 0, true);
  xTaskCreatePinnedToCore(faceTask, "face", 6144, NULL, 1, NULL, 0);
}

void face_set_state(FaceState s) {
  if (s < 0 || s >= FACE_COUNT) return;
  state_ = s; state_since = millis();
}
FaceState face_state() { return state_; }
