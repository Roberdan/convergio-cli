# Convergio V7: Principles Compliance & Alignment

**Date:** December 26, 2025  
**Purpose:** Ensure all V7 plans respect Convergio's core principles and values

---

## Executive Summary

**This document verifies that all V7 plans align with Convergio's fundamental principles:**

✅ **Open Source Core** - Core engine is MIT licensed, free forever  
✅ **Privacy First** - Data stays local, no cloud storage by default  
✅ **Accessibility Native** - Built-in, not added later  
✅ **Education Priority** - Free tier for students, GDPR compliant  
✅ **Security by Design** - Sandbox, safety filters, prompt injection protection  
✅ **Inclusivity** - Person-first language, no one left behind  
✅ **Truth & Verification** - Never invent data, always verify  
✅ **Multi-Provider** - No vendor lock-in  
✅ **Local AI Support** - MLX, AFM, Ollama for privacy/offline  
✅ **Proactive Teaching** - Maestri guide, don't just answer  

**All V7 plans have been reviewed and updated to ensure compliance.**

---

## Part 1: Core Principles from Convergio

### 1.1 Open Source Foundation

**Principle:** Core engine must be open source (MIT license), free forever.

**V7 Compliance:**
- ✅ Core C library: MIT license (open source)
- ✅ Plugin API: Open source
- ✅ CLI interface: Open source
- ✅ BYOK support: Users bring own keys (no lock-in)

**Documents Verified:**
- [V7Plan.md](./V7Plan.md) - Core is open source
- [V7Plan-CRITICAL-REVIEW.md](./V7Plan-CRITICAL-REVIEW.md) - Architecture maintains open source core

**Status:** ✅ COMPLIANT

### 1.2 Privacy First

**Principle:** Data stays on user's computer. No cloud storage by default. No data collection without consent.

**V7 Compliance:**
- ✅ Self-hosted option: Users can run core locally
- ✅ BYOK model: Users provide API keys (no data sent to us)
- ✅ Local state: SQLite on user's machine
- ✅ Telemetry: Opt-in only, minimal by default
- ✅ GDPR compliance: Education uses Azure OpenAI (EU regions)

**Documents Verified:**
- [V7Plan-Enhanced.md](./V7Plan-Enhanced.md) - Telemetry is opt-in
- [V7Plan-Billing-Security.md](./V7Plan-Billing-Security.md) - Privacy-first design
- [V7Plan-Education-Cost-Analysis.md](./V7Plan-Education-Cost-Analysis.md) - BYOK option

**Status:** ✅ COMPLIANT

### 1.3 Accessibility Native

**Principle:** Accessibility is not an add-on. It's built into the core design from day 1.

**V7 Compliance:**
- ✅ Education pack: Accessibility features built-in (dyslexia, ADHD, autism, motor difficulties)
- ✅ WCAG 2.1 AA compliance: Required for Education
- ✅ Jenny agent: Accessibility champion (included in Education pack)
- ✅ Adaptive UI: Adjusts to user needs automatically

**Documents Verified:**
- [V7Plan.md](./V7Plan.md) - Education pack includes accessibility
- [V7Plan-Voice-WebPlatform.md](./V7Plan-Voice-WebPlatform.md) - Voice I/O for accessibility

**Status:** ✅ COMPLIANT

### 1.4 Education Priority

**Principle:** Education is the flagship. Free tier for students. GDPR compliant. Safety first.

**V7 Compliance:**
- ✅ Education pack: 17 Maestri (free tier: 3 teachers, 30 questions/month)
- ✅ Azure OpenAI only: For GDPR compliance (EU regions)
- ✅ Safety filters: Content filtering, prompt injection protection
- ✅ Free tier: Sustainable with usage limits + grants

**Documents Verified:**
- [V7Plan-Education-Cost-Analysis.md](./V7Plan-Education-Cost-Analysis.md) - Free tier sustainability
- [V7Plan-Ecosystem-Strategy.md](./V7Plan-Ecosystem-Strategy.md) - Education as flagship
- [V7Plan.md](./V7Plan.md) - Education pack structure

**Status:** ✅ COMPLIANT

### 1.5 Security by Design

**Principle:** Security is not added later. Sandbox, safety filters, prompt injection protection from day 1.

**V7 Compliance:**
- ✅ Plugin sandboxing: Isolated execution
- ✅ Prompt injection protection: Built into all agents
- ✅ Content filtering: Age-appropriate responses
- ✅ Secure API key storage: Keychain, secrets manager
- ✅ PCI-DSS compliance: Stripe handles card data (we don't store)

**Documents Verified:**
- [V7Plan-Billing-Security.md](./V7Plan-Billing-Security.md) - Security requirements
- [V7Plan-Enhanced.md](./V7Plan-Enhanced.md) - Plugin security

**Status:** ✅ COMPLIANT

### 1.6 Inclusivity

**Principle:** Person-first language. No one left behind. Respect diversity.

**V7 Compliance:**
- ✅ Person-first language: Required for all agents
- ✅ Accessibility support: Dyslexia, ADHD, autism, motor difficulties
- ✅ Gender-neutral language: Default for all communications
- ✅ Cultural sensitivity: Diverse examples, no stereotypes

**Documents Verified:**
- [V7Plan.md](./V7Plan.md) - Education pack includes Jenny (accessibility champion)
- All plans respect inclusivity principles

**Status:** ✅ COMPLIANT

### 1.7 Truth & Verification

**Principle:** NEVER invent or fabricate data. Always verify with tools before asserting facts.

**V7 Compliance:**
- ✅ Tool-first approach: Agents use tools (file, web, etc.) before claiming facts
- ✅ Error handling: Report actual errors, don't invent output
- ✅ Verification required: System prompts enforce verification

**Documents Verified:**
- All plans assume agents follow truth & verification principle

**Status:** ✅ COMPLIANT (assumed in agent design)

### 1.8 Multi-Provider Support

**Principle:** No vendor lock-in. Support all LLM providers. Users choose.

**V7 Compliance:**
- ✅ LLM Router: Supports OpenAI, Anthropic, Google, Azure, Local (MLX, Ollama)
- ✅ Model selection: Automatic or user choice
- ✅ Provider failover: If one fails, use another
- ✅ Cost optimization: Choose cheapest model for task

**Documents Verified:**
- [V7Plan.md](./V7Plan.md) - Multi-provider LLM router
- [V7Plan-Architecture-DeepDive.md](./V7Plan-Architecture-DeepDive.md) - Provider support

**Status:** ✅ COMPLIANT

### 1.9 Local AI Support

**Principle:** Support local models (MLX, AFM, Ollama) for privacy and offline use.

**V7 Compliance:**
- ✅ MLX support: Apple Silicon local models
- ✅ AFM support: Apple Foundation Models (macOS 26+)
- ✅ Ollama support: Self-hosted models
- ✅ Offline mode: Works without internet

**Documents Verified:**
- [V7Plan.md](./V7Plan.md) - Local model support
- [V7Plan-Architecture-DeepDive.md](./V7Plan-Architecture-DeepDive.md) - Local provider integration

**Status:** ✅ COMPLIANT

### 1.10 Proactive Teaching (Education)

**Principle:** Maestri are proactive, not passive. They guide, propose, engage.

**V7 Compliance:**
- ✅ Proactive loop: Greet → Assess → Explain → Check → Propose → Create → Celebrate
- ✅ Tool proposals: Suggest mind maps, HTML pages, quizzes
- ✅ Engagement: Every response includes questions or proposals
- ✅ Personalization: Use student name, remember preferences

**Documents Verified:**
- [V7Plan.md](./V7Plan.md) - Education pack agents
- Teacher Manifesto principles embedded in agent prompts

**Status:** ✅ COMPLIANT

### 1.11 Maieutic Method (Socratic Teaching)

**Principle:** Guide students to discover answers, don't give them directly.

**V7 Compliance:**
- ✅ Socratic method: Agents ask questions, guide discovery
- ✅ Anti-cheating: Homework help guides, doesn't solve
- ✅ Step-by-step: Break down problems, let student solve

**Documents Verified:**
- [V7Plan.md](./V7Plan.md) - Education pack uses Socratic method
- Teacher Manifesto requires maieutic approach

**Status:** ✅ COMPLIANT

### 1.12 Storytelling

**Principle:** Transform concepts into stories. Humans remember stories, not lists.

**V7 Compliance:**
- ✅ Story-based explanations: Agents use stories, analogies, historical context
- ✅ Visual storytelling: HTML pages, diagrams, mind maps
- ✅ Real-world applications: Connect concepts to student interests

**Documents Verified:**
- [V7Plan.md](./V7Plan.md) - Education pack agents use storytelling
- Teacher Manifesto emphasizes storytelling

**Status:** ✅ COMPLIANT

### 1.13 Multimodal Learning

**Principle:** Support text, audio, images, diagrams. Not everyone learns the same way.

**V7 Compliance:**
- ✅ Text: Markdown responses
- ✅ Audio: Voice I/O, TTS for accessibility
- ✅ Visual: HTML pages, mind maps, diagrams
- ✅ Interactive: Quizzes, flashcards, calculators

**Documents Verified:**
- [V7Plan-Voice-WebPlatform.md](./V7Plan-Voice-WebPlatform.md) - Voice I/O
- [V7Plan.md](./V7Plan.md) - Tool plugins (visual, audio)

**Status:** ✅ COMPLIANT

---

## Part 2: Principles Applied to V7 Architecture

### 2.1 Open Source Core

**Architecture Decision:**
- Core C library: MIT license
- Plugin API: Open source
- CLI: Open source
- Web UI: Open source (SvelteKit)

**Commercial Components:**
- Premium plugins: Can be commercial (Education Pro, Developer Pro, etc.)
- SaaS hosting: Commercial service (but self-hosted option available)
- Enterprise features: Commercial (on-premise, custom development)

**Compliance:** ✅ Core remains open source, commercial value in plugins/services

### 2.2 Privacy-First Architecture

**Architecture Decision:**
- Self-hosted option: Users run core locally
- BYOK model: Users provide API keys (no LLM costs for us)
- Local state: SQLite on user's machine (not in cloud)
- Telemetry: Opt-in, minimal by default

**SaaS Option:**
- Multi-tenant: Isolated state per user
- GDPR compliant: EU data residency
- Encryption: At rest and in transit
- Data export/deletion: User rights respected

**Compliance:** ✅ Privacy-first design, SaaS is optional

### 2.3 Accessibility Built-In

**Architecture Decision:**
- Jenny agent: Accessibility champion (included in Education pack)
- Adaptive UI: Adjusts to user needs
- Voice I/O: For motor difficulties, dyslexia
- Visual tools: For dyscalculia, visual learners

**Web Platform:**
- WCAG 2.1 AA compliance: Required
- Screen reader support: Native
- Keyboard navigation: Full support
- High contrast mode: Available

**Compliance:** ✅ Accessibility is core feature, not add-on

### 2.4 Education as Flagship

**Architecture Decision:**
- Education pack: First plugin, highest priority
- Free tier: 3 teachers, 30 questions/month
- Azure OpenAI: GDPR compliance (EU regions)
- Safety filters: Built into Education agents

**Compliance:** ✅ Education prioritized, free tier sustainable

---

## Part 3: Principles Applied to Business Model

### 3.1 Open Source + Commercial Balance

**Model:**
- Core: Free (open source)
- Premium plugins: Paid (commercial)
- SaaS: Paid (commercial service)
- Enterprise: Paid (commercial)

**Compliance:** ✅ Core free, value in premium features

### 3.2 Privacy-First Pricing

**Model:**
- BYOK option: Free platform, users bring keys (no LLM costs)
- Self-hosted: Free core, optional premium features
- SaaS: Paid, but privacy-respecting (GDPR compliant)

**Compliance:** ✅ Privacy-first options available

### 3.3 Education Free Tier

**Model:**
- Free: 3 teachers, 30 questions/month
- Pro: $6.99/month + 100 questions + $0.01/extra
- Sustainability: Usage limits + grants + premium conversions

**Compliance:** ✅ Free tier for students, sustainable model

---

## Part 4: Principles Applied to Implementation

### 4.1 Security from Day 1

**Implementation:**
- Plugin sandboxing: From Phase 1
- Prompt injection protection: Built into agents
- Content filtering: Education agents
- Secure storage: Keychain, secrets manager

**Compliance:** ✅ Security not added later

### 4.2 Accessibility from Day 1

**Implementation:**
- Jenny agent: Included in Education pack (Phase 1)
- Adaptive UI: Built into web platform (Phase 2)
- Voice I/O: Priority feature (Phase 3)

**Compliance:** ✅ Accessibility from start

### 4.3 Privacy from Day 1

**Implementation:**
- Local state: SQLite from Phase 1
- BYOK support: From Phase 1
- Telemetry opt-in: From Phase 2
- GDPR compliance: From Phase 1 (Education)

**Compliance:** ✅ Privacy-first from start

---

## Part 5: Principles Verification Checklist

### Core Principles

- [x] **Open Source Core** - Core is MIT licensed
- [x] **Privacy First** - Data local, BYOK option, GDPR compliant
- [x] **Accessibility Native** - Built-in, not add-on
- [x] **Education Priority** - Free tier, flagship product
- [x] **Security by Design** - Sandbox, filters, protection from day 1
- [x] **Inclusivity** - Person-first, no one left behind
- [x] **Truth & Verification** - Never invent data
- [x] **Multi-Provider** - No vendor lock-in
- [x] **Local AI** - MLX, AFM, Ollama support
- [x] **Proactive Teaching** - Maestri guide, engage
- [x] **Maieutic Method** - Socratic teaching
- [x] **Storytelling** - Concepts as stories
- [x] **Multimodal** - Text, audio, visual, interactive

### Business Principles

- [x] **Open Source + Commercial** - Core free, premium paid
- [x] **Privacy-First Pricing** - BYOK option available
- [x] **Education Free Tier** - Sustainable free tier
- [x] **Customer Empowerment** - Enable users, not just serve

### Technical Principles

- [x] **Security from Day 1** - Not added later
- [x] **Accessibility from Day 1** - Built-in
- [x] **Privacy from Day 1** - Local-first
- [x] **Performance** - C-based core (efficient)
- [x] **Quality** - Testing, code reviews, documentation

---

## Part 6: Principles in Each V7 Document

### V7Plan.md (Core Architecture)
- ✅ Open source core (MIT)
- ✅ Multi-provider LLM router
- ✅ Plugin system (extensible)
- ✅ Education pack (17 Maestri)
- ✅ Local AI support (MLX, AFM)

### V7Plan-Enhanced.md (Web Platform)
- ✅ Telemetry opt-in (privacy-first)
- ✅ Multi-tenant architecture (isolation)
- ✅ Plugin security (sandboxing)

### V7Plan-Architecture-DeepDive.md (Deployment)
- ✅ Self-hosted option (privacy)
- ✅ State externalization (scalable)
- ✅ Security considerations

### V7Plan-Education-Cost-Analysis.md (Education Costs)
- ✅ Free tier sustainability
- ✅ GDPR compliance (Azure OpenAI)
- ✅ BYOK option

### V7Plan-Business-Case.md (Financial)
- ✅ BYOK model (privacy-first)
- ✅ Usage-based pricing (fair)
- ✅ Education free tier

### V7Plan-Ecosystem-Strategy.md (Ecosystem)
- ✅ Education as flagship
- ✅ Open source core
- ✅ Plugin marketplace (community-driven)

### V7Plan-Voice-WebPlatform.md (Voice & Web)
- ✅ Voice I/O (accessibility)
- ✅ Web Audio API (privacy-respecting)
- ✅ Offline fallback

### V7Plan-Billing-Security.md (Billing)
- ✅ PCI-DSS compliance (Stripe)
- ✅ GDPR compliance
- ✅ Privacy-first design

### V7Plan-10Year-Strategy.md (Long-term)
- ✅ Open source advantage
- ✅ Community lock-in (not vendor lock-in)
- ✅ Education focus

### V7Plan-CRITICAL-REVIEW.md (Unified Plan)
- ✅ All principles respected
- ✅ Unified architecture maintains principles
- ✅ Pricing model respects privacy

### V7Plan-PITCH.md (Investor Pitch)
- ✅ Open source core highlighted
- ✅ Education priority emphasized
- ✅ Privacy-first positioning

---

## Part 7: Principles Enforcement

### 7.1 Design Phase

**Checklist:**
- [ ] Core is open source (MIT)
- [ ] Privacy-first architecture
- [ ] Accessibility built-in
- [ ] Education prioritized
- [ ] Security by design
- [ ] Multi-provider support
- [ ] Local AI support

### 7.2 Implementation Phase

**Checklist:**
- [ ] Plugin sandboxing implemented
- [ ] Prompt injection protection active
- [ ] Content filtering for Education
- [ ] Accessibility features working
- [ ] Privacy controls in place
- [ ] GDPR compliance verified

### 7.3 Launch Phase

**Checklist:**
- [ ] Open source core released
- [ ] Free Education tier available
- [ ] BYOK option documented
- [ ] Accessibility tested
- [ ] Security audited
- [ ] Privacy policy published

---

## Part 8: Principles Violations (What We Avoid)

### ❌ What We DON'T Do

1. **Vendor Lock-In**
   - ❌ Force users to use one LLM provider
   - ✅ Support all providers, user chooses

2. **Data Collection**
   - ❌ Collect user data without consent
   - ✅ Opt-in telemetry, minimal by default

3. **Accessibility as Afterthought**
   - ❌ Add accessibility later
   - ✅ Built-in from day 1

4. **Education as Revenue Source**
   - ❌ Charge students for basic features
   - ✅ Free tier for students

5. **Security Added Later**
   - ❌ Add security after launch
   - ✅ Security from day 1

6. **Closed Source Core**
   - ❌ Make core proprietary
   - ✅ Core is open source (MIT)

7. **Cloud-Only**
   - ❌ Force cloud usage
   - ✅ Self-hosted option available

8. **Invented Data**
   - ❌ Make up facts
   - ✅ Always verify with tools

---

## Part 9: Principles in Action

### Example 1: Education Free Tier

**Principle:** Education priority, free for students

**Implementation:**
- Free tier: 3 teachers, 30 questions/month
- Cost: $0.03/user/month (sustainable)
- Funding: Grants + premium conversions
- Compliance: ✅ Respects principle

### Example 2: BYOK Model

**Principle:** Privacy first, no vendor lock-in

**Implementation:**
- Core: Free (open source)
- BYOK: Users bring own API keys
- Cost to us: $0 LLM costs
- Compliance: ✅ Respects principle

### Example 3: Accessibility Native

**Principle:** Accessibility built-in, not add-on

**Implementation:**
- Jenny agent: Accessibility champion (included)
- Adaptive UI: Adjusts automatically
- Voice I/O: For motor difficulties
- Compliance: ✅ Respects principle

### Example 4: Open Source Core

**Principle:** Core is open source, free forever

**Implementation:**
- Core C library: MIT license
- Plugin API: Open source
- CLI: Open source
- Compliance: ✅ Respects principle

---

## Part 10: Principles Monitoring

### Ongoing Verification

**Monthly Review:**
- [ ] All new features respect principles
- [ ] No vendor lock-in introduced
- [ ] Privacy maintained
- [ ] Accessibility not degraded
- [ ] Education remains priority

**Quarterly Audit:**
- [ ] Principles compliance review
- [ ] Security audit
- [ ] Accessibility testing
- [ ] Privacy policy update
- [ ] Open source license verification

**Annual Review:**
- [ ] Principles document update
- [ ] Compliance certification
- [ ] Community feedback integration
- [ ] Principles evolution (if needed)

---

## Conclusion

**All V7 plans respect Convergio's core principles:**

✅ **Open Source** - Core is MIT licensed  
✅ **Privacy First** - Data local, BYOK option  
✅ **Accessibility Native** - Built-in from day 1  
✅ **Education Priority** - Free tier, flagship  
✅ **Security by Design** - From day 1  
✅ **Inclusivity** - Person-first, no one left behind  
✅ **Truth & Verification** - Never invent data  
✅ **Multi-Provider** - No vendor lock-in  
✅ **Local AI** - MLX, AFM, Ollama support  
✅ **Proactive Teaching** - Maestri guide, engage  
✅ **Maieutic Method** - Socratic teaching  
✅ **Storytelling** - Concepts as stories  
✅ **Multimodal** - Text, audio, visual, interactive  

**These principles are non-negotiable and are embedded in all V7 plans.**

---

## Related Documents

**Master Index:** [V7Plan-MASTER-INDEX.md](./V7Plan-MASTER-INDEX.md) - Complete documentation hub

**Core Principles:**
- [CommonValuesAndPrinciples.md](../../../src/agents/definitions/CommonValuesAndPrinciples.md) - Convergio values
- [SAFETY_AND_INCLUSIVITY_GUIDELINES.md](../../../src/agents/definitions/education/SAFETY_AND_INCLUSIVITY_GUIDELINES.md) - Education safety
- [EducationManifesto.md](../../../education-pack/EducationManifesto.md) - Education vision
- [TeacherManifesto.md](../../../education-pack/TeacherManifesto.md) - Teaching principles

**V7 Plans:**
- [V7Plan-CRITICAL-REVIEW.md](./V7Plan-CRITICAL-REVIEW.md) - Unified plan (respects all principles)
- [V7Plan.md](./V7Plan.md) - Core architecture (open source)
- [V7Plan-Education-Cost-Analysis.md](./V7Plan-Education-Cost-Analysis.md) - Education sustainability

---

*This document ensures all V7 plans align with Convergio's fundamental principles. Any deviation must be justified and approved.*

