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

Treat this Issue as an Exploration Map, not an Epic or implementation backlog. Its purpose is to expose unknowns, dependencies, and decision points. If exploration reveals system structure or policy that must remain understandable after this change, record that it may require repository knowledge promotion later; do not create documentation merely to mirror the Map.

## 3. Create decision issues

For each precise unresolved question, create `[Decision] <question>` with Parent map, one Question, and Blocked by. Decisions may use `research`, `grilling`, `codebase-design`, or a throwaway prototype for evidence, but must not deliver production behavior.

## 4. Work the frontier

The frontier is every open Decision whose blockers are closed. Resolve one focused Decision at a time unless independent research can safely run in parallel.

For each resolved Decision:

1. record the answer and evidence,
2. identify whether the decision is local/history-only or durable enough to constrain future development,
3. if durable repository promotion is needed, record that requirement explicitly for the downstream Spec/Task rather than leaving future agents to rediscover it from the closed Issue,
4. close the Decision,
5. link the outcome from the parent Map,
6. promote newly-clear fog into further Decisions when needed,
7. move newly-out-of-scope items to Out of scope.

A Decision does not stay open merely to track later implementation. Its lifecycle ends when the choice/evidence are settled and any required durable promotion has either been performed within valid scope or explicitly handed downstream.

## 5. Finish the map

The Exploration Map is ready to close when:

- no unresolved in-scope Decision remains,
- remaining fog is empty or explicitly out of scope,
- implementation-relevant decisions are stable enough to write a Spec,
- the required downstream Spec has been established and linked.

Then use `to-spec`. The source Map may close after the Spec is created; it does not wait for Tasks, implementation, verification, or hardware validation to finish.

If the session stops early, use `handoff` on the active Decision or Map and include any pending repository knowledge promotion.

Wayfinder is done when the path to an implementable Spec is clear, no unresolved in-scope decision remains hidden in fog, and any durable knowledge that must survive planning has been carried forward for repository promotion.

Adapted from `mattpocock/skills` `wayfinder` for the Loop Engineering GitHub Issue model.
