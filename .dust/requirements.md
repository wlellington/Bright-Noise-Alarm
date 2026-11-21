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
- 2026-11-21, Technical Analyst: dropped REQ-022 in favor of REQ-030 per the owner's Q-019 answer; audio ramps before the alarm time, rather than starting at it.
- 2026-11-21, user and Technical Analyst: the owner added the active-alarm recovery exception to REQ-049 during review; corrected its acceptance criteria to match the approved statement.

## Clock and controls

### REQ-001 Display current time
- **Statement:** The system shall display the current RTC time on the four-digit display in a user-selected 12-hour or 24-hour format.
- **Priority:** must
- **Status:** approved
- **Source:** User request; 20261121-1108-legacy-feature-inventory-and-rewrite-decisions: Clock
- **Acceptance:**
  - Given a valid RTC and a selected display format, when the time changes, then the displayed hours and minutes reflect the current time in that format.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-002 Set time and date
- **Statement:** The system shall let the user set and save the clock time and date using the local interface.
- **Priority:** must
- **Status:** approved
- **Source:** Legacy feature inventory: Clock
- **Acceptance:**
  - Given a valid time and date entered through the interface, when the user saves them, then the RTC and displayed time use the saved values.
  - Given an unsaved edit, when the user cancels it, then the stored RTC time and date remain unchanged.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-003 Navigate local menus
- **Statement:** The system shall provide encoder-operated menus for clock, alarms, sleep timer, light effects, and system settings, with a way to return to the clock display.
- **Priority:** must
- **Status:** approved
- **Source:** User request; Legacy feature inventory: Menus
- **Acceptance:**
  - Given the clock display, when the user operates the encoder, then each named menu category can be reached and exited back to the clock.
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
- **Status:** approved
- **Source:** User hardware description; Q-002; Q-009
- **Acceptance:**
  - Given each connected control, when the user operates it, then the system responds according to its assigned function without requiring a network or companion device.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-034 Front button alarm and back behavior
- **Statement:** The system shall flash the front button indicator during an active alarm and use the button as a back control when no alarm is active.
- **Priority:** must
- **Status:** approved
- **Source:** Q-002
- **Acceptance:**
  - Given an active alarm, when its indicator is observed, then the front button flashes.
  - Given a menu and no active alarm, when the front button is pressed, then the interface returns to the preceding menu or clock display.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-039 Context-sensitive potentiometers
- **Statement:** The system shall use the two potentiometers as context-sensitive controls for audio volume and light brightness during an active alarm or sleep timer.
- **Priority:** must
- **Status:** approved
- **Source:** Q-009
- **Acceptance:**
  - Given an active sleep timer or alarm with audio and light, when the corresponding potentiometer is adjusted, then the audio volume or light brightness changes without requiring menu navigation.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-047 Wake dark display by tap
- **Statement:** The system shall temporarily wake the display when a tap is detected during dark hours.
- **Priority:** should
- **Status:** approved
- **Source:** Q-012
- **Acceptance:**
  - Given a blank display during dark hours, when the device detects a tap, then it temporarily shows the clock display before returning to the dark-hours state.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-050 Indicate RTC reset required
- **Statement:** When the RTC reports loss of valid time, the system shall indicate that the clock needs to be set.
- **Priority:** must
- **Status:** approved
- **Source:** Q-018
- **Acceptance:**
  - Given an RTC reporting lost or invalid time, when the device starts, then the user is told that the clock needs to be reset.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

## Alarms

### REQ-006 Manage multiple alarms
- **Statement:** The system shall let the user configure and independently enable or disable at least five alarms.
- **Priority:** must
- **Status:** approved
- **Source:** Legacy feature inventory: Alarm; Q-006
- **Acceptance:**
  - Given five configured alarms, when one is disabled, then the other four retain their settings and enabled states.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-007 Schedule recurring alarms
- **Statement:** The system shall support alarms scheduled for selected days of the week.
- **Priority:** must
- **Status:** approved
- **Source:** Legacy feature inventory: Alarm; Q-006
- **Acceptance:**
  - Given an enabled alarm for Monday and Friday, when its scheduled time occurs on those days, then it triggers; when that time occurs on another day, it does not trigger.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-008 Schedule one-shot alarms
- **Statement:** The system shall support a one-shot alarm that disables itself after it triggers.
- **Priority:** must
- **Status:** approved
- **Source:** Legacy feature inventory: Alarm; Q-006
- **Acceptance:**
  - Given an enabled one-shot alarm, when its scheduled time is reached, then it triggers once and remains disabled on subsequent days and after restart.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-009 Configure alarm outputs
- **Statement:** The system shall allow each alarm to select its light effect, sound, output volume and run duration.
- **Priority:** must
- **Status:** approved
- **Source:** Legacy feature inventory: Alarm; Q-004
- **Acceptance:**
  - Given two alarms with different selected outputs, when each is triggered separately, then it uses its own saved effect, sound, volume and duration.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-010 Snooze and dismiss alarms
- **Statement:** The system shall provide distinct actions to snooze an active alarm and dismiss it.
- **Priority:** must
- **Status:** approved
- **Source:** Legacy feature inventory: Ideas; Q-002; Q-003; Q-005
- **Acceptance:**
  - Given an active alarm, when snooze is requested, then its outputs pause and the alarm resumes after the configured snooze interval.
  - Given an active or snoozed alarm, when dismissal is requested, then its outputs stop and that occurrence does not resume.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

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
- **Status:** approved
- **Source:** Q-017; Legacy feature inventory: Audio
- **Acceptance:**
  - Given an enabled alarm with light and unavailable selected audio, when it triggers, then the light runs and the audio fault is indicated.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-030 Ramp light and sound to alarm time
- **Statement:** The system shall begin the configured alarm light and audio ramps before the alarm time so that both reach their configured peak at the alarm time.
- **Priority:** must
- **Status:** approved
- **Source:** Q-001; Q-004; Q-019
- **Acceptance:**
  - Given an alarm with 30-minute light and audio ramps and alarm time 07:00, when the occurrence runs, then both outputs begin ramping at 06:30 and reach their configured peaks at 07:00.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-033 Reduce outputs on tap
- **Statement:** When the device is tapped during an active alarm, the system shall reduce the current alarm light brightness and audio volume by half without stopping the alarm.
- **Priority:** must
- **Status:** approved
- **Source:** Q-003
- **Acceptance:**
  - Given an active alarm with light and audio outputs, when a tap is detected, then each current output level becomes 50 percent of its preceding level and the alarm remains active.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-035 Hold front button to dismiss
- **Statement:** The system shall dismiss an active alarm when the front button is held for two seconds and display a hold reminder while it is pressed.
- **Priority:** must
- **Status:** approved
- **Source:** Q-005
- **Acceptance:**
  - Given an active alarm, when the front button is pressed, then the display shows a hold reminder; when it is held for two seconds, then the alarm is dismissed.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-036 Five-minute snooze
- **Statement:** The system shall resume a snoozed alarm five minutes after snooze is requested.
- **Priority:** must
- **Status:** approved
- **Source:** Q-005
- **Acceptance:**
  - Given an active alarm, when snooze is requested, then its outputs pause and resume five minutes later.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-037 Alarm output priority
- **Statement:** The system shall give a newly occurring alarm priority over an active sleep timer or earlier alarm; simultaneous alarms shall use the lowest-numbered slot.
- **Priority:** must
- **Status:** approved
- **Source:** Q-010
- **Acceptance:**
  - Given a sleep timer or earlier alarm using the outputs, when a new alarm occurs, then the new alarm controls the outputs.
  - Given two alarms occurring at the same time, when they trigger, then the lower-numbered slot controls the outputs.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-046 Scheduled alarm timing tolerance
- **Statement:** The system shall start an enabled scheduled alarm within one second of its intended trigger time when the RTC time is valid and the device is powered.
- **Priority:** must
- **Status:** approved
- **Source:** Q-011
- **Acceptance:**
  - Given a valid RTC and a powered device with a scheduled alarm, when its intended start time is reached, then the alarm starts no more than one second before or after that time.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-049 Do not replay missed alarms
- **Statement:** The system shall not automatically replay alarm occurrences missed while the device was unpowered, unless the alarm would still be playing currently when power is regained. If so jump to the proper point in the volume and brightness ramping.
- **Priority:** must
- **Status:** approved
- **Source:** Q-018; user edit during 2026-11-21 review
- **Acceptance:**
  - Given an alarm occurrence that would have finished while the device was unpowered, when power is restored, then that missed occurrence does not start automatically.
  - Given an alarm occurrence that would still be active when power is restored, when the device restarts during the ramps, then the alarm resumes at the corresponding volume and brightness for the elapsed time.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-051 Front button snooze indication
- **Statement:** When an alarm is active, the system shall snooze it on a short press of the front button and alternate the display between the current time and SNZ while snoozed.
- **Priority:** must
- **Status:** approved
- **Source:** User decision 2026-11-21: front button snooze and clear indication
- **Acceptance:**
  - Given an active alarm, when the front button is briefly pressed and released, then its outputs pause for five minutes and the display alternates between the current time and SNZ while the occurrence remains snoozed.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-052 Two-second dismissal reminder
- **Statement:** While the front button is held during an active alarm, the system shall display a reminder that holding it for two seconds fully dismisses the alarm.
- **Priority:** must
- **Status:** approved
- **Source:** User decision 2026-11-21: explicit two-second hold reminder
- **Acceptance:**
  - Given an active alarm, while the front button is pressed, the display indicates that holding for two seconds fully dismisses it; a two-second hold dismisses rather than snoozes the occurrence.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-053 Do not resume preempted outputs
- **Statement:** When a new alarm takes priority, the system shall not resume the earlier alarm or sleep timer after the new alarm ends.
- **Priority:** must
- **Status:** approved
- **Source:** User clarification 2026-11-21: only the most recent alarm matters
- **Acceptance:**
  - Given an earlier alarm or sleep timer, when a new alarm preempts it and subsequently ends, then the preempted output does not resume.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-054 End snoozed occurrence on power loss
- **Statement:** When power is lost while an alarm is snoozed, the system shall consider that occurrence finished and shall not resume it after restart.
- **Priority:** must
- **Status:** approved
- **Source:** User clarification 2026-11-21: a snoozed alarm is done if power is lost
- **Acceptance:**
  - Given a snoozed alarm, when power is lost and later restored, then that occurrence does not resume.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-055 Do not trigger alarms from clock changes
- **Statement:** When the user sets the clock forward into an alarm occurrence's active window, the system shall not start that occurrence as a result of the clock change.
- **Priority:** must
- **Status:** approved
- **Source:** User clarification 2026-11-21: clock changes do not start an already-active occurrence
- **Acceptance:**
  - Given an enabled alarm, when the clock is set forward into its active window, then that occurrence does not start as a result of the clock change.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

## Light effects

### REQ-011 Provide sunrise effect
- **Statement:** The system shall provide a gradual sunrise effect using the addressable light panel.
- **Priority:** must
- **Status:** approved
- **Source:** User request; Legacy feature inventory: Light; Q-004
- **Acceptance:**
  - Given a configured sunrise, when it runs, then the panel transitions over its configured duration from a dim warm appearance to a brighter appearance.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-012 Provide simple light effects
- **Statement:** The system shall provide both a fixed-color effect and a breathing effect in addition to sunrise.
- **Priority:** should
- **Status:** approved
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
- **Status:** approved
- **Source:** Legacy feature inventory: Hardware inventory and conflicts; Q-013
- **Acceptance:**
  - Given verified panel wiring and channel order, when each color component is selected independently, then the intended physical red, green, blue or white channel illuminates.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-031 Forest light animation
- **Statement:** The system shall offer a dappled forest-light effect with a soft flicker using white and yellow light and optional green.
- **Priority:** must
- **Status:** approved
- **Source:** Q-007
- **Acceptance:**
  - Given the dappled forest effect, when it runs, then the panel produces varying soft light in white and yellow tones with optional green rather than a static single color.
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

## Sleep timer

### REQ-015 Start and stop sleep timer
- **Statement:** The system shall allow the user to start and stop a sleep timer with a selected duration and light or audio output.
- **Priority:** must
- **Status:** approved
- **Source:** Legacy feature inventory: Sleep; Q-008
- **Acceptance:**
  - Given a selected duration and output, when the timer is started, then the selected output runs until expiry or user cancellation and stops afterward.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-016 Show sleep countdown
- **Statement:** The system shall make the remaining sleep-timer time available on the clock display while the timer is active.
- **Priority:** should
- **Status:** approved
- **Source:** Legacy feature inventory: Sleep
- **Acceptance:**
  - Given an active sleep timer, when the user views the timer status, then the display indicates its remaining time and distinguishes it from the current clock time.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-040 Choose sleep outputs independently
- **Statement:** The system shall allow a sleep timer to run light only, audio only, or both together.
- **Priority:** must
- **Status:** approved
- **Source:** Q-008
- **Acceptance:**
  - Given an available light preset and audio file, when the user selects light only, audio only, or both, then only the selected outputs run and they stop at timer expiry.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-048 Do not resume sleep after restart
- **Statement:** The system shall not resume a previously running sleep timer after a power cycle.
- **Priority:** should
- **Status:** approved
- **Source:** Q-016
- **Acceptance:**
  - Given an active sleep timer, when the device is power-cycled, then the previous sleep timer and its outputs do not resume automatically.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

## Audio and storage

### REQ-017 Play user audio
- **Statement:** The system shall allow the user to select and play compatible WAV audio from the SD card for alarms and the sleep timer.
- **Priority:** must
- **Status:** approved
- **Source:** Legacy feature inventory: Audio; Q-015
- **Acceptance:**
  - Given two compatible WAV files on the SD card, when each is selected in turn for an alarm or sleep timer, then the chosen file is played through the speaker.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-018 Preserve saved settings
- **Statement:** The system shall restore saved alarm, light preset and system settings after a power cycle.
- **Priority:** must
- **Status:** approved
- **Source:** Legacy feature inventory: Persistence; Q-016
- **Acceptance:**
  - Given saved settings including alarm output selections and a light preset, when power is cycled with storage available, then those settings are restored unchanged.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-019 Report unavailable resources
- **Statement:** The system shall indicate when a required storage device, selected audio file, or valid clock time is unavailable.
- **Priority:** must
- **Status:** approved
- **Source:** Legacy feature inventory: Audio and persistence; Q-017; Q-018
- **Acceptance:**
  - Given missing SD storage, a missing selected file, or invalid RTC time, when the condition is detected, then the user receives an indication identifying the problem.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

## Serial diagnostics and control

### REQ-024 USB serial commands
- **Statement:** The system shall accept human-readable commands over USB serial to set the RTC time and start a selected alarm slot. These commands must encompass the ability to set/change any and all settings as well as stear the device by sending moce encoder turn and click commands. The API must also provide a "read screen" command that returns the text currently displayed on the seven segment display as well as a "read led" command that will return the values of all the LEDs in the matrix.
- **Priority:** must
- **Status:** approved
- **Source:** User request 2026-11-21: serial logging, monitoring and control
- **Acceptance:**
  - Given a connected serial client and a valid time command, when the client sends the command, then the RTC is updated and the client receives confirmation of the resulting time.
  - Given a configured alarm slot, when the client sends a valid start command naming that slot, then that slot starts and the client receives confirmation identifying the slot.
- **Added:** 2026-11-21 by user

### REQ-025 Shared control actions
- **Statement:** The system shall route equivalent local-menu and serial actions through the same application-level commands.
- **Priority:** must
- **Status:** approved
- **Source:** User request 2026-11-21: decouple menus and serial control
- **Acceptance:**
  - Given the same valid RTC time, when it is set once through the menu and once through serial, then both operations update the RTC with the same resulting value and apply the same validation.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-026 Serial command results
- **Statement:** The system shall return an explicit success or error response for each serial command or significant action in the device. For example, this could be the initiation of an alarm, pressing of buttons (and the action taken in response to that button etc). This should allow the USB serial port to be used for debugging of the physical hardware features.
- **Priority:** must
- **Status:** approved
- **Source:** User request 2026-11-21: command responses for hardware testing
- **Acceptance:**
  - Given a valid command, when it completes, then the client receives a success response identifying the action and result.
  - Given an unknown command, malformed time or invalid alarm slot, when it is submitted, then the client receives an error and the corresponding clock or alarm state remains unchanged.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-027 Timestamped serial event log
- **Statement:** The system shall emit timestamped serial log messages for RTC time changes and alarm start events. This should allow the USB serial port to be used for debugging of the physical hardware features.
- **Priority:** must
- **Status:** approved
- **Source:** User request 2026-11-21: timestamped logging and monitor
- **Acceptance:**
  - Given a valid RTC, when the time is changed or an alarm starts, then a serial monitor receives an event identifying the action and its date and time.
  - Given invalid RTC time, when an event is logged, then the log indicates that the timestamp is invalid instead of presenting it as valid.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-028 Query device state over serial
- **Statement:** The system shall provide serial commands to read the current RTC time and the state of a selected alarm slot. This should allow the USB serial port to be used for debugging of the physical hardware features.
- **Priority:** must
- **Status:** approved
- **Source:** User request 2026-11-21: serial monitoring and agent hardware testing
- **Acceptance:**
  - Given a connected serial client, when it requests current time or a valid alarm slot state, then it receives the corresponding current value or state.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-029 Serial control hardware test
- **Statement:** The system shall support exercising and observing RTC setting and alarm activation from a USB serial client on the physical device.
- **Priority:** must
- **Status:** approved
- **Source:** User request 2026-11-21: agents test the hardware over USB serial
- **Acceptance:**
  - Given target hardware connected by USB, when a test client sets the RTC, queries it, starts a configured alarm and queries its state, then the serial responses and observed hardware behavior agree.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-041 Configure all settings over serial
- **Statement:** The system shall allow a serial client to read and change every user-adjustable device setting.
- **Priority:** must
- **Status:** approved
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
- **Status:** approved
- **Source:** User edit to REQ-024 during 2026-11-21 review
- **Acceptance:**
  - Given a known menu or time shown on the display, when the client requests the screen, then the response represents the currently visible text.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-044 Read panel LEDs over serial
- **Statement:** The system shall provide a serial command returning the current output values of every light-panel LED.
- **Priority:** must
- **Status:** approved
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
- **Status:** approved
- **Source:** User decisions 2026-11-21: serial diagnostics only when computer-connected at startup
- **Acceptance:**
  - Given power from a wall socket without an active serial connection, when the device starts, then serial command handling and diagnostic logging are disabled.
  - Given a serial connection active at power-up, when the device starts, then serial diagnostics are available.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

### REQ-057 Permit diagnostic message loss under load
- **Statement:** The system shall prioritize normal alarm operation over serial diagnostic message delivery when performance requires messages to be dropped.
- **Priority:** must
- **Status:** approved
- **Source:** User decision 2026-11-21: messages may be dropped for performance
- **Acceptance:**
  - Given a slow serial client and constrained diagnostic output, when messages cannot be transmitted without affecting alarm operation, then the system can drop messages and continues to meet the alarm timing requirement.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)

## Output limits

### REQ-038 Cap light and audio output
- **Statement:** The system shall limit light brightness and audio output to 80 percent of their respective verified actual limits.
- **Priority:** must
- **Status:** approved
- **Source:** Q-014
- **Acceptance:**
  - Given verified light and audio maximums, when any interface requests output above 80 percent of either maximum, then the corresponding output remains at or below 80 percent.
- **Added:** 2026-11-21 by user
- **Approved:** 2026-11-21 by user (review window)
