---
name: implement
description: Implement exactly one ready GitHub `[Task]` Issue through branch, code, project verification, review, PR, and merge-or-handoff.
metadata:
  version: "1.1"
---

# Implement

Implement one Task. Do not reopen planning unless the ticket is contradictory or missing a material decision.

## 1. Pin the work item

Resolve the exact Task by Issue URL or `owner/repo#number`. Read the complete Task, Parent Spec, linked Decisions, `PROJECT.md`, blockers, existing PR/branch, and latest applicable handoff.

Fail closed if the parent cannot be resolved, the Task is absent from the Spec index, or a blocker remains open.

## 2. Establish the branch

Resume an existing unfinished PR/branch for the same Task. Otherwise branch from latest `main`. Do not implement normal production work directly on `main`.

## 3. Implement the smallest complete slice

Restate the behavior and Acceptance Criteria. Keep changes inside the selected Task. Use `codebase-design` where module/interface decisions are required and `tdd` where a stable observable seam exists.

If implementation reveals a missing material decision, stop and use `handoff`; return that decision to planning instead of inventing it.

## 4. Run required verification

Run the checks required by the Task/Spec and the repository paths changed. For firmware/distribution changes, align local checks with the existing Hosted CI contract where practical. Run `code-review`, fix valid blocking findings, and rerun affected checks.

This repository does not use Loop Verifier. Do not add or require verifier status as part of this Skill.

## 5. Open the PR

After applicable checks are green, commit/push and create or update one PR. Include `Parent spec: #<spec-number>`, `Closes #<task-number>`, and verification evidence.

## 6. Merge or hand off

Merge only when relevant GitHub checks are successful, Acceptance Criteria are met, review is clear, and the PR is mergeable. The Task is complete only when the merge closes it.

After merge, inspect the Parent Spec. If all indexed Tasks are closed and the full Spec Requirements are satisfied, leave a concise completion comment and close the Spec. Otherwise leave it open.

If the current environment cannot reach merge, stop adding changes and use `handoff`.

Adapted from `mattpocock/skills` `implement` for verifier-free project-specific CI.
