# ビルド環境

## 採用構成

M5Stack NanoC6向けFirmwareの開発環境として以下を採用する。

- Build system: PlatformIO
- Framework: Arduino
- Platform: pioarduino / platform-espressif32 `55.03.37`
- Arduino-ESP32: `3.3.7`
- Board definition: repository-local `boards/m5stack-nanoc6.json`
- PlatformIO Core: `6.1.19`
- CI: GitHub Actions

`platformio.ini` をローカル開発とCIの共通設定とする。

## PlatformIO + Arduinoを採用する理由

- NanoC6 / ESP32-C6をC/C++で扱える。
- toolchain、board、build flagsをrepositoryで固定できる。
- GitHub Actions上でローカルと同じbuild commandを使用できる。
- Servo、Wi-Fi、HTTP/HTTPS等をArduino API / ESP-IDF APIで実装できる。
- partition、Flash mapping、heap計測をESP-IDF APIで扱える。

## pioarduinoの固定

```ini
platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.37/platform-espressif32.zip
board = m5stack-nanoc6
framework = arduino
```

この構成でArduino-ESP32 `3.3.7` が導入されることを確認済みである。

## NanoC6専用board定義

汎用 `esp32-c6-devkitc-1` は8MB Flashを前提とする一方、NanoC6は4MB Flashである。そのため `boards/m5stack-nanoc6.json` で以下を明示する。

- MCU: ESP32-C6
- CPU: 160MHz
- Flash: 4MB
- PlatformIO reported RAM: 327680 bytes
- `ARDUINO_M5STACK_NANOC6`

## Device / release profile

host-sideのFlash検証とdistribution bundle生成では、`config/devices/<device-id>.json`をdevice/release contractとして使用する。現在のreference profileは`config/devices/m5stack-nanoc6.json`である。

profileはdevice identity、release表示名、ESP Web Tools `chipFamily`、Flash容量、PlatformIO release environment、partition CSV、full-flash image名、各flash partのoffset / limit / bundle filenameを保持する。

検証・bundle toolingはdeviceを明示選択する。

```cmd
python tools\check_flash_layout.py --device m5stack-nanoc6
python tools\build_release_bundle.py --device m5stack-nanoc6 --version dev
```

`--profile path\to\profile.json`でprofile pathを直接指定することもできる。`--device`も`--profile`も指定しない暗黙NanoC6 fallbackは使用しない。

`platformio.ini`のboard/build environmentは引き続きbuild system側の正本であり、このprofileはGPIOやruntime hardware adaptationを表さない。新deviceはprofile追加やcompile成功だけでsupportedとは扱わず、実機verificationを別途必要とする。

## Final partition layout

最終構成では以下を使用する。

```ini
board_build.partitions = partitions/cube_scrambler_4mb.csv
```

| Partition | Offset | Size | Purpose |
|---|---:|---:|---|
| nvs | `0x9000` | `0x5000` | NVS |
| otadata | `0xE000` | `0x2000` | OTA slot selection metadata / Arduino `boot_app0.bin` |
| ota_0 | `0x10000` | `0x150000` | Firmware slot A (1.3125 MiB) |
| ota_1 | `0x160000` | `0x150000` | Firmware slot B (1.3125 MiB) |
| solver | `0x2B0000` | `0x110000` | Min2Phase tables (1.0625 MiB) |
| web | `0x3C0000` | `0x40000` | SPIFFS Web UI / TLS material (256 KiB) |

Flash終端は `0x400000` でNanoC6の4MiBをちょうど使用する。

Arduino/pioarduino uploadは `boot_app0.bin` を `0xE000` へ書くため、`otadata` はArduino標準layoutと同じ `0xE000` に置く。独立した `phy_init` partitionは最終layoutでは使用しない。

### OTA policy

Partition layoutは将来の安全なapplication OTAを可能にするため `ota_0` / `ota_1` / `otadata` を確保する。

現時点のMVPでは、LAN上にFirmware upload APIを公開しない。Firmware更新の正式手段はUSB/PlatformIOとする。将来OTAを追加する場合も、現在実行中でないapp slotへ書き込み、検証後にboot slotを切り替えるESP-IDFの通常OTA方式を利用する。

Solver tableとWeb UIはapplication OTAとは独立したdata partitionである。これらをネットワーク更新する場合はapplication OTAとは別途、安全性・整合性・電源断耐性を設計する。

### Capacity gate

CIはbuild後に以下を実行する。

```cmd
python tools\check_flash_layout.py --device m5stack-nanoc6
```

これにより以下を検証する。

- profileで宣言されたFlash終端、partition overlap / alignment
- partition CSVとprofileのfirmware / solver / web / boot_app0 range一致
- `ota_0` と `ota_1` が同一サイズ
- `firmware.bin` が1 app slotへ収まること
- Min2Phase imageが`solver` partitionへ収まること
- SPIFFS imageが`web` partitionへ収まること

release bundle生成も同じprofileを使用し、source image容量をpreflightしてからoutputを作成する。profile、partition、imageが矛盾する場合はpublishable bundleを生成せずfailする。

## 過去のSolver PoC layout

Phase 2では `partitions/solver_poc.csv` を使用していた。これはSolverのFlash mmap成立性を確認するための履歴ファイルであり、現在のbuildには使用しない。

旧offsetは `app0=0x10000`, `solver=0x190000`, `web=0x2A0000` だった。最終layoutへの移行時にはSolver/Web imageの再書込みが必要である。

## ローカル環境

Windowsでの新規clone時セットアップは [`LOCAL_SETUP.md`](./LOCAL_SETUP.md) を参照する。

```cmd
scripts\setup-windows.cmd
```

通常のcmd session開始時:

```cmd
call .venv\Scripts\activate.bat
set PLATFORMIO_CORE_DIR=%CD%\.platformio-core
```

## Build / Upload / Monitor

```cmd
pio run -e m5stack-nanoc6
pio run -e m5stack-nanoc6 -t upload
pio device monitor -p COM3 -b 115200
```

COM番号は環境によって異なるため、必要に応じて以下で確認する。

```cmd
pio device list
```

Solver/Web partitionは専用スクリプトで書き込む。

```cmd
scripts\flash-solver-image.cmd .pio\min2phase-tables.bin COM3
scripts\flash-web-ui.cmd COM3
```

## GitHub Actions CI

`.github/workflows/firmware-build.yml` を通常のcompile/link/release-contract CIとする。

- push to `main`
- Pull Request
- manual dispatch

でFirmware、SPIFFS image、最終Flash layout、full-flash bundle、Web Serial manifest / release metadataを検証する。現在はNanoC6単独jobであり、未サポートdeviceをmatrixへ先行追加しない。

`.github/workflows/release-bundle.yml`も同じNanoC6 profile contractを使用してmanual distribution bundleを生成する。

GitHub-hosted runnerでは以下の実機項目は検証しない。

- boot
- credential-free first boot / provisioning
- USB Serial
- runtime heap / minimum free heap
- Servo
- physical stop
- Wi-Fi STA/AP / Web UI/API
- Flash mmap性能
- Solver initialization / solve / scramble /実行時間
- 実機へのWeb Serial install

新deviceを正式supportする場合、上記のうち対象deviceに適用される項目を実機で確認する。build成功だけではsupport完了としない。
