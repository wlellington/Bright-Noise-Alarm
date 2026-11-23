---
id: 20261121-1528-local-clock-editing-and-serial-command
type: task
title: Local clock editing and serial command
author: Copilot
status: active
created: "2026-11-21 15:28"
branch: feature/encoder-main-menu
inputs: ["20261121-1148-alarm-clock-rewrite-architecture.md", "20261121-1150-alarm-clock-architecture-review.md", "20261121-1152-architecture-self-review-addendum.md", "20261121-1211-host-side-clock-command-foundation.md", "20261121-1450-encoder-navigation-and-main-menu.md", "REQ-002", "REQ-024", "REQ-025", "REQ-026", "REQ-027", "REQ-041", "REQ-056", "REQ-057"]
confidence: medium
summary: "Clock Save/Back, hour/minute flashing, and serial commands pass device checks; wall-power-only startup is deferred."
open_questions: ["Wall-power-only startup remains untested until the user can provide that power configuration."]
next: "When wall power without a host is available, verify diagnostics remain disabled; keep broader serial requirements open."
signature:
  agent: user
  role: user
  signed: "2026-11-21 15:28"
  dust: 0.1.0
---
## Objective
Provide a usable local date/time editor and a serial command for setting the
same RTC through the existing shared clock application operation. The user
observed that the RTC currently displays an incorrect `16:45`.

## Inputs to read
- Approved architecture and review artifacts: shared application commands,
  serial policy, and the still-open general serial-diagnostics finding.
- Existing clock command foundation task/report: date/time validation and
  fake-RTC operation on `feature/clock-command-foundation`.
- Current menu implementation/task and physical test report: encoder navigation
  works; the Settings page and local clock editor remain to be implemented.
- REQ-002, REQ-024–027, REQ-041, REQ-056 and REQ-057.

## Branch
`feature/encoder-main-menu` (continue on the already-dirty feature branch so
the uncommitted menu implementation and current task stay together).

## Requirements
REQ-002 (local time/date setting); REQ-024 and REQ-025 (serial set-time command
through shared application behavior); REQ-026 and REQ-027 (explicit results
and timestamped time-change event); REQ-041 (the clock setting is serial
configurable); REQ-056 and REQ-057 (startup gate and bounded diagnostics).
This task implements only the clock-setting slice of REQ-024/041; it does not
claim those broad requirements satisfied.

The user approved this scope on 2026-11-21: "start requirement 002, but also
add a command to the serial API that lets you set it from the serial link as
well. We will need a command like this for every action,setting etc going
forward." The user selected the encoder editor flow, the command syntax
`SET_TIME YYYY-MM-DD HH:MM:SS`, and limiting this slice to time/date setting.

## Recommended tier
Standard: date/time editing, a bounded serial command parser, shared RTC
updates, hardware integration and focused host tests.

## Acceptance criteria
- [x] Settings offers a `Set Clock` flow for year, month, day, hour, minute,
      and second; encoder rotation edits the selected value and click advances.
- [x] A candidate is not written until explicit Save; Back/cancel leaves the
      RTC unchanged, including after edits to the final field.
- [ ] While editing hour or minute, the display shows `HH:MM` and flashes the
      active numeric pair instead of scrolling the field label.
- [x] Saving through the menu invokes the shared clock command exactly once;
      invalid values or RTC write failures are reported without false success.
- [x] A newline-delimited `SET_TIME YYYY-MM-DD HH:MM:SS` command invokes that
      same operation and returns an explicit line-framed success or error with
      the resulting time or failure reason.
- [x] Malformed, overlong, unknown and invalid-date commands do not change the
      RTC; serial input and output are bounded and service never waits in a
      blocking write loop. A command is accepted only when its result can be
      queued; if USB disconnects after execution, the client can safely retry
      the absolute set-time command.
- [x] A successful clock change can produce a timestamped event; event output
      is lower priority than command responses and may be dropped under load
      as allowed by REQ-057.
- [x] Host tests cover local/serial parity, date boundaries, parser errors,
      cancellation, save-once behavior, RTC write failures and bounded
      response handling. Firmware compiles for the RP2040 target.
- [x] Physical local Save and Back/cancel preserve the expected clock behavior.
- [x] USB serial tests return the expected invalid-date, success, time-change
      event, and unknown-command responses.
- [x] Startup without an active serial client leaves diagnostics disabled;
      connecting later and sending a command produces no response.
- [x] User physically confirms the latest hour/minute flashing display.
- [ ] Verify wall-power-only startup with no host connection when that power
      configuration is available. Do not claim the full serial diagnostics or
      alarm-timing review blocker resolved.

## Out of scope
Alarm commands, other serial commands/settings, the full device-state and
64-LED readouts, asynchronous diagnostics beyond the clock-change event, and
any claim that the broader serial queues/readouts or alarm timing are solved.

## Workflow
The user chose iterative development on 2026-11-21 and asked not to let a
separate plan-agent cycle stop implementation. Address the command-specific
serial risks in this slice's bounded parser, response handling and tests; keep
the broader serial-design finding open for later slices. This decision does
not authorize merging or claiming broad serial requirements satisfied.

## Physical feedback
The user can test Settings → Set Clock, edit/save, cancel/Back, and send
`SET_TIME YYYY-MM-DD HH:MM:SS` over a serial client opened before device
startup. Record actual observed time/result text here and in the journal.

The user physically confirmed local Save and Back/cancel work. They reported
that scrolling text is hard to read while setting hour/minute and requested a
stable numeric time display with the active pair flashing. Serial command and
wall-power-only startup remains pending. The user physically confirmed the
latest hour/minute flashing display passed. The latest display code passed host
tests and RP2040 compilation, was uploaded after programming-mode confirmation,
and re-enumerated as an Adafruit Feather RP2040 Prop-Maker. The agent's USB
serial checks passed for invalid date, successful set-time/event, unknown
command, and no-DTR startup behavior.
