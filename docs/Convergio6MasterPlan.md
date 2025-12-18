# Execution Plan: Convergio 6.0 - Zed Integration MVP

**Created**: 2025-12-18
**Last Updated**: 2025-12-18 21:10
**Status**: 🚀 FASE 3 - Convergio-Zed Fork
**Progress**: 7/16 tasks (44%)
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
FASE 3 (Fork):    Convergio-Zed custom editor        → 🚀 IN CORSO
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
| Z1 | Setup ambiente Rust + build Zed | ⬜ | 0.5 gg | rustup, cargo build |
| Z2 | Creare crate `crates/convergio_panel` | ⬜ | 1 gg | Copiare struttura da collab_panel |
| Z3 | Implementare `Panel` trait per ConvergioPanel | ⬜ | 1 gg | icon(), toggle_action(), render() |
| Z4 | Aggiungere in `initialize_panels()` | ⬜ | 0.5 gg | zed.rs line 645 |
| Z5 | UI lista 54 agenti | ⬜ | 1 gg | Lista scrollabile con icone |
| Z6 | Click agente → apre chat ACP | ⬜ | 1 gg | Spawn convergio-acp --agent |
| Z7 | Branding: logo Convergio | ⬜ | 0.5 gg | Assets, about dialog |
| Z8 | Build macOS app bundle | ⬜ | 1 gg | .app firmata |
| Z9 | Test E2E multi-agent | ⬜ | 1 gg | Workflow completo |

**Crates Zed rilevanti (207 totali):**
- `crates/collab_ui/src/collab_panel.rs` - Channels panel (**modello da seguire**)
- `crates/workspace/src/dock.rs` - `Panel` trait definition
- `crates/zed/src/zed.rs:645` - `initialize_panels()` dove aggiungere ConvergioPanel
- `crates/agent` - Agent core logic
- `crates/agent_ui` / `agent_ui_v2` - Agent panel UI

**Pattern per aggiungere pannello bottom bar:**
```rust
// 1. Implementare Panel trait
impl Panel for ConvergioPanel {
    fn icon(&self, ...) -> Option<IconName> { Some(IconName::Bot) }
    fn icon_tooltip(&self, ...) -> Option<&'static str> { Some("Convergio Agents") }
    fn toggle_action(&self) -> Box<dyn Action> { Box::new(ToggleFocus) }
}

// 2. Aggiungere in zed.rs initialize_panels()
let convergio_panel = convergio_panel::ConvergioPanel::load(...);
add_panel_when_ready(convergio_panel, ...);
```

---

## FASE 1 - IMPLEMENTAZIONE MVP ✅

### Architettura Implementata

```
┌─────────────┐      JSON-RPC       ┌─────────────────────┐
│     ZED     │◄───── stdio ───────►│   convergio-acp     │
└─────────────┘                     │   (~500 LOC)        │
                                    └──────────┬──────────┘
                                               │
                                               │ direct call
                                               │
                                    ┌──────────▼──────────┐
                                    │   orchestrator      │
                                    │   (esistente)       │
                                    │   + streaming cb    │
                                    └─────────────────────┘
```

### File Creati

```
src/acp/
├── acp_server.c      # Main loop, JSON-RPC dispatch, handlers (495 LOC)
└── acp_stubs.c       # Stubs per globals di main.c (45 LOC)

include/nous/
└── acp.h             # Header types e API (40 LOC)
```

### Configurazione Zed

File: `~/.config/zed/settings.json`

```json
"agent_servers": {
  "Convergio": {
    "type": "custom",
    "command": "/Users/roberdan/GitHub/ConvergioCLI/build/bin/convergio-acp",
    "args": [],
    "env": {}
  }
}
```

### Binary

- **Path**: `/Users/roberdan/GitHub/ConvergioCLI/build/bin/convergio-acp`
- **Size**: 33MB
- **Build**: `make convergio-acp`

---

## DEFINITION OF DONE (MVP)

- [x] `convergio-acp` compila senza errori
- [x] Zed configurato con agent server custom
- [x] Zed riconosce Convergio nel pannello Agent
- [x] Si può chattare con Ali
- [x] Streaming funziona (token by token)
- [ ] Tool calls visibili in Zed (da testare)

**MVP COMPLETATO** ✅ - 2025-12-18 20:30

---

## COMMITS

1. `90d67f4` - feat(acp): Add Agent Client Protocol server for Zed integration
2. `8dc2c31` - docs: Update master plan - MVP complete, ready for testing
3. `f98b4c6` - fix(acp): Fix ACP protocol format and use-after-free bugs

---

## NEXT STEPS AFTER MVP

Una volta che il MVP funziona:

1. **Feedback**: cosa manca? cosa non funziona?
2. **Agent Packs**: aggiungere business/education agents
3. **A11y Layer**: implementare come post-processing
4. **Publish**: creare extension.toml per distribuzione

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

---

## FASE 3 - ARCHITETTURA TARGET

```
┌─────────────────────────────────────────────────────────────────┐
│                      CONVERGIO-ZED                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐   │
│  │   Editor     │  │   Channels   │  │   CONVERGIO PANEL    │   │
│  │   (code)     │  │   (collab)   │  │   ┌──────────────┐   │   │
│  │              │  │              │  │   │ 🤖 Ali       │   │   │
│  │              │  │              │  │   │ 💼 amy-cfo   │   │   │
│  │              │  │              │  │   │ 🏗️ baccio    │   │   │
│  │              │  │              │  │   │ 🐛 dario     │   │   │
│  │              │  │              │  │   │ ...54 agents │   │   │
│  │              │  │              │  │   └──────────────┘   │   │
│  └──────────────┘  └──────────────┘  └──────────┬───────────┘   │
└─────────────────────────────────────────────────┼───────────────┘
                                                  │ click
                                                  ▼
                              ┌───────────────────────────────────┐
                              │         convergio-acp             │
                              │         --agent <name>            │
                              └───────────────────────────────────┘
```

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

**Piano aggiornato**: 2025-12-18 21:10
