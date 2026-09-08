# Merge Gate Policy

このrepositoryはpublic GitHub repositoryとして、Loop Engineeringのmerge条件をproject固有Hosted CIとreviewで運用します。Loop Verifier / Work Item Validation statusは使用しません。

## Canonical merge conditions

通常のimplementation PRは少なくとも次を満たすまでmergeしません。

- 対象Task / Parent Spec / blocker / PR metadataが`agent/WORK-TRACKING.md`の条件を満たす。
- TaskのAcceptance Criteriaを満たす。
- 変更範囲に適用されるGitHub Actions checkがlatest PR headでsuccessである。
- workflow対象外の変更では必要なmanual / local validation evidenceがPRに記録されている。
- 必要な実機verificationがTask / Specにある場合、そのevidenceが記録されている。
- blocking review findingがない。

## Canonical delivery hygiene

通常のLoop Task PRはsquash mergeを標準とします。1 Taskが1つのdurable mainline commitとなり、Issue / PR / commit historyの対応を追いやすくするためです。

repository設定で利用可能なら次を推奨します。

- Squash merging: enabled
- Merge commits: disabled
- Rebase merging: disabled
- Automatically delete head branches: enabled

例外的に別のhistory strategyが必要な場合はproject-wide decisionとして明示し、単発PRごとに方式を変えません。

## Current CI boundary

- firmware / distribution変更では`.github/workflows/firmware-build.yml`を主要gateとする。
- publishable release形状は`.github/workflows/release-bundle.yml`のcontractとも整合させる。
- docs / Loop metadataだけでproduct workflowのpath filter対象外なら、そのためだけにfirmware buildを強制しない。
- CI / Artifact / security boundaryの詳細は`agent/VERIFICATION.md`を参照する。

Loop work-item専用Actions workflowを追加しません。既存product CIをLoop metadata検証のために複雑化しません。

## Server-side enforcement

public repositoryでは利用可能なbranch protection / rulesetで`main`へのPR経由とrelevant checksをserver-side enforcementすることを推奨します。

最低限の候補:

- pull request経由を要求する。
- firmware/distribution変更へrelevant required status checkを要求する。
- force push / branch deletionを禁止する。

strict up-to-date requirement、approval数、conversation resolution等はproject事情に合わせ、Loop standardとして過剰に固定しません。

server-side ruleが未設定でもprocess-level policyは変わりません。

- `main`へ通常implementationを直接pushしない。
- merge前にlatest PR headのrelevant checksを確認する。
- missing / pending / failureのrequired checkをsuccess扱いしない。
- checkを通すためにtest、Acceptance Criteria、workflow permission、security boundaryを弱めない。
