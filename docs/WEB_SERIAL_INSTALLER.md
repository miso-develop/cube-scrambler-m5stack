# Web Serial Installer

Cube Scrambler NanoC6は、ESP Web Toolsを利用したbrowser-based installerを生成できます。

## Architecture

```text
repository
  -> scripts/build-release-bundle.cmd
  -> .pio/release/web-installer/
       index.html
       installer.js
       manifest.json
       cube-scrambler-nanoc6-full.bin
       ca.crt

local verification
  -> scripts/serve-web-installer.cmd
  -> http://localhost:8000/
```

公開hostingする場合は `.pio/release/web-installer/` の内容だけをHTTPS originへ配置できます。

## Installation flow

1. Installer pageでWi-Fi modeを選び、Station modeの場合はSSID/passwordを入力する。
2. ESP Web Tools v10がcomplete 4 MiB imageをoffset `0x0`へ書き込む。
3. ESP Web Toolsの書込み完了後、**Configure Wi-Fi via USB** を実行する。
4. BrowserがWeb Serialを115200 baudで開き、Firmwareの既存Serial commandを利用する。
5. Station modeでは以下を送信する。

```text
wifi-set <ssid>|<password>
wifi-mode sta
reboot
```

6. Access Point modeでは以下を送信する。

```text
wifi-mode ap
reboot
```

7. Installerは既存の `PASS` responseを確認してから次のcommandへ進む。最終rebootはUSB接続が切れるためresponseを必須としない。

独自flashing protocolは実装せず、Firmware installationはESP Web Toolsへ委譲する。

## Credential handling

Wi-Fi credentialはdistribution imageから分離する。

- Passwordはlive browser form / JavaScript memoryにのみ保持する。
- `manifest.json` へ書き込まない。
- Release fileへ書き込まない。
- Installerから外部network serviceへ送信しない。
- Serial log上では `wifi-set` のpasswordをmaskする。
- Provisioning成功後はpassword fieldをclearする。

FirmwareのSerial command formatは `|` をSSID/password separatorとして使うため、SSIDに `|` または改行は使用できない。Passwordの改行も拒否する。

## Build locally

```cmd
scripts\build-release-bundle.cmd dev
```

生成先:

```text
.pio\release\web-installer\
```

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

Release前にreal NanoC6で以下を確認する。

1. Full 4 MiB imageをESP Web Toolsからinstallできる。
2. Install後のFirmwareがbootする。
3. Station SSID/passwordをWeb Serial stepから設定できる。
4. Station modeがreboot後もpersistし接続できる。
5. AP modeを選択でき、reboot後もpersistする。
6. Serial provisioning failureがinstaller errorとして表示される。
7. Solver / Web UI / HTTPS assetsがfull-image install後に利用できる。
8. Bundled `ca.crt` がrelease Web partition内のHTTPS certificateと一致する。

## Public hosting

`.pio/release/web-installer/` はstatic filesだけで構成されるため、GitHub Pages等のHTTPS static hostingへ配置できる。

Source buildを公開hostingと分離したい場合でも、installer directoryだけをartifactまたは別branchへpublishできる。
