# Convergio V7 — Markets & Go-To-Market (Verticals)

**Status:** Draft for approval
**Date:** 2025-12-26
**Purpose:** Define go-to-market for vertical plugins built on Convergio Core.

---

## Context: Platform vs. Verticals

This document covers **vertical-specific GTM**. For the core platform strategy, see:
- `16-CORE-PLATFORM-ARCHITECTURE.md` — What the platform is
- `19-VERTICAL-STRATEGY.md` — Full vertical roadmap

**Key principle:** Markets here are served by **vertical plugins**, not by the core platform directly. The core is infrastructure; verticals are products.

```
Convergio Core (open source, free)
        │
        ├── Education Vertical (commercial) ← This doc
        ├── Business Vertical (commercial)  ← This doc
        └── [Future verticals]
```

---

## 1) Vertical A — Education (V1 Priority)

### Product promise
- Fast, accessible learning support with strong privacy controls.
- Local-first option (MLX/Ollama) for privacy and cost.

### Differentiators
- Accessibility-first UX.
- Curriculum-aligned agent packs.
- Offline/local mode that actually works.

### Pricing
- BYOK default + optional managed add-on.

### GTM tactics
- Short term (0–3 mo):
  - ship a single flagship “Education Pack” with 3–5 top teachers
  - creator partnerships (teachers), GitHub + community
- Medium (3–9 mo):
  - referral loop + family plans
  - content marketing + open curriculum templates
- Long (9–24 mo):
  - marketplace (only after proven demand)

---

## 2) Vertical A.2 — Schools (Education B2B2C)

### Product promise
- Admin-controlled AI learning assistant that is compliant and auditable.

### Non-negotiable requirements
- Data retention policies
- Admin dashboard + audit logs
- BYOK via school’s Azure tenant (preferred)
- Export and deletion flows

### Pricing options (choose one)
- Per-seat (teacher/admin) + student access included
- Per-student per-year (low ARPU, high volume)
- Pilot packages (fixed fee) to get procurement started

### GTM tactics
- Short term:
  - 5–10 pilot schools with strict scope
  - publish compliance posture and retention defaults
- Medium:
  - case studies + procurement playbook
  - integrations (SSO, roster)
- Long:
  - regional distribution partners

---

## 3) Vertical B — Business/SMB (V2 Priority)

### Product promise
- Orchestration + integrations + auditability, not “just chat”.

### Differentiators
- Multi-provider neutrality
- BYOK-first reduces security/compliance friction
- Tool/workflow orchestration with strong guardrails

### Pricing
- Org subscription (BYOK default) + add-ons

### GTM tactics
- Short term:
  - focus on 1–2 workflows (e.g., docs + tickets + code review)
  - templates and quickstart
- Medium:
  - integrations (Jira, GitHub, Slack)
- Long:
  - enterprise readiness (SLA, SSO, RBAC)

---

## 4) KPI Set (Per Vertical)

### Core Platform KPIs
- GitHub stars, forks, contributors
- Plugin ecosystem size
- Developer adoption (CLI downloads, API calls)

### Vertical-specific KPIs
- Education consumer: activation, retention, paid conversion, support load
- Schools: pilot-to-contract conversion, procurement cycle time, renewal
- Business: org activation, expansion, churn

---

## 5) Execution Priority

**Platform-first, vertical-second.**

| Phase | Focus | Why |
|-------|-------|-----|
| **Phase 0** | Core platform stable | Can't build verticals on unstable foundation |
| **Phase 1** | Education vertical (consumer + schools) | Beachhead, underserved, trust moat |
| **Phase 2** | Business vertical | Revenue diversification, proves platform generality |
| **Phase 3** | Community/partner verticals | Ecosystem expansion |

**Do not** try to maximize all verticals at once. The platform must prove itself first.

