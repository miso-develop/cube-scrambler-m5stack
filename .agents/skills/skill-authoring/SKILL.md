---
name: skill-authoring
description: Use when creating, editing, reviewing, or deciding whether to add an agent Skill. Keep Skills minimal, triggerable, standalone where practical, and demonstrably better than the default behavior.
metadata:
  version: "1.0"
---

# Skill Authoring

Create Skills only when a repeatable task or failure mode benefits from explicit procedure or reference material.

## 1. Classify before writing

First decide where the material belongs:

- `AGENTS.md`: always-applicable process rule or cross-cutting constraint.
- `agent/`: conditional operational policy or decision rule.
- existing Skill: extend it when the responsibility already belongs there.
- new Skill: reusable task procedure or reference that should load only when triggered.

Do not create a Skill merely to restate behavior an agent already performs reliably by default.

## 2. Define the trigger

The frontmatter `description` is the routing contract. Make it concrete enough that an agent can distinguish prompts that should trigger the Skill from nearby prompts that should not.

Prefer one responsibility per Skill. If the trigger needs a long list of unrelated cases, split the Skill.

## 3. Write the smallest effective body

Use progressive disclosure:

- keep main `SKILL.md` focused on steps/rules required on every invocation;
- move branch-specific reference material into sibling files only when it would otherwise obscure the main procedure;
- do not duplicate facts easily discoverable from repository configuration or tool output;
- keep each rule in one authoritative location.

Prefer positive target behavior over long prohibition lists. Give each procedure an observable completion criterion.

## 4. Remove accidental dependencies

A Skill should be standalone where practical. Do not reference another Skill, script, directory, tracker, environment, or tool unless that dependency is actually provided and necessary.

When adapting a third-party Skill:

1. run `skill-security-review` before adopting executable or instruction content;
2. preserve required license and attribution notices;
3. remove upstream project-specific assumptions;
4. state material modifications when the upstream license requires it.

## 5. Evaluate behavior

Before treating the Skill as useful, exercise a small evaluation set:

- 2–5 realistic prompts that should trigger it;
- at least 2 nearby prompts that should not trigger it;
- one difficult or ambiguous case if the responsibility has an important edge condition.

Compare desired behavior with and without the Skill when practical. Keep instructions that measurably improve correctness, consistency, safety, or efficiency; remove no-op or harmful instructions.

For changes to an existing Skill, include at least one regression prompt representing the problem that motivated the change.

## 6. Review cost

Before completion, ask:

- Does this overlap another Skill?
- Could this be a short pointer in `AGENTS.md` or `agent/` instead?
- Is the description specific enough to route correctly?
- Is any body text only explanatory prose with no behavioral effect?
- Can a reference section be removed or disclosed behind a pointer?

## Completion criteria

Done only when the Skill has a clear routing boundary, no unnecessary repository-specific dependency, an observable completion criterion, a small evaluation set, and any third-party attribution/security review is accounted for.

This Skill combines and substantially simplifies ideas from `mattpocock/skills` `writing-for-agents` and Anthropic's `skills/skill-creator`. The bundled version intentionally omits upstream evaluation UI, scripts, sub-agent orchestration, and platform-specific setup.
