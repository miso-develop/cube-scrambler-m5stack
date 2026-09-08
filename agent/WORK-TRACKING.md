# Work Tracking

Loop Engineeringのplanning / implementation状態はGitHub Issues / Pull Requestsへ集約します。repository内に別の進捗台帳を作りません。

## Manual issue creation

GitHub UIから作成する場合は`.github/ISSUE_TEMPLATE/`のIssue Formsを使用します。

- Loop Map → `[Map]`
- Loop Decision → `[Decision]`
- Loop Spec → `[Spec]`
- Loop Task → `[Task]`

Agent/APIがIssueを直接作成する場合も、以下のcanonical body形式に従います。

## Structural enforcement

このrepositoryにはLoop work-item専用Verifier / validator workflowを導入しません。work item構造はAgent / reviewerがこの文書に基づいてfail-closedに確認します。

`Blocked by`は次のどちらかだけを使用します。

```md
## Blocked by
- None
```

または同一repositoryのstandalone Issue参照:

```md
## Blocked by
- #123
- #456
```

Task blockerは`[Task]`、Decision blockerは`[Decision]`だけを参照し、self dependencyとcycleを禁止します。

PRでは最低限、Parent Specとclosing Taskが一致し、blockerがclosedで、同じTaskをcloseする競合open PRがないことを確認します。

## Work item types

### `[Map]`

```md
## Destination
<完了時に何が決まっていればよいか>

## Decisions so far
- <resolved decision and outcome>

## Not yet specified
- <まだ明確なquestionにできないin-scopeのfog>

## Out of scope
- <今回扱わないこと>
```

すべてのin-scope decisionが解決し、それを参照するSpecが作成されたらcloseします。

### `[Decision]`

```md
## Parent map
#<map-number>

## Question
<このIssueで決める1つのquestion>

## Blocked by
- None
```

解決時はanswer / evidenceをcommentに残してcloseし、親Mapの`Decisions so far`へlinkします。

### `[Spec]`

```md
## Problem
<user perspectiveの問題>

## Outcome
<実現する結果>

## Requirements
- <observable requirement>

## Decisions
- <implementation / behavior decision>

## Verification
- <重要なtest seam / observable check>

## Out of scope
- <明示的に扱わないこと>

## References
- `PROJECT.md`

## Implementation tasks
- [ ] #<task-number> <task title>
```

SpecはImplementation tasksの全Taskがclosedで、Requirements全体を満たしたことを確認してcloseします。

### `[Task]`

```md
## Parent spec
#<spec-number>

## What to build
<このticketだけで成立するbehavior>

## Acceptance criteria
- [ ] <observable criterion>

## Blocked by
- None
```

Taskは対応PRのmergeで`Closes #<task-number>`によりcloseします。

## Frontier

implementation frontierは次をすべて満たすTaskです。

- open
- Parent SpecがopenでImplementation tasksに登録済み
- dependency graphがacyclic
- Blocked byがすべてclosed
- 同Taskの未完了PRがない、またはそのPRを継続できる

## Planning route

- 小さく明確: `to-spec` → `to-tickets`
- 大きい / 曖昧 / decision dependencyが多い: `wayfinder` → `to-spec` → `to-tickets`
- 実装: readyな`[Task]` 1件 → `implement`

`wayfinder`はplanning専用でproduction codeを実装しません。

## PR lifecycle

1 Taskにつき1 branch / PRを基本とします。PR bodyには最低限:

```md
Parent spec: #<spec-number>
Closes #<task-number>
```

を含めます。

merge前にはTaskのAcceptance Criteria、required project checks、review findingsを確認します。Verifier statusは要求しません。

Task merge後、Parent SpecのTask一覧を確認し、全TaskがclosedかつRequirementsも満たしていればcompletion commentを残してSpecをcloseします。

## Handoff

正常merge済みiterationには別handoffを作りません。中断 / block時だけactive IssueまたはPRへ次を残します。

```md
## Handoff
- Branch / HEAD:
- PR:
- Completed:
- Verification:
- Blocker:
- Next action:
- References:
```

secretや個人情報をcheckpointへ記録しません。
