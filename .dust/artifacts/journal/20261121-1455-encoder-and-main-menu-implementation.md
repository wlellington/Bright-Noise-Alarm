---
id: 20261121-1455-encoder-and-main-menu-implementation
type: journal
title: Encoder and main menu implementation
author: Copilot
status: active
created: "2026-11-21 14:55"
branch: feature/encoder-main-menu
inputs: ["20261121-1450-encoder-navigation-and-main-menu", "20261121-1455-encoder-and-main-menu-implementation", "20261121-1148-alarm-clock-rewrite-architecture"]
confidence: medium
summary: "Encoder control history; latest user decision defers the two potentiometers until soldered. REQ-039 records soft pickup of current brightness/volume followed by independent 0-200% scaling with existing safety caps. No firmware change for this deferral."
open_questions: ["Which ADC GPIO is wired to brightness and which to volume after soldering?"]
next: "After the user confirms soldering and ADC GPIO mapping, implement and verify REQ-039 soft pickup/scaling. Do not use the placeholder pot pin definitions."
signature:
  agent: user
  role: user
  signed: "2026-11-21 14:55"
  dust: 0.1.0
---
## Entries
### 2026-11-22: Potentiometer takeover deferred (Copilot)
- Decision by user: These are two potentiometers, not rotary encoders, and are not wired yet. Save implementation until after soldering.
- Agreed behavior: One knob controls brightness and one volume during alarms/sleep. Neither changes the output by default. Movement must catch the current instantaneous output before taking control, then independently scale the ongoing output from 0-200% without defeating fade timing or safety caps.
- Progress: Recorded the decision in [REQ-039](../../requirements.md) and the [project brief](../../project.md). No firmware changes or uploads were made for this request.
- Next: Confirm ADC GPIO mapping after soldering; the current pot pin constants are unverified placeholders, including an SD conflict.

### 2026-11-21: Physical menu test results
- Decisions: Treat menu scrolling, category entry/return, and Back navigation as physically verified based on the user's report. Do not adjust the RTC without an agreed target time/date.
- Progress vs plan: User reported all tested menu interactions work as expected. The home display shows `16:45`, which the user says is wrong. The implementation reads the RTC but does not yet provide local time/date editing under approved REQ-002.
- Surprises / risks: Displayed time may be a valid but incorrectly set RTC value; no RTC adjustment or setting UI was performed.
- Standing instructions from the user: Preserve the legacy checkout. Tell the user when programming mode is needed.
### 2026-11-21: Revised menu firmware uploaded
- Decisions: Proceeded after the user said they were ready; Arduino CLI detected the Feather in UF2 bootloader mode. The unwired front button remains excluded from physical testing.
- Progress vs plan: Built and uploaded the revised Back/scrolling firmware. The mounted drive reported `RPI-RP2`; after upload, the board re-enumerated as an Adafruit Feather RP2040 Prop-Maker. Physical results have not yet been received.
- Surprises / risks: No observed display or encoder behavior is claimed; the firmware is uploaded but not physically verified.
- Standing instructions from the user: Preserve the legacy checkout. Tell the user when programming mode is needed.
### 2026-11-21: Back navigation and legacy display style
- Decisions: The front button is unwired; add a selectable `Back` entry at the end of the current list. Port the legacy four-character scroll window, custom seven-segment glyph mapping and `display_scrolling_text` naming. Keep the legacy checkout read-only.
- Progress vs plan: Added the final Back menu item and long-label scrolling. Host tests and RP2040 compilation pass; see the updated [implementation report](../reports/20261121-1455-encoder-and-main-menu-implementation.md).
- Surprises / risks: The previously uploaded firmware predates these changes. The revised build needs another upload and physical check; the user said the earlier menu/display behavior looked good apart from the unwired front button.
- Standing instructions from the user: Preserve the legacy checkout. Tell the user when programming mode is needed.
### 2026-11-21: Encoder and menu foundation implemented
- Decisions: Keep the menu state machine host-testable; front-button back navigates one level at a time. Display valid RTC time in temporary 24-hour form and show `SET` for invalid time; never replace RTC time with the build timestamp.
- Progress vs plan: Implemented input polling, menu categories and display integration on `feature/encoder-main-menu`. Host navigation tests and the configured RP2040 compile pass; see [implementation report](../reports/20261121-1455-encoder-and-main-menu-implementation.md).
- Surprises / risks: Encoder, front-button and display behavior have not been physically verified. The user confirmed programming mode and the firmware was uploaded; physical observations remain pending.
- Standing instructions from the user: Preserve the legacy checkout. Tell the user when programming mode is needed.
