# Convergio V7 — Pricing, Metering, BYOK (Azure-First)

**Status:** Draft for approval  
**Date:** 2025-12-26  
**Purpose:** Define billable units, tiering, and guardrails so margins cannot go negative.

---

## 1) Non-Negotiable Rules

- **BYOK is default.** Users can add provider keys (Azure OpenAI/OpenAI/Anthropic/etc.).
- **Managed keys are optional** and must be protected by budgets + explicit overage consent.
- Billing is based on **measured usage**, not a vague “question”.

**West Europe default:** for EU education and school deployments, prefer Azure OpenAI in **West Europe** unless an approved exception exists.

---

## 2) Metering Units (Authoritative)

Every request produces a **Usage Event** with these fields (minimum):
- `tenant_id`, `user_id`, `session_id`, `request_id`
- `provider`, `model`
- `input_tokens`, `output_tokens`
- `tool_calls_count`
- `audio_seconds_in`, `audio_seconds_out` (if voice)
- `latency_ms`, `success`, `error_code`
- `mode`: `byok` | `managed`

**Idempotency rule:** `request_id` is unique; retries must not double-count.

---

## 3) Pricing Model (Two-Lane)

### Lane A: BYOK (Default)
You charge for **orchestration + UX + reliability**. Users pay model tokens directly.

**Pros:** near-zero token cost risk, scales to large usage without margin collapse.  
**Cons:** requires excellent UX for key management and cost visibility.

### Lane B: Managed (Optional)
You pay token costs and must enforce:
- per-user and per-tenant budgets,
- forced downgrade to cheaper models,
- consent gates before overages.

---

## 4) Token Cost Model (Use Variables)

Let:
- \(P_{in}\) = price per 1M input tokens
- \(P_{out}\) = price per 1M output tokens
- \(T_{in}\), \(T_{out}\) = measured tokens

Then request cost is:

\[ cost = (T_{in}/1{,}000{,}000) \cdot P_{in} + (T_{out}/1{,}000{,}000) \cdot P_{out} \]

**Approval requirement:** replace \(P_{in}, P_{out}\) with the official Azure OpenAI sheet for **West Europe**.

---

## 5) Tier Proposals (Per Market)

### 5.1 Education Consumer (B2C)
- **Free (BYOK):** $0, basic features, strict rate limits (anti-abuse).
- **Starter (BYOK):** $6.99/mo, higher rate limits, saved projects.
- **Plus (Managed):** $12.99/mo includes a small managed token pack + overage consent.

### 5.2 Schools (B2B2C)
- **School BYOK:** school provides Azure OpenAI keys; you provide orchestration, policies, reporting.
- **School Managed:** only if procurement requires it, with budget caps.

**Must include:** GDPR retention controls, admin dashboard, audit logs, content policies.

### 5.3 Business (SMB)
- **Team BYOK:** $29.99 / org / month (baseline) + add-ons; BYOK default.
- **Team Managed:** token bundles + strict budgets; negotiated.

---

## 6) Guardrails (Must Be Enforced)

- **Budget caps:** per tenant and global daily spend caps for managed lane.
- **Shock protection:** stop/confirm at 80% and block at 110% unless approved.
- **Automatic downgrade:** when budget > 90%, force cheapest acceptable model.
- **Burst limits:** per-minute caps to prevent runaway costs and abuse.

---

## 7) Chosen Targets (So This Is Executable)

### BYOK vs Managed (first 6 months)
- **BYOK:** ~88% overall (85–90% band).
- **Managed:** ~12% overall (10–15% band), optional convenience only.

### 12-month planning targets (aligned with SSOT)
- Education consumer: 12,000 MAU, 8% paid conversion.
- Schools: 15 pilot schools (BYOK via school Azure tenant preferred).
- Business: 200 orgs onboarded, ~60 paying orgs.

---

## 8) What We Need to Implement (Acceptance)

- Usage event pipeline is authoritative (idempotent, auditable).
- Per-tenant usage dashboard:
  - tokens, tools, audio seconds,
  - cost estimate (managed) or provider breakdown (BYOK).
- Billing export (CSV) for schools and businesses.
