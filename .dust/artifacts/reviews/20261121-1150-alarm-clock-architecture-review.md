---
id: 20261121-1150-alarm-clock-architecture-review
type: review
title: Alarm clock architecture review
author: Dust
status: done
created: "2026-11-21 11:50"
branch: dev
inputs: [".dust/artifacts/plans/20261121-1148-alarm-clock-rewrite-architecture.md", ".dust/requirements.md"]
confidence: medium
summary: "Review findings remain for alarm recovery, requirement-level task criteria and the uncommitted baseline. Hardware verification gates affected output and timing work, not host-only foundation work."
open_questions:
  - "How should occurrence state survive failed writes, dismissal, and power loss?"
  - "What are the verified safe hardware limits and timing bounds?"
next: "Resolve findings applicable to the first implementation slice, establish a clean baseline, and independently review that slice before staffing."
signature:
  agent: user
  role: user
  signed: "2026-11-21 11:50"
  dust: 0.1.0
---
## Scope
Reviewed plan 20261121-1148 against the project brief and selected approved
requirements for scheduling, recovery, controls, diagnostics and safety.
This is a pre-implementation document review, not hardware validation.

## Verdict
The user approved proceeding with the architecture on 2026-11-21. The findings
below still need resolution for affected work; the plan now distinguishes
hardware-output gates from host-only foundation work.

## Findings
| # | Severity | Finding | Evidence / location | Question or fix |
|---|---|---|---|---|
| 1 | Blocking | Alarm recovery and arbitration are not specified enough to implement reliably. Dismissal, one-shot disablement, persistence failures, unequal ramp start times, overlapping alarms, clock changes and restart can change which alarm plays or replays. | Plan Design and Risks; REQ-008, REQ-010, REQ-030, REQ-037, REQ-049 | Architect and owner/Analyst define occurrence lifecycle, durable transitions, fault guarantees and conflict cases with concrete examples and tests. |
| 2 | Blocking for output and timing work | The 80% caps depend on verified physical limits; wiring, panel channel order, supported audio and loaded timing remain unverified. A compiling hardware-test sketch is not physical evidence. | Plan step 0; REQ-038, REQ-046 | Record safe measurement procedure and hardware evidence or explicitly gate affected work; never assume library maxima are verified safe limits. |
| 3 | Blocking for staffing | Steps group many requirements, but individual slices do not yet identify their exact REQs, testable acceptance criteria, dependencies and ownership. Several serial capabilities are distributed across later steps without explicit integration criteria. | Plan Steps and Requirement coverage; REQ-024–029, REQ-041–045 | Produce bounded tasks with exact REQ IDs, criteria, dependencies, verification and serial-to-hardware integration checks. |
| 4 | Blocking for feature-branch execution | The new workspace, including requirements and project scaffold, is uncommitted on dev; a clean feature branch would not contain that work. The previous scaffold commit was denied. | `git status --short --branch`; plan step branches | Preserve all work and obtain authorization to commit the reviewed baseline before starting feature branches; do not bypass the git-flow checks. |

## Checked and sound
The plan explicitly shares validation between menu and serial, models RTC and
monotonic time separately, calls for bounded I/O, preserves the legacy checkout,
and separates commanded LED frames from physical verification. REQ-022 is
dropped and REQ-030 requires both ramps to reach peak at alarm time. `dust req
check` passes; this checks requirement format, not design completeness.

2026-11-21 update: finding 2 does not block unrelated host-only work. The plan
now explicitly gates physical output operation and hardware-safety or timing
claims on verification. Findings 1, 3 and 4 remain open where applicable.
