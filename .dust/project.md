---
name: Bright-Noise-Alarm
summary: "Custom RP2040 light and audio alarm clock; preparing a clean rewrite based on legacy hardware and feature evidence."
stack: "Arduino C++ with Earle Philhower RP2040 core and Arduino CLI for hardware bring-up; rewrite architecture pending requirements."
created: 2026-11-21
dust_schema: 1
---
# Bright-Noise-Alarm

## Purpose and users
Build a maintainable custom alarm clock with a simulated sunrise, animations,
audio, a clock display, and physical controls.

## Goals / non-goals
Extract the legacy feature set and hardware configuration before designing the
rewrite. Two future special-function buttons are out of scope for now.

## Constraints
The legacy checkout at `../(OLD)Bright-Noise-Alarm-Working` is read-only and
contains uncommitted work not present in the remote repository.

## Conventions
- Git: git-flow (main / dev / feature/*), conventional commits referencing artifact ids.

## Workflow
Full project: fact-finding, living requirements, design/plan, and formal Analyst
requirements review are enabled.

## Key decisions
<!-- Links to decision artifacts. -->
