# 知知球 Zhiji Ball — ESP32 Pendant with a GrokBot Face

A 50 mm 3D-printed orb pendant: **XIAO ESP32-S3 + GC9A01 240×240 round display**,
running the ESP32 iPhone Agent firmware with a custom face layer ported from
**GrokBot**. BLE to an iOS agent app, Wi-Fi AP for video/HID, and a face that
blinks, glances in five directions, and plays 13 emotion states.

> Built on [ESP32 iPhone Agent](https://github.com/onehumanbeing/esp32-iphone-agent)
> by Henry Yuan (MIT). Face geometry ported from
> [GrokBot](https://github.com/nasawz/GrokBot) via
> [grokbot-web](https://github.com/Coiggahou2002/grokbot-web) (BSD-3-Clause).
> Attribution details in `NOTICE`.

## What's ours

- **`hardware/hid_bridge/face_display.{h,cpp}` + `face_expr.h`** — the face layer:
  25 GrokBot expressions (each eye a 48-point polygon ring, face-space ~228.541)
  + 3 mirrored glance variants, rendered point-by-point with smoothstep morphing,
  full-frame canvas (no flicker), blink timeline, gaze pools.
- **`pendant/`** — the 50 mm shell STL (the exact file we printed), wiring table,
  no-button flashing recipe, expression/state reference.
- BLE `face N` command — switch 13 emotion states remotely from any BLE client.

## Architecture (who runs what)

```
iPhone agent app ──BLE GATT──▶ XIAO ESP32-S3
 (agent loop, LLM calls,    (display task: face FSM, morphing,
  screenshot relay, HID)      BLE cmd/notify, Wi-Fi AP + video,
                              USB HID to phone)
        ▲
        └── board hotspot 192.168.4.1 for video/OTA data plane
```

The ESP32 owns: face state machine + rendering, BLE control plane, Wi-Fi AP
data plane (video TCP:7201, OTA TCP:7300, per-boot random token), USB HID.
The phone owns: the agent loop, model calls, task state. Face states follow
agent lifecycle (run→WORK, finished→HAPPY, error→ERROR, disconnect→SLEEP),
or `face N` over the BLE command characteristic.

## The face

- 13 states: `idle, work, happy, error, sleep, think, excite, surprise,
  curious, confused, sad, playful, celebrate`
- Each state has its own expression pool + cadence (see `pendant/FACE.md`)
- White capsule eyes on black; the rainbow effect comes from the glass cover,
  not firmware

## Hardware

| Part | Note |
|---|---|
| Seeed XIAO ESP32-S3 | 8 MB PSRAM build flags in `pendant/FLASHING.md` |
| GC9A01 240×240 round TFT | 7-wire SPI, ~40 mm bare panel |
| 50 mm printed shell | `pendant/zhiji-ball-50mm-shell-R1.stl` |
| 7× 30 AWG / dupont wires | wiring map in `pendant/WIRING.md` |

## License

MIT (upstream license retained in `LICENSE`). GrokBot expression data used
under BSD-3-Clause — see `NOTICE`. Public demos/videos: please credit both
upstream projects.

*Poke — if you're reading this, you're in the credits now. Deal's a deal. 🤝*
