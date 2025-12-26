# Convergio V7 — Vertical Strategy

**Status:** Draft for approval
**Date:** 2025-12-26
**Purpose:** Define the vertical expansion roadmap. Education is V1, not the entire product.

---

## 1) Strategic Model

```
                    Convergio Core (Open Source)
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
   ┌─────────┐          ┌─────────┐          ┌─────────┐
   │Education│          │Healthcare│          │Business │
   │   V1    │          │   V2    │          │   V3    │
   │ (2026)  │          │ (2027)  │          │ (2026)  │
   └─────────┘          └─────────┘          └─────────┘
        │                     │                     │
   B2C + B2B2C          B2B + B2B2C              B2B
   (Schools)            (Clinics)            (SMB/Enterprise)
```

**Principle:** Each vertical is a **plugin package** built on Core, not a fork.

---

## 2) Vertical Selection Criteria

| Criterion | Weight | Description |
|-----------|--------|-------------|
| **TAM** | 25% | Total addressable market size |
| **Regulatory fit** | 20% | Can we meet compliance requirements? |
| **AI value-add** | 20% | Does AI significantly improve the workflow? |
| **Distribution** | 15% | Do we have a path to customers? |
| **Competition** | 10% | How crowded is the space? |
| **Synergy** | 10% | Does it leverage existing capabilities? |

---

## 3) Vertical Roadmap

### V1: Education (2026)
**Status:** Primary focus

| Segment | Description | Target |
|---------|-------------|--------|
| **Consumer** | Students, self-learners | 12,000 MAU |
| **Schools** | K-12, vocational | 15 pilots |
| **Higher Ed** | Universities (future) | Research only |

**Key agents:**
- AI Tutor (personalized learning)
- Assignment Grader
- Curriculum Planner
- Study Buddy (Q&A)
- Progress Tracker

**Compliance:** GDPR, COPPA, FERPA, accessibility (WCAG 2.1)

**Why first:**
- Clear AI value-add (personalization)
- Underserved by big tech
- Trust differentiator
- Lower regulatory burden than healthcare

---

### V2: Business/SMB (2026-2027)
**Status:** Secondary focus (parallel development)

| Segment | Description | Target |
|---------|-------------|--------|
| **Teams** | Small teams (2-20) | 200 orgs |
| **Agencies** | Consulting, marketing | 50 orgs |
| **Developers** | Technical workflows | Community |

**Key agents:**
- Meeting Summarizer
- Document Analyzer
- Code Assistant
- Research Agent
- Workflow Automator

**Compliance:** SOC 2 Type II (target), GDPR

**Why parallel:**
- Lower compliance burden
- Faster sales cycles
- Revenue diversification
- Proves platform generality

---

### V3: Healthcare (2027+)
**Status:** Research phase

| Segment | Description | Target |
|---------|-------------|--------|
| **Clinics** | Small practices | Pilots only |
| **Mental Health** | Therapist assistants | Research |
| **Wellness** | Non-clinical apps | Consumer |

**Key agents:**
- Clinical Note Assistant
- Patient Intake
- Symptom Checker (non-diagnostic)
- Appointment Scheduler
- Treatment Plan Helper

**Compliance:** HIPAA, FDA (if applicable), GDPR

**Why later:**
- Heavy regulatory burden
- Requires specialized expertise
- Higher liability risk
- Longer sales cycles

---

### V4: Legal (2028+)
**Status:** Exploratory

| Segment | Description |
|---------|-------------|
| **Solo practitioners** | Document review, research |
| **Legal ops** | Contract analysis, due diligence |

**Key agents:**
- Contract Reviewer
- Legal Research
- Document Drafter
- Case Summarizer

**Compliance:** Bar association rules, confidentiality requirements

---

### V5: Creative (2028+)
**Status:** Exploratory

| Segment | Description |
|---------|-------------|
| **Writers** | Content creation, editing |
| **Marketers** | Campaign planning, copy |
| **Designers** | Concept generation, feedback |

**Key agents:**
- Content Writer
- Editor/Proofreader
- Campaign Planner
- Brand Voice Keeper

---

## 4) Vertical Development Process

### Phase 1: Research (4-6 weeks)
- Market analysis
- User interviews
- Compliance mapping
- Competitor analysis
- Technical feasibility

### Phase 2: MVP (8-12 weeks)
- Core agents (2-3)
- Essential tools
- Basic policies
- Minimal UI

### Phase 3: Pilot (12-16 weeks)
- 5-10 pilot customers
- Feedback integration
- Compliance validation
- Pricing validation

### Phase 4: Launch (4-8 weeks)
- Public availability
- Documentation
- Marketing
- Support processes

### Phase 5: Scale
- Feature expansion
- Enterprise features
- Partner integrations
- International expansion

---

## 5) Revenue Model by Vertical

| Vertical | Primary Model | Secondary Model |
|----------|---------------|-----------------|
| **Education Consumer** | Subscription ($6.99-$12.99/mo) | — |
| **Education Schools** | Annual contract ($2,500+/year) | Per-seat add-ons |
| **Business Teams** | Subscription ($29.99/org/mo) | Usage overage |
| **Business Enterprise** | Annual contract (negotiated) | Professional services |
| **Healthcare** | Per-seat license | Compliance add-ons |
| **Legal** | Per-seat license | Document volume pricing |

---

## 6) Build vs. Partner

| Vertical | Strategy | Rationale |
|----------|----------|-----------|
| **Education** | Build | Core differentiator, full control |
| **Business** | Build | Platform validation, revenue |
| **Healthcare** | Partner | Compliance expertise needed |
| **Legal** | Partner | Domain expertise needed |
| **Creative** | Community | Lower barrier, ecosystem play |

---

## 7) Vertical-Specific Compliance

| Vertical | Key Regulations | Certification Targets |
|----------|-----------------|----------------------|
| **Education** | GDPR, COPPA, FERPA, WCAG | ISO 27001 |
| **Business** | GDPR, SOC 2 | SOC 2 Type II |
| **Healthcare** | HIPAA, FDA, GDPR | HITRUST (future) |
| **Legal** | Bar rules, GDPR | — |

---

## 8) Cross-Vertical Synergies

| Capability | Education | Business | Healthcare | Legal |
|------------|-----------|----------|------------|-------|
| **Document analysis** | Assignments | Contracts | Records | Filings |
| **Q&A / RAG** | Study help | Knowledge base | Patient info | Case law |
| **Scheduling** | Classes | Meetings | Appointments | Court dates |
| **Summarization** | Lectures | Meetings | Notes | Depositions |
| **Translation** | Multilingual | International | Patient comms | International |

**Implication:** Build these as Core tools, not vertical-specific.

---

## 9) Success Metrics by Vertical

### Education
- MAU, DAU
- Learning outcome improvements (where measurable)
- Teacher time saved
- Student satisfaction (NPS)
- School renewal rate

### Business
- Orgs onboarded
- Active users per org
- Tasks automated per user
- Time saved per user
- Net revenue retention

### Healthcare
- Clinical time saved
- Documentation accuracy
- Patient satisfaction
- Compliance audit pass rate

---

## 10) Investment Allocation (Year 1)

| Area | Allocation | Rationale |
|------|------------|-----------|
| **Core Platform** | 40% | Foundation for all verticals |
| **Education Vertical** | 35% | Primary beachhead |
| **Business Vertical** | 20% | Revenue diversification |
| **Research (V3+)** | 5% | Future preparation |

---

## 11) Key Decisions Required

1. **Education vs Business priority**: Which ships first if constrained?
   - Recommendation: Education MVP first, Business fast-follow

2. **Healthcare timeline**: When to start serious investment?
   - Recommendation: Research in 2026, build in 2027

3. **Partner strategy**: Who to partner with for regulated verticals?
   - Recommendation: Identify 2-3 partners per vertical in research phase

4. **Community verticals**: How to enable/encourage community-built verticals?
   - Recommendation: SDK + documentation + showcase program

