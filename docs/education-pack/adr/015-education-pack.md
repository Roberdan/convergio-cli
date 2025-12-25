# ADR-015: MyConvergio Education Pack Architecture

**Status**: Proposed
**Date**: 2025-12-19
**Last Updated**: 2025-12-19
**Author**: Roberto with AI Team
**Plan Reference**: [EducationPackPlan.md](../plans/EducationPackPlan.md)
**Branch**: `feature/education-pack`
**Worktree**: `../ConvergioCLI-education`

---

## Context

Convergio needs to expand beyond technical users to serve the education market, specifically:

1. **Students with learning disabilities** (dyslexia, dyscalculia, ADHD, autism, cerebral palsy)
2. **Students needing personalized learning** (different learning styles, paces)
3. **Parents and educators** wanting transparent, adaptive learning support

The education technology market is growing rapidly, but there's a critical gap for:
- **Accessibility-first** learning platforms
- **Privacy-preserving** AI tutoring (no data sent to cloud)
- **Adaptive** systems that learn from each student's needs
- **Engaging** historical figures as teachers (not generic chatbots)

### Target User: Mario

Mario is 16 years old with:
- Dyslexia (moderate)
- Dyscalculia (mild)
- Cerebral palsy (affecting motor control)

Mario needs teachers who:
- Adapt their communication style to his needs
- Use visual representations for math
- Support voice input/output
- Never judge, always encourage
- Challenge appropriately without frustrating

---

## Decision

We will implement the **MyConvergio Education Pack** with:

### 1. 14 Historical Master Teachers

Each teacher is a great historical figure who embodies excellent pedagogy:

| ID | Master | Subject | Epoch | Superpower |
|----|--------|---------|-------|------------|
| ED01 | Socrates | Philosophy | 470-399 BC | Maieutic method |
| ED02 | Euclid | Mathematics | 300 BC | Step-by-step construction |
| ED03 | Richard Feynman | Physics | 1918-1988 | Simplification genius |
| ED04 | Herodotus | History | 484-425 BC | Storytelling |
| ED05 | Alexander von Humboldt | Geography | 1769-1859 | Connections & adventure |
| ED06 | Alessandro Manzoni | Italian Literature | 1785-1873 | Clear, accessible writing |
| ED07 | Charles Darwin | Natural Sciences | 1809-1882 | Observation & discovery |
| ED08 | Leonardo da Vinci | Art | 1452-1519 | Infinite curiosity |
| ED09 | Wolfgang Amadeus Mozart | Music | 1756-1791 | Joy & playfulness |
| ED10 | William Shakespeare | English | 1564-1616 | Universal stories |
| ED11 | Cicero | Civic Education | 106-43 BC | Rhetoric & citizenship |
| ED12 | Adam Smith | Economics | 1723-1790 | Intuition & examples |
| ED13 | Ada Lovelace | Computer Science | 1815-1852 | Computational thinking |
| ED14 | Hippocrates | Physical Ed/Health | 460-370 BC | Mind-body balance |

### 2. Comprehensive Didactic Toolkit

Tools shared by all teachers:

| Category | Priority | Tools |
|----------|----------|-------|
| Mind Maps | P0 | Mermaid generation, SVG/PNG/PDF export |
| Quiz Engine | P0 | 7 question types, adaptive difficulty |
| Flashcards | P0 | SM-2 spaced repetition, Anki export |
| Audio/TTS | P0 | M4A summaries, sync highlighting |
| Math Tools | P0 | Visual calculator, step-by-step solver |
| Language | P0 | Dictionary, conjugator, pronunciation |
| Video | P1 | Age-filtered YouTube search |
| Science | P1 | Periodic table, 3D anatomy |
| Creative | P1 | Whiteboard, music tools, art gallery |
| Coding | P1 | Python/Scratch sandbox, flowcharts |
| Gamification | P1 | XP, badges, streaks |

### 3. Deep Accessibility Integration

Adaptations for each condition:

#### Dyslexia
- OpenDyslexic font
- 1.5x line spacing, max 60 chars/line
- Cream background (#FDF6E3)
- TTS with synchronized highlighting
- Word syllabification

#### Dyscalculia
- Visual block representation (847 = [8][4][7])
- Color-coded place values
- Step-by-step ALWAYS visible
- Timer disabled for math
- Graphs instead of tables

#### Cerebral Palsy
- Voice input as primary
- Extended timeouts
- Frequent pause suggestions
- Large click areas
- Center-screen text positioning

#### ADHD
- Short responses (3-4 bullets max)
- Visible progress bar
- Micro-celebrations
- "Distraction parking" feature
- Focus mode (hide everything else)

#### Autism
- No metaphors or ambiguity
- Predictable response structure
- Topic change warnings
- "More details" always available
- No implicit social pressure

### 4. Curriculum System

Italian school curricula supported:
- Liceo Scientifico (1-5)
- Liceo Classico (1-5)
- Liceo Linguistico (1-5)
- Liceo Artistico (1-5)
- Istituto Tecnico Informatico
- Istituto Tecnico Commerciale
- Scuola Media (1-3)
- Elementari (1-5)
- Custom/Free Path

Curricula are JSON files that can be added without recompilation.

### 5. Coordination System

- **Ali as Principal**: Coordinates the virtual class council, sees all progress
- **Anna Integration**: Homework reminders, study session scheduling, spaced repetition alerts
- **Marcus Memory**: Tracks learning progress, remembers difficulties
- **Jenny Accessibility**: Manages accessibility profiles, suggests adaptations

---

## Architecture

```
                                ┌─────────────────────────┐
                                │     ALI (Principal)     │
                                │  Coordinates council    │
                                │  Global dashboard       │
                                └───────────┬─────────────┘
                                            │
          ┌─────────────────────────────────┼─────────────────────────────────┐
          │                                 │                                 │
          ▼                                 ▼                                 ▼
┌─────────────────┐              ┌─────────────────┐              ┌─────────────────┐
│ JENNY (A11y)    │              │ ANNA (Asst)     │              │ MARCUS (Memory) │
│ Profile mgmt    │              │ Reminders       │              │ Progress track  │
│ Adaptations     │              │ Scheduling      │              │ Spaced rep.     │
└─────────────────┘              └─────────────────┘              └─────────────────┘
          │                                 │                                 │
          └─────────────────────────────────┼─────────────────────────────────┘
                                            │
                                            ▼
                        ┌───────────────────────────────────────┐
                        │          STUDENT PROFILE              │
                        │  Name, Curriculum, A11y, Goals        │
                        └───────────────────┬───────────────────┘
                                            │
     ┌──────────┬──────────┬──────────┬─────┴─────┬──────────┬──────────┬──────────┐
     ▼          ▼          ▼          ▼           ▼          ▼          ▼          ▼
┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐
│SOCRATE │ │EUCLIDE │ │FEYNMAN │ │ERODOTO │ │HUMBOLDT│ │MANZONI │ │DARWIN  │ │  ...   │
└────┬───┘ └────┬───┘ └────┬───┘ └────┬───┘ └────┬───┘ └────┬───┘ └────┬───┘ └────────┘
     │          │          │          │          │          │          │
     └──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
                                       │
                       ┌───────────────┴───────────────┐
                       │      SHARED TOOLKIT           │
                       │  Mind maps, Quiz, Flashcards  │
                       │  Audio, Calculator, Video     │
                       └───────────────────────────────┘
```

---

## Database Schema

### Student Tables

```sql
-- Core student profile
CREATE TABLE student_profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    age INTEGER,
    curriculum_id TEXT NOT NULL,
    curriculum_year INTEGER DEFAULT 1,
    parent_contact TEXT,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    last_active INTEGER
);

-- Accessibility settings
CREATE TABLE student_accessibility (
    student_id INTEGER PRIMARY KEY REFERENCES student_profiles(id),
    dyslexia BOOLEAN DEFAULT FALSE,
    dyslexia_severity TEXT CHECK(dyslexia_severity IN ('mild', 'moderate', 'severe')),
    dyscalculia BOOLEAN DEFAULT FALSE,
    dyscalculia_severity TEXT,
    cerebral_palsy BOOLEAN DEFAULT FALSE,
    cerebral_palsy_notes TEXT,
    adhd BOOLEAN DEFAULT FALSE,
    adhd_type TEXT CHECK(adhd_type IN ('inattentive', 'hyperactive', 'combined')),
    autism BOOLEAN DEFAULT FALSE,
    autism_notes TEXT,
    visual_impairment BOOLEAN DEFAULT FALSE,
    hearing_impairment BOOLEAN DEFAULT FALSE,
    other_conditions TEXT,
    preferred_input TEXT DEFAULT 'keyboard' CHECK(preferred_input IN ('keyboard', 'voice', 'both')),
    preferred_output TEXT DEFAULT 'text' CHECK(preferred_output IN ('text', 'tts', 'both')),
    tts_speed REAL DEFAULT 1.0,
    tts_voice TEXT,
    font_family TEXT DEFAULT 'system',
    font_size INTEGER DEFAULT 16,
    high_contrast BOOLEAN DEFAULT FALSE,
    reduce_motion BOOLEAN DEFAULT FALSE,
    session_duration_minutes INTEGER DEFAULT 25,
    break_duration_minutes INTEGER DEFAULT 5
);

-- Learning goals
CREATE TABLE student_goals (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id INTEGER REFERENCES student_profiles(id),
    goal_type TEXT CHECK(goal_type IN ('short_term', 'medium_term', 'long_term')),
    description TEXT NOT NULL,
    target_date INTEGER,
    status TEXT DEFAULT 'active' CHECK(status IN ('active', 'achieved', 'abandoned')),
    created_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Progress tracking per topic
CREATE TABLE learning_progress (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id INTEGER REFERENCES student_profiles(id),
    maestro_id TEXT NOT NULL,
    topic TEXT NOT NULL,
    skill_level REAL DEFAULT 0.0 CHECK(skill_level >= 0.0 AND skill_level <= 1.0),
    last_interaction INTEGER,
    total_time_minutes INTEGER DEFAULT 0,
    quiz_attempts INTEGER DEFAULT 0,
    quiz_correct INTEGER DEFAULT 0,
    notes TEXT,
    UNIQUE(student_id, maestro_id, topic)
);

-- Individual learning sessions
CREATE TABLE learning_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id INTEGER REFERENCES student_profiles(id),
    maestro_id TEXT NOT NULL,
    topic TEXT,
    started_at INTEGER,
    ended_at INTEGER,
    duration_minutes INTEGER,
    engagement_score REAL,
    comprehension_score REAL,
    notes TEXT
);

-- Saved toolkit outputs (mind maps, quizzes, flashcards)
CREATE TABLE toolkit_outputs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id INTEGER REFERENCES student_profiles(id),
    tool_type TEXT NOT NULL CHECK(tool_type IN ('mindmap', 'quiz', 'flashcard', 'audio', 'note')),
    topic TEXT,
    content TEXT NOT NULL,
    format TEXT,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    last_accessed INTEGER
);

-- Flashcard spaced repetition data
CREATE TABLE flashcard_reviews (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    toolkit_output_id INTEGER REFERENCES toolkit_outputs(id),
    card_index INTEGER,
    ease_factor REAL DEFAULT 2.5,
    interval_days INTEGER DEFAULT 1,
    next_review INTEGER,
    review_count INTEGER DEFAULT 0,
    last_quality INTEGER
);

-- Performance indexes
CREATE INDEX idx_progress_student ON learning_progress(student_id);
CREATE INDEX idx_progress_maestro ON learning_progress(maestro_id);
CREATE INDEX idx_sessions_student ON learning_sessions(student_id);
CREATE INDEX idx_sessions_date ON learning_sessions(started_at);
CREATE INDEX idx_toolkit_student ON toolkit_outputs(student_id);
CREATE INDEX idx_flashcard_next ON flashcard_reviews(next_review);
```

---

## File Structure

```
src/
├── agents/definitions/education/    # 14 master teacher definitions
│   ├── socrate-filosofia.md
│   ├── euclide-matematica.md
│   ├── feynman-fisica.md
│   ├── erodoto-storia.md
│   ├── humboldt-geografia.md
│   ├── manzoni-italiano.md
│   ├── darwin-scienze.md
│   ├── leonardo-arte.md
│   ├── mozart-musica.md
│   ├── shakespeare-inglese.md
│   ├── cicerone-civica.md
│   ├── smith-economia.md
│   ├── lovelace-informatica.md
│   └── ippocrate-corpo.md
├── education/                       # Core education module
│   ├── education.h                  # Public API
│   ├── education.c                  # Main module
│   ├── profile.c                    # Student profile management
│   ├── curriculum.c                 # Curriculum engine
│   ├── adaptive.c                   # Adaptive learning system
│   └── tools/                       # Didactic toolkit
│       ├── mindmap.c                # Mind map generation
│       ├── quiz.c                   # Quiz engine
│       ├── flashcard.c              # Flashcards + SM-2
│       ├── audio.c                  # TTS + audio export
│       ├── calculator.c             # Visual calculator
│       ├── language.c               # Dictionary, conjugator
│       ├── video.c                  # YouTube search
│       └── gamification.c           # XP, badges, streaks

curricula/
├── it/                              # Italian curricula
│   ├── liceo_scientifico.json
│   ├── liceo_classico.json
│   ├── liceo_linguistico.json
│   ├── liceo_artistico.json
│   ├── iti_informatica.json
│   ├── itc.json
│   ├── scuola_media.json
│   └── elementari.json
└── custom_template.json             # Template for custom paths

include/nous/
└── education.h                      # Education module API

docs/
├── adr/
│   └── 015-education-pack.md        # This document
├── plans/
│   └── EducationPackPlan.md         # Detailed implementation plan
└── help/
    ├── education.md                 # /education command help
    ├── study.md                     # /study command help
    └── homework.md                  # /homework command help
```

---

## Implementation Phases

### Phase 1: Setup System (Sequential)
- Setup wizard (`/education setup`)
- Database schema creation
- Profile CRUD API
- Profile broadcasting to agents

### Phase 2: 14 Masters (7 Parallel Threads)
Each thread creates 2 masters simultaneously.

### Phase 3: Didactic Toolkit (10 Parallel Threads)
T1: Mind Maps | T2: Quiz | T3: Flashcards | T4: Audio
T5: Math | T6: Video | T7: Language | T8: Science
T9: Creative | T10: Coding | BONUS: Gamification

### Phase 4: Curriculum (3 Parallel Threads)
CUR-A: Licei | CUR-B: Medie/Elementari | CUR-C: Tecnici

### Phase 5: Didactic Features (4 Parallel Threads)
F1: Homework | F2: Study Sessions | F3: Progress | F4: Anna

### Phase 6: Accessibility (5 Parallel Threads)
A11Y-1: Dyslexia | A11Y-2: Dyscalculia | A11Y-3: Cerebral Palsy
A11Y-4: ADHD | A11Y-5: Autism

### Phase 7: Coordination (Sequential)
Ali as principal, cross-teacher communication

### Phase 8: Testing (5 Parallel Threads)
Unit tests, integration tests, user testing with real students

---

## Alternatives Considered

### Alternative 1: Generic AI Tutor (No Historical Figures)

**Rejected because**:
- Less engaging for students
- No unique personality or teaching style
- Missed opportunity for history education
- Less memorable experience

### Alternative 2: Cloud-Based Accessibility Analysis

**Rejected because**:
- Privacy concerns with sensitive disability data
- Requires internet connection
- Latency issues for real-time adaptations
- Our local-first philosophy

### Alternative 3: Single Universal Teacher Agent

**Rejected because**:
- Loses subject-specific expertise
- No cross-curricular coordination opportunities
- Less realistic school simulation
- Harder to maintain personality consistency

---

## Security Considerations

1. **Student Data Privacy**: All data stored locally, never sent to cloud
2. **Parental Access**: Optional parent contact with controlled visibility
3. **Content Filtering**: YouTube search filtered by age and educational relevance
4. **No External Services**: All AI processing uses configured providers
5. **COPPA Compliance**: Designed for minors with appropriate safeguards

---

## Performance Targets

| Metric | Target |
|--------|--------|
| Profile load | < 50ms |
| Accessibility adaptation | < 10ms |
| Mind map generation | < 2s |
| Quiz generation | < 3s |
| Flashcard review | < 50ms |
| Audio generation | < 5s per minute |
| Curriculum JSON parse | < 100ms |
| Spaced repetition calculation | < 5ms |

---

## Success Criteria

- [ ] 14 master teachers operational and tested
- [ ] P0 toolkit complete (mind maps, quiz, flashcards, audio, calculator)
- [ ] Intuitive setup wizard
- [ ] 3+ curricula complete (Liceo Sci, Classico, Medie)
- [ ] All P0 accessibility adaptations working
- [ ] Anna integration for reminders
- [ ] Ali operational as principal
- [ ] Testing with 5+ real students
- [ ] Feedback > 4/5 from users with disabilities

---

## References

- [EducationPackPlan.md](../plans/EducationPackPlan.md) - Detailed implementation plan
- [ADR-009: Anna Executive Assistant](./009-anna-executive-assistant.md) - Reminder integration
- [Convergio6Plan.md](../Convergio6Plan.md) - Master plan for v6.0 "Universalis"
- [OpenDyslexic Font](https://opendyslexic.org/) - Dyslexia-friendly font
- [SM-2 Algorithm](https://www.supermemo.com/en/archives1990-2015/english/ol/sm2) - Spaced repetition
- [WCAG 2.1 Guidelines](https://www.w3.org/WAI/WCAG21/quickref/) - Accessibility standards
