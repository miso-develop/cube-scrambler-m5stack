# Cube Scrambler engineering rules

Cube Scrambler固有の実装時だけ参照する補助ルールです。project-wide purpose / invariantsは`PROJECT.md`、NanoC6 baselineの詳細product requirementは`docs/REQUIREMENTS.md`、現在のworkはGitHub `[Spec]` / `[Task]` Issuesを正とします。

## Hardware priority

- 現在のreference deviceはM5Stack NanoC6（ESP32-C6、4 MB Flash、約512 KB SRAM）。
- NanoC6の既存安定動作とbuild / release baselineをregressionさせない。
- 新しいM5Stack deviceの対応可否は、datasheetだけでなく実build・boot・resource・Servo・Web・solverのmeasurementで判断する。
- 実装容易性や推測だけを理由に既存supportを落とさない。support変更は明示Spec / user decisionで行う。

## Architecture

目標責務は概ね次のとおり。

```text
Browser
  ↓ HTTP/HTTPS
M5Stack device
  ├─ WebServer
  │   ├─ Static assets
  │   └─ HTTP API
  ├─ CubeService
  │   ├─ Solver
  │   └─ Step generator
  ├─ MoveManager
  │   ├─ Parser
  │   ├─ Converter
  │   └─ Runner
  ├─ CubeRobot
  │   ├─ StandServo
  │   └─ ArmServo
  ├─ Hardware adaptation
  │   ├─ GPIO / button / LED
  │   └─ device-specific capabilities
  └─ DebugSerial (optional)
```

legacy PC実装のController / Service / Factory階層を機械的に再現しない。マイコン上で責務が明確なら小さい構造を優先する。

multi-device化では、Move / Robot semantics / Sequence / Solver / Web API等の共通domainへdevice名やGPIO差分を拡散させず、実在するvariationだけをhardware/build/flash boundaryへ閉じ込めることを優先する。ただし将来のdeviceを想像した抽象化は作らず、2つ以上の実在実装または明確な隔離圧力が出た時点で`codebase-design` / `ponytail`を使って判断する。

## Do not reintroduce by default

- `CubeChampleApi`
- `CubeChampleApiCache`
- `cornerOnly`
- `edgeOnly`
- `parity`
- `nonParity`
- PCとdevice間のWebSocket control path
- `opniz`
- PC側Device abstraction
- production用途のCLI / REPL

Debug Serialは許可するが、Web APIと同じcore serviceを呼ぶ薄いadapterにする。

## RAM / Flash

resource制約を各deviceの設計条件として扱う。

- Web UI全体をRAMへ展開しない。
- 静的Web資産はFlash filesystemまたは適切なFlash領域から配信する。
- 大きなHTTP responseはstreaming / chunked responseを優先する。
- Min2Phase系Move / Pruning table等のlarge read-only dataはRAM常駐を前提にしない。
- large fixed tableはFlash resident、dedicated partition、memory mapping等を優先する。
- 不要な`String`連結、一時buffer、大規模dynamic allocationを避ける。
- partition offset、full-flash image、solver/web領域はdevice profileごとに検証し、NanoC6の固定値を別deviceへ流用しない。
- optimizationは推測ではなくmeasurementを根拠にする。

## Solver

solver変更では少なくとも次を観測可能にする。

- initialization成功
- solved state validation
- known faceletsのsolve
- random cube / scramble生成
- representative solve duration
- free heap / minimum free heap
- firmware / Flash使用量

遅い場合はFlash access、table placement、initialization、search settingを計測してから最適化する。

GPL-only implementationをlicense確認なしにcopyしない。third-party solverのsource / algorithmを利用する場合は`docs/THIRD_PARTY.md`等でprovenance、revision/version、licenseを追跡可能にする。

## Web UI / HTTP API

- Browserとdevice間はHTTP APIを基本とし、PC-device WebSocket control pathを再導入しない。
- Servo sequenceやsolver実行でHTTP server全体を長時間blockしない。
- 必要ならrequest acceptanceとstatus pollingを分離する。
- 競合するrobot sequenceの二重実行を防止する。
- camera facelets inputを提供する場合は`getUserMedia()`のSecure Context要件を満たす。HTTPのみのPoCをcamera feature完成扱いにしない。
- 既存NanoC6 Web UIの主要操作性をreference behaviorとして扱い、device追加のためだけに不要なUI差分を作らない。

## Servo and stop

- StandServo / ArmServoをM5Stack deviceから直接PWM制御する。
- 既存のhold / release、D / D'、x / y rotation、calibration、必要delayの意味を維持する。
- Servo中の長時間busy waitでWi-Fi / Web serverを止めない。
- physical stopはWeb UI / Serialがなくても動作し、最低限robot move境界でcooperative stopしてArmを安全なready位置へ戻す。
- calibration / timing値を変更可能にする場合は範囲検証と永続化を行い、破損値は安全側へ戻す。

## Debug Serial

必要に応じて`solve`、`scramble`、`step`、`run`、`servo`、`status`、`heap`等のdiagnostic commandを追加してよい。

- Serial専用business logicを重複実装しない。
- production runtimeに不要ならcompile-timeで無効化可能にする。
- credentialやsecretをlogしない。

## Verification and measurements

hardware-independentなMoveParser / MoveConverter / cube-state logicはhost-side automated test可能な構造を優先する。

実機検証を行うwork itemでは必要に応じてIssue / PR verification evidenceへ次を記録する。

- device / board revision
- firmware size
- filesystem / solver table size
- free heap after boot
- minimum free heap
- solver initialization time
- representative solve times
- HTTP server稼働中のsolve結果
- Servo実行中のWeb responsiveness
- physical stop behavior

新deviceをsupported扱いにする場合は、build成功だけでなく実機boot、Wi-Fi / Web UI、Servo、stop、solver、distribution/install pathをAcceptance Criteriaで確認する。

独立した`PROGRESS.md`は更新しない。現在の進捗・measurement・known issueは対象Issue / PR / handoffへ記録する。

## Work granularity

- 大規模移植や全device一括対応を1 Taskで行わない。
- 高risk部分は小さいPoC / Decisionで先に検証する。
- task外のrefactorを混ぜない。
- readyなGitHub `[Task]` とblocker graphを優先する。
