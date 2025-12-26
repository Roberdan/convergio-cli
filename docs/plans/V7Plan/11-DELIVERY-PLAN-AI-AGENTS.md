# Convergio V7 — Delivery Plan (AI Agents + Humans)

**Status:** Draft for approval  
**Date:** 2025-12-26  
**Purpose:** Make execution fast and safe by defining how virtual AI agents should work from these docs.

---

## 1) Workstreams (Parallelizable)

1) Contracts
- API contract
- metering spec
- permission catalog
- data model/retention

2) Gateway (Azure)
- auth + BYOK vault
- policy + quotas + metering
- HTTP/WS implementation

3) Runtime
- Convergio kernel integration and isolation strategy
- provider routing policy
- tool execution

4) Web UI
- chat + streaming
- BYOK onboarding + cost dashboard
- admin for schools/business

5) Ops
- OTel, dashboards, alerts
- runbooks + budgets

---

## 2) Definition of Done (Per Feature)

A feature is “done” only if:
- it matches the canonical doc contract,
- it has telemetry and error codes,
- it has a rollback path (flag or deploy rollback),
- it has a cost/abuse guard.

---

## 3) AI Agent Execution Rules

- Agents must never invent APIs, schemas, or pricing.
- If a spec is missing, agent must open a “blocked” task and propose a minimal spec update.
- Changes must keep docs < 250 lines and consistent.

---

## 4) Milestones (Aggressive but Realistic)

### M0 (Week 1–2): Contracts locked
- Approve docs 01–07.
- Implement minimal metering event structure.

### M1 (Month 1–2): Azure MVP backend
- API gateway skeleton + auth + BYOK vault
- WS streaming for chat
- usage ledger

### M2 (Month 2–3): Education web MVP
- web UI basic chat + BYOK onboarding
- 3–5 flagship teachers
- cost dashboard

### M3 (Month 3–6): School pilots
- admin dashboard (users, retention, exports)
- audit logs
- pilot deployments

---

## 5) Microsoft Agent Framework Evaluation Plan

To decide whether to adopt it for SaaS:
- Build a 1-week spike with a single workflow.
- Compare:
  - latency,
  - cost,
  - integration speed,
  - portability.

Decision must be recorded in `01-SSOT-DECISIONS.md`.

