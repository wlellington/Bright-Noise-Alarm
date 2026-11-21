---
id: 20261121-1231-peripheral-bring-up-sketch-prepared
type: journal
title: Peripheral bring-up sketch prepared
author: Copilot
status: active
created: "2026-11-21 12:31"
branch: feature/peripheral-smoke-tests
inputs: ["20261121-1227-peripheral-hardware-smoke-test", "20261121-1231-peripheral-bring-up-sketch-build-report", "20261121-1148-alarm-clock-rewrite-architecture"]
confidence: medium
summary: "Diagnostic firmware is uploaded and the user confirmed all display segments illuminate; other physical and input checks remain pending."
open_questions: []
next: "Collect the user's visual/audio observations and control-input results before claiming peripherals work."
signature:
  agent: user
  role: user
  signed: "2026-11-21 12:31"
  dust: 0.1.0
---
## Entries
### 2026-11-21: Display confirmed
- Decisions: Treat the user's confirmation that all display segments illuminate as evidence for the display hardware check only.
- Progress vs plan: The display criterion is verified by user observation. Encoder, buttons, potentiometers, RGBW color order, and audible speaker output remain unverified.
- Surprises / risks: The encoder telemetry session ended without control events; no claim is made that the encoder or buttons work.
- Standing instructions from the user: Start the next implementation slice with the encoder and main menu system.
### 2026-11-21: Diagnostic firmware uploaded
- Decisions: The user confirmed bootloader mode and requested transfer. Uploaded only the standalone peripheral diagnostic; it keeps outputs off at startup and limits test levels.
- Progress vs plan: UF2 upload completed to `/run/media/contact/RPI-RP2`; the Feather reappeared at `/dev/ttyACM1`. Serial acknowledged display, RGBW and speaker tests, but physical output and control behavior are not independently observed.
- Surprises / risks: The user was unavailable to report visual/audio results or manipulate controls during telemetry capture. Do not mark requirements satisfied based on command acknowledgements.
- Standing instructions from the user: Preserve the legacy checkout. Do not begin complex alarm logic until the requested peripheral checks are established.
### 2026-11-21: Peripheral diagnostic prepared
- Decisions: The user requested basic peripheral proof before complex alarm logic. Keep the existing hardware test and legacy checkout untouched; no upload before the user enters programming mode. LED and speaker tests are explicit serial commands at fixed low levels.
- Progress vs plan: Added and compiled a separate Prop-Maker sketch; see [build report](../reports/20261121-1231-peripheral-bring-up-sketch-build-report.md). The board is detected on `/dev/ttyACM1` in normal serial mode; no physical result is claimed.
- Surprises / risks: The legacy backup's GPIO 8/9 pot pins are not RP2040 ADC inputs, so the diagnostic scans GPIO 26–29 rather than assume the mapping. Output tests do not establish maximum safe levels or satisfy REQ-038.
- Standing instructions from the user: Preserve the legacy checkout. Tell the user when programming mode is needed.
