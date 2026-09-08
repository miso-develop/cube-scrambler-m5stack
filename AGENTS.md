# Agent Instructions

## Source of truth

- ユーザーの最新かつ明示的な指示を最優先する。
- project全体の目的・scope・制約・不変条件は`PROJECT.md`を正とする。
- feature / work itemの詳細はGitHub Issuesを正とし、`[Map]` / `[Decision]` / `[Spec]` / `[Task]`の順に具体化する。
- `AGENTS.md`には開発プロセスと横断的制約だけを置く。
- 既存コードやtestは重要な根拠だが、明示要件と矛盾する場合に要件を黙って変更しない。

## Work items

production codeの実装対象として選択できるのは、GitHub上に実在するopenな`[Task]` Issueだけとする。

選択するTaskは次を満たす。

- `Parent spec`が明示されている。
- `Blocked by`のIssueがすべてclosedである。
- Acceptance Criteriaが外部から判定可能である。
- 同じTaskを扱う未完了PRがある場合は新規branchを作らず、そのPR / branchを継続する。

実装可能なTaskがない場合はproduction codeを推測して変更しない。新しいworkは規模と不確実性に応じて`wayfinder`、`to-spec`、`to-tickets`で具体化する。

Issue形式・関係・handoff規則は`agent/WORK-TRACKING.md`を参照する。

Dependabotのdependency-only PRは新しいproduction behaviorを設計するworkではないため、`[Task]` metadataを要求しない。ただし通常のCI / security reviewは適用する。

## One implementation iteration

1. `PROJECT.md`、対象`[Task]`、親`[Spec]`、参照Decision、関連コードとtestを確認する。
2. readyなTaskを正確に1件だけ選び、Issue URLまたは`owner/repo#number`を確定する。
3. 同Taskの未完了PR / branchがなければ最新`main`から作業branchを作る。通常実装を`main`へ直接commitしない。
4. 選択Taskと既存挙動維持に必要な最小変更だけを行う。設計判断には`codebase-design`、test-firstが適切なら`tdd`を使う。
5. 外部から観測可能な挙動を優先してtestを追加・更新し、targeted checkを実行する。
6. Task / Specおよび変更範囲に適用されるproject固有verificationを実行する。firmware / release変更では既存GitHub Actionsと同等のbuild / layout / bundle checksを基準にする。
7. `code-review`で要件適合とengineering qualityを確認し、有効なblocking findingを修正して影響範囲を再検証する。
8. greenであればcommit / pushし、PR本文に`Parent spec: #...`と`Closes #...`を入れる。
9. merge前に変更範囲へ適用されるGitHub Actions checksがsuccessであることを確認する。workflowのpath filterにより自動checkが存在しないdocs-only変更では、再現可能なmanual review / validationをPRへ記録する。
10. TaskはPR mergeによってcloseされて初めて完了とする。同じiterationで次のTaskへ進まない。

## Verifier boundary

このrepositoryではLoop Verifier mechanismを使用しない。

- `verifier/`を導入しない。
- `loop-verifier` statusをmerge条件にしない。
- Shared Local Verifier / Cloud Run verifierへfallbackしない。
- verificationはproject固有のtest、build、GitHub Actions、必要な実機確認で成立させる。

Verifier導入は別の明示Spec / user instructionなしに行わない。

## Incomplete / blocked iteration

iterationを完了できない場合は新しいproduction changeを増やすのを止め、`handoff`を使用して対象Task IssueまたはPRへ再開checkpointを残す。

checkpointにはsecretを含めず、branch / HEAD、PR、完了済み範囲、verification結果、blocker、次の具体的操作を記録する。独立した進捗ファイルを二重管理しない。

## Engineering constraints

- 要件にない機能、依存、抽象化、大規模refactorを追加しない。
- 選択Task外の既存挙動を意図せず変更しない。
- 一時debug code、不要log、生成物、credentialをcommitしない。
- secret、PAT、private key、authorization header等をrepository、Issue、PR、handoffへ記録しない。
- test / required CIがfailureのままmergeしない。
- green化のためにtestやAcceptance Criteriaを削除・弱体化しない。
- `.cmd`はCRLFを維持し、Windows command entrypointは`.cmd`に統一する。

## Auxiliary rules

- planning、task分解、handoff、work item lifecycle: `agent/WORK-TRACKING.md`
- merge gate / branch protection / ruleset: `agent/MERGE-GATES.md`
- reusable procedures: `.agents/skills/`

## Done

Taskを完了扱いにできるのは次をすべて満たす場合だけ。

- Acceptance Criteriaをすべて満たす。
- 必要な自動testまたは再現可能なverification evidenceがある。
- 変更範囲へ適用されるrequired CI / checksがgreenである。
- `code-review`のblocking findingが解消されている。
- 既知のregressionや未解決矛盾がない。
- PRがmergeされ、`Closes #<task-number>`により対象Issueがclosedになっている。
