# Phase 3 - Educational Toolkit

**Status**: DONE
**Progress**: 100% P0 complete
**Last Updated**: 2025-12-20
**Parallelization**: 10 threads

---

## Objective

Implement educational tools shared by all teachers: mind maps, quizzes, flashcards, audio/TTS, calculator, video, language/science tools.

---

## Priority Legend

- **P0**: Essential for MVP
- **P1**: Important, phase 2
- **P2**: Nice-to-have

---

## Thread T1 - Mind Maps (P0)

**File**: `mindmap.c` (378 LOC)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK01 | Map generation engine | [x] | P0 | `mindmap_generate_mermaid()` |
| TK02 | Mermaid renderer | [x] | P0 | Templates included |
| TK03 | SVG export | [x] | P0 | `mindmap_export_svg()` |
| TK04 | PNG export | [x] | P0 | `mindmap_export_png()` |
| TK05 | PDF export | [x] | P1 | `mindmap_export_pdf()` |
| TK06 | `/mindmap <topic>` command | [x] | P0 | + DETAILED_HELP |

---

## Thread T2 - Quiz Engine (P0)

**File**: `quiz.c` (682 LOC)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK07 | Base quiz framework | [x] | P0 | Core engine |
| TK08 | Multiple choice | [x] | P0 | `QUIZ_MULTIPLE_CHOICE` |
| TK09 | True/False | [x] | P0 | `QUIZ_TRUE_FALSE` |
| TK10 | Open answer | [x] | P1 | `QUIZ_OPEN_ANSWER` |
| TK11 | Sequence ordering | [x] | P1 | `QUIZ_SEQUENCE` |
| TK12 | Pair matching | [x] | P1 | `QUIZ_MATCHING` |
| TK13 | Fill in blanks | [x] | P1 | `QUIZ_CLOZE` |
| TK14 | Image identification | [x] | P2 | `QUIZ_IMAGE_IDENTIFY` |
| TK15 | Adaptive difficulty | [x] | P0 | `quiz_adjust_difficulty()` |
| TK16 | Quiz PDF export | [ ] | P1 | TODO |
| TK17 | `/quiz` command | [x] | P0 | + DETAILED_HELP |

---

## Thread T3 - Flashcards + Spaced Repetition (P0)

**File**: `flashcards.c` (638 LOC)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK18 | Flashcards engine | [x] | P0 | Front/back |
| TK19 | SM-2 algorithm | [x] | P0 | `sm2_calculate_next_review()` |
| TK20 | Flashcard study UI | [x] | P0 | `flashcards_ui_study()` |
| TK21 | Anki export (.apkg) | [ ] | P1 | TODO |
| TK22 | PDF export | [ ] | P1 | TODO |
| TK23 | Auto generation | [x] | P0 | LLM extracts concepts |
| TK24 | `/flashcards` command | [x] | P0 | + DETAILED_HELP |
| TK25 | Anna review reminder | [x] | P1 | `anna_spaced_repetition_reminder()` |

---

## Thread T4 - Audio/TTS (P0)

**File**: `audio_tts.c` (494 LOC)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK26 | TTS engine | [x] | P0 | AVSpeechSynthesizer |
| TK27 | Audio summary | [x] | P0 | `tts_generate_audio()` |
| TK28 | M4A export | [x] | P0 | `audio_path` output |
| TK29 | Study playlist | [ ] | P1 | TODO |
| TK30 | Text-audio sync | [x] | P0 | `tts_generate_synced_text()` |
| TK31 | Adaptive speed | [x] | P0 | From profile |
| TK32 | `/audio` command | [x] | P0 | `/audio speak` |
| TK33 | Audiobooks | [x] | P1 | `chapter_audio[]` |

---

## Thread T5 - Math Tools (P0)

**File**: `calculator.c` (559 LOC)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK34 | Visual calculator | [x] | P0 | Step-by-step |
| TK35 | Digit color coding | [x] | P0 | Units/tens/hundreds |
| TK36 | Block visualization | [x] | P0 | Place value |
| TK37 | Equation solver | [x] | P0 | `calc_solve_equation()` |
| TK38 | Function graphs | [ ] | P1 | TODO |
| TK39 | Interactive geometry | [ ] | P1 | TODO |
| TK40 | Unit converter | [x] | P1 | `/convert` |
| TK41 | Formula reference | [ ] | P1 | TODO |
| TK42 | Visual fractions | [x] | P0 | Pizza/pie |
| TK43 | `/calc` command | [x] | P0 | Implemented |

---

## Thread T6 - Video/YouTube (P1)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK44 | Filtered YouTube search | [x] | P1 | Age filter |
| TK45 | Channel whitelist | [x] | P1 | Curated |
| TK46 | Preview safety | [ ] | P1 | TODO |
| TK47 | Video embed | [x] | P2 | Clickable link |
| TK48 | Documentaries | [ ] | P2 | TODO |
| TK49 | `/video` command | [x] | P1 | Implemented |

---

## Thread T7 - Language Tools (P0)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK50 | Contextual dictionary | [x] | P0 | `/define` |
| TK51 | Grammar analysis | [x] | P0 | `/grammar` |
| TK52 | Verb conjugator | [x] | P0 | `/conjugate` IT/EN/LAT |
| TK53 | Audio pronunciation | [x] | P0 | `/pronounce` + TTS |
| TK54 | Educational translator | [ ] | P1 | TODO |
| TK55 | Poetry metric analysis | [ ] | P2 | TODO |
| TK56 | Rhyme dictionary | [ ] | P2 | TODO |
| TK57 | CLI commands | [x] | P0 | All implemented |

---

## Thread T8 - Science Tools (P1)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK58 | Periodic table | [x] | P1 | `/periodic` |
| TK59 | 3D anatomy | [ ] | P1 | TODO |
| TK60 | Circuit simulator | [ ] | P2 | TODO |
| TK61 | Ecosystem simulator | [ ] | P2 | TODO |
| TK62 | Virtual chemistry lab | [ ] | P2 | TODO |
| TK63 | Planetarium | [ ] | P2 | TODO |
| TK64 | CLI commands | [x] | P1 | `/periodic` |

---

## Thread T9 - Creative Tools (P1)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK65 | Digital whiteboard | [ ] | P1 | TODO |
| TK66 | Interactive sheet music | [ ] | P1 | TODO |
| TK67 | Virtual keyboard | [ ] | P2 | TODO |
| TK68 | Color palette theory | [ ] | P2 | TODO |
| TK69 | Art gallery | [ ] | P1 | TODO |
| TK70 | Rhythm generator | [ ] | P2 | TODO |
| TK71 | CLI commands | [ ] | P1 | TODO |

---

## Thread T10 - Computer Science Tools (P1)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK72 | Safe Python sandbox | [ ] | P1 | TODO |
| TK73 | Scratch/Blockly sandbox | [ ] | P1 | TODO |
| TK74 | Flowchart builder | [ ] | P1 | TODO |
| TK75 | Step-by-step debug | [ ] | P2 | TODO |
| TK76 | Virtual robot | [ ] | P2 | TODO |
| TK77 | Pixel art | [ ] | P2 | TODO |
| TK78 | CLI commands | [ ] | P1 | TODO |

---

## Gamification (Bonus)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK79 | XP/Level system | [x] | P1 | `/xp` |
| TK80 | Badges/Achievements | [x] | P1 | `/xp badges` |
| TK81 | Daily streak | [x] | P1 | `/xp` shows streak |
| TK82 | Daily challenges | [ ] | P2 | TODO |
| TK83 | Crosswords | [ ] | P2 | TODO |
| TK84 | Celebrations | [ ] | P2 | TODO |

---

## Interactive HTML (P0)

**ADR**: `docs/adr/ADR-001-html-generator-llm-approach.md`

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK85 | HTML5 framework | [x] | P0 | `html_save()` |
| TK86 | CSS/JS animations | [x] | P0 | LLM generates HTML |
| TK87 | Euclid: Geometry | [x] | P0 | Via LLM |
| TK88 | Feynman: Simulators | [x] | P0 | Via LLM |
| TK89 | Darwin: Trees | [x] | P1 | Via LLM |
| TK90 | Mozart: Sheet music | [x] | P1 | Via LLM |
| TK91 | Herodotus: Timelines | [x] | P1 | Via LLM |
| TK92 | Leonardo: Gallery | [x] | P1 | Via LLM |
| TK93 | Save to lessons | [x] | P0 | `~/.convergio/lessons/` |
| TK94 | Browser auto-open | [x] | P0 | `html_open_in_browser()` |
| TK95 | Offline bundle export | [ ] | P1 | TODO |
| TK96 | `/html` command | [x] | P0 | `cmd_html()` |

---

## Modified Files

- `src/education/mindmap.c` (378 LOC)
- `src/education/quiz.c` (682 LOC)
- `src/education/flashcards.c` (638 LOC)
- `src/education/audio_tts.c` (494 LOC)
- `src/education/calculator.c` (559 LOC)
- `src/education/html_generator.c`
- `src/education/education_commands.c`

---

## Tests

| ID | Test | Status | Note |
|----|------|--------|------|
| TKT01 | Mind map test | [ ] | TODO |
| TKT02 | Quiz all types test | [ ] | TODO |
| TKT03 | Flashcards SM-2 test | [ ] | TODO |
| TKT04 | TTS profile test | [ ] | TODO |
| TKT05 | Calculator step test | [ ] | TODO |
| TKT06 | Language tools test | [ ] | TODO |

---

## Acceptance Criteria

- [x] All P0 implemented and working
- [x] CLI commands for each core tool
- [x] Interactive HTML via LLM
- [ ] PDF export for quiz/flashcard

---

## Result

All P0 tools complete. CLI working with `/mindmap`, `/quiz`, `/flashcards`, `/audio`, `/calc`, `/define`, `/conjugate`, `/pronounce`, `/grammar`, `/xp`, `/video`, `/periodic`, `/convert`, `/html`.
