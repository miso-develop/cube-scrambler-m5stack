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

GitHub Issue / Pull Requestが進捗のsource of truthです。通常完了したiterationについて別の進捗ファイルを更新しません。

## Work item

- `[Map]`: 大きく曖昧なworkのdecision map
- `[Decision]`: Map配下の1つの判断事項
- `[Spec]`: 実装前に確定したfeature仕様
- `[Task]`: production codeを変更する実装単位

## 1. 新しい要件・機能

小さく内容がほぼ決まっている場合:

> この要件をSpec化し、Taskへ分解して実装まで進めてください。

大きい・曖昧・複数の設計判断がある場合:

> まずwayfinderで判断事項と依存関係を整理してください。

ユーザーが事前にIssueを作る必要はありません。

## 2. 要件だけ整理して実装しない

> この内容をSpec化してください。実装はまだ開始しないでください。

- 何を作るかがほぼ決まっている → `to-spec`
- 何を作るべきか、どう設計すべきかに未決定事項が多い → `wayfinder`

## 3. 既存要件を変更する

openなSpecの未実装部分ならSpec / Taskを必要に応じて更新します。

既にclose済みのSpecやproductionへmerge済みの要件を変更する場合は、過去Specを履歴として保持し、変更差分を新しい`[Spec]`として作成するのを基本とします。

project全体に恒久的に効く制約・不変条件が変わる場合だけ`PROJECT.md`も更新します。

## 4. 実装だけ進める / 次Taskを任せる

対象TaskをIssue URLまたは`owner/repo#number`で指定するのが最も確実です。

> owner/repo#42 を実装してください。

次のready Task選択を任せる場合:

> 現在readyなTaskから次の1件を選んで実装してください。

1 implementation iterationで扱うTaskは1件です。

## 5. 大きいSpecをTaskへ分割する

> [Spec] #30 を実装可能なTaskへ分解してください。まだ実装はしないでください。

`to-tickets`はvertical sliceを基本とし、作成TaskはSpecの`Implementation tasks`へ記録します。

## 6. 設計・技術調査

短い設計判断のpressure test:

> このAPI設計をgrillingしてください。

複数sessionにまたがるdecision map:

> このmulti-device設計をwayfinderで整理してください。

一次情報を使う技術調査:

> このdevice / libraryの仕様と互換性をresearchしてください。まだ実装は不要です。

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

Agentはactive IssueまたはPRへcheckpointを残します。正常merge済みworkにはhandoff不要です。

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

Specは`Implementation tasks`がすべてclosedでRequirementsが満たされている場合にcloseできます。

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

ユーザーは、何をしたいか、どこまで進めたいか、重要な意思決定に集中します。Skill名を覚える必要はありません。
