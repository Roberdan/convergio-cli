# Convergio V7 — Execution Pack (Start Here)

**Status:** Draft for approval (replace "approved/final" until contracts below are accepted)
**Date:** 2025-12-26
**Audience:** Founders, virtual AI agent team, engineers, ops, finance

---

## What This Folder Is

This folder is a **clean, execution-grade** set of documents intended to be:
- reviewed quickly,
- approved explicitly,
- executed at high speed by a team of AI agents + humans,
- kept consistent by design (short docs, strict contracts).

---

## 🔴 CRITICAL DECISIONS REQUIRED (Blocking)

**Execution CANNOT start until these decisions are made:**

### Decision 1: Architecture — Convergio Kernel vs Microsoft Framework

| Option | Description | Pros | Cons |
|--------|-------------|------|------|
| **C1** | Convergio kernel everywhere | Control, local/offline, OSS moat | More engineering |
| **C2** | Microsoft framework for SaaS | Azure speed, enterprise integrations | Lock-in, less differentiation |
| **C3** | Split (MS for cloud, Convergio for local) | Best of both | Complexity, two codebases |

**Action required:** Run 1-week spike (see `13-MICROSOFT-AGENT-FRAMEWORK-EVAL.md`), then decide.
**Record decision in:** `01-SSOT-DECISIONS.md` Section C

---

### Decision 2: Timeline — Concrete Dates

| Milestone | Target Date | Deliverable |
|-----------|-------------|-------------|
| **M0** | Week 1-2 Jan 2026 | Contracts locked (docs 01-07 approved) |
| **M1** | Feb 2026 | Azure MVP backend (gateway + auth + BYOK) |
| **M2** | Mar 2026 | Education web MVP (chat + 3-5 teachers) |
| **M3** | Apr-Jun 2026 | School pilots (admin dashboard + audit) |
| **Beta** | Q2 2026 | Public beta (before Microsoft Agent Framework) |

**Action required:** Confirm or adjust dates.
**Record decision in:** `11-DELIVERY-PLAN-AI-AGENTS.md`

---

### Decision 3: Azure Pricing — Replace Placeholders

Current docs use placeholder pricing (e.g., "$5/1M input tokens").

**Action required:** Get official Azure OpenAI pricing for **West Europe** and update:
- `03-PRICING-METERING-BYOK.md`
- `08-FINANCIAL-MODEL-AZURE.md`

---

### Decision 4: Local LLM Strategy — Needs Spec

Local/offline is a key differentiator but has no dedicated spec.

**Action required:** Create `16-LOCAL-LLM-STRATEGY.md` covering:
- MLX (Apple Silicon)
- Ollama (cross-platform)
- Model selection and fallback
- Privacy mode (zero cloud calls)

---

## Non‑Negotiable Principles

- **BYOK-first**: users can bring their own keys; managed keys are optional and strictly budgeted.
- **Azure-first** (target platform): deploy and cost-model on Microsoft Azure.
- **Contract-first**: API, metering, plugin permissions, data retention are specs, not “ideas”.
- **Security by design**: plugins are untrusted by default; enforce least privilege.
- **Performance matters**: define latency budgets and enforce them.
- **Education + accessibility** are strategic differentiators, not “nice-to-have”.

---

## Canonical Reading Order (Approval Flow)

1) `01-SSOT-DECISIONS.md`
2) `02-AZURE-REFERENCE-ARCHITECTURE.md`
3) `03-PRICING-METERING-BYOK.md`
4) `04-API-CONTRACT.md`
5) `05-DATA-MODEL-RETENTION.md`
6) `06-PLUGIN-SYSTEM-SECURITY.md`
7) `07-OPS-SLO-RUNBOOKS.md`
8) `08-FINANCIAL-MODEL-AZURE.md`
9) `09-MARKETS-GTM.md`
10) `10-COMPETITION-STRATEGY.md`
11) `13-MICROSOFT-AGENT-FRAMEWORK-EVAL.md`
12) `14-VOICE-PLAN.md`
13) `15-MIGRATION-PLAN.md`
14) `11-DELIVERY-PLAN-AI-AGENTS.md`

## “Definition of Approved”

This plan is considered approved only when these conditions are true:
- The **decision log** is signed off (what we build, what we do *not* build).
- The **metering spec** is accepted (billable units + rounding + idempotency).
- The **API contract** is accepted (HTTP + WS schemas, versioning).
- The **plugin permission model** is accepted (untrusted vs trusted, enforcement).
- The **Azure cost model** is accepted (assumptions + target margins).

---

## Operating Rules (For AI Agent Execution)

- Every task must reference one of the canonical docs as source-of-truth.
- If a task requires a missing spec, the task becomes **“blocked: needs contract”**.
- Any change to pricing, metering, or permissions must update:
  - `01-SSOT-DECISIONS.md`
  - `03-PRICING-METERING-BYOK.md`
  - `06-OPS-SLO-RUNBOOKS.md` (cost guardrails)


