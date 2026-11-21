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
summary: "Standalone peripheral diagnostic compiles for the Prop-Maker; no firmware has been uploaded and physical results remain pending."
open_questions: []
next: "Await user confirmation that the Feather is in programming mode, then upload and run the physical smoke tests."
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
- `arduino-cli board list` identified `/dev/ttyACM1` as an Adafruit Feather RP2040 Prop-Maker in normal serial mode. No upload or physical tests were performed.

## Deviations from the task
No change to the requested order. The test scans RP2040 ADC pins 26–29 rather
than assuming the legacy backup's GPIO 8/9 are valid analog inputs.

## Left to do / found along the way
User must place the device in programming mode before upload. Physical checks
of the RGBW channel order and speaker output remain pending; this smoke test
does not establish maximum safe output limits or satisfy REQ-038.
