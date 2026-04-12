# ADR-002: Security Audit Findings and Fixes

**Status:** Accepted
**Date:** 2025-07-26
**Deciders:** Roberto D'Angelo, Security Audit (Copilot)

## Context

The convergio-cli crate (16,404 LOC) was subjected to a comprehensive security
audit prior to its first public release. As a CLI that executes HTTP calls with
bearer tokens and writes secrets to disk, the attack surface is non-trivial.

## Findings and Fixes

### HIGH — SSRF / Auth Token Exfiltration
- **Issue:** `with_auth()` attached `CONVERGIO_AUTH_TOKEN` to any URL, including
  user-supplied `--api-url` values pointing at attacker-controlled hosts.
- **Fix:** `with_auth()` now validates the URL via `validate_daemon_url()` — only
  loopback addresses and HTTPS URLs receive the auth header.

### HIGH — Path Traversal via Project Names
- **Issue:** `project_output_dir()` joined unsanitized user input (`../`) into
  filesystem paths, enabling writes outside `~/.convergio/projects/`.
- **Fix:** Added `validate_identifier()` to reject `..`, `/`, `\`, and other
  unsafe characters in project names and IDs. Applied to `project create`,
  `audit project`, and all callers of `project_output_dir()`.

### HIGH — Path Traversal in Skill Transpile Output
- **Issue:** YAML `name` field from skill files was used directly in output
  filenames without sanitization.
- **Fix:** Added `sanitize_filename()` that strips path components and replaces
  unsafe characters. Applied to all transpile output paths.

### HIGH — Secrets Stored Without File Permissions
- **Issue:** `~/.convergio/env` (containing API keys, tokens) was written with
  default umask permissions (typically 0644, world-readable).
- **Fix:** Added `write_secret_file()` that creates files with mode 0600 on Unix.
  Applied to all secret-writing paths: env file, Telegram token, backups.

### HIGH — Unsafe Remote Env Import
- **Issue:** `mesh join` blindly merged arbitrary env vars from a coordinator
  response into local env, enabling injection of `PATH`, `LD_PRELOAD`, etc.
- **Fix:** Added `is_allowed_env_key()` allowlist. Only known Convergio/API keys
  are accepted from remote sources.

### HIGH — No HTTP Client Timeouts
- **Issue:** All HTTP calls used `reqwest::Client::new()` with no timeout,
  no redirect limits, and default settings.
- **Fix:** Centralized `hardened_http_client()` with 30s request timeout,
  10s connect timeout, 5-redirect limit. All 20+ call sites migrated.

### MEDIUM — Remote Shell Execution (curl | sh)
- **Issue:** Tailscale Linux install ran `curl | sh` from a remote URL.
- **Fix:** Replaced with package-manager-based install (apt/dnf) which provides
  supply-chain verification via signed packages.

### MEDIUM — Error Messages Leak Sensitive Data
- **Issue:** Error messages included raw URLs, response bodies, and internal
  paths that could contain tokens or sensitive info.
- **Fix:** Error messages in `post_and_return` and `get_and_return` now show
  only HTTP status codes, not raw URLs or response bodies.

### LOW — Loose HMAC Config Parsing
- **Issue:** `compute_mesh_signature()` matched any key starting with
  `shared_secret`, not just the exact key.
- **Fix:** Strict parsing with `strip_prefix("shared_secret")` + `strip_prefix('=')`.

## New Module

All security utilities are centralized in `src/security.rs` (≈175 LOC):
- `validate_identifier()` — safe identifiers for path construction
- `sanitize_filename()` — safe filenames from external sources
- `validate_daemon_url()` — SSRF prevention for auth-bearing requests
- `validate_path_containment()` — canonical path containment check
- `is_allowed_env_key()` — env import allowlist
- `hardened_http_client()` — timeout + redirect-limited HTTP client
- `write_secret_file()` — restrictive-permission file writes

All functions have unit tests.

## Files Modified (18)

- `src/security.rs` — NEW
- `src/lib.rs` — added security module
- `src/cli_http.rs` — hardened client, safe auth, redacted errors
- `src/paths.rs` — identifier validation in path construction
- `src/cli_mesh_join.rs` — env allowlist, hardened client, safe perms
- `src/cli_setup_steps.rs` — secret file perms, hardened client
- `src/cli_setup_telegram.rs` — secret file perms, hardened client
- `src/cli_setup_inference.rs` — backup file perms
- `src/cli_setup_service.rs` — replaced curl|sh with package manager
- `src/cli_skill_transpile.rs` — filename sanitization
- `src/cli_project_handlers.rs` — identifier validation, hardened client
- `src/cli_audit_project.rs` — identifier validation, hardened client
- `src/cli_ops.rs` — strict HMAC parsing, hardened client
- `src/cli_launch.rs` — hardened client
- `src/cli_build.rs` — hardened client
- `src/cli_bus_watch.rs` — hardened client
- `src/cli_newproject.rs` — hardened client
- `src/cli_preflight_mutations.rs` — hardened client
- Plus 5 more: cli_ask, cli_org, cli_report, cli_agent_format, cli_skill_*

## Decision

All HIGH and MEDIUM findings are fixed. The `security.rs` module provides a
centralized, tested foundation for future security work.

## Consequences

- Auth tokens are no longer sent to untrusted URLs
- Secret files have restrictive permissions by default
- All HTTP calls have bounded timeouts
- Path traversal is blocked at input validation layer
- Remote env import is restricted to known keys
