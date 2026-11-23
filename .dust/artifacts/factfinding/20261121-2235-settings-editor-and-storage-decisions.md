---
id: 20261121-2235-settings-editor-and-storage-decisions
type: factfinding
title: Settings editor and storage decisions
author: Copilot
status: active
created: "2026-11-21 22:35"
branch: feature/encoder-main-menu
inputs: [".dust/artifacts/tasks/20261121-2215-configuration-editors-and-sd-settings-persistence.md", ".dust/artifacts/reviews/20261121-2217-configuration-editors-and-sd-persistence-plan-revi.md", ".dust/requirements.md"]
confidence: medium
summary: "Owner questions for missing editor ranges, defaults, one-shot setup and blank-store policy. Suggestions are not approved; blocking items must be answered before implementation."
open_questions:
  - "Q-020 through Q-044: settings and storage choices needed to complete the field matrix."
next: "Owner answers the questions; update the settings matrix and submit it for review and approval."
signature:
  agent: user
  role: user
  signed: "2026-11-21 22:35"
  dust: 0.1.0
---
## How to answer
Answer in the review window or type under each **Answer:** line here. Short answers, "you decide", and "defer" are fine. Suggestions are proposals, not decisions: unanswered blocking questions do not authorize implementation.
Format (the review window and `dust questions` rely on it): each question is a `### Q-nnn [blocking|important|nice] <question>` heading with an `**Answer:**` line; the answer is the text after it, up to the next heading, so answers can use lists, tables and bold but not headings.
Tags: **[blocking]** I can't proceed without it, **[important]** it changes the design, **[nice]** it refines the details.

## Context
The settings task needs specific domains and first-boot choices before coding.
Already decided: new configuration fields save immediately; clock editing uses
Save/Back; alarm time is the ramp's peak; and physical outputs are capped at
80% of verified actual limits. Nothing below changes those decisions.

## Purpose and success

### Q-020 [blocking] On a brand-new SD card with no settings, should the clock create a complete set of defaults automatically, or guide you through setup first?
*Why it matters:* an empty card has no existing values to restore.
*If unanswered I'll assume:* propose automatic defaults with alarms disabled; wait for approval before implementation.
**Answer:** yes, choose something reasonable for each and we can update as we go

## Users and use cases

### Q-021 [blocking] Should there be exactly five alarm slots, or more? If more, how many?
*Why it matters:* the settings model must allocate a known number of independent alarms.
*If unanswered I'll assume:* propose five, as previously discussed.
**Answer:**

### Q-022 [blocking] How many saved light presets should be available?
*Why it matters:* alarm and sleep editors need a definite set of slots.
*If unanswered I'll assume:* propose four presets.
**Answer:**

## Scope and behaviour

### Q-023 [blocking] What should a new alarm slot start with: enabled or disabled, and what default alarm time?
*Why it matters:* first-boot slots must be usable without accidentally sounding.
*If unanswered I'll assume:* propose disabled at 07:00.
**Answer:**

### Q-024 [blocking] What should the default clock display format be: 12-hour or 24-hour?
*Why it matters:* both formats are required but the initial choice is unspecified.
*If unanswered I'll assume:* propose 12-hour.
**Answer:**

### Q-025 [blocking] Should alarm and dark-hour times be set to the nearest minute, or do you need seconds?
*Why it matters:* this determines the editor precision and saved representation.
*If unanswered I'll assume:* propose minute precision; the clock time editor still supports seconds.
**Answer:**

### Q-026 [blocking] What should dark hours default to: disabled, or enabled with specific start and end times?
*Why it matters:* the display's first-boot behavior should be intentional.
*If unanswered I'll assume:* propose disabled, with configurable start and end times.
**Answer:**

### Q-027 [blocking] If dark hours have identical start and end times, should that mean disabled, dark all day, or an invalid setting?
*Why it matters:* without a rule the same saved interval can be interpreted three ways.
*If unanswered I'll assume:* propose treating it as disabled.
**Answer:**

### Q-028 [blocking] For a recurring alarm, what weekdays should be selected initially, and may the selection be empty?
*Why it matters:* an empty weekday selection could create an enabled alarm that never rings.
*If unanswered I'll assume:* propose Monday through Friday initially and reject enabling a recurring alarm with no days selected.
**Answer:**

### Q-029 [blocking] For a one-shot alarm, should you select a calendar date, or should it fire at the next occurrence of the chosen time?
*Why it matters:* this determines which settings the editor must save.
*If unanswered I'll assume:* propose the next occurrence of the chosen time.
**Answer:**

### Q-030 [blocking] Should a new alarm default to recurring weekdays or one-shot?
*Why it matters:* each alarm needs a clear initial schedule type.
*If unanswered I'll assume:* propose recurring weekdays.
**Answer:**

### Q-031 [blocking] Should an alarm select a saved light preset or select an effect directly?
*Why it matters:* the alarm needs an unambiguous light configuration.
*If unanswered I'll assume:* propose selecting a saved preset.
**Answer:**

### Q-032 [blocking] What initial effect and color should a new light preset use?
*Why it matters:* the first-boot preset needs usable contents.
*If unanswered I'll assume:* propose a fixed warm-white preset; the complete effect set remains sunrise, fixed, breathing, forest and fade-to-color.
**Answer:**

### Q-033 [important] For the forest effect, do you want an explicit option for adding green, or should it always be chosen by the animation?
*Why it matters:* this determines whether the preset editor needs an extra field.
*If unanswered I'll assume:* propose an optional green on/off setting, initially off.
**Answer:**

## Timing and sleep settings

### Q-034 [blocking] What range, step and default should the alarm's run duration use?
*Why it matters:* the editor needs finite supported values.
*If unanswered I'll assume:* propose 1–120 minutes in one-minute steps, default 30 minutes.
**Answer:**

### Q-035 [blocking] Should the run duration in Q-034 count from the start of the ramp or from the alarm's peak time?
*Why it matters:* this specifies when the alarm stops; the alarm time itself is already agreed to be the peak.
*If unanswered I'll assume:* propose counting from the peak.
**Answer:**

### Q-036 [blocking] Should light and audio ramp durations be individually adjustable, and what ranges and defaults should they have?
*Why it matters:* both outputs begin before the already-agreed peak time.
*If unanswered I'll assume:* propose independent 0–120 minute durations in one-minute steps, each defaulting to 30 minutes.
**Answer:**

### Q-037 [blocking] What range, step and default should the sleep timer duration use?
*Why it matters:* the sleep editor needs finite supported values.
*If unanswered I'll assume:* propose 1–180 minutes in one-minute steps, default 30 minutes.
**Answer:**

### Q-038 [blocking] What should a new sleep timer select by default: light only, audio only, or both?
*Why it matters:* all three options are supported, but an initial selection is needed.
*If unanswered I'll assume:* propose light only.
**Answer:**

### Q-039 [blocking] Should saved sleep configuration be remembered after a restart, even though a running sleep timer never resumes?
*Why it matters:* stored configuration and the active timer are distinct.
*If unanswered I'll assume:* propose remembering the selected configuration, not the running timer.
**Answer:**

## Quality attributes (performance, reliability, security, privacy)

### Q-040 [blocking] What range, step and initial value should the clock display brightness control use?
*Why it matters:* the display has a separate brightness setting from the light panel.
*If unanswered I'll assume:* propose a logical 0–100% control with a conservative initial value you approve, mapped to supported hardware levels.
**Answer:**

### Q-041 [blocking] What range, step and initial value should the light-panel brightness control use?
*Why it matters:* the preset editor needs a defined brightness domain.
*If unanswered I'll assume:* propose a logical 0–100% control with a conservative initial value you approve; 100% is still limited to 80% of the verified physical maximum.
**Answer:**

### Q-042 [blocking] What range, step and initial value should alarm audio volume use?
*Why it matters:* each alarm's saved volume needs a defined domain.
*If unanswered I'll assume:* propose a logical 0–100% control with a conservative initial value you approve; 100% is still limited to 80% of the verified physical maximum.
**Answer:**

### Q-043 [blocking] What range and initial color should each RGBW component use?
*Why it matters:* saved presets need independent, well-defined color values.
*If unanswered I'll assume:* propose logical values from 0 to 255 for each component and an initial color you approve; these numbers do not establish safe hardware maxima.
**Answer:**

## Constraints (technology, budget, time, legal, environment)

## Data and integrations

### Q-044 [blocking] Which light preset and WAV file should alarms and sleep select initially, including when none is available?
*Why it matters:* saved selections need explicit initial values and must not silently point to a different item.
*If unanswered I'll assume:* propose the first available preset and no audio until selected; show a clear missing-file indication rather than substituting audio.
**Answer:**

## Failure, edge cases and operations

## Assumptions I'm making

- Suggested values above are proposals only; unanswered blocking questions do
  not permit implementation.
- No settings choice changes the existing clock Save/Back behavior or the
  verified-limit safety gate for physical light and audio.
- WAV encoding compatibility and actual hardware limits require technical
  verification rather than guesses.

## Round log

- Round 1 prepared 2026-11-21; awaiting owner answers.
