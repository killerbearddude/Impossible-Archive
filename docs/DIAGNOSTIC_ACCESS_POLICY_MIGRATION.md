# Diagnostic Access Policy

## Current status

The diagnostic/detail access helper is implemented in:

```text
src/diagnostic_access_policy.h
```

The initial formatter migration is complete. `KnowledgeHorizon` and `ContradictionBudget` diagnostic/detail formatters now route the migrated access decisions through the helper instead of relying only on direct inline curator checks.

## Current policy shape

The helper intentionally preserves the existing v28 behavior:

```text
Diagnostic detail is curator-or-higher by default.
Public/scholar detail output remains aggregate-only or found:false unless a surface has an explicit public-safe summary exception.
No new access level is introduced.
No public diagnostic ID expansion is introduced.
```

## Migrated surfaces

The first migrated formatter surfaces are:

```text
KnowledgeHorizon finding detail
KnowledgeHorizon finding lists
KnowledgeHorizon validation errors
ContradictionBudget bucket detail
ContradictionBudget bucket lists
ContradictionBudget policy detail
ContradictionBudget validation errors
```

## Public/scholar behavior preserved

### KnowledgeHorizon

Public and scholar output must continue to preserve these behaviors:

```text
- aggregate-only list output
- found:false for hidden/inaccessible finding detail
- no hidden IDs below curator/debug access
- no explanations below curator/debug access
```

### ContradictionBudget

Public and scholar output must continue to preserve these behaviors:

```text
- archive bucket summary remains visible
- non-archive bucket detail returns found:false
- reason codes remain restricted
- representative contradiction IDs remain restricted
- hidden causes remain restricted
- policy thresholds remain restricted
- diagnostic notes remain restricted
```

The archive-bucket public/scholar summary exception is represented by:

```cpp
can_view_contradiction_budget_public_bucket_summary(access, bucket_id)
```

## Validation used for migration

The source-changing migration was validated locally before merge with:

```bash
make test
make CXXSTD=c++17 test
make strict
make smoke
```

The smoke workflow included the existing public-detail blocking checks for KnowledgeHorizon and ContradictionBudget.

## Non-goals

```text
No output expansion.
No runtime behavior change.
No new access level.
No public exposure of diagnostic IDs.
No v29 draft/detail implementation.
```

## Future migration targets

The next likely candidates for helper-based detail access migration are:

```text
CandidateArtifactPlan detail
CandidateArtifactPlanEvaluation detail
CandidateArtifactProposal detail
CandidateArtifactProposalAudit detail
ControlLayerAudit entry detail
```

Each future migration should remain behavior-preserving and should keep public/scholar output at least as restrictive as the current formatter behavior.
