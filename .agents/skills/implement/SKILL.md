---
name: implement
description: Implement exactly one ready GitHub `[Task]` Issue through branch, repository changes, project verification, review, PR, and merge-or-handoff.
metadata:
  version: "1.1"
---

# Implement

Implement one Task. Do not reopen planning unless the ticket is contradictory or missing a material decision.

## 1. Pin the work item

Resolve the exact Task by Issue URL or `owner/repo#number`. Read the complete Task, Parent Spec, linked Decisions, `PROJECT.md`, blockers, existing PR/branch, latest applicable handoff, and repository current truth that materially constrains the Task, including relevant code, configuration, durable documentation, and executable verification.

Closed Issues explain change history; when knowledge is supposed to remain valid, prefer the repository's current state over reconstructing the contract from historical Issues.

Fail closed if the parent cannot be resolved, the Task is absent from the Spec index, or a blocker remains open.

## 2. Establish the branch

Resume an existing unfinished PR/branch for the same Task. Otherwise branch from latest `main`. Do not implement normal production work directly on `main`.

## 3. Implement the smallest complete slice

Restate the behavior and Acceptance Criteria. Keep changes inside the selected Task. Use `codebase-design` where module/interface decisions are required and `tdd` where a stable observable seam exists.

A Task may include code, configuration, durable documentation, and verification changes required to leave repository current truth accurate.

If implementation reveals a missing material decision, stop and use `handoff`; return that decision to planning instead of inventing it.

If implementation reveals durable knowledge that must survive the change but is not covered by this Task, do not leave it only in Issue/PR discussion. Update the repository within scope or keep the Parent Spec open and add the missing work through `to-tickets`.

## 4. Run required verification

Run the checks required by the Task/Spec and the repository paths changed. For firmware/distribution changes, align local checks with the existing Hosted CI contract where practical. Run `code-review`, fix valid blocking findings, and rerun affected checks.

This repository does not use Loop Verifier. Do not add or require verifier status as part of this Skill.

For docs/process-only changes that do not trigger Hosted CI because of path filters, do not touch product paths merely to create a CI run. Record reproducible review/validation evidence instead.

## 5. Open the PR

After applicable checks are green, commit/push and create or update one PR. Include `Parent spec: #<spec-number>`, `Closes #<task-number>`, and verification evidence.

## 6. Merge or hand off

Merge only when relevant GitHub checks are successful, Acceptance Criteria are met, review is clear, and the PR is mergeable. The Task is complete only when the merge closes it.

After merge, inspect the Parent Spec and re-evaluate repository knowledge impact. Close the Spec only when:

- every indexed Task is closed,
- all Spec Requirements are satisfied,
- appropriate automated or reproducible verification exists and required hardware evidence is complete where applicable,
- required durable knowledge promotion is merged into `main`,
- no known contradiction remains among repository documentation, implementation, configuration, and verification.

If work is still missing, leave the Spec open and create the missing Task through `to-tickets` rather than treating the feature as complete.

If the current environment cannot reach merge because of pending checks, failing external dependency, unresolved review finding, missing human decision, pending hardware evidence, pending repository knowledge promotion, or a session boundary, stop adding changes and use `handoff`.

Adapted from `mattpocock/skills` `implement` for verifier-free project-specific CI.
