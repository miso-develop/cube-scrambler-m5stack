# Web Serial Installer

Cube Scramblerは、device/release profileからESP Web Toolsを利用したper-device browser installerを生成する。

## Architecture

```text
config/devices/<device-id>.json
  -> scripts/build-release-bundle.cmd [version] <device-id>
  -> tools/build_release_bundle.py --device <device-id>
  -> .pio/release/web-installer/
       index.html
       installer.js
       manifest.json
       <device-specific-full-image>.bin
       ca.crt

local verification
  -> scripts/serve-web-installer.cmd
  -> http://localhost:8000/
```

公開hostingする場合は `.pio/release/web-installer/` の内容だけをHTTPS originへ配置できる。

Installer source (`tools/web_installer/`) はdevice-neutralなtemplateであり、bundle生成時に選択済みprofileからdevice名、full-image名、Serial表示名、必要なrecovery guidanceを埋め込む。未解決template placeholderや必須metadata欠落はpublishable bundleを生成せずfailする。

## Device selection

Device選択は常に明示する。

- NanoC6: `m5stack-nanoc6`
- AtomS3 Lite: `m5stack-atoms3-lite`

USB VID/PID、serial-port label、ESP chip family、runtime board detectionからM5Stack modelを推測しない。`chipFamily`はESP Web Toolsが選択済みbundleと接続chipの互換性を確認するための安全guardであり、product identity detectorではない。

各deviceは独立したmanifest/full imageを持つ。

| Device | Manifest chipFamily | Full image |
| --- | --- | --- |
| M5Stack NanoC6 | `ESP32-C6` | `cube-scrambler-nanoc6-full.bin` |
| M5Stack AtomS3 Lite | `ESP32-S3` | `cube-scrambler-atoms3-lite-full.bin` |

AtomS3 Liteのinstaller/release contractは実装対象だが、real-hardware support gate (#27) が完了するまではbuild/installer生成成功だけでsupportedとは扱わない。

## Installation flow

1. 対象deviceを明示して生成したInstaller pageを開く。
2. Installer pageでWi-Fi modeを選び、Station modeの場合はSSID/passwordを入力する。
3. ESP Web Tools v10が対象deviceのcomplete full-flash imageをoffset `0x0`へ書き込む。
4. ESP Web Toolsの書込み完了後、**Configure Wi-Fi via USB** を実行する。
5. BrowserがWeb Serialを115200 baudで開き、Firmwareの既存Serial commandを利用する。
6. Station modeでは以下を送信する。

```text
wifi-set <ssid>|<password>
wifi-mode sta
reboot
```

7. Access Point modeでは以下を送信する。

```text
wifi-mode ap
reboot
```

8. Installerは既存の `PASS` responseを確認してから次のcommandへ進む。最終rebootはUSB接続が切れるためresponseを必須としない。

独自flashing protocolは実装せず、Firmware installationはESP Web Toolsへ委譲する。

### AtomS3 Lite download-mode recovery

通常のWeb Serial flashingでdownload modeへ入れない場合、AtomS3 LiteではResetを約2秒保持し、内部green LEDが点灯したら離してからinstallを再試行する。このguidanceはAtomS3 Lite profileから生成installerへ埋め込まれ、NanoC6 installerには表示されない。

## Credential handling

Wi-Fi credentialはdistribution imageから分離する。

- Passwordはlive browser form / JavaScript memoryにのみ保持する。
- `manifest.json` へ書き込まない。
- `release.json` へ書き込まない。
- Full imageへ書き込まない。
- Installerから外部network serviceへ送信しない。
- Serial log上では `wifi-set` のpasswordをmaskする。
- Provisioning成功後はpassword fieldをclearする。

FirmwareのSerial command formatは `|` をSSID/password separatorとして使うため、SSIDに `|` または改行は使用できない。Passwordの改行も拒否する。

## Build locally

Device IDを必ず明示する。

NanoC6:

```cmd
scripts\build-release-bundle.cmd dev m5stack-nanoc6
```

AtomS3 Lite:

```cmd
scripts\build-release-bundle.cmd dev m5stack-atoms3-lite
```

生成先はいずれも以下で、最後に生成したdevice bundleが入る。

```text
.pio\release\web-installer\
```

複数deviceのrelease artifactを同時に生成・保持するHosted release workflowは#26で扱う。

## Serve locally

```cmd
scripts\serve-web-installer.cmd
```

Default URL:

```text
http://localhost:8000/
```

Alternate port:

```cmd
scripts\serve-web-installer.cmd 8080
```

Web SerialをサポートするChromium系desktop browserを使用する。

## Hardware validation checklist

正式support前に対象deviceのreal hardwareで以下を確認する。

1. Device-specific full imageをESP Web Toolsからinstallできる。
2. Install後のFirmwareがbootする。
3. Station SSID/passwordをWeb Serial stepから設定できる。
4. Station modeがreboot後もpersistし接続できる。
5. AP modeを選択でき、reboot後もpersistする。
6. Serial provisioning failureがinstaller errorとして表示される。
7. Solver / Web UI / HTTPS assetsがfull-image install後に利用できる。
8. Bundled `ca.crt` がrelease Web partition内のHTTPS certificateと一致する。
9. AtomS3 Liteでは通常installとmanual download-mode recoveryの両方を確認する。

AtomS3 Liteの完全なsupport evidenceはIssue #27で管理する。

## Public hosting

`.pio/release/web-installer/` はstatic filesだけで構成されるため、GitHub Pages等のHTTPS static hostingへ配置できる。

Source buildを公開hostingと分離したい場合でも、installer directoryだけをartifactまたは別branchへpublishできる。
