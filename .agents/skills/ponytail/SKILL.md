---
name: ponytail
description: Use when a coding, refactoring, design, or review task risks over-engineering, speculative abstraction, unnecessary dependencies, or the user asks for the simplest/minimal/YAGNI solution. Apply only after understanding the real code flow; never override explicit requirements, work-item acceptance criteria, project verification, security boundaries, or required testing.
metadata:
  version: "1.0"
  upstream: "DietrichGebert/ponytail@b6c04480c03e8db2f035751d7c46289779ec3362"
  license: "MIT"
---

# Ponytail — Loop adaptation

Use minimalism as an implementation decision tool, not as a persistent mode. This Skill does not install hooks, alter global agent behavior, or replace the repository's normal implementation lifecycle.

## Precedence

The user's explicit request, `AGENTS.md`, `PROJECT.md`, the current `[Spec]` / `[Task]`, acceptance criteria, project verification, merge gates, and security requirements take priority over this Skill.

Do not use minimalism to reinterpret a requirement away. The goal is the smallest correct implementation of the actual requirement.

## Decision ladder

After reading the task and tracing the code path it affects, stop at the first option that fully satisfies the requirement:

1. **No new mechanism** — does the requested outcome already exist, or can the requirement be met without adding code/configuration?
2. **Existing repository capability** — reuse an existing helper, module, type, workflow, convention, or configuration surface.
3. **Standard library** — prefer a language/runtime standard facility over custom infrastructure.
4. **Native platform capability** — prefer platform, browser, database, OS, GitHub, cloud, or protocol features when they directly cover the need.
5. **Already-installed dependency** — reuse an existing dependency before adding another one.
6. **Smallest correct change** — add only the code needed at the correct ownership/root-cause boundary.

Do not turn the ladder into a research project. First understand the flow; then choose the highest rung that actually holds.

## Root-cause fixes

For a bug, the smallest durable fix is usually at the shared invariant boundary rather than at one reported symptom.

Before adding a local guard, inspect the relevant callers/consumers. If they all pass through one function or module that owns the invariant, fix it there. If behavior intentionally differs by caller, keep the fix scoped rather than inventing a false abstraction.

## Avoid unnecessary structure

Unless current requirements justify them, avoid:

- interfaces with one foreseeable implementation,
- factories for one product,
- configuration for values that are not intended to vary,
- wrappers that only rename another API,
- dependencies that replace a few clear standard-library/native lines,
- scaffolding for hypothetical future features,
- broad refactors when a narrow root-cause fix is sufficient.

Deletion or reuse is preferable to new code only when behavior, readability, security, and maintainability remain correct.

## Guardrails

Never simplify away:

- explicit user requirements or acceptance criteria,
- required project verification or merge-gate checks,
- tests required to prove changed behavior or by the selected TDD workflow,
- validation at trust boundaries,
- authorization, credential, privacy, or data-loss protections,
- accessibility requirements,
- error handling needed for deterministic/recoverable operation,
- reproducibility needed for CI, deployment, or operations.

Do not impose a universal "one test is enough" rule. Test scope follows the changed behavior and repository verification policy.

Do not alter work-item tracking, branch protection, workflow permissions, or security controls merely to make the solution smaller.

## Communication

When the chosen implementation deliberately omits a plausible larger mechanism, mention the omitted mechanism and the concrete condition that would justify adding it later. Keep this proportional to the task; user-requested reports, walkthroughs, or detailed explanations take precedence.

## Upstream adaptation

This Skill adapts the useful minimal-solution ladder from `DietrichGebert/ponytail` while intentionally omitting its always-on persistence, plugin/hooks integration, fixed terse-output rule, and universal minimal-test guidance because those conflict with this repository's explicit task, project verification, review, and testing contracts.
