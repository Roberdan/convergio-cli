# Convergio V7 — Ops, SLOs, Observability, Runbooks

**Status:** Draft for approval  
**Date:** 2025-12-26

---

## 1) Initial SLOs (Targets)

- **Availability:** 99.5% API uptime
- **Latency:**
  - P95 HTTP < 800 ms (non-stream)
  - P95 first-token (WS) < 1.5 s
- **Error budget:** alert if 25% consumed in 1 day

---

## 2) Golden Signals (Must Exist)

### Metrics
- request count / success / error by endpoint
- latency histograms (P50/P95/P99)
- provider failover counts
- queue depth + worker utilization
- cost (managed): $/tenant/day, $/model/day
- voice: connect time, dropout rate, reconnect success

### Logs
Structured JSON with:
- request_id, tenant_id, user_id
- provider/model, mode (byok|managed)
- tokens/tools/audio seconds
- latency_ms, error_code

### Traces
OpenTelemetry spans:
- auth → metering → provider call → tool calls → stream

---

## 3) Cost Guardrails (Ops + Billing)

- Global daily spend cap (managed lane)
- Per-tenant budgets:
  - alert at 80%
  - enforce downgrade at 90%
  - block at 110% unless admin override

**Rule:** throttle before provider calls.

---

## 4) Deployment Strategy (Azure)

- Canary rollout for gateway and policy changes.
- Feature flags for:
  - routing policy
  - billing/metering behavior
  - voice mode
  - plugin permissions

---

## 4.1) Code Quality & Security Gates

### Pre-commit hooks (mandatory)
All commits must pass:
- **Formatting:** `rustfmt` (Rust), `prettier` (TS/Svelte)
- **Linting:** `clippy` (Rust), `eslint` (TS)
- **Secrets detection:** `gitleaks` or `detect-secrets`
- **Type checking:** `cargo check`, `tsc --noEmit`

### CI/CD pipeline gates
- **SAST (Static Application Security Testing):**
  - `cargo audit` for Rust dependencies
  - `npm audit` / `snyk` for Node dependencies
  - CodeQL or Semgrep for code scanning
- **Test coverage threshold:** **90% minimum** for merged PRs
  - Block merge if coverage drops below threshold
  - Enforce per-crate and per-package coverage
- **Build verification:** all targets must compile (macOS, Linux, WASM)

### Security scanning cadence
- Dependency audit: daily in CI
- Full SAST scan: on every PR
- Container image scan: on every deploy

---

## 5) Backups & DR

- PostgreSQL: daily snapshots + PITR; test restore monthly
- Redis: cache by default; persistence only if required
- Key Vault: access policies + rotation policy

Initial DR targets:
- **RPO:** 24h
- **RTO:** 24h

---

## 6) Runbooks (Minimum Set)

### RB-1 High error rate
- detect: 5xx > 2% for 5 min
- triage: recent deploy? provider outage? auth failures?
- mitigate: rollback, force cheaper provider, disable feature flag

### RB-2 Latency regression
- detect: P95 > SLO for 10 min
- triage: queue depth, DB slow, provider slow
- mitigate: scale workers, lower model, disable tools

### RB-3 Cost overrun triggered
- detect: spend > 80% daily cap
- triage: top tenants/models/tools
- mitigate: enforce downgrade, block managed lane for new tenants

### RB-4 Voice pipeline broken
- detect: dropout > 5% / 10 min
- triage: WS errors, browser permissions, codec mismatch
- mitigate: fall back to text, push-to-talk, disable voice flag

### RB-5 DB outage/restore
- detect: DB errors/replication lag
- triage: failing queries, connection pool, region issues
- mitigate: read-only mode, restore snapshot, comms template

---

## 7) Compliance Ops

- audit logs for admin actions
- quarterly review of access, budgets, retention
- documented incident response and disclosure process

