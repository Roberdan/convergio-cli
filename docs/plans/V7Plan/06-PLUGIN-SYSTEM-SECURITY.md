# Convergio V7 — Plugin System & Security Model

**Status:** Draft for approval  
**Date:** 2025-12-26  
**Purpose:** Define plugin trust levels, permissions, and enforcement. Default is deny.

---

## 1) Trust Levels (Hard Line)

- **Untrusted plugins:** **WASM only**.
  - No direct filesystem/network/exec.
  - Access only via audited host calls.
- **Trusted plugins:** signed + reviewed.
  - May include native modules, but only if policy allows.

**Rule:** Marketplace == untrusted by default.

---

## 2) Plugin Manifest (Minimum Fields)

Each plugin ships with `plugin.json`:
- `id`, `name`, `version`, `author`, `license`
- `min_core_version`
- `entrypoint` (wasm module path)
- `capabilities` (agents/tools/ui)
- `permissions` (see catalog below)
- `signing`: `{ signer_id, signature, sha256 }` (required for trusted)

---

## 3) Permission Catalog (Default Deny)

### Filesystem
- `fs.read`: allowlist paths (read-only)
- `fs.write`: allowlist paths (write)

### Network
- `net.http`: allowlist domains + methods
- `net.websocket`: allowlist domains

### Execution
- `proc.exec`: **never allowed for untrusted**; trusted only, allowlist binaries

### Secrets
- `secrets.read`: scoped secret IDs only (never raw env access)

### UI
- `ui.embed`: allow embedding UI components (web)

### Telemetry
- `telemetry.emit`: allow plugin metrics/events (redacted)

**High-risk permissions** (net, write, exec, secrets) require explicit admin approval.

---

## 4) Enforcement Model

### Load-time checks
- verify manifest schema
- verify signature (trusted)
- validate requested permissions
- reject if core version incompatible

### Runtime enforcement
- WASM runtime mediates all host calls.
- Host call layer enforces allowlists.
- Resource limits: CPU time, memory, wall time per call.

### Observability
- log every plugin load/unload, permission denial, crash
- metrics: call latency, errors, resource usage

---

## 5) Secrets Handling (BYOK + Plugins)

- Plugins **never** read process environment.
- Secrets are stored in Azure Key Vault.
- Plugins receive:
  - a short-lived token (scoped to allowed secret IDs), or
  - a brokered call that returns redacted/limited data.

---

## 6) Distribution & Governance

- Official plugins: signed by Convergio release key.
- Marketplace submissions:
  - automated scanning (SBOM + known vulns)
  - manual review if requesting high-risk permissions
- Version upgrades repeat verification.

---

## 7) Performance Considerations

- WASM sandbox adds overhead; mitigate by:
  - batching tool calls,
  - caching plugin metadata,
  - limiting cross-boundary calls.

**Rule:** security is not optional; performance tuning happens after enforcement is correct.

