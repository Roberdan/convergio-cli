# Convergio Education Pack - Feature Overview

> **The School of the Future** - AI-powered personalized learning with historical master teachers

## Quick Navigation

| Section | Description |
|---------|-------------|
| [Maestri (Teachers)](#maestri-ai-teachers) | 15 AI master teachers with unique personalities |
| [Voice Interaction](#voice-interaction) | Real-time voice conversations with emotion detection |
| [Learning Science](#learning-science) | Khan Academy + Duolingo engagement techniques |
| [Accessibility](#accessibility) | Support for dyslexia, ADHD, autism, and more |
| [Tools & Commands](#tools--commands) | Interactive educational tools |
| [Gamification](#gamification) | XP, badges, streaks, levels |
| [Architecture](#architecture) | Technical decisions (ADRs) |

---

## Maestri (AI Teachers)

Our 15 master teachers, each inspired by historical figures with distinct teaching styles:

### Core Subjects

| Maestro | Subject | Style | File |
|---------|---------|-------|------|
| **Euclide** | Mathematics | Patient, step-by-step, visual | [euclide-matematica.md](../../src/agents/definitions/education/euclide-matematica.md) |
| **Feynman** | Physics | Enthusiastic, analogies, experiments | [feynman-fisica.md](../../src/agents/definitions/education/feynman-fisica.md) |
| **Manzoni** | Italian | Literary, warm, grammatical rigor | [manzoni-italiano.md](../../src/agents/definitions/education/manzoni-italiano.md) |
| **Darwin** | Natural Sciences | Curious, methodical, observational | [darwin-scienze.md](../../src/agents/definitions/education/darwin-scienze.md) |
| **Erodoto** | History | Storyteller, narrative, dramatic | [erodoto-storia.md](../../src/agents/definitions/education/erodoto-storia.md) |
| **Humboldt** | Geography | Explorer, scientific, global | [humboldt-geografia.md](../../src/agents/definitions/education/humboldt-geografia.md) |

### Languages & Humanities

| Maestro | Subject | Style | File |
|---------|---------|-------|------|
| **Shakespeare** | English | Theatrical, poetic, playful | [shakespeare-inglese.md](../../src/agents/definitions/education/shakespeare-inglese.md) |
| **Socrate** | Philosophy | Maieutic questioning, dialectic | [socrate-filosofia.md](../../src/agents/definitions/education/socrate-filosofia.md) |
| **Cicerone** | Civic Education | Oratorical, persuasive, debate | [cicerone-civica.md](../../src/agents/definitions/education/cicerone-civica.md) |

### Arts & Creative

| Maestro | Subject | Style | File |
|---------|---------|-------|------|
| **Leonardo** | Art | Creative, visual, innovative | [leonardo-arte.md](../../src/agents/definitions/education/leonardo-arte.md) |
| **Mozart** | Music | Joyful, melodic, rhythmic | [mozart-musica.md](../../src/agents/definitions/education/mozart-musica.md) |
| **Chris** | Storytelling & Public Speaking | TED-style, inspiring, practical | [chris-storytelling.md](../../src/agents/definitions/education/chris-storytelling.md) |

### STEM & Modern

| Maestro | Subject | Style | File |
|---------|---------|-------|------|
| **Smith** | Economics | Analytical, practical, clear | [smith-economia.md](../../src/agents/definitions/education/smith-economia.md) |
| **Lovelace** | Computer Science | Logical, encouraging, creative | [lovelace-informatica.md](../../src/agents/definitions/education/lovelace-informatica.md) |
| **Ippocrate** | Health & PE | Caring, nurturing, wellness | [ippocrate-corpo.md](../../src/agents/definitions/education/ippocrate-corpo.md) |

### School Management

| Agent | Role | File |
|-------|------|------|
| **Ali** | Principal / Coordinator | [ali_preside.c](../../src/education/ali_preside.c) |
| **Anna** | Executive Assistant / Reminders | (integrated) |

---

## Voice Interaction

> **ADR**: [ADR-002-voice-interaction-architecture.md](../adr/ADR-002-voice-interaction-architecture.md)

### Technology Stack

**Primary**: Hume AI EVI 3 (Empathic Voice Interface)
- Ultra-low latency (<200ms)
- Best-in-class emotion detection
- 100,000+ custom voice personalities
- Claude integration

**Fallback**: OpenAI GPT-Realtime
- Native speech-to-speech
- Function calling support
- Good instruction following

### Features

- **Natural Conversation** - Speak with maestri as if they were real teachers
- **Emotion Detection** - System detects frustration, confusion, engagement
- **Interruption Handling** - Barge-in capability for natural dialog
- **Custom Voices** - Each maestro has unique voice personality
- **Multi-language** - Italian, English, Spanish, French, German

### Emotion-Aware Teaching

| Detected Emotion | System Response |
|-----------------|-----------------|
| Frustration | Slow down, simplify, offer break |
| Confusion | Rephrase, add visual, step back |
| Boredom | Add challenge, gamify, change pace |
| Excitement | Match energy, go deeper |
| Anxiety | Reassure, praise effort, reduce pressure |

---

## Learning Science

### Inspired by Khan Academy

> **Mastery Learning** - Students advance only after demonstrating understanding

- **Personalized Pace** - No one left behind, no one held back
- **Gap Detection** - AI identifies weak areas automatically
- **Visual Progress** - Clear skill trees and mastery levels
- **Immediate Feedback** - Know if you're right instantly

### Inspired by Duolingo

> **Engagement Engineering** - Making learning addictive (in a good way)

#### Gamification Elements
- **Streaks** - Daily learning builds habits (3.6x more engagement)
- **XP System** - Points for every activity
- **Badges** - Achievement milestones
- **Levels** - Progressive skill recognition
- **Leaderboards** - Optional friendly competition

#### Psychological Drivers
- **Variable Rewards** - Surprise bonuses keep motivation high
- **Loss Aversion** - Streak freeze prevents discouragement
- **Micro-rewards** - Constant positive reinforcement
- **Social Proof** - See classmates' progress

### Spaced Repetition

Using FSRS (Forecasting Spaced Repetition Schedule) algorithm:
- Review concepts at optimal intervals
- Predict forgetting curves per student
- Personalized review schedules
- Long-term retention optimization

---

## Accessibility

> **Every student deserves excellent education, regardless of ability**

### Supported Conditions

| Condition | Adaptations |
|-----------|-------------|
| **Dyslexia** | OpenDyslexic font, TTS, extra spacing, visual over text |
| **Dyscalculia** | Color-coded numbers, visual blocks, no timed tests |
| **ADHD** | Short sessions, gamification, immediate feedback, breaks |
| **Autism** | Predictable structure, explicit instructions, low sensory |
| **Cerebral Palsy** | Voice input, large touch targets, extended time |
| **Visual Impairment** | Screen reader support, high contrast, audio descriptions |

### Implementation

- Stored in student profile
- Applied automatically by every maestro
- Adjustable per session
- WCAG 2.2 AA compliant

---

## Tools & Commands

### Mathematics (`/calc`)
- Visual calculator with color-coded digits
- Step-by-step equation solver
- Fraction visualizer (pizza slices)
- Unit converter

### Linguistics (`/define`, `/conjugate`, `/pronounce`, `/grammar`)
- Dictionary with accessibility adaptations
- Verb conjugation (IT/EN/ES/FR/DE/LA)
- IPA pronunciation with audio
- Grammatical analysis

### Science (`/periodic`)
- Interactive periodic table
- Element details and properties
- (Future: anatomy, simulations)

### Media (`/video`)
- Educational YouTube search
- Curated safe channels
- Age-appropriate filtering

### Gamification (`/xp`)
- View XP and level
- Badge collection
- Streak status

### Visual Learning (`/html`)
- LLM-generated interactive visualizations
- Maestri create custom HTML for any concept
- Saved to `~/.convergio/education/lessons/`

---

## Gamification

### XP System

| Activity | XP Awarded |
|----------|------------|
| Complete lesson | 10 XP |
| Pass quiz | 25 XP |
| Perfect quiz | 50 XP |
| Daily streak | 5 XP bonus |
| Help classmate | 15 XP |
| Master skill | 100 XP |

### Levels

| Level | XP Required | Title |
|-------|-------------|-------|
| 1 | 0 | Principiante |
| 2 | 100 | Apprendista |
| 3 | 300 | Studente |
| 4 | 600 | Studioso |
| 5 | 1000 | Esperto |
| 6 | 1500 | Maestro |
| 7 | 2100 | Virtuoso |
| 8 | 2800 | Genio |

### Badges

- First Step - Complete first lesson
- Streak Master - 7 days in a row
- Quiz Champion - 5 perfect quizzes
- Curious Mind - Ask 10 questions
- Subject Expert - Master all skills in one subject
- Renaissance Student - Progress in all subjects

---

## Architecture

### Architectural Decision Records (ADRs)

| ADR | Topic | Status |
|-----|-------|--------|
| [ADR-001](../adr/ADR-001-html-generator-llm-approach.md) | HTML Generator - LLM vs Templates | Accepted |
| [ADR-002](../adr/ADR-002-voice-interaction-architecture.md) | Voice Interaction Architecture | Proposed |

### Core Files

```
src/education/
├── education_core.c          # Core education functions
├── education_db.c            # SQLite database layer
├── setup_wizard.c            # First-time setup + curricula
├── ali_preside.c             # Principal agent
├── features/
│   ├── study_session.c       # Pomodoro-based sessions
│   ├── homework.c            # Anti-cheating homework help
│   ├── flashcards.c          # SM-2 spaced repetition
│   ├── quiz.c                # Adaptive quizzing
│   └── mindmap.c             # Visual mind maps
└── tools/
    ├── calculator.c          # Visual calculator
    ├── linguistic_tools.c    # Dictionary, conjugation
    ├── html_generator.c      # HTML visualization wrapper
    └── (future) science_tools.c
```

### Database Schema

- `students` - Profile, preferences, accessibility
- `progress` - Per-subject mastery tracking
- `grades` - Libretto (grade book)
- `activity_log` - Daily activity diary
- `goals` - Learning objectives
- `streaks` - Gamification tracking

---

## Roadmap

### Completed
- 15 Maestri with unique personalities
- Accessibility runtime (6 conditions)
- Core tools (calculator, linguistics, periodic table)
- Gamification basics (XP, badges, streaks)
- HTML interactive visualizations
- Setup wizard with 16 curricula

### In Progress
- Voice interaction integration (Hume EVI 3)
- Storytelling as cross-cutting skill

### Planned
- Anatomy 3D visualization
- Physics simulations
- Code sandbox (Python, Scratch)
- Parent dashboard
- Multi-student classroom mode

---

## Philosophy

### The School of the Future

We believe education should be:

1. **Personalized** - Every student learns differently
2. **Engaging** - Learning should be exciting, not tedious
3. **Accessible** - No one left behind due to disability
4. **Human-Centered** - AI enhances teachers, doesn't replace them
5. **Joyful** - Curiosity is natural, boredom is a design failure

### Inspiration

- **Khan Academy** - Mastery learning, personalized pace
- **Duolingo** - Engagement engineering, habit formation
- **TED** - Storytelling, idea communication
- **Montessori** - Self-directed learning, follow the child
- **Feynman** - Explanation as understanding test

---

*Built with love for the next generation of learners.*
