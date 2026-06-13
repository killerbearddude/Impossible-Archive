# v29.2 Runtime Session Design — In-Memory First

## Restated goal

Introduce the smallest useful persistent runtime concept without adding file/database persistence, GUI/API behavior, background execution, multi-user state, or changes to existing one-shot CLI queries.

The intended user flow is:

```text
Initialize session
Run query
Run query
Run query
End session
Return to shell prompt
```

The session persists one `ArchiveEngineState` in process memory so multiple queries can inspect the same derived civilization/archive state without rebuilding that state for every query.

## Scope boundary

This is a design and implementation-planning document. It does not define a storage format and does not authorize changes to archive generation, artifact insertion, discovery scheduling, draft/review policy, fragment resolution, or public archive mutation.

The first implementation must be opt-in. Existing command-line behavior must remain a one-shot query path unless an explicit session mode is selected.

## Smallest useful version

The smallest useful implementation is a synchronous in-memory session loop over the existing CLI query dispatcher:

```text
Start process
Build one ArchiveEngineState using existing runtime selection
Derive the same advisory/control layers that one-shot CLI derives
Accept query commands from stdin or an equivalent command stream
Execute each query against the same session state
End on explicit end command
Exit process
```

This gives repeated query convenience without introducing persistence or a GUI.

## Inputs and outputs

### Inputs

```text
Initial runtime-selection options:
- runtime mode
- spec file / civilization id when applicable
- fixture id when applicable
- seed/archive-year options that are already valid in the one-shot path

Per-query options:
- query name
- access level
- query-specific IDs and filters
- end-session command
```

### Outputs

```text
- formatted query output using existing formatters
- session status output for initialize/end commands
- usage/runtime errors for invalid per-query commands
```

## Minimal data model

```text
RuntimeSession
- bool active
- ArchiveEngineState state
- ArchiveRuntimeMode runtime_mode
- std::string source_label
- std::vector<std::string> command_history_summary
```

Only `active` and `state` are required for the first implementation. `runtime_mode`, `source_label`, and a compact command-history summary are useful diagnostics, but they must not become persistence.

## Algorithm

### Initialize session

```text
parse session-start options
reuse existing build_runtime_state_for_query flow
build ArchiveEngineState once
derive current advisory/control layers into state
validate full state
if validation fails, return error and do not enter active session
set active = true
print session initialized summary
```

### Run query

```text
while active:
    read one command line
    if command is end-session:
        active = false
        print session ended summary
        break

    parse command using existing CLI option semantics where practical
    reject options that would rebuild runtime state inside the active session unless explicitly allowed later
    execute the requested query against RuntimeSession.state
    print formatted result
```

### End session

```text
clear or destroy RuntimeSession
return control to shell
```

## Query behavior

The first implementation should support read/query inspection commands that already work against an `ArchiveEngineState`. Mutating workflows require explicit review before being allowed in session mode, because repeated commands against the same state change the meaning of rollback, provenance, and validation.

Recommended first allowlist:

```text
summary/list/show/validate/archive-snapshot/control-layer queries
```

Recommended first denylist:

```text
materialize-hidden-cluster
materialize-hidden-mutation-artifact-candidate
any command that intentionally mutates ArchiveEngineState
```

The denylist can be relaxed in a later PR after session mutation semantics are explicit.

## Constraints check

```text
Persistence: memory only.
Concurrency: single-threaded.
Users: single process / single user.
Input format: line-oriented commands are sufficient.
Compatibility: one-shot CLI behavior unchanged.
Determinism: session initialization should produce the same state as the equivalent one-shot query setup.
Failure tolerance: invalid per-query commands must not destroy the active session.
Security/access: existing access gates remain in force per query.
```

## Validation criteria

A v29.2 implementation is acceptable only if:

```text
- existing make test passes
- C++17 test passes
- strict build/test passes
- README smoke workflow passes
- one-shot CLI output remains unchanged for existing smoke-covered queries
- session init builds one state successfully
- two different read-only queries can run against the same session
- invalid query reports an error and leaves the session active
- end-session exits cleanly
- no JSON/file/database persistence is introduced
```

## Test cases

| Test | Input | Expected output | Purpose |
|---|---|---|---|
| Initialize fixed fixture session | start session with fixed fixture | initialized summary with fixed-fixture source | proves state can be built once |
| Run two read-only queries | summary query then list query | both succeed without rebuilding process state | proves repeated query flow |
| Invalid query inside session | unknown query | error output, session remains active | proves failure isolation |
| End session | end-session | ended summary and process exits | proves lifecycle closure |
| Existing one-shot query | existing smoke command | unchanged output pattern | protects compatibility |
| Mutating command denied | materialization command in session | explicit unsupported-in-session error | avoids undefined mutation semantics |

## Implementation plan

### PR 1 — RuntimeSession model and query dispatcher seam

```text
Add RuntimeSession model/API/source files.
Add initialize/run/end functions over ArchiveEngineState.
Route session query execution through existing formatting/query helpers where possible.
Keep mutating commands denied in session mode.
Add minimal self-tests.
No new public archive mutation behavior.
No persistence.
```

### PR 2 — CLI session loop

```text
Add explicit session mode flag or command.
Implement line-oriented Initialize -> Run Query -> End Session loop.
Add README smoke coverage for session init, two read-only queries, invalid query recovery, and end-session.
Keep existing one-shot CLI path unchanged.
```

### PR 3 — Snapshot/smoke/docs reconciliation

```text
Add any needed session diagnostics to smoke coverage.
Document the landed v29.2 baseline.
Do not add storage or GUI behavior.
```

## Non-goals

```text
No file/database persistence.
No JSON state save/load.
No GUI/API layer.
No background worker.
No multi-user server.
No async task system.
No behavior change for existing one-shot CLI queries.
No artifact generation.
No Artifact insertion.
No PublicClaim insertion.
No discovery scheduling.
No hidden truth mutation from session-only code.
No PublicArchive mutation from session-only code.
```
