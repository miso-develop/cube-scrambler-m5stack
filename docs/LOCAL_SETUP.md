# Windows ローカル開発環境のセットアップ

新規clone後に、プロジェクト専用のPython仮想環境 `.venv` とPlatformIO Core領域 `.platformio-core` を作成する手順です。

## 前提

- Windows
- Git
- Scoop
- Scoopの `python312` パッケージ
- Node.js 18以降（Min2Phase tableのoffline生成時に使用）

本プロジェクトではFirmware build用Pythonとして **Python 3.12** を使用します。pioarduino `55.03.37` はPython 3.14をサポートしないため、Scoopの通常の `python` パッケージが3.14以降を指している場合でも使用しません。

Python 3.12が未導入の場合:

```cmd
scoop install python312
```

Node.jsが未導入の場合:

```cmd
scoop install nodejs-lts
```

## 推奨セットアップ

リポジトリrootで以下を実行します。

```cmd
scripts\setup-windows.cmd
```

スクリプトは以下を行います。

1. Scoop版Python 3.12の存在確認
2. `.venv` の作成
3. `.venv` へPlatformIO Core `6.1.19` をインストール
4. `.venv` へ `esptool==5.1.0` をインストール
5. HTTPS証明書生成用 `cryptography` をインストール
6. `PLATFORMIO_CORE_DIR` を `<repo>\.platformio-core` に設定
7. PlatformIO / esptoolのversion確認

`.venv/` と `.platformio-core/` はローカル生成物であり、Git管理しません。

## 手動セットアップ

Scoopが既定位置 `%USERPROFILE%\scoop` にインストールされている場合:

```cmd
"%USERPROFILE%\scoop\apps\python312\current\python.exe" -m venv .venv
call .venv\Scripts\activate.bat
python --version
python -m pip install platformio==6.1.19 esptool==5.1.0 "cryptography>=45,<47"
set PLATFORMIO_CORE_DIR=%CD%\.platformio-core
pio --version
python -m esptool version
```

`python --version` は `Python 3.12.x`、`pio --version` は `PlatformIO Core, version 6.1.19` であることを確認します。

Scoopを既定位置以外へ導入している場合は以下で配置先を確認できます。

```cmd
scoop prefix python312
```

## 2回目以降のcmd session

```cmd
call .venv\Scripts\activate.bat
set PLATFORMIO_CORE_DIR=%CD%\.platformio-core
```

その後、通常のPlatformIOコマンドを利用できます。

```cmd
pio run -e m5stack-nanoc6
pio run -e m5stack-nanoc6 -t upload
pio device monitor -b 115200
```

## Solver table imageの生成

実Min2Phase tableはNanoC6起動時には生成せず、host上で生成してFlashへ書き込みます。

```cmd
scripts\generate-solver-tables.cmd
```

内部では以下を実行します。

```cmd
node tools\generate_min2phase_tables.js --output .pio\min2phase-tables.bin
```

Generatorは以下の固定sourceを使用します。

```text
cs0x7f/min2phase.js
revision: 0ba83a6177d816f72af1a45c9015349da597456a
Git blob: b8a641b03cea8e6d7a6b0f2a6da74ff98e03475e
```

固定revisionとGit blob SHA-1を検証してからtableを生成し、version / source revision / CRC32付きのsolver imageにします。

## Solver / Web partitionへの書込み

最終layoutではSolverを `0x2B0000`、Web UIを `0x3C0000` に配置します。

```cmd
scripts\flash-solver-image.cmd .pio\min2phase-tables.bin COM3
scripts\flash-web-ui.cmd COM3
```

Solverを直接書く場合:

```cmd
python -m esptool --chip esp32c6 -p COM3 -b 460800 write-flash 0x2B0000 .pio\min2phase-tables.bin
```

旧Solver PoC layoutではSolverが `0x190000`、Web UIが `0x2A0000` にありました。最終layoutへ初回移行するときはFirmware / Solver / Web UIをすべて書き直してください。

PlatformIO package `tool-esptoolpy@5.1.2` をWindowsから直接 `pio pkg exec` で起動するとPython依存が解決されない場合があるため、本プロジェクトでは `.venv` に導入したPyPI版esptoolを直接使用します。

## 環境分離

```text
cube-scrambler-m5stack/
├─ .venv/             # PlatformIO CLI / esptoolを含むPython 3.12 venv
├─ .platformio-core/  # toolchain / framework / pioarduino packages
├─ platformio.ini
└─ src/
```

これにより、ユーザー共通の `%USERPROFILE%\.platformio` や他プロジェクトのPython環境との干渉を避けます。

## Troubleshooting

### `python --version` が `Python` とだけ表示される

WindowsのApp Execution AliasがScoop版Pythonより先に解決されている可能性があります。セットアップ時はScoop版Python 3.12をフルパスで指定してください。

### Python 3.14を使用しているというエラー

`.venv` を削除してPython 3.12で再作成します。

```cmd
deactivate
rmdir /s /q .venv
"%USERPROFILE%\scoop\apps\python312\current\python.exe" -m venv .venv
```

### PlatformIO環境が壊れた場合

このプロジェクトでは `.venv` と `.platformio-core` を削除し、`scripts\setup-windows.cmd` で再生成できます。
