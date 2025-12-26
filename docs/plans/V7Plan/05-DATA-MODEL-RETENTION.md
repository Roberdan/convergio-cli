# Convergio V7 — Data Model, Tenancy, Retention (Azure)

**Status:** Draft for approval  
**Date:** 2025-12-26  
**Purpose:** Define minimal entities, retention, export/delete semantics for consumer, schools, and business.

---

## 1) Tenancy Model

- **Consumer:** `tenant = user`
- **Schools / Business:** `tenant = organization`

Every request must be attributable to:
- `tenant_id` (org or user)
- `user_id`

---

## 2) Minimal Entities (Authoritative)

### Identity & access
- `tenants` (id, type, name, region, created_at)
- `users` (id, tenant_id, email, role, created_at)
- `sessions` (id, user_id, tenant_id, created_at, expires_at)

### BYOK vault (references only)
- `provider_keys` (id, tenant_id, provider, key_ref, status, created_at)
  - `key_ref` points to **Azure Key Vault secret id**, not the secret value.

### Conversations
- `conversations` (id, tenant_id, user_id, created_at, title)
- `messages` (id, conversation_id, role, content_ref, created_at)
  - store large content in blob (optional) referenced by `content_ref`.

### Usage ledger (billing authority)
- `usage_events`
  - (id, request_id UNIQUE, tenant_id, user_id, provider, model,
     input_tokens, output_tokens, tool_calls, audio_in_s, audio_out_s,
     mode byok|managed, success, error_code, created_at)

### Entitlements / quotas
- `plans` (id, name)
- `entitlements` (tenant_id, plan_id, limits_json, effective_at)
- `quota_state` (tenant_id, period_start, counters_json)

### Billing (SaaS)
- `billing_accounts` (tenant_id, stripe_customer_id?, status)
- `invoices` (id, tenant_id, period, amount, status, provider_ref)

### Plugins (SaaS)
- `plugins` (id, version, hash, trusted, created_at)
- `plugin_installs` (tenant_id, plugin_id, enabled, permissions_json)

---

## 3) Retention Policy (Defaults)

### Consumer default
- conversations: 180 days (configurable)
- usage events: 24 months (for disputes/accounting)

### Schools (minors risk)
- conversations: **30–90 days default** (policy-driven)
- export: admin export available
- deletion: stricter, auditable

### Business
- conversations: 365+ days (configurable)
- usage events: 24–84 months (configurable for accounting)

---

## 4) Delete / Export Semantics (GDPR)

### Delete
- user requests deletion:
  - remove PII from `users`
  - delete conversations/messages content
  - keep `usage_events` minimally retained with pseudonymization if required by accounting

### Export
- export formats: JSONL + CSV (usage ledger)
- must include checksum and export metadata.

---

## 5) Security Requirements

- Encrypt at rest (Azure managed) + TLS in transit.
- Key Vault for BYOK secrets; never log raw keys.
- Admin actions must be audit-logged.

---

## 6) Implementation Notes

- Prefer Postgres as the source of truth.
- Redis is cache only (rebuildable).
- Blob for large transcripts/attachments is optional; can start with Postgres and evolve.

