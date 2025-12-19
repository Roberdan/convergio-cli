# Education Pack Implementation Plan

**Created**: 2025-12-19
**Last Updated**: 2025-12-20 00:15
**Status**: In Progress
**Progress**: 47/156 tasks (30%)
**Branch**: `feature/education-pack`
**Worktree**: `../ConvergioCLI-education`
**Goal**: Sistema educativo con maestri storici, toolkit didattico completo, accessibilita adattiva

---

## INSTRUCTIONS

> Aggiornare dopo ogni task completato.
> **PARALLELIZZARE** al massimo: usare tutti i thread indicati.
> Ogni maestro deve rileggere il profilo accessibilita prima di rispondere.
> Tool essenziali (P0) prima, nice-to-have (P1/P2) dopo.

---

## EXECUTIVE SUMMARY

**Visione**: Un consiglio di classe virtuale con i piu grandi maestri della storia, dotati di toolkit didattici avanzati, coordinati da Ali (preside), che si adattano alle esigenze specifiche di ogni studente.

**Principi Pedagogici**:
- **Challenging but Achievable**: Spingere oltre le capacita attuali, mai frustrare
- **Maieutica**: Guidare con domande, non servire risposte
- **Storytelling**: Ogni concetto diventa una storia coinvolgente
- **Multimodale**: Mappe mentali, quiz, audio, video per ogni stile di apprendimento
- **Accessibilita**: Adattamento totale a dislessia, discalculia, paralisi cerebrale, ADHD, autismo

---

## QUICK STATUS

```
FASE 1 (Setup):      Profilo studente + Setup wizard              → [~] IN PROGRESS (DB done, wizard TODO)
FASE 2 (Maestri):    14 Maestri storici [7 THREAD PARALLELI]      → [x] DONE (14/14)
FASE 3 (Toolkit):    Tool didattici [10 THREAD PARALLELI]         → [~] IN PROGRESS (5 core tools done)
FASE 4 (Curriculum): Liceo Scientifico + altri [3 THREAD]         → [~] IN PROGRESS (1/7 curricula)
FASE 5 (Features):   Quiz, compiti, study sessions [4 THREAD]     → [ ] TODO
FASE 6 (A11y):       Accessibilita profonda [5 THREAD PARALLELI]  → [ ] TODO
FASE 7 (Coord):      Ali preside + Anna reminder                  → [ ] TODO
FASE 8 (Test):       Test con utenti reali [5 THREAD]             → [ ] TODO
```

---

## ARCHITETTURA COMPLETA

```
                                    ┌─────────────────────────┐
                                    │     ALI (Preside)       │
                                    │  Coordina il consiglio  │
                                    │  Dashboard progressi    │
                                    └───────────┬─────────────┘
                                                │
              ┌─────────────────────────────────┼─────────────────────────────────┐
              │                                 │                                 │
              ▼                                 ▼                                 ▼
    ┌─────────────────┐              ┌─────────────────┐              ┌─────────────────┐
    │ JENNY (A11y)    │              │ ANNA (Assistant)│              │ MARCUS (Memory) │
    │ Profilo access. │              │ Reminder compiti│              │ Progressi stud. │
    │ Adattamenti     │              │ Scheduling      │              │ Spaced repetit. │
    └─────────────────┘              └─────────────────┘              └─────────────────┘
              │                                 │                                 │
              └─────────────────────────────────┼─────────────────────────────────┘
                                                │
                                                ▼
                        ┌───────────────────────────────────────────┐
                        │            STUDENT PROFILE                 │
                        │  Nome, Curriculum, Accessibilita, Goals   │
                        └───────────────────┬───────────────────────┘
                                            │
         ┌──────────┬──────────┬──────────┬─┴─┬──────────┬──────────┬──────────┐
         ▼          ▼          ▼          ▼   ▼          ▼          ▼          ▼
    ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐
    │SOCRATE │ │EUCLIDE │ │FEYNMAN │ │ERODOTO │ │HUMBOLDT│ │MANZONI │ │ ...    │
    └───┬────┘ └───┬────┘ └───┬────┘ └───┬────┘ └───┬────┘ └───┬────┘ └────────┘
        │          │          │          │          │          │
        └──────────┴──────────┴──────────┴──────────┴──────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │      TOOLKIT DIDATTICO        │
                    │  (condiviso da tutti i maestri) │
                    └───────────────────────────────┘
                                    │
    ┌───────────┬───────────┬───────┴───────┬───────────┬───────────┐
    ▼           ▼           ▼               ▼           ▼           ▼
┌───────┐  ┌───────┐  ┌───────────┐  ┌───────────┐  ┌───────┐  ┌───────┐
│ MAPPE │  │ QUIZ  │  │FLASHCARDS │  │  AUDIO    │  │CALCOLA│  │ VIDEO │
│MENTALI│  │ENGINE │  │  SPACED   │  │ RIASSUNTI │  │ TRICE │  │YOUTUBE│
└───────┘  └───────┘  └───────────┘  └───────────┘  └───────┘  └───────┘
```

---

## I 14 MAESTRI STORICI

| ID | Maestro | Materia | Tool Primari |
|----|---------|---------|--------------|
| ED01 | **Socrate** | Filosofia | Mappe mentali, Diagrammi flusso, Quiz aperti |
| ED02 | **Euclide** | Matematica | Calcolatrice visuale, Geometria, Grafici, Formulario |
| ED03 | **Feynman** | Fisica | Simulatori, Lab virtuale, Video YouTube |
| ED04 | **Erodoto** | Storia | Timeline, Mappe geografiche, Documentari |
| ED05 | **Humboldt** | Geografia | Mappe interattive, Galleria immagini, Video |
| ED06 | **Manzoni** | Italiano | Dizionario, Analisi grammaticale, TTS, Audiolibri |
| ED07 | **Darwin** | Scienze | Anatomia 3D, Ecosistemi, Tavola periodica |
| ED08 | **Leonardo** | Arte | Lavagna, Galleria opere, Palette colori |
| ED09 | **Mozart** | Musica | Spartiti, Tastiera, Generatore ritmi |
| ED10 | **Shakespeare** | Inglese | Pronuncia, Traduttore, Coniugatore, Video |
| ED11 | **Cicerone** | Ed. Civica | Mappe mentali, Diagrammi, Quiz dibattito |
| ED12 | **Adam Smith** | Economia | Infografiche, Calcolatrice, Grafici |
| ED13 | **Lovelace** | Informatica | Sandbox coding, Flowchart, Debug, Robot |
| ED14 | **Ippocrate** | Sport/Corpo | Anatomia 3D, Timer esercizi, Video |

---

## FASE 1 - SETUP SISTEMA (Sequenziale)

### 1.1 Setup Wizard

| ID | Task | Status | Note |
|----|------|--------|------|
| S01 | Comando `/education setup` | [ ] | Entry point |
| S02 | Step 1: Nome e info base | [ ] | Nome, eta, genitore (opz) |
| S03 | Step 2: Selezione curriculum | [ ] | Dropdown predefiniti |
| S04 | Step 3: Assessment accessibilita | [ ] | Checklist condizioni |
| S05 | Step 4: Preferenze input/output | [ ] | Voce, TTS, velocita |
| S06 | Step 5: Metodo studio attuale | [ ] | Personalizzazione |
| S07 | Step 6: Obiettivi personali | [ ] | Goals studente |
| S08 | Generazione profilo | [ ] | Salvataggio DB |
| S09 | Broadcast profilo a maestri | [ ] | Tutti caricano |

### 1.2 Database Schema (PARALLELO - 3 thread) ✅ COMPLETE

| ID | Task | Status | Thread |
|----|------|--------|--------|
| S10 | Schema `student_profiles` | [x] | Thread A |
| S11 | Schema `learning_progress` | [x] | Thread A |
| S12 | Schema `accessibility_settings` | [x] | Thread B |
| S13 | Schema `student_goals` | [x] | Thread B |
| S14 | Schema `learning_sessions` | [x] | Thread C |
| S15 | Schema `toolkit_outputs` (mappe, quiz salvati) | [x] | Thread C |

> **BONUS**: Implementati anche: `flashcard_decks`, `flashcard_reviews`, `quiz_history`, `gamification`, `curriculum_progress`, `inbox` (12 tabelle totali in `education_db.c`)

### 1.3 API Layer

| ID | Task | Status | Note |
|----|------|--------|------|
| S16 | API CRUD profile | [ ] | load/save/update |
| S17 | API profile broadcast | [ ] | Push a tutti i maestri |
| S18 | API adaptive learning | [ ] | Impara da interazioni |

---

## FASE 2 - I 14 MAESTRI (PARALLELO - 7 thread) ✅ COMPLETE

Ogni thread crea 2 maestri in parallelo.

### Thread 1 - Umanistici ✅
| ID | Task | Status | File |
|----|------|--------|------|
| M01 | Socrate (Filosofia) | [x] | `education/socrate-filosofia.md` |
| M02 | Manzoni (Italiano) | [x] | `education/manzoni-italiano.md` |

### Thread 2 - Scientifici ✅
| ID | Task | Status | File |
|----|------|--------|------|
| M03 | Euclide (Matematica) | [x] | `education/euclide-matematica.md` |
| M04 | Feynman (Fisica) | [x] | `education/feynman-fisica.md` |

### Thread 3 - Storico-Geografici ✅
| ID | Task | Status | File |
|----|------|--------|------|
| M05 | Erodoto (Storia) | [x] | `education/erodoto-storia.md` |
| M06 | Humboldt (Geografia) | [x] | `education/humboldt-geografia.md` |

### Thread 4 - Scienze/Arte ✅
| ID | Task | Status | File |
|----|------|--------|------|
| M07 | Darwin (Scienze) | [x] | `education/darwin-scienze.md` |
| M08 | Leonardo (Arte) | [x] | `education/leonardo-arte.md` |

### Thread 5 - Lingue/Musica ✅
| ID | Task | Status | File |
|----|------|--------|------|
| M09 | Shakespeare (Inglese) | [x] | `education/shakespeare-inglese.md` |
| M10 | Mozart (Musica) | [x] | `education/mozart-musica.md` |

### Thread 6 - Extended 1 ✅
| ID | Task | Status | File |
|----|------|--------|------|
| M11 | Cicerone (Ed. Civica) | [x] | `education/cicerone-civica.md` |
| M12 | Adam Smith (Economia) | [x] | `education/smith-economia.md` |

### Thread 7 - Extended 2 ✅
| ID | Task | Status | File |
|----|------|--------|------|
| M13 | Ada Lovelace (Informatica) | [x] | `education/lovelace-informatica.md` |
| M14 | Ippocrate (Sport/Corpo) | [x] | `education/ippocrate-corpo.md` |

---

## FASE 3 - TOOLKIT DIDATTICO (PARALLELO - 10 thread)

### Priorita Tool
- **P0 (Essenziale)**: Senza questi il pack non funziona
- **P1 (Importante)**: Migliora significativamente l'esperienza
- **P2 (Nice-to-have)**: Da fare se c'e tempo

---

### Thread T1 - MAPPE MENTALI (P0) - `mindmap.c` (378 LOC)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK01 | Engine generazione mappa da testo | [x] | P0 | `mindmap_generate_mermaid()` |
| TK02 | Renderer Mermaid | [x] | P0 | Templates Mermaid inclusi |
| TK03 | Export SVG | [ ] | P0 | Per modifica |
| TK04 | Export PNG | [ ] | P0 | Per stampa |
| TK05 | Export PDF | [ ] | P1 | Print-ready |
| TK06 | Comando `/mindmap <topic>` | [ ] | P0 | Entry point |

---

### Thread T2 - QUIZ ENGINE (P0) - `quiz.c` (682 LOC) ✅ CORE COMPLETE

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK07 | Framework quiz base | [x] | P0 | Core engine completo |
| TK08 | Tipo: Scelta multipla | [x] | P0 | `QUIZ_MULTIPLE_CHOICE` |
| TK09 | Tipo: Vero/Falso | [x] | P0 | `QUIZ_TRUE_FALSE` |
| TK10 | Tipo: Risposta aperta | [x] | P1 | `QUIZ_OPEN_ANSWER` |
| TK11 | Tipo: Riordina sequenza | [x] | P1 | `QUIZ_SEQUENCE` |
| TK12 | Tipo: Abbina coppie | [x] | P1 | `QUIZ_MATCHING` |
| TK13 | Tipo: Riempi vuoti | [x] | P1 | `QUIZ_CLOZE` |
| TK14 | Tipo: Identifica su immagine | [x] | P2 | `QUIZ_IMAGE_IDENTIFY` |
| TK15 | Generazione adattiva difficolta | [ ] | P0 | Auto-adjust |
| TK16 | Export quiz PDF | [ ] | P1 | Per stampare |
| TK17 | Comando `/quiz <topic> [n]` | [ ] | P0 | Genera n domande |

---

### Thread T3 - FLASHCARDS + SPACED REPETITION (P0) - `flashcards.c` (638 LOC)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK18 | Engine flashcards | [x] | P0 | Fronte/retro completo |
| TK19 | Algoritmo SM-2 (Anki-like) | [x] | P0 | `sm2_calculate_next_review()` |
| TK20 | UI studio flashcards | [ ] | P0 | Swipe/click |
| TK21 | Export Anki (.apkg) | [ ] | P1 | Compatibilita Anki |
| TK22 | Export PDF stampabile | [ ] | P1 | Fronte-retro |
| TK23 | Generazione auto da lezione | [ ] | P0 | LLM estrae concetti |
| TK24 | Comando `/flashcards <topic>` | [ ] | P0 | Entry point |
| TK25 | Anna reminder ripasso | [ ] | P1 | "E' ora di ripassare X" |

---

### Thread T4 - AUDIO/TTS (P0) - `audio_tts.c` (494 LOC)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK26 | TTS engine (AVSpeechSynthesizer) | [x] | P0 | `tts_speak()` |
| TK27 | Generazione riassunto audio | [x] | P0 | `tts_generate_audio()` |
| TK28 | Export M4A | [x] | P0 | `audio_path` output |
| TK29 | Playlist studio | [ ] | P1 | Collezione audio |
| TK30 | Sincronizzazione testo-audio | [x] | P0 | `tts_generate_synced_text()` |
| TK31 | Velocita adattiva a profilo | [x] | P0 | `TTSSettings` da profilo |
| TK32 | Comando `/audio <topic>` | [x] | P0 | `/audio speak` implementato |
| TK33 | Audiolibri/brani letteratura | [x] | P1 | `chapter_audio[]` per libri |

---

### Thread T5 - CALCOLATRICE & MATH TOOLS (P0) - `calculator.c` (559 LOC)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK34 | Calcolatrice visuale base | [x] | P0 | Step-by-step completo |
| TK35 | Codifica colore cifre | [x] | P0 | `COLOR_UNITS/TENS/HUNDREDS` |
| TK36 | Visualizzazione blocchi | [x] | P0 | Place value blocks |
| TK37 | Risolutore equazioni step-by-step | [ ] | P0 | Mai salta passaggi |
| TK38 | Grafici funzioni | [ ] | P1 | Plot f(x) |
| TK39 | Geometria interattiva | [ ] | P1 | Tipo GeoGebra lite |
| TK40 | Convertitore unita | [ ] | P1 | km↔m, kg↔g |
| TK41 | Formulario interattivo | [ ] | P1 | Cerca formula |
| TK42 | Frazioni visuali (pizza/torta) | [x] | P0 | `use_visual_fractions` |
| TK43 | Comando `/calc`, `/graph`, `/formula` | [ ] | P0 | Entry points |

---

### Thread T6 - VIDEO YOUTUBE & MULTIMEDIA (P1)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK44 | Ricerca YouTube filtrata | [ ] | P1 | Eta + argomento |
| TK45 | Whitelist canali educativi | [ ] | P1 | Solo verificati |
| TK46 | Preview video prima di proporre | [ ] | P1 | Safety check |
| TK47 | Embed video in risposta | [ ] | P2 | Link clickabile |
| TK48 | Documentari suggeriti | [ ] | P2 | Netflix/Prime edu |
| TK49 | Comando `/video <topic>` | [ ] | P1 | Cerca video |

---

### Thread T7 - STRUMENTI LINGUISTICI (P0)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK50 | Dizionario contestuale | [ ] | P0 | Popup in-place |
| TK51 | Analisi grammaticale | [ ] | P0 | Parti del discorso |
| TK52 | Coniugatore verbi | [ ] | P0 | IT/EN/LAT |
| TK53 | Pronuncia audio (IPA) | [ ] | P0 | Click → suono |
| TK54 | Traduttore didattico | [ ] | P1 | Mostra struttura |
| TK55 | Analisi metrica poesia | [ ] | P2 | Sillabe, figure |
| TK56 | Rimario | [ ] | P2 | Per poesia |
| TK57 | Comando `/define`, `/conjugate`, `/pronounce` | [ ] | P0 | Entry points |

---

### Thread T8 - STRUMENTI SCIENTIFICI (P1)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK58 | Tavola periodica interattiva | [ ] | P1 | Click → info |
| TK59 | Anatomia 3D semplificata | [ ] | P1 | Corpo esplorabile |
| TK60 | Simulatore circuiti base | [ ] | P2 | Fisica |
| TK61 | Simulatore ecosistemi | [ ] | P2 | Catena alimentare |
| TK62 | Lab virtuale chimica | [ ] | P2 | Reazioni sicure |
| TK63 | Planetario semplice | [ ] | P2 | Sistema solare |
| TK64 | Comando `/periodic`, `/anatomy`, `/simulate` | [ ] | P1 | Entry points |

---

### Thread T9 - STRUMENTI CREATIVI (P1)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK65 | Lavagna digitale | [ ] | P1 | Disegno libero |
| TK66 | Spartito interattivo | [ ] | P1 | Vedi + ascolta |
| TK67 | Tastiera virtuale | [ ] | P2 | Suona melodie |
| TK68 | Palette colori teoria | [ ] | P2 | Complementari, etc |
| TK69 | Galleria opere arte | [ ] | P1 | Museo virtuale |
| TK70 | Generatore ritmi | [ ] | P2 | Pattern musicali |
| TK71 | Comando `/draw`, `/music`, `/gallery` | [ ] | P1 | Entry points |

---

### Thread T10 - STRUMENTI INFORMATICA (P1)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK72 | Sandbox Python sicuro | [ ] | P1 | Esecuzione safe |
| TK73 | Sandbox Scratch/Blockly | [ ] | P1 | Visual coding |
| TK74 | Flowchart builder | [ ] | P1 | Algoritmi visuali |
| TK75 | Debug step-by-step | [ ] | P2 | Vedi esecuzione |
| TK76 | Robot virtuale | [ ] | P2 | Comandi → movimento |
| TK77 | Pixel art coordinate | [ ] | P2 | Intro programmazione |
| TK78 | Comando `/code`, `/flowchart`, `/robot` | [ ] | P1 | Entry points |

---

### Thread BONUS - GAMIFICATION (P1)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| TK79 | Sistema XP/Livelli | [ ] | P1 | Punti per attivita |
| TK80 | Badge/Achievement | [ ] | P1 | Traguardi visuali |
| TK81 | Streak giornaliero | [ ] | P1 | "5 giorni consecutivi!" |
| TK82 | Sfide giornaliere | [ ] | P2 | Mini-quiz del giorno |
| TK83 | Cruciverba tematici | [ ] | P2 | Ripasso ludico |
| TK84 | Celebrazioni animate | [ ] | P2 | Confetti, suoni |

---

## FASE 4 - CURRICULUM (PARALLELO - 3 thread)

### Thread CUR-A - Licei
| ID | Task | Status | File |
|----|------|--------|------|
| C01 | Parser curriculum JSON | [x] | In `education_db.c` |
| C02 | Liceo Scientifico (1-5) | [x] | `curricula/it/liceo_scientifico.json` (23KB) |
| C03 | Liceo Classico (1-5) | [ ] | `curricula/it/liceo_classico.json` |
| C04 | Liceo Linguistico (1-5) | [ ] | `curricula/it/liceo_linguistico.json` |
| C05 | Liceo Artistico (1-5) | [ ] | `curricula/it/liceo_artistico.json` |

### Thread CUR-B - Medie/Elementari
| ID | Task | Status | File |
|----|------|--------|------|
| C06 | Scuola Media (1-3) | [ ] | `curricula/it/scuola_media.json` |
| C07 | Elementari (1-5) | [ ] | `curricula/it/elementari.json` |

### Thread CUR-C - Tecnici/Custom
| ID | Task | Status | File |
|----|------|--------|------|
| C08 | Istituto Tecnico Informatico | [ ] | `curricula/it/iti_informatica.json` |
| C09 | Istituto Tecnico Commerciale | [ ] | `curricula/it/itc.json` |
| C10 | Sistema Percorso Libero | [ ] | Custom selection |
| C11 | Hot-reload JSON | [ ] | Watch file changes |

---

## FASE 5 - FEATURES DIDATTICHE (PARALLELO - 4 thread)

### Thread F1 - Homework Helper
| ID | Task | Status | Priority |
|----|------|--------|----------|
| F01 | Comando `/homework` | [ ] | P0 |
| F02 | Parser richiesta compito | [ ] | P0 |
| F03 | Modalita anti-cheating | [ ] | P0 |
| F04 | Hints progressivi | [ ] | P0 |
| F05 | Verifica comprensione finale | [ ] | P1 |
| F06 | Log per trasparenza genitori | [ ] | P1 |

### Thread F2 - Study Sessions
| ID | Task | Status | Priority |
|----|------|--------|----------|
| F07 | Comando `/study` | [ ] | P0 |
| F08 | Timer Pomodoro | [ ] | P0 |
| F09 | Notifiche native pause | [ ] | P0 |
| F10 | Mini-quiz fine sessione | [ ] | P1 |
| F11 | Tracking tempo/materia | [ ] | P1 |
| F12 | Suggerimenti pausa attiva | [ ] | P2 |

### Thread F3 - Progress Tracking
| ID | Task | Status | Priority |
|----|------|--------|----------|
| F13 | Dashboard progressi studente | [ ] | P0 |
| F14 | Tracker argomenti completati | [ ] | P0 |
| F15 | Suggeritore prossimo argomento | [ ] | P1 |
| F16 | Report genitori (PDF/email) | [ ] | P1 |
| F17 | Certificati completamento | [ ] | P2 |

### Thread F4 - Anna Integration
| ID | Task | Status | Priority |
|----|------|--------|----------|
| F18 | Connessione Anna ↔ Education | [ ] | P0 |
| F19 | Reminder compiti automatici | [ ] | P0 |
| F20 | Reminder spaced repetition | [ ] | P0 |
| F21 | Reminder pause ADHD | [ ] | P1 |
| F22 | Celebrazione completamenti | [ ] | P1 |

---

## FASE 6 - ACCESSIBILITA (PARALLELO - 5 thread)

### Thread A11Y-1 - Dislessia
| ID | Task | Status | Priority |
|----|------|--------|----------|
| DY01 | Font OpenDyslexic | [ ] | P0 |
| DY02 | Spaziatura 1.5x | [ ] | P0 |
| DY03 | Max 60 char/riga | [ ] | P0 |
| DY04 | Sfondo crema | [ ] | P0 |
| DY05 | TTS + highlighting sync | [ ] | P0 |
| DY06 | Sillabazione parole | [ ] | P1 |
| DY07 | Glossario popup | [ ] | P1 |

### Thread A11Y-2 - Discalculia
| ID | Task | Status | Priority |
|----|------|--------|----------|
| DC01 | Visualizzazione blocchi | [ ] | P0 |
| DC02 | Colori unita/decine/centinaia | [ ] | P0 |
| DC03 | Step-by-step SEMPRE | [ ] | P0 |
| DC04 | Grafici vs tabelle | [ ] | P1 |
| DC05 | Timer disabilitato math | [ ] | P0 |
| DC06 | Verifica metodi alternativi | [ ] | P1 |

### Thread A11Y-3 - Paralisi Cerebrale
| ID | Task | Status | Priority |
|----|------|--------|----------|
| CP01 | Voice input primario | [ ] | P0 |
| CP02 | Comandi vocali nav | [ ] | P0 |
| CP03 | Timeout estesi | [ ] | P0 |
| CP04 | Pause suggerite | [ ] | P1 |
| CP05 | Grandi aree click | [ ] | P1 |

### Thread A11Y-4 - ADHD
| ID | Task | Status | Priority |
|----|------|--------|----------|
| AD01 | Risposte brevi (3-4 bullet) | [ ] | P0 |
| AD02 | Progress bar visibile | [ ] | P0 |
| AD03 | Micro-celebrazioni | [ ] | P1 |
| AD04 | Parcheggio distrazioni | [ ] | P1 |
| AD05 | Modalita focus singolo | [ ] | P1 |
| AD06 | Gamification spinta | [ ] | P1 |

### Thread A11Y-5 - Autismo
| ID | Task | Status | Priority |
|----|------|--------|----------|
| AU01 | No metafore/ambiguita | [ ] | P0 |
| AU02 | Struttura prevedibile | [ ] | P0 |
| AU03 | Avvisi cambio topic | [ ] | P0 |
| AU04 | Opzione "piu dettagli" | [ ] | P1 |
| AU05 | No pressione sociale | [ ] | P0 |
| AU06 | Preferenze sensoriali | [ ] | P1 |

---

## FASE 7 - COORDINAMENTO (Sequenziale)

### Ali come Preside
| ID | Task | Status | Note |
|----|------|--------|------|
| AL01 | Estensione Ali ruolo preside | [ ] | Context education |
| AL02 | Dashboard studente per Ali | [ ] | Vede tutti progressi |
| AL03 | Consiglio classe virtuale | [ ] | Convoca maestri |
| AL04 | Report settimanale auto | [ ] | Sintesi |
| AL05 | Gestione casi difficili | [ ] | Escalation |
| AL06 | Comunicazione genitori | [ ] | Se configurato |

### Comunicazione Maestri
| ID | Task | Status | Note |
|----|------|--------|------|
| CM01 | Shared context studente | [ ] | Profilo condiviso |
| CM02 | Segnalazione cross-materia | [ ] | Collegamenti |
| CM03 | Progetti interdisciplinari | [ ] | Collaborazione |

---

## FASE 8 - TESTING (PARALLELO - 5 thread)

### Thread TEST-1 - Unit Tests
| ID | Task | Status |
|----|------|--------|
| T01 | Test profile CRUD | [ ] |
| T02 | Test curriculum loading | [ ] |
| T03 | Test toolkit tools | [ ] |
| T04 | Test quiz generation | [ ] |
| T05 | Test spaced repetition | [ ] |

### Thread TEST-2 - Integration Tests
| ID | Task | Status |
|----|------|--------|
| T06 | Test flusso completo | [ ] |
| T07 | Test coordinamento maestri | [ ] |
| T08 | Test Anna integration | [ ] |
| T09 | Test Ali preside | [ ] |

### Thread TEST-3/4/5 - User Testing (Parallelo)
| ID | Task | Status | Thread |
|----|------|--------|--------|
| T10 | Test studente dislessico | [ ] | TEST-3 |
| T11 | Test studente discalculico | [ ] | TEST-3 |
| T12 | Test studente ADHD | [ ] | TEST-4 |
| T13 | Test studente autistico | [ ] | TEST-4 |
| T14 | Test Mario (multi) | [ ] | TEST-5 |
| T15 | Iterazione feedback | [ ] | TEST-5 |

---

## PARALLELIZATION MAP COMPLETA

```
TIME ────────────────────────────────────────────────────────────────────────►

FASE 1 (Setup)
    [S01-S09] ──► [S10-S15 ▓▓▓ 3 thread] ──► [S16-S18]

FASE 2 (Maestri) - 7 THREAD PARALLELI
    Thread 1 ─► M01, M02 (Socrate, Manzoni)      ─┐
    Thread 2 ─► M03, M04 (Euclide, Feynman)       │
    Thread 3 ─► M05, M06 (Erodoto, Humboldt)      │
    Thread 4 ─► M07, M08 (Darwin, Leonardo)       ├──► MERGE
    Thread 5 ─► M09, M10 (Shakespeare, Mozart)    │
    Thread 6 ─► M11, M12 (Cicerone, Smith)        │
    Thread 7 ─► M13, M14 (Lovelace, Ippocrate)   ─┘

FASE 3 (Toolkit) - 10 THREAD PARALLELI
    T1  ─► Mappe Mentali (TK01-TK06)      ─┐
    T2  ─► Quiz Engine (TK07-TK17)         │
    T3  ─► Flashcards (TK18-TK25)          │
    T4  ─► Audio/TTS (TK26-TK33)           │
    T5  ─► Math Tools (TK34-TK43)          ├──► MERGE
    T6  ─► Video/YouTube (TK44-TK49)       │
    T7  ─► Linguistici (TK50-TK57)         │
    T8  ─► Scientifici (TK58-TK64)         │
    T9  ─► Creativi (TK65-TK71)            │
    T10 ─► Informatica (TK72-TK78)        ─┘
    BONUS ─► Gamification (TK79-TK84)

FASE 4 (Curriculum) - 3 THREAD PARALLELI
    CUR-A ─► Licei (C01-C05)              ─┐
    CUR-B ─► Medie/Elementari (C06-C07)    ├──► MERGE
    CUR-C ─► Tecnici/Custom (C08-C11)     ─┘

FASE 5 (Features) - 4 THREAD PARALLELI
    F1 ─► Homework (F01-F06)              ─┐
    F2 ─► Study Sessions (F07-F12)         ├──► MERGE
    F3 ─► Progress (F13-F17)               │
    F4 ─► Anna (F18-F22)                  ─┘

FASE 6 (A11y) - 5 THREAD PARALLELI
    A11Y-1 ─► Dislessia (DY01-DY07)       ─┐
    A11Y-2 ─► Discalculia (DC01-DC06)      │
    A11Y-3 ─► Paralisi (CP01-CP05)         ├──► MERGE
    A11Y-4 ─► ADHD (AD01-AD06)             │
    A11Y-5 ─► Autismo (AU01-AU06)         ─┘

FASE 7 (Coord)
    [AL01-AL06] ──► [CM01-CM03]

FASE 8 (Test) - 5 THREAD PARALLELI
    TEST-1 ─► Unit (T01-T05)              ─┐
    TEST-2 ─► Integration (T06-T09)        │
    TEST-3 ─► User DY/DC (T10-T11)         ├──► MERGE
    TEST-4 ─► User ADHD/AU (T12-T13)       │
    TEST-5 ─► Mario + Iter (T14-T15)      ─┘
```

---

## PRIORITY SUMMARY

| Priority | Count | Description |
|----------|-------|-------------|
| **P0** | ~60 | Essenziali per MVP |
| **P1** | ~50 | Importanti, fase 2 |
| **P2** | ~25 | Nice-to-have |

**Ordine esecuzione**:
1. Tutte le P0 prima
2. P1 in parallelo dove possibile
3. P2 se tempo disponibile

---

## SUCCESS CRITERIA

- [x] 14 maestri operativi e testati
- [~] Toolkit P0 completo (mappe, quiz, flashcards, audio, calc) - Core done, comandi TODO
- [ ] Setup wizard intuitivo
- [~] 3+ curriculum completi - 1/7 done
- [ ] Tutte le condizioni A11y supportate (P0)
- [ ] Anna integration funzionante
- [ ] Ali preside operativo
- [ ] Test con 5+ studenti reali
- [ ] Feedback >4/5 da utenti con disabilita

---

## FILES SUMMARY

### Nuovi File (stima)
- 14 file maestri: `src/agents/definitions/education/*.md`
- 10+ file curriculum: `curricula/it/*.json`
- ~15 file toolkit: `src/education/tools/*.c`
- ~5 file core: `src/education/*.c`
- **Totale: ~50 nuovi file, ~15.000 LOC**

### File da Modificare
- `ali-chief-of-staff.md`: Ruolo preside
- `anna-executive-assistant.md`: Reminder studio
- `jenny-inclusive-accessibility-champion.md`: Profili A11y
- `registry.c`: 14 nuovi agenti
- `commands.c`: Nuovi comandi /education, /study, etc.

---

## CHANGELOG

| Data | Modifica |
|------|----------|
| 2025-12-19 21:00 | Creazione piano iniziale |
| 2025-12-19 22:00 | Aggiunto FASE 3 Toolkit completo (84 tool) |
| 2025-12-19 22:00 | Parallelizzazione massima (10 thread toolkit) |
| 2025-12-19 22:00 | Prioritizzazione P0/P1/P2 |
| 2025-12-20 00:15 | **SYNC**: Aggiornato stato reale implementazione (47/156 = 30%) |
| 2025-12-20 00:15 | FASE 2 completata: tutti 14 maestri implementati |
| 2025-12-20 00:15 | FASE 1.2 completata: 12 tabelle DB (6 pianificate + 6 bonus) |
| 2025-12-20 00:15 | FASE 3 parziale: 5 core tools (mindmap, quiz, flashcards, audio, calc) |
| 2025-12-20 00:15 | FASE 4 parziale: Liceo Scientifico (23KB) |

---

**Piano creato**: 2025-12-19
**Ultimo aggiornamento**: 2025-12-20 00:15
**Task totali**: 156
**Task completati**: 47
**Percentuale**: 30%
**Thread paralleli max**: 10
**Autore**: Roberto con supporto team agenti AI
