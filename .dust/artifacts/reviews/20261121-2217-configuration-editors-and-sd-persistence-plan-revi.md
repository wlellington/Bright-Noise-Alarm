---
id: 20261121-2217-configuration-editors-and-sd-persistence-plan-revi
type: review
title: Configuration editors and SD persistence plan review
author: Reviewer
status: done
created: "2026-11-21 22:17"
branch: feature/encoder-main-menu
inputs: [".dust/artifacts/plans/20261121-1148-alarm-clock-rewrite-architecture.md", ".dust/artifacts/tasks/20261121-2215-configuration-editors-and-sd-settings-persistence.md", ".dust/requirements.md", ".dust/artifacts/reviews/20261121-1150-alarm-clock-architecture-review.md", ".dust/artifacts/reviews/20261121-1152-architecture-self-review-addendum.md"]
confidence: medium
summary: "No-go: the draft does not enumerate all required editable fields or resolve the preview/REQ-014 mismatch. Specify field domains and initial-storage behavior, then resubmit; alarm recovery, serial reconciliation and physical output safety remain gated."
open_questions:
  - "What are the approved value domains/defaults for brightness, volume, durations, dark-hour times, effect/preset slots and sleep selections?"
  - "Is REQ-014 deferred until safe physical preview is verified, or is another approved panel-preview behavior intended?"
  - "What should the UI expose on a blank settings store, and how should a one-shot alarm be configured without claiming its unresolved occurrence lifecycle?"
next: "Update the task with a requirement-to-field/validation/test matrix, explicit REQ-014 deferral or safe-preview evidence, and blank-store/error recovery behavior; then obtain user approval before implementation."
signature:
  agent: user
  role: user
  signed: "2026-11-21 22:17"
  dust: 0.1.0
---
## Scope
Independent, read-only review of the bounded “Configuration editors and SD
persistence” section in plan
[20261121-1148](../plans/20261121-1148-alarm-clock-rewrite-architecture.md)
and task
[20261121-2215](../tasks/20261121-2215-configuration-editors-and-sd-settings-persistence.md),
against the approved requirements and prior reviews
[20261121-1150](20261121-1150-alarm-clock-architecture-review.md) and
[20261121-1152](20261121-1152-architecture-self-review-addendum.md).
No firmware, plan, task or requirements were changed. No hardware behavior was
tested.

## Verdict
**No-go.** The direction is sound, and the task preserves the major safety and
unresolved-feature gates, but the field contract is incomplete and REQ-014's
approved physical-preview criterion conflicts with the disabled-preview gate.
Implementation would require choices not fixed by the task/approved
requirements. Resolve the blocking findings below, then obtain the task's
required user approval before coding.

## Findings
| # | Severity | Finding | Evidence / location | Question or fix |
|---|---|---|---|---|
| 1 | blocker | The task does not enumerate all locally editable fields, their validation domains or expected initial values, despite the plan explicitly requiring that before implementation. It omits the user-selectable 12/24-hour display format (REQ-001); does not identify the complete approved effect set (REQ-011/012, REQ-031/032); and does not say which light preset/audio item the sleep timer selects (REQ-015/017/040). “Supported” and “approved effects” are not testable domains. Brightness, volume, duration, schedule, preset-slot and sleep-item ranges/defaults are not fixed by the listed requirements. The alarm criteria also say “one-shot mode” without stating whether its stored schedule needs a date or other configuration beyond a time. | Plan, “Configuration editors and SD persistence,” requires actual settings and validation to be enumerated before implementation. Task “Requirements”/acceptance criteria enumerate broad families but not every field or value domain. Approved REQ-001, 004–009, 011–015, 017, 031–032 and 040. | Add a requirement-to-field matrix: each field and owning settings object, type/allowed values and bounds, initial/default behavior, validation, shared command, save-on-change behavior and positive/negative test. Include format, every approved effect, per-alarm and sleep output selections. Resolve missing numeric domains/defaults with the owner or cite an already approved authoritative source; do not infer them from unverified hardware. State that one-shot occurrence execution/recovery remains deferred and REQ-008 is not complete in this slice. |
| 2 | blocker for claiming the full preset-editor requirement | REQ-014 requires the panel to reflect color/brightness adjustment before save. The plan calls for a “visible preview” before the save attempt, while the task only presents a “candidate preview state” and disables physical panel preview until REQ-038 safety limits are verified. A state shown in the editor is not the panel preview specified by REQ-014. The task cannot both meet that requirement now and keep the physical-output safety gate closed. | Plan lines in the bounded section on REQ-014/REQ-038; task light-preset acceptance and out-of-scope clauses; REQ-014 acceptance; unresolved safe-output finding in review 20261121-1150. | Explicitly mark physical REQ-014 preview as deferred/not satisfied by this task, and define/test any editor-only candidate presentation without calling it the REQ-014 panel preview; keep the physical test gated until REQ-038 limits are verified. Alternatively, obtain and record the required safety evidence and make a safe pre-save panel preview part of acceptance. |
| 3 | should-fix | Storage acceptance does not define the first-boot/blank-store case or a sufficiently observable recovery oracle. It requires restoration and rejects silent replacement, but does not state what settings the UI loads when an SD card is present with no settings file, what happens to editors when no last-known-good value exists, or how tests distinguish a blank store from corrupt existing data. The task also lacks a specific observable check that a failed field write leaves the previously displayed/applied value unchanged. | Plan requires storage faults not to prevent clock UI and not to replace saved settings; task criteria for missing/corrupt storage and last-successfully-saved values do not define empty-store behavior. | Specify factory/default handling for a genuinely empty store versus corrupt/unreadable existing settings; require an explicit fault and no destructive replacement for the latter. Test both cases and assert runtime/display value, persisted value and unrelated fields after write failure and restart. |
| 4 | should-fix | REQ-025 is in scope, and the plan requires field changes to use the application command and shared validation where equivalent serial operations exist, but the task acceptance only says “host tests cover every settings field family.” It does not verify that local editor changes use that command path or that they share its validation/persistence outcome. REQ-041 is correctly not claimed complete, but should not be implied satisfied. | Plan shared-command bullet; task REQ-025/041 list, test criterion and explicit exclusion of broad serial configuration. | Add a host test/criterion proving each editor invokes the application-level command and applies the same validation/result semantics; explicitly leave serial read/write coverage and REQ-041 incomplete for a later task. |

## Checked and sound
The immediate-save policy and Back behavior are clear, and the clock editor is
explicitly exempted so its draft/Save/Back flow is preserved. The task also
appropriately excludes alarm/sleep execution, audio playback, broad serial
configuration and one-shot/restart recovery; it does not pretend the previous
alarm-occurrence blocker is solved. Missing/unreadable/corrupt/full storage,
versioning, integrity checks, unrelated audio preservation, host failure tests,
target build and an SD power-cycle test are called out. Physical preview is
gated on REQ-038 rather than treated as verified.

The unresolved gates from the prior reviews remain intact: occurrence lifecycle
and recovery still require their own design (20261121-1150 finding 1); physical
output safety is still unverified (finding 2); startup-gated serial behavior
and requirement reconciliation remain open (20261121-1152 finding 2 and its
follow-up). They do not need to be solved by this settings/storage slice if
their requirements remain explicitly incomplete. The task itself requires
user approval after independent review; this review is not implementation
authorization.
