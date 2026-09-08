---
name: grilling
description: Stress-test a plan, decision, or design through structured questioning before implementation.
metadata:
  version: "1.0"
  source: "adapted from mattpocock/skills"
  license: "MIT"
---

# Grilling

Map the subject as a decision tree and ask only questions on the current frontier whose prerequisites are already settled. Number questions and include a recommended answer. Recompute the frontier after each decision.

Facts are the agent's job: inspect repository, tools, documentation, and evidence instead of asking the user for discoverable facts. Material product, architecture, risk, cost, compatibility, and operational trade-offs belong to the user.

Do not grill routine reversible details, repeat answered questions, or ask downstream questions while prerequisites remain unresolved. Surface contradictions and hidden assumptions explicitly. Do not begin implementation until material decisions are sufficiently resolved.
