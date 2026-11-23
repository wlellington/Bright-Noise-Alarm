---
id: 20261121-1541-local-clock-editing-and-serial-command-implementat
type: report
title: Local clock editing and serial command implementation
author: Copilot
status: active
created: "2026-11-21 15:41"
branch: feature/encoder-main-menu
inputs: ["20261121-1528-local-clock-editing-and-serial-command"]
confidence: medium
summary: "Clock Save/Back, hour/minute flashing, and USB serial checks pass; wall-power-only startup is deferred."
open_questions: ["Wall-power-only startup remains untested until that power configuration is available."]
next: "Verify wall-power-only startup when available; keep broad serial requirements open."
signature:
  agent: user
  role: user
  signed: "2026-11-21 15:41"
  dust: 0.1.0
---
## What was done
Implemented Settings → Set Clock editing with Save/Back, sharing date/time
validation and the RTC-setting operation with the newline-delimited
`SET_TIME YYYY-MM-DD HH:MM:SS` serial command. Serial replies are bounded and
prioritized ahead of events; diagnostics are gated on USB CDC DTR during the
startup window.

Key files: [Menu_System.cpp](../../../Firmware/Light_Clock_Firmware/Menu_System.cpp),
[Clock_Command.cpp](../../../Firmware/Light_Clock_Firmware/Clock_Command.cpp),
[Serial_Command.cpp](../../../Firmware/Light_Clock_Firmware/Serial_Command.cpp),
[Serial_Diagnostics.cpp](../../../Firmware/Light_Clock_Firmware/Serial_Diagnostics.cpp),
[Time_Keeping.cpp](../../../Firmware/Light_Clock_Firmware/Time_Keeping.cpp).

## Evidence
- `make -C tests clean test` — both host test executables passed, including
  CRLF success, embedded-CR rejection, parser/error cases, and preserving a
  queued reply when partial input is reset.
- RP2040 compile succeeded for `rp2040:rp2040:adafruit_feather_prop_maker`:
  73,248 bytes program storage and 10,192 bytes global RAM.
- After the user confirmed programming mode, Arduino CLI wrote the 180,736-byte
  UF2 image to `RPI-RP2`; the board re-enumerated as an Adafruit Feather RP2040
  Prop-Maker on `/dev/ttyACM1`.
- The user physically confirmed clock Save and Back/cancel. The user found
  scrolling hour/minute labels difficult to read and requested a stable
  `HH:MM` view with the active pair flashing. The revision now passes
  `make -C tests clean test` and compiles to 73,432 bytes program storage and
  10,192 bytes global RAM. After the user confirmed programming mode, the
  180,736-byte UF2 was uploaded and the Feather re-enumerated as an Adafruit
  Feather RP2040 Prop-Maker.
- With a serial client open across startup, the Feather emitted
  `EVENT SERIAL_READY`. A malformed date returned `ERR SET_TIME
  INVALID_DATE_TIME`; `SET_TIME 2026-11-21 15:56:53` returned matching `OK`
  and `EVENT TIME_SET` lines; an unknown command returned
  `ERR UNKNOWN_COMMAND`. The successful command set the RTC to the host's
  current local time. Thirteen lower-priority startup logs were reported
  dropped, as permitted by the bounded event policy.
- After another reboot with no serial client open during startup, connecting
  late and sending an unknown command produced no response, confirming the
  DTR startup gate. A wall-power-only boot was not physically tested.
- The user physically confirmed that the latest hour/minute flashing display
  passed. The user cannot test wall-power-only startup at this time and asked
  to defer it.
- `git diff --check && dust req check` — `ok`.
- `dust req coverage` now traces REQ-002 to one test and two code references;
  it still reports unrelated project requirements without traceability.
- REQ-002 is marked `implemented`, not `satisfied`; hardware behavior has not
  yet been demonstrated.

## Deviations from the task
None in the implemented clock-setting slice. Broader serial control,
readouts, and alarm timing remain explicitly out of scope.

## Left to do / found along the way
Wall-power-only startup remains deferred. Serial command, no-DTR startup, and
the flashing hour/minute view were verified. Do not claim
REQ-024/026/027/041/056/057 fully satisfied or the broader serial design review
blocker resolved.
