# Education Pack - Architecture

**Last Updated**: 2025-12-20

---

## System Overview

```
                                    +-------------------------+
                                    |     ALI (Principal)     |
                                    |  Coordinates council    |
                                    |  Progress dashboard     |
                                    +-----------+-------------+
                                                |
              +---------------------------------+---------------------------------+
              |                                 |                                 |
              v                                 v                                 v
    +-----------------+              +-----------------+              +-----------------+
    | JENNY (A11y)    |              | ANNA (Assistant)|              | MARCUS (Memory) |
    | A11y profile    |              | Homework remind |              | Student progress|
    | Adaptations     |              | Scheduling      |              | Spaced repetit. |
    +-----------------+              +-----------------+              +-----------------+
              |                                 |                                 |
              +---------------------------------+---------------------------------+
                                                |
                                                v
                        +-------------------------------------------+
                        |            STUDENT PROFILE                 |
                        |  Name, Curriculum, Accessibility, Goals   |
                        +---------------------+---------------------+
                                              |
         +----------+----------+----------+---+---+----------+----------+----------+
         v          v          v          v   v   v          v          v          v
    +--------+ +--------+ +--------+ +--------+ +--------+ +--------+ +--------+
    |SOCRATES| | EUCLID | |FEYNMAN | |HERODOT.| |HUMBOLDT| |MANZONI | | ...    |
    +---+----+ +---+----+ +---+----+ +---+----+ +---+----+ +---+----+ +--------+
        |          |          |          |          |          |
        +----------+----------+----------+----------+----------+
                                    |
                    +---------------+---------------+
                    |      EDUCATIONAL TOOLKIT       |
                    |  (shared by all teachers)      |
                    +-------------------------------+
                                    |
    +-----------+-----------+-------+-------+-----------+-----------+
    v           v           v               v           v           v
+-------+  +-------+  +-----------+  +-----------+  +-------+  +-------+
| MIND  |  | QUIZ  |  | FLASHCARDS|  |   AUDIO   |  | CALC  |  | VIDEO |
| MAPS  |  |ENGINE |  |  SPACED   |  | SUMMARIES |  |ULATOR |  |YOUTUBE|
+-------+  +-------+  +-----------+  +-----------+  +-------+  +-------+
```

---

## Database Schema

```
+-------------------------------------------------------------------+
|                         SQLite Database                            |
+-------------------------------------------------------------------+
|                                                                    |
|  +------------------+    +------------------+                      |
|  | student_profiles |<---| accessibility_   |                      |
|  |                  |    |    settings      |                      |
|  +--------+---------+    +------------------+                      |
|           |                                                        |
|           v                                                        |
|  +------------------+    +------------------+                      |
|  |  student_goals   |    | learning_sessions|                      |
|  +------------------+    +------------------+                      |
|                                                                    |
|  +------------------+    +------------------+                      |
|  | learning_progress|    |  toolkit_outputs |                      |
|  +------------------+    +------------------+                      |
|                                                                    |
|  +------------------+    +------------------+                      |
|  |  flashcard_decks |<---| flashcard_reviews|                      |
|  +------------------+    +------------------+                      |
|                                                                    |
|  +------------------+    +------------------+                      |
|  |   quiz_history   |    |   gamification   |                      |
|  +------------------+    +------------------+                      |
|                                                                    |
|  +------------------+    +------------------+                      |
|  |curriculum_progress|   |      inbox       |                      |
|  +------------------+    +------------------+                      |
|                                                                    |
|  +------------------+    +------------------+                      |
|  | student_gradebook|    |    daily_log     |                      |
|  +------------------+    +------------------+                      |
|                                                                    |
+-------------------------------------------------------------------+
```

---

## Voice System Architecture

```
+-------------------------------------------------------------------+
|                        Voice Mode CLI                              |
|                      (/voice command)                              |
+-----------------------------+-------------------------------------+
                              |
                              v
+-------------------------------------------------------------------+
|                      Voice Gateway                                 |
|             (voice_gateway.c - orchestrator)                       |
+-------------------------------------------------------------------+
|  - Emotion detection                                               |
|  - Response adaptation                                             |
|  - Teacher voice switching                                         |
|  - Fallback chain management                                       |
+-----------------------------+-------------------------------------+
                              |
              +---------------+---------------+
              v               v               v
    +----------------+ +--------------+ +--------------+
    | Azure Realtime | |   OpenAI     | |    Local     |
    |  (primary)     | | (fallback)   | |   (last)     |
    +----------------+ +--------------+ +--------------+
              |
              v
    +--------------------------------------------+
    |           WebSocket Client                  |
    |     (voice_websocket.c - libwebsockets)    |
    +--------------------------------------------+
              |
              v
    +--------------------------------------------+
    |           Audio Subsystem                   |
    |     (voice_audio.m - CoreAudio)            |
    |  - Capture (microphone)                     |
    |  - Playback (speaker)                       |
    +--------------------------------------------+
```

---

## Edition System Architecture

```
+-------------------------------------------------------------------+
|                    CONVERGIO CORE CODEBASE                         |
|  (kernel, providers, orchestrator, memory, tools, UI, ACP)        |
+-------------------------------------------------------------------+
|                         BUILD SYSTEM                               |
|            make all | make EDITION=education                       |
+-------------+---------------------+-------------------------------+
|  EDUCATION  |      BUSINESS       |        DEVELOPER              |
|  Edition    |      Edition        |        Edition                |
+-------------+---------------------+-------------------------------+
| Agents:     | Agents:             | Agents:                       |
| - Teachers  | - Ali               | - Rex                         |
| - Ali       | - Fabio             | - Paolo                       |
| - Anna      | - Andrea            | - Baccio                      |
| - Jenny     | - Sofia             | - Dario                       |
+-------------+---------------------+-------------------------------+
| Features:   | Features:           | Features:                     |
| - Toolkit   | - CRM               | - Code Review                 |
| - Gradebook | - Pipeline          | - Architecture                |
| - Quiz      | - Analytics         | - CI/CD                       |
+-------------+---------------------+-------------------------------+
```

---

## File Structure

```
src/education/
+-- education.h              # Main header
+-- education_db.c           # Database operations (1338 LOC)
+-- education_commands.c     # CLI commands
+-- setup_wizard.c           # Profile wizard (745 LOC)
+-- accessibility_runtime.c  # A11y adaptations
+-- ali_preside.c            # Ali coordination (754 LOC)
+-- anna_integration.c       # Anna reminders (814 LOC)
+-- mindmap.c                # Mind maps (378 LOC)
+-- quiz.c                   # Quiz engine (682 LOC)
+-- flashcards.c             # Flashcards + SM-2 (638 LOC)
+-- audio_tts.c              # TTS/Audio (494 LOC)
+-- calculator.c             # Math tools (559 LOC)
+-- homework.c               # Homework helper
+-- study_session.c          # Pomodoro timer
+-- html_generator.c         # Interactive HTML
+-- libretto.c               # Student gradebook

src/voice/
+-- voice.h                  # Voice API
+-- voice_gateway.c          # Main orchestrator
+-- voice_websocket.c        # WebSocket client (620 LOC)
+-- voice_audio.m            # CoreAudio (macOS)
+-- voice_mode.c             # CLI mode
+-- openai_realtime.c        # OpenAI client
+-- azure_realtime.c         # Azure client

src/agents/definitions/education/
+-- socrate-filosofia.md
+-- euclide-matematica.md
+-- feynman-fisica.md
+-- erodoto-storia.md
+-- humboldt-geografia.md
+-- manzoni-italiano.md
+-- darwin-scienze.md
+-- leonardo-arte.md
+-- mozart-musica.md
+-- shakespeare-inglese.md
+-- cicerone-civica.md
+-- smith-economia.md
+-- lovelace-informatica.md
+-- ippocrate-corpo.md
+-- chris-storytelling.md

curricula/it/
+-- elementari.json
+-- scuola_media.json
+-- liceo_scientifico.json
+-- liceo_classico.json
+-- liceo_linguistico.json
+-- liceo_artistico.json
+-- iti_informatica.json
```

---

## Data Flow

```
User Input
    |
    v
+----------------+
|  CLI Parser    |
|  (commands.c)  |
+-------+--------+
        |
        v
+----------------+     +----------------+
|   Education    |---->|    Teacher     |
|   Commands     |     |   Selection    |
+-------+--------+     +-------+--------+
        |                      |
        v                      v
+----------------+     +----------------+
|    Student     |     |   A11y         |
|    Profile     |---->|   Runtime      |
+-------+--------+     +-------+--------+
        |                      |
        v                      v
+----------------+     +----------------+
|    Toolkit     |     |    LLM         |
|    (tools)     |---->|   Provider     |
+-------+--------+     +-------+--------+
        |                      |
        v                      v
+----------------+     +----------------+
|   Database     |     |   Response     |
|   (SQLite)     |     |   (adapted)    |
+----------------+     +----------------+
```

---

## Technology Stack

| Component | Technology |
|-----------|------------|
| Language | C (C11) |
| Database | SQLite 3 |
| TTS | AVSpeechSynthesizer (macOS) |
| Voice | Azure OpenAI Realtime |
| WebSocket | libwebsockets |
| Audio | CoreAudio (macOS) |
| HTTP | libcurl |
| JSON | cJSON |
| Build | Make |
| Testing | Unity test framework |
