# Loop Engineering 運用ガイド

このrepositoryではGitHub Issues / Pull Requestsを中心にLoop Engineeringを運用します。Verifier infrastructureは使用しません。

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

詳細は`agent/WORK-TRACKING.md`を参照します。

## Typical requests

### 小さく明確なfeature

> この要件をSpec化し、Taskへ分解して実装まで進めてください。

### 大きい / 曖昧な変更

> まずwayfinderで判断事項と依存関係を整理してください。

### Specだけ作る

> この内容をSpec化してください。実装はまだ開始しないでください。

### readyな次Taskを進める

> 現在readyなTaskから次の1件を選んで実装してください。

### 状態確認

> openなMap / Decision / Spec / Taskと進行中PRを整理してください。

### 中断

> ここで止めます。次回再開できるようhandoffを残してください。

## Verification

Verifierは使用しません。Taskごとに、変更に対応するproject固有verificationを行います。

firmware / release pathでは既存のHosted GitHub Actionsが基準です。必要に応じてPlatformIO build、SPIFFS build、flash-layout validation、release-bundle validation、実機確認をAcceptance Criteriaへ明示します。

docs / planningだけの変更でfirmware workflowが起動しない場合、workflowを無理に起動するためproduct pathを変更しません。reviewと再現可能なvalidation evidenceをPRへ記録します。

## User responsibility

ユーザーはSkill名を覚える必要はありません。目的、どこまで進めたいか、重要な意思決定だけを自然文で伝えれば、Agentが適切なwork item / Skillへroutingします。
