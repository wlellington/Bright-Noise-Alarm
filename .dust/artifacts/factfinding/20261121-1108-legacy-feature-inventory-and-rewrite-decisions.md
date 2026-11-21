---
id: 20261121-1108-legacy-feature-inventory-and-rewrite-decisions
type: factfinding
title: Legacy feature inventory and rewrite decisions
author: Technical Analyst
status: done
created: "2026-11-21 11:08"
branch: dev
inputs:
  - .dust/project.md
  - "../(OLD)Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Light_Clock_Firmware.ino"
  - "../(OLD)Bright-Noise-Alarm-Working/FEATURES.md"
confidence: medium
summary: "Legacy inventory and owner-reviewed answers, including serial control and hardware-test scope. Q-019 resolves alarm timing: audio and light ramp before the alarm time. REQ-022 is retired in favor of REQ-030."
open_questions: []
next: "Proceed to independent requirements review before architecture; verify hardware channel order on device."
signature:
  agent: user
  role: user
  signed: "2026-11-21 11:08"
  dust: 0.1.0
---
## How to answer
Run `dust review` to answer in a small window (it edits this file), or type under each **Answer:** line right here. Short is fine, and "don't know" or "you decide" are valid answers. Write `defer` to postpone a question. A blank answer means I'll go with the stated assumption and flag it as unconfirmed.
Format (the review window and `dust questions` rely on it): each question is a `### Q-nnn [blocking|important|nice] <question>` heading with an `**Answer:**` line; the answer is the text after it, up to the next heading, so answers can use lists, tables and bold but not headings.
Tags: **[blocking]** I can't proceed without it, **[important]** it changes the design, **[nice]** it refines the details.

## Context
The owner wants a ground-up, maintainable RP2040 light and audio alarm clock,
preserving kinds of menus and animations but not exact menus, ordering, or
implementation details. This inventory reads the old checkout only; the new
checkout's tracked firmware is an older snapshot, not the rewrite. Neither
source inspection nor a successful hardware-test compile proves on-device
behavior. All proposed scope is subject to owner approval.

## Simplified feature set and evidence
Paths below are relative to the new repository. "Present" means code exists,
not that the behavior has been verified on hardware.

| Area | Simplified feature and evidence | Status / caveat |
|---|---|---|
| Clock | RTC-backed time and date, configurable 12/24-hour display; time-setting menu. [Menu definitions](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.hpp#L42), [time edit](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.cpp#L419), [RTC update](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Light_Clock_Firmware.ino#L242). | Present; hardware unverified. |
| Menus | Encoder navigates clock, alarms, sleep timer, animations, time and system settings, with back and timeout. [Menu states](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.hpp#L8), [navigation](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.cpp#L364). | Present; exact labels/order need not survive. |
| Alarm | Five slots, enable, time, one-shot or selected days, animation, song, volume and duration. [Alarm fields](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.hpp#L221), [scheduler and trigger](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Light_Clock_Firmware.ino#L1202). | Partial: saved alarms omit song, volume, animation and duration; selected song is not assigned in trigger. Timing relative to sunrise is undecided. |
| Light | Addressable RGBW 8x8 panel; fixed, breathe and sunrise effects, adjustable colors, brightness and fades; preview and saved slots. [Algorithms](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Light_Clock_Firmware.ino#L696), [editor](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.cpp#L639). | Present, partly integrated; use a small set rather than duplicate all parameters. |
| Sleep | Select an animation, music and duration, start or stop; fade and remaining-time display. [Start/stop](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.cpp#L534), [countdown](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Display_Driver_New.cpp#L801). | Partial: currently requires a discovered song to proceed and timer completion does not visibly clear its active flag in the animation stop path. |
| Audio | SD WAV discovery, looping playback, volume and optional menu clicks. [Scanner](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Light_Clock_Firmware.ino#L885), [playback](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Light_Clock_Firmware.ino#L436). | Partial: alarm playback uses shared sleep state; legacy README boot-sound claim conflicts with disabled main-audio code. WAV formats and missing-file behavior require decisions. |
| Dark hours | Scheduled display blanking with temporary wake by encoder or detected tap. [Loop](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Light_Clock_Firmware.ino#L291), [tap detection](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Light_Clock_Firmware.ino#L920). | Present, with incomplete settings (end time fixed in code); motion snooze is documented intent, not demonstrated. |
| Persistence | SD settings for display, alarm schedule and some animation data. [Save/load](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Light_Clock_Firmware.ino#L978). | Partial: not every edited alarm/animation field is stored; replacement deletes the old file before rename. Do not assume power-fail safety. |
| Controls | Encoder rotation/press are read; accelerometer detects tap for dark display; front button configured in hardware test. [Input processing](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Input_Processing.cpp#L62), [hardware test](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Hardware_Test/Light_Clock_Hardware_Test.ino#L229). | Front-button purpose and potentiometer assignments need decisions; backup reads two pots at pins 8/9, but actual wiring is unverified. [Backup definitions](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware_Backup/Defines.hpp#L7). |
| Ideas, not proven | Motion snooze, battery-aware behavior, quick potentiometer settings, rainbow/pulse patterns and gradual alarm audio appear in [feature notes](../../../../%28OLD%29Bright-Noise-Alarm-Working/FEATURES.md). | Documentation intent or incomplete integration, not evidence that they work. |

### Hardware inventory and conflicts
The owner describes a four-digit display with individual dots and colon, an
illuminated front button, an I2C rotary encoder, two fixed-range potentiometers,
and a mono speaker. The [hardware test](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Hardware_Test/Light_Clock_Hardware_Test.ino#L12)
uses a PCF8523 RTC, display address 0x70, seesaw encoder 0x36, LIS3DH at 0x18,
SD CS 10, and I2S output; the active code configures a 64-pixel RGBW panel.
These are source-based configuration clues, not a verified wiring diagram.
The [active firmware](../../../../%28OLD%29Bright-Noise-Alarm-Working/Firmware/Light_Clock_Firmware/Light_Clock_Firmware.ino#L24)
uses `NEO_GRBW` while the hardware test uses `NEO_RGBW`; verify individual
channels on the device. Potentiometer pins and any electrical limits remain
unconfirmed. The two future special-function buttons are outside this scope.

## Core use cases
- **UC-01, set the clock:** From the time display, the owner navigates to time
  settings, edits the time/date, and saves or cancels the edit.
- **UC-02, wake to an alarm:** The owner configures an alarm and its light and
  sound; at the scheduled occurrence the device activates the chosen outputs.
  The owner can snooze or dismiss it. The sunrise/audio timing is Q-004.
- **UC-03, fall asleep:** The owner selects a sleep duration and output, starts
  the timer, and may check remaining time or stop it early.
- **UC-04, configure lighting:** The owner selects an effect, adjusts its color
  and brightness with preview, and saves a preset for later use.
- **UC-05, recover from a fault:** If time, storage, or selected audio is
  unavailable, the owner can recognize the problem; Q-017 and Q-018 determine
  safe alarm and restart behavior.
- **UC-06, exercise hardware over USB:** A tester or agent sends commands over
  serial to set and read the RTC and start and inspect an alarm, observing
  timestamped responses and physical output without navigating menus.

## Purpose and success
### Q-001 [important] What would make the first usable rewrite a success on a normal morning?
*Why it matters:* a concrete outcome helps prioritize the first release.
*If unanswered I'll assume:* reliable scheduled light and sound, with straightforward local configuration.
**Answer:** I would like to be able to 1) set the time and make sure the RTC module is doing the actual time  keeping, 2) set an alarm, 3) have the alarm go off at the right time, playing its audio with a slowly ramping up volume and light animation.

## Users and use cases

### Q-002 [important] What should the illuminated front button do during an alarm, and when no alarm is active?
*Why it matters:* it is a distinct physical control whose role is not established by the active code.
*If unanswered I'll assume:* short press snoozes an active alarm and otherwise wakes the display.
**Answer:** While alarm is going off it should be flashing. While not active it should act as a back button in the menus.

### Q-003 [important] What should happen when the clock is tapped during an alarm?
*Why it matters:* legacy tap detection wakes the display, while motion snooze is only an intended feature.
*If unanswered I'll assume:* tapping snoozes an active alarm and wakes a dark display otherwise.
**Answer:** It should make turn down the volume and brightness to 50% of the current level, but not totally turn it off.

## Scope and behaviour

### Q-004 [blocking] Does the configured alarm time mean the start of sunrise, or the time at which sound/full brightness begins?
*Why it matters:* scheduling the light and sound differently changes the essential alarm behavior.
*If unanswered I'll assume:* sunrise begins before the alarm time and reaches its target when audio begins.
**Answer:** The "alarm time" should mean when things get to their peak sound/brightness. so animations and sounds should start playing sooner than that depending on the length settings.

### Q-005 [important] How long should snooze last, and how should an alarm be dismissed instead?
*Why it matters:* snooze and dismissal must be distinct, predictable actions.
*If unanswered I'll assume:* a 10-minute snooze and a deliberate long press to dismiss.
**Answer:** 5 minute snooze. to dismiss the front buttom must be held for 2 seconds. Make the screen say somethign like "HOLD" when teh button is pressed as a reminder.

### Q-006 [important] Are five independently configurable alarms sufficient, including one-shot and selected-day recurrence?
*Why it matters:* this sets the minimum alarm-management capability.
*If unanswered I'll assume:* yes, five alarms with both recurrence types.
**Answer:** that should be good. i really only expect to use two of them for now but want room to grow.

### Q-007 [important] Which light effects are essential beyond sunrise?
*Why it matters:* preserve distinct animation types without cloning every old option.
*If unanswered I'll assume:* solid color and breathing, with adjustable brightness and color.
**Answer:** I want the ability to simulate dappled light through leaves like you would see on a forest flor under aspen trees. so a mixture of white/yellow light  that has a soft flicker and makybe some green mixed in. Another mode will just be a fade in to a specified color.

### Q-008 [important] How should the sleep timer combine light and sound?
*Why it matters:* the legacy menu requires audio even for a light-only timer.
*If unanswered I'll assume:* light, sound, or both can be selected independently and stop at timer expiry.
**Answer:** light, sound, or both can be selected independently and stop at timer expiry.

### Q-009 [important] What should the two potentiometers control?
*Why it matters:* their behavior is not established in the active firmware.
*If unanswered I'll assume:* one controls light brightness and the other controls speaker volume.
**Answer:** Depends on the context. During sleep timer, they should control volume and brightness of the audio and sound. During setting the alarm they can be used for setting colors (huge and brightness for example). When an alarm is going of they can do the same. Treat them as additional controls for each menu whenever more than the potentiometer will be helpful.

### Q-010 [important] What should happen when an alarm arrives while a sleep timer or another alarm is running?
*Why it matters:* both share light and audio outputs.
*If unanswered I'll assume:* the new alarm takes priority and cancels the sleep timer; simultaneous alarm handling needs a clear rule.
**Answer:** The new alarm takes priority. If they are identical/happen at the same time, take the lower numbered alarm (according to the alarm slot names)

## Quality attributes (performance, reliability, security, privacy)

### Q-011 [important] What alarm timing tolerance is acceptable?
*Why it matters:* a numeric threshold makes reliability testable.
*If unanswered I'll assume:* a scheduled alarm starts within one second of its intended trigger when time is valid and the device is powered.
**Answer:** within 1 second is fine

### Q-012 [important] What should the display do during dark hours?
*Why it matters:* nighttime visibility and sleeping comfort conflict.
*If unanswered I'll assume:* blank during configurable dark hours and temporarily wake on a control or tap.
**Answer:** blank during configurable dark hours and temporarily wake on a control or tap.

## Constraints (technology, budget, time, legal, environment)

### Q-013 [blocking] Are the existing RP2040 board and connected peripherals the required target, and is their actual wiring documented elsewhere?
*Why it matters:* hardware compatibility and pin/channel assignments cannot be established from conflicting legacy source alone.
*If unanswered I'll assume:* reuse the current hardware and verify wiring and RGBW channel order on-device before implementation is accepted.
**Answer:** the version of the code that I gave in the local folder is the correct wiring for the device. All the correct pins and addressses are currently used by the version found in /home/contact/dev/bright_noise_v2/(OLD)Bright-Noise-Alarm-Working

### Q-014 [important] Are there brightness, sound, or power limits that must not be exceeded?
*Why it matters:* output limits affect comfort and safe operation.
*If unanswered I'll assume:* respect the physical hardware's verified limits; exact limits remain to be established.
**Answer:** Lets cap everything at 80% of the actual limit for now, then change that later if we need to.

## Data and integrations

### Q-015 [important] Which audio files and format should users be able to supply?
*Why it matters:* the SD scan currently looks for WAV files, but supported encoding and library compatibility are not proven.
*If unanswered I'll assume:* user-provided WAV files on SD; encoding is confirmed during hardware bring-up.
**Answer:** Assume you have WAV files for now. we can add other codecs later if they are lower power usage/more stable/higher performance.

### Q-016 [important] Which settings should survive a restart?
*Why it matters:* legacy storage omits several per-alarm fields.
*If unanswered I'll assume:* all saved alarms, light presets and system settings survive; a running sleep timer does not resume.
**Answer:** all saved alarms, light presets and system settings survive; a running sleep timer does not resume.

## Failure, edge cases and operations

### Q-017 [blocking] What should happen if the SD card or selected alarm sound is unavailable at alarm time?
*Why it matters:* a silent or invisible alarm can fail the primary use case.
*If unanswered I'll assume:* show a clear fault and still run the configured light; a fallback sound requires a separately confirmed source.
**Answer:** show a clear fault and still run the configured light; a fallback sound requires a separately confirmed source.

### Q-018 [important] What should happen after power loss, including when RTC time is invalid or an alarm was missed?
*Why it matters:* recovery, time validity and missed-alarm behavior determine trustworthiness.
*If unanswered I'll assume:* retain stored settings and RTC time where available, clearly indicate invalid time, and do not replay missed alarms.
**Answer:** retain stored settings and RTC time where available, clearly indicate invalid time, and do not replay missed alarms. if the RTC seems to have failed (back to it default time state, indicate that the time needs to be reset somehow)

## Review follow-up

### Q-019 [blocking] Should REQ-022 be retired in favor of REQ-030 so audio ramps up before the alarm time?
*Why it matters:* both requirements were approved, but REQ-022 starts audio at the alarm time while REQ-030 ramps it up beforehand.
*If unanswered I'll assume:* no decision; the conflicting requirements must be reconciled before implementation.
**Answer:** Start audio before and have it ramp. The alarm time is when everything should peak.

## Assumptions I'm making
- Single local user; no network or companion app is required.
- Preserve menu categories and distinct effects, not exact legacy menu order or formulas.
- User-supplied audio, not any specific copyrighted sample, is the intended content.
- Two future special-function buttons remain out of scope.
- Untested legacy behaviors are evidence and candidates, not guarantees or approved requirements.

## Round log
- Round 1 prepared 2026-11-21; owner reviewed and answered 13 of 18 questions.
- 2026-11-21: Owner requested USB serial commands, logging and monitoring for
  physical-device and agent testing; see proposed REQ-024 through REQ-029.
- 2026-11-21: Answers informed proposed REQ-030 through REQ-040. Q-004 says
  audio ramps before the alarm time, but approved REQ-022 starts audio at the
  alarm time. REQ-030 records the requested ramp and is marked needs-info
  pending explicit reconciliation with REQ-022; neither is implementation-ready
  while they conflict. Q-017 remains unanswered despite approval of REQ-023:
  the accepted light-and-fault behavior is known, but any fallback sound is not.
- Q-013 identifies the legacy folder as the wiring source; its RGBW order
  differs between active firmware and hardware test, so confirm on hardware.
- 2026-11-21: Owner answered all initial questions and approved REQ-030, but
  REQ-022 remains approved. Q-019 requests an explicit decision rather than
  silently retiring an approved requirement.
- 2026-11-21: Owner expanded REQ-024 to cover all settings, simulated encoder
  input, and screen/LED readback, and expanded REQ-026 for physical debugging.
  REQ-041 through REQ-045 propose focused acceptance criteria for this scope.
  REQ-046 through REQ-050 carry forward the remaining answered questions.
- 2026-11-21: Owner answered Q-019 and approved REQ-041 through REQ-050.
  REQ-022 was dropped in favor of REQ-030. The owner also amended REQ-049:
  if power returns while an alarm would still be active, resume at the
  appropriate point in its ramps; acceptance criteria were reconciled.
