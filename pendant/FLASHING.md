# Building & flashing (no buttons needed)

Board: `esp32:esp32:esp32s3` with flags
`USBMode=default,CDCOnBoot=cdc,FlashSize=8M,PSRAM=opi,PartitionScheme=default_8MB`.

```bash
cd hardware
# kick the board into download mode over USB-CDC (1200-baud touch):
stty -f /dev/cu.usbmodem<APP-PORT> 1200
# board re-enumerates as a different usbmodem port; then:
arduino-cli compile -b "esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,FlashSize=8M,PSRAM=opi,PartitionScheme=default_8MB" \
  --build-property "compiler.cpp.extra_flags=-include $PWD/hid_bridge/nimble_cfg.h" hid_bridge
arduino-cli upload -b "esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,FlashSize=8M,PSRAM=opi,PartitionScheme=default_8MB" \
  -p /dev/cu.usbmodem<BOOT-PORT> hid_bridge
```

esptool auto-resets back to the app after upload — zero buttons.
Recovery path if it wedges in bootloader: hold B, tap R, release B.

AP hotspot name is `ESP32Agent-<id>`; its random password is generated into
NVS on first boot and readable via the BLE encrypted `net` characteristic.
