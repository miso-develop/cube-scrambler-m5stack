# Verification Policy

## Purpose

このファイルは、通常の実装で常時意識する必要はないが、GitHub Actions、Artifact、Visual QA、release bundle、実機検証、verification trust boundaryを変更・判断・検証する際に参照する補助ルールです。

このrepositoryはLoop Verifier infrastructureを使用しません。required verificationはTask / Spec、repository code/config、既存GitHub Actions、必要な実機evidenceで成立させます。

## Source of truth

- featureごとの必須verificationは対象`[Spec]` / `[Task]`を正とする。
- 通常firmware/distribution CIは`.github/workflows/firmware-build.yml`をauthoritative Hosted CI entrypointとする。
- manual release bundle生成は`.github/workflows/release-bundle.yml`を正とする。
- build / flash layout / bundleの具体的な実装契約は`platformio.ini`、`partitions/`、`tools/`、関連docsを参照する。
- device固有の実機measurement guidanceは`agent/CUBE-ENGINEERING.md`を参照する。

## GitHub Actions usage

- 不要なGitHub Actions runは行わない。
- docs / planningだけなどproduct buildが不要な変更では、path filterによりActionsを起動しない構成を優先する。
- 通常CIは自動判定に必要な処理へ限定し、目視確認や保存された成果物が必要な処理は必要に応じて分離する。
- firmware / distribution変更ではhost-side source validation、solver/web asset生成、credential-free firmware、filesystem、flash layout、full-flash bundle、manifest validationの現行contractを弱めない。
- high-load checkを追加する場合は、毎PRで本当に必要かをSpecで判断し、manual / explicit executionで足りるものを自動triggerへ入れない。

## Pull-request trust boundary

PR headのrepository code、test、build scriptはuntrusted inputとして扱う。

- PR headをcheckoutしてbuild / testするjobは原則`contents: read`のみとする。
- untrusted checkoutでは`persist-credentials: false`を維持する。
- build/test jobへrepository secret、cloud credential、PAT、write-capable tokenを不要に渡さない。
- `pull_request_target`等のprivileged contextでPR head codeを実行しない。
- third-party Actionsは可能な限りimmutable commit SHAでpinする。
- workflow permissionやcredential boundaryをCIを通す目的だけで弱めない。

## Artifact policy

Artifactは通常CIの常設副産物ではなく、保存された実ファイルが必要な場合だけ生成する。

- Artifactが不要なrunでは生成しない。
- 通常firmware CIは原則Artifactをuploadしない。
- release bundle等、ユーザー確認・配布準備で成果物が必要なworkflowだけArtifact生成を行う。
- retentionは原則1日とする。
- Artifactには確認・配布判断に必要なファイルだけを含め、不要log、debug output、中間生成物を保存しない。
- Artifactを長期履歴やbackupとして使用しない。
- Visual QA側で安価に再生成できる場合、通常CIからArtifactを受け渡すためだけの常時uploadを作らない。

## Visual QA

Visual QAは通常のPR更新ごとに無条件実行しない。

次の場合に必要性を判断する。

- Web UIの見た目・操作性へ影響する変更。
- camera / rendering / generated asset等、自動testや数値だけでは品質を十分判定できない変更。
- 新しいdeviceで表示差・browser差を確認する必要がある場合。
- ユーザーが成果物・画面の目視確認を要求した場合。

必要な場合だけ専用の確認手順・生成物をTask / PRに記録する。

## Firmware / release validation

firmware、partition、release tooling、Web Serialに影響する変更では必要に応じて次を確認する。

- target PlatformIO environmentがbuild可能。
- credential-free firmwareが生成される。
- solver table / Web filesystemが所定領域へ収まる。
- partition overlap、flash size、full image sizeがvalid。
- release bundleのmanifest / binary path / chip family / repository URLが期待値と一致する。
- distribution artifactにlocal credential、private key、不要debug dataが含まれない。
- device profile追加時はNanoC6のlayout値を暗黙に再利用せず、対象flash size / offsetsを個別検証する。

release workflowのupload successだけをfirmware correctnessの代替にしない。

## Real-hardware verification

hardware behaviorを変更するTask、または新device supportを追加するTaskでは、build successだけで完了扱いにしない。Task / Specで必要な実機evidenceを明示し、可能な範囲で次を確認する。

- boot / reset stability
- Wi-Fi station / AP
- Web UI / HTTP API
- solver initialization / representative solve
- StandServo / ArmServo behavior
- physical stop
- heap / resource headroom
- Web Serial / flashing path

measurement値は独立した進捗ファイルではなく対象Issue / PRへ残す。

## Documentation-only and planning changes

product pathに影響しない変更でfirmware workflowが起動しないことは正常です。

- CIを起動するためだけにproduct fileへ無意味な変更を加えない。
- applicable checkが存在しない場合はdiff review、syntax/format確認、reference解決等の再現可能なvalidation evidenceをPRへ記録する。

## Failure handling

unexpected failureではtest failure、resource exhaustion、environment failure、workflow/security misconfigurationを区別する。exit codeやrunner failureだけからroot causeを断定せず、`diagnosing-bugs`で再現可能なevidenceを作る。再利用価値のあるfailureは`troubleshooting-cases`へ記録する。
