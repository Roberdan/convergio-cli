# Convergio 6.0 - Piano Strategico

**Versione**: 6.0 "Universalis"
**Data**: 2025-12-18
**Autore**: Roberto con supporto team agenti AI

---

## Executive Summary

Convergio 6.0 trasforma la piattaforma da tool tecnico a **ecosistema di consulenza AI universale**, accessibile a:

- **Business professionals** (CEO, HR, Marketing, Sales, Logistica, PM)
- **Studenti** (con agenti-professori per ogni materia)
- **Persone con disabilità cognitive** (dislessia, discalculia, ADHD, autismo)

La visione: **"Il tuo team di esperti personale, che parla la tua lingua"**

---

## Parte 1: Architettura Verticali

### 1.1 Sistema di Profili Utente

```
┌─────────────────────────────────────────────────────────────┐
│                    PROFILO UTENTE                           │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │   Ruolo     │  │  Settore    │  │  Accessibilità      │ │
│  │  Business   │  │  Retail     │  │  Dislessia: ON      │ │
│  │  Student    │  │  Healthcare │  │  Discalculia: ON    │ │
│  │  Developer  │  │  Education  │  │  ADHD mode: ON      │ │
│  │  Personal   │  │  Finance    │  │  Alto contrasto     │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
│                           ↓                                 │
│              AGENT POOL PERSONALIZZATO                      │
│   (Solo agenti rilevanti per il profilo selezionato)       │
└─────────────────────────────────────────────────────────────┘
```

**Implementazione:**

```c
// include/nous/profile.h
typedef struct {
    char name[64];
    ProfileRole role;           // BUSINESS, STUDENT, DEVELOPER, PERSONAL
    char industry[32];          // Settore specifico
    AccessibilityFlags a11y;    // Flags accessibilità
    char language[8];           // it, en, es, de, fr...
    char agent_preset[32];      // "marketing", "education_high_school", etc.
} UserProfile;

typedef enum {
    A11Y_DYSLEXIA      = 1 << 0,  // Font OpenDyslexic, spaziatura aumentata
    A11Y_DYSCALCULIA   = 1 << 1,  // Numeri visuali, step-by-step math
    A11Y_ADHD          = 1 << 2,  // Risposte brevi, bullet points, timer
    A11Y_LOW_VISION    = 1 << 3,  // Alto contrasto, font grandi
    A11Y_AUTISM        = 1 << 4,  // Comunicazione diretta, no ambiguità
    A11Y_COGNITIVE     = 1 << 5,  // Linguaggio semplificato
} AccessibilityFlags;
```

**File:** `src/profile/profile.c` (~800 LOC)

---

### 1.2 Vertical: Business Professionals

#### Agent Preset: "Enterprise"

| Agente | Ruolo | Target User |
|--------|-------|-------------|
| **Carlo CEO** | Strategic advisor, board prep | CEO, Founders |
| **Giulia HR** | Recruiting, performance, culture | HR Directors |
| **Marco Marketing** | Campaigns, brand, analytics | CMO, Marketing Mgr |
| **Luca Sales** | Pipeline, forecasting, coaching | Sales Directors |
| **Anna Operations** | Supply chain, logistics, efficiency | COO, Ops Mgr |
| **Paolo Finance** | Budgeting, forecasting, compliance | CFO, Controllers |
| **Sara PM** | Project planning, risk, stakeholders | Project Managers |
| **Elena Legal** | Contracts, compliance, risk | Legal Counsel |

#### Funzionalità Business-Specifiche

**1. Document Intelligence**
```
/upload report.pdf
→ Estrazione automatica KPI, trend, anomalie
→ Generazione executive summary
→ Suggerimenti actionable
```

**2. Meeting Assistant**
```
/meeting prep "Board Q4 Review"
→ Agenda suggerita basata su storico
→ Talking points per ogni stakeholder
→ Risk areas da evidenziare
```

**3. Dashboard Narrativo**
```
/dashboard weekly
→ Trasforma numeri in storia comprensibile
→ "Le vendite sono cresciute del 12%, trainata dal nuovo prodotto X..."
→ Evidenzia cosa richiede attenzione
```

**4. Email/Communication Drafting**
```
/draft email stakeholder "bad news about project delay"
→ Tone-appropriate communication
→ Multiple versioni (formal, diplomatic, direct)
→ Suggerimenti su timing
```

**File:** `src/agents/definitions/business/` (15 nuovi agent files)

---

### 1.3 Vertical: Education (Studenti)

#### Agent Preset: "Classroom"

| Agente-Professore | Materia | Stile |
|-------------------|---------|-------|
| **Prof. Einstein** | Fisica | Analogie intuitive, esperimenti mentali |
| **Prof.ssa Curie** | Chimica | Hands-on, sicurezza, applicazioni reali |
| **Prof. Euclide** | Matematica | Step-by-step, visualizzazioni |
| **Prof.ssa Woolf** | Letteratura | Analisi critica, contesto storico |
| **Prof. Herodotus** | Storia | Storytelling, cause-effetto |
| **Prof. Chomsky** | Lingue | Pratica conversazionale, grammatica |
| **Prof.ssa Lovelace** | Informatica | Problem solving, coding graduale |
| **Prof. Darwin** | Biologia | Osservazione, evoluzione, ecosistemi |
| **Prof. Socrate** | Filosofia | Metodo socratico, domande |
| **Prof.ssa Montessori** | Metodo Studio | Tecniche apprendimento, organizzazione |

#### Funzionalità Education-Specifiche

**1. Adaptive Learning**
```
/learn "equazioni di secondo grado"
→ Assessment livello iniziale (3-5 domande)
→ Spiegazione calibrata sul livello
→ Esercizi progressivi con feedback immediato
→ Ripasso spaced repetition
```

**2. Homework Helper (Non Cheating Mode)**
```
/homework math "problema 5 pag 123"
→ NON dà la risposta diretta
→ Guida step-by-step con domande socratiche
→ Hints progressivi se bloccato
→ Spiega il "perché" non solo il "come"
```

**3. Exam Preparation**
```
/exam prep "storia romana" --date "2025-01-15"
→ Piano di studio personalizzato
→ Quiz giornalieri
→ Simulazioni d'esame
→ Tracking progressi
```

**4. Study Buddy**
```
/study session 45min
→ Pomodoro timer integrato
→ Notifiche pause
→ Mini-quiz a fine sessione
→ Celebrazione progressi
```

**File:** `src/agents/definitions/education/` (12 nuovi agent files)

---

### 1.4 Vertical: Accessibilità Cognitiva

#### Modalità Dislessia

```
┌─────────────────────────────────────────────────────────────┐
│  CONVERGIO - Modalità Dislessia Attiva                      │
│                                                             │
│  Font: OpenDyslexic 16pt                                   │
│  Spaziatura: 1.5x                                          │
│  Larghezza riga: max 60 caratteri                          │
│  Colore sfondo: #FDF6E3 (crema, riduce affaticamento)      │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  Vuoi che ti legga la risposta ad alta voce?        │   │
│  │  [Sì, leggi] [No, grazie] [Sempre]                  │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

**Funzionalità:**
- **Text-to-Speech nativo** (AVSpeechSynthesizer su macOS)
- **Highlighting sincronizzato** mentre legge
- **Font OpenDyslexic** con weighted bottoms
- **Sillabazione visuale** per parole lunghe
- **Glossario contestuale** per termini complessi
- **Riassunti audio** automatici

**Implementazione:**
```c
// src/a11y/dyslexia.c
void dyslexia_format_output(const char* text, A11yOutput* out) {
    // Spezza righe lunghe
    wrap_text_at_chars(text, 60, out->wrapped);

    // Evidenzia parole chiave
    highlight_keywords(out->wrapped, out->highlighted);

    // Prepara per TTS
    prepare_ssml_for_speech(out->highlighted, out->ssml);

    // Genera versione semplificata se richiesta
    if (user_profile.a11y & A11Y_COGNITIVE) {
        simplify_language(out->highlighted, out->simplified);
    }
}
```

#### Modalità Discalculia

```
┌─────────────────────────────────────────────────────────────┐
│  CONVERGIO - Modalità Discalculia Attiva                    │
│                                                             │
│  Problema: 847 + 256 = ?                                   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  Visualizzazione Blocchi:                           │   │
│  │                                                     │   │
│  │  847 = [████████] [████] [███████]                  │   │
│  │         800       40      7                         │   │
│  │                                                     │   │
│  │  256 = [██] [█████] [██████]                        │   │
│  │         200   50      6                             │   │
│  │                                                     │   │
│  │  Step 1: 7 + 6 = 13 (scrivi 3, riporti 1)          │   │
│  │  Step 2: 40 + 50 + 10 = 100 (scrivi 0, riporti 1)  │   │
│  │  Step 3: 800 + 200 + 100 = 1100                    │   │
│  │                                                     │   │
│  │  Risultato: 1103                                    │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

**Funzionalità:**
- **Visualizzazione numerica** con blocchi/colori
- **Step-by-step** sempre visibile
- **Calculator contestuale** integrata
- **Grafici invece di tabelle** dove possibile
- **Analogie concrete** per concetti astratti
- **Verifica risultati** con metodi alternativi

#### Modalità ADHD

```
┌─────────────────────────────────────────────────────────────┐
│  CONVERGIO - Modalità Focus Attiva                          │
│                                                             │
│  ⏱️ Sessione: 12:34 / 25:00  [████████░░░░] 50%            │
│                                                             │
│  📋 Task corrente: Scrivere email a cliente                │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  ✓ Punto 1: Saluto iniziale                         │   │
│  │  → Punto 2: Riassunto situazione (SEI QUI)          │   │
│  │  ○ Punto 3: Proposta soluzione                      │   │
│  │  ○ Punto 4: Call to action                          │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  💡 Prossima pausa tra 12 minuti                           │
│  🎯 Completato oggi: 3/5 task                              │
└─────────────────────────────────────────────────────────────┘
```

**Funzionalità:**
- **Pomodoro integrato** con notifiche native
- **Risposte brevi** (max 3-4 bullet points)
- **Progress tracking visuale**
- **Celebrazioni micro-achievements**
- **"Parcheggio" per distrazioni** ("Lo noto dopo")
- **Reminder gentili** per ritornare on-track
- **Modalità "una cosa alla volta"**

#### Modalità Autismo

**Funzionalità:**
- **Comunicazione diretta** (no metafore ambigue)
- **Struttura prevedibile** (stesso formato risposte)
- **Avvisi per cambiamenti** ("Sto per cambiare argomento...")
- **Opzione "più dettagli"** sempre disponibile
- **Nessuna pressione sociale** implicita
- **Preferenze sensoriali** (no animazioni, suoni opzionali)

**File:** `src/a11y/` (nuovo modulo ~2000 LOC)

---

## Parte 2: Funzionalità Cross-Vertical

### 2.1 Knowledge Map (evoluzione da Repository Map)

Invece di mappare codice, mappa **documenti, relazioni, concetti**.

```
┌─────────────────────────────────────────────────────────────┐
│                    KNOWLEDGE MAP                            │
│                                                             │
│  ┌──────────┐    references    ┌──────────────┐            │
│  │ Q4 Report│◄─────────────────│ Board Deck   │            │
│  └────┬─────┘                  └──────┬───────┘            │
│       │ contains                      │ mentions           │
│       ▼                               ▼                    │
│  ┌──────────┐    impacts       ┌──────────────┐            │
│  │ KPI: CAC │─────────────────►│ Budget 2025  │            │
│  └──────────┘                  └──────────────┘            │
│                                                             │
│  🔍 "Quali documenti parlano di CAC?"                      │
│  → Q4 Report (pag 12), Board Deck (slide 5), Budget 2025   │
└─────────────────────────────────────────────────────────────┘
```

**Implementazione:**
```c
// src/knowledge/knowledge_map.c
typedef struct {
    char doc_id[64];
    char doc_type[32];      // "pdf", "xlsx", "email", "note"
    char* entities;         // JSON array of extracted entities
    float* embedding;       // 768-dim document embedding
    time_t last_accessed;
    float relevance_score;  // PageRank-style
} KnowledgeNode;

// Estrae entità da documenti (NER)
void extract_entities(const char* content, EntityList* out);

// Costruisce grafo relazioni
void build_knowledge_graph(KnowledgeNode* nodes, int count, KnowledgeGraph* graph);

// Query semantica
void query_knowledge(const char* query, KnowledgeGraph* graph, QueryResult* results);
```

**File:** `src/knowledge/knowledge_map.c` (~1500 LOC)

---

### 2.2 Watch Mode (Adattato per Business)

Non monitora codice, ma **email, CRM, file condivisi, calendari**.

```
/watch inbox --trigger "urgent|asap|ceo"
→ Notifica quando arriva email urgente
→ Suggerisce risposta prioritaria
→ Prepara context per risposta

/watch calendar --trigger "conflict|overlap"
→ Rileva conflitti scheduling
→ Suggerisce risoluzione
→ Draft email reschedule

/watch folder "/Shared/Reports"
→ Notifica nuovi file
→ Estrae summary automatico
→ Aggiorna knowledge map
```

**Implementazione:**
```c
// src/watch/watch.c
typedef struct {
    WatchType type;         // INBOX, CALENDAR, FOLDER, CRM
    char path[256];         // Path o endpoint
    char trigger_regex[128];
    WatchAction action;     // NOTIFY, SUMMARIZE, DRAFT_RESPONSE
    int check_interval_sec;
} WatchConfig;

// Usa FSEvents per folder, MailKit per email, EventKit per calendar
void start_watch(WatchConfig* config);
void stop_watch(const char* watch_id);
```

**File:** `src/watch/watch.c` (~1000 LOC)

---

### 2.3 Voice I/O (Critico per Accessibilità)

```
┌─────────────────────────────────────────────────────────────┐
│  CONVERGIO - Voice Mode                                     │
│                                                             │
│  🎤 "Aiutami a scrivere una email al cliente Rossi"        │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  ░░░░░░████████████░░░░░░  Listening...             │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  🔊 Risposta vocale: "Certo! Di cosa vuoi parlare          │
│     nell'email? Una proposta, un follow-up, o altro?"      │
│                                                             │
│  [Digita] [Parla] [Pausa]                                  │
└─────────────────────────────────────────────────────────────┘
```

**Implementazione:**
```c
// src/voice/voice.c
typedef struct {
    bool input_enabled;
    bool output_enabled;
    char language[8];
    float speech_rate;      // 0.5 - 2.0
    char voice_id[64];      // Voce specifica
} VoiceConfig;

// Input: Apple Speech Framework (offline) o Whisper API
void start_voice_input(VoiceConfig* config, VoiceCallback callback);

// Output: AVSpeechSynthesizer
void speak_text(const char* text, VoiceConfig* config);

// SSML per enfasi, pause, pronuncia
void speak_ssml(const char* ssml, VoiceConfig* config);
```

**File:** `src/voice/voice.m` (~800 LOC, Objective-C per API Apple)

---

### 2.4 Document Intelligence

Trasforma documenti complessi in insight comprensibili.

```
/analyze quarterly_report.pdf

┌─────────────────────────────────────────────────────────────┐
│  📄 Analisi: Q3 2025 Financial Report                       │
│                                                             │
│  📊 KPI Principali:                                         │
│  • Revenue: €2.3M (+12% vs Q2)                             │
│  • Gross Margin: 68% (stabile)                             │
│  • CAC: €145 (-8%, ottimo trend)                           │
│                                                             │
│  ⚠️ Attenzione:                                            │
│  • Churn rate in aumento (da 3.2% a 4.1%)                  │
│  • Cash runway ridotto a 14 mesi                           │
│                                                             │
│  💡 Suggerimenti:                                           │
│  • Investigare cause churn (exit interviews?)              │
│  • Considerare bridge round Q1 2026                        │
│                                                             │
│  [Approfondisci Revenue] [Analizza Churn] [Export Summary] │
└─────────────────────────────────────────────────────────────┘
```

**Implementazione:**
- Usa librerie native per parsing:
  - PDF: PDFKit (macOS nativo)
  - Excel: libxlsxwriter + libxlsxio
  - Word: libdocx o Pandoc
- Estrazione tabelle → JSON strutturato
- NLP per entity extraction (nomi, date, numeri)
- Trend detection su serie temporali

**File:** `src/documents/` (~2000 LOC)

---

### 2.5 Adaptive Communication

L'AI adatta il suo stile in base al profilo utente.

```c
// src/communication/adaptive.c
typedef struct {
    ToneLevel formality;       // CASUAL, PROFESSIONAL, FORMAL
    ComplexityLevel complexity; // SIMPLE, MODERATE, ADVANCED
    ResponseLength length;      // BRIEF, MODERATE, DETAILED
    bool use_analogies;
    bool use_examples;
    bool use_visuals;
    char cultural_context[32]; // "italian", "american", etc.
} CommunicationStyle;

// Determina stile ottimale basato su profilo
CommunicationStyle determine_style(UserProfile* profile, MessageContext* ctx);

// Adatta risposta allo stile
char* adapt_response(const char* raw_response, CommunicationStyle* style);
```

**Esempi di adattamento:**

| Profilo | Input AI Raw | Output Adattato |
|---------|--------------|-----------------|
| CEO | "The customer acquisition cost decreased by 8.3% quarter-over-quarter due to improved marketing channel efficiency" | "CAC giù dell'8%. Marketing funziona meglio." |
| Studente (dislessia) | "L'equazione di secondo grado ax²+bx+c=0 si risolve con la formula risolutiva" | "Per risolvere l'equazione: • Prendi i numeri a, b, c • Mettili nella formula • Calcola passo per passo" |
| HR Manager | "Employee engagement metrics show concerning trends" | "Il morale del team sembra in calo. Vediamo cosa possiamo fare?" |

**File:** `src/communication/adaptive.c` (~600 LOC)

---

## Parte 3: Architettura Tecnica

### 3.1 Struttura Directory Aggiornata

```
src/
├── core/                    # Esistente
├── orchestrator/            # Esistente (Ali)
├── agents/
│   └── definitions/
│       ├── technical/       # Agenti esistenti (Baccio, Marco, etc.)
│       ├── business/        # NUOVO: Carlo CEO, Giulia HR, etc.
│       ├── education/       # NUOVO: Prof. Einstein, Curie, etc.
│       └── personal/        # NUOVO: Life coach, wellness, etc.
├── profile/                 # NUOVO: Gestione profili utente
│   ├── profile.c
│   ├── presets.c
│   └── migration.c
├── a11y/                    # NUOVO: Accessibilità
│   ├── dyslexia.c
│   ├── dyscalculia.c
│   ├── adhd.c
│   ├── autism.c
│   └── a11y_common.c
├── voice/                   # NUOVO: Voice I/O
│   ├── voice.m              # Apple Speech Framework
│   ├── whisper.c            # Whisper API fallback
│   └── tts.m                # Text-to-Speech
├── documents/               # NUOVO: Document Intelligence
│   ├── pdf.c
│   ├── excel.c
│   ├── word.c
│   └── extractor.c
├── knowledge/               # NUOVO: Knowledge Map
│   ├── knowledge_map.c
│   ├── entity_extraction.c
│   └── graph.c
├── watch/                   # NUOVO: Watch Mode
│   ├── watch.c
│   ├── fsevents.m
│   └── integrations/
│       ├── mail.m
│       └── calendar.m
├── communication/           # NUOVO: Adaptive Communication
│   ├── adaptive.c
│   └── simplifier.c
└── vertical/                # NUOVO: Vertical-specific logic
    ├── business/
    ├── education/
    └── personal/
```

### 3.2 Database Schema Updates

```sql
-- profiles.sql
CREATE TABLE user_profiles (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    role TEXT CHECK(role IN ('business', 'student', 'developer', 'personal')),
    industry TEXT,
    agent_preset TEXT,
    language TEXT DEFAULT 'en',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_active TIMESTAMP
);

CREATE TABLE accessibility_settings (
    profile_id INTEGER PRIMARY KEY REFERENCES user_profiles(id),
    dyslexia_mode BOOLEAN DEFAULT FALSE,
    dyslexia_font TEXT DEFAULT 'OpenDyslexic',
    dyslexia_font_size INTEGER DEFAULT 16,
    dyscalculia_mode BOOLEAN DEFAULT FALSE,
    dyscalculia_visual_math BOOLEAN DEFAULT TRUE,
    adhd_mode BOOLEAN DEFAULT FALSE,
    adhd_pomodoro_minutes INTEGER DEFAULT 25,
    autism_mode BOOLEAN DEFAULT FALSE,
    tts_enabled BOOLEAN DEFAULT FALSE,
    tts_voice TEXT,
    tts_speed REAL DEFAULT 1.0,
    high_contrast BOOLEAN DEFAULT FALSE,
    reduce_motion BOOLEAN DEFAULT FALSE
);

CREATE TABLE knowledge_nodes (
    id TEXT PRIMARY KEY,
    doc_type TEXT,
    title TEXT,
    content_hash TEXT,
    embedding BLOB,  -- 768-dim float16
    entities_json TEXT,
    created_at TIMESTAMP,
    last_accessed TIMESTAMP,
    access_count INTEGER DEFAULT 0,
    relevance_score REAL DEFAULT 0.0
);

CREATE TABLE knowledge_edges (
    from_id TEXT REFERENCES knowledge_nodes(id),
    to_id TEXT REFERENCES knowledge_nodes(id),
    relation_type TEXT,
    weight REAL DEFAULT 1.0,
    PRIMARY KEY (from_id, to_id, relation_type)
);
```

### 3.3 Nuove Dipendenze

| Dipendenza | Uso | Tipo |
|------------|-----|------|
| OpenDyslexic font | Accessibilità dislessia | Asset (OFL license) |
| libxlsxwriter | Parsing Excel | Static link |
| PDFKit | Parsing PDF | macOS Framework |
| Speech.framework | Voice input | macOS Framework |
| AVFoundation | TTS output | macOS Framework |
| EventKit | Calendar integration | macOS Framework |
| tree-sitter | Entity extraction | Static link |

---

## Parte 4: Roadmap Implementazione

### Fase 1: Foundation (v6.0-alpha) - 6 settimane

| Week | Deliverable | Owner |
|------|-------------|-------|
| 1-2 | Sistema Profili + DB schema | Core |
| 2-3 | Modulo Accessibilità base (dyslexia, TTS) | A11y |
| 3-4 | Voice Input (Apple Speech) | Voice |
| 4-5 | Agent presets (business base) | Agents |
| 5-6 | Integration testing + bug fixing | QA |

**Exit Criteria:**
- [ ] Profilo utente funzionante con switch preset
- [ ] Modalità dislessia attiva con font + TTS
- [ ] Voice input base funzionante
- [ ] 5 agenti business operativi

### Fase 2: Business Vertical (v6.0-beta) - 4 settimane

| Week | Deliverable | Owner |
|------|-------------|-------|
| 1 | Document Intelligence (PDF, Excel) | Documents |
| 2 | Knowledge Map base | Knowledge |
| 3 | Watch Mode (folder + calendar) | Watch |
| 4 | Full business agent suite | Agents |

**Exit Criteria:**
- [ ] Upload e analisi PDF/Excel funzionante
- [ ] Knowledge graph navigabile
- [ ] Watch mode su folder attivo
- [ ] 8 agenti business completi

### Fase 3: Education Vertical (v6.0-rc1) - 4 settimane

| Week | Deliverable | Owner |
|------|-------------|-------|
| 1 | Agent-professori (10 materie) | Agents |
| 2 | Adaptive Learning engine | Education |
| 3 | Homework Helper (Socratic mode) | Education |
| 4 | Study session + Pomodoro | Productivity |

**Exit Criteria:**
- [ ] 10 professori virtuali operativi
- [ ] Quiz adattivi funzionanti
- [ ] Modalità compiti senza cheating
- [ ] Timer Pomodoro con notifiche

### Fase 4: Full Accessibility (v6.0-rc2) - 3 settimane

| Week | Deliverable | Owner |
|------|-------------|-------|
| 1 | Discalculia mode (visual math) | A11y |
| 2 | ADHD mode (focus features) | A11y |
| 3 | Autism mode + testing completo | A11y |

**Exit Criteria:**
- [ ] Tutte le modalità A11y testate con utenti reali
- [ ] WCAG 2.1 AA compliance verificata
- [ ] Feedback loop con associazioni disabilità

### Fase 5: Polish & Launch (v6.0) - 3 settimane

| Week | Deliverable | Owner |
|------|-------------|-------|
| 1 | Performance optimization | Core |
| 2 | Documentation + tutorials | Docs |
| 3 | Beta testing + fixes | QA |

---

## Parte 5: Metriche di Successo

### Adoption Metrics

| Metrica | Target v6.0 | Target v6.6 |
|---------|-------------|-------------|
| Utenti business attivi | 500 | 5,000 |
| Utenti education attivi | 1,000 | 10,000 |
| Utenti a11y attivi | 200 | 2,000 |
| Retention 30-day | 40% | 60% |
| NPS | 30 | 50 |

### Accessibility Metrics

| Metrica | Target |
|---------|--------|
| WCAG 2.1 AA compliance | 100% |
| Screen reader compatibility | 100% |
| Keyboard-only navigation | 100% |
| Feedback utenti disabili | 4.0/5.0 |

### Performance Metrics

| Metrica | Target |
|---------|--------|
| Time to first response | < 2s |
| Voice input latency | < 500ms |
| Document parsing (10 page PDF) | < 3s |
| Memory usage (idle) | < 100MB |

---

## Parte 6: Rischi e Mitigazioni

| Rischio | Probabilità | Impatto | Mitigazione |
|---------|-------------|---------|-------------|
| Complessità A11y sottovalutata | Alta | Alto | Coinvolgere esperti A11y da subito |
| Vertical troppo ampi | Media | Alto | Focus su 1-2 use case per vertical |
| Voice accuracy insufficiente | Media | Medio | Fallback a Apple Dictation + Whisper |
| Resistenza utenti non-tech | Media | Alto | Onboarding wizard + tutorial video |
| Privacy concerns (voice, docs) | Alta | Alto | Tutto locale, zero cloud, messaging chiaro |

---

## Parte 7: Differenziatori Competitivi

| Feature | Convergio 6 | ChatGPT | Claude | Copilot |
|---------|-------------|---------|--------|---------|
| Multi-agent orchestration | ✅ 54+ | ❌ | ❌ | ❌ |
| Business vertical | ✅ Native | ❌ | ❌ | Partial |
| Education vertical | ✅ Native | ❌ | ❌ | ❌ |
| Accessibility modes | ✅ 5 tipi | ❌ | ❌ | ❌ |
| 100% offline | ✅ MLX | ❌ | ❌ | ❌ |
| Voice I/O nativo | ✅ | Partial | ❌ | ❌ |
| Document Intelligence | ✅ | ❌ | ❌ | Partial |
| Knowledge Map | ✅ | ❌ | ❌ | ❌ |
| Apple Silicon optimized | ✅ | ❌ | ❌ | ❌ |
| Cost transparency | ✅ | ❌ | ❌ | ❌ |
| Privacy-first | ✅ | ❌ | ❌ | ❌ |

---

## Conclusione

Convergio 6.0 "Universalis" trasforma la piattaforma da tool tecnico a **ecosistema di consulenza AI universale**.

La visione:
- **Business professionals** ottengono un team di consulenti esperti sempre disponibile
- **Studenti** hanno professori pazienti e adattivi per ogni materia
- **Persone con disabilità cognitive** finalmente hanno un'AI che parla la loro lingua

Il tutto mantenendo i principi fondanti:
- **100% privacy** (tutto locale con MLX)
- **Cost transparency** (budget tracking granulare)
- **Apple-native performance** (Metal, Neural Engine, GCD)

Convergio non è un chatbot. È il tuo team personale.

---

*Piano creato il 2025-12-18*
*Versione documento: 1.0*
