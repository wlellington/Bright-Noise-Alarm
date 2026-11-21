---
id: 20261121-1231-peripheral-bring-up-sketch-build-report
type: report
title: Peripheral bring-up sketch build report
author: Copilot
status: active
created: "2026-11-21 12:31"
branch: feature/peripheral-smoke-tests
inputs: ["20261121-1227-peripheral-hardware-smoke-test", "20261121-1148-alarm-clock-rewrite-architecture", "REQ-020", "REQ-021", "REQ-038"]
confidence: medium
summary: "Diagnostic firmware is uploaded and the user confirmed all display segments illuminate; encoder/buttons, RGBW color, and speaker confirmation remain pending."
open_questions: []
next: "Obtain the user's visual/audio observations and input-control results before claiming peripheral functionality."
signature:
  agent: user
  role: user
  signed: "2026-11-21 12:31"
  dust: 0.1.0
---
## What was done
Added `Firmware/Peripheral_Bringup_Test/Peripheral_Bringup_Test.ino`. It
tests display, buttons, encoder and ADC telemetry without RTC/SD/IMU setup;
LED and speaker outputs require explicit serial commands and use fixed low
levels. Existing hardware-test sketch and legacy checkout were left untouched.

## Evidence
- `.arduino/bin/arduino-cli --config-file arduino-cli.yaml compile --fqbn rp2040:rp2040:adafruit_feather_prop_maker Firmware/Peripheral_Bringup_Test` — successful; 80,588 bytes program storage and 10,536 bytes RAM.
- `arduino-cli board list` identified the bootloader as `UF2_Board`; UF2 upload wrote 196,096 bytes to `/run/media/contact/RPI-RP2/NEW.UF2`. The board returned as an Adafruit Feather RP2040 Prop-Maker on `/dev/ttyACM1`.
- USB serial acknowledged `d`, `l`, and `s`: `Display pattern shown`, `LED test complete; panel off`, and `Speaker test complete`. The speaker routine used 440 Hz for 100 ms at amplitude 512/32767; the LED routine scanned one pixel at a time at brightness 8/255.
- The user visually confirmed that all display segments are on.
- No human visual/audio observation was available, and no button, encoder, or knob was physically operated during telemetry capture. Serial routine completion is not proof of physical output.

## Deviations from the task
No change to the requested order. The test scans RP2040 ADC pins 26–29 rather
than assuming the legacy backup's GPIO 8/9 are valid analog inputs.

## Left to do / found along the way
The user must confirm the RGBW channel order and audible tone, and operate the
front/encoder buttons, encoder rotation and both knobs while observing serial
telemetry. The display is visually confirmed. This smoke test does not
establish maximum safe output limits or satisfy REQ-038.
