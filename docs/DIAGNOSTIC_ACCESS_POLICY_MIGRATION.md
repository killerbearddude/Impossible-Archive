# Diagnostic Access Policy

## Current status

The diagnostic/detail access helper is implemented in:

```text
src/diagnostic_access_policy.h
```

The migrated formatter surfaces now route diagnostic/detail access decisions through the helper instead of relying only on direct inline curator checks.

## Current policy shape

The helper intentionally preserves the existing v28 behavior:

```text
Diagnostic detail is curator-or-higher by default.
Public/scholar detail output remains aggregate-only or found:false unless a surface has an explicit public-safe summary exception.
No new access level is introduced.
No public diagnostic ID expansion is introduced.
```

## Migrated surfaces

```text
KnowledgeHorizon finding detail
KnowledgeHorizon finding lists
KnowledgeHorizon validation errors
ContradictionBudget bucket detail
ContradictionBudget bucket lists
ContradictionBudget policy detail
ContradictionBudget validation errors
CandidateArtifactPlan summary/list/detail/validation-error diagnostics
CandidateArtifactPlanEvaluation summary/list/detail/validation-error diagnostics
CandidateArtifactProposal summary/list/detail/validation-error diagnostics
CandidateArtifactProposalAudit summary/list/detail/validation-error diagnostics
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

### Candidate artifact layers

Public and scholar output must continue to preserve these behaviors:

```text
- list output remains aggregate-first
- public-safe detail summaries remain visible only when currently allowed
- hidden source IDs remain restricted
- diagnostic IDs remain restricted
- validation errors remain restricted below curator/debug access
- curator/debug diagnostic detail remains visible
```

## Validation used for migrations

The source-changing migrations were validated before merge with:

```bash
make test
make CXXSTD=c++17 test
make strict
make smoke
```

The smoke workflow includes public-detail blocking checks for KnowledgeHorizon, ContradictionBudget, CandidateArtifactPlan, CandidateArtifactPlanEvaluation, CandidateArtifactProposal, and CandidateArtifactProposalAudit.

## Non-goals

```text
No output expansion.
No runtime behavior change.
No new access level.
No public exposure of diagnostic IDs.
No v29 draft/detail implementation.
```

## Future migration target

The remaining likely helper-based detail access migration candidate is:

```text
ControlLayerAudit entry detail
```

Each future migration should remain behavior-preserving and should keep public/scholar output at least as restrictive as the current formatter behavior.
