---
name: diagnosing-bugs
description: Diagnose hard bugs, regressions, flaky failures, or performance problems with a reproducible feedback loop and falsifiable hypotheses.
metadata:
  version: "1.2"
  source: "adapted from mattpocock/skills"
  license: "MIT"
---

# Diagnosing Bugs

Debug from evidence. Build the tightest practical reproduction of the exact symptom: focused failing test, CLI/HTTP reproduction, deterministic harness, browser/E2E path, differential comparison, or repeated stress loop.

Confirm the reproduction matches the report, minimize it, then form several falsifiable hypotheses. For each hypothesis state a distinguishing prediction and test one variable at a time.

When the cause is established, retain/add a regression test when practical, confirm red before the fix, apply the smallest root-cause fix, confirm green, rerun the original reproduction, and run applicable project verification.

Before completion:

- remove temporary diagnostic output and throwaway harnesses unless intentionally retained;
- state the confirmed root cause and evidence;
- record a materially new recurring failure in `troubleshooting-cases` when it provides reusable prevention value.

Do not weaken tests, requirements, project verification, or security controls to make the symptom disappear.
