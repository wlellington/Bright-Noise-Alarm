---
id: 20261121-1541-local-clock-editing-and-serial-command
type: journal
title: Local clock editing and serial command
author: Copilot
status: active
created: "2026-11-21 15:41"
branch: feature/encoder-main-menu
inputs: ["20261121-1528-local-clock-editing-and-serial-command"]
confidence: medium
summary: "Clock Save/Back, hour/minute flashing, and USB serial commands pass; wall-power-only startup is deferred."
open_questions: ["Wall-power-only startup remains untested until the configuration is available."]
next: "Verify wall-power-only startup when available."
signature:
  agent: user
  role: user
  signed: "2026-11-21 15:41"
  dust: 0.1.0
---
## Entries
### 2026-11-21: Clock editor and SET_TIME implementation
- Decisions: Continued on `feature/encoder-main-menu` to preserve the existing dirty menu work. Implemented the user-approved Settings → Set Clock flow and `SET_TIME YYYY-MM-DD HH:MM:SS` command through one shared clock operation.
- Progress vs plan: Host tests, RP2040 compilation, `git diff --check`, and `dust req check` pass. After the user confirmed programming mode, the 180,736-byte UF2 was uploaded and the Feather re-enumerated. REQ-002 is implemented but not demonstrated. See the [implementation report](../reports/20261121-1541-local-clock-editing-and-serial-command-implementat.md).
- Surprises / risks: Device tests are still needed for local save/cancel, serial results, and startup diagnostics gating; the broader serial design finding remains open.
- Standing instructions from the user: Preserve the legacy checkout. Tell the user when programming mode is needed; do not merge without the required gates and user approval.

### 2026-11-21: Hour/minute display readability feedback
- Decisions: At the user's request, replace scrolling field labels while editing hour/minute with `HH:MM` digits and blink the active pair.
- Progress vs plan: The user reported local Save and Back/cancel passed. Added the display criterion to REQ-002 and the active task; host tests and RP2040 compilation pass. After programming-mode confirmation, the revised 180,736-byte UF2 was uploaded and the Feather re-enumerated.
- Surprises / risks: The flashing display behavior itself still needs physical verification. Serial set-time and wall-power startup checks remain pending.
- Standing instructions from the user: Preserve the legacy checkout. Tell the user when programming mode is needed; do not merge without the required gates and user approval.

### 2026-11-21: Agent-run serial tests
- Decisions: The user directed the agent to exercise the serial API itself. To enable the startup-gated API, the monitor was held open across a firmware reset.
- Progress vs plan: Device returned `ERR SET_TIME INVALID_DATE_TIME`, `OK SET_TIME 2026-11-21 15:56:53`, matching `EVENT TIME_SET`, and `ERR UNKNOWN_COMMAND`. The valid command set the RTC to host local time. A no-client startup followed by a late command produced no response, confirming the DTR gate.
- Surprises / risks: Thirteen lower-priority startup events were dropped, as permitted. Wall-power-only startup and visual confirmation of the new flashing hour/minute display remain pending.
- Standing instructions from the user: Preserve the legacy checkout. Tell the user when programming mode is needed; do not merge without the required gates and user approval.

### 2026-11-21: Display readability confirmed
- Decisions: The user confirmed the updated flashing hour/minute display passed. Wall-power-only startup is deferred because the user cannot test that power configuration now.
- Progress vs plan: Clock Save/Back, the flashing display, USB serial set-time/errors, and late-connect startup gating have all been verified. See the [implementation report](../reports/20261121-1541-local-clock-editing-and-serial-command-implementat.md).
- Surprises / risks: Wall-power-only diagnostics behavior remains untested; broad serial requirements and the review blocker remain open.
- Standing instructions from the user: Preserve the legacy checkout. Tell the user when programming mode is needed; do not merge without the required gates and user approval.
