---
name: diagnosing-bugs
description: Diagnose hard bugs, regressions, flaky failures, or performance problems with a reproducible feedback loop and falsifiable hypotheses.
metadata:
  version: "1.1"
  source: "adapted from mattpocock/skills"
  license: "MIT"
---

# Diagnosing Bugs

Debug from evidence. Build the tightest practical reproduction of the exact symptom: focused failing test, CLI/HTTP reproduction, deterministic harness, browser/E2E path, differential comparison, or repeated stress loop.

Confirm the reproduction matches the report, minimize it, then form several falsifiable hypotheses. For each hypothesis state a distinguishing prediction and test one variable at a time.

When the cause is established, retain/add a regression test when practical, confirm red before the fix, apply the smallest root-cause fix, confirm green, rerun the original reproduction, and run applicable project verification.

Remove temporary diagnostics before completion. Do not weaken tests or requirements to make the symptom disappear.
