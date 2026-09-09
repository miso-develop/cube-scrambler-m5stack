# Cube Scrambler M5Stack Project

このファイルはproject全体に適用する長寿命の目的・scope・制約・不変条件を保持します。featureごとの仕様、実装Task、進捗はGitHub Issues / Pull Requestsを正とし、詳細なNanoC6 baseline requirementは`docs/REQUIREMENTS.md`を参照します。

## Purpose

M5Stackデバイス上で単体動作するCube Scrambler firmwareを提供し、通常利用時にPC側runtimeへ依存せず、Web UI、Cube logic / solver、move conversion、Servo controlまでdevice内で完結させる。

基本構成は次とする。

```text
Smartphone / Tablet / PC Browser
            ↓ Wi-Fi
        M5Stack device
        ├─ Web UI / HTTP API
        ├─ Cube logic / Solver
        └─ Servo / hardware control
```

現在の正式なreference deviceはM5Stack NanoC6。複数M5Stackデバイスへの対応拡張をproject scopeに含む。

## Scope

### In scope

- Cube Scramblerのfirmwareと共通domain logic。
- random scramble、3x3 solve、facelets validation、step generation。
- MoveParser / MoveConverter / MoveRunner相当のhardware-independent logic。
- servo / button / LED等のhardware adaptationと安全な停止・calibration。
- Wi-Fi station / AP mode、Web UI、HTTP API、配布後に変更可能なcredential設定経路。
- device別PlatformIO build / flash layout / Web Serial manifest / release bundle。
- Flash上のWeb assets / solver tableと、各deviceのresource制約内での安定動作。
- NanoC6 baselineの主要挙動・操作性をregressionさせずにM5Stack deviceを追加すること。
- public GitHub repository上のCI / release / Loop Engineering運用。

### Out of scope

- 通常利用時のNode.js server、`opniz`、PC側device abstractionへの依存。
- PCとM5Stack device間のWebSocket control pathの再導入。
- legacy `CubeChampleApi` / `CubeChampleApiCache`や`cornerOnly` / `edgeOnly` / `parity` / `nonParity`機能の復活（明示Specで再要求されない限り）。
- production用途のPC CLI / REPL。diagnostic USB Serial commandは許可する。
- 1つのuniversal firmware binaryを全deviceでruntime自動判定して動かすこと（明示Specで変更しない限り）。
- Loop Verifier、Shared Local Verifier、Cloud Run verifier等のVerifier infrastructure。

## Constraints

- repositoryはpublicであり、公開可能な情報だけをtracked file / Issue / PR / handoffへ記録する。
- `include/wifi_credentials.h`、private key、token、authorization header等をcommitしない。
- reference deviceのNanoC6は4 MB Flash、約512 KB SRAMを前提とする。追加deviceでは各deviceの実容量・partition・runtime memoryを個別に確認し、NanoC6の値を暗黙に流用しない。
- firmwareは対象ESP32 familyでnative実行可能なC/C++を基本とし、現在のbuild/toolingはPlatformIOを使用する。
- Web UI全体やsolverのlarge read-only tableをSRAMへ全展開しない。Flash resident / filesystem / dedicated partition / mapping / streamingを優先する。
- Servo sequence、solver、HTTP処理はwatchdog reset、heap exhaustion、Web server全体の長時間blockingを起こさない設計とする。
- Browser controlはHTTP APIを基本とし、競合sequenceの二重実行を防止する。
- cameraによるfacelets入力を維持する場合は、対象browserのSecure Context要件を満たす配信方式を成立させる。
- GPL-only等のthird-party solver sourceをlicense確認なしに取り込まない。利用元、revision/version、licenseを追跡可能にする。
- optimizationやdevice capability判断は推測ではなくmeasurementを根拠にする。
- GitHub Actionsはpublic repository向けHosted CIを基本とし、不要な高負荷runを避ける。documentation-only changeではfirmware buildを無理に起動しない。
- third-party Actionsは可能な限りcommit SHAでpinし、最小permissions、`persist-credentials: false`等のtrust boundaryを維持する。
- 現在のNanoC6 build / flash layout / release pathをreference baselineとしてregressionさせない。
- `.cmd`は`.gitattributes`に従ってcommitted blobも含めCRLFを維持する。Windows command entrypointは`.cmd`を使用し、新しい`.bat`を導入しない。

## Device / release profile contract

`config/devices/<device-id>.json`を、host-side build / flash validation / distribution toolingが参照するdevice/release contractとする。

profileには少なくとも次を明示する。

- stable device id / display name / release name
- ESP Web Toolsの`chipFamily`
- Flash総容量
- PlatformIO release environment名
- partition CSV
- full-flash image名
- bootloader / partition table / boot_app0 / firmware / solver / Web imageのoffset、exclusive limit、bundle内filename

`tools/check_flash_layout.py`と`tools/build_release_bundle.py`は`--device`または`--profile`による明示選択を要求し、NanoC6を暗黙defaultとして適用しない。profileとpartition CSVまたは実image容量が矛盾する場合はpublishable bundle生成前にfail closedとする。

このprofileはhost-sideのbuild/release contractであり、GPIO、Servo pin、button active level、LED、device library等のruntime hardware adaptationを表すものではない。runtime abstractionは具体的な2台目deviceの実差分を確認したうえで`[Decision]` / `[Spec]`により決定し、将来deviceを想像したHALを先行導入しない。

profileやPlatformIO targetを追加してbuildが成功しただけでは、そのdeviceをsupportedとは扱わない。supported deviceとする前に実機で少なくともboot、credential-free first boot / provisioning、Wi-Fi STA/AP、Web UI/API、Stand/Arm Servo、利用可能なphysical stop、solver initialization / solve / scramble、resource measurement、Web Serial install / distribution pathを確認する。

## Invariants / decisions

- 通常利用はM5Stack device内部でWeb UI、Cube logic、solver、move conversion、Servo controlまで完結する。
- StandServo / ArmServoはdeviceから直接制御し、`opniz`を使用しない。
- physical stopはWeb UI / Serialが利用できなくても機能し、少なくともrobot move境界でcooperativeに停止してArmを安全なready位置へ戻す。
- large solver tableはSRAM常駐を前提としない。
- standalone operationを損なうPC runtime dependencyや外部scramble / solver APIを再導入しない。
- current reference deviceはNanoC6。実装容易性だけを理由にNanoC6 supportを落とさず、support変更には実測結果と明示Spec / user decisionを必要とする。
- multi-device architectureの詳細は`[Map]` / `[Decision]` / `[Spec]`で決定し、未確定のfuture device向け抽象化を先回りして増やさない。
- project-wide requirementを実装都合だけで黙って変更しない。変更が必要な場合はGitHub `[Decision]` / `[Spec]`またはユーザーの明示指示で合意を残す。
- GitHub Issues / Pull RequestsをLoop Engineeringの進捗source of truthとし、別の進捗台帳を二重管理しない。
- production codeの実装は原則としてopenな`[Task]` Issueから行う。
- Loop Engineeringを使用するが、Verifier mechanismはこのrepositoryへ導入しない。required verificationはproject固有のtest / build / GitHub Actions /必要な実機確認で定義する。

## References

- `README.md` — user-facing usage / build overview。
- `docs/REQUIREMENTS.md` — NanoC6 baselineの詳細product requirements。
- `docs/BUILD.md` — firmware build手順。
- `docs/THIRD_PARTY.md` — third-party provenance / licensing。
- `docs/WEB_SERIAL_INSTALLER.md` — Web Serial distribution。
- `AGENTS.md` — development lifecycle / cross-cutting constraints。
- `agent/CUBE-ENGINEERING.md` — Cube固有のconditional engineering rules。
- `agent/VERIFICATION.md` — CI / Artifact /実機verification policy。
- `agent/WORK-TRACKING.md` — GitHub Issue-based work lifecycle。
