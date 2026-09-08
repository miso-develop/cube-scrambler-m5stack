# Work Tracking

Loop Engineeringのplanning / implementation状態はGitHub Issues / Pull Requestsへ集約します。repository内に別の進捗台帳を作りません。

## Manual issue creation

GitHub UIからwork itemを手動作成する場合は`.github/ISSUE_TEMPLATE/`のIssue Formsを使用します。

- `Loop Map` → `[Map]`
- `Loop Decision` → `[Decision]`
- `Loop Spec` → `[Spec]`
- `Loop Task` → `[Task]`

Formsはtitle prefixと主要sectionの入力漏れを減らす補助です。Agent/APIがIssueを直接作成する場合も以下のcanonical body形式に従います。

## Structural enforcement

このrepositoryにはLoop work-item専用Verifier / validator workflowを導入しません。work item構造はAgent / reviewerがこの文書に基づいてfail-closedに確認します。

`Blocked by`はdependency graphのcanonical source of truthとし、次のどちらかだけを使用します。

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

- `owner/repo#123`、Issue URL、説明文付き`#123 because ...`等をblocker欄では使用しない。
- Task blockerは`[Task]`、Decision blockerは`[Decision]`だけを参照する。
- self dependencyとmulti-hop cycleを禁止する。
- blocked item自体は正当なのでIssue作成時点ではblocker closeを要求しない。実装frontierへ入る時点でclosedを要求する。

`to-tickets`はTaskを作成してIssue番号を確定した後にParent Specの`Implementation tasks`を更新する二段階方式とする。そのためTask作成直後の短い間はindex未登録でもよいが、production implementation開始前にはmembershipを必須とする。

PRでは次を確認する。

- `Parent spec: #<spec-number>`が正確に1件ある。
- standaloneな`Closes #<task-number>`が正確に1件ある。
- `Closes`先がopenな`[Task]` Issueである。
- PRのParent SpecとTask本文のParent Specが一致する。
- Parent Specがopenである。
- TaskがParent Specの`Implementation tasks`に列挙されている。
- Taskの`Blocked by`がすべてclosedである。
- 同じTaskをcloseする別のopen PRが存在しない。

Dependabotのdependency-only maintenance PRはTask metadata例外として扱えるが、bot-generated dependency-only changeに限定する。人手の追加実装・挙動変更が必要になった時点で通常の`[Spec]` / `[Task]` lifecycleへ戻す。

## Work item types

### `[Map]`

大きい・曖昧・複数sessionにまたがるworkのdecision mapです。`wayfinder`が作成します。

```md
## Destination
<map完了時に何が決まっていればよいか>

## Decisions so far
- [<closed decision title>](<url>): <one-line outcome>

## Not yet specified
- <まだquestionとして切れないin-scopeのfog>

## Out of scope
- <今回扱わないこと>
```

Mapはすべてのin-scope Decisionが解決し、それを参照する`[Spec]`が作成された時点でcloseします。close commentには作成したSpecをlinkします。

### `[Decision]`

`[Map]`の下で1つのquestion / investigationを解決するIssueです。production implementation taskではありません。

```md
## Parent map
#<map-number>

## Question
<このIssueで決める1つのquestion>

## Blocked by
- None
```

open DecisionのParent Mapはopenである必要があります。解決時はanswer / evidenceをcommentに残してcloseし、親Mapの`Decisions so far`へlinkします。

### `[Spec]`

実装前に確定したfeature仕様です。`to-spec`がconversationまたは完了Mapから作成します。

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
- <Map / Decision / external references>

## Implementation tasks
- [ ] #<task-number> <task title>
```

`Implementation tasks`はそのSpec配下Taskのcanonical indexです。Task merge後はcheckboxを更新してよいですが、Issueのopen/closed stateを完了判定の正とします。

project-wide constraintをSpecへ複製せず`PROJECT.md`を参照します。

Specはindexの全Taskがclosedで、Spec Requirementsに未達がないことを確認した時点でcloseします。不足Taskが判明した場合はSpecをcloseせず、先にTaskを作成してindexへ追加します。

### `[Task]`

production codeを変更する唯一の実装work itemです。`to-tickets`がSpecをvertical sliceへ分解して作成します。

```md
## Parent spec
#<spec-number>

## What to build
<このticketだけでend-to-endに成立するbehavior>

## Acceptance criteria
- [ ] <observable criterion>

## Blocked by
- None
```

Task dependency graphはacyclicでなければなりません。native dependency / sub-issue機能は必須にせず、本文のcanonical Issue参照をportableなsource of truthとします。

Taskは対応PRのmergeで`Closes #<task-number>`によりcloseさせます。実装途中やlocal verification完了だけではcloseしません。

## Frontier

implementation frontierは次をすべて満たすTaskです。

- open
- Parent Specがopenで、その`Implementation tasks` indexに登録済み
- dependency graphがacyclic
- `Blocked by`がすべてclosed
- 同Taskの未完了PRがない、またはそのPRを継続できる

Issue番号だけを曖昧な文脈で解決しません。実装開始時はtitleとIssue URLまたは`owner/repo#number`を確定します。

## Planning route

- 小さく明確で既に合意済み: `to-spec` → `to-tickets`
- 大きい / 曖昧 / decision dependencyが多い: `wayfinder` → `to-spec` → `to-tickets`
- 実装: readyな`[Task]` 1件 → `implement`

`wayfinder`はplanning専用でproduction codeを実装しません。

## PR lifecycle

`implement`は1`[Task]`ごとに1 branch / PRを基本とします。

PR bodyには最低限:

```md
Parent spec: #<spec-number>
Closes #<task-number>
```

を含めます。

merge前にはTaskのAcceptance Criteria、required project checks、review findingsを確認します。Verifier statusは要求しません。

Task merge後、Parent Specの`Implementation tasks`を確認します。全TaskがclosedでSpec Requirements全体も満たされていれば、completion summaryをSpecへ残してcloseします。これは次のimplementation Taskではなく完了iterationのcloseoutです。

## Handoff

正常にmergeして終了したiterationには別handoffを作りません。GitHub Issue / PR / commit historyがその記録です。

中断・block時だけ`handoff`がcurrent Task / DecisionまたはPRへcheckpoint commentを追加します。

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

既にSpec / Issue / PR / diffにある内容を長文で複製しません。secretや不要な個人情報をcheckpointへ記録しません。
