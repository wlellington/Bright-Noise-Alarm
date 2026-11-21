---
id: 20261121-1211-host-side-clock-command-foundation
type: task
title: Host-side clock command foundation
author: Dust
status: review
created: "2026-11-21 12:11"
branch: feature/clock-command-foundation
inputs: [".dust/artifacts/plans/20261121-1148-alarm-clock-rewrite-architecture.md", ".dust/artifacts/reviews/20261121-1150-alarm-clock-architecture-review.md", ".dust/requirements.md"]
confidence: medium
summary: "Shared host-testable clock validation/write operation is implemented and target compilation passes. Awaiting independent Tester verification."
open_questions: []
next: "Tester verifies the implementation report and acceptance criteria on feature/clock-command-foundation; do not merge without user approval."
signature:
  agent: user
  role: user
  signed: "2026-11-21 12:11"
  dust: 0.1.0
---
## Objective
Introduce the smallest host-testable application operation for validating and
setting a clock, shared by eventual local and serial interfaces. Do not drive
the physical RTC or implement either interface in this slice.

## Inputs to read
- Plan 20261121-1148: application command ownership and hardware boundaries.
- Requirements REQ-002 and REQ-025: clock editing and shared validation.
- Review 20261121-1150: requirements for bounded tasks and Git baseline.

## Branch
`feature/clock-command-foundation` from clean `dev`; the branch does not yet
exist. One Builder owns it.

## Requirements
REQ-002 and REQ-025, partial foundation only. Neither requirement is satisfied
by fake-RTC host tests; local and serial integration and target verification
remain future work.
User approved proceeding with this first implementation scope and criteria on
2026-11-21: "commit everything and start work" and "I give approval for
whatever you need to make progress."

## Recommended tier
Standard, difficulty 3/5: a bounded application boundary, validation, fake
dependency and tests with design judgment but no physical hardware.

## Acceptance criteria
- [x] A single application operation accepts valid calendar date/time values,
      rejects invalid values without changing the fake RTC, and reports success
      or error explicitly.
- [x] Tests cover month boundaries, leap-year validity, invalid clock values
      and propagation of a fake RTC write failure without reporting success.
- [x] Separate test callers representing local and serial actions invoke the
      same operation and observe identical validation and results.
- [x] Editing a candidate value and canceling it leaves the fake RTC unchanged;
      saving a valid candidate updates it exactly once.
- [x] Host tests pass and the implementation compiles for the configured
      RP2040 board without flashing the device.

## Out of scope
Actual menus, USB protocol, RTC hardware integration, persistence, alarms,
physical outputs, and claiming either requirement satisfied.

## Dependencies
Git baseline commit `258e0f3` contains the requirements and plan. Commit this
task approval and the first-slice review update on `dev` before branching.
The first-slice review is recorded in review 20261121-1150; unresolved alarm,
serial and physical-output findings remain gates for later work.
