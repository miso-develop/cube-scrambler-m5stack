---
name: tdd
description: Apply test-driven development with a red-green loop for test-first features or reproducible bug fixes.
metadata:
  version: "1.1"
  source: "adapted from mattpocock/skills"
  license: "MIT"
---

# Test-Driven Development

Identify a stable observable seam, then work one behavior at a time:

1. write the smallest behavioral test;
2. run it and confirm it fails for the expected reason;
3. implement only enough to make it pass;
4. rerun and confirm green;
5. repeat for the next behavior;
6. run project-required verification before declaring the Task complete.

Prefer public/supported boundaries and expected values from independent requirements or known examples. Avoid tests coupled to private implementation details, tautological assertions, speculative batches of tests, and weakening existing tests to obtain green.

Refactor only after behavior is green and within the selected Task scope.
