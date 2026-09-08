---
name: handoff
description: Preserve resumable state when a Loop Engineering planning or implementation session must stop before completion. Write a compact checkpoint to the current GitHub Issue or PR.
metadata:
  version: "1.1"
---

# Handoff

Use only when work must continue in another session or agent. Completed work needs no separate handoff: closed Issues, merged PRs, commits, and project verification are the durable record.

Prefer the current Task Issue, implementation PR, Decision Issue, then Spec/Map as the checkpoint target.

Post only state the next agent cannot cheaply reconstruct:

```md
## Handoff
- Branch / HEAD: `<branch>` / `<sha>`
- PR: <url or none>
- Completed: <what is already true>
- Verification: <commands/checks and current result>
- Blocker: <why this session cannot finish>
- Next action: <single concrete resume step>
- References: <spec / decisions / logs / artifact links>
```

Do not claim a check passed unless it did. Never include credentials, tokens, private keys, authorization headers, or unnecessary personal information.

A resuming agent must reread the current Issue/PR, parent Spec, linked Decisions, branch HEAD, and relevant code. The handoff is a pointer, not a replacement for source-of-truth artifacts.

Adapted from `mattpocock/skills` `handoff`.
