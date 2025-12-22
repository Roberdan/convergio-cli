# Convergio Education Edition

## Learn from History's Greatest Teachers

Imagine having Socrates guide you through philosophy, Einstein explain physics, or Ada Lovelace teach you programming. **Convergio Education** brings 15 historical masters to life as AI tutors, each with their authentic teaching style and deep subject expertise.

---

## Why Convergio Education?

### Personalized Learning at Scale
Every student learns differently. Convergio Education adapts to individual learning styles, pace, and accessibility needs. Whether you have dyslexia, ADHD, or visual impairment, the platform adjusts to help you succeed.

### The Maieutic Method
Like Socrates, our teachers don't just give answers - they guide students to discover knowledge themselves. This builds deeper understanding and critical thinking skills.

### Multimodal Learning
- **Visual**: Mind maps, diagrams, interactive HTML
- **Auditory**: Voice interaction, pronunciation guides
- **Kinesthetic**: Interactive quizzes, flashcards with spaced repetition
- **Narrative**: Stories that make concepts memorable

### Real Curriculum Alignment
Mapped to Italian educational curricula from elementary to high school, including:
- Liceo Classico, Scientifico, Linguistico, Artistico
- ITIS, ITE, IPSIA
- Scuola Media and Elementare

---

## The 15 Maestri

| Maestro | Subject | Why They're Amazing |
|---------|---------|---------------------|
| **Euclide** | Mathematics | The father of geometry teaches logical reasoning |
| **Feynman** | Physics | Nobel laureate makes physics intuitive and fun |
| **Manzoni** | Italian | Master storyteller brings literature to life |
| **Darwin** | Sciences | Observation and curiosity drive discovery |
| **Erodoto** | History | The first historian makes the past vivid |
| **Humboldt** | Geography | Explorer who connects nature and culture |
| **Leonardo** | Art | Renaissance genius teaches creativity |
| **Shakespeare** | English | The Bard makes language memorable |
| **Mozart** | Music | Prodigy who composed at age 5 |
| **Cicerone** | Civics/Latin | Rome's greatest orator teaches rhetoric |
| **Smith** | Economics | Father of modern economics |
| **Lovelace** | Computer Science | First programmer, visionary thinker |
| **Ippocrate** | Health | "First, do no harm" - wellness wisdom |
| **Socrate** | Philosophy | Ask the right questions |
| **Chris** | Storytelling | Modern master of narrative craft |

---

## Coordination Team

| Agent | Role | Superpower |
|-------|------|------------|
| **Ali (Preside)** | School Principal | Coordinates all teachers, designs study plans |
| **Anna** | Assistant | Homework reminders, exam scheduling |
| **Jenny** | Accessibility Champion | Adapts content for special needs |

---

## Key Features

### Study Tools
```
/study <topic>       - Interactive study session with the right teacher
/quiz <topic>        - Generate a personalized quiz
/flashcards <topic>  - Spaced repetition flashcards (FSRS algorithm)
/mindmap <topic>     - Visual concept maps
/homework <text>     - Get step-by-step homework guidance
```

### Gradebook & Progress
```
/libretto            - Digital gradebook with all your grades
/xp                  - View experience points and achievements
```

### Language Tools
```
/define <word>       - Dictionary with etymology and usage
/conjugate <verb>    - Full verb conjugations
/pronounce <text>    - Hear correct pronunciation
/grammar             - Grammar explanations
```

### Reference Tools
```
/periodic            - Interactive periodic table
/convert <units>     - Unit conversions
/calc <expression>   - Scientific calculator
```

### Voice & Accessibility
```
/voice               - Enable voice interaction (TTS/STT)
/style <name>        - Adjust interface for your needs
```

---

## Accessibility First

Convergio Education is designed for **all** learners:

| Condition | Adaptations |
|-----------|-------------|
| **Dyslexia** | OpenDyslexic font, extra spacing, audio support |
| **Visual Impairment** | High contrast, screen reader compatible, large text |
| **ADHD** | Chunked content, timers, rewards, minimal distractions |
| **Motor Impairments** | Keyboard navigation, voice control |
| **Hearing Impairments** | Visual cues, transcripts |

---

## Spaced Repetition with FSRS

Our flashcard system uses the Free Spaced Repetition Scheduler (FSRS), the most advanced algorithm for long-term retention. Cards are scheduled based on your memory patterns, not arbitrary intervals.

---

## Mastery-Based Learning

Students must achieve 80% mastery before advancing. This ensures:
- No knowledge gaps
- Solid foundations for advanced topics
- Confidence in learned material

---

## Getting Started

```bash
# Build Education Edition
make build-edu
# or: make EDITION=education

# Run
./build/bin/convergio-edu

# Start a study session
/study quadratic equations

# Choose your teacher
@euclide Can you help me understand this problem?
```

## Testing

```bash
# Run all static tests (100+)
make test-edu

# Run real LLM interaction tests (requires Azure OpenAI)
make test-edu-llm

# Run all tests with verbose output
make test-edu-verbose

# Run full test suite with JSON output
make test-edu-full
```

---

## Technical Specs

- **Version Suffix**: `-edu`
- **Total Agents**: 18
- **Curricula Supported**: 8 Italian school types
- **Accessibility**: WCAG 2.1 AA compliant
- **Voice**: TTS (say) + STT (optional Whisper)

---

## Who Is This For?

- **Students (ages 6-19)**: From elementary to high school
- **Parents**: Support your child's learning at home
- **Teachers**: AI assistant for lesson planning and student support
- **Homeschoolers**: Complete educational support
- **Students with Special Needs**: Accessible learning for everyone

---

*"Education is not the filling of a pail, but the lighting of a fire." - W.B. Yeats*

*Copyright (c) 2025 Convergio.io - All rights reserved*
