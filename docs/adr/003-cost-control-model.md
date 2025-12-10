# ADR-003: Cost Control Model

**Date**: 2024-12-10
**Status**: Approved
**Author**: AI Team

## Context

L'utente richiede:
- Sapere esattamente quanto sta spendendo
- Budget caps per evitare sorprese
- Tracciamento per agente, sessione, totale
- Trasparenza completa sui costi

## Decision

### Multi-Level Cost Tracking

```
┌─────────────────────────────────────────────────────────────────┐
│                    COST CONTROLLER                               │
├─────────────────────────────────────────────────────────────────┤
│  Per-Message   │  Per-Agent    │  Per-Session  │  All-Time      │
│  ─────────────  ─────────────   ─────────────   ─────────────   │
│  input_tokens   input_tokens    input_tokens    input_tokens    │
│  output_tokens  output_tokens   output_tokens   output_tokens   │
│  cost_usd       cost_usd        cost_usd        cost_usd        │
│  timestamp      api_calls       api_calls       api_calls       │
└─────────────────────────────────────────────────────────────────┘
```

### Pricing Model

Claude Sonnet 4 (current model):
- **Input**: $3.00 / 1M tokens
- **Output**: $15.00 / 1M tokens

```c
#define CLAUDE_SONNET_INPUT_COST   3.00   // $ per 1M input tokens
#define CLAUDE_SONNET_OUTPUT_COST  15.00  // $ per 1M output tokens

static double calculate_cost(uint64_t input, uint64_t output) {
    return (input / 1e6) * CLAUDE_SONNET_INPUT_COST +
           (output / 1e6) * CLAUDE_SONNET_OUTPUT_COST;
}
```

### Budget Enforcement

```c
typedef struct {
    double session_budget;    // Max per session (e.g., $5.00)
    double daily_budget;      // Max per day
    double monthly_budget;    // Max per month
    bool auto_pause_80pct;    // Warning at 80%
    bool hard_stop_100pct;    // Stop at 100%
} BudgetConfig;
```

**Enforcement levels:**
1. **Warning (80%)**: Notifica all'utente, continua
2. **Soft cap (95%)**: Chiede conferma per continuare
3. **Hard cap (100%)**: Stop, nessuna chiamata API

### Real-Time Display

Ogni prompt mostra lo stato:

```
convergio [$0.0234 spent | $4.98 remaining]>
```

Report dettagliato con comando `cost`:

```
╔══════════════════════════════════════════════════════════════╗
║                    COST REPORT                               ║
╠══════════════════════════════════════════════════════════════╣
║ SESSION (23 min)
║   Input tokens:       12,456 ($0.0374)
║   Output tokens:       3,891 ($0.0584)
║   API calls:              7
║   Total cost:         $0.0958
╠══════════════════════════════════════════════════════════════╣
║ TOP AGENTS BY COST
║   1. ali        - $0.0421 (44%)
║   2. baccio     - $0.0287 (30%)
║   3. omri       - $0.0250 (26%)
╠══════════════════════════════════════════════════════════════╣
║ Budget: $0.10 / $5.00 (2%)
╚══════════════════════════════════════════════════════════════╝
```

### Token Counting

**Pre-call estimation** (rough, for budget check):
```c
// ~3-4 chars per token for English
uint64_t estimate_tokens(const char* text) {
    return (strlen(text) / 3) + 1;
}
```

**Post-call actual** (from API response):
```json
{
  "usage": {
    "input_tokens": 1234,
    "output_tokens": 567
  }
}
```

### Persistence

Costs are persisted to SQLite for:
- Historical analysis
- Daily/monthly aggregates
- Per-agent tracking over time

```sql
-- Daily aggregates
CREATE TABLE cost_history (
  date TEXT PRIMARY KEY,
  input_tokens INTEGER,
  output_tokens INTEGER,
  total_cost REAL,
  api_calls INTEGER
);

-- Per-agent per-session
CREATE TABLE agent_usage (
  agent_name TEXT,
  session_id TEXT,
  input_tokens INTEGER,
  output_tokens INTEGER,
  cost_usd REAL
);
```

## API Design

```c
// Record usage after each API call
void cost_record_usage(uint64_t input_tokens, uint64_t output_tokens);
void cost_record_agent_usage(ManagedAgent* agent, uint64_t in, uint64_t out);

// Query costs
double cost_get_session_spend(void);
double cost_get_total_spend(void);
double cost_get_remaining_budget(void);
bool cost_check_budget(void);  // Returns false if exceeded

// Budget management
void cost_set_budget(double limit_usd);
void cost_reset_session(void);

// Reporting
char* cost_get_report(void);
char* cost_get_status_line(void);  // For prompt
char* cost_get_agent_report(ManagedAgent* agent);
```

## Consequences

**Positive:**
- Complete transparency
- No surprise bills
- Per-agent attribution helps optimize usage
- Historical data for analysis

**Negative:**
- Slight overhead per API call
- Token estimation is approximate pre-call

## Implementation Status

**IMPLEMENTED** in `src/orchestrator/cost.c`

---

*Transparency is core to trust. Every token is tracked.*
