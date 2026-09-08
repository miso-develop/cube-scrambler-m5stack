---
name: wayfinder
description: Plan a large, ambiguous, multi-session effort by mapping unresolved decisions before implementation. Use when the destination is known but the route is still foggy or depends on multiple decisions/research steps.
metadata:
  version: "1.0"
---

# Wayfinder

Wayfinder is planning only. It resolves decisions and prerequisites; it does not implement production code.

## 1. Define the destination

State what must be decided or specified when the map is complete. If the destination itself is unclear, use `grilling` first.

## 2. Create the map

Create one GitHub Issue titled `[Map] <name>` using `agent/WORK-TRACKING.md`. Record Destination, Decisions so far, Not yet specified, and Out of scope.

## 3. Create decision issues

For each precise unresolved question, create `[Decision] <question>` with Parent map, one Question, and Blocked by. Decisions may use `research`, `grilling`, `codebase-design`, or a throwaway prototype for evidence, but must not deliver production behavior.

## 4. Work the frontier

The frontier is every open Decision whose blockers are closed. Resolve a decision by recording answer/evidence, closing it, linking the outcome from the parent Map, and promoting newly-clear fog into further Decisions when needed.

## 5. Finish the map

The Map is complete when no in-scope Decision remains open, remaining fog is empty or explicitly out of scope, and implementation-relevant decisions are stable enough for `to-spec`.

If the session stops early, use `handoff` on the active Decision or Map.

Adapted from `mattpocock/skills` `wayfinder` for the Loop Engineering GitHub Issue model.
