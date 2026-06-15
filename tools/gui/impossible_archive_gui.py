#!/usr/bin/env python3
"""Local read-only GUI wrapper for the Impossible Archive CLI."""

from __future__ import annotations

import argparse
import html
import json
import os
import shlex
import subprocess
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
<html lang=\"en\">
<head>
  <meta charset=\"utf-8\">
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
  <title>Impossible Archive Explorer</title>
  <style>
    :root { color-scheme: light dark; font-family: system-ui, sans-serif; }
    body { margin: 0; }
    header { padding: 1rem 1.25rem; border-bottom: 1px solid #8884; }
    main { display: grid; grid-template-columns: minmax(18rem, 26rem) 1fr; min-height: calc(100vh - 4rem); }
    aside { border-right: 1px solid #8884; padding: 1rem; overflow: auto; }
    section { padding: 1rem; overflow: auto; }
    button, select, input { font: inherit; }
    button { display: block; width: 100%; text-align: left; margin: 0.25rem 0; padding: 0.45rem 0.6rem; }
    button.active { outline: 2px solid currentColor; }
    label { display: block; margin: 0.75rem 0 0.25rem; font-weight: 600; }
    input, select { width: 100%; box-sizing: border-box; padding: 0.45rem; }
    pre { white-space: pre-wrap; border: 1px solid #8884; padding: 1rem; min-height: 24rem; overflow: auto; }
    .meta { opacity: 0.75; font-size: 0.9rem; }
    .row { margin-bottom: 1rem; }
    .run { margin-top: 1rem; text-align: center; font-weight: 700; }
  </style>
</head>
<body>
  <header>
    <strong>Impossible Archive Explorer</strong>
    <span class=\"meta\">read-only CLI wrapper prototype</span>
  </header>
  <main>
    <aside>
      <label for=\"filter\">Filter</label>
      <input id=\"filter\" placeholder=\"query, label, category\">
      <div id=\"catalog\"></div>
    </aside>
    <section>
      <div class=\"row\"><strong id=\"title\">Select a query</strong></div>
      <div class=\"meta\" id=\"description\"></div>
      <label for=\"access\">Access</label>
      <select id=\"access\">
        <option>public</option>
        <option>scholar</option>
        <option>curator</option>
        <option>canon</option>
        <option>debug</option>
      </select>
      <div id=\"optionBox\"></div>
      <button class=\"run\" id=\"run\">Run selected query</button>
      <label>Output</label>
      <pre id=\"output\">Loading catalog...</pre>
    </section>
  </main>
<script>
let catalog = [];
let selected = null;

function matches(entry, term) {
  const haystack = (entry.query + ' ' + entry.label + ' ' + entry.category).toLowerCase();
  return haystack.includes(term.toLowerCase());
}

function renderCatalog() {
  const term = document.getElementById('filter').value;
  const root = document.getElementById('catalog');
  root.innerHTML = '';
  catalog.filter(entry => matches(entry, term)).forEach(entry => {
    const button = document.createElement('button');
    button.textContent = entry.category + ': ' + entry.label;
    if (selected && selected.query === entry.query) button.classList.add('active');
    button.onclick = () => selectEntry(entry);
    root.appendChild(button);
  });
}

function selectEntry(entry) {
  selected = entry;
  document.getElementById('title').textContent = entry.label;
  document.getElementById('description').textContent = entry.query + ' · ' + entry.category + ' · ' + entry.argv.join(' ');
  document.getElementById('access').value = entry.default_access;
  const optionBox = document.getElementById('optionBox');
  optionBox.innerHTML = '';
  if (entry.required_flag) {
    const label = document.createElement('label');
    label.textContent = entry.required_flag;
    label.setAttribute('for', 'optionValue');
    const input = document.createElement('input');
    input.id = 'optionValue';
    input.value = entry.example_value || '';
    optionBox.appendChild(label);
    optionBox.appendChild(input);
  }
  renderCatalog();
}

async function loadCatalog() {
  const response = await fetch('/api/catalog');
  const data = await response.json();
  catalog = data.entries;
  document.getElementById('output').textContent = 'Loaded ' + catalog.length + ' read-only catalog entries.';
  renderCatalog();
  if (catalog.length) selectEntry(catalog[0]);
}

async function runSelected() {
  if (!selected) return;
  const optionInput = document.getElementById('optionValue');
  const payload = {
    query: selected.query,
    access: document.getElementById('access').value,
    option_value: optionInput ? optionInput.value : null,
  };
  document.getElementById('output').textContent = 'Running...';
  const response = await fetch('/api/run', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(payload),
  });
  const data = await response.json();
  document.getElementById('output').textContent = [
    '$ ' + data.argv.join(' '),
    'exit_code: ' + data.exit_code,
    '',
    data.stdout || '',
    data.stderr ? String.fromCharCode(10) + 'stderr:' + String.fromCharCode(10) + data.stderr : '',
  ].join(String.fromCharCode(10));
}

function boot() {
  document.getElementById('filter').addEventListener('input', renderCatalog);
  document.getElementById('run').addEventListener('click', runSelected);
  loadCatalog().catch(error => {
    console.error(error);
    document.getElementById('output').textContent = 'Catalog load failed:' + String.fromCharCode(10) + error;
  });
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', boot);
} else {
  boot();
}
</script>
</body>
</html>
"""


class GuiRequestHandler(BaseHTTPRequestHandler):
    binary: str = DEFAULT_BINARY
    timeout: float = 10.0
    catalog: list[CatalogEntry] = []

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
        self.send_error(HTTPStatus.NOT_FOUND)

    def do_POST(self) -> None:  # noqa: N802 - http.server API
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
        return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
