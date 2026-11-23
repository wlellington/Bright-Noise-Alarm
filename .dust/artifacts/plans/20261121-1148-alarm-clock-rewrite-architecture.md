---
id: 20261121-1148-alarm-clock-rewrite-architecture
type: plan
title: Alarm clock rewrite architecture
author: Copilot
status: review
created: "2026-11-21 11:48"
branch: dev
inputs: [".dust/project.md", ".dust/requirements.md", ".dust/artifacts/factfinding/20261121-1108-legacy-feature-inventory-and-rewrite-decisions.md"]
confidence: medium
summary: "The user approved all configuration editors with immediate field saves to SD, except the clock editor's Save/Back. The settings task now has a field matrix; undefined values need owner decisions, review and approval before implementation."
open_questions:
  - "How are alarm occurrences consumed across restart, dismissal, one-shot disablement, and failed persistence?"
  - "What are the verified output limits, panel channel order, potentiometer wiring, and audio/storage timing bounds?"
  - "How can the target reliably detect an active serial connection at power-up, and how should existing serial requirements be reconciled with startup gating and permissible dropped messages?"
  - "What are the owner-approved settings domains, defaults, one-shot setup and blank-store initialization policy identified in the settings task?"
next: "Obtain owner decisions, independent review and user approval of the revised settings/storage task; preserve the current worktree and hardware safety gates."
signature:
  agent: user
  role: user
  signed: "2026-11-21 11:48"
  dust: 0.1.0
---
## Goals / non-goals

Design a maintainable ground-up rewrite for the existing RP2040 hardware,
addressing the approved requirements, including USB-based hardware testing.
The tracked firmware is a skeleton; the separate legacy checkout is evidence
for wiring and behavior, not a proven implementation. No network, companion
application, or two future special-function buttons are in scope.

The user approved proceeding with the plan on 2026-11-21. This approves the
direction, not an assertion that unresolved review findings or hardware
measurements are complete. Resolve findings applicable to each implementation
slice and complete its independent review before staffing it.

Hardware verification is a gate for powering or validating the affected outputs
and for claims about timing or physical safety, not a prerequisite for all
host-only foundation work. A proposed foundation task must still identify its
approved requirements, testable acceptance criteria, dependencies and branch;
the uncommitted project baseline must be preserved and committed first.

## Design

### Boundaries and data flow

Prefer a small set of C++ files with explicit ownership, not a generic framework:

- **Hardware adapters:** RTC, display, encoder/button/pots/tap, RGBW panel,
  SD, audio and USB serial. Adapters report errors; no peripheral failure may
  trap the entire startup in a retry loop. Legacy hardware-test configuration
  is a starting point, not proof of actual pinout, channel order or limits.
- **Application:** owns validated settings and runtime state. A typed command
  handler is the single authority for changes from both menu and serial.
  Settings commands validate complete values and report errors without partial
  mutation. Input events from physical and simulated encoder controls feed the
  same menu state machine; physical input events and their resulting actions
  are logged when diagnostics are enabled, subject to the unresolved
  requirements reconciliation below. The clock editor keeps a draft until
  Save or Back; new configuration editors save each changed field immediately
  as described in the bounded settings/storage scope below.
- **Time and scheduling:** RTC provides civil time and validity; monotonic
  time drives short durations, debounce, snooze and animation between samples.
  Scheduler computes dated occurrences from enabled slots and independent
  light/audio ramp durations; ramp starts precede peak time, and ramp progress
  is derived from elapsed time rather than loop iteration count. Use a
  deterministic ordering for due work and explicitly record occurrence identity
  and lifecycle to avoid duplicate triggers. If the clock is set forward into
  an occurrence's active window, do not start that occurrence due to the clock
  change; recompute future scheduling without replaying crossed intervals.
- **Output coordinator:** owns the single effective light/audio output request.
  New alarms preempt sleep or earlier alarms; only the most recent alarm
  matters and preempted alarms or sleep timers do not resume afterward.
  Simultaneous starts use the lowest-numbered slot. Snooze suspends output for five monotonic minutes;
  dismissal terminates that occurrence. Tap halves each current alarm level;
  pots adjust the active output levels. Define composition of ramps, repeated
  taps and pots explicitly before implementation. Apply verified 80% caps at
  the final hardware-output boundary for every source, including previews.
- **Alarm button and display:** The user decided on 2026-11-21 that a short
  front-button press during an active alarm snoozes rather than dismisses it.
  Classify the press on release, so a hold reaching two seconds dismisses
  without first snoozing. During the press, clearly show that holding for two
  seconds fully turns off the alarm; a proposed four-character sequence is
  `HOLD`, `2SEC`, `OFF`, shown in time to be read before the hold threshold.
  After a short press, alternate the clock time and `SNZ` throughout snooze so
  the user can distinguish snooze from dismissal; resume after five minutes.
  A two-second hold can also dismiss a snoozed occurrence, consistent with
  REQ-010. Device tap remains the separate REQ-033 output-reduction gesture.
  Approved REQ-051 and REQ-052 capture the new details.
- **Presentation and diagnostics:** derive four-digit display frames and 64
  RGBW pixel values from the actual committed output frame. The display query
  reports visible content, including blanking; the LED query returns all 64
  final per-channel values. Treat software output as commanded values, not
  proof of electrical or physical output. A line-oriented serial protocol
  offers setting read/write, RTC and alarm operations, encoder simulation,
  screen/LED reads, state queries, and explicit success/error replies.

### Startup-gated serial diagnostics

The user decided on 2026-11-21 that serial API and logging are enabled only
when a serial connection is active at power-up; power-only operation must run
without serial command handling or diagnostic logging. The intended use is
either computer-connected or wall-powered at startup, not connecting USB
afterward. Latch the startup decision without holding alarm initialization
indefinitely. Verify
the RP2040 USB implementation on target: power alone must not count as an
active connection, and define the bounded startup detection window and the
observable host/serial-client signal before implementing. USB enumeration can
occur after power-on; do not equate VBUS with a usable serial connection.

The user permits dropping messages if performance requires it. Proposed
approach: separate bounded command-response and event queues; admit commands
only when response capacity is available, prioritize replies, and allow
asynchronous logs to be dropped rather than delaying alarms or audio. Track
and report dropped-log counts when the client can receive them. A disconnected
or stalled client cannot be guaranteed to receive a reply; define behavior for
an accepted command whose connection is lost. Read all 64 pixels from one
consistent output-frame snapshot, with explicit framing and no interleaving
inside a response. These are design proposals, not measured guarantees.

This decision changes the assumptions of approved REQ-024, REQ-026–029 and
REQ-041–045: late-connected clients may no longer use the API, and dropped
events may not satisfy unconditional log/response wording. Reconcile those
requirements with the Analyst and obtain approval of any changes before
implementation; do not silently weaken or mark them satisfied. Verify on
physical hardware with power-only startup, host-connected startup, late host
connection, disconnect/reconnect, slow clients and saturated output, while
measuring the one-second alarm timing bound and response/frame integrity.

### Configuration editors and SD persistence

The user approved this bounded scope on 2026-11-21: provide all configuration
editors and SD-backed settings persistence. In the new editors, each changed
field is saved immediately; Back navigates out and does not undo earlier
successful field saves. The existing clock editor is the exception: keep its
date/time draft, explicit Save and Back/cancel without changing the RTC
(REQ-002). Immediate field saves must not be postponed until leaving an editor
or require a separate Save action.

- Cover the user-adjustable configuration required by the approved
  requirements: at least five independent alarm slots, their schedules and
  output selections, light presets, sleep configuration, and clock/display
  and system settings. The clock's date/time stays in the RTC; persisted
  settings do not include an active sleep-timer session. The
  [settings task field matrix](../tasks/20261121-2215-configuration-editors-and-sd-settings-persistence.md#requirement-to-field-matrix)
  enumerates objects, fields, allowed values where known, commands, saves and
  tests. Its explicitly unresolved domains and defaults require owner decisions;
  neither the matrix nor legacy hardware guesses authorize those values.
- Route each field change through the application command and its validation,
  shared with equivalent serial operations where available (REQ-025/041).
  Attempt persistence for each accepted changed field, not just the final
  editor value. Do not report a field saved until its write completes; on
  invalid input or write failure restore the prior displayed and runtime value,
  preserve the previously persisted value and unrelated fields after restart,
  explicitly indicate the failure, and do not silently claim persistence.
  Demonstrate this with fault injection, including failure around commit and
  recovery; do not promise storage atomicity without evidence.
- Use a versioned, validated SD settings representation with an integrity
  check and recovery from an interrupted update where feasible; preserve the
  last known good settings and unrelated SD audio. Missing, unreadable, corrupt
  or full storage must be reported (REQ-019), without silently replacing saved
  settings or preventing the clock UI from operating. Verify actual filesystem
  behavior rather than assuming a rename or flush is power-loss atomic.
  A genuinely blank store is distinct from an existing unreadable or corrupt
  store: initial settings must come from owner-approved defaults, which are not
  yet specified; no guessed defaults or silent write is authorized. Until
  initialization is decided, expose an explicit uninitialized/setup-needed
  state. For corrupt or unreadable existing data, report the fault and preserve
  it; recover only a verified last-known-good copy, otherwise expose no saved
  settings and do not silently reset or overwrite. Clock operation remains
  available in both cases.
- Restore saved alarms, presets and system settings on restart (REQ-018).
  Whether sleep configuration persists is an owner decision; do not restore
  active outputs or an active sleep timer (REQ-048). One-shot occurrence and snooze recovery still require their own
  durable lifecycle design; this editor slice must not claim those behaviors
  complete.
- Show an editor-only candidate value before its field save attempt, without
  driving the panel. This is not REQ-014's physical panel preview. REQ-014
  remains not verified and not satisfied in this slice; panel preview remains
  gated on verified safe output limits under REQ-038 and later physical testing.
- SD access and write frequency must be bounded and measured alongside other
  active work; do not silently defer or coalesce accepted field saves. The
  audio/SD ownership and alarm-timing implications remain hardware gates.
  The current clock-editor behavior and user-confirmed display must not regress.

Acceptance for this scope: each configuration field can be reached and changed
locally; each valid changed field causes an immediate save attempt, survives
navigation and power cycle when storage succeeds, and leaves unrelated
fields intact; failures are visible and do not masquerade as saved values;
clock Back cancels and clock Save commits as before. Host tests exercise every
field family, repeated changes, invalid values, missing/full/corrupt storage,
interrupted writes, recovery, clock Save/Back, and each local editor's shared
application-command path. Target build and an SD
power-cycle test are required; safely gated preview and alarm/audio behavior
need separate physical evidence. Full serial settings coverage, alarm
scheduling and audio playback are not demonstrated merely by completing
the editor/storage slice. One-shot configuration does not complete REQ-008's
occurrence/disablement or recovery behavior; REQ-041 remains incomplete until
the full serial setting read/write coverage is implemented and verified.

Initialization identifies missing RTC, storage and audio without blocking the
remaining controls. Invalid RTC is never silently replaced by build time;
indicate that it must be set, and do not schedule against invalid time.
Unavailable alarm audio reports a fault while configured light continues.
Settings persistence must handle versioning, failed writes and interrupted
updates without reporting success prematurely. One-shot disablement and
occurrence lifecycle need durable handling consistent with restart recovery;
an active sleep timer is runtime-only and must not resume after restart.

On restart with valid RTC, evaluate occurrences that could still be active,
resume at the elapsed ramp/output phase when required, and do not replay
already finished occurrences. The user clarified on 2026-11-21 that an alarm
which was snoozed when power was lost is finished and must not resume.
Determine whether an occurrence was previously snoozed, dismissed or consumed
before recovery, subject to the unresolved persistence rules below. Recovery
must also apply the ordinary priority rules.

Use a bounded, non-blocking main loop with time-sliced I/O and rendering; measure
the end-to-end scheduler latency under audio, SD, serial and LED load against
the one-second tolerance. Do not assume multicore is necessary. If measured
audio servicing requires core 1, use explicit synchronized message passing
and a single owner for each peripheral and SD transaction; do not share
mutable state or a filesystem object unsafely between cores. Establish playback
buffering and hardware-timing feasibility before relying on this arrangement.

### Requirement coverage

| Area | Approved requirement IDs | Design responsibility |
|---|---|---|
| Clock and local controls | REQ-001–005, REQ-020, REQ-034, REQ-039, REQ-047, REQ-050 | RTC validity, menu/input and visible display state |
| Scheduling and alarms | REQ-006–010, REQ-023, REQ-030, REQ-033, REQ-035–037, REQ-046, REQ-049, REQ-051–055 | Dated occurrences, ramp timing, arbitration, interaction and recovery |
| Lighting | REQ-011–014, REQ-021, REQ-031–032, REQ-038 | Effects, presets, final RGBW frame and verified output caps |
| Sleep | REQ-015–016, REQ-040, REQ-048 | Independent outputs, countdown, cancellation and restart behavior |
| Audio and persistence | REQ-017–019 | WAV playback, saved settings and explicit resource faults |
| Serial and hardware test | REQ-024–029, REQ-041–045, REQ-056–057 | Shared actions, protocol, observability and physical-device verification |

REQ-022 is dropped and is not part of this design. Each implementation task
must expand its applicable IDs and acceptance criteria; ranges above group only
approved IDs, not REQ-022. REQ-051–057 capture the user's approved snooze,
preemption, recovery, clock-change and serial-diagnostics decisions. The new
serial requirements still need reconciliation with existing requirements.

## Options considered

- **Shared application commands vs separate menu/serial implementations:**
  shared commands avoid divergent validation and satisfy REQ-025; simulated
  encoder events still traverse the menu, rather than bypassing it.
- **One cooperative owner vs immediate multicore split:** begin with single
  ownership and measured latency; introduce a synchronized audio worker only
  if timing evidence requires it. This avoids inherited unsynchronized startup
  flags and competing SD access.
- **Elapsed-time scheduler vs periodic exact-time equality:** dated occurrences
  and elapsed progress support ramps and recovery; equality polling alone
  misses work during load or restart. Civil-time changes and consumed
  occurrences require explicit policies.

## Steps

Dependencies are sequential unless a later approved task explicitly identifies
independent work and integration ownership. Feature branch names are
proposals, not existing branches. Staffing and implementation require approval.

| # | Step | Branch | Acceptance criteria | Verification |
|---|---|---|---|---|
| 0 | Resolve slice-specific semantics and hardware feasibility | none | Resolve relevant review findings before each slice; verify target wiring, safe output limits, audio/SD timing and supported WAV format before affected hardware operation or claims. | On-device channel, level, audio and load measurements with recorded setup; no unverified maximum treated as safe. Host-only work does not require these measurements. |
| 1 | Configuration editors and storage | assigned in approved task | All configuration editors and SD persistence; each new-editor field change saves immediately, while clock Save/Back is unchanged; explicit storage faults. | Host tests for field saves, clock cancel, storage failure/restart; target build and SD power-cycle test. Preview and output hardware remain gated. |
| 2 | Clock, inputs and serial | feature/clock-controls | Time and menu actions, physical controls, serial queries/results, simulated encoder and display read share application state. | Host command/menu parity and malformed-input tests; target serial/input/display checks. |
| 3 | Lighting and audio | feature/output-drivers | RGBW frames, effects, previews, WAV playback and final output caps work through bounded adapters. | Host effect/frame/cap tests; target RGBW and audio checks, including missing files. |
| 4 | Alarms and sleep | feature/scheduling | All alarm, ramp, snooze, dismissal, priority, sleep and restart requirements operate using dated occurrence state. | Fake RTC/monotonic tests for boundaries, conflict, reboot and invalid RTC; target timing under concurrent load. |
| 5 | Full hardware validation | feature/hardware-verification | All approved requirements are traced to evidence, including end-to-end serial-controlled hardware operation. | Target tests for applicable approved REQs, 64-pixel readback and physical observations; record timing measurements and gaps. |

Each task includes only its applicable approved REQs; integration and any
remaining behavior may require additional bounded tasks. Keep the normal
independent plan/code review, Tester verification and formal Analyst
requirements sign-off gates. The user approves the completed plan and staffing
before execution, and merge/release separately.

For the alarm button, verify a short press causes snooze and alternating
time/`SNZ`; holding through two seconds dismisses without first snoozing;
the complete two-second dismissal reminder is legible on the four-digit
display during the hold; and holding while snoozed prevents resumption.
Check the distinction between button press and physical device tap, including
input event logs. These criteria supplement REQ-010, REQ-033, REQ-035 and
REQ-036 and approved REQ-051 and REQ-052.
Test that the alarm reaches peak at its configured time, preempted outputs do
not resume, a snoozed occurrence stays finished after power loss, and setting
the clock forward into an active window does not trigger that occurrence.

## Risks and mitigations

- **Semantics requiring owner/Analyst resolution:** Define occurrence identity
  and persistence on one-shot start, snooze and dismissal; if settings writes
  fail, specify what is reported and what can be guaranteed after power loss.
  Define end-of-alarm/run-duration anchor, each ramp's start, zero-duration
  ramps, same-start versus same-peak conflicts, and restart during or after
  peak. Define date/time recurrence behavior so REQ-046 and REQ-049 are
  testable without duplicate alarms. Do not reopen the user decisions recorded
  above; the remaining issue is implementing and testing them reliably.
- **Physical safety and pinout:** REQ-038 depends on verified actual maxima.
  Do not infer electrical safety from a numeric library range or legacy source.
  Verify panel RGBW order, pot pins, power budget and level limits on hardware.
- **Realtime behavior:** LED updates, USB output, SD access and audio buffering
  can delay scheduling. Stress-test actual hardware and bound synchronous
  operations; do not claim the one-second tolerance from a host test.
- **Faults and observability:** Software frame readback is not a sensor.
  Exercise disconnected SD, missing audio and invalid RTC, and compare serial
  state with physical output. Define visible indication when the display or
  serial is itself unavailable.
- **Hardware evidence:** Existing test code and library dependencies compile,
  but hardware behavior, I2S format support and storage concurrency have not
  been demonstrated by this plan.

## Rollback

Do not alter the legacy checkout. Develop on approved feature branches, retain
the prior firmware and settings until migration is verified, and document the
previous flash image and settings backup before hardware tests. Revert a
problematic feature or reflash a verified prior image through the agreed
release process; do not silently erase user settings.
