---
id: requirements
type: requirements
title: Requirements
author: user
status: active
created: "2026-11-21 11:09"
summary: "Living requirements: use cases and REQ blocks added over time. Read single items with `dust req show`, never the whole file."
signature:
  agent: user
  role: user
  signed: "2026-11-21 11:09"
  dust: 0.1.0
---
## How this file works
One living file, added to over time. Sections (`##`) are free-form areas; add new ones as the project grows (`dust req add ... --section "Sync"`). Don't read it whole: use `dust req list`, `dust req find "<keywords>"` and `dust req show <ids>`.

Status: **proposed** (suggested, not decided) · **needs-info** (waiting on a named question) · **approved** (the user said yes, this matters) · **in-progress** · **implemented** (built, not yet demonstrated) · **satisfied** (built and demonstrated, with evidence: test path::name, QA case or demo note) · **deferred** · **dropped**.
Only the user moves a requirement to approved. Priority: must, should, could, wont.

## Scope
<!-- What this file covers and links to the fact-finding documents it draws on. -->

## Use cases
<!-- One block per use case; requirements cite them as UC-n in Source.

### UC-01 Short verb-phrase title
- **Actor:** who
- **Goal:** what they want to achieve
- **Preconditions:** what must already be true
- **Main flow:** 1. ... 2. ... 3. ...
- **Alternates and errors:** what can go differently, and what should happen
-->

## Requirements
<!-- Add with `dust req add "<title>" --statement "The system shall ..." --source "Q-3, UC-1" --accept "Given..., when..., then..."`.
IDs are allocated for you and never reused. One testable behaviour per requirement. The stored block looks like:

### REQ-001 Short title
- **Statement:** The system shall ... (or "When <trigger>, the system shall ...")
- **Priority:** must
- **Status:** proposed
- **Source:** Q-003, UC-01 (where it came from: answer, use case, person or document)
- **Acceptance:**
  - Given <context>, when <action>, then <observable result>
- **Verification:** (set with `dust req set REQ-001 status satisfied --evidence "tests/test_x.py::test_req_001"`)
- **Notes:** (optional)
-->

## Out of scope (won't)

## Change log
<!-- YYYY-MM-DD, who, which REQ ids changed, and why. Approved requirements change only through an entry here. -->
- 2026-11-22, user and implementer: REQ-031 forest green is no longer optional. Per the user's request the Forest Green setting is removed from the alarm, sleep and Live light menus and from serial SET_ALARM/SET_SLEEP/START_LIGHT and GET reports; the Forest effect always renders its green. The former storage byte is kept as a reserved byte so existing alarm and sleep records load unchanged. REQ-003 menus show compact slot labels SLT1-SLT3 and ALM1-ALM5.
- 2026-11-22, user and implementer: REQ-002/003 local editing now saves every valid adjustment automatically, per the user's explicit request to remove Save from all menus; REQ-061 records this shared behavior. This supersedes explicit clock Save/cancel and alarm/sleep draft-commit navigation. Back and inactivity leave successful changes intact; validation and storage failures remain visible. RTC edits clamp shorter-month days and preserve live untouched fields. Native alarm/sleep edits persisted through reboot and all original settings were restored.
- 2026-11-22, user and implementer: REQ-033 temporary tap dimming is now non-stacking: only the first tap halves light/audio during an interaction-wake interval; further taps or encoder interactions only restart/extend the timer. Expiry still restores the ongoing envelopes and leaves manual Dim Half unchanged. Raise the default disturbance threshold from 80 to the explicitly requested 120 milli-g. User authorized stopping the active test for upload; latest saved presets remain unchanged.
- 2026-11-22, user and implementer: REQ-033 tap attenuation is temporary for the same interval as REQ-047 display wake, rather than lasting for the run. Each tap halves again and restarts the interval; encoder interaction extends it. Expiry removes only tap attenuation and restores the current light/audio envelopes without restart; separate alarm Dim Half remains persistent. Raise the default disturbance threshold from 50 to 80 milli-g in response to table bumps. User authorized stopping the active whale test for upload; latest saved presets, including slot-1 volume 50%, are retained.
- 2026-11-22, user and implementer: physical click detection woke the display and halved LEDs, but the user reported needing to lift/drop the device. Replace its per-axis click threshold with a gravity-filtered combined X/Y/Z disturbance detector, default 50 milli-g, with live diagnostic sensitivity tuning. This implements the user's requested aggregate three-axis sensitivity without attempting drifting position integration; wake and reduction semantics are unchanged.
- 2026-11-22, user and implementer: REQ-033 now applies to both alarms and sleep timers: each accepted tap halves the ongoing light/audio envelopes without snoozing, changing their deadlines or editing saved presets. REQ-047 wake duration is user configurable (1-300 seconds, default 30) through local Dark/Wake and serial controls, persisted in BNSA version 3 with version-1/2 migration. Physical taps and serial TAP share one handler; front-button snoozing remains separate.
- 2026-11-22, user and implementer: user requested 20-percent audio and chose to leave the current Ocean test at 5 percent while preparing the change afterward. REQ-059's temporary digital commissioning cap is raised from 10 to 20 percent; its unverified calibrated maximum remains pending. Live serial volume adjustments preserve the current alarm/sleep playback timeline and do not edit saved settings.
- 2026-11-22, user and implementer: REQ-015/016/040/060 clarification includes the fixed 30-second sleep rise within Hold, so total duration is exactly Hold + Fade Out. Hold zero skips the rise; zero total completes immediately without starting audio. Existing saved numeric Hold/Fade values and record versions are retained. Alarm timing is unchanged. Sleep slot 1 is explicitly saved as Hold 10/Fade Out 10/Nebula/WhaleSeaSleep with both outputs, total 1200 seconds. REQ-003 menu presentation now uses full scrolling words with endpoint pauses, retaining value-only numeric editors; standalone light ownership conflicts are distinguished from failures.
- 2026-11-22, user and implementer: REQ-009/015/030/040/060 timing amendment adds independent Hold after the rise. Alarm audio rises in the second half of Fade In; both hold at peak; audio falls in the first half of Fade Out while lights fall across the full period. Sleep initially used a separate fixed 30-second rise, user-set Hold and Fade Out (superseded by inclusive Hold above); lights fall in the first half while audio uses the full fade. BNAL version 4 derives legacy Hold = Run minus Fade Out, including recovery snapshots; BNSL version 2 derives legacy Hold = Duration minus old Fade In minus Fade Out. Standalone light timing is unchanged. Exact boundaries and migrations are covered by the alarm and sleep hold tests.
- 2026-11-22, user: clarified REQ-039 as two physical potentiometers, not rotary encoders. They are not wired yet; implementation is deferred until soldering and GPIO mapping confirmation. Approved soft pickup of the current brightness/volume, followed by 0-200% scaling of the ongoing output with existing safety caps.
- 2026-11-22, user and implementer: REQ-030 now delays alarm audio until halfway through Fade In, then ramps it to the same peak as light. The user also changed sleep light fade-out to finish halfway through Fade Out while audio continues to expiry (REQ-015/040/060 timing clarification). Sleep fade-in, alarm fade-out and standalone light durations remain unchanged; no new menu setting or record format is introduced.
- 2026-11-22, user and implementer: REQ-009/030/060 implementation now uses one shared Fade In and one shared Fade Out for light/audio, per the user's explicit request; existing light durations govern legacy migration. REQ-016 presentation now uses forced-dark sleep with interaction-woken clock/countdown alternation. User also requested three saved sleep slots, calmer scenes and one-time Goodnight followed by suppression until disturbance or the next upcoming noon. Existing acceptance statements are preserved; details and evidence are in README.md.
- 2026-11-22, implementer: corrected stale pending notes for REQ-037/053/017 to match their already-attached sleep/audio verification.
- 2026-11-22, user: temporarily removed the light power-budget restriction and requested brightness below 50%. REQ-038 is replaced by REQ-058 for light and REQ-059 for the unchanged audio limit.
- 2026-11-21, Technical Analyst: dropped REQ-022 in favor of REQ-030 per the owner's Q-019 answer; audio ramps before the alarm time, rather than starting at it.
- 2026-11-21, user and Technical Analyst: the owner added the active-alarm recovery exception to REQ-049 during review; corrected its acceptance criteria to match the approved statement.
- 2026-11-21, user: added a stable HH:MM display with the active hour/minute digits flashing after physical clock-editor testing found scrolling labels hard to read.

## Clock and controls

### REQ-001 Display current time
- **Statement:** The system shall display the current RTC time on the four-digit display in a user-selected 12-hour or 24-hour format.
- **Priority:** must
- **Status:** in-progress
- **Source:** User request; 20261121-1108-legacy-feature-inventory-and-rewrite-decisions: Clock
- **Acceptance:**
  - Given a valid RTC and a selected display format, when the time changes, then the displayed hours and minutes reflect the current time in that format.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-002 Set time and date
- **Statement:** The system shall let the user set and save the clock time and date using the local interface.
- **Priority:** must
- **Status:** implemented
- **Source:** Legacy feature inventory: Clock
- **Acceptance:**
  - Given a valid time/date field adjustment through the interface, when the encoder changes it, then the RTC and displayed time use the new value immediately without a separate Save action.
  - Given successful field adjustments, when the user goes Back or the menu times out, then those RTC changes remain in effect.
  - Given the user is editing the hour or minute, when that field is active, then the display shows HH:MM and flashes the active numeric pair instead of scrolling its label.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-003 Navigate local menus
- **Statement:** The system shall provide encoder-operated menus for clock, alarms, sleep timer, light effects, and system settings, with a way to return to the clock display.
- **Priority:** must
- **Status:** implemented
- **Source:** User request; Legacy feature inventory: Menus
- **Acceptance:**
  - Given the clock display, when the user operates the encoder, then each named menu category can be reached and exited back to the clock.
  - Given an alarm or sleep settings editor, when the user adjusts a valid setting, then it saves immediately without a Save item or an intervening success screen.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-004 Adjust display brightness
- **Statement:** The system shall allow the user to adjust the seven-segment display brightness.
- **Priority:** should
- **Status:** approved
- **Source:** Legacy feature inventory: Menus
- **Acceptance:**
  - Given the display settings, when the user selects and saves another supported brightness, then the display uses that brightness.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-005 Configure dark hours
- **Statement:** The system shall support configurable dark hours during which the time display is blank and can be temporarily awakened by a local control.
- **Priority:** should
- **Status:** approved
- **Source:** Legacy feature inventory: Dark hours; Q-012
- **Acceptance:**
  - Given configured dark hours spanning midnight, when the clock is inside the interval, then the display is blank until a local control wakes it temporarily.
  - Given a time outside the interval, when the clock display is selected, then it is visible normally.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-020 Use physical controls
- **Statement:** The system shall support the rotary encoder, illuminated front button and both potentiometers as local controls.
- **Priority:** must
- **Status:** in-progress
- **Source:** User hardware description; Q-002; Q-009
- **Acceptance:**
  - Given each connected control, when the user operates it, then the system responds according to its assigned function without requiring a network or companion device.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-034 Front button alarm and back behavior
- **Statement:** The system shall flash the front button indicator during an active alarm and use the button as a back control when no alarm is active.
- **Priority:** must
- **Status:** in-progress
- **Source:** Q-002
- **Acceptance:**
  - Given an active alarm, when its indicator is observed, then the front button flashes.
  - Given a menu and no active alarm, when the front button is pressed, then the interface returns to the preceding menu or clock display.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-039 Context-sensitive potentiometers
- **Statement:** The system shall use the two potentiometers as context-sensitive controls for audio volume and light brightness during an active alarm or sleep timer.
- **Priority:** must
- **Status:** deferred
- **Source:** Q-009; user clarification 2026-11-22
- **Acceptance:**
  - Given an active sleep timer or alarm with audio and light, when the corresponding potentiometer is adjusted, then the audio volume or light brightness changes without requiring menu navigation.
  - Given a newly active alarm or sleep timer, stationary potentiometers do not change its outputs; each control engages only after movement catches its current instantaneous brightness or volume.
  - After pickup, the brightness and volume controls independently scale the ongoing animation/audio from 0-200%, preserving scheduled fades and the existing LED/audio safety caps rather than changing saved presets.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** User explicitly deferred implementation until the two potentiometers are soldered. Confirm which ADC GPIO drives brightness and which drives volume before enabling reads; current POT_1_PIN/POT_2_PIN definitions are placeholders and must not be used as verified wiring.

### REQ-047 Wake dark display by tap
- **Statement:** The system shall temporarily wake the display when a tap is detected during dark hours.
- **Priority:** should
- **Status:** satisfied
- **Source:** Q-012; user clarification 2026-11-22
- **Acceptance:**
  - Given a blank display during dark hours, when the device detects a tap, then it temporarily shows the clock display before returning to the dark-hours state.
  - Given a user-selected wake duration, when a tap wakes the display, then it returns to dark mode after that duration unless another interaction restarts the wake interval; the setting survives restart.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Clock_Command_Test.cpp::test_tap_filter_and_serial_wake_setting; tests/Menu_System_Test.cpp::test_configurable_wake_editor_and_wraparound_expiry; tests/Settings_Storage_Test.cpp::test_wake_duration_migrates_v2_and_survives_restart. Feather demo 2026-11-22: user confirmed both gentle taps woke the display; diagnostic wake expired after three seconds and saved duration survived firmware reboot. See README.md filtered tap verification and session tap-device-evidence.json.

### REQ-050 Indicate RTC reset required
- **Statement:** When the RTC reports loss of valid time, the system shall indicate that the clock needs to be set.
- **Priority:** must
- **Status:** implemented
- **Source:** Q-018
- **Acceptance:**
  - Given an RTC reporting lost or invalid time, when the device starts, then the user is told that the clock needs to be reset.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-061 Save local edits automatically
- **Statement:** The system shall automatically persist each valid local clock, alarm, sleep timer and system setting adjustment without a separate Save menu action.
- **Priority:** must
- **Status:** satisfied
- **Source:** User request 2026-11-22: remove Save from every menu and save changes immediately
- **Acceptance:**
  - Given a valid menu adjustment, when the encoder changes a value or confirms a sound/day choice, then the setting is saved immediately without leaving the editor for a success screen.
  - Given automatically saved edits, when Back, the one-minute menu timeout, or a reboot occurs, then the saved values remain in effect.
  - Given an invalid adjustment or failed storage write, when automatic saving fails, then the device shows an error instead of reporting success and retains the last confirmed settings.
- **Added:** 2026-11-22 by user
- **Verification:** tests/Menu_System_Test.cpp::test_alarm_and_sleep_fields_autosave_and_survive_idle_reload; tests/Menu_System_Test.cpp::test_autosave_failures_show_errors_and_restore_confirmed_values; tests/Menu_System_Test.cpp::test_req_002_local_editor_saves_each_adjustment; tests/Menu_System_Test.cpp::test_clock_autosave_clamps_short_months_and_leap_years; Feather native encoder demo 2026-11-22: alarm volume and sleep Hold persisted without Save across reboot, clock seconds immediate; all original settings restored; README.md Automatic-save verification; session autosave-device-evidence.json

## Alarms

### REQ-006 Manage multiple alarms
- **Statement:** The system shall let the user configure and independently enable or disable at least five alarms.
- **Priority:** must
- **Status:** satisfied
- **Source:** Legacy feature inventory: Alarm; Q-006
- **Acceptance:**
  - Given five configured alarms, when one is disabled, then the other four retain their settings and enabled states.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Alarm_System_Test.cpp::test_saved_slots_and_days

### REQ-007 Schedule recurring alarms
- **Statement:** The system shall support alarms scheduled for selected days of the week.
- **Priority:** must
- **Status:** satisfied
- **Source:** Legacy feature inventory: Alarm; Q-006
- **Acceptance:**
  - Given an enabled alarm for Monday and Friday, when its scheduled time occurs on those days, then it triggers; when that time occurs on another day, it does not trigger.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Alarm_System_Test.cpp::test_saved_slots_and_days

### REQ-008 Schedule one-shot alarms
- **Statement:** The system shall support a one-shot alarm that disables itself after it triggers.
- **Priority:** must
- **Status:** satisfied
- **Source:** Legacy feature inventory: Alarm; Q-006
- **Acceptance:**
  - Given an enabled one-shot alarm, when its scheduled time is reached, then it triggers once and remains disabled on subsequent days and after restart.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Alarm_System_Test.cpp::test_ramp_one_shot_and_recovery

### REQ-009 Configure alarm outputs
- **Statement:** The system shall allow each alarm to select its light effect, sound, output volume and run duration.
- **Priority:** must
- **Status:** satisfied
- **Source:** Legacy feature inventory: Alarm; Q-004
- **Acceptance:**
  - Given two alarms with different selected outputs, when each is triggered separately, then it uses its own saved effect, sound, volume and duration.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** Saved effects, WAV paths, volumes, durations and shared light/audio fades survive record migration/reload. Versions 1/2 migrate to version 3 using the existing light durations, including restart snapshots. Host cases and native encoder/WAV alarm checks passed on 2026-11-22; see README.md Alarm audio integration verification.
- **Verification:** tests/Alarm_System_Test.cpp::test_req_009_saved_sound_volume_duration_and_fade_out

### REQ-010 Snooze and dismiss alarms
- **Statement:** The system shall provide distinct actions to snooze an active alarm and dismiss it.
- **Priority:** must
- **Status:** satisfied
- **Source:** Legacy feature inventory: Ideas; Q-002; Q-003; Q-005
- **Acceptance:**
  - Given an active alarm, when snooze is requested, then its outputs pause and the alarm resumes after the configured snooze interval.
  - Given an active or snoozed alarm, when dismissal is requested, then its outputs stop and that occurrence does not resume.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Alarm_System_Test.cpp::test_snooze_dismiss_and_button

### REQ-022 Coordinate sunrise and alarm time
- **Statement:** The system shall complete the configured sunrise at the alarm time and start the selected alarm sound at that time.
- **Priority:** must
- **Status:** dropped
- **Source:** Q-004; Legacy feature inventory: Alarm
- **Acceptance:**
  - Given a 30-minute sunrise and an alarm set for 07:00, when the occurrence runs, then sunrise starts at 06:30 and reaches its target as sound begins at 07:00.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-023 Alarm when audio is unavailable
- **Statement:** When the selected alarm audio or its SD card is unavailable, the system shall indicate the fault and run the configured alarm light.
- **Priority:** must
- **Status:** in-progress
- **Source:** Q-017; Legacy feature inventory: Audio
- **Acceptance:**
  - Given an enabled alarm with light and unavailable selected audio, when it triggers, then the light runs and the audio fault is indicated.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** Native missing selected WAV reports FILE and alternating AErr/time while configured light continues; no repeated start attempts. Cold startup without SD cannot load the saved alarm bank, and storage write failures block new starts. Full card-unavailability behavior remains open.

### REQ-030 Ramp light and sound to alarm time
- **Statement:** The system shall begin the configured alarm light and audio ramps before the alarm time so that both reach their configured peak at the alarm time.
- **Priority:** must
- **Status:** satisfied
- **Source:** Q-001; Q-004; Q-019; user timing amendment 2026-11-22
- **Acceptance:**
  - Given an alarm with a 30-minute Fade In and alarm time 07:00, when the occurrence runs, then light begins at 06:30, audio begins at 06:45, and both reach their configured peaks at 07:00.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** One setting still controls both alarm fade-ins; audio uses its second half, including recovered snapshots. Host tests cover the five-minute example, 30-minute schedule and zero fade-in. Alarm fade-out remains shared over the full period.
- **Verification:** tests/Alarm_System_Test.cpp::test_req_030_thirty_minute_light_and_audio_ramps; tests/Alarm_System_Test.cpp::test_audio_settings_migration_and_shared_ramps; README.md::Staggered timing verification

### REQ-033 Reduce outputs on tap
- **Statement:** When the device is tapped during an active alarm or sleep timer, the system shall temporarily apply one non-stacking 50-percent reduction to light brightness and audio volume for the display-wake interval without stopping or snoozing it.
- **Priority:** must
- **Status:** satisfied
- **Source:** Q-003; user clarification 2026-11-22
- **Acceptance:**
  - Given an active alarm or sleep timer with light and audio outputs and no tap attenuation, when a tap is detected, then each current output level becomes 50 percent of its scheduled level, the run remains active without snoozing, and its fade timing and saved configuration are unchanged.
  - Given temporary tap attenuation, when the configurable interaction-wake interval expires, then tap attenuation clears and both outputs follow their current scheduled envelopes without restart; separate manual Dim Half remains applied.
  - Given an active tap attenuation interval, when another tap or encoder interaction wakes the display, then its expiry is extended to match the new display-wake interval without any further reduction.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Sleep_Timer_Test.cpp::test_taps_halve_sleep_without_restarting_fades_and_retry_audio; tests/Alarm_System_Test.cpp::test_temporary_tap_restore_preserves_manual_dim_and_fade. Feather demo 2026-11-22: first diagnostic tap changed gain/white 200/8 -> 100/4; repeated taps kept 100/4, extended expiry beyond the initial interval and did not enqueue further audio reductions. Encoder extension, expiry restoration to 200/8, and a fresh dim interval after expiry passed with zero underruns. Device default reports 120 milli-g. Saved presets and wake duration were unchanged. Earlier physical taps verified the shared handler; see README.md non-stacking tap verification and session single-tap-device-evidence.json.
- **Notes:** Temporary tap attenuation is one non-stacking half, tracked separately from persistent alarm Dim Half and cleared from the same wrap-safe wake state used by the display. Combined X/Y/Z disturbances use gravity calibration, a median-of-three spike filter and a 400-ms ringing cooldown. Sensitivity now defaults to 120 milli-g; diagnostic threshold changes are runtime-only. Table-bump immunity depends on mounting/surface and is not guaranteed by the threshold change.

### REQ-035 Hold front button to dismiss
- **Statement:** The system shall dismiss an active alarm when the front button is held for two seconds and display a hold reminder while it is pressed.
- **Priority:** must
- **Status:** in-progress
- **Source:** Q-005
- **Acceptance:**
  - Given an active alarm, when the front button is pressed, then the display shows a hold reminder; when it is held for two seconds, then the alarm is dismissed.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** Two-second hold and reminder are implemented and host-tested; the front button remains unwired and its physical path is unverified.

### REQ-036 Five-minute snooze
- **Statement:** The system shall resume a snoozed alarm five minutes after snooze is requested.
- **Priority:** must
- **Status:** satisfied
- **Source:** Q-005
- **Acceptance:**
  - Given an active alarm, when snooze is requested, then its outputs pause and resume five minutes later.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** README.md::Alarm audio integration verification (real five-minute pause and frozen-gain resume)

### REQ-037 Alarm output priority
- **Statement:** The system shall give a newly occurring alarm priority over an active sleep timer or earlier alarm; simultaneous alarms shall use the lowest-numbered slot.
- **Priority:** must
- **Status:** satisfied
- **Source:** Q-010
- **Acceptance:**
  - Given a sleep timer or earlier alarm using the outputs, when a new alarm occurs, then the new alarm controls the outputs.
  - Given two alarms occurring at the same time, when they trigger, then the lower-numbered slot controls the outputs.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** Earlier-alarm/manual-light preemption, lowest-slot simultaneous priority and sleep-timer preemption are host- and device-tested.
- **Verification:** tests/Alarm_System_Test.cpp::test_priority_clock_changes_and_invalid_time; tests/Sleep_Timer_Test.cpp::test_req_037_048_053_preemption_stop_and_no_resume; README.md sleep verification 2026-11-22: scheduled alarm preempted sleep

### REQ-046 Scheduled alarm timing tolerance
- **Statement:** The system shall start an enabled scheduled alarm within one second of its intended trigger time when the RTC time is valid and the device is powered.
- **Priority:** must
- **Status:** satisfied
- **Source:** Q-011
- **Acceptance:**
  - Given a valid RTC and a powered device with a scheduled alarm, when its intended start time is reached, then the alarm starts no more than one second before or after that time.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** README.md::Alarms (2026-11-22 starts within one second)

### REQ-049 Do not replay missed alarms
- **Statement:** The system shall not automatically replay alarm occurrences missed while the device was unpowered, unless the alarm would still be playing currently when power is regained. If so jump to the proper point in the volume and brightness ramping.
- **Priority:** must
- **Status:** satisfied
- **Source:** Q-018; user edit during 2026-11-21 review
- **Acceptance:**
  - Given an alarm occurrence that would have finished while the device was unpowered, when power is restored, then that missed occurrence does not start automatically.
  - Given an alarm occurrence that would still be active when power is restored, when the device restarts during the ramps, then the alarm resumes at the corresponding volume and brightness for the elapsed time.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** Expired occurrences remain suppressed. Native mid-ramp reboot recovered both outputs at elapsed gain/brightness with the immutable active WAV snapshot; snoozed reboot did not replay. Audio content seeks to elapsed position modulo file duration. Verified 2026-11-22.
- **Verification:** README.md::Alarm audio integration verification

### REQ-051 Front button snooze indication
- **Statement:** When an alarm is active, the system shall snooze it on a short press of the front button and alternate the display between the current time and SNZ while snoozed.
- **Priority:** must
- **Status:** in-progress
- **Source:** User decision 2026-11-21: front button snooze and clear indication
- **Acceptance:**
  - Given an active alarm, when the front button is briefly pressed and released, then its outputs pause for five minutes and the display alternates between the current time and SNZ while the occurrence remains snoozed.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** Short-release snooze is host-tested; time/SNZ alternation and real five-minute serial/menu snooze are device-tested. Physical front-button verification awaits wiring.

### REQ-052 Two-second dismissal reminder
- **Statement:** While the front button is held during an active alarm, the system shall display a reminder that holding it for two seconds fully dismisses the alarm.
- **Priority:** must
- **Status:** in-progress
- **Source:** User decision 2026-11-21: explicit two-second hold reminder
- **Acceptance:**
  - Given an active alarm, while the front button is pressed, the display indicates that holding for two seconds fully dismisses it; a two-second hold dismisses rather than snoozes the occurrence.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** HLD2 reminder and two-second hold-to-dismiss are implemented and host-tested; physical front-button/display verification awaits wiring.

### REQ-053 Do not resume preempted outputs
- **Statement:** When a new alarm takes priority, the system shall not resume the earlier alarm or sleep timer after the new alarm ends.
- **Priority:** must
- **Status:** satisfied
- **Source:** User clarification 2026-11-21: only the most recent alarm matters
- **Acceptance:**
  - Given an earlier alarm or sleep timer, when a new alarm preempts it and subsequently ends, then the preempted output does not resume.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** Preempted earlier alarms, manual lighting and sleep timers do not resume, verified by host and device checks.
- **Verification:** tests/Alarm_System_Test.cpp::test_priority_clock_changes_and_invalid_time; tests/Sleep_Timer_Test.cpp::test_req_037_048_053_preemption_stop_and_no_resume; README.md sleep verification 2026-11-22: no sleep after alarm dismissal

### REQ-054 End snoozed occurrence on power loss
- **Statement:** When power is lost while an alarm is snoozed, the system shall consider that occurrence finished and shall not resume it after restart.
- **Priority:** must
- **Status:** satisfied
- **Source:** User clarification 2026-11-21: a snoozed alarm is done if power is lost
- **Acceptance:**
  - Given a snoozed alarm, when power is lost and later restored, then that occurrence does not resume.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** README.md::Alarm audio integration verification (no replay after snoozed restart)

### REQ-055 Do not trigger alarms from clock changes
- **Statement:** When the user sets the clock forward into an alarm occurrence's active window, the system shall not start that occurrence as a result of the clock change.
- **Priority:** must
- **Status:** satisfied
- **Source:** User clarification 2026-11-21: clock changes do not start an already-active occurrence
- **Acceptance:**
  - Given an enabled alarm, when the clock is set forward into its active window, then that occurrence does not start as a result of the clock change.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Alarm_System_Test.cpp::test_priority_clock_changes_and_invalid_time

## Light effects

### REQ-011 Provide sunrise effect
- **Statement:** The system shall provide a gradual sunrise effect using the addressable light panel.
- **Priority:** must
- **Status:** implemented
- **Source:** User request; Legacy feature inventory: Light; Q-004
- **Acceptance:**
  - Given a configured sunrise, when it runs, then the panel transitions over its configured duration from a dim warm appearance to a brighter appearance.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-012 Provide simple light effects
- **Statement:** The system shall provide both a fixed-color effect and a breathing effect in addition to sunrise.
- **Priority:** should
- **Status:** in-progress
- **Source:** Legacy feature inventory: Light; Q-007
- **Acceptance:**
  - Given fixed-color mode, when it runs, then the panel holds the selected color; given breathing mode, when it runs, then its light output repeatedly rises and falls.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-013 Configure light presets
- **Statement:** The system shall let the user save and later select configurable light presets with an effect, color and brightness.
- **Priority:** must
- **Status:** approved
- **Source:** Legacy feature inventory: Light
- **Acceptance:**
  - Given a configured light preset, when the user saves it and selects it later, then the selected effect, color and brightness are restored.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-014 Preview light settings
- **Statement:** The system shall show a light preview while the user adjusts a preset color or brightness.
- **Priority:** should
- **Status:** approved
- **Source:** Legacy feature inventory: Light
- **Acceptance:**
  - Given the preset editor, when the user changes its color or brightness, then the panel reflects the adjustment before it is saved.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-021 Drive RGBW panel
- **Statement:** The system shall control the existing RGBW light panel with independently selectable red, green, blue and white output components.
- **Priority:** must
- **Status:** implemented
- **Source:** Legacy feature inventory: Hardware inventory and conflicts; Q-013
- **Acceptance:**
  - Given verified panel wiring and channel order, when each color component is selected independently, then the intended physical red, green, blue or white channel illuminates.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-031 Forest light animation
- **Statement:** The system shall offer a dappled forest-light effect with a soft flicker using white and yellow light with green always present.
- **Priority:** must
- **Status:** implemented
- **Source:** Q-007
- **Acceptance:**
  - Given the dappled forest effect, when it runs, then the panel produces varying soft light in white and yellow tones with scattered green rather than a static single color, and no setting disables the green.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-032 Fade to selected color
- **Statement:** The system shall offer a light effect that gradually fades in to a user-selected color.
- **Priority:** must
- **Status:** approved
- **Source:** Q-007
- **Acceptance:**
  - Given the fade-to-color effect and a selected color, when it runs, then the light increases gradually from dim to that color.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-060 Selectable light fade durations
- **Statement:** The system shall allow light fade-in and fade-out durations to be selected independently from 0, 1, 5, 10, 15, 20, 30, 45, or 60 minutes.
- **Priority:** must
- **Status:** implemented
- **Source:** User request 2026-11-22
- **Acceptance:**
  - Given a supported duration, when selected for fade-in or fade-out, the effect ramps over that duration; zero makes the transition immediate.
- **Added:** 2026-11-22 by user
- **Notes:** User timing amendment 2026-11-22: selectable Fade In applies to standalone lights and alarms; sleep is the explicit exception with a fixed 30-second rise included within Hold (skipped for zero Hold). Alarm Fade Out defines the full light period, with audio using its first half. Sleep Fade Out defines the full audio period, with light using its first half and remaining black through expiry. Sleep total is Hold + Fade Out; alarm total remains Fade In + Hold + Fade Out.

## Sleep timer

### REQ-015 Start and stop sleep timer
- **Statement:** The system shall allow the user to start and stop a sleep timer with a selected duration and light or audio output.
- **Priority:** must
- **Status:** satisfied
- **Source:** Legacy feature inventory: Sleep; Q-008
- **Acceptance:**
  - Given a selected duration and output, when the timer is started, then the selected output runs until expiry or user cancellation and stops afterward.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Sleep_Timer_Test.cpp::test_req_015_040_outputs_expiry_and_synchronized_fades; tests/Sleep_Timer_Test.cpp::test_sleep_hold_zero_phases_and_longest_timer; README.md inclusive sleep Hold verification 2026-11-22: exact 1200-second host envelope/deadline, uploaded slot 1 reports 1200 seconds after reboot, short native muted whale playback/stop. Earlier native Start/Stop and real expiry remain historical evidence.

### REQ-016 Show sleep countdown
- **Statement:** The system shall make the remaining sleep-timer time available on the clock display while the timer is active.
- **Priority:** should
- **Status:** satisfied
- **Source:** Legacy feature inventory: Sleep
- **Acceptance:**
  - Given an active sleep timer, when the user views the timer status, then the display indicates its remaining time and distinguishes it from the current clock time.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Menu_System_Test.cpp::test_req_015_016_040_sleep_menu_edits_audio_selection_and_actions; tests/Menu_System_Test.cpp::test_sleep_dark_wake_goodnight_and_next_noon; README.md saved sleep/shared fade verification 2026-11-22: interaction-woken clock/countdown alternation

### REQ-040 Choose sleep outputs independently
- **Statement:** The system shall allow a sleep timer to run light only, audio only, or both together.
- **Priority:** must
- **Status:** satisfied
- **Source:** Q-008
- **Acceptance:**
  - Given an available light preset and audio file, when the user selects light only, audio only, or both, then only the selected outputs run and they stop at timer expiry.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Sleep_Timer_Test.cpp::test_req_015_040_outputs_expiry_and_synchronized_fades; README.md sleep verification 2026-11-22: all three output selections

### REQ-048 Do not resume sleep after restart
- **Statement:** The system shall not resume a previously running sleep timer after a power cycle.
- **Priority:** should
- **Status:** satisfied
- **Source:** Q-016
- **Acceptance:**
  - Given an active sleep timer, when the device is power-cycled, then the previous sleep timer and its outputs do not resume automatically.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Sleep_Timer_Test.cpp::test_req_037_048_053_preemption_stop_and_no_resume; README.md sleep verification 2026-11-22: identical-firmware restart left outputs off

## Audio and storage

### REQ-017 Play user audio
- **Statement:** The system shall allow the user to select and play compatible WAV audio from the SD card for alarms and the sleep timer.
- **Priority:** must
- **Status:** satisfied
- **Source:** Legacy feature inventory: Audio; Q-015
- **Acceptance:**
  - Given two compatible WAV files on the SD card, when each is selected in turn for an alarm or sleep timer, then the chosen file is played through the speaker.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** Core-1 WAV streaming, alarm/sleep file selection, looping and volume ramps are device-verified; owner previously confirmed audible Ooblets playback. Fade durations are now shared with light. README.md documents input formats, 16-bit output resolution and measured throughput limits.
- **Verification:** tests/Audio_Playback_Test.cpp; README.md alarm audio and sleep verification 2026-11-22: selected Ooblets and PCM fixtures streamed through core-1 I2S

### REQ-018 Preserve saved settings
- **Statement:** The system shall restore saved alarm, light preset and system settings after a power cycle.
- **Priority:** must
- **Status:** in-progress
- **Source:** Legacy feature inventory: Persistence; Q-016
- **Acceptance:**
  - Given saved settings including alarm output selections and a light preset, when power is cycled with storage available, then those settings are restored unchanged.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** Alarm and system records are implemented and reboot-tested; saved standalone light presets and saved audio selections remain pending.

### REQ-019 Report unavailable resources
- **Statement:** The system shall indicate when a required storage device, selected audio file, or valid clock time is unavailable.
- **Priority:** must
- **Status:** in-progress
- **Source:** Legacy feature inventory: Audio and persistence; Q-017; Q-018
- **Acceptance:**
  - Given missing SD storage, a missing selected file, or invalid RTC time, when the condition is detected, then the user receives an indication identifying the problem.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Notes:** Storage/RTC diagnostics and PCM read/output error tests remain available. Native alarm missing-file faults now display AErr/time with light continuing, plus FILE in serial status. Missing/invalid-file device checks passed. Physical card removal remains unverified.

## Serial diagnostics and control

### REQ-024 USB serial commands
- **Statement:** The system shall accept human-readable commands over USB serial to set the RTC time and start a selected alarm slot. These commands must encompass the ability to set/change any and all settings as well as stear the device by sending moce encoder turn and click commands. The API must also provide a "read screen" command that returns the text currently displayed on the seven segment display as well as a "read led" command that will return the values of all the LEDs in the matrix.
- **Priority:** must
- **Status:** in-progress
- **Source:** User request 2026-11-21: serial logging, monitoring and control
- **Acceptance:**
  - Given a connected serial client and a valid time command, when the client sends the command, then the RTC is updated and the client receives confirmation of the resulting time.
  - Given a configured alarm slot, when the client sends a valid start command naming that slot, then that slot starts and the client receives confirmation identifying the slot.
- **Added:** 2026-11-21 by user

### REQ-025 Shared control actions
- **Statement:** The system shall route equivalent local-menu and serial actions through the same application-level commands.
- **Priority:** must
- **Status:** in-progress
- **Source:** User request 2026-11-21: decouple menus and serial control
- **Acceptance:**
  - Given the same valid RTC time, when it is set once through the menu and once through serial, then both operations update the RTC with the same resulting value and apply the same validation.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-026 Serial command results
- **Statement:** The system shall return an explicit success or error response for each serial command or significant action in the device. For example, this could be the initiation of an alarm, pressing of buttons (and the action taken in response to that button etc). This should allow the USB serial port to be used for debugging of the physical hardware features.
- **Priority:** must
- **Status:** in-progress
- **Source:** User request 2026-11-21: command responses for hardware testing
- **Acceptance:**
  - Given a valid command, when it completes, then the client receives a success response identifying the action and result.
  - Given an unknown command, malformed time or invalid alarm slot, when it is submitted, then the client receives an error and the corresponding clock or alarm state remains unchanged.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-027 Timestamped serial event log
- **Statement:** The system shall emit timestamped serial log messages for RTC time changes and alarm start events. This should allow the USB serial port to be used for debugging of the physical hardware features.
- **Priority:** must
- **Status:** in-progress
- **Source:** User request 2026-11-21: timestamped logging and monitor
- **Acceptance:**
  - Given a valid RTC, when the time is changed or an alarm starts, then a serial monitor receives an event identifying the action and its date and time.
  - Given invalid RTC time, when an event is logged, then the log indicates that the timestamp is invalid instead of presenting it as valid.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-028 Query device state over serial
- **Statement:** The system shall provide serial commands to read the current RTC time and the state of a selected alarm slot. This should allow the USB serial port to be used for debugging of the physical hardware features.
- **Priority:** must
- **Status:** satisfied
- **Source:** User request 2026-11-21: serial monitoring and agent hardware testing
- **Acceptance:**
  - Given a connected serial client, when it requests current time or a valid alarm slot state, then it receives the corresponding current value or state.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Clock_Command_Test.cpp::test_alarm_serial_settings_actions_and_queries

### REQ-029 Serial control hardware test
- **Statement:** The system shall support exercising and observing RTC setting and alarm activation from a USB serial client on the physical device.
- **Priority:** must
- **Status:** satisfied
- **Source:** User request 2026-11-21: agents test the hardware over USB serial
- **Acceptance:**
  - Given target hardware connected by USB, when a test client sets the RTC, queries it, starts a configured alarm and queries its state, then the serial responses and observed hardware behavior agree.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** README.md::Alarms (2026-11-22 USB serial hardware checks)

### REQ-041 Configure all settings over serial
- **Statement:** The system shall allow a serial client to read and change every user-adjustable device setting.
- **Priority:** must
- **Status:** in-progress
- **Source:** User edit to REQ-024 during 2026-11-21 review
- **Acceptance:**
  - Given each setting available through the local interface, when a serial client changes it to a valid value and reads it back, then the returned value and device behavior reflect that setting.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-042 Simulate encoder input over serial
- **Statement:** The system shall accept serial commands that simulate encoder rotation and click input.
- **Priority:** must
- **Status:** approved
- **Source:** User edit to REQ-024 during 2026-11-21 review
- **Acceptance:**
  - Given the same starting menu, when the client sends a clockwise turn, counterclockwise turn or click, then the interface responds as it does to the equivalent physical encoder input.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-043 Read display over serial
- **Statement:** The system shall provide a serial command that returns the currently displayed seven-segment text.
- **Priority:** must
- **Status:** satisfied
- **Source:** User edit to REQ-024 during 2026-11-21 review
- **Acceptance:**
  - Given a known menu or time shown on the display, when the client requests the screen, then the response represents the currently visible text.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
- **Verification:** tests/Clock_Command_Test.cpp::test_read_screen_and_invalid_time

### REQ-044 Read panel LEDs over serial
- **Statement:** The system shall provide a serial command returning the current output values of every light-panel LED.
- **Priority:** must
- **Status:** implemented
- **Source:** User edit to REQ-024 during 2026-11-21 review
- **Acceptance:**
  - Given a known 64-pixel RGBW panel output, when the client requests LED values, then the response contains the current red, green, blue and white output value for each of the 64 pixels.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-045 Log physical input actions
- **Statement:** The system shall log physical button and encoder input events and the resulting actions over USB serial.
- **Priority:** must
- **Status:** approved
- **Source:** User edit to REQ-026 during 2026-11-21 review
- **Acceptance:**
  - Given a connected serial monitor, when a physical button is pressed or the encoder is turned or clicked, then the monitor receives an event identifying the input and the action taken in response.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-056 Enable diagnostics at startup only
- **Statement:** The system shall enable serial diagnostics only when a serial connection is active at power-up, and shall keep diagnostics disabled when powered only from a wall socket.
- **Priority:** must
- **Status:** in-progress
- **Source:** User decisions 2026-11-21: serial diagnostics only when computer-connected at startup
- **Acceptance:**
  - Given power from a wall socket without an active serial connection, when the device starts, then serial command handling and diagnostic logging are disabled.
  - Given a serial connection active at power-up, when the device starts, then serial diagnostics are available.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-057 Permit diagnostic message loss under load
- **Statement:** The system shall prioritize normal alarm operation over serial diagnostic message delivery when performance requires messages to be dropped.
- **Priority:** must
- **Status:** in-progress
- **Source:** User decision 2026-11-21: messages may be dropped for performance
- **Acceptance:**
  - Given a slow serial client and constrained diagnostic output, when messages cannot be transmitted without affecting alarm operation, then the system can drop messages and continues to meet the alarm timing requirement.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

## Output limits

### REQ-038 Cap light and audio output
- **Statement:** The system shall limit light brightness and audio output to 80 percent of their respective verified actual limits.
- **Priority:** must
- **Status:** dropped
- **Source:** Q-014
- **Acceptance:**
  - Given verified light and audio maximums, when any interface requests output above 80 percent of either maximum, then the corresponding output remains at or below 80 percent.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-058 Temporary low panel brightness
- **Statement:** The system shall limit light-panel channel output to less than 50 percent of its maximum digital level while the power-budget restriction is temporarily removed.
- **Priority:** must
- **Status:** implemented
- **Source:** User request 2026-11-22
- **Acceptance:**
  - Given a request for light output, when the panel is driven, then every commanded channel value is below 50 percent of the digital full scale.
- **Added:** 2026-11-22 by user

### REQ-059 Cap audio output
- **Statement:** The system shall limit audio output to 80 percent of its verified actual limit.
- **Priority:** must
- **Status:** in-progress
- **Source:** Q-014; REQ-038; user change 2026-11-22
- **Acceptance:**
  - Given a verified audio maximum, when any interface requests audio above 80 percent of that maximum, then the audio output remains at or below 80 percent.
- **Added:** 2026-11-22 by user
- **Notes:** User initially requested commissioning tests at no more than 10 percent, then authorized 20 percent on 2026-11-22. Streaming controls reject gain above 20 percent, with sample peaks bounded to 6553/32768. The verified actual amplifier maximum and calibrated 80-percent limit are not established; this requirement is not yet satisfied.
