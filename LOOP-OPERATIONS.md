# Loop Engineering 運用ガイド

このrepositoryではGitHub Issues / Pull Requestsを中心にLoop Engineeringを運用します。Verifier infrastructureは使用しません。詳細なwork item形式やagent側lifecycleは`agent/WORK-TRACKING.md`を参照します。

## 基本モデル

```text
PROJECT.md
   │
   ├─ 小さく明確な変更
   │    └─ to-spec → to-tickets → implement
   │
   └─ 大きい / 曖昧 / 判断事項が多い変更
        └─ wayfinder → to-spec → to-tickets → implement

中断 / blocker
   └─ handoff
```

GitHub Issues / Pull Requestsはplanning・判断・implementation stateと変更履歴のsource of truthです。repositoryは現在有効なsystem stateのsource of truthで、code、configuration、durable documentation、tests/checksなど必要な形で保持します。通常完了したiterationについて別の進捗ファイルを更新しません。

`[Spec]`は「何が成立すれば完成か」というcontractです。test / static check / build check / runtime check / reviewはそのcontractを確かめるverification evidenceであり、Specそのものと同義ではありません。

Map / Decision / Specで得た内容をすべてrepositoryへコピーするわけではありません。次回以降の開発でも現在仕様として必要なdurable knowledgeだけをrepositoryへ反映します。詳細な判定・closeout ruleは`agent/WORK-TRACKING.md`を参照します。

## Work item

- `[Map]`: 大きく曖昧なworkのExploration Map。implementation Epicではない。
- `[Decision]`: Map配下の1つの判断事項。
- `[Spec]`: 実装前に確定したfeature contract。
- `[Task]`: repositoryを変更する実装単位。必要ならcodeに加えてconfig / durable docs / verification更新も含む。

## 1. 新しい要件・機能

小さく内容がほぼ決まっている場合:

> この要件をSpec化し、Taskへ分解して実装まで進めてください。

大きい・曖昧・複数の設計判断がある場合:

> まずwayfinderで判断事項と依存関係を整理してください。

ユーザーが事前にIssueを作る必要はありません。

Mapは実装進捗を追うEpicではありません。in-scopeのDecisionが解決し、残るfogが整理され、必要なSpecが作成された時点でcloseできます。TaskやPRの完了までは待ちません。

Decisionも実装完了を待つためにopenにしません。answer / evidenceが確定し、必要なdurable knowledge更新のownershipが決まったらcloseできます。

## 2. 要件だけ整理して実装しない

> この内容をSpec化してください。実装はまだ開始しないでください。

- 何を作るかがほぼ決まっている → `to-spec`
- 何を作るべきか、どう設計すべきかに未決定事項が多い → `wayfinder`

Spec作成時には、実装要件だけでなく「この変更後もrepository current truthとして残すべき知識があるか」も初期評価します。ただし最終判断はcloseout時に再評価します。

## 3. 既存要件を変更する

openなSpecの未実装部分ならSpec / Taskを必要に応じて更新します。

既にclose済みのSpecやproductionへmerge済みの要件を変更する場合は、過去Specを履歴として保持し、変更差分を新しい`[Spec]`として作成するのを基本とします。

project全体に恒久的に効く制約・不変条件が変わる場合は`PROJECT.md`を更新します。それ以外でも、将来の開発が現在仕様として知る必要があるarchitecture / decision rationale / durable contractが変わる場合は、適切なrepository truthも更新します。過去Issueだけを現在仕様の唯一の参照元にしません。

## 4. 実装だけ進める / 次Taskを任せる

対象TaskをIssue URLまたは`owner/repo#number`で指定するのが最も確実です。

> owner/repo#42 を実装してください。

次のready Task選択を任せる場合:

> 現在readyなTaskから次の1件を選んで実装してください。

1 implementation iterationで扱うTaskは1件です。

AgentはTask / Parent Spec / Decisionだけでなく、実装を拘束するrepository current truthも確認します。

## 5. 大きいSpecをTaskへ分割する

> [Spec] #30 を実装可能なTaskへ分解してください。まだ実装はしないでください。

`to-tickets`はvertical sliceを基本とし、作成TaskはSpecの`Implementation tasks`へ記録します。

repository knowledge promotionがbehavior changeと不可分なら同じTaskのAcceptance Criteriaへ含め、独立してreview可能なら専用Taskにできます。Task Issueそのものを恒久documentationとしてrepositoryへコピーする必要はありません。

## 6. 設計・技術調査

短い設計判断のpressure test:

> このAPI設計をgrillingしてください。

複数sessionにまたがるdecision map:

> このmulti-device設計をwayfinderで整理してください。

一次情報を使う技術調査:

> このdevice / libraryの仕様と互換性をresearchしてください。まだ実装は不要です。

調査やDecisionから将来も必要なarchitecture / compatibility / ownership ruleが得られた場合は、closed Issueだけに閉じ込めずdownstream Spec / Taskへrepository promotionを引き継ぎます。

## 7. 不具合を修正する

症状だけ伝えて原因調査から依頼できます。

> Windowsでこのコマンドだけ失敗します。原因調査から修正まで進めてください。

通常は:

1. `diagnosing-bugs`でreproductionとroot causeを特定する。
2. 既存open Taskに含まれるならそのworkを継続する。
3. 独立修正なら最小のSpec / Taskを作る。
4. `implement`で修正する。
5. 再利用価値のあるfailureだけ`troubleshooting-cases`へ残す。

## 8. 中断 / 別チャットへ引き継ぐ

> ここで止めます。次回再開できるようhandoffを残してください。

Agentはactive IssueまたはPRへcheckpointを残します。未反映のdurable knowledgeがある場合は、そのpromotionもcheckpointに明示します。handoff comment自体を恒久仕様の保存先にはしません。

正常merge済みworkにはhandoff不要です。

## 9. PR review / conflict解消

reviewのみ:

> PR #18 をcode-reviewしてください。修正はまだしないでください。

blocking findingの修正まで:

> PR #18 をreviewし、blocking findingがあれば修正して再検証してください。

merge conflict:

> PR #18 のmainとの競合を解消してください。

`resolving-merge-conflicts`はours/theirsを機械的に選ばず双方の変更意図を確認します。

## 10. 外部Skillを導入する

候補repository / Skillを指定します。

> このSkillを追加する価値を評価し、安全性も確認してください。

`skill-security-review`でcandidateをuntrusted dataとして静的reviewし、採用時は`skill-authoring`で責務・trigger・依存を整理します。third-party Skillを実行して安全性を確認しません。

## 11. 現在の進捗 / feature完了を確認する

status:

> openなMap / Decision / Spec / Taskと進行中PRを整理してください。

feature closeout:

> [Spec] #30 は完了扱いにできますか。全TaskとRequirementsを確認してください。

Specは単に`Implementation tasks`がすべてclosedなら完了ではありません。closeoutでは次も確認します。

- 全Taskがclosed
- Requirementsが成立
- 必要なautomated / reproducible verificationが存在し結果を確認済み
- 必要な実機verificationがSpecに含まれる場合、そのevidenceがある
- repository knowledge impactを再判定済み
- 必要なdurable knowledge更新が`main`へmerge済み
- docs / implementation / configuration / verificationに既知の矛盾がない

不足があればSpecをopenのまま維持し、必要なTaskを追加します。

## Verification

Verifierは使用しません。Taskごとに変更に対応するproject固有verificationを行います。詳細は`agent/VERIFICATION.md`を参照します。

firmware / release pathでは既存Hosted GitHub Actionsが基準です。必要に応じてPlatformIO build、filesystem、flash-layout validation、release-bundle validation、実機確認をAcceptance Criteriaへ明示します。

docs / planningだけの変更でfirmware workflowが起動しない場合、workflowを無理に起動するためproduct pathを変更しません。reviewと再現可能なvalidation evidenceをPRへ記録します。

## ユーザーが通常やらなくてよいこと

原則としてAgent側で行います。

- 手動でSpec / Task Issueを作る
- Taskごとのbranchを手動作成する
- `PROGRESS.md`のような進捗台帳を更新する
- merge済Taskを手動closeする
- PR bodyへ`Closes #...`を手作業で入れる
- verification結果を別文書へ転記する
- 正常完了iterationのhandoffを作る
- durable knowledgeを過去Issueから毎回探し直す

ユーザーは、何をしたいか、どこまで進めたいか、重要な意思決定に集中します。Skill名を覚える必要はありません。
