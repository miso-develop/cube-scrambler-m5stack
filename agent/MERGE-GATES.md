# Merge Gate Policy

このrepositoryはpublic GitHub repositoryとして、Loop Engineeringのmerge条件をproject固有Hosted CIとreviewで運用します。Loop Verifier / Work Item Validation statusは使用しません。

## Canonical merge conditions

通常のimplementation PRは少なくとも次を満たすまでmergeしません。

- 対象Task / Parent Spec / blocker / PR metadataが`agent/WORK-TRACKING.md`の条件を満たす。
- TaskのAcceptance Criteriaを満たす。
- 変更範囲に適用されるGitHub Actions checkがsuccessである。
- workflow対象外の変更では必要なmanual / local validation evidenceがPRに記録されている。
- blocking review findingがない。

## Delivery hygiene

通常のLoop Task PRはsquash mergeを推奨します。1 Taskを1 durable mainline commitとして追跡しやすくするためです。

可能ならrepository設定は次を推奨します。

- pull request経由を要求
- relevant required status checksを要求
- squash mergeを標準化
- merge後のhead branch削除

## Current CI boundary

firmware / distribution変更では`.github/workflows/firmware-build.yml`を主要gateとします。release bundleのpublishable形状は`.github/workflows/release-bundle.yml`のcontractとも整合させます。

Loop work-item専用Actions workflowを追加しません。既存product CIをLoop metadata検証のために複雑化しません。

## Server-side enforcement

public repositoryでは利用可能なbranch protection / rulesetで`main`へのPR経由とrelevant checksをserver-side enforcementすることを推奨します。

server-side ruleが未設定でもprocess-level policyは変わりません。

- `main`へ通常implementationを直接pushしない。
- merge前にlatest PR headのrelevant checksを確認する。
- missing / pending / failureのrequired checkをsuccess扱いしない。
- checkを通すためにtestやAcceptance Criteriaを弱めない。
