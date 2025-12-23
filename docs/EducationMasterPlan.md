# Convergio Education Edition - Master Plan

**Last Updated**: 2025-12-22 19:45 CET

---

## 📊 CURRENT STATUS (22 Dec 2025)

### ✅ COMPLETED
| Item | Status |
|------|--------|
| Azure OpenAI deployments | ✅ GPT-5.2-edu, GPT-5-mini, GPT-5-nano deployed in Sweden Central |
| Edition system (edition.c) | ✅ Education edition locked at compile time |
| 17 Maestri definitions | ✅ All 17 teacher agents created and embedded |
| Agent categorization | ✅ Sciences, Humanities, Languages, Arts properly categorized |
| E2E test framework | ✅ Comprehensive test suite (50+ tests) |
| Database security fix | ✅ Fixed path security for ~/.convergio/ |
| Curie (Chemistry) agent | ✅ Created and tested |
| Galileo (Astronomy) agent | ✅ Created and tested |
| Ali's welcome message | ✅ Fixed "Scuola Virtuale" branding |

### 🔄 IN PROGRESS
| Item | Status |
|------|--------|
| Full E2E test pass | 🔄 Running, ~90% pass rate |
| Help command output | 🔄 Needs fix (test #10 failing) |

### ❌ NOT STARTED (PENDING)
| Item | Priority | Description |
|------|----------|-------------|
| Provider integration | HIGH | Use edition_get_preferred_model() in orchestrator |
| /onboarding command | HIGH | First-run student profile setup |
| Quiz study tool | HIGH | Interactive quiz generation |
| Flashcards study tool | HIGH | Spaced repetition flashcards |
| Mindmap study tool | MEDIUM | Visual concept mapping |
| Voice interaction | MEDIUM | Test Azure Realtime API |
| Accessibility testing | HIGH | Screen reader, high contrast |
| XP/Achievements system | LOW | Gamification for engagement |
| Libretto tracking | MEDIUM | Progress reports for parents |
| Makefile targets | LOW | `make test-edu` convenience |

### 🚫 DEFERRED
| Item | Reason |
|------|--------|
| Claude on Azure | Zero quota - requires manual request |

---

## Overview

The Education Edition is a specialized version of Convergio designed for K-12 students (ages 6-19). It provides a safe, accessible, and pedagogically sound learning environment with AI-powered tutoring.

---

## LLM Provider Configuration

### ADR-EDU-005: Azure OpenAI for Education

**Decision**: All Maestri in Education Edition use **Azure OpenAI** exclusively.

**Rationale**:
- GDPR compliance (data stays in EU region)
- Enterprise-grade security and SLAs
- Content filtering built-in for minors
- VirtualBPM credits system for cost control
- Consistent performance for educational use

**Configuration** (from `.env`):
```bash
# Azure OpenAI for Education (user provides their own keys)
AZURE_OPENAI_ENDPOINT=https://<your-resource>.openai.azure.com/
AZURE_OPENAI_API_KEY=<your-key>
AZURE_OPENAI_DEPLOYMENT=gpt-5.2-edu  # GPT-5.2 Chat for education
AZURE_OPENAI_API_VERSION=2025-01-01-preview

# For Education Realtime Voice
AZURE_OPENAI_REALTIME_ENDPOINT=https://<your-resource>.openai.azure.com/
AZURE_OPENAI_REALTIME_API_KEY=<your-key>
AZURE_OPENAI_REALTIME_DEPLOYMENT=gpt-4o-realtime
```

### VirtualBPM Azure Deployments (Sweden Central - EU Data Zone)

| Deployment Name | Model | Version | Use Case | Capacity |
|-----------------|-------|---------|----------|----------|
| `gpt-5.2-edu` | gpt-5.2-chat | 2025-12-11 | Premium education queries | 10K TPM |
| `gpt-5-edu-mini` | gpt-5-mini | 2025-08-07 | Standard education (cost-effective) | 20K TPM |
| `gpt-5-chat` | gpt-5-chat | 2025-10-03 | Emotional intelligence | 10K TPM |
| `gpt-5-nano` | gpt-5-nano | 2025-08-07 | Fast responses (low latency) | 30K TPM |
| `gpt-4o-realtime` | gpt-realtime | 2025-08-28 | Voice interaction | 1K TPM |
| `gpt4o-mini-deployment` | gpt-4o-mini | 2024-07-18 | Legacy/fallback | 100K TPM |

**Recommended for Education**:
- **Primary**: `gpt-5-edu-mini` (best balance of cost and quality)
- **Complex queries**: `gpt-5.2-edu` (latest model, best safety filters)
- **Quick responses**: `gpt-5-nano` (low latency for interactive use)
- **Voice**: `gpt-4o-realtime` (real-time audio)

### Claude on Azure (Microsoft Foundry)

Claude models are available via Microsoft Foundry in Sweden Central:
- `claude-opus-4-5` (20251101) - Requires quota request
- `claude-sonnet-4-5` (20250929) - Requires quota request
- `claude-haiku-4-5` (20251001) - Requires quota request

**Status**: Quota request pending. Claude requires separate AIServices resource.

**Important**:
- Each user/organization must configure their own Azure OpenAI instance
- Development/test keys are not distributed
- Schools can use their existing Azure subscriptions
- VirtualBPM provides managed instances for partners

**Consequences**:
- Azure OpenAI is primary provider for education
- Claude available as fallback (pending quota approval)
- Provider-specific optimizations needed
- Azure content safety filters active
- Costs tracked through VirtualBPM

**Implementation** (edition.c):
```c
// Edition-specific provider selection
int edition_get_preferred_provider(void);    // Returns PROVIDER_OPENAI for education
const char* edition_get_preferred_model(void);  // Returns "gpt-5.2-pro" for education
bool edition_uses_azure_openai(void);           // Returns true for education
```

**Why GPT-5.2 Pro**:
- Best-in-class content safety filters
- Advanced age-appropriate response generation
- Improved handling of sensitive educational topics
- Better multilingual support for Italian curriculum

---

## Core Principles

### 1. Edition Isolation

**CRITICAL REQUIREMENT**: Education Edition MUST be completely isolated from other editions.

| Principle | Description |
|-----------|-------------|
| No cross-edition agents | Students should never see business, enterprise, or developer agents |
| No corporate terminology | No "stakeholders", "ROI", "KPIs", "corporate strategy" |
| School-appropriate language | All content uses school-friendly vocabulary |
| Age-appropriate content | All responses filtered for minors (6-19 years) |

**Implementation**:
- Agent definitions are in `src/agents/definitions/education/`
- Agent filtering in `src/education/edition_filter.c`
- 17 Maestri + 3 coordination agents (Ali, Anna, Jenny) = 20 total
- Intent router restricted to education context

### 2. The 17 Maestri (Teachers)

| # | Name | Subject | Persona |
|---|------|---------|---------|
| 1 | Euclide | Mathematics | Patient, step-by-step explanations |
| 2 | Feynman | Physics | Analogies, real-world examples |
| 3 | Manzoni | Italian | Grammar, literature, language |
| 4 | Darwin | Science/Biology | Evolution, nature, organisms |
| 5 | Erodoto | History | Stories, chronology, context |
| 6 | Humboldt | Geography | Exploration, maps, ecosystems |
| 7 | Leonardo | Art | Visual thinking, creativity |
| 8 | Shakespeare | English | Language, literature, expression |
| 9 | Mozart | Music | Theory, appreciation, rhythm |
| 10 | Cicerone | Civics | Citizenship, democracy, rights |
| 11 | Smith | Economics | Markets, value, decisions |
| 12 | Lovelace | Computer Science | Coding, algorithms, logic |
| 13 | Ippocrate | Health/PE | Body, wellness, nutrition |
| 14 | Socrate | Philosophy | Critical thinking, ethics |
| 15 | Chris | Storytelling | Communication, narratives |
| 16 | Curie | Chemistry | Elements, reactions, experiments |
| 17 | Galileo | Astronomy | Space, planets, universe |

**Coordination Agents**:
| Name | Role |
|------|------|
| Ali | Principal (Preside) - welcomes students, coordinates |
| Anna | Executive Assistant - scheduling, reminders |
| Jenny | Accessibility Champion - inclusive design support |

### 3. Pedagogical Approach

All Maestri follow these teaching principles:

1. **Maieutic Method**: Guide students to discover answers through questions
2. **Person-First Language**: Focus on the student, not the disability
3. **Growth Mindset**: Encourage effort and learning, not innate ability
4. **No Judgment**: Mistakes are learning opportunities
5. **Age-Appropriate**: Content adapted to student's level
6. **Encouragement**: Positive reinforcement and support

### 4. Safety Requirements

| Requirement | Implementation |
|-------------|----------------|
| Prompt injection protection | Sanitized inputs, defense prompts |
| Self-harm content blocking | Redirect to trusted adults |
| Violence filtering | Educational context only |
| Adult content blocking | Strict filtering for all ages |
| Jailbreak prevention | Multiple defense layers |

---

## Study Tools

| Tool | Purpose | Implementation Status |
|------|---------|----------------------|
| Quiz | Self-assessment tests | Planned |
| Flashcards | Memory reinforcement | Planned |
| Mindmap | Visual organization | Planned |
| Notes | Structured note-taking | Planned |
| Summary | Text summarization | Planned |
| Timeline | Historical chronology | Planned |
| Pomodoro | Study timer | Planned |
| Calculator | Math assistance | Planned |
| Dictionary | Word definitions | Planned |

---

## Accessibility Features

### Required Accommodations

| Feature | Target Users | Implementation |
|---------|--------------|----------------|
| OpenDyslexic font | Dyslexia | Font option in settings |
| High contrast mode | Visual impairment | Color scheme option |
| Increased line spacing | Reading difficulties | Typography settings |
| TTS integration | Blind/low vision | External TTS support |
| Short responses | ADHD | Response length option |
| No metaphors mode | Autism spectrum | Literal language option |
| Keyboard navigation | Motor impairment | Full keyboard support |
| Screen reader compat | Blind users | Semantic output |

### Jenny - Accessibility Champion

Jenny is a specialized agent focused on:
- WCAG 2.1 AA compliance guidance
- Inclusive design recommendations
- Accommodations suggestions
- Universal Design for Learning (UDL)

---

## Commands

### Education-Specific Commands

| Command | Description |
|---------|-------------|
| `/education` | Main education dashboard |
| `/libretto` | View grades and progress |
| `/quiz` | Start a quiz session |
| `/flashcards` | Create/review flashcards |
| `/mindmap` | Create mind maps |
| `/pomodoro` | Study timer |
| `/profile` | Student profile |
| `/settings` | Accessibility settings |

### Blocked Commands

These commands are NOT available in Education Edition:
- `/project` (business)
- `/git`, `/pr`, `/commit` (developer)
- `/deploy`, `/docker` (operations)
- `/strategy`, `/okr` (enterprise)

---

## Testing Strategy

### Test Categories

1. **Edition Identity** (8 tests)
   - Verify education branding
   - Confirm Maestri visibility
   - Block non-education agents

2. **Menu & Navigation** (8 tests)
   - All education commands work
   - Help system accurate
   - Settings accessible

3. **Maestri Availability** (17 tests)
   - Each Maestro responds correctly
   - Subject matter expertise
   - Appropriate persona

4. **Study Tools** (10 tests)
   - Each tool functions
   - Output is useful
   - Integration works

5. **Pedagogy** (10 tests)
   - Maieutic method used
   - Patient explanations
   - Encouraging tone

6. **Lesson Examples** (10 tests)
   - Real curriculum content
   - Age-appropriate explanations
   - Cross-subject integration

7. **Accessibility** (10 tests)
   - All accommodations work
   - Settings persist
   - Jenny guidance accurate

8. **Safety** (10 tests)
   - Prompt injection blocked
   - Harmful content filtered
   - Redirects to adults

9. **Ali Preside** (10 tests)
   - Principal persona
   - Delegation to Maestri
   - Warm welcome

10. **Cross-Subject** (5 tests)
    - Subject integration
    - Maestri collaboration

11. **Progress Tracking** (5 tests)
    - Libretto functions
    - Statistics accurate

### Running Tests

```bash
# Run all static education tests (100+ tests)
./tests/e2e_education_comprehensive_test.sh

# Run with verbose output
./tests/e2e_education_comprehensive_test.sh --verbose

# Run specific section
./tests/e2e_education_comprehensive_test.sh --section 5
```

### Real LLM Interaction Tests

```bash
# Run real LLM tests using Azure OpenAI (50+ tests)
./tests/e2e_education_llm_test.sh

# Run with full response output
./tests/e2e_education_llm_test.sh --verbose

# Save all responses to JSON for analysis
./tests/e2e_education_llm_test.sh --save

# Test specific maestro only
./tests/e2e_education_llm_test.sh --maestro euclide
```

**LLM Test Categories**:
- Section 1: Azure OpenAI Connection
- Section 2: Ali Preside Real Conversations
- Section 3: Maestri Pedagogical Interactions (all 17)
- Section 4: Safety with Real LLM Filtering
- Section 5: Accessibility Real Interactions
- Section 6: Realistic Lesson Simulations
- Section 7: Voice and TTS Integration
- Section 8: Complete Student Journey

### Voice and Accessibility Tests

```bash
# Test voice integration (requires Azure Realtime)
./tests/e2e_education_llm_test.sh --section 7

# Test accessibility features
./tests/e2e_education_llm_test.sh --section 5
```

**Voice Tests**:
- Azure Realtime voice connection
- TTS for reading text
- Voice input for answers
- Accessibility voice commands

**Accessibility Tests**:
- OpenDyslexic font support
- High contrast mode
- ADHD short responses
- Screen reader compatibility
- Motor impairment alternatives

---

## Architecture Decisions

### ADR-EDU-001: Edition Isolation

**Decision**: Education Edition is completely isolated from other editions.

**Rationale**:
- Students should not be exposed to business/enterprise concepts
- Age-appropriate content requires strict filtering
- Simpler mental model for young users

**Consequences**:
- Separate agent definitions directory
- Edition-specific intent routing
- No agent sharing between editions

### ADR-EDU-002: 17 Maestri Limit

**Decision**: Limit to exactly 17 subject-specific teachers.

**Rationale**:
- Matches comprehensive school curriculum
- Covers all major subjects (including Chemistry and Astronomy)
- Each Maestro has clear expertise

**Consequences**:
- No dynamic agent addition
- Cross-subject topics handled by collaboration
- Jenny added as accessibility coordinator (not counted in 17)
- Total agents: 17 Maestri + 3 Coordinators = 20 agents

### ADR-EDU-003: Conversational Onboarding

**Decision**: Use LLM-based conversational onboarding instead of forms.

**Rationale**:
- More natural for students
- Reduces form fatigue
- Better accessibility
- Gathers information conversationally

**Consequences**:
- Depends on LLM availability
- Fallback to form mode needed
- Uses `conversational_config` module

### ADR-EDU-004: Person-First Language

**Decision**: All agents use person-first language.

**Rationale**:
- UN Disability-Inclusive Language Guidelines
- Research.com Inclusive Language Guide 2025
- Respects student identity

**Consequences**:
- Regular audits of agent definitions
- Training data review
- Prompt engineering for inclusivity

---

## Current Status (Brutally Honest)

### COMPLETED
| Task | Evidence |
|------|----------|
| 15 Maestri agent definitions | Files in `src/agents/definitions/education/` |
| Edition isolation architecture | `src/core/edition.c` with whitelist |
| Basic safety filtering | `edition_has_agent()` filtering |
| Ali Preside implementation | Agent file exists |
| Azure GPT-5 models deployed | `gpt-5.2-edu`, `gpt-5-edu-mini`, `gpt-5-chat`, `gpt-5-nano` on Sweden Central |
| E2E test scripts written | `tests/e2e_education_comprehensive_test.sh` (100+), `tests/e2e_education_llm_test.sh` (50+) |
| Master Plan documentation | This document |
| app-release-manager updated | Includes education tests in release checklist |

### NOT COMPLETED / NEEDS VERIFICATION
| Task | Status | Blocker |
|------|--------|---------|
| **Test scripts execution** | NOT VERIFIED | Need to run `./tests/e2e_education_comprehensive_test.sh` to confirm |
| **Makefile test targets** | MISSING | `make test-edu` not in ConvergioCLI-education Makefile |
| **Claude on Azure** | BLOCKED | Quota = 0 for Claude models, need to request quota via Azure portal |
| **Provider selection integration** | NOT INTEGRATED | `edition_get_preferred_provider()` exists but LLM calls don't use it yet |
| **/onboarding command** | BROKEN | Dispatches to wrong agent, reported in previous session |
| **Conversational onboarding** | PARTIAL | `conversational_config.c` written but not integrated |
| **Study tools (Quiz, Flashcards, etc.)** | NOT STARTED | Only planned |
| **Voice interaction** | NOT TESTED | Azure Realtime deployed but not tested |
| **Accessibility features** | NOT TESTED | Code exists but not verified |

### CRITICAL GAPS
1. **No one has actually RUN the tests** - test scripts written but execution not verified
2. **Provider selection not used** - `edition_get_preferred_model()` returns "gpt-5-edu-mini" but orchestrator doesn't call it
3. **Claude requires manual quota request** - zero quota available, must request via Azure portal

---

## Roadmap

### Phase 1: Foundation (Current)
- [x] 15 Maestri definitions
- [x] Edition isolation
- [x] Basic safety filtering
- [x] Ali Preside implementation
- [x] Test suite scripts written (100+ static, 50+ LLM)
- [x] Azure GPT-5 models deployed
- [ ] **TEST EXECUTION VERIFICATION** (CRITICAL)
- [ ] **Provider selection integration** (CRITICAL)
- [ ] Conversational onboarding integration
- [ ] Study tools implementation

### Phase 2: Engagement
- [ ] Gamification elements
- [ ] Achievement system
- [ ] Progress tracking
- [ ] Parent dashboard

### Phase 3: Advanced
- [ ] Voice interaction testing
- [ ] Multi-language support
- [ ] Curriculum alignment
- [ ] Teacher portal

---

## Quality Gates

Before any release:

1. **All 101 tests must pass** (`e2e_education_comprehensive_test.sh`)
2. **No offensive terminology** in agent definitions
3. **Safety tests green** (Section 8)
4. **Accessibility tests green** (Section 7)
5. **Edition isolation verified** (Section 1)

---

## References

- UN Disability-Inclusive Language Guidelines
- Research.com Inclusive Language Guide 2025
- OWASP LLM Security Top 10 2025
- OpenAI Teen Safety Measures 2025
- WCAG 2.1 AA Guidelines
- Universal Design for Learning (UDL) Framework

---

*Last Updated: 2025-12-22 22:00*
*Document Owner: Convergio Education Team*
*Version: 1.4.0*

---

## Changelog

### v1.4.0 (2025-12-22)
- **ADDED "Brutally Honest" status section** showing what's done vs not done
- Identified critical gaps: tests not run, provider selection not integrated
- Updated app-release-manager to include education tests in release checklist
- Clarified Claude quota blocker

### v1.3.0 (2025-12-22)
- Deployed GPT-5 models on Azure Sweden Central (EU Data Zone):
  - gpt-5.2-edu (GPT-5.2 Chat - latest)
  - gpt-5-edu-mini (GPT-5 Mini - cost-effective)
  - gpt-5-chat (emotional intelligence)
  - gpt-5-nano (low latency)
- Created AIServices resource for Claude (ai-virtualbpm-foundry)
- Claude models available but require quota request
- Documented deployment recommendations for education
- Updated model selection to use `gpt-5.2-chat` deployment

### v1.2.0 (2025-12-22)
- Added edition-specific provider configuration (Azure OpenAI for Education)
- Added `edition_get_preferred_provider()`, `edition_get_preferred_model()`, `edition_uses_azure_openai()`
- Updated build/test commands in Makefile
- Added helper scripts in `scripts/`

### v1.1.0 (2025-12-22)
- Added real LLM interaction tests with Azure OpenAI
- Added comprehensive test suite (100+ tests)
- Documented voice and accessibility testing

### v1.0.0 (2025-12-22)
- Initial Education Master Plan
- 15 Maestri definitions
- Edition isolation architecture
