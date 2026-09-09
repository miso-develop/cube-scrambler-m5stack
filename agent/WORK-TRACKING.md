# Work Tracking

Loop Engineeringのplanning / implementation状態はGitHub Issues / Pull Requestsへ集約します。repository内に別の進捗台帳を作りません。

## Source-of-truth boundary

Loop Engineeringではchange lifecycleとcurrent system truthを分けます。

```text
GitHub Issues / Pull Requests
    = planning / decision / implementation state と変更履歴の source of truth

Repository
    = 現在のsystem stateの source of truth
```

repositoryのcurrent truthはdocumentationだけを意味しません。現在有効な状態は、必要に応じて次の組み合わせで表現します。

- code
- configuration
- durable documentation
- executable tests / static checks / build checks / runtime verification

closed Issueは「なぜそうなったか」を知る履歴として重要ですが、将来も有効なarchitecture ruleやcontractを理解するために過去Issueの探索を必須にしてはいけません。Map / Decision / Specで得た知識のうち、今後もcurrent system truthとして必要なものはrepositoryへpromotionします。

一方、Issueの内容を機械的にrepository documentへコピーしません。Issueとrepository documentを二重正本にするとdriftするため、promotionは情報の寿命と責務に基づいて判断します。

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

## Repository knowledge lifecycle

Map / Decision / Specで得られた情報について、workの各段階、とくにSpec closeout時に次を問います。

> この情報は、次回以降の開発でも「現在のsystem state」として知られている必要があるか？

YESならrepository current truthへpromotionします。promotion先は情報の性質とproject既存構造に合わせます。候補は`PROJECT.md`、既存architecture/specification docs、必要に応じて作成するdocs、code/configuration、executable verificationなどです。

すべてのMap / Decision / Specをrepository documentへコピーしません。必要なdurable knowledgeだけを既存構造へ統合します。

### Map knowledge

GitHubの`[Map]`は原則として**Exploration Map**です。問題空間、unknown、dependency、decision pointを構造化する一時的planning artifactであり、EpicやTask一覧ではありません。

探索結果として現在のsystem structure自体を将来も参照する必要がある場合、その部分はrepository architecture documentation等へpromotionできます。Exploration Map Issueそのものを恒久保存する必要はありません。

### Decision knowledge

Decisionは1つの判断を確定するwork itemです。

Issue履歴だけでよい例:

- 一時的なimplementation choice
- 変更完了後に将来を拘束しない局所判断
- code / Specから現在の意図が十分明確な判断

repositoryへpromotionすべき例:

- architecture / dependency direction
- security / trust boundary
- compatibility policy
- subsystem ownership / responsibility boundary
- 将来別案へ変更するときに現在案の理由を知る必要がある判断

Decisionは実装進捗を追うために開け続けません。answer / evidenceが確定し、必要なdurable promotionが完了済みか、下流のSpec / Taskへ明示的にownership移管されたらcloseできます。

### Spec and verification

SpecとTest / Checkは同義ではありません。

```text
Spec
    = systemが満たすべき contract

Test / Static Check / Build Check / Runtime Check / Review
    = contractが成立していることを確認する verification evidence
```

したがって`Spec != Test`です。automated testで完全に表現できるSpecもありますが、architecture constraint、security boundary、operational rule、実機support contractなどはbuild check、runtime check、review、durable documentation等と組み合わせて表現される場合があります。

Change-specific Specは変更履歴としてclosed Issueに残れば十分な場合があります。変更後も将来を拘束するDurable Contractは、Issueだけに閉じ込めずrepository current truthへpromotionします。

### Task knowledge

Taskは実装・変更単位です。Task Issue自体を恒久documentationとしてrepositoryへ保存しません。履歴はTask Issue → PR → commit historyに残ります。

production code変更は必ずopenなTaskの下で行いますが、Taskは同じ成果を成立させるために必要なconfiguration、durable documentation、verification updateを含めて構いません。repository knowledge promotionが独立して実装・review可能なら専用Taskにしてもよく、behavior changeと不可分なら同じTaskのAcceptance Criteriaに含めます。

## Work item types

### `[Map]`

大きい・曖昧・複数sessionにまたがるworkのExploration Mapです。`wayfinder`が作成します。

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

Mapは次を満たした時点でcloseします。

- unresolvedなin-scope Decisionがない。
- `Not yet specified`が空、または明示的にout of scopeへ移されている。
- 必要なdownstream`[Spec]`が作成・linkされている。

Taskやimplementationの完了までは待ちません。MapをEpicとして使わないためです。close commentには作成したSpecをlinkします。

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

open DecisionのParent Mapはopenである必要があります。解決時はanswer / evidenceをcommentに残します。durable decisionならrepository promotionの要否とownershipも明示します。判断が確定し、必要なpromotionが完了済みまたはdownstream Spec / Taskへ明示的に引き継がれたらDecisionをcloseし、親Mapの`Decisions so far`へlinkします。

### `[Spec]`

実装前に確定したfeature contractです。`to-spec`がconversationまたは完了Mapから作成します。

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

## Repository knowledge
<初期assessment。必要に応じてdurable knowledge候補を記載。closeout時に必ず再判定する>

## Implementation tasks
- [ ] #<task-number> <task title>
```

`Repository knowledge`はadvisoryな補助sectionです。Spec作成時に正確に判定できない場合があるため、closeout時の再判定をcanonical ruleとします。

`Implementation tasks`はそのSpec配下Taskのcanonical indexです。Task merge後はcheckboxを更新してよいですが、Issueのopen/closed stateを完了判定の正とします。

project-wide constraintをSpecへ複製せず`PROJECT.md`を参照します。

Specは次をすべて満たした場合だけcloseできます。

1. `Implementation tasks`に列挙されたすべての`[Task]`がclosedである。
2. Spec Requirementsがすべて成立している。
3. 必要なautomated / reproducible verificationが存在し、結果が確認されている。実機verificationがcontractに含まれる場合はそのevidenceも含む。
4. repository knowledge impactをcloseout時点で再判定済みである。
5. 必要なdurable knowledge promotionがdefault branchへmerge済みである。
6. repository documentation / implementation / configuration / verificationの間に既知の矛盾がない。

Taskやpromotionが不足していることが判明した場合はSpecをcloseせず、不足Taskを作成してindexへ追加します。promotion用PRを作成しただけではclose条件を満たさず、必要な変更がdefault branchへ反映済みであることを要求します。

### `[Task]`

repositoryを変更するimplementation work itemです。production codeを変更できるのはこのwork itemだけです。

```md
## Parent spec
#<spec-number>

## What to build
<このticketだけでend-to-endに成立するbehavior / repository state>

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

Task merge後、Parent Specの`Implementation tasks`を確認します。全Taskがclosedでも即座にSpecをcloseせず、Requirements、verification、repository knowledge promotion、current repository truthの整合を上記closeout ruleで再確認します。必要なpromotionが未mergeならSpecをopenのまま維持し、不足Taskを追加します。すべて満たした場合のみcompletion summaryをSpecへ残してcloseします。

## Handoff

正常にmergeして終了したiterationには別handoffを作りません。GitHub Issue / PR / commit historyがその記録です。

中断・block時だけ`handoff`がcurrent Task / DecisionまたはPRへcheckpoint commentを追加します。

```md
## Handoff
- Branch / HEAD:
- PR:
- Completed:
- Verification:
- Repository knowledge:
- Blocker:
- Next action:
- References:
```

`Repository knowledge`には未反映のdurable promotionまたは`None`を記載します。handoff comment自体をcurrent specificationの保存先にしません。

既にSpec / Issue / PR / diffにある内容を長文で複製しません。secretや不要な個人情報をcheckpointへ記録しません。
