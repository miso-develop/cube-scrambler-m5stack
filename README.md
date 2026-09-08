# Cube Scrambler NanoC6

既存の [Cube Scrambler](https://github.com/miso-develop/cube-scrambler) でPC側が担当していた処理を M5Stack NanoC6（ESP32-C6）へ移植し、**NanoC6単体でWeb UI・Solver・Move変換・サーボ制御まで完結**させた実装です。

通常利用時にNode.jsサーバー、PC、`opniz`、PC↔MCU WebSocketは不要です。

**最終4 MiB Flash layout上で、PCなしの電源投入から Random Scramble → カメラFacelets scan → Solve → Cubeが物理的に完成状態へ戻るまでのE2E実機検証を完了しています。**

## 構成

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

HTTP port 80はアプリケーションAPIを提供せず、以下だけを担当します。

- `/ca.crt`: ローカルCA証明書の配布
- その他: HTTPSへの307 Redirect

## 実装済み機能

- Random Scramble
- FaceletsからのSolve
- Cube notationのSequence実行
- STEP 2〜7生成・実行
- 実行状態取得 / cooperative Stop
- StandServo / ArmServoの直接制御
- 既存Web UIのFlash配信
- カメラによるFacelets入力
- HTTPS / Secure Context対応
- USB Serial診断コマンド
- PC/Serial Monitorなしでのstandalone起動

以下は意図的に移植していません。

- `CubeChampleApi` / `CubeChampleApiCache`
- `cornerOnly` / `edgeOnly` / `parity` / `nonParity`
- Node.js / Express runtime
- PC↔MCU通信用WebSocket
- `opniz`
- 既存CLI / REPL一式

## ハードウェア

第一対象は **M5Stack NanoC6** です。

- SoC: ESP32-C6
- CPU: RISC-V single core / 160 MHz
- Flash: 4 MiB
- Wi-Fi
- StandServo: GPIO2
- ArmServo: GPIO1

NanoC6実機でSolver、HTTPS、カメラ、全Robot primitive、Random/Step、非同期Sequence実行、最終E2Eまで確認済みで、ESP32-S3へ移行する必要は確認されていません。

## Final Flash layout

| Partition | Offset | Size | 用途 |
|---|---:|---:|---|
| nvs | `0x9000` | `0x5000` | NVS |
| otadata | `0xE000` | `0x2000` | OTA metadata |
| ota_0 | `0x10000` | `0x150000` | Firmware slot A |
| ota_1 | `0x160000` | `0x150000` | Firmware slot B |
| solver | `0x2B0000` | `0x110000` | Min2Phase tables |
| web | `0x3C0000` | `0x40000` | Web UI / TLS material |

現在の代表値:

- Firmware image: 1,139,184 / 1,376,256 bytes
- Solver image: 1,004,832 / 1,114,112 bytes
- Web UI source assets: 153,542 bytes
- Firmware RAM: 47,424 / 327,680 bytes

Flash layoutは将来のapplication OTAを可能にするため2つのapp slotを持ちますが、**現在のMVPではネットワーク経由のFirmware upload APIは公開していません**。通常の更新方法はUSB + PlatformIOです。

## 開発環境

- PlatformIO Core `6.1.19`
- Arduino framework
- pioarduino / platform-espressif32 `55.03.37`
- Arduino-ESP32 `3.3.7`
- Python 3.12

Windowsで初回セットアップ:

```cmd
scripts\setup-windows.cmd
```

新しいcmd sessionでは:

```cmd
call .venv\Scripts\activate.bat
set PLATFORMIO_CORE_DIR=%CD%\.platformio-core
```

## Wi-Fi設定

```cmd
copy include\wifi_credentials.example.h include\wifi_credentials.h
```

`include/wifi_credentials.h` を編集します。

```cpp
#define CUBE_WIFI_SSID "YOUR_SSID"
#define CUBE_WIFI_PASSWORD "YOUR_PASSWORD"
```

このファイルはGit管理対象外です。

## 初回書込み

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

Windows/NanoC6環境ではPlatformIOの`uploadfs`が転送途中で不安定になることがあったため、Web UIは`flash-web-ui.cmd`からPyPI版`esptool`で書き込む方法を正式手順としています。

## HTTPS証明書

`generate-https-cert.cmd` はローカルCAとNanoC6用server certificateを生成します。

生成後、ブラウザを使う端末へ次のいずれかを信頼済みRoot CAとしてインストールします。

```text
local/https/root-ca.crt
local/https/root-ca.der.crt
```

秘密鍵を含む`local/https/`はGit管理しません。

CA導入後は通常、以下から利用します。

```text
https://cube-scrambler.local/
```

または証明書生成時に指定したIP:

```text
https://<NanoC6-IP>/
```

## 通常のFirmware更新

partition配置、Solver table、Web UI、TLS materialを変更していない場合、Firmwareだけ更新できます。

```cmd
pio run -e m5stack-nanoc6 -t upload
```

Solver/Webを変更した場合だけ、それぞれ専用スクリプトで再書込みします。

## HTTPS API

アプリケーションAPIはHTTPS側だけで提供します。

```text
GET  /api/status
GET  /api/solve?facelets=...
GET  /api/scramble?type=0
GET  /api/step?number=2..7
GET  /api/sequence?sequence=...
POST /api/sequence?sequence=...
POST /api/stop
```

物理動作を伴う要求はHTTP 202で受理し、Web UIは`/api/status`をpollして`finished`まで待ちます。実行中の重複要求は拒否します。

## 実機検証結果

主要な結果は以下です。

- Min2Phase代表5ケース: 5/5 PASS
- Web server稼働中Solve: 8 / 205 / 676 ms min/avg/max
- D / D' / x / y / y': 全てhardware PASS
- Async MoveRunner / busy rejection / Stop: PASS
- Random generator: PASS
- STEP 2〜7 generator: 全てPASS
- Web UI / HTTPS API: PASS
- Secure Context / `getUserMedia()`: PASS
- カメラFacelets scanner: PASS
- HTTP→HTTPS redirect: PASS
- PC/Serial非依存boot: PASS
- Final Flash layout relocation後のUI / physical Sequence: PASS
- Final E2E: **Power on → Random Scramble → camera scan → Solve → physically solved Cube: PASS**

詳細値とテスト履歴は [`PROGRESS.md`](./PROGRESS.md) を参照してください。

## ドキュメント

- [`REQUIREMENTS.md`](./REQUIREMENTS.md): 要件・制約・受け入れ条件
- [`AGENTS.md`](./AGENTS.md): AIエージェント向け実装ルール
- [`PROGRESS.md`](./PROGRESS.md): 実機結果・進捗
- [`docs/BUILD.md`](./docs/BUILD.md): Build / partition / OTA方針
- [`docs/LOCAL_SETUP.md`](./docs/LOCAL_SETUP.md): Windows環境構築
- [`docs/WEB_UI.md`](./docs/WEB_UI.md): Web UI / SPIFFS
- [`docs/THIRD_PARTY.md`](./docs/THIRD_PARTY.md): Min2Phase provenance / license

## Min2Phase

Solverは `cs0x7f/min2phase.js` の固定revisionを参照し、MIT optionを選択しています。

```text
revision: 0ba83a6177d816f72af1a45c9015349da597456a
Git blob: b8a641b03cea8e6d7a6b0f2a6da74ff98e03475e
```

大規模tableは起動時にRAMへ展開せず、hostで生成したbinaryを専用Flash partitionへ配置し、NanoC6からmemory mappingして参照します。

詳細は [`docs/THIRD_PARTY.md`](./docs/THIRD_PARTY.md) を参照してください。
