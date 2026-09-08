# Product Requirements

## 1. Purpose

既存の `miso-develop/cube-scrambler` でPC側が担当していた処理をM5Stack上へ移植し、**通常利用時にPCを必要としないCube Scrambler** を実現する。

現在の正式対象はM5Stack NanoC6である。

```text
Smartphone / Tablet / PC Browser
            ↓ Wi-Fi
       M5Stack NanoC6
       ├─ Web UI
       ├─ Cube logic / Solver
       └─ 2 Servo control
```

PCは開発・書込み・デバッグには使用してよいが、通常利用時には不要であること。

## 2. Hardware

### REQ-HW-001: Current supported target

正式に実機検証済みの対象を **M5Stack NanoC6（ESP32-C6 / 4 MiB Flash）** とする。

### REQ-HW-002: Additional M5Stack devices

他のM5Stackデバイスを追加する場合、board-specific GPIO、button / LED、power management、Flash layout、USB modeをdevice profileとして分離し、Cube logic / Solver / Move / Webの共通実装を不必要に分岐させないこと。

正式サポートを名乗るdeviceは実機E2Eを完了すること。

### REQ-HW-003: Servo

StandServo / ArmServoの2系統をMCUから直接制御する。Servoへの給電条件は対象deviceごとに確認し、Grove等の5 V出力能力を超える構成を前提としないこと。

### REQ-HW-004: Physical stop

オンボードbuttonを利用できるdeviceではRobot sequenceの物理Stopを提供する。

- Web UI / Serialが利用できなくても機能すること。
- チャタリングと多重発火を避けること。
- 少なくともrobot move境界でcooperativeに停止すること。
- 停止後はArmをready位置へ戻すこと。

## 3. Platform and build

### REQ-PLATFORM-001

FirmwareはESP32系SoC上でnative実行できるC/C++を基本とし、PlatformIO + Arduino frameworkを使用する。

### REQ-PLATFORM-002: Reproducible build

Toolchain、platform release、board definition、partition layoutをrepositoryで追跡可能にする。

### REQ-PLATFORM-003: CI

GitHub Actionsで少なくとも以下を検証する。

- Web installer / Python tool syntax
- Min2Phase table generation
- Web UI generation
- credential-free release firmware compile/link
- SPIFFS image build
- Flash layout / capacity
- release bundle generation

実機依存項目はGitHub-hosted runnerでは検証対象外とし、正式サポートdeviceのrelease判断時に実機で確認する。

## 4. Standalone operation

- 通常利用時にNode.js serverを必要としないこと。
- 通常利用時に `opniz` を必要としないこと。
- MCU自身がWeb UIとHTTP/HTTPS APIを提供すること。
- Cube処理からServo制御までdevice内部で完結すること。
- PC↔MCU制御用WebSocketを必要としないこと。

## 5. Network and provisioning

### REQ-NET-001

Station modeで同一network上のbrowserから利用できること。

### REQ-NET-002

Access Point modeを提供し、外部routerなしでも設定・利用経路を確保すること。

### REQ-NET-003

Station / AP modeとstation credentialを再起動後も維持できること。

### REQ-NET-004

配布imageへ利用者固有SSID/passwordを固定しないこと。Web SerialまたはUSB Serial等、配布後に設定可能な経路を提供すること。

### REQ-NET-005

配布buildはignoredなdeveloper用 `include/wifi_credentials.h` を取り込まないこと。

## 6. Web UI and HTTPS

- HTML / CSS / JavaScript / image等はFlashから配信すること。
- Web assets全体をSRAMへ展開しないこと。
- カメラFacelets入力を提供すること。
- `getUserMedia()` のSecure Context要件を満たすHTTPS配信を提供すること。
- HTTPはCA配布とHTTPS redirectのみに限定してよい。
- Solver / Servo実行中もwatchdog resetやheap exhaustionを起こさないこと。

## 7. API

少なくとも以下を提供する。

```text
GET  /api/status
GET  /api/solve?facelets=...
GET  /api/scramble?type=0
GET  /api/step?number=2..7
GET  /api/sequence?sequence=...
POST /api/sequence?sequence=...
POST /api/stop
```

物理動作を伴う処理はWeb server全体を長時間blockしないこと。実行中の競合sequenceを二重実行しないこと。不正なfacelets / sequence / step numberは異常終了させずエラーとして扱うこと。

## 8. Solver and sequence generation

- 54 faceletsで表される有効な3x3 Cube状態からSolve sequenceを生成できること。
- 外部APIなしでrandom scrambleを生成できること。
- 不可能なCube状態を検出できること。
- STEP 2〜7向け状態を生成できること。
- `CubeChampleApi` / `CubeChampleApiCache` に依存しないこと。
- `cornerOnly` / `edgeOnly` / `parity` / `nonParity` は対象外とする。
- 大規模Move / Pruning tableをSRAMへ全展開せず、Flash resident / partition / mmapを使用すること。
- Third-party solverの由来とlicenseを追跡可能にすること。

## 9. Move processing

- `MoveParser` 相当のnotation parserを提供すること。
- 一般的なCube notationをrobot moveへ変換する `MoveConverter` を提供すること。
- 変換済みrobot moveを順に実行する `MoveRunner` を提供すること。
- Move変換ロジックはhardware-independentを維持すること。

## 10. Servo control

- `opniz`を使用せず直接PWM制御すること。
- hold / release / D / D' / x / yを扱うこと。
- calibration角度とsleep値をUSB Serialから確認・変更できること。
- 設定値は範囲検証すること。
- calibration / timingはNVS等へ永続化し、破損・不整合時はsafe defaultへfallbackすること。

## 11. Debug Serial

開発・recoverabilityのためUSB Serialから主要機能を呼べること。Serial専用にSolver / Move / Servoロジックを重複実装しないこと。

代表command:

```text
solve <facelets>
scramble
step 3
run R U R' U'
status
heap
wifi-set <ssid>|<password>
wifi-mode sta|ap
reboot
```

## 12. Resource constraints

- Web UIやsolver tableをSRAMへ全展開しないこと。
- Out Of Memory、stack overflow、watchdog resetを通常操作で発生させないこと。
- Firmware / Solver / Web imageがpartition容量内に収まることをbuild時に検証すること。
- 代表Solveは実用的な時間内に完了すること。

## 13. Flash layout

NanoC6では以下の4 MiB layoutを使用する。

| Partition | Offset | Size |
|---|---:|---:|
| nvs | `0x9000` | `0x5000` |
| otadata | `0xE000` | `0x2000` |
| ota_0 | `0x10000` | `0x150000` |
| ota_1 | `0x160000` | `0x150000` |
| solver | `0x2B0000` | `0x110000` |
| web | `0x3C0000` | `0x40000` |

Application OTA用slotは確保するが、現在のMVPではLAN上へFirmware upload APIを公開しない。

## 14. Distribution

### REQ-DIST-001

一般利用者がsource buildなしで書き込めるcredential-free full-flash imageを生成可能にすること。

### REQ-DIST-002

ESP Web Toolsを利用したWeb Serial installerを生成可能にすること。

### REQ-DIST-003

配布bundleにFirmware、partition table、Solver table、Web UI / TLS materialを含め、再現可能なrelease手順をrepositoryへ保持すること。

### REQ-DIST-004

Wi-Fi credential等の利用者固有値を配布imageへ埋め込まないこと。

## 15. Current NanoC6 acceptance criteria

NanoC6について少なくとも以下を満たすこと。

1. standalone bootできる。
2. Station / AP modeでbrowser accessできる。
3. Web UIを表示できる。
4. external APIなしでrandom scrambleを生成できる。
5. FaceletsからSolveできる。
6. STEP 2〜7を生成できる。
7. Cube notationをrobot moveへ変換できる。
8. 2 Servoで実際にCubeへsequenceを適用できる。
9. 実行中のStopが機能する。
10. Solver / Servo動作中にWeb serverが破綻しない。
11. Camera Facelets入力がSecure Context上で動作する。
12. Web Serial配布bundleを生成できる。
13. Power on → Random Scramble → camera scan → Solve → physically solved Cube のE2Eが完了する。
