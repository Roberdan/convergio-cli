# Convergio V7 — Azure Reference Architecture (Costed)

**Status:** Draft for approval  
**Date:** 2025-12-26  
**Goal:** Azure-first design with explicit cost drivers.

**Primary region:** **West Europe**  
**Data residency:** PostgreSQL + Key Vault + Blob in West Europe; Front Door is global.

---

## 1) Core Architecture (Azure SaaS)

**Client surfaces**
- Web UI (primary)
- macOS app (secondary)
- CLI (developer workflows)

**Backend services**
- API Gateway (HTTP + WebSocket)
- Orchestration runtime (Convergio kernel and/or Microsoft agent framework)
- Usage/metering service
- Auth + BYOK vault
- Billing (Stripe/Paddle) + invoicing

**State**
- PostgreSQL (authoritative state + billing ledger)
- Redis (hot cache, websocket session assist)
- Blob Storage (artifacts, plugin packages, exports)

---

## 2) Azure Service Mapping (Recommended Baseline)

### Edge / Ingress
- **Azure Front Door** (global entry, WAF, TLS termination)
  - Alternative: Application Gateway + WAF (regional)

### Compute
- **Azure Container Apps** for API Gateway + workers (simple ops)
  - Alternative (higher control): AKS
  - Alternative (early MVP): single VM scale set (fastest, simplest)

### Data
- **Azure Database for PostgreSQL Flexible Server**
- **Azure Cache for Redis**
- **Azure Blob Storage**

### Security
- **Azure Key Vault** (BYOK vault entries, encryption keys, secrets)
- Managed identity everywhere possible.

### Observability
- **Azure Monitor + Application Insights**
- Export OpenTelemetry to your preferred backend if needed.

---

## 3) Performance Model (What Actually Matters)

### Latency budgets (initial)
- P95 HTTP: < 800 ms (non-stream)
- P95 first-token (WS): < 1.5 s
- Voice dropout rate: < 5% / 10 min session

### The critical path
Front Door → API Gateway → (policy/metering) → provider call → stream back

**Rule:** budget enforcement must happen *before* the provider call.

---

## 4) Cost Model (Explicit Assumptions)

### 4.1 Cost drivers
1) **LLM tokens** (dominant cost unless BYOK)
2) Compute (gateway + workers)
3) Postgres + Redis
4) Observability (logs/traces can be expensive)
5) Bandwidth (voice streaming can be meaningful)

### 4.2 Metered units (required)
- Input tokens
- Output tokens
- Tool calls (count + optional CPU time)
- Audio seconds (STT/TTS / realtime)

---

## 5) Rough Azure Monthly Baselines (MVP → Scale)

These are **order-of-magnitude** baselines for **West Europe**. Replace with your SKU selection during approval.

### MVP (≤ 1k active users)
- Container Apps: $200–$800
- Postgres Flexible: $150–$600
- Redis: $50–$250
- Blob: $10–$100
- Front Door/WAF: $50–$300
- Monitor/AppInsights: $100–$800

**Infra subtotal:** **$560–$2,850 / month**

### Growth (≈ 10k active users)
- Container Apps/AKS: $1,000–$4,000
- Postgres: $600–$2,500
- Redis: $250–$1,000
- Front Door/WAF: $300–$1,000
- Observability: $500–$3,000

**Infra subtotal:** **$2,650–$11,500 / month**

### Scale (≈ 50k active users)
Infra becomes second-order; LLM + observability dominate.

---

## 6) Two Execution Modes (Azure)

### Mode A: BYOK-first (recommended default)
- Your margin comes from orchestration + UX + reliability.
- LLM cost risk is largely eliminated.
- You still need throttling to protect infra and avoid abuse.

### Mode B: Managed keys (optional)
- You pay tokens; you must enforce budgets and overage consent.
- Requires the strictest metering and billing discipline.

---

## 7) Decision Needed: Microsoft Agent Framework vs Convergio Kernel

On Azure, if the priority is **speed and enterprise compatibility**, Microsoft’s framework may accelerate orchestration.

If the priority is **differentiation (education + local/offline + OSS + performance)**, Convergio kernel remains a core moat.

This decision is formalized in `13-MICROSOFT-AGENT-FRAMEWORK-EVAL.md`.
