# Execution Plan: Convergio 6.0 - Zed Integration MVP

**Created**: 2025-12-18
**Status**: MVP Complete - Ready for Testing
**Progress**: 4/8 tasks (50%)
**Goal**: Convergio funzionante in Zed il prima possibile

---

## INSTRUCTIONS

> Aggiornare dopo ogni task completato.

---

## QUICK SUMMARY

**Obiettivo**: Vedere Convergio dentro Zed con gli agenti esistenti.

**Approccio**: MVP minimale → test → iterate

```
FASE 1 (MVP):     convergio-acp + test locale        → 3-4 giorni
FASE 2 (Polish):  agent packs + UI miglioramenti     → dopo test
FASE 3 (Publish): extension pubblica + a11y layer   → dopo validazione
```

---

## STATUS TRACKING

### FASE 1 - MVP (Parallelizzabile)

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| M1 | ACP protocol handler (initialize, session/new) | ✅✅ | 1 gg | Completato 2025-12-18 |
| M2 | ACP prompt handler (session/prompt + streaming) | ✅✅ | 1 gg | Completato 2025-12-18 |
| M3 | Bridge a orchestrator esistente | ✅✅ | 1 gg | Completato 2025-12-18 |
| M4 | Build + test locale in Zed | ✅✅ | 0.5 gg | Build OK, Zed configurato, pronto per test |

### FASE 2 - Polish (Post-MVP)

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| P1 | Agent packs (business, education) | ⬜ | 2 gg | |
| P2 | Pack selection in config | ⬜ | 1 gg | |
| P3 | Accessibility layer | ⬜ | 3 gg | |
| P4 | Extension manifest + pubblicazione | ⬜ | 1 gg | |

---

## FASE 1 - IMPLEMENTAZIONE MVP

### Architettura Minimale

```
┌─────────────┐      JSON-RPC       ┌─────────────────────┐
│     ZED     │◄───── stdio ───────►│   convergio-acp     │
└─────────────┘                     │   (nuovo, ~500 LOC) │
                                    └──────────┬──────────┘
                                               │
                                               │ spawn + pipe
                                               │
                                    ┌──────────▼──────────┐
                                    │   convergio CLI     │
                                    │   (esistente)       │
                                    │   --mode pipe       │
                                    │   --format json     │
                                    └─────────────────────┘
```

### M1: ACP Protocol Handler

**File**: `src/acp/acp_server.c`

```c
// Metodi da implementare:
// - initialize → return capabilities + agent list
// - session/new → spawn convergio CLI, return sessionId
// - session/cancel → kill session
```

### M2: ACP Prompt Handler

**File**: `src/acp/acp_prompt.c`

```c
// Metodi da implementare:
// - session/prompt → forward a CLI, stream response
// - session/update notifications → chunk streaming
```

### M3: Bridge

**File**: `src/acp/convergio_bridge.c`

```c
// Logica:
// - Spawn convergio --mode pipe --format json
// - Forward messaggi
// - Parse JSON output → ACP notifications
```

### M4: Test Locale

```bash
# 1. Build
make convergio-acp

# 2. Install
cp build/bin/convergio-acp /usr/local/bin/

# 3. Config Zed (~/.config/zed/settings.json)
{
  "agent_servers": {
    "Convergio": {
      "type": "custom",
      "command": "/usr/local/bin/convergio-acp"
    }
  }
}

# 4. Riavvia Zed, apri Agent Panel (cmd-?)
```

---

## FILE DA CREARE

```
src/acp/
├── acp_server.c      # Main loop, JSON-RPC dispatch
├── acp_protocol.c    # Protocol types, serialize/deserialize
├── acp_prompt.c      # Prompt handling, streaming
└── convergio_bridge.c # Bridge to existing CLI

include/nous/
└── acp.h             # Header types
```

**Modifica Makefile**: aggiungere target `convergio-acp`

---

## DIPENDENZE

```
M1 ──┐
     ├──► M3 ──► M4
M2 ──┘
```

M1 e M2 parallelizzabili, M3 li unisce, M4 è il test finale.

---

## DEFINITION OF DONE (MVP)

- [ ] `convergio-acp` compila senza errori
- [ ] Zed riconosce Convergio nel pannello Agent
- [ ] Si può chattare con Ali
- [ ] Streaming funziona (token by token)
- [ ] Tool calls visibili in Zed

---

## NEXT STEPS AFTER MVP

Una volta che il MVP funziona:

1. **Feedback**: cosa manca? cosa non funziona?
2. **Agent Packs**: aggiungere business/education agents
3. **A11y Layer**: implementare come post-processing
4. **Publish**: creare extension.toml per distribuzione

---

**Piano aggiornato**: 2025-12-18
