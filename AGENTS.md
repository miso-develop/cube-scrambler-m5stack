# Agent Instructions

## Source of truth

- ユーザーの最新かつ明示的な指示を最優先する。
- project全体の目的・scope・制約・不変条件は`PROJECT.md`を正とする。
- feature / work itemのplanning・decision・implementation stateと変更履歴はGitHub Issues / Pull Requestsを正とし、`[Map]` / `[Decision]` / `[Spec]` / `[Task]`の順に具体化する。
- repositoryは現在のsystem stateのsource of truthとし、必要に応じてcode、configuration、durable documentation、executable verificationで表現する。closed Issueだけを現在仕様の参照元にしない。
- Map / Decision / Specから得た知識のうち将来もcurrent truthとして必要なものは、`agent/WORK-TRACKING.md`のrepository knowledge lifecycleに従ってrepositoryへ反映する。
- Specは満たすべきcontractであり、test・static check・build check・runtime check・reviewはverification evidenceである。verification artifactだけを理由にSpecの意味・意図・境界を省略しない。
- `AGENTS.md`には開発プロセスと横断的制約だけを置く。
- 既存コードやtestは重要な根拠だが、明示要件と矛盾する場合に要件を黙って変更しない。

Cube Scrambler固有のfirmware / device resource / solver / Servo / Web UI制約は`agent/CUBE-ENGINEERING.md`、CI / Artifact /実機検証は`agent/VERIFICATION.md`を参照する。

## Work items

production codeの実装対象として選択できるのは、GitHub上に実在するopenな`[Task]` Issueだけとする。

選択するTaskは次を満たす。

- `Parent spec`が明示されている。
- Parent Specがopenで、Taskがその`Implementation tasks` indexに登録されている。
- `Blocked by`のIssueがすべてclosedである。
- Acceptance Criteriaが外部から判定可能である。
- 同じTaskを扱う未完了PRがある場合は新規branchを作らず、そのPR / branchを継続する。

実装可能なTaskがない場合はproduction codeを推測して変更しない。新しいworkは規模と不確実性に応じて`wayfinder`、`to-spec`、`to-tickets`で具体化する。

Issue形式・関係・handoff・repository knowledge promotion規則は`agent/WORK-TRACKING.md`を参照する。

### Automated dependency maintenance

Dependabotのdependency-only PRは新しいproduction behaviorを設計するworkではないため、`[Task]` metadataを要求しない。ただし通常のCI / security reviewは適用する。

- ordinary user / agent PRへこの例外を拡張しない。
- Dependabot PRへ人手の追加実装や挙動変更が必要になった場合はdependency-only maintenanceとして扱わず、必要な`[Spec]` / `[Task]`を作って通常lifecycleへ戻す。

## One implementation iteration

1. `PROJECT.md`、対象`[Task]`、親`[Spec]`、参照Decision / artifact / comments、関連するrepository current truth（code / config / durable docs / verification）を確認する。
2. readyなTaskを正確に1件だけ選び、Issue URLまたは`owner/repo#number`を確定する。
3. 同Taskの未完了PR / branchがなければ最新`main`から作業branchを作る。通常実装を`main`へ直接commitしない。
4. 選択Taskと既存挙動維持に必要な最小変更だけを行う。設計判断には`codebase-design`、over-engineering判断には`ponytail`、test-firstが適切なら`tdd`を使う。Taskが必要とするcurrent truth更新はcodeだけでなくconfig / durable docs / verificationも含めてよい。
5. 外部から観測可能な挙動を優先してtestを追加・更新し、targeted checkを実行する。
6. Task / Specおよび変更範囲に適用されるproject固有verificationを`agent/VERIFICATION.md`に従って実行する。firmware / release変更では既存Hosted CIと同等のbuild / layout / bundle checksを基準にする。
7. `code-review`で要件適合とengineering qualityを確認し、有効なblocking findingを修正して影響範囲を再検証する。
8. greenであればcommit / pushし、PR本文に`Parent spec: #...`とstandaloneな`Closes #...`を入れる。
9. merge前に変更範囲へ適用されるGitHub Actions checksがlatest PR headでsuccessであることを確認する。workflowのpath filterにより自動checkが存在しないdocs-only変更では、再現可能なmanual review / validationをPRへ記録する。
10. TaskはPR mergeによってcloseされて初めて完了とする。同じiterationで次のTaskへ進まない。
11. Task merge後にParent Specのcloseoutを再評価し、全Task closedだけでなくRequirements、verification、repository knowledge promotion、repository current truthの整合がすべて成立した場合だけSpecをcloseする。不足があればSpecをopenのまま維持し、必要なTaskを追加する。

## Verifier boundary

このrepositoryではLoop Verifier mechanismを使用しない。

- `verifier/`を導入しない。
- `loop-verifier`やLoop work-item validator statusをmerge条件にしない。
- Shared Local Verifier / Cloud Run verifierへfallbackしない。
- verificationはproject固有のtest、build、GitHub Actions、必要な実機確認で成立させる。

Verifier導入は別の明示Spec / user instructionなしに行わない。

## Incomplete / blocked iteration

iterationを完了できない場合は新しいproduction changeを増やすのを止め、`handoff`を使用して対象Task IssueまたはPRへ再開checkpointを残す。

checkpointにはsecretを含めず、branch / HEAD、PR、完了済み範囲、verification結果、未反映のrepository knowledge、blocker、次の具体的操作を記録する。独立した進捗ファイルを二重管理しない。

## Engineering constraints

- 要件にない機能、依存、抽象化、大規模refactorを追加しない。
- 選択Task外の既存挙動を意図せず変更しない。
- 一時debug code、不要log、生成物、credentialをcommitしない。
- secret、PAT、private key、webhook secret、authorization header等をrepository、Issue、PR、handoffへ記録しない。
- test / required CIがfailureのままmergeしない。
- green化のためにtest、Acceptance Criteria、project verification contract、security boundaryを削除・弱体化しない。contract変更には明示された`[Spec]` / `[Task]`またはuser instructionを必要とする。
- `.cmd`は`.gitattributes`に従いworking treeだけでなくcommitted blobもCRLFを維持する。
- Windows command entrypointは`.cmd`に統一し、`.bat`は新規導入しない。

## Auxiliary rules

`agent/`配下は常時適用ではないconditional policy、`.agents/skills/`は特定タスク向けの再利用可能procedureとして役割を分ける。

- planning、task分解、handoff、work item lifecycle、repository knowledge promotion: `agent/WORK-TRACKING.md`
- merge gate / branch protection / ruleset: `agent/MERGE-GATES.md`
- firmware、device resource、solver、Servo、Web UI / HTTP API、Serial debug: `agent/CUBE-ENGINEERING.md`
- GitHub Actions、Artifact、Visual QA、release validation、実機verification: `agent/VERIFICATION.md`
- reusable procedures: `.agents/skills/`

## Failure handling

想定外のfailure、regression、broken automation、同じ失敗の反復が起きた場合は`diagnosing-bugs`でroot causeを確認し、再利用価値がある場合だけ`troubleshooting-cases`へcaseを残す。正常なiterationではcase libraryを常時読み込まない。

## Done

Taskを完了扱いにできるのは次をすべて満たす場合だけ。

- Acceptance Criteriaをすべて満たす。
- 必要な自動testまたは再現可能なverification evidenceがある。
- 変更範囲へ適用されるrequired CI / checksがgreenである。
- 必要な実機確認がTask / Specに定義されている場合、そのevidenceがある。
- `code-review`のblocking findingが解消されている。
- 既知のregressionや未解決矛盾がない。
- Taskで必要なrepository current truth更新がmerge対象に含まれている。
- PRがmergeされ、`Closes #<task-number>`により対象Issueがclosedになっている。
