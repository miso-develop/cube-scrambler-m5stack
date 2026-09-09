---
name: to-tickets
description: Break a settled GitHub `[Spec]` Issue into small vertical `[Task]` implementation tickets with explicit blockers.
metadata:
  version: "1.0"
---

# To Tickets

Convert one settled Spec into implementation tickets without redesigning the feature.

Read the Spec, relevant Decision outcomes, references, `PROJECT.md`, and repository current truth that materially affects decomposition, including relevant code, configuration, durable documentation, and executable verification.

If decomposition exposes an unresolved product or architecture decision, route it back to planning rather than hiding that decision inside a Task.

Revisit the Spec's initial `Repository knowledge` assessment. It is advisory only; decomposition may reveal durable knowledge work that was not obvious when the Spec was written.

Each Task should be one narrow but complete, independently verifiable repository change that fits one implementation iteration and one coherent PR.

A Task may include production code together with configuration, durable documentation, or executable verification required to make the same repository state complete and truthful. If required repository knowledge promotion is independently reviewable, create a dedicated Task for it. If it is inseparable from a behavioral slice, include it in that Task's Acceptance Criteria instead of creating duplicate documentation work.

Create Task Issues using the canonical Parent spec, What to build, Acceptance criteria, and Blocked by sections in `agent/WORK-TRACKING.md`.

Create Issues first so real issue numbers exist, then update blocker references and the parent Spec's canonical `Implementation tasks` list.

Verify that:

- every Spec requirement is covered or intentionally requires no repository change,
- every currently known repository knowledge promotion requirement is covered by a Task or explicitly requires no separate change,
- the blocker graph is acyclic,
- at least one Task is ready unless an explicit prerequisite blocks the whole Spec.

Do not implement any Task in this Skill.

Adapted from `mattpocock/skills` `to-tickets`.
