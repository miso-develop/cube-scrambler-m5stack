# Third-Party Notices

## Matt Pocock skills

The following bundled skills are adapted from `mattpocock/skills`:

- `grilling`
- `research`
- `wayfinder`
- `to-spec`
- `to-tickets`
- `implement`
- `handoff`
- `codebase-design`
- `tdd`
- `diagnosing-bugs`
- `code-review`
- `resolving-merge-conflicts`

Upstream repository: https://github.com/mattpocock/skills

The bundled versions are modified for the Loop Engineering GitHub Issue model used by this repository. Planning and implementation use `[Map]` / `[Decision]` / `[Spec]` / `[Task]` work items and durable GitHub Issue / PR handoffs. Project verification is repository-specific; no Loop Verifier infrastructure is bundled here.

The upstream project is distributed under the MIT License:

MIT License

Copyright (c) 2026 Matt Pocock

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Ponytail

The bundled `ponytail` Skill is an adapted derivative of DietrichGebert's `ponytail` Skill.

- Upstream repository: https://github.com/DietrichGebert/ponytail
- Reviewed upstream path: `skills/ponytail/SKILL.md`
- Reviewed upstream revision: `b6c04480c03e8db2f035751d7c46289779ec3362`
- Review decision: `adapt`

The adaptation retains the useful YAGNI / existing-code / standard-library / native-platform / already-installed-dependency / minimum-correct-change decision ladder and root-cause-fix principle.

It intentionally does **not** import upstream plugin hooks, command installers, persistence behavior, global "active every response" semantics, fixed terse-output requirements, or a universal minimal-test rule. Those behaviors would broaden the Skill beyond its responsibility or could conflict with explicit `[Task]` acceptance criteria, project verification, TDD, security, and reporting requirements.

No upstream executable files or dependencies are bundled by this adaptation.

Ponytail is distributed under the MIT License. A copy is included at:

`licenses/PONYTAIL-MIT.txt`
