# Loop Engineering Skills

このdirectoryにはCube Scrambler M5Stackで利用する再利用可能なAgent手順を置きます。すべてを毎iteration読み込まず、各`SKILL.md`の`description`に該当する作業でのみ使用します。ユーザー指示、`AGENTS.md`、`PROJECT.md`、current GitHub work itemをSkillより優先します。

## Planning / tracking

- `wayfinder`: 大きく曖昧なworkをMap / Decisionへ分解する。
- `to-spec`: settled decisionsを`[Spec]` Issueへ固定する。
- `to-tickets`: Specを実装可能な`[Task]`へvertical sliceする。
- `loop-status`: current GitHub stateからready / blocked / closeout状態を整理する。
- `handoff`: 中断時だけIssue / PRへ再開checkpointを残す。

## Implementation / engineering

- `implement`: readyなTaskを1件だけbranch → code → verification → review → PR → mergeまで進める。
- `ponytail`: YAGNI / reuse / native-platform / root-causeの順で要件を満たす最小の正しい実装を選ぶ。
- `code-review`: requirementsとengineering qualityを分けてreviewする。
- `codebase-design`: module boundary / interface / seamを設計する。
- `tdd`: observable behaviorをred → greenで実装する。
- `diagnosing-bugs`: reproducible feedback loopからroot causeを特定する。
- `resolving-merge-conflicts`: 両側の変更意図を保ったconflict解消を行う。
- `windows-cmd-scoop`: Windows`cmd.exe` / `.cmd`とScoop CLI境界のquoting、`call`、exit code、shim、CRLFを安全に扱う。
- `troubleshooting-cases`: 実際に解決した再利用価値のあるfailureをcase libraryとして残す。

## Investigation / Skill maintenance

- `research`: authoritative sourceを使って技術的不確実性を解消する。
- `grilling`: materialなdesign decisionをstructured questioningで詰める。
- `skill-authoring`: Skillの責務、trigger、context load、評価方法を設計する。
- `skill-security-review`: third-party Skill / prompt packageを採用前に静的security / supply-chain reviewする。

## Main flow

```text
PROJECT.md
   |
   +-- small / clear ---------------------------+
   |                                            v
   +-- large / ambiguous -> wayfinder -> to-spec -> to-tickets
                                                       |
                                                       v
                                                ready [Task]
                                                       |
                                                   implement
                                                       |
                                         tdd / verification / review
                                                       |
                                                      PR
                                                       |
                                                     merge

blocked / interrupted -> handoff
status / next frontier -> loop-status
unexpected failure -> diagnosing-bugs -> troubleshooting-cases (when reusable)
```

`wayfinder` / `to-spec` / `to-tickets`はplanning専用でproduction implementationを行いません。`loop-status`はread-onlyです。

`ponytail`は常時適用ではありません。over-engineeringや不要dependencyが実際の判断点になる場合だけ使用し、Task acceptance、project verification、security、TDDより優先しません。

`windows-cmd-scoop`はWindows batch / Scoop固有の実装・診断時だけ使用します。一般application designへ持ち込みません。

Skill自体を追加・変更する場合は`skill-authoring`を使用し、third-party Skillを取り込む場合はその前に`skill-security-review`を使用します。

## State / boundary

- project-wide state: `PROJECT.md`
- planning / implementation state: GitHub `[Map]` / `[Decision]` / `[Spec]` / `[Task]`
- delivery state: branch / PR / project verification / commit history
- interrupted state: `handoff` comment
- aggregated current view: `loop-status`

`AGENTS.md`はalways-applicable process、`agent/`はconditional operational policy、`.agents/skills/`はtriggerable procedureです。別の`PROGRESS.md`やTask一覧ファイルは標準では持ちません。

このrepositoryではLoop Verifier infrastructureを使用しません。Skill内のverificationはTask / Specで定義されたchecks、既存GitHub Actions、必要な実機確認を意味します。

Third-party adaptation provenanceは`THIRD-PARTY-NOTICES.md`を参照してください。
