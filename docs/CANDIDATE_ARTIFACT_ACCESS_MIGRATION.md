# Candidate Artifact Diagnostic Access Migration

## Current status

This branch prepares the next behavior-preserving diagnostic access migration for candidate artifact detail surfaces.

The helper enum now includes:

```text
CandidateArtifactPlan
CandidateArtifactPlanEvaluation
CandidateArtifactProposal
CandidateArtifactProposalAudit
```

The source-changing migration is captured in:

```text
scripts/apply_candidate_artifact_diagnostic_access_migration.py
```

## Intended migrated surfaces

```text
CandidateArtifactPlan summary/list/detail/validation-error diagnostics
CandidateArtifactPlanEvaluation summary/list/detail/validation-error diagnostics
CandidateArtifactProposal summary/list/detail/validation-error diagnostics
CandidateArtifactProposalAudit summary/list/detail/validation-error diagnostics
```

## Public/scholar behavior to preserve

```text
Public/scholar list output remains aggregate-first.
Public/scholar detail output remains public-safe summary only when currently visible.
Hidden source IDs remain restricted.
Diagnostic IDs remain restricted.
Validation errors remain restricted below curator/debug access.
Curator/debug diagnostic detail remains visible.
```

## Apply locally

From the repository root on this branch:

```bash
python3 scripts/apply_candidate_artifact_diagnostic_access_migration.py
```

Then validate:

```bash
make test
make CXXSTD=c++17 test
make strict
make smoke
```

If validation passes, commit the resulting source changes back to this branch before merge.

## Non-goals

```text
No output expansion.
No new access level.
No public exposure of diagnostic IDs.
No v29 draft/detail implementation.
No artifact generation.
No proposal materialization.
No persistence.
```
