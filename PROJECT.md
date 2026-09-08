# Cube Scrambler M5Stack Project

このファイルはproject全体に適用する長寿命の前提だけを保持します。featureごとの仕様、実装Task、進捗はGitHub Issues / Pull Requestsを正とします。

## Purpose

M5Stackデバイス上で単体動作するCube Scrambler firmwareを提供し、Cube操作、solver、Web UI / API、Wi-Fi、Web Serial配布を一貫した公開プロジェクトとして維持する。

現在の正式なreference deviceはM5Stack NanoC6。複数M5Stackデバイスへの対応拡張をproject scopeに含む。

## Scope

### In scope

- Cube Scramblerのfirmwareと共通domain logic
- servo / button / LED等のhardware adaptation
- solver / scramble sequence
- Wi-Fi、Web UI、HTTP API
- device別PlatformIO build / flash layout
- Web Serial installerとrelease bundle
- M5Stack device追加と、そのためのarchitecture整理
- public GitHub repository上のCI / release / Loop Engineering運用

### Out of scope

- 旧PC runtimeの復活
- secret / credentialを含む配布物のcommit
- 1つのuniversal firmware binaryを全deviceでruntime自動判定して動かすこと（明示Specで変更しない限り）
- Loop Verifier、Shared Local Verifier、Cloud Run verifier等のVerifier infrastructure

## Constraints

- repositoryはpublicであり、公開可能な情報だけをtracked file / Issue / PRへ記録する。
- `include/wifi_credentials.h`、private key、token等をcommitしない。
- GitHub Actionsはpublic repository向けHosted CIを基本とする。
- third-party Actionsは可能な限りcommit SHAでpinし、最小permissionsを維持する。
- 現在のNanoC6 build / release pathをreference baselineとしてregressionさせない。
- `.cmd` は`.gitattributes`に従ってCRLFを維持する。

## Invariants / decisions

- GitHub Issues / Pull RequestsをLoop Engineeringの進捗source of truthとし、別の進捗台帳を二重管理しない。
- production codeの実装は原則としてopenな`[Task]` Issueから行う。
- current reference deviceはNanoC6。multi-device architectureの詳細は`[Map]` / `[Decision]` / `[Spec]`で決定する。
- Loop Engineeringを使用するが、Verifier mechanismはこのrepositoryへ導入しない。
- required verificationはproject固有のtest / build / GitHub Actionsで定義する。

## References

- `README.md`
- `docs/REQUIREMENTS.md`
- `docs/BUILD.md`
- `docs/WEB_SERIAL_INSTALLER.md`
- `AGENTS.md`
- `agent/WORK-TRACKING.md`
