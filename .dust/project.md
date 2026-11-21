---
name: Bright-Noise-Alarm
summary: "Custom RP2040 alarm clock rewrite; peripheral diagnostic is uploaded, with physical output and control checks pending."
stack: "Arduino C++ with Earle Philhower RP2040 core and Arduino CLI installed for hardware bring-up; architecture pending."
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
Full project: fact-finding, living requirements, design/plan, and formal Analyst
requirements review are enabled.

## Key decisions
<!-- Links to decision artifacts. -->
