---
name: windows-cmd-scoop
description: Use when creating, modifying, reviewing, or debugging Windows cmd.exe .cmd scripts, existing .bat scripts, or Scoop-based CLI setup. Focus on quoting, batch-call control flow, error propagation, shim behavior, malformed Scoop manifests, line endings, and reproducible validation.
metadata:
  version: "1.1"
---

# Windows cmd + Scoop

Windows`cmd.exe`はPOSIX shellではない。batch control flow、argument quoting、command shim、exit-code handlingを明示的な設計条件として扱う。

このrepositoryでは新しいWindows command entrypointは`.cmd`を使用し、新しい`.bat`を導入しない。既存`.bat`を調査する必要がある場合だけ同じcmd.exe semanticsを適用する。

## Baseline batch shape

```bat
@echo off
setlocal
cd /d "%~dp0\..\.."

where node >nul 2>nul
if errorlevel 1 (
  echo Node.js is required.
  exit /b 1
)

rem work...

set "EXIT_CODE=%ERRORLEVEL%"
endlocal & exit /b %EXIT_CODE%
```

- `setlocal`でtemporary variableをcallerへ漏らさない。
- `%~dp0`からpathを解決し、callerのcurrent directoryを仮定しない。
- drive changeがあり得る場合は`cd /d`を使う。
- `set "NAME=value"`を使い、filesystem pathやspaceを含み得る値をquoteする。
- interactive alias、AutoRun、prompt固有設定へ依存しない。

## Calling other Windows commands

別batchから`.cmd` / `.bat`を直接実行するとcontrolが戻らない場合があるため、batch shimには`call`を使用する。

```bat
call npm --version
if errorlevel 1 exit /b %ERRORLEVEL%
```

`where <command>`で実体を確認する。`npm.cmd`、`npx.cmd`、Scoop shim等が対象になり得る。native`.exe`へ`call`を機械的に付けない。

## Exit codes

- 成否が重要なcommand直後にfailureを確認する。
- `echo`、`set`、別CLI等で置換される前に`%ERRORLEVEL%`をcaptureする。
- reusable helperでは`exit`ではなく`exit /b`を使う。
- pipelineは最後のcommand statusになる点に注意する。

## Expansion and metacharacters

`cmd.exe` metacharacterは`& | < > ( ) ^ % !`等。

- batch loop variableは`%%A`を使う。
- parenthesized block内の`%VAR%`はblock parse時に展開される。
- delayed expansionは必要な場合だけ有効化し、literal`!`を壊すことを考慮する。
- untrusted valueから巨大なcommand stringを組み立てない。

## Node.js -> Windows CLI boundary

NodeからWindows batch shimを呼ぶ場合、machine-local shell設定へ依存しない明示的な`cmd.exe /d /c` boundaryを優先する。`/d`はcmd AutoRunを無効化する。

command constructionは1つのhelperへ集約し、spaces、quotes、Windows pathを含むrepresentative argumentでtestする。

## Scoop-specific diagnosis

`scoop search`の`Invalid JSON`は、検索対象packageではなく別custom bucketの壊れたmanifestが原因になり得る。

1. configured bucketを確認しcustom bucketを特定する。
2. `%USERPROFILE%\scoop\buckets\<bucket>\bucket\`等のmanifestを確認する。
3. suspicious JSONをvalidateしてからScoop自体の再installを考える。
4. manifest修復またはbucket再登録の影響を確認する。
5. dataがvalidになってから元commandを再実行する。

Node.jsが既存prerequisiteならJSON validatorとして利用できる。

```cmd
node -e "JSON.parse(require('fs').readFileSync(process.argv[1],'utf8'))" path\to\manifest.json
```

stderrを隠してbroken bucketを見えなくしない。

## Scoop and shim assumptions

- `where scoop`、`where gh`等でcmd.exeが実行する実体を確認する。
- install成功後もcurrent processの`PATH`がrefreshされていない場合がある。
- `where`で解決できる場合はScoop shim pathをhard-codeしない。
- task目的でないglobal bucket / PATH mutationをscriptへ混ぜない。

## Line endings

このrepositoryでは`.cmd`をworking treeだけでなくcommitted Git blobもCRLFにする。

`.gitattributes`の`*.cmd -text`はproducerが作成したbytesをGitがnormalizeしないためのcontractであり、`.editorconfig`だけに依存しない。

- `.cmd`作成・rewrite時はblob/file生成前にCRLFを明示する。
- LF blobを作りcheckout変換で直ることを期待しない。
- `text eol=crlf`へ置き換えない。Git blobがLFへnormalizeされる構成はこのrepository contractと異なる。
- generated/scripted change後はworking treeと`HEAD:<path>` blobをbyte-awareに確認し、LF-only、mixed-LF、bare-CRを拒否する。

## Validation checklist

non-trivial batch changeでは必要な境界だけを再現可能に確認する。

1. repository root以外から実行して`%~dp0` anchoringを確認する。
2. path handling変更時はspaceを含むpathを試す。
3. failing child commandがnon-zero batch exit codeになることを確認する。
4. called`.cmd` shimからcontrolがwrapperへ戻ることを確認する。
5. committed `.cmd` blobがCRLF-onlyであることを確認する。
6. 変更対象commandのrelevant test / dry-runを実行する。
7. cloud / GitHub / Scoop等のmutationが不要な検証ではread-only / dry-runを使う。

quoting確認のためだけにlive infrastructureへ不要なdestructive actionを行わない。
