# Convergio V7 — Migration Plan (V6 → V7)

**Status:** Draft for approval  
**Date:** 2025-12-26  
**Goal:** Upgrade users safely with rollback and data preservation.

---

## 1) Scope

Artifacts:
- conversations, memory DB, settings
- BYOK keys (must be migrated carefully)
- plugin state

Surfaces:
- CLI, web (new), macOS app

Environments:
- local, self-hosted, SaaS (Azure)

---

## 2) Migration Phases

### Phase M1 — Inventory
- Extract V6 schemas and data locations.
- Map fields to V7 entities (`tenant`, `user`, `usage_events`, `conversations`).

### Phase M2 — Tools
Provide CLI tools:
- `convergio migrate export` → JSONL + checksums
- `convergio migrate import` → Postgres/Redis
- dry-run mode + error report

### Phase M3 — Compatibility
- Keep key CLI commands working via the API Gateway.
- Versioned API paths (`/v1` legacy shim).

### Phase M4 — Rollout
- dogfood → opt-in beta → staged rollout
- per-tenant flag to stay on V6 until migration is verified

---

## 3) SaaS Migration Strategy (Azure)

- Snapshot old store.
- Export artifact signed + hashed.
- Import idempotently to Postgres.
- Verify counts and checksums.
- Switch reads to V7.
- Keep V6 read-only fallback for 7 days.

---

## 4) Rollback Plan

- Preserve V6 snapshots for 30 days.
- If P0/P1 issues:
  - flip read pointer back to V6
  - disable V7 writes
  - reconcile diffs later

---

## 5) Metrics

- migration success rate
- mismatch count
- duration per tenant
- rollback rate
- post-migration crash rate and support tickets

