# Education Pack - Master Plan

**Created**: 2025-12-19
**Last Updated**: 2025-12-20
**Status**: 12/13 Phases Complete (Phase 13 Localization pending)
**Version**: 2.4 (stubs being replaced with real implementations)
**Branch**: `feature/education-pack`
**Worktree**: `/Users/roberdan/GitHub/ConvergioCLI-education`

---

## Vision

A virtual classroom council with the greatest historical teachers, equipped with advanced educational toolkit, coordinated by Ali (principal), who adapt to each student's specific needs.

**Pedagogical Principles**: Challenging but Achievable | Maieutics | Storytelling | Multimodal | Accessibility

---

## Quick Status

| # | Phase | Status | Progress | Blocking Issues | Link |
|---|-------|:------:|----------|-----------------|------|
| 1 | System Setup | ✅ | 100% | All P0 complete | [phase-01-setup.md](phases/phase-01-setup.md) |
| 2 | 15 Historical Teachers | ✅ | 100% | - | [phase-02-maestri.md](phases/phase-02-maestri.md) |
| 3 | Educational Toolkit | ✅ | 100% P0 | TKT01-06 tests done | [phase-03-toolkit.md](phases/phase-03-toolkit.md) |
| 4 | Italian Curricula | ✅ | 100% | JSON parser working | [phase-04-curriculum.md](phases/phase-04-curriculum.md) |
| 5 | Educational Features | ✅ | 100% | All fixed | [phase-05-features.md](phases/phase-05-features.md) |
| 6 | Accessibility | ✅ | 100% | AT04-09 tests done | [phase-06-accessibility.md](phases/phase-06-accessibility.md) |
| 7 | Coordination | ✅ | 100% | Ali fixed with 15 maestri | [phase-07-coordination.md](phases/phase-07-coordination.md) |
| 8 | Testing | ✅ | 100% | 36 education tests | [phase-08-testing.md](phases/phase-08-testing.md) |
| 9 | Verticalization | ✅ | 100% | Edition system complete | [phase-09-verticalization.md](phases/phase-09-verticalization.md) |
| 10 | Voice Interaction | ✅ | 100% | A11y profile loads | [phase-10-voice.md](phases/phase-10-voice.md) |
| 11 | Learning Science | ✅ | 100% | FSRS + Mastery done | [phase-11-learning-science.md](phases/phase-11-learning-science.md) |
| 12 | Storytelling | ✅ | 100% | All strings in English | [phase-12-storytelling.md](phases/phase-12-storytelling.md) |
| 13 | Localization | ⬜ | 0% | Architecture needed | [phase-13-localization.md](phases/phase-13-localization.md) |

**Legend**: ✅ Done | 🔄 In Progress | ⬜ Not Started

**Build**: Clean (0 errors, 0 warnings) | **Tests**: 132 total test functions (36 education-specific)

---

## Critical Issues (from Codex Audit)

| # | Issue | Status | Priority |
|---|-------|--------|----------|
| 1 | Curriculum parser is stub - doesn't load real JSON | ✅ Fixed | P0 |
| 2 | Wizard shows 15 curricula but only 8 JSON files exist | ✅ Fixed | P0 |
| 3 | Maestro IDs inconsistent (need slug as canonical) | ✅ Fixed | P0 |
| 4 | Ali Preside hardcodes wrong maestro IDs | ✅ Fixed | P0 |
| 5 | Voice mode doesn't load accessibility profile | ✅ Fixed | P1 |
| 6 | /xp persistence is fake | ✅ Fixed | P1 |
| 7 | /video command not implemented | ✅ Fixed | P2 |
| 8 | Story hooks had wrong IDs | ✅ Fixed | - |
| 9 | Italian strings in code | ✅ Fixed | - |

---

## Definition of Done

- [x] 15 teachers operational and tested
- [x] P0 Toolkit complete (mindmaps, quiz, flashcards, audio, calc)
- [x] Setup wizard working
- [x] 8 curricula complete (JSON files)
- [x] All A11y conditions supported (P0)
- [x] Anna integration working
- [x] Ali principal operational
- [x] Voice interaction working with all teachers
- [x] Mastery learning system active (FSRS + 80% threshold)
- [x] All code and strings in English
- [x] Curriculum parser loads real JSON data
- [ ] Test with 5+ real students
- [ ] Feedback >4/5 from users with disabilities

---

## Audit Log

### 2025-12-20 - Phase 9 Verticalization Complete + Architecture Decision

**Edition system implemented:**
- `include/nous/edition.h` - Types and public API (EDITION_MASTER as default)
- `src/core/edition.c` - Full implementation with agent/feature/command whitelists
- Agent filtering integrated into `registry.c`
- Edition-specific system prompts for Education, Business, Developer
- Build with `make EDITION=education` produces filtered binary
- 18 education agents (15 maestri + Ali, Anna, Jenny)
- Ali (Chief of Staff) included in ALL editions
- Anna (Executive Assistant) included in ALL editions
- Translated Italian strings in registry.c to English

**Architecture Decision (ADR-003):**
- Hybrid approach: Education = separate binary (security), others = runtime switching (future)
- See `docs/adr/ADR-003-edition-system-architecture.md`

**Edition Documentation Created:**
- `editions/README.md` - Overview with value propositions
- `editions/README-master.md` - Master edition (all 60+ agents)
- `editions/README-education.md` - Education pack (18 agents)
- `editions/README-business.md` - Business pack (10 agents)
- `editions/README-developer.md` - Developer pack (11 agents)

**Build optimization:**
- ccache support added to Makefile (incremental builds <1 second)

### 2025-12-20 - Full English Translation Complete

**All Italian strings translated to English across the codebase:**
- `education_commands.c` - /study, /homework, /quiz, /flashcards, /mindmap examples; entire /libretto command
- `commands.c` - help text for libretto and mindmap commands
- `setup_wizard.c` - accessibility step, profile sharing messages
- `education_db.c` - LLM error message, default system prompt
- `quiz.c` - grade comments (Excellent, Good work, Passing, Needs review)
- `mastery.c` - skill status labels (Mastered, Proficient, Familiar, In Progress, Not Started)
- `accessibility_runtime.c` - place values, celebration messages, metaphors

**Build optimization added:**
- ccache support for faster rebuilds (incremental builds <1 second)
- Parallel compilation with -j8

**All 9 critical issues from Codex audit now fixed.**

### 2025-12-20 - Stub Elimination Session

**Findings from Codex Analysis**:
1. Curriculum parser was a stub - doesn't parse real JSON subjects/topics
2. Wizard shows 15 curricula but only 8 JSON files exist
3. Maestro IDs inconsistent across system (need slug as canonical)
4. Ali Preside hardcodes wrong maestro IDs
5. Voice mode doesn't load accessibility profile
6. /xp persistence is fake
7. /video command not implemented
8. Story hooks used wrong IDs
9. Many strings still in Italian

**Corrections Applied**:
- Translated all Italian strings to English in storytelling.c
- Fixed story hooks to use correct English slug IDs (socrates-philosophy, etc.)
- Added Humboldt and Chris hooks (was missing)
- Now 15 maestri have story hooks with correct IDs
- Curriculum parser now loads real JSON data

### 2025-12-20 - Brutal Reality Check

**Findings**:
1. Multiple phases marked 100% had incomplete tasks
2. Test count was 253 but actual is 454+
3. Teacher test verifies 14/15 (missing chris-storytelling)
4. Many unit tests marked [ ] in phase files but integration tests pass
5. Compiler warnings present (should be 0)

**Corrections Applied**:
- Phase 1-6 status corrected from ✅ to 🔄
- Test count updated to 454
- Added "Blocking Issues" column to status table
- P0/P1 task counts recalculated

---

## Documents

| Document | Description |
|----------|-------------|
| [Architecture](architecture.md) | System diagrams and structure |
| [Execution Log](execution-log.md) | Event timeline and decisions |
| [Education README](../education/README.md) | Feature overview |
| [Editions Overview](../../editions/README.md) | All editions with value propositions |
| [ADR-001 HTML Generator](../adr/ADR-001-html-generator-llm-approach.md) | LLM vs Templates |
| [ADR-002 Voice](../adr/ADR-002-voice-interaction-architecture.md) | Voice architecture |
| [ADR-003 Edition System](../adr/ADR-003-edition-system-architecture.md) | Edition architecture |
| [Voice Setup](../voice/VOICE_SETUP.md) | Setup guide |

---

## Request Management

### How to track new requests

1. **Classification**: Bug Fix (BF), Enhancement (EN), New Feature (NF)
2. **ID Format**: `REQ-YYYYMMDD-NN` (e.g., REQ-20251220-01)
3. **Tracking**: Add to table below + appropriate phase file

### Active Requests

| ID | Type | Description | Phase | Status |
|----|------|-------------|-------|--------|
| REQ-20251220-16 | NF | Localization system (EN master, IT) | 13 | New |

### Completed Requests

| ID | Type | Description | Phase | Date |
|----|------|-------------|-------|------|
| REQ-20251219-01 | NF | LLM-based Interactive HTML | 3 | 2025-12-19 |
| REQ-20251220-01 | NF | Voice CLI + Audio | 10 | 2025-12-20 |
| REQ-20251220-02 | BF | Fix all compiler warnings | ALL | 2025-12-20 |
| REQ-20251220-03 | BF | Fix teacher test 14->15 | 2 | 2025-12-20 |
| REQ-20251220-04 | EN | Implement S18 Adaptive Learning API | 1 | 2025-12-20 |
| REQ-20251220-05 | EN | Add maieutic/a11y tests (MT02, MT03) | 2 | 2025-12-20 |
| REQ-20251220-06 | EN | Add ITE Commerciale curriculum | 4 | 2025-12-20 |
| REQ-20251220-11 | EN | Voice mode load a11y profile | 10 | 2025-12-20 |
| REQ-20251220-12 | EN | Implement real /xp persistence | 5 | 2025-12-20 |
| REQ-20251220-13 | BF | Fix /video command | 5 | 2025-12-20 |
| REQ-20251220-14 | BF | Translate Italian strings to English | 12 | 2025-12-20 |
| REQ-20251220-15 | BF | Fix story hooks IDs | 12 | 2025-12-20 |
| REQ-20251220-17 | EN | Full codebase English translation | ALL | 2025-12-20 |
| REQ-20251220-18 | EN | Build optimization (ccache) | - | 2025-12-20 |

---

## Instructions

> - Update Quick Status after each completed task
> - Each teacher reads accessibility profile before responding
> - Essential tools (P0) first, nice-to-have (P1/P2) after
> - **VERIFY IT COMPILES WITH 0 WARNINGS** before marking as DONE
> - **RUN TESTS** before marking as DONE
> - Parallelize as much as indicated in phase files
> - **ALL CODE AND STRINGS MUST BE IN ENGLISH**

---

## Stats

- **Total tasks**: 193
- **P0 tasks completed**: 101/101 (100%)
- **P1/P2 tasks completed**: ~85/92 (92%)
- **Max parallel threads**: 10
- **LOC education**: ~12000+
- **Test suites**: 10 (fuzz, unit, anna, compaction, plan_db, output, education, tools, web_search, help)
- **Total test functions**: 132
- **Education tests**: 36
- **Phases completed**: 12/13 (Phase 13 Localization pending)
- **Curricula JSON files**: 8 (in `curricula/it/`)

---

## Roadmap

### Phase 14: Proactive Teaching & Student Experience (P0) ⭐ NEW

**Reference**: [TeacherManifesto.md](TeacherManifesto.md)

This phase transforms teachers from passive responders to inspiring mentors.

#### 14.1 Welcome & First Contact

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| PT01 | Ali welcome message on first interaction | [ ] | P0 | Automatic greeting with name |
| PT02 | Detect first-time user vs returning | [ ] | P0 | Check profile exists |
| PT03 | Ali asks for name if not known | [ ] | P0 | Store in profile |
| PT04 | Use name throughout all interactions | [ ] | P0 | All 15 maestri + Ali |

#### 14.2 Student Profile System

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| SP01 | Profile data structure | [ ] | P0 | Name, age, grade, conditions, interests |
| SP02 | Multi-profile support | [ ] | P0 | Multiple students per device |
| SP03 | Profile selector at startup | [ ] | P1 | "Chi sei oggi?" |
| SP04 | Parent/guardian mode for setup | [ ] | P1 | Guided configuration |
| SP05 | Profile persistence (JSON) | [ ] | P0 | `~/.convergio/profiles/` |
| SP06 | Profile sharing between sessions | [ ] | P0 | All agents access same profile |

#### 14.3 First-Time Setup Wizard (with parent)

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| FW01 | Interactive Ali-guided setup | [ ] | P0 | Conversational, not forms |
| FW02 | Student name collection | [ ] | P0 | "Come ti chiami?" |
| FW03 | Age/grade level | [ ] | P0 | Adapts vocabulary |
| FW04 | Learning conditions assessment | [ ] | P1 | Dyslexia, ADHD, etc. |
| FW05 | Interests discovery | [ ] | P1 | For personalized examples |
| FW06 | Curriculum/study plan selection | [ ] | P1 | What are you studying? |
| FW07 | Initial skills assessment | [ ] | P2 | Optional diagnostic |
| FW08 | Feature tour | [ ] | P1 | Show all tools available |
| FW09 | Parent email for progress reports | [ ] | P2 | Optional |

#### 14.4 Proactive Teacher Behavior

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| PB01 | Inject proactivity rules in all prompts | [ ] | P0 | From TeacherManifesto |
| PB02 | Auto-propose mind maps | [ ] | P0 | After explanations |
| PB03 | Auto-propose HTML visuals | [ ] | P0 | For complex concepts |
| PB04 | Comprehension questions after explanations | [ ] | P0 | "Ti è chiaro?" |
| PB05 | Suggest next topics | [ ] | P1 | Based on curriculum |
| PB06 | Celebrate progress | [ ] | P0 | Encouragement phrases |

#### 14.5 Education Tools - Fix & Improve

**Current Issues:**
- HtmlInteractive tool not registered → agents fail with max iterations
- MindMap uses external `mmdc` CLI instead of embedded Mermaid.js
- Quiz, Flashcards, Audio tools need verification
- No unified browser-based viewer for all visual content

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| TL01 | Register HtmlInteractive in tools.c | [x] | P0 | Tool enum + execution |
| TL02 | Increase max iterations to 15 | [x] | P0 | For complex HTML generation |
| TL03 | Rewrite MindMap with embedded Mermaid.js | [ ] | P0 | No external dependency |
| TL04 | Auto-open MindMap in browser | [ ] | P0 | Like HtmlInteractive |
| TL05 | Verify Quiz tool is callable | [ ] | P0 | Check registration |
| TL06 | Verify Flashcards tool is callable | [ ] | P0 | Check registration |
| TL07 | Verify Audio tool is callable | [ ] | P1 | TTS integration |
| TL08 | Dedicated lesson folder | [ ] | P0 | `~/Documents/ConvergioEducation/` |
| TL09 | Auto-create folder if missing | [ ] | P0 | On first save |
| TL10 | Template library for common visuals | [ ] | P1 | Geometry, timelines, etc. |

**Mermaid.js Browser Approach:**
```html
<!DOCTYPE html>
<html>
<head>
  <script src="https://cdn.jsdelivr.net/npm/mermaid/dist/mermaid.min.js"></script>
</head>
<body>
  <div class="mermaid">
    mindmap
      root((Topic))
        Branch 1
        Branch 2
  </div>
  <script>mermaid.initialize({startOnLoad:true});</script>
</body>
</html>
```
- No external CLI needed
- Works offline (bundle Mermaid.js)
- Exportable via browser print-to-PDF

#### 14.6 Empathy & Engagement

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| EE01 | Empathy rules in all teacher prompts | [ ] | P0 | From TeacherManifesto |
| EE02 | Never say "wrong" | [ ] | P0 | Use "Quasi! Proviamo così..." |
| EE03 | Acknowledge struggles | [ ] | P0 | "Questo è difficile..." |
| EE04 | Personalized encouragement | [ ] | P0 | Using student's name |
| EE05 | Warm session endings | [ ] | P1 | Summary + teaser |

#### 14.7 Human-Friendly Error Messages

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| ER01 | LLM error interpreter system | [ ] | P0 | Transform cryptic errors to human messages |
| ER02 | Agent-style adaptation | [ ] | P0 | Errors match agent's personality |
| ER03 | Empathetic error messages | [ ] | P0 | "Mi dispiace, qualcosa non ha funzionato..." |
| ER04 | Actionable suggestions | [ ] | P1 | What to try next |
| ER05 | Hide technical details | [ ] | P0 | No stack traces for students |

**Example transformation**:
```
BEFORE: "Agent 'euclide-matematica' exceeded maximum iterations (5)"
AFTER:  "Euclide: Mi dispiace, ho avuto qualche difficoltà a creare
        la pagina. Proviamo in modo diverso - posso spiegarti il
        concetto con un disegno più semplice?"
```

#### 14.8 Document & Media Upload (School Materials)

**Purpose**: Allow students to upload school lesson materials (textbooks, PDFs, images, photos of handwritten notes) so teachers can help with specific school assignments.

**Key Principle**: This mode runs PARALLEL to the established curriculum - students can deep-dive into school-assigned topics with all Convergio tools.

##### Implementation Strategy (December 2025)

**Native LLM Document Processing** - Both Claude and OpenAI now support direct document input:

| Provider | API Endpoint | Supported Formats | Max Size | Max Pages |
|----------|--------------|-------------------|----------|-----------|
| Claude | Files API (Beta) | PDF, DOCX, TXT, CSV, HTML, RTF, EPUB, images | 500 MB/file | 100 pages |
| OpenAI | Chat Completions | PDF, DOCX, PPTX, XLSX, images | 32 MB/request | 100 pages |

**Key Capability**: Both APIs extract text AND visual content (charts, diagrams, formulas) from documents!

##### Implementation Tasks

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| DU01 | File picker with restricted navigation | [ ] | P0 | Only Desktop, Documents, Downloads |
| DU02 | Simple folder navigation UI | [ ] | P0 | Friendly to students |
| DU03 | Use Claude Files API for upload | [ ] | P0 | file_id reusable across calls |
| DU04 | Use OpenAI file input for vision | [ ] | P0 | Direct PDF/image to chat |
| DU05 | Camera access for photo capture | [ ] | P1 | AVFoundation on macOS |
| DU06 | OCR via LLM vision (native) | [ ] | P1 | Claude/OpenAI vision = built-in OCR |
| DU07 | Document context injection | [ ] | P0 | Pass file_id or base64 content |
| DU08 | Topic extraction from document | [ ] | P1 | LLM prompt: "What subject is this?" |
| DU09 | Route to appropriate teacher | [ ] | P1 | Based on detected subject |
| DU10 | Explain concept from textbook | [ ] | P0 | "Explain this paragraph" |
| DU11 | Adapt visuals for topic | [ ] | P0 | MindMaps, HTML for school topic |
| DU12 | PPTX support (presentations) | [ ] | P1 | Common for school slides |
| DU13 | XLSX support (spreadsheets) | [ ] | P2 | For data-related lessons |

##### Claude Files API Usage (Preferred)
```c
// 1. Upload file once (500 MB max)
POST /v1/files
Content-Type: multipart/form-data
file: <PDF binary>
purpose: "user_data"

Response: { "id": "file-abc123", "filename": "math_textbook.pdf" }

// 2. Reference in messages (no re-upload!)
{
  "model": "claude-sonnet-4-20250514",
  "messages": [{
    "role": "user",
    "content": [
      { "type": "file", "file_id": "file-abc123" },
      { "type": "text", "text": "Explain page 42 about fractions" }
    ]
  }]
}
```

##### OpenAI PDF Input (Alternative)
```c
// Direct file in chat completion (32 MB max)
{
  "model": "gpt-4o",
  "messages": [{
    "role": "user",
    "content": [
      {
        "type": "file",
        "file": { "data": "<base64_pdf>", "type": "application/pdf" }
      },
      { "type": "text", "text": "Explain this page" }
    ]
  }]
}
```

**User Flow**:
1. Student: "Ho bisogno di aiuto con questa pagina del libro"
2. Ali: "Certo! Carica la pagina - puoi fotografarla o cercare un PDF"
3. [Simple file picker: Desktop / Documents / Downloads]
4. [File uploaded to Claude Files API → file_id stored in session]
5. Ali: "Vedo che studi le frazioni! Chiamo Euclide..."
6. Euclide: [Receives file_id, explains the exact content from the textbook]

**Security Sandbox**:
- ONLY access Desktop, Documents, Downloads
- No access to system folders, Applications, etc.
- Files processed via LLM provider API (encrypted in transit)
- Session-only file_id references (cleared on exit)

**Cross-Edition Note**: Document upload is valuable for ALL editions:
- Business: Contract review, financial reports
- Developer: Code review, architecture diagrams
- Master: All document types

**Sources**:
- [Claude Files API](https://platform.claude.com/docs/en/build-with-claude/files)
- [Claude PDF Support](https://docs.claude.com/en/docs/build-with-claude/pdf-support)
- [OpenAI PDF Files Guide](https://platform.openai.com/docs/guides/pdf-files)

#### Cross-Edition Applicability

| Feature | Education | Business | Developer | Master |
|---------|:---------:|:--------:|:---------:|:------:|
| Welcome message | ✅ | ✅ | ✅ | ✅ |
| User name usage | ✅ | ✅ | ✅ | ✅ |
| Profile system | ✅ | ✅ | ✅ | ✅ |
| Proactive proposals | ✅ | ✅ | ✅ | ✅ |
| Document upload | ✅ | ✅ | ✅ | ✅ |
| Camera capture | ✅ | ⚪ | ⚪ | ✅ |
| Empathetic tone | ✅ | ✅ | ✅ | ✅ |
| Fast HTML/visuals | ✅ | ✅ | ✅ | ✅ |

---

### Phase 15: Additional Editions (P1)

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| E01 | Design Strategy Edition | [ ] | P1 | Domik, Satya, Antonio, Ethan, Evan, Angela |
| E02 | Design Creative Edition | [ ] | P1 | Jony, Sara, Stefano, Riccardo |
| E03 | Design Compliance Edition | [ ] | P2 | Elena, Dr. Enzo, Sophia, Guardian, Luca |
| E04 | Design HR Edition | [ ] | P2 | Giulia, Coach, Dave, Behice |
| E05 | Add whitelists to edition.c | [ ] | P1 | After E01-E04 design |
| E06 | Create README for each new edition | [ ] | P1 | Following existing pattern |

**Proposed Editions:**

| Edition | Agents | Target Audience |
|---------|--------|-----------------|
| Strategy | Ali, Domik, Satya, Antonio, Ethan, Evan, Angela, Anna | C-suite, consultants |
| Creative | Ali, Jony, Sara, Stefano, Riccardo, Anna | Designers, agencies |
| Compliance | Ali, Elena, Dr. Enzo, Sophia, Guardian, Luca, Anna | Legal, healthcare, gov |
| HR | Ali, Giulia, Coach, Dave, Behice, Anna | HR teams, people ops |

### Phase 16: Runtime Edition Switching (P2)

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| R01 | Refactor edition selection to runtime | [ ] | P2 | Keep compile-time for Education |
| R02 | Add `--edition` CLI flag | [ ] | P2 | `convergio --edition business` |
| R03 | Add `CONVERGIO_EDITION` env var | [ ] | P2 | Alternative to CLI flag |
| R04 | Add edition to config.toml | [ ] | P2 | Persistent setting |
| R05 | Hot-reload agent registry on switch | [ ] | P2 | Without restart |
| R06 | Update documentation | [ ] | P2 | CLI help, man page |

**Architecture**: See [ADR-003](../adr/ADR-003-edition-system-architecture.md)

### Phase 16: Licensing System (P3)

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| L01 | Design license key format | [ ] | P3 | Edition + expiry + features |
| L02 | Implement license validation | [ ] | P3 | Local + optional server check |
| L03 | Add license file storage | [ ] | P3 | `~/.convergio/license.key` |
| L04 | Grace period for expired licenses | [ ] | P3 | 7 days warning |
| L05 | Trial mode implementation | [ ] | P3 | 14 days full access |
| L06 | License UI in setup wizard | [ ] | P3 | Enter/validate key |

### Phase 17: Distribution & Installers (P3)

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| D01 | macOS DMG per edition | [ ] | P3 | Convergio-Education-5.3.1.dmg |
| D02 | macOS PKG installer | [ ] | P3 | For enterprise deployment |
| D03 | Homebrew formula | [ ] | P3 | `brew install convergio` |
| D04 | Linux AppImage | [ ] | P3 | Portable Linux binary |
| D05 | GitHub Actions release workflow | [ ] | P3 | Automated per-edition releases |
| D06 | Code signing (macOS) | [ ] | P3 | Apple Developer certificate |

### Phase 18: ACP & IDE Integration (P3)

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| A01 | ACP server per edition | [ ] | P3 | convergio-acp-edu, -biz, -dev |
| A02 | Zed extension per edition | [ ] | P3 | Marketplace submissions |
| A03 | VS Code extension | [ ] | P3 | Alternative to Zed |
| A04 | Edition-aware context in ACP | [ ] | P3 | Only show relevant agents |

---

## Roadmap Timeline (Tentative)

```
2025 Q1: Phase 14 (Additional Editions)
2025 Q2: Phase 15 (Runtime Switching)
2025 Q3: Phase 16 (Licensing) + Phase 17 (Distribution)
2025 Q4: Phase 18 (ACP & IDE Integration)
```

---

*Author: Roberto with AI agent team support*
