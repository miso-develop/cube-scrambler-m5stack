# Standalone Web UI

The browser UI is stored in the dedicated `web` SPIFFS partition instead of being compiled into firmware RAM/rodata.

## Source

The host generator downloads the existing Cube Scrambler UI from the pinned source revision:

- repository: `miso-develop/cube-scrambler`
- revision: `ff14a5e9e1b721f7f009bd73945e4e5e4b3d4e51`

`tools/generate_web_ui.js` applies only standalone/deployment-specific changes:

1. HTTP 202 execution requests poll `/api/status` until `finished`, `stopped`, or `error`.
2. CubeChample-dependent scramble buttons are removed; only Random remains.
3. Deployment paths are flattened to short `c/`, `i/`, and `j/` paths and HTML/ES-module references are rewritten automatically, because ESP32 SPIFFS rejects long object names such as the original nested `js/alpine/cubeUi/...` paths.

The existing camera scanner is retained and is served from the production HTTPS origin so `getUserMedia()` receives a Secure Context.

## Generate assets

```cmd
scripts\generate-web-ui.cmd
```

Equivalent command:

```cmd
node tools\generate_web_ui.js --output .pio\web-ui
```

Generated files are under `.pio/web-ui` and are intentionally not committed. The generator also checks that every SPIFFS deployment path is shorter than the object-name limit.

The current generated UI contains 18 source assets and about 150 KiB of payload.

## Build filesystem image

```cmd
pio run -e m5stack-nanoc6 -t buildfs
```

PlatformIO uses:

```ini
[platformio]
data_dir = .pio/web-ui

[env:m5stack-nanoc6]
board_build.filesystem = spiffs
```

The final partition table contains:

```text
web, data, spiffs, 0x3C0000, 0x40000
```

This gives the Web UI / TLS material a 256 KiB partition while freeing enough Flash for two OTA application slots.

## Upload filesystem image

On the tested Windows/NanoC6 setup, PlatformIO `uploadfs` has intermittently stopped mid-transfer with `The chip stopped responding`. Use the project PyPI `esptool` path instead of PlatformIO's bundled uploader:

```cmd
scripts\flash-web-ui.cmd COM3
```

The script builds `.pio\build\m5stack-nanoc6\spiffs.bin` when needed and writes it directly at the final `web` partition offset using 460800 baud:

```cmd
.venv\Scripts\python.exe -m esptool --chip esp32c6 -p COM3 -b 460800 write-flash 0x3C0000 .pio\build\m5stack-nanoc6\spiffs.bin
```

`pio run -e m5stack-nanoc6 -t uploadfs` is therefore not the recommended Windows deployment path for this project.

Re-uploading application firmware does not require re-uploading the Web UI unless the Web assets or partition layout change. Migrating from the old PoC layout does require a one-time Web image reflash because the partition moved from `0x2A0000` to `0x3C0000`.

## Firmware behavior

At boot, `CubeHttpServer` mounts SPIFFS partition label `web` without formatting it.

When the image is present:

```text
Web UI SPIFFS: READY used=... total=... bytes
Web UI: READY
```

The production UI and application APIs are served over HTTPS on port 443. Plain HTTP on port 80 only serves `/ca.crt` for trust bootstrap and otherwise redirects to HTTPS.

CSS, JavaScript, PNG, favicon, and JSON files are streamed directly from SPIFFS. `/tls.key` and `/tls.crt` are not exposed as static files.

If the filesystem is absent or invalid, the firmware deliberately does not format the partition because doing so would destroy a recoverable Web image.

`GET /api/status` includes:

```json
{"webUiReady":true,"httpsReady":true}
```

Normal standalone startup initializes `CubeRobot` before browser controls are exposed. Serial diagnostics remain available but are not required for boot or normal operation.
