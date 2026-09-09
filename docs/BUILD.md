# ビルド環境

## 採用構成

Cube ScramblerのM5Stack firmware開発環境として以下を採用する。

- Build system: PlatformIO
- Framework: Arduino
- Platform: pioarduino / platform-espressif32 `55.03.37`
- Arduino-ESP32: `3.3.7`
- PlatformIO Core: `6.1.19`
- CI: GitHub Actions Hosted (`ubuntu-latest`)

`platformio.ini` をローカル開発とCIの共通設定とする。

## Device targets

現在のcompile-time targetは次の2系統。1つのuniversal firmwareでruntime device判定は行わない。

| Device | Normal env | Release env | Board | Flash | Status |
| --- | --- | --- | --- | ---: | --- |
| M5Stack NanoC6 | `m5stack-nanoc6` | `m5stack-nanoc6-release` | repository-local `m5stack-nanoc6` | 4 MiB | supported reference |
| M5Stack AtomS3 Lite | `m5stack-atoms3-lite` | `m5stack-atoms3-lite-release` | `esp32-s3-devkitc-1` | 8 MiB | support pending #27 real-hardware gate |

両deviceともStand ServoはGPIO2、Arm ServoはGPIO1を使用する。button/RGB等の実差分は`src/Hardware/DeviceControlsConfig.h`へ閉じ込める。

## PlatformIO + Arduinoを採用する理由

- ESP32-C6 / ESP32-S3をC/C++で扱える。
- toolchain、board、build flagsをrepositoryで固定できる。
- GitHub Actions上でローカルと同じbuild commandを使用できる。
- Servo、Wi-Fi、HTTP/HTTPS等をArduino API / ESP-IDF APIで実装できる。
- partition、Flash mapping、heap計測をESP-IDF APIで扱える。

## pioarduinoの固定

共通platformは以下へ固定する。

```ini
platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.37/platform-espressif32.zip
framework = arduino
```

この構成でArduino-ESP32 `3.3.7` が導入されることを確認済みである。

NanoC6は汎用 `esp32-c6-devkitc-1` のFlash容量前提が実機と異なるため、repository-local `boards/m5stack-nanoc6.json`を使用する。AtomS3 LiteはDecision #17に従い、pioarduinoの`esp32-s3-devkitc-1`を使用し、8 MiB / no-PSRAM条件をproject build設定で明示する。

## Device / release profile

host-sideのFlash検証、distribution bundle、installer生成では、`config/devices/<device-id>.json`をdevice/release contractとして使用する。

- `config/devices/m5stack-nanoc6.json`
- `config/devices/m5stack-atoms3-lite.json`

profileはdevice identity、release表示名、ESP Web Tools `chipFamily`、Flash容量、PlatformIO release environment、partition CSV、full-flash image名、各flash partのoffset / limit / bundle filename、installer表示/recovery metadataを保持する。

検証・bundle toolingはdeviceを明示選択する。

```cmd
python tools\check_flash_layout.py --device m5stack-nanoc6
python tools\check_flash_layout.py --device m5stack-atoms3-lite
python tools\build_release_bundle.py --device m5stack-nanoc6 --version dev
python tools\build_release_bundle.py --device m5stack-atoms3-lite --version dev
```

`--profile path\to\profile.json`でprofile pathを直接指定することもできる。`--device`も`--profile`も指定しない暗黙NanoC6 fallbackは使用しない。

profileはbuild/release contractであり、GPIO等のruntime hardware adaptationの正本ではない。profile追加・compile成功・CI成功だけでは新deviceを正式supportedとは扱わず、実機verificationを別途必要とする。

## Flash partition layouts

### NanoC6 — 4 MiB baseline

```ini
board_build.partitions = partitions/cube_scrambler_4mb.csv
```

| Partition | Offset | Size | Purpose |
|---|---:|---:|---|
| nvs | `0x9000` | `0x5000` | NVS |
| otadata | `0xE000` | `0x2000` | OTA slot selection metadata / Arduino `boot_app0.bin` |
| ota_0 | `0x10000` | `0x150000` | Firmware slot A |
| ota_1 | `0x160000` | `0x150000` | Firmware slot B |
| solver | `0x2B0000` | `0x110000` | Min2Phase tables |
| web | `0x3C0000` | `0x40000` | SPIFFS Web UI / TLS material |

Flash終端は `0x400000`。NanoC6の既存release baselineとしてregressionさせない。

### AtomS3 Lite — 8 MiB contract

```ini
board_build.partitions = partitions/cube_scrambler_atoms3_lite_8mb.csv
```

| Partition | Offset | Size | Purpose |
|---|---:|---:|---|
| nvs | `0x9000` | `0x5000` | NVS |
| otadata | `0xE000` | `0x2000` | OTA slot selection metadata / Arduino `boot_app0.bin` |
| ota_0 | `0x10000` | `0x280000` | Firmware slot A (2.5 MiB) |
| ota_1 | `0x290000` | `0x280000` | Firmware slot B (2.5 MiB) |
| solver | `0x510000` | `0x180000` | Min2Phase tables (1.5 MiB) |
| web | `0x690000` | `0x170000` | SPIFFS Web UI / TLS material |

Flash終端は `0x800000`。AtomS3 Liteのfull imageは`cube-scrambler-atoms3-lite-full.bin`としてexactly 8 MiBで生成する。

Arduino/pioarduino uploadは `boot_app0.bin` を `0xE000` へ書くため、両layoutとも`otadata`をArduino標準layoutと同じ位置へ置く。

### OTA policy

Partition layoutは将来の安全なapplication OTAを可能にするため `ota_0` / `ota_1` / `otadata` を確保する。

現時点ではLAN上にFirmware upload APIを公開しない。Firmware更新の正式手段はUSB/PlatformIOまたはdevice-specific Web Serial installerとする。Solver tableとWeb UIはapplication OTAとは独立したdata partitionであり、将来network updateを追加する場合は別途安全性・整合性・電源断耐性を設計する。

## Capacity / release gate

CIは各device laneで次を実行する。

1. credential-free release firmware build
2. SPIFFS build
3. `tools/check_flash_layout.py --device <device-id>`
4. `tools/build_release_bundle.py --device <device-id> --version ci`
5. profileから独立したbaseline値に対するmanifest / release metadata / image size / critical offset検証

これにより以下をfail closedで確認する。

- profileで宣言されたFlash終端、partition overlap / alignment
- partition CSVとprofileのfirmware / solver / web / boot_app0 range一致
- `ota_0` と `ota_1` が同一サイズ
- firmware / solver / SPIFFS imageが各reserved rangeへ収まること
- full imageのfile name / exact Flash size
- ESP Web Tools `chipFamily`
- device-specific installer identity / recovery metadata

## 過去のSolver PoC layout

Phase 2では `partitions/solver_poc.csv` を使用していた。これはSolverのFlash mmap成立性を確認するための履歴ファイルであり、現在のbuildには使用しない。

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

NanoC6:

```cmd
pio run -e m5stack-nanoc6
pio run -e m5stack-nanoc6 -t upload
```

AtomS3 Lite:

```cmd
pio run -e m5stack-atoms3-lite
pio run -e m5stack-atoms3-lite -t upload
```

Serial monitor例:

```cmd
pio device list
pio device monitor -p COM3 -b 115200
```

## Release bundle locally

Device IDを明示する。

```cmd
scripts\build-release-bundle.cmd dev m5stack-nanoc6
scripts\build-release-bundle.cmd dev m5stack-atoms3-lite
```

最後に生成したdeviceのbundleが`.pio\release\`へ出力される。

## GitHub Actions CI

`.github/workflows/firmware-build.yml` は通常のcompile/link/release-contract CIである。

- push to `main`
- Pull Request
- manual dispatch

host-side testを1回実行後、NanoC6とAtomS3 Liteを2要素matrixで独立buildする。両laneともfirmware、SPIFFS、layout、full-flash bundle、manifest/release metadataまで検証する。

`.github/workflows/release-bundle.yml`はmanual dispatch時に`device`をchoiceから明示選択し、選択した1deviceだけをbuild/validateして1日保持のartifactを生成する。USB VID/PID等による自動model判定は行わない。

GitHub-hosted runnerでは以下の実機項目は検証しない。

- boot / cold boot
- credential-free first boot / provisioning
- USB Serial / Web Serial reconnect
- actual Flash capacity
- runtime heap / minimum free heap
- Servo
- physical stop / status LED
- Wi-Fi STA/AP / Web UI/API
- Flash mmap性能
- Solver initialization / solve / scramble /実行時間
- 実機へのWeb Serial install / recovery procedure

AtomS3 Liteは上記をIssue #27で実機確認するまで正式support完了とは扱わない。Hosted CIの成功はrelease contractの成立を示すものであり、実機support evidenceの代替ではない。
