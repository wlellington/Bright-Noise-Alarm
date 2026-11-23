---
id: 20261121-2215-configuration-editors-and-sd-settings-persistence
type: task
title: Configuration editors and SD settings persistence
author: Copilot
status: active
created: "2026-11-21 22:15"
branch: feature/encoder-main-menu
inputs: ["20261121-1148-alarm-clock-rewrite-architecture.md", "20261121-1108-legacy-feature-inventory-and-rewrite-decisions.md", "20261121-2217-configuration-editors-and-sd-persistence-plan-revi.md", "REQ-001", "REQ-002", "REQ-004", "REQ-005", "REQ-006", "REQ-007", "REQ-008", "REQ-009", "REQ-011", "REQ-012", "REQ-013", "REQ-014", "REQ-015", "REQ-017", "REQ-018", "REQ-019", "REQ-021", "REQ-025", "REQ-030", "REQ-031", "REQ-032", "REQ-038", "REQ-040", "REQ-041", "REQ-048"]
confidence: medium
summary: "Display format, brightness and dark hours use shared menu/serial SD saves and survived power cycles; alarm, preset, sleep editors and interruption recovery remain open."
open_questions:
  - "How does the real SD card behave across interrupted writes, including an error reported after a write completed?"
  - "Safe light preview, other settings editors and serial coverage remain unfinished."
next: "Continue the remaining settings editors and test SD interruption recovery; preserve physical-output safety gates."
signature:
  agent: user
  role: user
  signed: "2026-11-21 22:15"
  dust: 0.1.0
---
## Objective
Implement the approved configuration editors for system settings, five alarms,
light presets and sleep-timer options, backed by durable SD-card settings
storage. Every valid field change in a new editor is saved immediately; Back
navigates away but does not roll back earlier successful saves. Preserve the
existing clock editor's draft, explicit Save and Back/cancel behavior.

## Iteration update (2026-11-21)

The project now uses iterative delivery. The user authorized reasonable
first-boot choices in Q-020. The first implementation adds 12/24-hour format
and display brightness (0–15), defaulting to the previous 24-hour and level-3
behavior, with immediate saves to two versioned, checksummed SD files.
`make -C tests test`, the RP2040 target build and `dust req check` pass.
On 2026-11-21, on-device serial set 12-hour format and brightness 4, rejected
brightness 16, and confirmed both settings survived a user-performed USB
power cycle; serial then restored 24-hour format and brightness 3. This is not
an interrupted-write test.

On 2026-11-22, dark-hours editing, a 30-second temporary wake, serial setting
commands and simulated encoder input were added. Version-1 settings are read
without modification and migrate on a successful save. Host tests and the
RP2040 build passed. On-device serial reported blanking, wake on a simulated
encoder click, reblanking after 30 seconds, and restoration of selected hours
after a USB power cycle; defaults were restored afterward. A subsequent
`SD_LIST` showed both WAVs and two 24-byte settings files. The physical screen
and encoder were not independently observed during this test, and interrupted
SD writes remain unverified on hardware.

The matrix and acceptance criteria below remain the wider task's goals, not a
requirement to obtain approval for every reversible default before coding.
An SD error reported after a write completed can leave the new record valid;
on restart it may be selected even though the UI reported failure. The stricter
after-restart failed-write invariant remains unverified, not satisfied. The UI
preserves its prior value and stops further writes until restart. All other
editors, physical preview and output safety remain open; no requirements are
marked satisfied.

On-device read-only SD inspection found `Ooblets_32.wav`, `click.wav.wav`,
`alarm_settings.txt`, two same-named `alarm_settings_tmp.txt` entries, and
`general_settings.txt`. The alarm settings files contained unexpected binary
directory-entry-like data. A read-only `fsck.fat -n -v /dev/sda1` on the
unmounted card confirmed corrupt FAT entries, duplicate/broken directory
entries and cross-linked settings files; it made no changes. Both WAV files
were copied into `SD-Card-Backup/`, with matching SHA-256 hashes and `cmp`
against the originals and readable WAV headers. The three settings reads were
also backed up in the session workspace; only one duplicate temporary entry
could be read by name. The user chose to reformat and restore only the two
backed-up WAVs. The card was reformatted FAT32 as `BRIGHTNOISE`, the two WAVs
were restored and matched byte-for-byte, and a subsequent unmounted
`fsck.fat -n -v /dev/sda1` passed. Firmware with read-only serial SD listing
and reading (without the removal command) was uploaded. On-device `SD_LIST`
confirmed the two expected WAV files and sizes; after settings changes, it
also showed the two new 16-byte settings files.

The work is governed by the updated
[architecture plan](../plans/20261121-1148-alarm-clock-rewrite-architecture.md).
Do not implement alarm scheduling, alarm playback, sleep output execution or
unsafe physical light preview as part of this editor/storage slice.

## Inputs to read
- `.dust/requirements.md`: read only the listed REQs; they define editable
  fields and persistence behavior.
- `20261121-1108-legacy-feature-inventory-and-rewrite-decisions.md`: approved
  answers for alarm count, sleep outputs, lighting modes and persistence.
- `20261121-1148-alarm-clock-rewrite-architecture.md` and the two architecture
  reviews: storage failure/recovery and output-safety gates.
- Current `Menu_System.*`, `File_Management.*`, `Defines.hpp`, and application
  integration: preserve existing encoder and clock behavior.

## Branch
`feature/encoder-main-menu` (preserve the existing dirty work and uploaded,
physically tested clock implementation; do not merge).

## Requirements
REQ-001, REQ-002, REQ-004 and REQ-005 (clock format, unchanged clock
Save/Back, display brightness and dark hours); REQ-006–009 (five independent
alarms, weekly/one-shot schedules and per-alarm outputs); REQ-011–014,
REQ-021 and REQ-031–032 (effect and RGBW preset configuration; physical
REQ-014 preview remains incomplete); REQ-015, REQ-017 and REQ-040 (sleep
duration, output selection, preset and WAV item); REQ-018 and REQ-019
(persistence and faults); REQ-025 and REQ-041 (shared commands and eventual
serial configuration); REQ-030 (ramp configuration); REQ-038 (output safety);
REQ-048 (do not restore an active sleep timer).

This slice implements local editors and persistence only. It does not claim
the broader serial settings API, alarm/sleep execution, audio playback,
one-shot occurrence recovery, alarm restart recovery, or physical preview safe
limits complete.

The user approved the scope on 2026-11-21 by selecting all configuration
editors plus SD persistence, and selected immediate save for each changed
field. The approved exception is the existing clock editor, which retains
Save/Back. The independent review
[20261121-2217](../reviews/20261121-2217-configuration-editors-and-sd-persistence-plan-revi.md)
returned no-go; this revision does not itself constitute approval to implement.

## Requirement-to-field matrix

This is the single field contract for the plan and task. `U` means **unresolved
owner decision**, not a default or a permission to pick a value. The default
for every new setting below is `U` unless explicitly stated otherwise; no
approved source establishes the first-boot values. Natural types and semantic
choices below do not assert unverified hardware ranges. The named commands are
proposed application operations, not claims that those APIs already exist.

For **every** new-editor row, save policy `I` means: invoke the indicated
shared application command for each valid changed field, immediately attempt
the field save, and acknowledge/apply only a successful write; Back never rolls
back earlier successful saves. Each row's positive and negative host tests
must exercise that local editor through the command, validation and storage
result, not call storage directly. On invalid input or failed writes, assert
the prior displayed, runtime and persisted value and unrelated fields remain
unchanged, including after restart, with an explicit error. No implementation
may assume that a filesystem operation is power-loss atomic; if this invariant
cannot be guaranteed for a failure class, report it as a blocker.

| REQ / owning object | Editable field | Domain and first-boot default | Validation and shared command | Save | Positive / negative host test |
|---|---|---|---|---|---|
| REQ-002 / RTC clock | Year, month, day, hour, minute, second | Current `clock_command::isValid`: 2000–2099, valid calendar date, 00–23:00–59:00–59; RTC value, not SD default | Existing `setDateTime`; valid complete date/time | Explicit Save; Back cancels, not I | Save leap day through local editor; invalid date and Back leave RTC unchanged. |
| REQ-001 / system | Time format | 12-hour or 24-hour; default U | `SetSystem(format, value)`; reject other enum | I | Select each format and restore; invalid enum rejected. |
| REQ-004 / system | Display brightness | Supported range/step and default U | `SetSystem(displayBrightness, value)`; check approved range | I | Save an approved value and restore; out-of-range value rejected. |
| REQ-005 / system | Dark-hours start | Civil clock time; precision, default U | `SetSystem(darkStart, value)`; validate time and approved precision | I | Save start in a midnight-spanning interval; malformed time rejected. |
| REQ-005 / system | Dark-hours end | Civil clock time; precision, default U | `SetSystem(darkEnd, value)`; validate time and approved precision | I | Save end in a midnight-spanning interval; malformed time rejected. |
| REQ-006 / alarms | Slot identity | At least five independently addressable slots; exact count/initial contents U | `SetAlarm(slot, field, value)`; slot must exist | I for fields, not slot navigation | Edit two slots independently; invalid slot cannot mutate any slot. |
| REQ-006 / alarm slot | Enabled | Boolean; default U | `SetAlarm(slot, enabled, value)`; reject non-boolean | I | Toggle one slot; invalid value rejected and other slots unchanged. |
| REQ-007, REQ-008 / alarm slot | Alarm peak time | Civil clock time; precision and default U | `SetAlarm(slot, time, value)`; validate time and approved precision | I | Save valid peak time; malformed/out-of-range time rejected. |
| REQ-007, REQ-008 / alarm slot | Schedule mode | Recurring selected weekdays or one-shot; default U | `SetAlarm(slot, mode, value)`; reject unsupported mode | I | Save each mode; unknown mode rejected. |
| REQ-007 / alarm slot | Weekdays | Independently selected Monday through Sunday; empty-set policy and default U | `SetAlarm(slot, weekdays, value)`; validate seven-day set and owner policy | I | Save Monday/Friday and restore; reject an invalid weekday. |
| REQ-008 / alarm slot | One-shot occurrence selection | One-shot mode and peak time are required; whether a calendar date or next eligible occurrence is needed is U; date range/default U if selected | `SetAlarm(slot, oneShotTarget, value)`; validate the owner-approved representation; do not invent a date rule | I for each applicable changed field | Configure and restore the approved one-shot target; reject malformed or invalid target. Trigger/disable/recovery are not tested or claimed here. |
| REQ-009, REQ-011–013, REQ-031–032 / alarm slot | Light effect or preset reference | Sunrise, fixed color, breathing, forest, fade-to-color; direct effect vs preset-reference selection and default U | `SetAlarm(slot, lightSelection, value)`; accept approved effects or existing preset per owner choice | I | Save an approved selection per two independent slots; reject unknown effect or invalid preset. |
| REQ-009, REQ-017 / alarm slot | Sound selection | Available compatible user WAV item; allowed encoding, identity and default U | `SetAlarm(slot, sound, value)`; validate approved item/reference; report unavailable resource | I | Select and restore an available WAV; reject invalid item and explicitly report missing file. |
| REQ-009, REQ-038 / alarm slot | Target volume | Range/step/default U; actual output capped at 80% of verified physical limit | `SetAlarm(slot, volume, value)`; validate approved range, not a guessed hardware maximum | I | Save an approved volume; reject out-of-range value. Physical cap is not verified by this editor test. |
| REQ-009 / alarm slot | Run duration | Units/range/step/default and end anchor U | `SetAlarm(slot, runDuration, value)`; validate approved duration | I | Save a valid duration; reject malformed/out-of-range duration. |
| REQ-030 / alarm slot | Light ramp duration | Units/range/step/default U; separate vs shared ramp control U | `SetAlarm(slot, lightRamp, value)` if separately configured, otherwise approved shared ramp operation | I | Save an approved duration; reject invalid value. This does not verify ramp timing. |
| REQ-030 / alarm slot | Audio ramp duration | Units/range/step/default U; separate vs shared ramp control U | `SetAlarm(slot, audioRamp, value)` if separately configured, otherwise approved shared ramp operation | I | Save an approved duration; reject invalid value. This does not verify ramp timing. |
| REQ-013 / light presets | Preset identity | Select an existing saved preset slot; slot count and initial contents U | `SetPreset(slot, field, value)`; slot must exist | I for fields, not slot navigation | Save and restore two distinct presets; reject nonexistent slot. |
| REQ-011–013, REQ-031–032 / light preset | Effect | Sunrise, fixed color, breathing, forest, fade-to-color; default U | `SetPreset(slot, effect, value)`; accept each named effect, reject others | I | Select and restore every named effect; reject unknown effect. |
| REQ-013, REQ-021 / light preset | Red, green, blue and white components | Four independently selectable components; component ranges/steps/defaults U | `SetPreset(slot, red/green/blue/white, value)`; validate each approved component domain | I per component | Change and restore each channel independently; reject invalid component or value without changing the others. |
| REQ-013, REQ-038 / light preset | Brightness | Range/step/default U; 80% physical limit applies at output boundary | `SetPreset(slot, brightness, value)`; validate approved range | I | Save approved brightness; reject out-of-range value. No physical panel output. |
| REQ-031 / light preset | Forest optional green | Optional green is supported; whether it is an explicit editable switch/amount and its domain/default U | If owner selects a field, `SetPreset(slot, forestGreen, value)`; validate agreed domain | I if applicable | Configure approved green option and restore; reject invalid option. Animation remains out of scope. |
| REQ-015, REQ-040 / sleep configuration | Duration | Units/range/step/default U | `SetSleep(duration, value)`; validate approved duration | I | Save an approved duration; reject malformed/out-of-range duration. |
| REQ-015, REQ-040 / sleep configuration | Output selection | Light only, audio only, or both; default U | `SetSleep(outputs, value)`; accept those three choices; reject unsupported selection | I | Save each of three choices; reject invalid selection. No outputs are started. |
| REQ-015, REQ-040 / sleep configuration | Light preset selection | Existing light preset; selection/default U | `SetSleep(lightPreset, value)`; validate preset reference | I | Select an existing preset; reject invalid reference. |
| REQ-015, REQ-017, REQ-040 / sleep configuration | Audio item selection | Available compatible user WAV item; encoding, identity and default U | `SetSleep(audioItem, value)`; validate approved item/reference; report unavailable resource | I | Select an available WAV; reject invalid item and report missing file. |

Slot selection is navigation, not a settings mutation; individual slot fields
are saved independently. A missing reference must not silently become another
item. The matrix does not authorize actual light/audio output, alarm or sleep
execution, or a full serial settings API. REQ-008 one-shot triggering,
disablement and recovery, and REQ-041 serial read/write coverage remain
incomplete.
REQ-039's live potentiometer controls during alarm or sleep are runtime
behavior, not a proven persisted sleep-volume or brightness setting; their
execution and verification remain outside this editor/storage slice.

## Owner decisions required before implementation

1. First-boot defaults for every field above; exact alarm count (minimum five),
   preset count, initial slot contents, and whether unset/incomplete objects
   are allowed. Specify how to initialize a blank store without inventing
   values and whether sleep configuration is persisted across restart.
2. Display/panel brightness and audio volume domains, units, steps and
   defaults; RGBW component domains/steps/defaults. Physical maxima are a
   separate measurement, not values inferred from a library range.
3. Alarm peak and dark-hour time precision/defaults; dark-hours enable/disable
   and equal-endpoint behavior if desired. Define allowed empty weekday sets.
4. One-shot setup: explicit date versus next occurrence, applicable date range
   and default; default schedule mode. This decision is not occurrence
   execution/recovery approval.
5. Alarm run, light-ramp, audio-ramp and sleep durations: units, ranges, steps,
   defaults, whether ramps are independent, and run-duration end anchor.
6. Alarm light selection by effect or preset, preset slot count/default
   effect, optional forest-green configuration, sleep preset and WAV defaults,
   available WAV format/identity rules, and behavior when a selected item is
   unavailable. Choose the three-way sleep output default.

Do not treat these questions or command names as binding product decisions.
Choose reasonable reversible defaults with evidence, preserving the user's
immediate-save and clock Save/Back decisions and all approved requirements.

## Blank store and failure recovery

- **Genuinely blank store:** an accessible SD card with no settings record is
  not corruption. Show an uninitialized/setup-needed state and keep the clock
  interface usable. The first iteration uses display defaults of 24-hour and
  level 3; it writes settings only after an edit so the existing card can be
  inspected first. Other
  setting defaults will be chosen as their editors are implemented.
- **Existing corrupt/unreadable store:** report the fault, retain the original
  data and do not silently initialize or overwrite it. Load a verified
  last-known-good copy only if available; otherwise expose settings as
  unavailable and keep the clock interface usable. Missing card and full
  storage likewise produce explicit faults, not fabricated saved settings.
- **Failed field save:** host fault injection at each relevant write/commit
  stage must check the old displayed value, runtime value, persisted value and
  unrelated fields immediately and after restart; report an error. A write
  reported as failed may nevertheless have reached the card; the current
  implementation explicitly leaves that after-restart outcome unresolved.
  Verify actual SD filesystem behavior physically, including interruption.

REQ-014 requires the **physical panel** to reflect a color/brightness
adjustment before save. Showing the candidate in the editor is only UI
feedback; it does not satisfy or verify REQ-014. Keep panel preview disabled
until REQ-038 verified limits allow it, and leave REQ-014 incomplete pending
separate safe on-device verification.

## Recommended tier
Standard: this spans a typed settings model, multiple editor flows, SD
versioning/recovery and host-testable storage failures.

## Acceptance criteria
- [ ] System Settings edits 12/24-hour format, approved display brightness,
      and dark-hour start and end times; intervals spanning midnight work.
- [ ] Alarm configuration manages at least five independent slots with
      enablement, alarm time, weekly selected days or one-shot mode, light
      effect, audio selection, volume and duration.
- [ ] Light preset editors support all five matrix effects, independent RGBW
      color and brightness. The editor displays the candidate before its
      immediate save attempt, without driving the panel. This is not REQ-014
      preview; physical panel preview remains incomplete and safety-gated.
- [ ] Sleep configuration edits duration, chooses light only, audio only or
      both, and selects the light preset and audio item. An active sleep-timer
      session is never restored on reboot.
- [ ] Every valid changed field in these new editors attempts a durable save
      immediately. Successful fields remain saved when Back is selected.
      Invalid input or storage failure retains the previous displayed, runtime
      and persisted values and unrelated fields after restart and presents an
      explicit error.
- [ ] SD settings use a versioned, validated representation with an integrity
      check and recovery from interrupted writes; loading selects the last
      known-good settings and does not overwrite unrelated audio files.
- [ ] Missing, unreadable, corrupt or full SD storage is reported without
      trapping startup, silently replacing existing settings or claiming a
      failed write succeeded.
- [ ] A genuinely blank store and an existing corrupt/unreadable store take
      the distinct paths above; no undefined defaults are invented.
- [ ] Saved alarm, preset and system settings restore after restart; active
      outputs and sleep-timer sessions do not.
- [ ] Host tests cover each matrix row's positive and negative cases, and
      prove local editor events invoke the listed shared application command
      with the same validation, persistence and result semantics. Cover
      immediate saves, unrelated field preservation, storage failure,
      corruption/interruption, recovery and unchanged clock Save/Back.
      The RP2040 target builds.
- [ ] A physical SD power-cycle test confirms saved settings restore. No claim
      is made that alarm timing, one-shot occurrence/recovery, audio, physical
      REQ-014 preview or REQ-041 serial coverage is verified by this task.

## Out of scope
Alarm scheduling and output execution; one-shot/snooze/dismiss recovery;
audio playback and file browsing beyond selecting an available item; actual
RGBW preview until REQ-038 limits are verified; complete serial settings
commands; changes to the clock editor's existing Save/Back flow; wall-power
serial-startup verification from the prior task.

## Feedback and try-out
After implementation and upload, the user will navigate System Settings,
Alarms, Light Effects and Sleep Timer with the encoder. Check that each
changed field persists immediately, that Back does not undo successful edits,
and that values return after a power cycle with the SD card installed.
Physical panel preview and output behavior remain gated by verified safe
limits.

## Workflow
Iterative, intent-directed delivery is the project default. Preserve approved
requirements and review findings; formal planning and re-review are not
automatic gates. Continue on the current feature branch, keep the physical
output safety gate, and obtain permission for upload, merge or release.
