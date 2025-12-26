# Convergio V7 — SSOT Decisions (Lock These)

**Status:** Draft for approval  
**Date:** 2025-12-26  
**Purpose:** Single Source Of Truth for decisions. If it’s not here, it is not decided.

---

## A. Strategic Positioning (Brutal)

- **Reality:** Microsoft’s agent stack on Azure will compress the “generic agent framework” market fast.
- **Winning strategy:** do not compete head-on on “enterprise orchestration platform”. Win on:
  - **Education + accessibility** (product + trust + distribution),
  - **local/offline performance** (MLX/Ollama),
  - **open source kernel** + multi-provider neutrality,
  - and a **BYOK-first** cost model that scales without margin collapse.

---

## B. Platform & Deployment

- **Primary target platform:** Microsoft Azure.
- **Primary Azure region (data residency):** **West Europe**.
- **Primary product surface (Phase 1):** Web app + API (cross-platform).
- **Secondary surfaces:** macOS native app, CLI (developer workflows), later mobile.

---

## C. Architecture (Two viable options)

### Option C1 (Recommended): Convergio Kernel + Thin Gateway (Own the runtime)

- Keep the existing high-performance **Convergio C kernel** as the runtime engine.
- Build an **API gateway** (Rust preferred) that handles:
  - auth, BYOK vault, metering, quotas,
  - HTTP + WebSocket streaming (chat + voice events),
  - process pool / isolation strategy,
  - observability and cost guardrails.

**Why:** maximal control, local-first story, performance, OSS differentiation.

### Option C2 (Azure-speed): Microsoft Agent Framework as orchestration, Convergio as “tool/runtime”

- Use Microsoft’s agent framework for graph/planning/orchestration on Azure.
- Keep Convergio kernel as:
  - tool engine,
  - local runtime for macOS/offline,
  - compatibility layer for MCP and your existing agent packs.

**Why:** time-to-market and Azure-native integration.  
**Risk:** lock-in + loss of differentiation if everything becomes “standard Microsoft patterns”.

**Decision required:** pick C1, or pick C2 for SaaS while keeping C1 for local editions.

---

## D. BYOK & Billing Model (Mandatory)

- **BYOK is default**, not an advanced option.
- **Managed keys** are allowed only with:
  - strict budgets,
  - explicit user consent for overages,
  - automatic downgrade/throttle behavior.

**Billing units must be token/tool/audio based**, not “questions”.

### Adoption targets (first 6 months)

- **BYOK:** **~88%** overall (85–90% band)
  - consumer + business: 85–90%
  - schools: **95%+** (use the school’s Azure tenant)
- **Managed:** **~12%** overall (10–15% band)
  - consumer convenience only, strongly capped
  - limited school pilots only when procurement requires it

### 12-month outcome targets (realistic)

- **Education consumer:** 12,000 monthly active users (MAU), **8% paid conversion**.
- **Schools:** 15 pilot schools (BYOK preferred).
- **Business:** 200 organizations onboarded, ~60 paying orgs.

---

## E. Plugin Trust Model

- Default: **plugins are untrusted**.
- **Untrusted plugins must be WASM-only** with capability-based host calls.
- **Native plugins are trusted/official only** and must be signed and reviewed.

---

## F. Protocol Strategy

- Maintain MCP support.
- Design an abstraction layer to add:
  - A2A (priority),
  - ACP/ANP (monitor until real adoption).

---

## G. Scope Cuts (To Move Faster)

- Marketplace: **Year 2+** unless proven demand.
- Multiple web frameworks: pick one (SvelteKit recommended for simplicity/perf).
- Anything that requires undefined contracts: blocked until specs are approved.

---

## H. Change Control

Any change to pricing, metering, permissions, retention, or region requires:
- update this file,
- update the linked spec,
- a short “decision record” entry in `docs/adr/` (or a dedicated V7 ADR log).
