# Cube Scrambler for M5Stack

M5Stack上で **Web UI・Solver・Move変換・サーボ制御まで完結**する、PCレスのCube Scrambler firmwareです。

現時点で正式に実機検証済みの対象は **M5Stack NanoC6（ESP32-C6）** です。リポジトリ名は今後のM5Stackデバイス対応を見据えてdevice-neutralにしていますが、NanoC6以外はまだ正式サポート対象ではありません。

既存の [Cube Scrambler](https://github.com/miso-develop/cube-scrambler) でPC側が担当していた処理をNanoC6へ移植しており、通常利用時にNode.js server、PC、`opniz`、PC↔MCU WebSocketは不要です。

NanoC6の最終4 MiB Flash layout上で、**Power on → Random Scramble → camera Facelets scan → Solve → physical solve** のE2E実機検証を完了しています。

## Architecture

```text
Smartphone / Tablet / PC Browser
            ↓ HTTPS
       M5Stack NanoC6
       ├─ Static Web UI (SPIFFS)
       ├─ HTTPS API
       ├─ Min2Phase Solver
       ├─ Random / Step Generator
       ├─ MoveParser / MoveConverter
       ├─ Async MoveRunner
       └─ StandServo / ArmServo
```

HTTP port 80はアプリケーションAPIを提供せず、`/ca.crt` のCA証明書配布とHTTPSへの307 redirectのみを担当します。

## Implemented features

- Random Scramble
- FaceletsからのSolve
- Cube notation sequence実行
- STEP 2〜7生成・実行
- 実行状態取得 / cooperative Stop
- NanoC6物理ボタンによるStop
- StandServo / ArmServoの直接制御
- オンボードRGB LEDによるidle / busy表示
- Flash上のWeb UI
- カメラによるFacelets入力
- HTTPS / Secure Context対応
- Station / AP Wi-Fi mode
- USB SerialによるWi-Fi・Servo設定
- Web Serial配布installer
- PC/Serial Monitorなしでのstandalone boot

以下は意図的に移植していません。

- `CubeChampleApi` / `CubeChampleApiCache`
- `cornerOnly` / `edgeOnly` / `parity` / `nonParity`
- Node.js / Express runtime
- PC↔MCU通信用WebSocket
- `opniz`
- 既存CLI / REPL一式

## Hardware

現在の正式対象は **M5Stack NanoC6** です。

- SoC: ESP32-C6
- CPU: RISC-V single core / 160 MHz
- Flash: 4 MiB
- Wi-Fi
- StandServo: GPIO2
- ArmServo: GPIO1

## Flash layout

| Partition | Offset | Size | Purpose |
|---|---:|---:|---|
| nvs | `0x9000` | `0x5000` | NVS |
| otadata | `0xE000` | `0x2000` | OTA metadata |
| ota_0 | `0x10000` | `0x150000` | Firmware slot A |
| ota_1 | `0x160000` | `0x150000` | Firmware slot B |
| solver | `0x2B0000` | `0x110000` | Min2Phase tables |
| web | `0x3C0000` | `0x40000` | Web UI / TLS material |

代表的な実測値:

- Firmware image: 1,139,184 / 1,376,256 bytes
- Solver image: 1,004,832 / 1,114,112 bytes
- Web UI source assets: 153,542 bytes
- Firmware RAM: 47,424 / 327,680 bytes

Flash layoutは将来のapplication OTAを可能にするため2つのapp slotを持ちますが、現在はLAN上へFirmware upload APIを公開していません。通常の更新方法はUSB + PlatformIOです。

## Development environment

- PlatformIO Core `6.1.19`
- Arduino framework
- pioarduino / platform-espressif32 `55.03.37`
- Arduino-ESP32 `3.3.7`
- Python 3.12
- Node.js 18+

Windowsで初回セットアップ:

```cmd
scripts\setup-windows.cmd
```

新しいcmd sessionでは:

```cmd
call .venv\Scripts\activate.bat
set PLATFORMIO_CORE_DIR=%CD%\.platformio-core
```

詳細は [`docs/LOCAL_SETUP.md`](./docs/LOCAL_SETUP.md) を参照してください。

## Wi-Fi configuration for development builds

```cmd
copy include\wifi_credentials.example.h include\wifi_credentials.h
```

`include/wifi_credentials.h` を編集します。

```cpp
#define CUBE_WIFI_SSID "YOUR_SSID"
#define CUBE_WIFI_PASSWORD "YOUR_PASSWORD"
```

このファイルはGit管理対象外です。配布buildではdeveloper credentialを取り込まないよう `CUBE_DISTRIBUTION_BUILD=1` を使用します。

## Build and flash

Solver table、Web UI、HTTPS証明書、Firmwareを生成します。

```cmd
scripts\generate-solver-tables.cmd
scripts\generate-web-ui.cmd
scripts\generate-https-cert.cmd <NanoC6-IP>

pio run -e m5stack-nanoc6
pio run -e m5stack-nanoc6 -t buildfs
python tools\check_flash_layout.py
```

`FLASH LAYOUT CHECK: PASS` を確認後、Firmwareとdata partitionを書き込みます。

```cmd
pio run -e m5stack-nanoc6 -t upload
scripts\flash-solver-image.cmd .pio\min2phase-tables.bin COM3
scripts\flash-web-ui.cmd COM3
```

COM番号は環境に合わせて変更してください。

## Web Serial distribution bundle

配布用のcredential-free full-flash imageとWeb Serial installerは以下で生成できます。

```cmd
scripts\build-release-bundle.cmd dev
scripts\serve-web-installer.cmd
```

生成物は `.pio/release/` 配下へ出力されます。GitHub Actionsの `Release Bundle` workflowからも同じ配布bundleを生成できます。

詳細は [`docs/WEB_SERIAL_INSTALLER.md`](./docs/WEB_SERIAL_INSTALLER.md) を参照してください。

## HTTPS certificate

`generate-https-cert.cmd` はローカルCAとNanoC6用server certificateを生成します。生成後、ブラウザを使う端末へ次のいずれかを信頼済みRoot CAとしてインストールします。

```text
local/https/root-ca.crt
local/https/root-ca.der.crt
```

秘密鍵を含む `local/https/` はGit管理しません。

通常は以下から利用します。

```text
https://cube-scrambler.local/
```

または証明書生成時に指定したIPを使用します。

## HTTPS API

```text
GET  /api/status
GET  /api/solve?facelets=...
GET  /api/scramble?type=0
GET  /api/step?number=2..7
GET  /api/sequence?sequence=...
POST /api/sequence?sequence=...
POST /api/stop
```

物理動作を伴う要求はHTTP 202で受理し、Web UIは `/api/status` をpollして完了まで待ちます。実行中の競合sequence要求は拒否します。

## Verified on NanoC6

- Min2Phase representative cases: PASS
- Web server稼働中Solve: PASS
- D / D' / x / y / y': hardware PASS
- Async MoveRunner / busy rejection / Stop: PASS
- Random generator: PASS
- STEP 2〜7 generator: PASS
- Web UI / HTTPS API: PASS
- Secure Context / `getUserMedia()`: PASS
- Camera Facelets scanner: PASS
- HTTP→HTTPS redirect: PASS
- PC/Serial非依存boot: PASS
- Final E2E: **Power on → Random Scramble → camera scan → Solve → physically solved Cube: PASS**

## Documentation

- [`docs/REQUIREMENTS.md`](./docs/REQUIREMENTS.md): product requirements / constraints
- [`docs/BUILD.md`](./docs/BUILD.md): build / partition policy
- [`docs/LOCAL_SETUP.md`](./docs/LOCAL_SETUP.md): Windows setup
- [`docs/WEB_UI.md`](./docs/WEB_UI.md): Web UI / SPIFFS
- [`docs/WEB_SERIAL_INSTALLER.md`](./docs/WEB_SERIAL_INSTALLER.md): browser installer / distribution bundle
- [`docs/THIRD_PARTY.md`](./docs/THIRD_PARTY.md): third-party provenance / licensing

## Min2Phase

Solverは `cs0x7f/min2phase.js` の固定revisionを参照し、MIT optionを選択しています。

```text
revision: 0ba83a6177d816f72af1a45c9015349da597456a
Git blob: b8a641b03cea8e6d7a6b0f2a6da74ff98e03475e
```

大規模tableは起動時にRAMへ展開せず、hostで生成したbinaryを専用Flash partitionへ配置し、NanoC6からmemory mappingして参照します。詳細は [`docs/THIRD_PARTY.md`](./docs/THIRD_PARTY.md) を参照してください。
