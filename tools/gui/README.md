# Impossible Archive GUI

**Packaging decision:** local Python HTTP server (current).  
A native UI is deferred. The server drives one long-lived `--repl` subprocess and does not link against engine internals.

## Launch

From the repository root:

```bash
make build
python3 tools/gui/impossible_archive_gui.py
```

Open http://127.0.0.1:8765/

- Binds to 127.0.0.1 by default.
- Advisory / read-only. No materialization controls.
- Writer commands (status, drafts, generate, texts, backend, access) use the REPL bridge.
- Catalog queries remain available under Advanced.

Optional:

```bash
python3 tools/gui/impossible_archive_gui.py --binary ./impossible_archive_mvp_v28_11 --port 8765 --no-browser
```


# Impossible Archive GUI wrapper prototype

This directory contains a minimal local GUI wrapper for the existing
`impossible_archive_mvp_v28_11` CLI binary.

The wrapper intentionally does not link against engine internals. It discovers
GUI-safe actions by running:

```bash
./impossible_archive_mvp_v28_11 --query gui-query-catalog
```

It then renders those catalog entries in a browser and runs only commands derived
from catalog-emitted `argv` lines. Commands are executed with Python's
`subprocess.run(argv, shell=False)`, so the UI never builds shell command strings.

## Run

From the repository root:

```bash
make build
python3 tools/gui/impossible_archive_gui.py
```

Then open:

```text
http://127.0.0.1:8765/
```

Optional binary override:

```bash
python3 tools/gui/impossible_archive_gui.py --binary ./impossible_archive_mvp_v28_11
```

## Current scope

- local-only browser UI
- fixed-fixture runtime by default
- read-only catalog entries only
- no persistence
- no mutation commands
- no external Python packages
- no shell command construction

The engine CLI remains the source of truth for all archive behavior.
