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
summary: "The peripheral diagnostic compiles; physical input/display and opt-in LED/speaker tests await the user entering programming mode."
open_questions: []
next: "After the user places the Feather in programming mode, upload the test sketch and record each peripheral's observed result."
signature:
  agent: user
  role: user
  signed: "2026-11-21 12:31"
  dust: 0.1.0
---
## Entries
### 2026-11-21: Peripheral diagnostic prepared
- Decisions: The user requested basic peripheral proof before complex alarm logic. Keep the existing hardware test and legacy checkout untouched; no upload before the user enters programming mode. LED and speaker tests are explicit serial commands at fixed low levels.
- Progress vs plan: Added and compiled a separate Prop-Maker sketch; see [build report](../reports/20261121-1231-peripheral-bring-up-sketch-build-report.md). The board is detected on `/dev/ttyACM1` in normal serial mode; no physical result is claimed.
- Surprises / risks: The legacy backup's GPIO 8/9 pot pins are not RP2040 ADC inputs, so the diagnostic scans GPIO 26–29 rather than assume the mapping. Output tests do not establish maximum safe levels or satisfy REQ-038.
- Standing instructions from the user: Preserve the legacy checkout. Tell the user when programming mode is needed.
