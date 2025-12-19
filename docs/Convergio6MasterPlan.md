# Execution Plan: Convergio 6.0 - Zed Integration MVP

**Created**: 2025-12-18
**Last Updated**: 2025-12-19 23:14
**Status**: ✅ MVP COMPLETATO - Ready for Release
**Progress**: 44/44 MVP tasks (100%) - 3 post-MVP tasks deferred (P4-P6), 7 FASE 8 tasks planned
**Branch**: `feature/acp-zed-integration` (ConvergioCLI), `main` (convergio-zed)
**Goal**: Editor AI-first con multi-agent panel integrato (MVP)

---

## SCOPE

**MVP (questo piano)**: Convergio Studio funzionante con 54 agenti, Ali panel, persistenza, memoria storica.

**Post-MVP (FASE 8+)**: Git Graph, Agent Packs, Accessibility, Extension publishing.

---

## QUICK SUMMARY

**Obiettivo MVP**: Editor AI-first con 54 agenti accessibili da pannello visivo.

**Approccio**: Fork Zed → Custom multi-agent panel → Distribuzione

```
FASE 1 (MVP):     convergio-acp + test locale        → ✅ COMPLETATO
FASE 2 (ACP):     --agent flag + routing             → ✅ COMPLETATO (P4-P6 deferred)
FASE 3 (Fork):    Convergio-Zed custom editor        → ✅ COMPLETATO
FASE 4 (Features):Ali panel + persistence + context   → ✅ COMPLETATO
FASE 5 (Polish):  Icons, themes, onboarding          → ✅ COMPLETATO
FASE 6 (Files):   File tools + context + icons       → ✅ COMPLETATO
FASE 7 (Memory):  Ali historical memory              → ✅ COMPLETATO
```

**Repositories:**
- ConvergioCLI: `/Users/roberdan/GitHub/ConvergioCLI` (branch: `feature/acp-zed-integration`)
- Convergio-Zed: `/Users/roberdan/GitHub/convergio-zed` (branch: `main`, fork)
- GitHub: https://github.com/Roberdan/convergio-zed

**Build Artifacts:**
- `build/bin/convergio-acp` - ACP server per Zed
- convergio-zed release build - Zed.app con Convergio Panel

---

## STATUS TRACKING

### FASE 1 - MVP ✅ COMPLETATA

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| M1 | ACP protocol handler (initialize, session/new) | ✅✅ | 1 gg | Completato 2025-12-18 |
| M2 | ACP prompt handler (session/prompt + streaming) | ✅✅ | 1 gg | Completato 2025-12-18 |
| M3 | Bridge a orchestrator esistente | ✅✅ | 1 gg | Completato 2025-12-18 |
| M4 | Build + test locale in Zed | ✅✅ | 0.5 gg | Build OK, Zed configurato 2025-12-18 19:05 |

### FASE 2 - Multi-Agent Panel

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| P1 | Multi-agent servers (ogni agente = server separato) | ✅✅ | 1 gg | --agent flag implementato |
| P2 | Arg --agent per selezionare agente specifico | ✅✅ | 0.5 gg | convergio-acp --agent ali |
| P3 | Generazione automatica settings.json | ✅✅ | 0.5 gg | scripts/generate_zed_config.sh |
| P4 | Agent packs (raggruppamento tematico) | ⏸️ | 1 gg | Sospeso - focus su Fase 3 |
| P5 | Accessibility layer | ⏸️ | 3 gg | Sospeso - focus su Fase 3 |
| P6 | Extension manifest + pubblicazione | ⏸️ | 1 gg | Sospeso - focus su Fase 3 |

### FASE 3 - Convergio-Zed Fork ✅

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| Z1 | Setup ambiente Rust + build Zed | ✅✅ | 0.5 gg | Release build OK |
| Z2 | Creare crate `crates/convergio_panel` | ✅✅ | 1 gg | Cargo.toml, convergio_panel.rs, settings.rs, panel.rs |
| Z3 | Implementare `Panel` trait per ConvergioPanel | ✅✅ | 1 gg | icon(), toggle_action(), render() funzionanti |
| Z4 | Aggiungere in `initialize_panels()` | ✅✅ | 0.5 gg | zed.rs + main.rs integrati |
| Z5 | UI lista agenti | ✅✅ | 1 gg | 54 agenti con icone, descrizioni, selezione |
| Z6 | Click agente → apre chat ACP | ✅✅ | 1 gg | NewExternalAgentThread dispatch |
| Z7 | 54 agenti + Categorie + Search | ✅✅ | 1 gg | 14 categorie collassabili + search per nome/skills |
| Z8 | settings.json 54 agenti | ✅✅ | 0.5 gg | ~/.config/zed/settings.json aggiornato |
| Z9 | Build + Test E2E | ✅✅ | 1 gg | Release build completata 2025-12-19 |

### FASE 4 - Feature Avanzate ✅

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| F1 | Super chat Ali (bottom panel) | ✅✅ | 2 gg | Ali bottom panel + Enter key + Open Chat button |
| F2 | Ali consapevole di tutte le conversazioni | ✅✅ | 3 gg | ACP salva context, Ali carica da ~/.convergio/agent_context/ |
| F3 | Persistenza conversazioni per agente | ✅✅ | 2 gg | HistoryStore.save_acp_thread + thread_by_agent_name |
| F4 | Branding: icon Convergio Panel | ✅✅ | 0.5 gg | UserGroup icon per Convergio, Ai per Ali |

### ✅ Chat Persistence - COMPLETATO

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| BUG1a | DB: Aggiungere campo agent_name (Zed) | ✅✅ | 0.5 gg | DbThreadMetadata + DbThread + colonna DB |
| BUG1b | Ricerca: thread_by_agent_name usa agent_name | ✅✅ | 0.5 gg | Cerca per agent_name invece che title |
| BUG1c | Salvataggio: agent_name in save_thread_metadata | ✅✅ | 0.5 gg | Convergio threads salvano agent server name |
| BUG1d | ACP Resume: Persistenza sessioni su disco | ✅✅ | 1 gg | ~/.convergio/sessions/ con JSON |
| BUG1e | ACP Resume: Auto-resume per agent_name | ✅✅ | 0.5 gg | Server cerca automaticamente sessioni precedenti |
| BUG1f | ACP Resume: History context nel prompt | ✅✅ | 0.5 gg | Messaggi precedenti inclusi come contesto |

**Implementazione completa**:
1. ✅ **Lato Zed**: I thread vengono salvati con `agent_name`
2. ✅ **Lato convergio-acp**:
   - Sessioni salvate su disco in `~/.convergio/sessions/`
   - Auto-resume per agent_name quando si apre una nuova sessione
   - History dei messaggi inclusa come contesto nel prompt
   - L'agente "ricorda" le conversazioni precedenti

**File modificati** (2025-12-19):
- **convergio-zed**:
  - `crates/agent/src/db.rs` - agent_name in DbThreadMetadata + DbThread + query
  - `crates/agent/src/history_store.rs` - thread_by_agent_name migliorato
  - `crates/agent_ui/src/agent_panel.rs` - ricerca per full agent name
  - `crates/agent_ui/src/acp/thread_view.rs` - salva agent_name + log per ACP resume
- **ConvergioCLI**:
  - `include/nous/acp.h` - ACPSession con message history
  - `src/acp/acp_server.c` - persistenza sessioni, auto-resume, history context

---

### FASE 5 - Polish & UX ✅ COMPLETATA

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| U1 | LLM-based icon resolution | ✅✅ | 1 gg | Icone semantiche per ogni agente (ZedAgent, Debug, etc.) |
| U2 | Custom icon set per agenti | ✅✅ | 2 gg | Mapping agent_name → IconName dalla libreria Zed |
| U3 | Themes e colori per categoria | ✅✅ | 1 gg | HSLA colors distintivi per ogni categoria |
| U4 | Onboarding wizard | ✅✅ | 2 gg | Welcome screen con quick start guide |

### FASE 6 - File Interaction & E2E Testing ✅ COMPLETATA

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| X1 | File read tool (ACP) | ✅✅ | 1 gg | Orchestrator già ha tool file_read - funziona via ACP |
| X2 | File write tool (ACP) | ✅✅ | 1 gg | Orchestrator già ha tool file_write/edit - funziona via ACP |
| X3 | Editor context awareness | ✅✅ | 1 gg | embeddedContext=true + context handler nel prompt |
| X4 | E2E Test Suite per ACP | ✅✅ | 1 gg | 19/19 test passing - tests/test_acp_e2e.sh |
| X5 | Ali panel Enter key fix | ✅✅ | 0.5 gg | Usa menu::Confirm invece di keybinding custom |
| X6 | Persistence verification | ✅✅ | 0.5 gg | AcpThreadView integrato in Ali panel |
| X7 | Custom Convergio icons | ✅✅ | 1 gg | convergio.svg + convergio_ali.svg + IconName enum |
| X8 | Ali full chat integration | ✅✅ | 2 gg | Chat completa embedded in bottom dock |

**Implementazione** (2025-12-19):
- **Icone migliorate**: ZedAgent per Ali, Debug per Dario, SwatchBook per UX, etc.
- **Colori categoria**: Barra colorata (HSLA) per visual distinction
- **Onboarding**: Welcome screen persistente con "Get Started" button
- **File**: `crates/convergio_panel/src/panel.rs` - +265 lines

**Implementazione** (2025-12-20):
- **Ali Full Chat**: Ali panel ora embeds AcpThreadView completo (non solo bottone)
- **Custom Icons**: Aggiunti `convergio.svg` e `convergio_ali.svg` in `assets/icons/`
- **IconName enum**: Aggiunto `Convergio` e `ConvergioAli` per utilizzo nelle UI
- **File**: `crates/ali_panel/src/panel.rs` - Rewrite completo con AcpThreadView

### FASE 7 - Ali Historical Memory ✅ COMPLETATA

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| H1 | Fix conversation resume for all agents | ✅✅ | 1 gg | Fixed with cx.observe + pending_resume pattern |
| H2 | Ali icon visibility in dock | ✅✅ | 0.5 gg | IconName::ConvergioAli in icons.rs + SVG |
| H3 | Cross-session memory summaries | ✅✅ | 2 gg | memory_generate_summary() con LLM |
| H4 | Memory storage format | ✅✅ | 1 gg | ~/.convergio/memory/summaries/*.json |
| H5 | Memory injection in prompts | ✅✅ | 1 gg | memory_build_context() iniettato in Ali |
| H6 | Memory search/retrieval | ✅✅ | 2 gg | memory_search() + memory_load_by_agent() |

**Implementazione** (2025-12-19):
- **include/nous/memory.h**: Header con API completa per memoria storica
- **src/memory/memory.c**: Implementazione con storage JSON, summarization LLM, search
- **src/acp/acp_server.c**: Integrazione memoria in Ali (H5) + generazione automatica (H3)
- **Storage**: ~/.convergio/memory/summaries/ con MemoryEntry (id, agent, summary, topics, decisions, action_items, timestamp, importance)

**Obiettivo RAGGIUNTO**: Ali diventa la memoria storica dell'utente. Riassunti generati automaticamente ogni 4 messaggi, iniettati nel contesto di Ali all'avvio.

### FASE 8 - Git Graph Panel ⬜ PIANIFICATA

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| G1 | Research Zed git panel architecture | ⬜ | 0.5 gg | Capire come estendere git_ui crate |
| G2 | Create git_graph crate | ⬜ | 1 gg | Nuovo crate per visualizzazione grafo |
| G3 | Implement commit graph data model | ⬜ | 1 gg | DAG con branches, merges, commits |
| G4 | Render graph with GPUI | ⬜ | 2 gg | Linee colorate, nodi, branch labels |
| G5 | Integrate with git panel | ⬜ | 1 gg | Tab o toggle in git panel esistente |
| G6 | Interactivity (click, hover, context menu) | ⬜ | 1 gg | Checkout, revert, cherry-pick |
| G7 | Performance optimization for large repos | ⬜ | 1 gg | Virtual scrolling, lazy loading |

**Obiettivo**: Visualizzazione grafica della storia git simile a GitGraph di VS Code, integrata nel pannello git di Zed.

**Ispirazione**: [VS Code Git Graph](https://marketplace.visualstudio.com/items?itemName=mhutchie.git-graph)

---

## ARCHITETTURA ATTUALE

```
┌────────────────────────────────────────────────────────────────────────┐
│                        CONVERGIO-ZED                                   │
├──────────────────┬─────────────────────────────────────────────────────┤
│                  │                                                      │
│  CONVERGIO       │              ZED AGENT PANEL                         │
│  PANEL           │              (existing UI)                           │
│  ──────────────  │                                                      │
│  🔍 Search...    │  ┌────────────────────────────────────────────────┐ │
│  ──────────────  │  │ Chat with: Baccio - Architect                  │ │
│                  │  │                                                │ │
│  ▼ Leadership (2)│  │ YOU: Help me design the system architecture    │ │
│    ● Ali         │  │                                                │ │
│    ○ Satya       │  │ BACCIO: Based on your requirements, I suggest  │ │
│  ▼ Technology (7)│  │ a microservices approach with...               │ │
│    ● Baccio ◄────┼──│                                                │ │
│    ○ Dario       │  │                                                │ │
│    ○ Rex         │  └────────────────────────────────────────────────┘ │
│    ...           │                                                      │
│  ▶ Finance (4)   │                                                      │
│  ▶ Security (5)  │                                                      │
│  ...             │                                                      │
│  [54 agents]     │                                                      │
│  [14 categories] │                                                      │
│                  │                                                      │
└──────────────────┴─────────────────────────────────────────────────────┘

Features:
- 54 agenti organizzati in 14 categorie collassabili
- Search per nome, descrizione e skills
- Click agente → apre chat con agent server specifico
- Ogni agente ha icona, nome e descrizione
```

---

## FEATURE RICHIESTE (FASE 4)

### F1: Super Chat Ali (Bottom Panel)

```
┌─────────────────────────────────────────────────────────────────────┐
│                      CODE EDITOR                                     │
│  function calculate() {                                             │
│    // ...                                                           │
│  }                                                                  │
├─────────────────────────────────────────────────────────────────────┤
│  ALI - CHIEF OF STAFF (sempre visibile, come il terminale)         │
│  ───────────────────────────────────────────────────────────────── │
│  YOU: What's the status of the project?                            │
│  ALI: Based on conversations with other agents:                     │
│       - Baccio suggests microservices architecture                  │
│       - Dario found 3 bugs in the auth module                       │
│       - Rex reviewed PR #42, approved with minor changes            │
└─────────────────────────────────────────────────────────────────────┘
```

### F2: Context Sharing tra Agenti

Ali deve poter:
- Vedere summary delle conversazioni recenti con altri agenti
- Aggregare insights da multiple conversazioni
- Fornire overview strategica del progetto

**Implementazione proposta:**
- File JSON locale con summary delle conversazioni
- Ali legge questo file al startup
- Ogni agente scrive summary quando la conversazione finisce

### F3: Persistenza Conversazioni

- Click su agente → se esiste conversazione precedente, la riprende
- Opzione "Nuova conversazione" per iniziare da zero
- History delle conversazioni per agente nel pannello

---

## DEFINITION OF DONE (MVP)

- [x] `convergio-acp` compila senza errori
- [x] Zed configurato con agent server custom
- [x] Zed riconosce Convergio nel pannello Agent
- [x] Si può chattare con Ali
- [x] Streaming funziona (token by token)
- [x] 54 agenti disponibili nel pannello
- [x] Categorie collassabili
- [x] Search per nome/skills
- [x] Test E2E completo (19/19 passing)

---

## COMMITS

1. `90d67f4` - feat(acp): Add Agent Client Protocol server for Zed integration
2. `8dc2c31` - docs: Update master plan - MVP complete, ready for testing
3. `f98b4c6` - fix(acp): Fix ACP protocol format and use-after-free bugs

---

## LOG

| Data | Ora | Evento |
|------|-----|--------|
| 2025-12-18 | 19:05 | Build convergio-acp completato |
| 2025-12-18 | 19:05 | Zed settings.json configurato |
| 2025-12-18 | 19:09 | Commits pushati su feature branch |
| 2025-12-18 | 20:00 | Debugging: SIGABRT in Zed, trovati bug use-after-free |
| 2025-12-18 | 20:15 | Fix ACP schema format (sessionUpdate, content.text) |
| 2025-12-18 | 20:25 | Test locale OK: init, session/new, session/prompt funzionanti |
| 2025-12-18 | 20:28 | Streaming funziona: orchestrator risponde token-by-token |
| 2025-12-18 | 20:30 | ✅ **TEST IN ZED RIUSCITO** - Ali risponde, streaming OK |
| 2025-12-18 | 20:45 | P1-P3 completati: --agent flag, routing, generate_zed_config.sh |
| 2025-12-18 | 20:47 | 54 agenti disponibili via --list-agents |
| 2025-12-18 | 21:05 | Decisione: fork Zed per multi-agent panel |
| 2025-12-18 | 21:05 | Fork creato: github.com/Roberdan/convergio-zed |
| 2025-12-18 | 21:10 | Piano Fase 3 definito (9 task) |
| 2025-12-18 | 21:30 | Z2: Crate convergio_panel creato |
| 2025-12-18 | 21:45 | Z3: Panel trait implementato (icon, toggle_action, render) |
| 2025-12-18 | 21:50 | Z4: Integrazione in initialize_panels() e main.rs |
| 2025-12-18 | 22:00 | Z5: ✅ **CONVERGIO PANEL FUNZIONANTE** - 12 agenti visibili |
| 2025-12-18 | 23:30 | Z6: Click handler con NewExternalAgentThread |
| 2025-12-18 | 23:45 | Z7: 54 agenti + 14 categorie + search field |
| 2025-12-19 | 00:00 | Z8: settings.json aggiornato con tutti 54 agenti |
| 2025-12-19 | 00:30 | Z9: Release build avviata |
| 2025-12-19 | 01:30 | F1: Ali bottom panel implementato (Enter key, Open Chat button) |
| 2025-12-19 | 01:30 | F4: Icons aggiornate (UserGroup per Convergio, Ai per Ali) |
| 2025-12-19 | 01:30 | Fix: Rimossi mock Baccio/Dario da Ali panel |
| 2025-12-19 | 01:30 | F3: Infrastruttura persistenza aggiunta (resume_session_id) |
| 2025-12-19 | 02:00 | F2: Context sharing implementato (ACP salva/carica agent_context) |
| 2025-12-19 | 02:00 | F3: Conversation persistence completato (KEY_VALUE_STORE) |
| 2025-12-19 | 02:00 | Commit convergio-zed: d19c1100e4 (Convergio Panel + Ali) |
| 2025-12-19 | 02:00 | Commit ConvergioCLI: d6bb014 (Context sharing ACP) |
| 2025-12-19 | 15:00 | F3: Fix persistenza - HistoryStore.save_acp_thread + thread_by_agent_name |
| 2025-12-19 | 15:10 | Release build completata: Zed-aarch64.dmg |
| 2025-12-19 | 15:11 | ✅ **FASE 4 COMPLETATA** - Convergio Studio MVP ready |
| 2025-12-19 | 15:20 | 🚨 BUG CRITICO: Click agente apre nuova chat invece di riprendere esistente |
| 2025-12-19 | 18:00 | H1: Fix conversation resume - cx.observe + pending_resume_thread |
| 2025-12-19 | 18:30 | All 35 phase tests passing |
| 2025-12-19 | 18:45 | Commit convergio-zed: 1db07e3989 (resume fix) |
| 2025-12-19 | 19:00 | Commit ConvergioCLI: 1274f42 (test suite) |
| 2025-12-19 | 21:00 | H3-H6: Ali Historical Memory implementato |
| 2025-12-19 | 22:00 | include/nous/memory.h - API memoria storica |
| 2025-12-19 | 22:30 | src/memory/memory.c - Storage, summarization, search |
| 2025-12-19 | 23:00 | H5: Memory injection in Ali prompts |
| 2025-12-19 | 23:14 | ✅ **FASE 7 COMPLETATA** - MVP 100% completo |
| 2025-12-20 | 00:30 | X8: Ali panel rewrite - AcpThreadView embedded (full chat) |
| 2025-12-20 | 01:00 | X7: Custom icons aggiunti - convergio.svg, convergio_ali.svg |
| 2025-12-20 | 01:15 | ✅ Release build completata con Ali full chat |
| 2025-12-20 | 01:30 | Commits pushati: 86ef57f (icons), a724634 (AliPanel fix) |

---

## USAGE

```bash
# List all available agents
./build/bin/convergio-acp --list-agents

# Generate Zed config for all agents
./scripts/generate_zed_config.sh > zed_agents.json

# Start ACP server for specific agent
./build/bin/convergio-acp --agent amy-cfo
```

---

## PROSSIMI PASSI (Post-MVP)

### Priorità 0: Release Alignment System
Tenere sempre allineate le versioni di:
- **convergio** (CLI principale)
- **convergio-acp** (server per Zed)
- **convergio-zed** (fork Zed con Convergio Panel)

**Implementazione proposta**:
1. Version file condiviso (`VERSION`) in entrambi i repos
2. Script di release che:
   - Incrementa versione in entrambi i repos
   - Crea tag git sincronizzati
   - Build automatica di tutti gli artefatti
   - Genera CHANGELOG unificato
3. CI/CD check che verifica compatibilità ACP tra versioni
4. Release notes che documentano dipendenze tra versioni

### Priorità 1: FASE 8 - Git Graph Panel
- Visualizzazione grafica della storia git in Zed
- Ispirato a VS Code Git Graph extension

### Priorità 2: Fasi sospese (P4-P6)
- **P4**: Agent packs (raggruppamento tematico agenti)
- **P5**: Accessibility layer (screen reader, keyboard navigation)
- **P6**: Extension manifest + pubblicazione su marketplace

### Priorità 3: Ottimizzazioni
- Performance profiling per repos grandi
- Memory optimization per sessioni lunghe
- Caching intelligente delle risposte

---

**Piano aggiornato: 2025-12-19 23:30
