---
id: 20261121-1152-architecture-self-review-addendum
type: review
title: Architecture self-review addendum
author: Copilot
status: done
created: "2026-11-21 11:52"
branch: dev
inputs: [".dust/artifacts/plans/20261121-1148-alarm-clock-rewrite-architecture.md", ".dust/artifacts/reviews/20261121-1150-alarm-clock-architecture-review.md", ".dust/requirements.md"]
confidence: medium
summary: "Self-review supplements the existing architecture review. The user specified snooze controls and startup-gated serial diagnostics with permissible dropped messages. Requirement reconciliation, USB detection, and earlier blockers remain open."
open_questions:
  - "How is an active serial connection detected at power-up, and how are existing serial requirements reconciled with gating and permissible dropped messages?"
next: "Resolve these findings and the earlier review's blockers in the plan; obtain an independent plan review and user approval before implementation."
signature:
  agent: user
  role: user
  signed: "2026-11-21 11:52"
  dust: 0.1.0
---
## Scope

Self-review by the author of plan 20261121-1148, against the approved
requirements and review 20261121-1150. This supplements that review; it is not
an independent review and does not replace the required independent gate.
No firmware was changed or hardware tested.

## Verdict

Stop before implementation. The earlier review's four blockers remain open.
Finding 1 has a user decision recorded in the plan; check the current status
of its requirements. Finding 2 has a user decision and a proposed approach in
the plan, but detection, requirement reconciliation and validation remain open.

## Findings
| # | Severity | Finding | Evidence / location | Question or fix |
|---|---|---|---|---|
| 1 | Blocking for alarm controls | Snooze has a five-minute timer but no specified physical gesture or UI route. The front button hold is dismissal and tap reduces outputs, so neither should silently be repurposed. Users may be unable to invoke an otherwise implemented snooze locally. | Plan Design; REQ-010, REQ-033, REQ-035, REQ-036; factfinding Q-002, Q-003, Q-005 | Agree a discoverable local snooze action, distinguish it from dismiss and tap, and add physical and simulated-input tests including dismissal while snoozed. |
| 2 | Blocking for serial design | Bounded non-blocking I/O is stated, but the draft does not define how responses, event logs and the 64-pixel readout are queued or framed under a slow or disconnected client, nor whether a readout is one coherent frame. Blocking writes could miss the alarm deadline; unchecked dropping or interleaving could lose required responses/events or produce an incoherent readout. | Plan Design; REQ-026, REQ-027, REQ-044–046 | Specify bounded queues, response/event framing, overload and disconnect behavior without silent success, and coherent frame snapshots. Test a slow serial consumer while pixels change and an alarm becomes due; demonstrate response integrity and the timing bound on hardware. |

2026-11-21: The user chose a short front-button press for snooze, alternating
time and `SNZ` while snoozed, and a reminder that a two-second hold fully
dismisses the alarm. The plan records this; REQ-051 and REQ-052 were added
for review.
Later the user chose to enable serial diagnostics only if a connection is
active at power-up and permitted dropping messages for performance. The plan
records a proposed bounded-queue approach and the impact on existing approved
serial requirements; neither detection nor the changed requirements are
resolved by this self-review.

## Checked and sound

The draft identifies shared command validation, explicit fault reporting, RTC
validity, output caps and physical verification. The existing review covers
occurrence lifecycle, safe limits, task decomposition and the uncommitted
baseline; these findings remain unresolved. `dust req check` passed on this
checkout, but checks document consistency rather than behavior.
