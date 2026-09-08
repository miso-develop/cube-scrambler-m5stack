---
name: troubleshooting-cases
description: Use only after an unexpected failure, regression, broken automation, repeated failed attempt, or behavior that contradicts the intended design. Do not load during normal implementation or successful operation.
metadata:
  version: "1.0"
---

# Troubleshooting Cases

Loop Engineeringで発生したfailureの再発防止用case libraryです。正常なiterationでは読み込みません。

## Procedure

1. 観測できるsymptomと失敗境界を特定する。
2. `cases/`をerror text、command、runtime、subsystemで検索する。
3. 証拠が一致する既知caseだけを再利用する。似ているだけのcaseへ無理に当てはめない。
4. documented fixとpre-detection / regression protectionを適用する。
5. materially newなfailureを解決した場合だけ、新しいcaseを追加する。

## Case requirements

各caseには最低限次を記録する。

- Context
- Symptom
- Root cause
- Fix
- Pre-detection
- Regression protection
- Evidence

credential、PAT、private key、secret実値、不要な個人情報は記録しない。

新規caseは`cases/_TEMPLATE.md`から作成する。
