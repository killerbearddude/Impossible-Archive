#!/usr/bin/env python3
"""Local read-only GUI wrapper for the Impossible Archive CLI."""

from __future__ import annotations

import argparse
import html
import json
import os
import shlex
import subprocess
import select
import threading
import queue
import sys
import webbrowser
from dataclasses import dataclass
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any


ALLOWED_ACCESS = {"public", "scholar", "curator", "canon", "debug"}
DEFAULT_BINARY = os.environ.get("IA_BIN", "./impossible_archive_mvp_v28_11")


@dataclass(frozen=True)
class CatalogEntry:
    query: str
    label: str
    category: str
    default_access: str
    runtime_modes: tuple[str, ...]
    required_flag: str | None
    example_value: str | None
    argv: tuple[str, ...]

    def to_json(self) -> dict[str, Any]:
        return {
            "query": self.query,
            "label": self.label,
            "category": self.category,
            "default_access": self.default_access,
            "runtime_modes": list(self.runtime_modes),
            "required_flag": self.required_flag,
            "example_value": self.example_value,
            "argv": list(self.argv),
        }


def run_cli(binary: str, args: list[str], timeout: float) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [binary, *args],
        check=False,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


def parse_catalog(text: str) -> list[CatalogEntry]:
    entries: list[dict[str, str]] = []
    current: dict[str, str] | None = None

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("query: "):
            if current is not None:
                entries.append(current)
            current = {"query": line.removeprefix("query: ").strip()}
            continue
        if current is None or ": " not in line:
            continue
        key, value = line.split(": ", 1)
        current[key.strip()] = value.strip()

    if current is not None:
        entries.append(current)

    parsed: list[CatalogEntry] = []
    for entry in entries:
        query = entry.get("query", "")
        argv = tuple(shlex.split(entry.get("argv", "")))
        required_options = entry.get("required_options", "none")
        required_flag = None if required_options == "none" else required_options
        runtime_modes = tuple(
            mode.strip()
            for mode in entry.get("runtime_modes", "fixed-fixture").split(",")
            if mode.strip()
        )
        if not query or "--query" not in argv:
            continue
        query_index = argv.index("--query")
        if query_index + 1 >= len(argv) or argv[query_index + 1] != query:
            continue
        parsed.append(
            CatalogEntry(
                query=query,
                label=entry.get("label", query),
                category=entry.get("category", "Uncategorized"),
                default_access=entry.get("default_access", "public"),
                runtime_modes=runtime_modes,
                required_flag=required_flag,
                example_value=entry.get("example_value"),
                argv=argv,
            )
        )
    return parsed


def load_catalog(binary: str, timeout: float) -> list[CatalogEntry]:
    result = run_cli(binary, ["--query", "gui-query-catalog"], timeout)
    if result.returncode != 0:
        raise RuntimeError(result.stderr or result.stdout or "gui-query-catalog failed")
    entries = parse_catalog(result.stdout)
    if not entries:
        raise RuntimeError("gui-query-catalog returned no parseable entries")
    return entries


def replace_flag_value(argv: list[str], flag: str, value: str) -> None:
    if flag not in argv:
        raise ValueError(f"catalog argv is missing {flag}")
    index = argv.index(flag)
    if index + 1 >= len(argv):
        raise ValueError(f"catalog argv has no value after {flag}")
    argv[index + 1] = value


def build_catalog_command(entry: CatalogEntry, access: str, option_value: str | None) -> list[str]:
    if access not in ALLOWED_ACCESS:
        raise ValueError(f"unsupported access level: {access}")

    argv = list(entry.argv)
    replace_flag_value(argv, "--access", access)

    if entry.required_flag is not None:
        value = (option_value or "").strip()
        if not value:
            raise ValueError(f"{entry.query} requires {entry.required_flag}")
        replace_flag_value(argv, entry.required_flag, value)

    if "--runtime" in argv:
        runtime_index = argv.index("--runtime")
        if runtime_index + 1 >= len(argv) or argv[runtime_index + 1] != "fixed-fixture":
            raise ValueError("prototype wrapper only runs fixed-fixture catalog commands")
    else:
        argv = ["--runtime", "fixed-fixture", *argv]

    return argv


HTML_PAGE = """<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Impossible Archive</title>
  <style>
    :root { color-scheme: light dark; font-family: system-ui, sans-serif; }
    body { margin: 0; }
    header { padding: 0.75rem 1rem; border-bottom: 1px solid #8884; display: flex; gap: 1rem; align-items: center; }
    main { display: grid; grid-template-columns: 22rem 1fr; min-height: calc(100vh - 3.5rem); }
    aside, section { padding: 0.75rem 1rem; overflow: auto; }
    aside { border-right: 1px solid #8884; }
    button, select, input { font: inherit; }
    button { margin: 0.2rem 0.2rem 0.2rem 0; padding: 0.35rem 0.6rem; }
    .controls { display: flex; flex-wrap: wrap; gap: 0.4rem; margin-bottom: 0.75rem; }
    pre { white-space: pre-wrap; border: 1px solid #8884; padding: 0.75rem; min-height: 12rem;
          font-size: 1rem; line-height: 1.45; font-family: ui-monospace, monospace; }
    body { font-size: 15px; }
    button, select, input { font-size: 0.95rem; }
    .list { border: 1px solid #8884; min-height: 6rem; max-height: 16rem; overflow: auto; margin: 0.4rem 0; }
    .list div { padding: 0.3rem 0.5rem; cursor: pointer; }
    .list div:hover, .list div.active { background: #8882; }
    .meta { opacity: 0.75; font-size: 0.9rem; }
    h3 { margin: 0.6rem 0 0.3rem; }
  </style>
</head>
<body>
  <header>
    <strong>Impossible Archive</strong>
    <span class="meta">local only · advisory / read-only · no materialization</span>
    <span class="meta" id="sessionStatus">session…</span>
    <label>access <select id="access">
      <option>public</option><option>scholar</option><option>curator</option><option>canon</option><option>debug</option>
    </select></label>
    <button id="applyAccess">Apply</button>
  </header>
  <main>
    <aside>
      <div class="controls">
        <button id="btnStatus">status</button>
        <button id="btnPressures">pressures</button>
        <button id="btnReport">report</button>
        <button id="btnDrafts">Load drafts</button>
        <button id="btnTexts">texts</button>
      </div>
      <h3>Drafts</h3>
      <div id="draftList" class="list"></div>
      <h3>Generated texts <span class="meta">(advisory, not in archive)</span></h3>
      <div id="textList" class="list"></div>
      <div class="controls">
        <button id="btnShowText">show text</button>
        <button id="btnCopyText">copy</button>
        <button id="btnExportText">export .txt</button>
      </div>
      <h3>Actions</h3>
      <button id="btnShow">show selected</button>
      <button id="btnGenerate">generate selected</button>
      <h3>Text backend</h3>
      <div class="meta" id="backendStatus">backend: template</div>
      <div class="controls">
        <select id="backend">
          <option value="template">template (deterministic)</option>
          <option value="external">external command</option>
        </select>
        <input id="extCmd" placeholder="command that reads stdin, writes stdout" style="flex:1">
        <button id="btnBackend">set</button>
      </div>
      <div class="meta">External command receives the outline on stdin and should write the artifact text to stdout. Falls back to template on failure.</div>
    </aside>
    <section>
      <h3>Output</h3>
      <pre id="output">Ready.</pre>
      <details>
        <summary>Advanced: raw query catalog</summary>
        <div id="catalog"></div>
        <button id="runCatalog">Run selected catalog query</button>
      </details>
    </section>
  </main>
<script>
let catalog = [];
let selectedCatalog = null;
let selectedDraft = null;
let selectedText = null;

async function repl(command) {
  const res = await fetch('/api/repl', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({command})
  });
  const data = await res.json();
  if (data.error) throw new Error(data.error);
  const raw = data.output || '';
  const lines = raw.split('\\n');
  let parsed = null;
  for (let i = lines.length - 1; i >= 0; --i) {
    const s = lines[i].trim();
    if (s.startsWith('{') && s.endsWith('}')) {
      try { parsed = JSON.parse(s); break; } catch (e) {}
    }
  }
  return { text: raw, json: parsed };
}

function setOutput(text) {
  const cleaned = String(text || '').split('\\n').filter(l => {
    const s = l.trim();
    if (s.startsWith('{') && s.endsWith('}') && s.includes('"command"')) return false;
    if (s.includes('] >') && s.indexOf('] >') < 40) return false;
    return true;
  }).join('\\n').trim();
  document.getElementById('output').textContent = cleaned || '(empty)';
}

async function run(cmd) {
  try {
    const res = await repl(cmd);
    setOutput(res.text || '(empty)');
    return res;
  } catch (e) {
    setOutput('error: ' + e);
    return {text: '', json: null};
  }
}

async function refreshStatus() {
  try {
    const s = await fetch('/api/repl/status').then(r => r.json());
    document.getElementById('sessionStatus').textContent = s.alive ? 'session alive' : 'session down';
  } catch (e) {
    document.getElementById('sessionStatus').textContent = 'session ?';
  }
}

document.getElementById('btnStatus').onclick = () => run('status');
document.getElementById('btnPressures').onclick = () => run('pressures');
document.getElementById('btnReport').onclick = () => run('report');

document.getElementById('btnDrafts').onclick = async () => {
  const res = await run('drafts');
  const list = document.getElementById('draftList');
  list.innerHTML = '';
  const items = (res.json && res.json.drafts) ? res.json.drafts : [];
  if (!items.length) {
    const div = document.createElement('div');
    div.textContent = '(no drafts returned — try Load drafts again)';
    list.appendChild(div);
  }
  items.forEach(d => {
    const div = document.createElement('div');
    div.textContent = d.title + '  [' + d.status + ']';
    div.onclick = () => {
      selectedDraft = d.id;
      document.querySelectorAll('#draftList div').forEach(el => el.classList.remove('active'));
      div.classList.add('active');
    };
    list.appendChild(div);
  });
};

document.getElementById('btnTexts').onclick = async () => {
  const res = await run('texts');
  const list = document.getElementById('textList');
  list.innerHTML = '';
  const items = (res.json && res.json.texts) ? res.json.texts : [];
  items.forEach(t => {
    const div = document.createElement('div');
    div.textContent = '[' + t.backend + '] ' + t.title;
    div.onclick = () => {
      selectedText = t.id;
      document.querySelectorAll('#textList div').forEach(el => el.classList.remove('active'));
      div.classList.add('active');
      document.getElementById('btnShowText').click();
    };
    list.appendChild(div);
  });
};

document.getElementById('btnShow').onclick = async () => {
  if (!selectedDraft) { setOutput('Click a draft in the list first (list loads on startup).'); return; }
  await run('show ' + selectedDraft);
};

document.getElementById('btnGenerate').onclick = async () => {
  const level = document.getElementById('access').value;
  if (level !== 'curator' && level !== 'canon' && level !== 'debug') {
    setOutput('generate requires curator, canon, or debug access');
    return;
  }
  if (!selectedDraft) { setOutput('Click a draft in the list first (list loads on startup).'); return; }
  const res = await run('generate ' + selectedDraft);
  // Refresh the list without replacing the readable output
  const listed = await repl('texts');
  const list = document.getElementById('textList');
  list.innerHTML = '';
  const items = (listed.json && listed.json.texts) ? listed.json.texts : [];
  items.forEach(t => {
    const div = document.createElement('div');
    div.textContent = '[' + t.backend + '] ' + t.title;
    div.onclick = () => {
      selectedText = t.id;
      document.querySelectorAll('#textList div').forEach(el => el.classList.remove('active'));
      div.classList.add('active');
      document.getElementById('btnShowText').click();
    };
    list.appendChild(div);
  });
  if (res && res.text) setOutput(res.text);
};

function updateAccessGuards() {
  const level = document.getElementById('access').value;
  const canGenerate = (level === 'curator' || level === 'canon' || level === 'debug');
  const btn = document.getElementById('btnGenerate');
  btn.disabled = !canGenerate;
  btn.title = canGenerate ? 'generate advisory text' : 'requires curator or higher';
}
document.getElementById('access').onchange = updateAccessGuards;
document.getElementById('applyAccess').onclick = async () => {
  await run('access ' + document.getElementById('access').value);
  updateAccessGuards();
  document.getElementById('btnDrafts').click();
};
updateAccessGuards();

document.getElementById('btnBackend').onclick = async () => {
  const b = document.getElementById('backend').value;
  const cmd = document.getElementById('extCmd').value.trim();
  const line = (b === 'external' && cmd) ? ('backend external ' + cmd) : ('backend ' + b);
  await run(line);
  document.getElementById('backendStatus').textContent =
    'backend: ' + (b === 'external' ? ('external' + (cmd ? ' (' + cmd + ')' : '')) : 'template');
};

document.getElementById('btnShowText').onclick = async () => {
  if (!selectedText) { setOutput('select a generated text first'); return; }
  await run('show-text ' + selectedText);
};

document.getElementById('btnCopyText').onclick = async () => {
  if (!selectedText) { setOutput('select a generated text first'); return; }
  const res = await repl('show-text ' + selectedText);
  try {
    await navigator.clipboard.writeText(res.text);
    setOutput('copied\\n\\n' + res.text);
  } catch (e) {
    setOutput('copy failed: ' + e);
  }
};

document.getElementById('btnExportText').onclick = async () => {
  if (!selectedText) { setOutput('select a generated text first'); return; }
  const res = await repl('show-text ' + selectedText);
  const blob = new Blob([res.text], {type: 'text/plain'});
  const a = document.createElement('a');
  a.href = URL.createObjectURL(blob);
  a.download = selectedText + '.txt';
  a.click();
};

async function loadCatalog() {
  try {
    const res = await fetch('/api/catalog');
    const data = await res.json();
    catalog = data.entries || [];
    const box = document.getElementById('catalog');
    catalog.forEach(entry => {
      const btn = document.createElement('button');
      btn.textContent = entry.label || entry.query;
      btn.onclick = () => { selectedCatalog = entry; setOutput('catalog: ' + entry.query); };
      box.appendChild(btn);
    });
  } catch (e) {
    setOutput('catalog load failed: ' + e);
  }
}

document.getElementById('runCatalog').onclick = async () => {
  if (!selectedCatalog) { setOutput('select a catalog query'); return; }
  const access = document.getElementById('access').value;
  const res = await fetch('/api/run', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({query: selectedCatalog.query, access})
  });
  const data = await res.json();
  setOutput(data.stdout || data.error || JSON.stringify(data));
};

refreshStatus();
loadCatalog();
run('format json').then(() => {
  document.getElementById('btnDrafts').click();
});
</script>
</body>
</html>
"""



class ReplSession:
    """Long-lived --repl subprocess. One instance shared by the GUI."""

    def __init__(self, binary: str, timeout: float = 15.0) -> None:
        self.binary = binary
        self.timeout = timeout
        self.proc: subprocess.Popen[str] | None = None
        self.lock = threading.Lock()
        self._start()

    def _start(self) -> None:
        self.proc = subprocess.Popen(
            [self.binary, "--repl"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )

    def send(self, command: str) -> str:
        if self.proc is None or self.proc.poll() is not None:
            self._start()
        assert self.proc is not None and self.proc.stdin is not None and self.proc.stdout is not None
        with self.lock:
            self.proc.stdin.write(command.strip() + "\n")
            self.proc.stdin.flush()
            lines: list[str] = []
            deadline = __import__("time").time() + self.timeout
            while True:
                if __import__("time").time() > deadline:
                    lines.append("\n[gui] timed out waiting for REPL (rebuild the binary so it emits __IA_END__)\n")
                    break
                ready, _, _ = select.select([self.proc.stdout], [], [], 0.05)
                if not ready:
                    continue
                line = self.proc.stdout.readline()
                if not line:
                    break
                if line.strip() == "__IA_END__":
                    break
                lines.append(line)
            return "".join(lines)

    def close(self) -> None:
        if self.proc and self.proc.poll() is None:
            try:
                if self.proc.stdin:
                    self.proc.stdin.write("end\n")
                    self.proc.stdin.flush()
                self.proc.wait(timeout=2)
            except Exception:
                self.proc.kill()


class GuiRequestHandler(BaseHTTPRequestHandler):
    binary: str = DEFAULT_BINARY
    timeout: float = 10.0
    catalog: list[CatalogEntry] = []
    repl: ReplSession | None = None

    def log_message(self, format: str, *args: Any) -> None:
        sys.stderr.write("[gui] " + format % args + "\n")

    def send_json(self, payload: dict[str, Any], status: HTTPStatus = HTTPStatus.OK) -> None:
        body = json.dumps(payload, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self) -> None:  # noqa: N802 - http.server API
        if self.path == "/" or self.path == "/index.html":
            body = HTML_PAGE.encode("utf-8")
            self.send_response(HTTPStatus.OK)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if self.path == "/api/catalog":
            self.send_json({"entries": [entry.to_json() for entry in self.catalog]})
            return
        if self.path == "/api/repl/status":
            alive = self.repl is not None and self.repl.proc is not None and self.repl.proc.poll() is None
            self.send_json({"alive": alive})
            return
        self.send_error(HTTPStatus.NOT_FOUND)

    def do_POST(self) -> None:  # noqa: N802 - http.server API
        if self.path == "/api/repl":
            length = int(self.headers.get("Content-Length", "0"))
            try:
                payload = json.loads(self.rfile.read(length).decode("utf-8"))
                command = str(payload.get("command", "")).strip()
                if not command:
                    raise ValueError("missing command")
                if self.repl is None:
                    raise RuntimeError("repl session not started")
                output = self.repl.send(command)
                self.send_json({"command": command, "output": output})
            except Exception as ex:
                self.send_json({"error": str(ex)}, HTTPStatus.BAD_REQUEST)
            return
        if self.path != "/api/run":
            self.send_error(HTTPStatus.NOT_FOUND)
            return
        length = int(self.headers.get("Content-Length", "0"))
        try:
            payload = json.loads(self.rfile.read(length).decode("utf-8"))
            query = str(payload.get("query", ""))
            access = str(payload.get("access", ""))
            option_value = payload.get("option_value")
            entry = next((item for item in self.catalog if item.query == query), None)
            if entry is None:
                raise ValueError(f"unknown catalog query: {query}")
            argv = build_catalog_command(entry, access, None if option_value is None else str(option_value))
            result = run_cli(self.binary, argv, self.timeout)
            self.send_json(
                {
                    "argv": [self.binary, *argv],
                    "exit_code": result.returncode,
                    "stdout": result.stdout,
                    "stderr": result.stderr,
                }
            )
        except subprocess.TimeoutExpired as ex:
            self.send_json({"error": f"command timed out after {ex.timeout} seconds"}, HTTPStatus.REQUEST_TIMEOUT)
        except Exception as ex:  # Keep local GUI errors visible in the output pane.
            self.send_json({"error": str(ex)}, HTTPStatus.BAD_REQUEST)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Impossible Archive GUI wrapper prototype")
    parser.add_argument("--binary", default=DEFAULT_BINARY, help="path to impossible_archive_mvp_v28_11")
    parser.add_argument("--host", default="127.0.0.1", help="host interface to bind; defaults to local-only")
    parser.add_argument("--port", default=8765, type=int, help="port to bind")
    parser.add_argument("--timeout", default=10.0, type=float, help="CLI command timeout in seconds")
    parser.add_argument("--no-browser", action="store_true", help="do not open the browser automatically")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    binary = args.binary
    catalog = load_catalog(binary, args.timeout)

    GuiRequestHandler.binary = binary
    GuiRequestHandler.timeout = args.timeout
    GuiRequestHandler.catalog = catalog
    repl = ReplSession(binary, timeout=max(args.timeout, 15.0))
    GuiRequestHandler.repl = repl

    server = ThreadingHTTPServer((args.host, args.port), GuiRequestHandler)
    url = f"http://{args.host}:{args.port}/"
    print(f"Impossible Archive GUI wrapper listening on {url}")
    print(f"catalog entries: {len(catalog)}")
    print("Press Ctrl+C to stop.")
    if not args.no_browser:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping GUI wrapper.")
        repl.close()
        return 0
    finally:
        repl.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
