# Execution Plan: Convergio 6.0 - Zed Integration MVP

**Created**: 2025-12-18
**Last Updated**: 2025-12-19 00:30
**Status**: 🚀 FASE 3 - Convergio-Zed Fork
**Progress**: 15/16 tasks (94%)
**Branch**: `feature/acp-zed-integration`
**Goal**: Editor AI-first con multi-agent panel integrato

---

## INSTRUCTIONS

> Aggiornare dopo ogni task completato.

---

## QUICK SUMMARY

**Obiettivo**: Editor AI-first con 54 agenti accessibili da pannello visivo.

**Approccio**: Fork Zed → Custom multi-agent panel → Distribuzione

```
FASE 1 (MVP):     convergio-acp + test locale        → ✅ COMPLETATO
FASE 2 (ACP):     --agent flag + routing             → ✅ COMPLETATO
FASE 3 (Fork):    Convergio-Zed custom editor        → 🚀 IN CORSO (94%)
FASE 4 (Dist):    Build + distribuzione macOS/Linux  → dopo validazione
```

**Repositories:**
- ConvergioCLI: `/Users/roberdan/GitHub/ConvergioCLI`
- Convergio-Zed: `/Users/roberdan/GitHub/convergio-zed` (fork di Zed)
- GitHub: https://github.com/Roberdan/convergio-zed

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

### FASE 3 - Convergio-Zed Fork 🚀

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
| Z9 | Build + Test E2E | 🚀 | 1 gg | Release build in corso |

### FASE 4 - Feature Avanzate 🚀

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| F1 | Super chat Ali (bottom panel) | ✅✅ | 2 gg | Ali bottom panel + Enter key + Open Chat button |
| F2 | Ali consapevole di tutte le conversazioni | ✅✅ | 3 gg | ACP salva context, Ali carica da ~/.convergio/agent_context/ |
| F3 | Persistenza conversazioni per agente | ✅✅ | 2 gg | KEY_VALUE_STORE per sessions + resume_session_id |
| F4 | Branding: icon Convergio Panel | ✅✅ | 0.5 gg | UserGroup icon per Convergio, Ai per Ali |

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
- [ ] Test E2E completo (build in corso)

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

**Piano aggiornato**: 2025-12-19 00:30
