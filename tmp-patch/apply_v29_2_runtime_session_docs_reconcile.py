#!/usr/bin/env python3
from pathlib import Path


def rw(path, fn):
    p = Path(path)
    s = p.read_text()
    ns = fn(s)
    if ns == s:
        raise SystemExit(f"no changes made to {path}")
    p.write_text(ns)


def once(s, old, new):
    if old not in s:
        raise SystemExit(f"expected text not found: {old[:120]!r}")
    return s.replace(old, new, 1)


rw("FUTURE_VERSIONS.md", lambda s: once(
    s,
    "## Recommended next slices\n\n### v29.2 — Runtime Session Planning, In-Memory First\n\nPurpose: design and then implement the smallest persistent-runtime interface before introducing storage. The first useful target is an in-memory session that can initialize once, run multiple read-only queries against the same state, and end explicitly.",
    "### v29.2 — RuntimeSession Core and CLI Loop, In-Memory Only\n\nImplemented the first opt-in in-memory RuntimeSession path. The session initializes one ArchiveEngineState, accepts line-oriented read-only query commands, rejects invalid or explicitly unsupported commands without ending the session, and exits on an explicit end command.\n\nImplemented surfaces include `src/runtime_session_model.h`, `src/runtime_session_api.h`, `src/runtime_session.cpp`, `--session` / `--runtime-session`, and CLI smoke coverage. Existing one-shot CLI behavior remains the default.\n\n## Recommended next slices\n\n### v29.3 — RuntimeSession Query Coverage and Dispatch Hardening\n\nPurpose: broaden read-only session query coverage only after shared one-shot/session formatting seams are easier to maintain."
).replace(
    "interactive runtime\nGUI/API layer",
    "interactive runtime beyond the current in-memory session loop\nGUI/API layer",
))

rw("docs/CURRENT_STATE_AUDIT.md", lambda s: once(
    once(
        once(
            once(
                once(
                    once(
                        once(
                            s,
                            "# Current State Audit — v29.1 CandidateArtifactDraftReview Baseline",
                            "# Current State Audit — v29.2 RuntimeSession Baseline",
                        ),
                        "The engine is not release-ready. It is at the v29.1 CandidateArtifactDraftReview baseline.",
                        "The engine is not release-ready. It is at the v29.2 RuntimeSession baseline.",
                    ),
                    "v29.1 extends the v29.0 draft-outline baseline with non-mutating `CandidateArtifactDraftReview` records derived from `CandidateArtifactDraft` records.",
                    "v29.2 extends the v29.1 draft-review baseline with an opt-in in-memory `RuntimeSession` path over one initialized `ArchiveEngineState`.",
                ),
                "v29.1     CandidateArtifactDraftReview policy layer, snapshot coverage, and smoke coverage",
                "v29.1     CandidateArtifactDraftReview policy layer, snapshot coverage, and smoke coverage\nv29.2     RuntimeSession core seam and opt-in CLI loop, in-memory only",
            ),
            "| CandidateArtifactDraftReview | Review-policy-only records derived from CandidateArtifactDraft records. Scores outline completeness, traceability, safety, specificity, and revision pressure without enabling artifact prose, insertion, discovery, mutation, persistence, or resolver/composition behavior. |\n| ControlLayerAudit | Audit-only inventory of the control stack. |",
            "| CandidateArtifactDraftReview | Review-policy-only records derived from CandidateArtifactDraft records. Scores outline completeness, traceability, safety, specificity, and revision pressure without enabling artifact prose, insertion, discovery, mutation, persistence, or resolver/composition behavior. |\n| RuntimeSession | Opt-in in-memory session convenience path. Reuses one initialized state for bounded read-only query commands and exits explicitly. No storage or new archive-changing behavior. |\n| ControlLayerAudit | Audit-only inventory of the control stack. |",
        ),
        "The smoke workflow covers representative runtime selection, specs, fragments, fixtures, snapshots, EvidencePotential, KnowledgeHorizon, ContradictionBudget, CandidateArtifactPlan, CandidateArtifactPlanEvaluation, CandidateArtifactProposal, CandidateArtifactProposalAudit, CandidateArtifactDraft, CandidateArtifactDraftReview, and ControlLayerAudit query surfaces, including public-detail blocking checks and expected failures.",
        "The CLI smoke workflow covers representative runtime selection, specs, fragments, fixtures, snapshots, EvidencePotential, KnowledgeHorizon, ContradictionBudget, CandidateArtifactPlan, CandidateArtifactPlanEvaluation, CandidateArtifactProposal, CandidateArtifactProposalAudit, CandidateArtifactDraft, CandidateArtifactDraftReview, ControlLayerAudit, and RuntimeSession query surfaces, including public-detail blocking checks, expected failures, invalid session-query recovery, unsupported session-query rejection, and explicit session end.",
    ),
    "v29.2 — Persistent Runtime Session Planning, In-Memory First",
    "v29.3 — RuntimeSession query coverage and dispatch hardening",
))

rw("docs/ARCHITECTURE_MAP.md", lambda s: once(
    once(
        once(
            once(
                once(
                    once(
                        s,
                        "current v29.1 runtime and control-layer code paths. It describes the implemented repository state after the CandidateArtifactDraftReview policy layer and its snapshot/smoke coverage.",
                        "current v29.2 runtime and control-layer code paths. It describes the implemented repository state after the RuntimeSession core seam and opt-in CLI loop.",
                    ),
                    "CLI options\n-> build_runtime_state_for_query(...)\n-> ArchiveRuntimeMode",
                    "CLI options\n-> one-shot query path or --session path\n-> build_runtime_state_for_query(...)\n-> ArchiveRuntimeMode",
                ),
                "This is an in-memory state object for one CLI invocation. There is no file-backed or database-backed session persistence.",
                "This is an in-memory state object. One-shot CLI mode builds it for one invocation. `--session` mode wraps it in RuntimeSession and reuses it for multiple bounded read-only query commands in the same process. There is no file-backed or database-backed session persistence.",
            ),
            "## 25. ControlLayerAudit",
            "## 25. RuntimeSession in-memory CLI seam\n\nPrimary files:\n\n- `src/runtime_session_model.h`\n- `src/runtime_session_api.h`\n- `src/runtime_session.cpp`\n- `src/cli.cpp`\n- `src/cli_model.h`\n- `scripts/smoke_test_cli_workflows.sh`\n\nFlow:\n\n```text\n--session / --runtime-session\n-> build one ArchiveEngineState through existing runtime selection\n-> initialize RuntimeSession\n-> read line-oriented query commands from stdin\n-> accept bounded read-only query subset\n-> reject unknown or unsupported commands while keeping session active\n-> end on end-session / quit / exit\n```\n\nRuntimeSession is process-local convenience, not storage. The first CLI loop intentionally supports a bounded read-only subset; broadening support should follow dispatch hardening.\n\n## 26. ControlLayerAudit",
        ),
        "## 26. Test and release surfaces",
        "## 27. Test and release surfaces",
    ).replace("## 27. Explicit non-goals in the current architecture", "## 28. Explicit non-goals in the current architecture"),
    "- `scripts/smoke_test_readme_workflows.sh`",
    "- `scripts/smoke_test_cli_workflows.sh`",
))

rw("docs/RUNTIME_SESSION_DESIGN.md", lambda s: once(
    once(
        once(
            once(
                once(
                    s,
                    "# v29.2 Runtime Session Design — In-Memory First",
                    "# v29.2 Runtime Session Design and Landed Baseline — In-Memory First",
                ),
                "Introduce the smallest useful persistent runtime concept without adding file/database persistence, GUI/API behavior, background execution, multi-user state, or changes to existing one-shot CLI queries.",
                "Introduce and land the smallest useful runtime-session concept without adding file/database persistence, GUI/API behavior, background execution, multi-user state, or changes to existing one-shot CLI queries.",
            ),
            "This is a design and implementation-planning document.",
            "This began as a design and implementation-planning document and now records the landed v29.2 baseline.",
        ),
        "The first implementation must be opt-in.",
        "The first implementation is opt-in through `--session` / `--runtime-session`.",
    ),
    "- README smoke workflow passes",
    "- CLI smoke workflow passes",
).replace("### PR 1 — RuntimeSession model and query dispatcher seam", "### PR 1 — RuntimeSession model and query dispatcher seam — landed").replace("### PR 2 — CLI session loop", "### PR 2 — CLI session loop — landed").replace("### PR 3 — Snapshot/smoke/docs reconciliation", "### PR 3 — Docs reconciliation — this slice"))

print("Reconciled docs for v29.2 RuntimeSession baseline.")
