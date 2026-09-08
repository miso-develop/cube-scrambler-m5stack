---
name: to-tickets
description: Break a settled GitHub `[Spec]` Issue into small vertical `[Task]` implementation tickets with explicit blockers.
metadata:
  version: "1.0"
---

# To Tickets

Convert one settled Spec into implementation tickets without redesigning the feature.

Read the Spec, relevant Decision outcomes, references, and `PROJECT.md`. If decomposition exposes an unresolved product or architecture decision, route it back to planning.

Each Task should be one narrow but complete, independently verifiable vertical slice that fits one implementation iteration and one coherent PR.

Create Task Issues using the canonical Parent spec, What to build, Acceptance criteria, and Blocked by sections in `agent/WORK-TRACKING.md`.

Create Issues first so real issue numbers exist, then update blocker references and the parent Spec's canonical `Implementation tasks` list.

Verify that the Task set covers the Spec requirements, the blocker graph is acyclic, and at least one Task is ready unless an explicit prerequisite blocks the whole Spec.

Do not implement any Task in this Skill.

Adapted from `mattpocock/skills` `to-tickets`.
