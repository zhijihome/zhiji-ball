# Face reference

Expression data: `hardware/hid_bridge/face_expr.h` — 28 rings
(25 GrokBot expressions + 3 mirrored glances of expr 0), each
`2 eyes × 48 points` in face-space (~228.541 span, centered on the face).

States (BLE/serial `face N`):

| N | state | pool | behavior |
|---|---|---|---|
| 0 | idle | {10,0,26,25,27,1} | center capsule + 5-direction glance, 6–11 s |
| 1 | work | {7,16,11,10} | fast rotation |
| 2 | happy | {2,11,17} | auto-returns to idle ~4 s |
| 3 | error | {7,16} | red eyes, auto-returns ~4 s |
| 4 | sleep | {13,22,4} | closed bars, no blink |
| 5 | think | {14,17,5} | |
| 6 | excite | {2,17,3,11} | rapid |
| 7 | surprise | {3,12} | fast blink |
| 8 | curious | {3,0,15,26} | |
| 9 | confused | {14,5,23} | |
| 10 | sad | {4,13,22} | the "/\" pout lives here |
| 11 | playful | {2,17,11} | |
| 12 | celebrate | {2,17,20} | |

Rendering: full-frame `Arduino_Canvas` buffer pushed atomically per frame
(kills SPI flicker), smoothstep morph ~450 ms, blink ≈120 ms close /
60 ms hold / 180 ms open, gaze ±14 px.
