---
name: codebase-design
description: Use when designing or changing module boundaries, public interfaces, abstractions, dependency seams, or test seams. Prefer small interfaces that hide complexity and avoid speculative layers.
metadata:
  version: "1.0"
---

# Codebase Design

Use when the shape of the code is itself a decision, not for ordinary localized implementation.

Identify callers and observable behavior that must remain stable, where complexity currently leaks, and the narrowest interface that can own that complexity.

Introduce a seam only for real variation or isolation pressure: multiple concrete implementations, external integration, deterministic testing, or a security/process boundary. Do not add interfaces merely because future variation is imaginable.

Prefer dependencies supplied at an existing composition boundary when substitution is required; avoid dependency-injection infrastructure when a parameter or constructor is sufficient.

Treat the public interface as the default test surface. If two designs are materially plausible, compare interface size, locality, testability, and migration cost.

Existing repository conventions and explicit requirements override this Skill. Avoid unrelated architecture cleanup.

Adapted from `mattpocock/skills` `codebase-design`.
