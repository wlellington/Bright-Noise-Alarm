---
id: 20261121-1455-encoder-and-main-menu-implementation
type: report
title: Encoder and main menu implementation
author: Copilot
status: active
created: "2026-11-21 14:55"
branch: feature/encoder-main-menu
inputs: ["20261121-1450-encoder-navigation-and-main-menu", "../(OLD)Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.cpp", "../(OLD)Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.hpp", "REQ-001", "REQ-003", "REQ-019", "REQ-020", "REQ-034", "REQ-050"]
confidence: medium
summary: "User confirmed menu scrolling, category entry/return, and Back navigation work; the RTC display reads 16:45, which the user says is incorrect."
open_questions: ["Should the next implementation task cover local time/date setting under approved REQ-002? No RTC adjustment was made."]
next: "Agree the next scope for local clock setting; do not test the unwired front button."
signature:
  agent: user
  role: user
  signed: "2026-11-21 14:55"
  dust: 0.1.0
---
## What was done
Implemented input polling for the seesaw encoder and the board's external
front button, including rotation deltas and 30 ms button debounce. Replaced the
menu stub with a host-testable state machine for Clock, Alarms, Sleep, Light,
Settings and a final `Back` item; connected it to the firmware loop and display.
The home display shows valid RTC time in 24-hour form or `SET` when RTC time is
unavailable/reset. RTC or seesaw initialization no longer traps startup in an
infinite loop, and invalid RTC time is not replaced with the build timestamp.
Ported the legacy `format_menu_text`, `display_scrolling_text`, and four-digit
segment glyph map, with a medium default speed and scroll reset when the menu
item or screen changes. Changes remain uncommitted on `feature/encoder-main-menu`;
nothing was merged.

## Evidence
- `make -C tests test` — passed; covers categories, `Back`, wraparound,
  category selection, four-character scroll windows, padding and glyph mapping.
- `.arduino/bin/arduino-cli --config-file arduino-cli.yaml compile --fqbn rp2040:rp2040:adafruit_feather_prop_maker Firmware/Light_Clock_Firmware` — passed; 70,144 bytes program storage and 9,984 bytes RAM.
- Revised UF2 was uploaded with `.arduino/bin/arduino-cli --config-file arduino-cli.yaml upload --fqbn rp2040:rp2040:adafruit_feather_prop_maker --port UF2_Board --input-file /tmp/bright-noise-menu-upload/Light_Clock_Firmware.ino.uf2 Firmware/Light_Clock_Firmware`. The Feather mounted as `RPI-RP2` in bootloader mode, then re-enumerated as an Adafruit Feather RP2040 Prop-Maker.
- `git diff --check` — passed.
- `dust req check` — passed.
- `dust req coverage` finds test/code references for REQ-003 and REQ-050. REQ-001, REQ-019, REQ-020 and REQ-034 remain partial/in-progress.
- User said the earlier display/menu behavior looked good, but the front
  button is not wired. No detailed physical result was reported.
- The revised Back/scrolling build is on the device. Physical verification of
  those changes was performed by the user: menu rotation/scrolling, category
  placeholder entry/return, and final Back-to-home all passed. The home display
  reads `16:45`, which the user reports is incorrect. The firmware reads the
  RTC but does not yet offer local time/date setting (approved REQ-002); no
  RTC adjustment was made.

## Deviations from the task
No alarm/sleep/light behavior, settings persistence, potentiometer handling or
user-selectable 12/24-hour setting was added. This slice uses a temporary
24-hour home display and does not claim REQ-001, REQ-019, REQ-020 or REQ-034
satisfied. REQ-003 and REQ-050 are implemented but await demonstration.

## Left to do / found along the way
The menu's basic physical navigation passed. Local time/date setting remains
the next relevant gap; the front button cannot be tested until wired. The
peripheral diagnostic's remaining RGBW, speaker, knob and button checks are
separate and still pending.
