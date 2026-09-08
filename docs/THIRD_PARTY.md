# Third-party provenance / license notes

## Min2Phase

Cube Scramblerの既存実装ではhistorical revisionを利用していました。

```text
Repository: cs0x7f/min2phase.js
Revision: c3431fabd961fc2680b2de605ddd99a203665215
```

このhistorical revisionの `package.json` は `GPL-3.0` と記載されているため、本repositoryの移植元には使用していません。

現在のNanoC6向けsolver実装では以下のupstream revisionを固定しています。

```text
Repository: cs0x7f/min2phase.js
Revision: 0ba83a6177d816f72af1a45c9015349da597456a
package.json license: (MIT or GPL-3.0)
Copyright (c) 2023 Chen Shuang
```

同revisionのREADMEにはGPLv3とMITの両license textが含まれているため、本repositoryでは **MIT optionを選択して利用**しています。

移植・派生sourceにはMIT noticeを保持し、由来をこのdocumentから追跡可能にします。旧GPL-only revisionやGPL-only forkからsourceをコピーしません。

## Porting policy

- Algorithm/reference source: `cs0x7f/min2phase.js@0ba83a6177d816f72af1a45c9015349da597456a`
- Selected license: MIT
- Solver algorithmはC++へ移植しています。
- 大規模Move / Symmetry / Pruning tableはhost側で生成し、NanoC6ではFlash-resident imageをmemory-mapして参照します。
- Table generatorはsource revision、format version、CRC32を記録します。
- Runtimeで大規模tableをSRAMへ全展開しません。

## Relevant files

- `src/Solver/Min2PhaseSolver.*`
- `src/Solver/SolverTableCatalog.*`
- `src/Solver/SolverTableFormat.*`
- `src/Solver/SolverTableStorage.*`
- `tools/generate_min2phase_tables.js`
- `partitions/cube_scrambler_4mb.csv`
