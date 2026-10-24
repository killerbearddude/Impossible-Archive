# Model wrapper

`ia_text_wrapper.py` is a stdin/stdout filter for the archive external backend.

## Local Ollama (default)

```bash
# terminal 1
ollama serve
ollama pull llama3.2

# terminal 2 — GUI
python3 tools/gui/impossible_archive_gui.py
```

In the GUI:

- Text backend: **external command**
- Command: `python3 tools/model/ia_text_wrapper.py`
- Click **set**
- Generate a draft

Or in the REPL:

```text
backend external python3 tools/model/ia_text_wrapper.py
generate <draft-id>
```

## Environment

| Variable | Default | Meaning |
|---|---|---|
| `IA_MODEL_URL` | `http://127.0.0.1:11434/api/generate` | Ollama generate endpoint |
| `IA_MODEL_NAME` | `llama3.2` | Model name |
| `IA_MODEL_STYLE` | (empty) | Optional: inscription, ledger, notice, oral |

If the endpoint is down, the wrapper prints the outline and exits 0. The archive does not fail the generate.

## Other backends

Any program that reads stdin and writes stdout can be used the same way. This wrapper is only the Ollama-shaped default.
