# V7 Migration Plan (v6 → v7)

Goal: upgrade users from v6 to v7 safely, with rollback and data preservation.

## Scope
- Artifacts: conversations, memory DB, settings, API keys, plugin state.
- Surfaces: CLI, Web (new), Native Mac.
- Environments: local, self-hosted, SaaS.

## Phases
1) Inventory & schema map  
- Export v6 SQLite schemas (memory, settings).  
- Define v7 PostgreSQL/Redis schemas (owners, tenants, quotas).  
- Map fields: conversations, embeddings, settings, agent prefs, licenses.

2) Migration tools  
- Build `convergio migrate export` (v6 → JSON/CSV/ndjson).  
- Build `convergio migrate import` (JSON → v7 PostgreSQL/Redis).  
- Add dry-run and checksum.  
- Add progress + error report.

3) Compatibility layer  
- Keep v6 CLI commands working via thin shim calling API Gateway.  
- Maintain REPL defaults; warn on deprecated flags.  
- Versioned API: `/v1` for legacy, `/v2` for new features.

4) Rollout sequence  
- Internal dogfood → beta cohort (opt-in) → staged % rollout (SaaS).  
- Per-tenant flag to keep using v6 until completion.  
- Shadow mode: write to both stores, read from v6; then flip read to v7; then stop v6 writes.

5) Data migration steps (SaaS)  
- Snapshot v6 SQLite (or existing DB).  
- Run export; store artifact signed + hashed.  
- Import to PostgreSQL/Redis (idempotent).  
- Verify counts and checksums.  
- Cut read to v7; monitor; keep v6 read-only fallback for 7 days.

6) Rollback plan  
- Preserve v6 snapshots for 30 days.  
- Switch read pointer back to v6 if P0/P1 issues.  
- Disable writes to v7 during rollback; reconcile diffs manually if needed.

7) Communication  
- Pre-announce with timeline, what changes, how to rollback.  
- In-app banner + email for beta and GA waves.  
- Publish migration FAQ and support channel.

## Owners & Checks
- DRI: Migration lead.  
- Approvals: Eng (data), Security, Support.  
- Preflight checklist: backup ok, dry-run ok, metrics/alerts on, rollback tested.

## Metrics to watch
- Migration success rate, duration per tenant, data mismatch count.  
- Error rate (5xx), latency regression, crash rate (CLI/app).  
- User complaints/support tickets, rollback rate.

