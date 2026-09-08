---
name: loop-status
description: Summarize the current ticket-driven Loop Engineering state for a repository. Use for current progress, remaining work, ready/blocked Tasks, planning items, PRs, handoffs, or next work.
metadata:
  version: "1.1"
---

# Loop Status

Build current state from GitHub Issues / Pull Requests. Do not maintain a separate progress ledger.

Collect open `[Map]`, `[Decision]`, `[Spec]`, `[Task]` Issues, open PRs, and relevant recent `## Handoff` comments.

For each open Task, resolve its Parent Spec, Blocked by references, blocker state, Spec index membership, and any open PR that closes it.

Classify:

- **Ready**: open, indexed by an open Parent Spec, blockers closed, no conflicting unfinished implementation; an existing resumable PR remains ready and should be resumed.
- **Blocked**: a declared blocker remains open or a required parent/reference cannot be resolved.
- **Closeout pending Spec**: Spec remains open but every indexed Task is closed; requirements still require semantic review before closing.

Report Summary, Ready Tasks, Blocked Tasks, Closeout pending Specs, Planning, Open PRs, latest actionable handoffs, and one concrete Next action when requested.

A status request is read-only. Do not silently create/edit/close Issues or begin implementation.
