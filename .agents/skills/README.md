# Loop Engineering Skills

このdirectoryには、Cube Scrambler M5Stackで利用する再利用可能なAgent手順を置きます。

## Planning / tracking

- `wayfinder`: 大きく曖昧なworkをMap / Decisionへ分解する。
- `to-spec`: settled decisionsを`[Spec]` Issueへ固定する。
- `to-tickets`: Specを実装可能な`[Task]`へvertical sliceする。
- `loop-status`: current GitHub stateからready / blocked / closeout状態を整理する。
- `handoff`: 中断時だけIssue / PRへ再開checkpointを残す。

## Implementation / engineering

- `implement`: readyなTaskを1件だけbranch → code → verification → review → PR → mergeまで進める。
- `ponytail`: YAGNI / reuse / native-platform / root-causeの順で、要件を満たす最小の正しい実装を選ぶ。
- `code-review`: requirementsとengineering qualityを分けてreviewする。
- `codebase-design`: module boundary / interface / seamを設計する。
- `tdd`: observable behaviorをred → greenで実装する。
- `diagnosing-bugs`: reproducible feedback loopからroot causeを特定する。
- `resolving-merge-conflicts`: 両側の変更意図を保ったconflict解消を行う。

## Investigation

- `research`: authoritative sourceを使って技術的不確実性を解消する。
- `grilling`: materialなdesign decisionをstructured questioningで詰める。

## Repository-specific boundary

このrepositoryではLoop Verifier infrastructureを使用しません。Skill内のverificationは、Task / Specで定義されたchecks、既存GitHub Actions、必要な実機確認を意味します。

Third-party adaptation provenanceは`THIRD-PARTY-NOTICES.md`を参照してください。
