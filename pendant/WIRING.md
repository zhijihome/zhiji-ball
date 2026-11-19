# Wiring — XIAO ESP32-S3 ↔ GC9A01 (7-wire)

| Display pin (module silk) | XIAO GPIO | Wire color (ours) |
|---|---|---|
| GND | GND | black |
| VCC (3V3) | 3V3 | red |
| SCL / SCK | GPIO7 (D8) | white |
| SDA / MOSI | GPIO9 (D10) | yellow |
| DC | GPIO4 | per harness |
| RES / RST | GPIO5 | per harness |
| CS | GPIO3 | per harness |
| BLK / BL | GPIO6 | per harness |

Pin defines live in `hardware/hid_bridge/face_display.cpp` (`PIN_*`).
Power: any USB-C supply; BLE/Wi-Fi are wireless — plugging into the iPhone
only adds USB HID injection.
