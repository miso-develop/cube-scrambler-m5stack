---
name: code-review
description: Review a branch, PR, commit range, or work-in-progress change against requested behavior and repository engineering constraints.
metadata:
  version: "1.1"
  source: "adapted from mattpocock/skills"
  license: "MIT"
---

# Code Review

Establish the exact diff first. Read the explicit user instruction, selected Task and Parent Spec, `PROJECT.md`, applicable `AGENTS.md` / `agent/` rules, relevant tests, and architecture documentation.

## Behavior / spec compliance

Look for missing Acceptance Criteria, incorrect behavior, unrequested behavior, regressions outside the selected Task, and tests/checks weakened to obtain green.

## Engineering quality

Look for material duplicated logic, speculative abstraction, unclear ownership, brittle coupling, unsafe error/resource handling, misleading interfaces/naming, temporary debug code, or generated/secret material accidentally retained.

## Validate findings

Every finding should identify the concrete location and failure/maintenance risk, cite the conflicting requirement/evidence where applicable, and distinguish confirmed defects from judgement calls.

Report findings in severity order. If no material findings remain, say so and note verification that could not be performed.

A review is not an implementation pass unless fixes were also requested.
