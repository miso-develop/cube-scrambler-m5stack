---
name: to-spec
description: Turn already-settled conversation or completed Wayfinder decisions into an implementation-ready GitHub `[Spec]` Issue. Use after alignment is complete and before implementation tickets are created.
metadata:
  version: "1.0"
---

# To Spec

Synthesize what has already been decided. Do not reopen settled choices and do not invent answers to unresolved questions.

Read `PROJECT.md`, the current conversation, completed Map/Decision evidence, current repository truth that materially constrains the feature, and architecture references that matter.

Closed Issues explain how the current state was reached, but they are not a substitute for repository current truth when information must remain valid for future development.

If an unresolved choice changes observable behavior, architecture, security, cost, compatibility, support criteria, or acceptance criteria, route it back through `grilling` / `wayfinder` instead of guessing.

Create or update one `[Spec] <feature>` using `agent/WORK-TRACKING.md` with Problem, Outcome, Requirements, Decisions, Verification, Out of scope, References, Repository knowledge, and Implementation tasks.

Requirements describe observable behavior. Project-wide constraints stay in `PROJECT.md` rather than being duplicated.

A Spec defines the contract. Tests, static checks, build checks, runtime/hardware checks, and review are verification evidence for that contract; do not collapse the specification into a list of tests.

`Repository knowledge` is an initial advisory assessment. Identify candidate durable knowledge where practical, but re-evaluate it at Spec closeout. Durable knowledge may need to become repository truth through code, configuration, durable documentation, or executable verification; do not assume every Spec needs a separate specification document.

Before publishing, ensure every requirement has an observable verification path, no unresolved decision is disguised as implementation freedom, and any obvious durable-knowledge impact is recorded.

If the Spec was produced from a Map, close the Map only after all in-scope Decisions are resolved, remaining fog is handled, and this Spec is created and linked. The Map does not remain open for implementation progress.

Do not implement repository changes. The next step is `to-tickets`.

Adapted from `mattpocock/skills` `to-spec`.
