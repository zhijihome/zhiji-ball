// 独立点亮测试: 红绿蓝轮流 + 画脸. 屏板 JK071-008, 7pin SPI
#include <Arduino_GFX_Library.h>
#define PIN_DC 4
#define PIN_CS 3
#define PIN_SCK 7
#define PIN_MOSI 9
#define PIN_RES 5

Arduino_DataBus *bus;
Arduino_GC9A01 *gfx;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("face_test boot");
  bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI, GFX_NOT_DEFINED, HSPI);
  gfx = new Arduino_GC9A01(bus, PIN_RES, 0, true);
  gfx->begin();
  Serial.println("gfx begin done");
  gfx->fillScreen(0xF800); delay(800);
  gfx->fillScreen(0x07E0); delay(800);
  gfx->fillScreen(0x001F); delay(800);
  Serial.println("color test done");
}

void loop() {
  gfx->fillScreen(0x0000);
  gfx->fillEllipse(75, 100, 16, 22, 0xFFFF);
  gfx->fillEllipse(165, 100, 16, 22, 0xFFFF);
  gfx->drawArc(120, 158, 20, 16, 30, 150, 0xC618);
  delay(1500);
  // 眨眼
  gfx->fillRect(50, 70, 140, 60, 0x0000);
  gfx->fillEllipse(75, 100, 16, 3, 0xFFFF);
  gfx->fillEllipse(165, 100, 16, 3, 0xFFFF);
  delay(300);
}
