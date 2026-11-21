---
id: 20261121-1227-peripheral-hardware-smoke-test
type: task
title: Peripheral hardware smoke test
author: Copilot
status: active
created: "2026-11-21 12:27"
branch: feature/peripheral-smoke-tests
inputs: ["20261121-1148-alarm-clock-rewrite-architecture", "20261121-1150-alarm-clock-architecture-review", "20261121-1108-legacy-feature-inventory-and-rewrite-decisions", "REQ-001", "REQ-017", "REQ-020", "REQ-021", "REQ-038", "REQ-039"]
confidence: medium
summary: "Standalone peripheral diagnostic sketch compiles for the Prop-Maker; on-device checks await programming mode."
open_questions: []
next: "Wait for the user to enter programming mode, then upload and record physical observations without claiming full REQ satisfaction."
signature:
  agent: user
  role: user
  signed: "2026-11-21 12:27"
  dust: 0.1.0
---
## Objective
Prove the connected Feather RP2040 Prop-Maker peripherals work before
implementing alarm/menu logic. Add a dedicated diagnostic sketch that does
not change the RTC or require unrelated SD/IMU hardware. Keep the existing
hardware-test sketch and legacy checkout untouched.

## Inputs to read
- Plan 20261121-1148 and review 20261121-1150: hardware verification is gated
  by conservative output tests and evidence; unknown maxima must not be treated
  as safe limits.
- Factfinding 20261121-1108 and the legacy firmware pin definitions: user
  identified the legacy checkout as the wiring source; it is read-only.
- REQ-001, REQ-017, REQ-020, REQ-021, REQ-038 and REQ-039: diagnostic evidence
  only; this task must not mark these requirements satisfied.

## Branch
`feature/peripheral-smoke-tests` from `dev`.

## Requirements
Partial diagnostic evidence only: REQ-001, REQ-017, REQ-020, REQ-021, REQ-038,
REQ-039. None is satisfied by this task. The user approved this first-stage
scope on 2026-11-21: "Start by proving out led, screen, button, knob, encoder,
and speaker functionalities first before we get into the compelx logic."

## Recommended tier
Standard, difficulty 3/5: several hardware interfaces, one focused test
sketch, and physical observations required.

## Acceptance criteria
- [x] The standalone test sketch compiles for the configured RP2040 board and
      does not initialize, block on, or modify RTC, SD or IMU state.
- [ ] The four-digit display test visibly exercises all digit positions,
      decimal points and colon at low brightness.
- [ ] Front (board pin 19) and encoder (seesaw GPIO 24) buttons report
      press/release; encoder address 0x36 reports changing position. Scan
      RP2040 ADC pins GPIO 26–29 so the two potentiometer inputs can be
      identified without assuming the legacy backup's non-ADC pins 8/9.
- [ ] An explicitly requested RGBW test exercises red, green, blue and white
      on each panel pixel, one pixel at a time, with global brightness 8/255,
      then turns the panel off and disables its external power. No maximum
      safe brightness is inferred.
- [ ] An explicitly requested 100 ms I2S test tone uses signed 16-bit samples
      capped at amplitude 512/32767; audio is silent at startup and stops
      after the bounded test.
- [ ] After the user enters programming mode and the sketch is flashed, record
      which physical peripherals passed or failed; do not claim full REQ
      satisfaction or verified output limits from this smoke test.

## Out of scope
Alarm/menu logic, production input handling, WAV/SD playback, full-range output
calibration, updating REQ statuses, flashing before the user is told and has
entered programming mode, and any changes to the legacy checkout.
