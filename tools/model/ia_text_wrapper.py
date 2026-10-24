#!/usr/bin/env python3
"""Stdin/stdout text wrapper for the Impossible Archive external backend.

Reads the draft outline on stdin. Writes artifact text on stdout.
Configure with environment variables:

  IA_MODEL_URL     default http://127.0.0.1:11434/api/generate  (Ollama)
  IA_MODEL_NAME    default llama3.2
  IA_MODEL_STYLE   inscription | ledger | notice | oral   (optional hint)

If the endpoint is unreachable, prints a short fallback and exits 0
so the archive still receives text.
"""

from __future__ import annotations

import json
import os
import sys
import urllib.error
import urllib.request


def build_prompt(outline: str) -> str:
    style = os.environ.get("IA_MODEL_STYLE", "").strip()
    hint = f" Write it as a {style}." if style else ""
    return (
        "You are writing a short in-world document for a fictional archive.\n"
        "Use only the facts in the outline. Do not mention outlines, drafts, "
        "or that you are an AI. Keep it under 180 words."
        f"{hint}\n\n"
        "Outline:\n"
        f"{outline.strip()}\n"
    )


def call_ollama(prompt: str) -> str:
    url = os.environ.get("IA_MODEL_URL", "http://127.0.0.1:11434/api/generate")
    model = os.environ.get("IA_MODEL_NAME", "llama3.2")
    body = json.dumps({"model": model, "prompt": prompt, "stream": False}).encode()
    req = urllib.request.Request(
        url,
        data=body,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=60) as resp:
        payload = json.loads(resp.read().decode())
    text = payload.get("response") or payload.get("text") or ""
    return text.strip()


def main() -> int:
    outline = sys.stdin.read()
    if not outline.strip():
        print("(empty outline)")
        return 0
    prompt = build_prompt(outline)
    try:
        text = call_ollama(prompt)
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError, OSError) as ex:
        print(f"[wrapper fallback: {ex}]")
        print(outline.strip())
        return 0
    if not text:
        print(outline.strip())
        return 0
    print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
