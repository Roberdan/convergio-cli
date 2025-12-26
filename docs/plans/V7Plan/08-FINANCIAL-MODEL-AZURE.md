# Convergio V7 — Financial Model (Azure West Europe, Multi-Market)

**Status:** Draft for approval  
**Date:** 2025-12-26  
**Purpose:** Put concrete, reviewable numbers on the plan for Education (consumer + schools) and Business.

---

## 1) Chosen Stance (So It Works)

- **Primary region:** Azure **West Europe**.
- **BYOK-first** is mandatory to avoid margin collapse.
- **Managed** exists only as a capped add-on.

Adoption assumption (first 6 months):
- **BYOK:** ~88% (85–90%)
- **Managed:** ~12% (10–15%)

---

## 2) Explicit Assumptions (You Can Reject These)

### 2.1 Token economics (managed lane)
We will not publish a fake “$ per question”. We measure tokens.

Assumption placeholders for West Europe (replace with Azure sheet during approval):
- GPT-4o: \(P_{in}\) = $5 / 1M input tokens, \(P_{out}\) = $20 / 1M output tokens
- GPT-4o-mini: \(P_{in}\) = $0.6 / 1M, \(P_{out}\) = $2.4 / 1M

### 2.2 Usage shape
- Education consumer: median 120 requests/month, p90 350
- Business: median 300 requests/org/month, p90 1,000

### 2.3 Infra (West Europe)
Use the ranges from `02-AZURE-REFERENCE-ARCHITECTURE.md`.
For the 12-month target, we budget **$4,000–$9,000/month infra** (compute+db+redis+obs+edge).

---

## 3) 12-Month Targets (Realistic)

- **Education consumer:** 12,000 MAU
  - paid conversion: 8% (960 paying)
  - managed add-on uptake: 2% of MAU (240)
- **Schools:** 15 pilot schools
- **Business:** 200 orgs onboarded
  - paying orgs: ~60

---

## 4) Revenue Model (Concrete)

### 4.1 Education consumer
- Starter (BYOK): $6.99/mo
  - 960 paying → **$6,710 MRR**
- Plus (Managed): $12.99/mo
  - 240 paying → **$3,118 MRR**

**Education consumer subtotal:** **~$9,828 MRR**

### 4.2 Schools (pilot year)
Assumption: pilot package **$2,500/year per school** (covers admin, reporting, onboarding).
- 15 schools → **$37,500 ARR** (~$3,125 MRR equivalent)

### 4.3 Business teams
Assumption: Team BYOK baseline **$29.99/org/month**.
- 60 paying orgs → **$1,799 MRR**

**Total MRR (month 12 equivalent):** **~$14,700 MRR** (range: $10k–$20k depending on conversion)

---

## 5) Costs (Month 12)

### 5.1 Infra
- **$4,000–$9,000/month** (West Europe)

### 5.2 Managed token cost
Because managed is capped and minority:
- Budget: **$500–$2,000/month** for managed tokens + voice providers

### 5.3 Support and compliance overhead
- Consumer support: low-touch (self-serve)
- Schools: high-touch pilots (time cost, not only infra)

---

## 6) Profitability Reality (Brutal)

With BYOK-first, you can reach **positive gross margin** early.
However, you still need to fund:
- product development,
- school procurement cycles,
- and support.

**Year 1 expectation:** negative net (investment phase), but not because tokens explode.

---

## 7) What Must Be Measured to Validate This

- token distribution per request (P50/P90/P99)
- managed vs BYOK mix over time
- infra cost per MAU and per paying org
- support tickets per 100 users
- school pilot conversion rate to paid contracts

