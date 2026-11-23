---
name: Bright-Noise-Alarm
summary: "Uploaded automatic menu saving: clock, alarm and sleep adjustments persist immediately, with no Save items or success-screen interruptions. Back/idle keep successful changes; invalid or failed saves show errors. Native encoder alarm/sleep edits survived reboot, clock seconds updated immediately, and original presets/draft/display/dark settings were restored. Seven strict host executables, menu ASan/UBSan and firmware build pass (156,464 program / 23,604 static RAM bytes). Non-stacking temporary tap dimming remains at 120 milli-g, one half-level modifier until interaction-wake expiry. Full-word scrolling menus and inclusive sleep Hold remain intact. Playback stopped. Standalone presets, no-SD startup, pots and front-button wiring remain pending."
stack: "Arduino C++ with Earle Philhower RP2040 core; approved architecture plan and RP2040 build environment."
created: 2026-11-21
dust_schema: 1
---
# Bright-Noise-Alarm

## Purpose and users
Build a maintainable custom alarm clock with a simulated sunrise, animations,
audio, a clock display, and physical controls.

## Goals / non-goals
Build a maintainable rewrite against the reviewed living requirements and the
user-approved architecture direction.
Two future special-function buttons are out of scope for now.

## Constraints
The legacy checkout at `../(OLD)Bright-Noise-Alarm-Working` is read-only and
contains uncommitted work not present in the remote repository.
Keep the design simple: group related functionality in files and objects,
reuse shared behavior, and make the firmware easy to maintain and tweak by hand.
Include the approved USB serial control and hardware-test observability
requirements in the architecture and test plan.

## Conventions
- Git: git-flow (main / dev / feature/*), conventional commits referencing artifact ids.

## Workflow
Iterative, intent-directed delivery is the project default, approved by the
user on 2026-11-21. Fact-finding, requirements drafting, formal plans and formal
Analyst review are optional, used when useful or explicitly requested. Preserve
existing approved requirements, decisions, plans, reviews and evidence; do not
silently change binding requirements. Merge, release and host permissions remain
separate.

## Key decisions
<!-- Links to decision artifacts. -->
- 2026-11-22: Defer REQ-039 until the user solders the two potentiometers and
  confirms their ADC GPIO mapping. One controls active light brightness and
  one active audio volume. No takeover until movement catches the current
  output; thereafter use 0-200% scaling of the ongoing alarm/sleep output,
  preserving fades and existing safety caps. Keep the menu encoder separate.
