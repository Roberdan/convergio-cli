# Execution Plan: Convergio 6.0 - Zed Integration MVP

**Created**: 2025-12-18
**Last Updated**: 2025-12-18 20:47
**Status**: ✅ FASE 2 IN CORSO - Multi-Agent Panel
**Progress**: 7/10 tasks (70%)
**Branch**: `feature/acp-zed-integration`
**Goal**: Convergio funzionante in Zed il prima possibile

---

## INSTRUCTIONS

> Aggiornare dopo ogni task completato.

---

## QUICK SUMMARY

**Obiettivo**: Vedere Convergio dentro Zed con gli agenti esistenti.

**Approccio**: MVP minimale → test → iterate

```
FASE 1 (MVP):     convergio-acp + test locale        → ✅ COMPLETATO
FASE 2 (Polish):  agent packs + UI miglioramenti     → dopo test
FASE 3 (Publish): extension pubblica + a11y layer   → dopo validazione
```

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
| P4 | Agent packs (raggruppamento tematico) | ⬜ | 1 gg | Business, Dev, Design, etc. |
| P5 | Accessibility layer | ⬜ | 3 gg | |
| P6 | Extension manifest + pubblicazione | ⬜ | 1 gg | |

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

**Piano aggiornato**: 2025-12-18 20:47
