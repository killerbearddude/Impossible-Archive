# Impossible Archive

Impossible Archive is a C++ prototype for building and inspecting a deterministic fictional archive engine. It models hidden truth, public archive records, contradiction pressure, evidence potential, candidate artifact planning, draft review, and in-memory runtime-session seams without treating those advisory layers as permission to mutate the archive.

The current repository is an implementation prototype, not a release-ready application.

## Current focus

The current development line is v29.2. The active focus is an in-memory runtime session path that can initialize one archive state, run multiple read-only queries against that state, and end explicitly.

The current session work is intentionally bounded:

```text
Initialize session
Run read-only query
Run read-only query
End session
```

It does not add file/database persistence, a GUI/API layer, background workers, multi-user behavior, artifact generation, discovery scheduling, or new public archive mutation behavior.

## Build and test

```bash
make test
make CXXSTD=c++17 test
make strict
make smoke
```

For the full local release gate:

```bash
make release-check
```

## Common commands

```bash
./impossible_archive_mvp_v28_11 --self-test
./impossible_archive_mvp_v28_11 --query archive-snapshot
./impossible_archive_mvp_v28_11 --query candidate-artifact-draft-review-summary
./impossible_archive_mvp_v28_11 --query control-layer-audit-summary
```

## Documentation map

- `docs/CLI_WORKFLOW_REFERENCE.md` — detailed CLI workflow and milestone reference formerly held in `README.md`.
- `docs/RUNTIME_SESSION_DESIGN.md` — v29.2 in-memory runtime session design.
- `docs/CURRENT_STATE_AUDIT.md` — current implemented baseline and architectural readiness audit.
- `docs/ARCHITECTURE_MAP.md` — module and control-layer architecture map.
- `FUTURE_VERSIONS.md` — roadmap and staged future work.
- `RELEASE_CHECKLIST.md` — release validation checklist.

## Repository status

The engine remains deliberately conservative. Advisory records such as candidate plans, proposals, audits, drafts, and draft reviews are inspectable and validateable, but they do not by themselves generate artifacts, insert claims, schedule discoveries, persist state, or mutate the public archive.
