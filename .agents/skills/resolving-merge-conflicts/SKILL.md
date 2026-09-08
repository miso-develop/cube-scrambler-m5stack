---
name: resolving-merge-conflicts
description: Resolve an in-progress Git merge or rebase conflict while preserving the intent of both sides and avoiding invented behavior.
metadata:
  version: "1.1"
  source: "adapted from mattpocock/skills"
  license: "MIT"
---

# Resolving Merge Conflicts

Inspect the active merge/rebase state, conflicting files, relevant history, requirements, PR context, and tests. Identify why each side changed the conflicting area.

Resolve each hunk by preserving both intents when compatible. When incompatible, choose the result that matches the current requirement and state the trade-off rather than inventing a third behavior.

Remove every conflict marker, inspect the combined result as normal code, run focused checks for affected behavior and applicable project verification, then complete the merge/rebase only when coherent and green.

Do not delete a test or requirement merely to resolve a conflict, assume newer code is automatically correct, or silently combine mutually exclusive behavior.
