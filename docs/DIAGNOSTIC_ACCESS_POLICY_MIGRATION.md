# Diagnostic Access Policy Migration

## Current status

This repository now has a small helper scaffold for diagnostic/detail access gates:

```text
src/diagnostic_access_policy.h
```

The helper is intentionally narrow. It preserves the current v28 behavior that diagnostic detail is curator-or-higher unless a surface has an explicit public-safe summary rule.

## Intended first migrations

The first formatter migrations should remain behavior-preserving:

```text
KnowledgeHorizon finding detail
ContradictionBudget bucket detail
ContradictionBudget validation policy detail
ContradictionBudget validation errors
```

## Target replacements

### KnowledgeHorizon

In `src/knowledge_horizon.cpp`, replace direct curator checks for finding diagnostics with:

```cpp
can_view_diagnostic_detail(access, DiagnosticDetailSurface::KnowledgeHorizonFinding)
```

Use this for:

```text
format_knowledge_horizon_summary
format_knowledge_horizon_validation
format_knowledge_horizon_findings
format_knowledge_horizon_finding_detail
```

The public/scholar behavior must remain:

```text
- aggregate-only list output
- found: false for hidden/inaccessible finding detail
- no hidden IDs or explanations below curator/debug access
```

### ContradictionBudget

In `src/contradiction_budget.cpp`, replace direct curator checks for bucket/policy diagnostics with:

```cpp
can_view_diagnostic_detail(access, DiagnosticDetailSurface::ContradictionBudgetBucket)
can_view_diagnostic_detail(access, DiagnosticDetailSurface::ContradictionBudgetPolicy)
can_view_diagnostic_detail(access, DiagnosticDetailSurface::ValidationErrors)
```

For public/scholar bucket detail, preserve the existing archive-summary exception with:

```cpp
can_view_contradiction_budget_public_bucket_summary(access, bucket_id)
```

The public/scholar behavior must remain:

```text
- archive bucket summary remains visible
- non-archive bucket detail returns found: false
- reason codes, representative contradiction IDs, hidden causes, policy thresholds, and notes remain restricted
```

## Validation requirements

After migration, run:

```bash
make test
make CXXSTD=c++17 test
make strict
make smoke
```

At minimum, the affected translation units should compile with:

```bash
g++ -std=c++20 -Wall -Wextra -pedantic -O0 -c src/knowledge_horizon.cpp
g++ -std=c++20 -Wall -Wextra -pedantic -O0 -c src/contradiction_budget.cpp
```

## Non-goals

```text
No output expansion.
No runtime behavior change.
No new access level.
No public exposure of diagnostic IDs.
No v29 draft/detail implementation.
```
