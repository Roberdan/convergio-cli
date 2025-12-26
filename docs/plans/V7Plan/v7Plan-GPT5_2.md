# Convergio V7 Plans — Brutal Review (GPT-5.2)

**Date:** 2025-12-26  
**Scope:** Review of the repository `/ConvergioCLI-v7-plans` and planning docs in `docs/plans/V7Plan/`.  
**Goal:** Identify gaps, inconsistencies, and high-risk assumptions; propose a minimal set of “execution-grade” specs to unblock implementation.

---

## Executive Summary (Blunt)

The V7 plan set is unusually thorough on **vision, narrative, and broad architecture choices**. However, it is not yet “execution-grade” because it lacks a few non-negotiable **contracts and definitions**:

- **Metering definition** (what exactly is billable “usage”, with tokens/tools/audio/overhead).
- **API contract** (HTTP + WebSocket event schemas, versioning, errors, compatibility).
- **Plugin runtime and permissioning contract** (what “untrusted” means, how it is enforced, and what is allowed).
- **Multi-tenant data model** (tenants/users/entitlements/quotas/retention, GDPR deletion/export).

Until these are specified, “parallel development” will drift, and the system will accumulate accidental coupling, security exceptions, and pricing/math bugs.

---

## What’s Strong (Keep)

- **Master index and unified decisions:** Great navigation + clear “stack” intent (C core + Rust gateway + web UI).
- **Repo security posture is real:** `SECURITY_AUDIT.md` shows concrete hardening work (TOCTOU, injection, memory safety mitigations).
- **Guardrails exist:** `V7Plan-PRICING-GUARDRAILS.md` and `V7Plan-OPS-SLO.md` are pragmatic and protect against the two most common failures: cost blowups and operational chaos.
- **Voice testing approach:** Targets + degraded modes + instrumentation is the right mindset.

---

## High-Risk Gaps (Missing Specs)

### 1) Metering / Billing Spec (Critical)

Many documents use “$ per question”. In production this breaks unless a “question” is defined as a measurable, auditable unit.

**Must specify:**
- **Billable resources:** tokens (input/output), tool invocations, retrieval, audio seconds (STT/TTS), retries, streaming overhead.
- **Rounding rules:** per request vs per session; minimum billable units; partial audio segments.
- **Idempotency:** how you prevent double counting when retries/reconnects happen.
- **Auditability:** per-tenant ledger, dispute flow, and reconciliation vs provider invoices.
- **Cost protection:** budgets, caps, shock-protection confirmations, and forced downgrade rules.

**Outcome:** Without this, pricing guardrails are conceptual, not enforceable.

### 2) API Contract (Critical)

“Rust API Gateway (HTTP/WebSocket)” is referenced everywhere, but the contract is not defined.

**Must specify:**
- **OpenAPI (HTTP):** endpoints, request/response schemas, pagination, error codes.
- **WebSocket event protocol:** stream events, tool-call events, voice events, errors, reconnect semantics.
- **Versioning:** `/v1` legacy shim and `/v2` new; deprecations and compatibility guarantees.
- **Auth:** session model, tokens, BYOK handling, and admin endpoints.

**Outcome:** Without this, frontend/CLI/voice teams can’t build against stable interfaces.

### 3) Plugin Runtime + Permissions Contract (Critical)

`V7Plan-PLUGIN-SECURITY.md` outlines correct principles, but `V7Plan.md` suggests native dynamic library plugins via constructor macros.

**You must pick a hard line:**
- **Untrusted plugins:** WASM-only (capability-based host calls), default-deny permissions.
- **Native plugins:** trusted/official only, signature verified; ideally isolated behind a broker process if they need sensitive capabilities.

**Must specify:**
- **Plugin manifest schema:** id, version, required core version, permissions, capabilities, signing info.
- **Permission catalog:** filesystem read/write scopes, network domains, process execution, secrets access, UI integration, telemetry.
- **Enforcement points:** load-time checks + runtime policy enforcement, not “best effort”.

**Outcome:** Without this, “plugin marketplace” becomes an attack surface you can’t control.

### 4) Multi-tenant Data Model + Retention (Critical)

Plans mention PostgreSQL/Redis but do not lock the minimal schema and retention rules.

**Must specify:**
- **Entities:** tenant/org/user, sessions, conversations, memory artifacts, plugin installs, entitlements, quotas, usage ledger, invoices.
- **Retention:** default retention periods, opt-outs, deletion semantics, exports, backups.
- **Education/minors:** strict retention + consent/workflow expectations (even if “phase 2”, it must be designed now).

---

## Inconsistencies to Fix (Credibility + Execution)

### Web platform choice: SvelteKit vs Next.js

SvelteKit is the stated recommendation, but some diagrams/text still reference Next.js.

**Action:** Standardize docs and diagrams to a single default (or clearly mark alternatives).

### Plugin model: in-process dylib vs broker vs WASM

Docs simultaneously imply:
- direct dynamic loading (high risk),
- process isolation/broker (safer),
- WASM (best for marketplace untrusted).

**Action:** Write a single decision: “untrusted = WASM; native = trusted only”, and enforce it everywhere.

### “Final/Approved/100% complete” status claims

These claims are not credible without the contracts above.

**Action:** Replace with “v1 locked” + a short list of prerequisites for “final”.

---

## Risk Register (Reality Check)

1) **Cost blowups (P0):** Metering ambiguity + retries + tool usage can exceed margins quickly.  
2) **Security incident via plugins (P0):** Marketplace without strict sandbox/permissions is game over.  
3) **Scope creep (P0):** Marketplace + SaaS + voice + migration simultaneously is too much for a small team.  
4) **Integration thrash (P1):** No API contract means every team guesses semantics.  
5) **Compliance drag (P1):** Education + minors + GDPR needs explicit retention/deletion flows.  
6) **Operational gaps (P1):** SLOs exist, but runbooks + budgets + on-call ownership must be operationalized.

---

## Decisions You Must Lock (Non-negotiable)

1) **Billing unit:** tokens/tools/audio, not “questions” as a vague proxy.  
2) **Plugin trust boundary:** untrusted WASM only; native only if trusted/signed.  
3) **Voice transport:** WebSocket-only vs WebRTC; choose and design degraded modes accordingly.  
4) **Gateway ownership:** Rust gateway is the source of truth for auth/metering/quotas.  
5) **Data retention policy:** define now; do not “add later”.

---

## Recommended Minimal “Execution-Grade” Docs to Add

Add these as short, strict specs (not essays):

1) **V7 API Contract** (OpenAPI + WS events + errors + versioning)  
2) **V7 Metering & Billing Spec** (resources, rounding, idempotency, audit)  
3) **V7 Plugin Manifest & Permissions Catalog** (schema + enforcement rules)  
4) **V7 Data Model & Retention** (entities + retention/export/delete)

---

## Concrete Action Plan (Prioritized)

### Week 1–2 (Foundation)
- Write the 4 specs above and treat them as the source of truth.
- Standardize docs on one web stack (or mark alternatives explicitly).
- Add “bill shock protection” and “budget enforcement before provider call” as a hard rule.

### Month 1–2 (Integration-ready)
- Implement metering hooks end-to-end (gateway + provider calls + tool calls + voice).
- Implement plugin loading policy (WASM untrusted) with default-deny permissions.
- Define CLI compatibility strategy (which commands remain local vs go through gateway).

### Month 3+ (Scaling and polish)
- Add runbooks that map directly to SLOs (latency, error rate, cost overruns, voice failure).
- Prepare compliance posture for Education (retention, consent, DPA readiness).

---

## Notes on the Repository Reality

- The repo already contains meaningful security hardening; keep leveraging this advantage.
- Telemetry integration is partially tracked (`docs/TELEMETRY_INTEGRATION_PLAN.md`), but V7 will require telemetry to feed both **ops** and **billing/metering**.

---

## Bottom Line

The strategy is strong, but the plan becomes real only after you lock the contracts:
**API + metering + plugin permissions + data model/retention**. Everything else (web UI, voice UX, marketplace) depends on them.


