# ADR-035: CLI `--human` Flag for Readable Output

**Status:** Accepted
**Date:** 2026-04-07

## Context

The `cvg` CLI outputs raw JSON by default, which is ideal for scripting and
agent consumption but unreadable for human operators. Several commands already
had a per-command `--human` flag, but its implementation (`serde_json::to_string_pretty`)
still produced JSON — just indented.

## Decision

1. **Global `--human` / `-H` flag** added to the top-level `Cli` struct with
   `#[arg(global = true)]`. This makes it available on every subcommand without
   modifying individual command enums.

2. **`human_output.rs` module** provides formatting helpers:
   - `format_human(val)` — auto-detects arrays vs objects vs wrapped lists
   - `format_list(arr, columns)` — explicit column selection
   - `format_status(val)` — health/status endpoints with icons
   - `format_single(map)` — key-value pairs

3. **Global atomic flag** (`OnceLock`-style `AtomicBool`) set once in `main.rs`
   before dispatch. The `print_value()` helper in `cli_http.rs` checks both the
   per-command `human` parameter and the global flag.

4. **No external dependencies** — formatting uses simple padded columns with
   ANSI escape codes, consistent with existing CLI style (doctor, status).

5. **Backwards compatible** — without `-H`, output remains raw JSON. Existing
   per-command `--human` flags continue to work.

## Consequences

- Human operators can run `cvg -H plan list` for readable tables.
- Scripts using `cvg plan list | jq ...` are unaffected.
- New commands automatically get human output through `print_value()`.
- Column detection is heuristic (prioritizes id/name/status fields); specific
  commands can use `format_list()` with explicit columns if needed.
