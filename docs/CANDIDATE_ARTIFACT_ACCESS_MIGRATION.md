# Candidate Artifact Diagnostic Access Migration

## Current status

The candidate artifact diagnostic access migration is complete.

The helper enum includes:

```text
CandidateArtifactPlan
CandidateArtifactPlanEvaluation
CandidateArtifactProposal
CandidateArtifactProposalAudit
```

The formatter surfaces now route migrated diagnostic/detail visibility decisions through:

```cpp
can_view_diagnostic_detail(access, DiagnosticDetailSurface::<surface>)
```

The temporary local application script has been removed after the source-changing migration was applied and merged.

## Migrated surfaces

```text
CandidateArtifactPlan summary/list/detail/validation-error diagnostics
CandidateArtifactPlanEvaluation summary/list/detail/validation-error diagnostics
CandidateArtifactProposal summary/list/detail/validation-error diagnostics
CandidateArtifactProposalAudit summary/list/detail/validation-error diagnostics
```

## Public/scholar behavior preserved

```text
Public/scholar list output remains aggregate-first.
Public/scholar detail output remains public-safe summary only when currently visible.
Hidden source IDs remain restricted.
Diagnostic IDs remain restricted.
Validation errors remain restricted below curator/debug access.
Curator/debug diagnostic detail remains visible.
```

## Validation used for migration

The source-changing migration was validated before merge with:

```bash
make test
make CXXSTD=c++17 test
make strict
make smoke
```

The smoke workflow includes public-detail blocking checks for:

```text
CandidateArtifactPlan
CandidateArtifactPlanEvaluation
CandidateArtifactProposal
CandidateArtifactProposalAudit
```

## Non-goals

```text
No output expansion.
No runtime behavior change.
No new access level.
No public exposure of diagnostic IDs.
No v29 draft/detail implementation.
No artifact generation.
No proposal materialization.
No persistence.
```

## Remaining related target

The remaining diagnostic-access migration candidate is:

```text
ControlLayerAudit entry detail
```

Any future migration should remain behavior-preserving and should keep public/scholar output at least as restrictive as the current formatter behavior.
