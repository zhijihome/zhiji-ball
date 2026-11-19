// Zhiji pendant face — GC9A01 240x240 round screen on the agent board.
// Parametric eyes (no bitmaps): blink, drift, state mouth. Runs on core 0 at low
// priority so BLE/HID/AirPlay paths are never blocked.
//
// Wiring (ZHIJI-PENDANT-SPEC §2):
//   VCC->3V3 GND->GND SCL->D8 SDA->D10 DC->D3 CS->D2 RES->D4 BLK->D5
#ifndef FACE_DISPLAY_H
#define FACE_DISPLAY_H

#include <Arduino.h>

enum FaceState : int {
  FACE_IDLE = 0, FACE_WORK, FACE_HAPPY, FACE_ERROR, FACE_SLEEP,
  FACE_THINK, FACE_EXCITE, FACE_SURPRISE, FACE_CURIOUS,
  FACE_CONFUSED, FACE_SAD, FACE_PLAYFUL, FACE_CELEBRATE, FACE_COUNT
};

void face_begin();                  // call once in setup()
void face_set_state(FaceState s);   // agent side sets intent
FaceState face_state();

#endif
