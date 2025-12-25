# Phase 2 - The 17 Historical Teachers

**Status**: DONE
**Progress**: 100%
**Last Updated**: 2025-12-23
**Parallelization**: 7 threads (2 teachers per thread)

---

## Objective

Create 17 educational agents based on historical figures, each specialized in a subject, with capability to generate interactive HTML content and adapt to accessibility profiles.

---

## The Teachers

| ID | Teacher | Subject | Primary Tools | Voice |
|----|---------|---------|---------------|-------|
| ED01 | Socrates | Philosophy | Mind maps, Flow diagrams, Open quizzes | verse |
| ED02 | Euclid | Mathematics | Visual calculator, Geometry, Graphs | sage |
| ED03 | Feynman | Physics | Simulators, Virtual lab, YouTube videos | echo |
| ED04 | Herodotus | History | Timeline, Geographic maps, Documentaries | verse |
| ED05 | Humboldt | Geography | Interactive maps, Image gallery | alloy |
| ED06 | Manzoni | Italian Literature | Dictionary, Grammar analysis, TTS | coral |
| ED07 | Darwin | Science | 3D Anatomy, Ecosystems, Periodic table | alloy |
| ED08 | Leonardo | Art | Whiteboard, Art gallery, Color palette | shimmer |
| ED09 | Mozart | Music | Sheet music, Keyboard, Rhythm generator | shimmer |
| ED10 | Shakespeare | English | Pronunciation, Translator, Conjugator | coral |
| ED11 | Cicero | Civic Education | Mind maps, Diagrams, Debate quizzes | verse |
| ED12 | Adam Smith | Economics | Infographics, Calculator, Charts | sage |
| ED13 | Lovelace | Computer Science | Coding sandbox, Flowchart, Debug | echo |
| ED14 | Hippocrates | Sports/Body | 3D Anatomy, Exercise timer, Video | alloy |
| ED15 | Chris | Storytelling | TED framework, Public speaking | echo |
| ED16 | Curie | Chemistry | Periodic table, Experiments, Lab safety | alloy |
| ED17 | Galileo | Astronomy | Star charts, Observations, Scientific method | sage |

---

## Tasks by Thread

### Thread 1 - Humanities

| ID | Task | Status | File |
|----|------|--------|------|
| M01 | Socrates (Philosophy) | [x] | `education/socrate-filosofia.md` |
| M02 | Manzoni (Italian) | [x] | `education/manzoni-italiano.md` |

### Thread 2 - Sciences

| ID | Task | Status | File |
|----|------|--------|------|
| M03 | Euclid (Mathematics) | [x] | `education/euclide-matematica.md` |
| M04 | Feynman (Physics) | [x] | `education/feynman-fisica.md` |

### Thread 3 - Historical-Geographic

| ID | Task | Status | File |
|----|------|--------|------|
| M05 | Herodotus (History) | [x] | `education/erodoto-storia.md` |
| M06 | Humboldt (Geography) | [x] | `education/humboldt-geografia.md` |

### Thread 4 - Science/Art

| ID | Task | Status | File |
|----|------|--------|------|
| M07 | Darwin (Science) | [x] | `education/darwin-scienze.md` |
| M08 | Leonardo (Art) | [x] | `education/leonardo-arte.md` |

### Thread 5 - Languages/Music

| ID | Task | Status | File |
|----|------|--------|------|
| M09 | Shakespeare (English) | [x] | `education/shakespeare-inglese.md` |
| M10 | Mozart (Music) | [x] | `education/mozart-musica.md` |

### Thread 6 - Extended 1

| ID | Task | Status | File |
|----|------|--------|------|
| M11 | Cicero (Civic Education) | [x] | `education/cicerone-civica.md` |
| M12 | Adam Smith (Economics) | [x] | `education/smith-economia.md` |

### Thread 7 - Extended 2

| ID | Task | Status | File |
|----|------|--------|------|
| M13 | Ada Lovelace (Computer Science) | [x] | `education/lovelace-informatica.md` |
| M14 | Hippocrates (Sports/Body) | [x] | `education/ippocrate-corpo.md` |
| M15 | Chris (Storytelling) | [x] | `education/chris-storytelling.md` |

---

## Modified Files

- `src/agents/definitions/education/*.md` (17 files)
- `src/agents/embedded_agents.c` (agent registration)
- `src/education/education.h` (teacher enum)

---

## Tests

| ID | Test | Status | Note |
|----|------|--------|------|
| MT01 | Verify 17 teacher files exist | [x] | `test_maestri_exist()` |
| MT02 | Test maieutic response for each teacher | [x] | `test_maestri_maieutic_prompts()` |
| MT03 | Test accessibility adaptation in responses | [x] | `test_maestri_accessibility_adaptation()` |

---

## Acceptance Criteria

- [x] 17 .md files created in `src/agents/definitions/education/`
- [x] Each teacher has primary tools defined
- [x] Each teacher has voice profile assigned
- [x] Interactive HTML available for all
- [x] Maieutic tests for each teacher

---

## Result

17 teachers operational with complete definitions (15 original + Curie + Galileo). Voice profiles assigned. Interactive HTML via LLM. All tests passing (MT01-MT03).
