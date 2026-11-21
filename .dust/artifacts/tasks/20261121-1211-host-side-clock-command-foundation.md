---
id: 20261121-1211-host-side-clock-command-foundation
type: task
title: Host-side clock command foundation
author: Dust
status: blocked
created: "2026-11-21 12:11"
branch: feature/clock-command-foundation
inputs: [".dust/artifacts/plans/20261121-1148-alarm-clock-rewrite-architecture.md", ".dust/artifacts/reviews/20261121-1150-alarm-clock-architecture-review.md", ".dust/requirements.md"]
confidence: medium
summary: "Proposed host-only clock-command foundation; define shared date/time validation and test save/cancel against a fake RTC. Blocked until the baseline commit and first-slice review are resolved."
open_questions:
  - "Has the user approved this bounded scope and its acceptance criteria?"
  - "Has the baseline been committed and the first slice independently reviewed?"
next: "Obtain scope approval and review, establish the clean dev baseline, then assign the task to the Builder."
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

## Recommended tier
Standard, difficulty 3/5: a bounded application boundary, validation, fake
dependency and tests with design judgment but no physical hardware.

## Acceptance criteria
- [ ] A single application operation accepts valid calendar date/time values,
      rejects invalid values without changing the fake RTC, and reports success
      or error explicitly.
- [ ] Tests cover month boundaries, leap-year validity, invalid clock values
      and propagation of a fake RTC write failure without reporting success.
- [ ] Separate test callers representing local and serial actions invoke the
      same operation and observe identical validation and results.
- [ ] Editing a candidate value and canceling it leaves the fake RTC unchanged;
      saving a valid candidate updates it exactly once.
- [ ] Host tests pass and the implementation compiles for the configured
      RP2040 board without flashing the device.

## Out of scope
Actual menus, USB protocol, RTC hardware integration, persistence, alarms,
physical outputs, and claiming either requirement satisfied.

## Dependencies
The current `dev` baseline must be committed with permission. The first slice
must pass the applicable independent plan-review gate and the scope and
acceptance criteria need explicit user approval before staffing.
