---
id: 20261121-1450-encoder-navigation-and-main-menu
type: task
title: Encoder navigation and main menu
author: Copilot
status: active
created: "2026-11-21 14:50"
branch: feature/encoder-main-menu
inputs: [".dust/artifacts/plans/20261121-1148-alarm-clock-rewrite-architecture.md", ".dust/artifacts/reviews/20261121-1150-alarm-clock-architecture-review.md", "../(OLD)Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.cpp", "../(OLD)Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.hpp", "REQ-001", "REQ-003", "REQ-019", "REQ-020", "REQ-034", "REQ-050"]
confidence: medium
summary: "User physically confirmed menu scrolling, category entry/return, and Back navigation; the RTC display reads an incorrect time, and time-setting is not implemented."
open_questions: ["Should the next implementation task cover local time/date setting under approved REQ-002? The RTC currently displays 16:45, which the user says is wrong."]
next: "Confirm the next scope for clock setting; keep the front button excluded until wired."
signature:
  agent: user
  role: user
  signed: "2026-11-21 14:50"
  dust: 0.1.0
---
## Objective
Implement the physical encoder and front-button path into a small, testable
main-menu state machine. Users can reach Clock, Alarms, Sleep, Light and
Settings, enter a category placeholder, and return through the menu to the
clock display. Keep the existing branch and legacy checkout untouched.

## Inputs to read
- Plan 20261121-1148: approved UI/input architecture, RTC validity and non-blocking startup.
- Review 20261121-1150: implementation slices require bounded criteria and independent verification.
- REQ-001, REQ-003, REQ-019, REQ-020, REQ-034 and REQ-050: clock home, menu categories, physical input, back behavior and invalid-clock indication.

## Branch
`feature/encoder-main-menu`, created from `dev`.

## Requirements
REQ-003 and REQ-034 are the navigation target. REQ-020 is partial (encoder
and front button only); potentiometers remain out of scope. REQ-001 is partial
clock-home display only; user-selectable 12/24-hour formatting remains future
work. REQ-019 and REQ-050 cover the invalid/unavailable RTC indicator only;
storage and audio faults remain out of scope. No requirement is marked
satisfied by this implementation task alone.

The user approved implementation scope with: "Start with getting the encoder
and main menu system working."
The user clarified that the front button is not yet wired and requested a
selectable `Back` entry at the end of each list as a temporary alternative.
The user also requested porting the original project's scrolling text and
seven-segment font style, and said the prior menu/display behavior looked
good apart from the unwired front button. The legacy checkout remains read-only.

## Recommended tier
Standard, difficulty 3/5: hardware input polling, menu state transitions,
display integration, host tests and a target compile; physical verification
remains a separate user-assisted gate.

## Acceptance criteria
- [x] Encoder rotation events open the menu from the clock display and advance
      or reverse selection through all five categories, including wraparound.
- [x] Encoder select and front-button back events navigate one level at a time
      (category to menu, then menu to clock) in the menu state machine.
- [ ] Valid RTC time remains visible at home; an unavailable or reset RTC
      displays `SET` and does not block input/menu startup or get silently
      adjusted to build time.
- [ ] Encoder, front-button and display initialization failures do not trap
      startup in an infinite retry loop; front button uses the board's defined
      external-button pin.
- [x] Host tests cover category reachability, selection wraparound, entering a
      category and both back transitions.
- [x] The firmware compiles for the configured RP2040 target. No device upload
      is attempted until the user confirms programming mode.
- [ ] After the user confirms programming mode, upload the revised firmware
      and physically verify encoder rotation, encoder select, the `Back` item,
      and the clock display; record remaining peripheral gaps. The front
      button is unwired and is not part of this device test.
- [x] The main menu's final entry is `Back`; selecting it returns to the clock.
- [x] Long menu labels use the legacy four-character seven-segment window,
      character mapping, medium default scroll speed and per-label reset.

## Physical test results
The user reported that menu scrolling/rotation, category entry/return, and
selecting the final `Back` item all passed. The home display reads `16:45`,
which the user says is incorrect. The menu/clock slice does not yet include a
local time/date setting flow (approved REQ-002); no RTC adjustment was made.

## Out of scope
Alarm behavior, sleep countdowns, light effects, settings persistence, 12/24
hour selection, potentiometers, speaker/LED output changes, and claiming
physical encoder/button behavior before on-device observation.

