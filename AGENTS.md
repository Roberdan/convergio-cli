# Repository Guidelines

## Project Structure & Module Organization
- `src/` holds core modules: `core/` (entry, config), `orchestrator/` (Ali registry, cost, bus), `agents/` (definitions, embedded generator), `intent/`, `runtime/`, `neural/`, `metal/`, `memory/`, `tools/`. Headers live in `include/`.
- `shaders/similarity.metal` contains the Metal compute kernel; outputs land in `build/similarity.metallib`.
- `scripts/` includes build helpers such as `embed_agents.sh`; `data/` stores notes/knowledge artifacts created at runtime (do not commit secrets).
- `tests/` houses C test harnesses; `build/` and `dist/` are generated outputs; `docs/` and `Formula/` contain user docs and Homebrew packaging.

## Build, Test, and Development Commands
- `make` builds the release binary to `build/bin/convergio` (auto-generates embedded agents and Metal libs when the toolchain is present).
- `make debug` enables sanitizers; `make run` builds and runs; `make clean` removes artifacts.
- `make fuzz_test` compiles and executes the fuzz harness in `tests/fuzz_test.c`; `make dist` creates a tarball; `make install`/`make uninstall` manage `/usr/local` installs (requires sudo).
- CMake flow: `cmake -S . -B build && cmake --build build` to compile; `ctest --output-on-failure --test-dir build` runs `test_fabric`.

## Coding Style & Naming Conventions
- C17/ObjC17 with 4-space indents, K&R braces, and no tabs. Keep clang warnings clean.
- Naming: functions `snake_case`, types `PascalCase`, constants `UPPER_SNAKE_CASE`, globals prefixed `g_`; guard headers with `#ifndef/#define/#endif`.
- Prefer `//` for single-line comments; free allocations you own; keep CLI/help text succinct.
- Agent definitions live in `src/agents/definitions/*.md`; update frontmatter and run `scripts/embed_agents.sh` (or rely on `make` auto-generation) when adding agents.

## Testing Guidelines
- Add new C tests under `tests/` and register them in `CMakeLists.txt` (for `ctest`) or a Make target; keep names `test_*` and use the existing `TEST`/`RUN_TEST` macros for consistency.
- Run `cmake --build build && ctest --output-on-failure --test-dir build` after configuring, plus `make fuzz_test` when touching auth/security or parser paths.
- Aim for deterministic assertions; include boundary cases (null inputs, large node counts, malformed tool payloads).

## Commit & Pull Request Guidelines
- Follow conventional commits: `type(scope): description` (e.g., `fix(memory): guard null embeddings`, `docs(readme): clarify setup`).
- PRs should summarize behavior changes, link issues, list tests run (hardware/OS), and attach screenshots when CLI output/ANSI formatting changes.
- Keep docs updated when adding commands or agents; ensure builds pass on Apple Silicon with required env vars (`ANTHROPIC_API_KEY`, optional `CLAUDE_MAX`) and avoid committing runtime artifacts from `data/` or `build/`.
