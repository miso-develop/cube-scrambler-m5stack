---
name: to-spec
description: Turn already-settled conversation or completed Wayfinder decisions into a durable GitHub `[Spec]` Issue. Use after alignment is complete and before implementation tickets are created.
metadata:
  version: "1.0"
---

# To Spec

Synthesize what has already been decided. Do not reopen settled choices and do not invent answers to unresolved questions.

Read `PROJECT.md`, the current conversation, completed Map/Decision evidence, and architecture references that materially constrain the feature.

If an unresolved choice changes observable behavior, architecture, security, cost, compatibility, or acceptance criteria, route it back through `grilling` / `wayfinder` instead of guessing.

Create or update one `[Spec] <feature>` using `agent/WORK-TRACKING.md` with Problem, Outcome, Requirements, Decisions, Verification, Out of scope, References, and Implementation tasks.

Requirements describe observable behavior. Project-wide constraints stay in `PROJECT.md` rather than being duplicated.

Before publishing, ensure every requirement has an observable verification path and no unresolved decision is disguised as implementation freedom.

Do not implement production code. The next step is `to-tickets`.

Adapted from `mattpocock/skills` `to-spec`.
