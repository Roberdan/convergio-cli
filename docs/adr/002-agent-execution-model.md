# ADR-002: Agent Execution Model - Parallel vs Sequential

**Date**: 2024-12-10
**Status**: Approved
**Author**: AI Team

## Context

Il sistema di orchestrazione deve gestire multipli agenti che possono:
1. Lavorare in parallelo su task indipendenti
2. Eseguire in sequenza quando ci sono dipendenze
3. Convergere i risultati verso l'orchestrator (Ali)

## Decision

### Hybrid Execution Model

Utilizziamo **Grand Central Dispatch (GCD)** per massimizzare l'uso del M3 Max:

```
┌─────────────────────────────────────────────────────────────────┐
│                    ALI - ORCHESTRATOR                            │
│  Analizza il task → Decide execution strategy                    │
└─────────────────────────────────────────────────────────────────┘
                         │
           ┌─────────────┼─────────────┐
           ▼             ▼             ▼
    ┌──────────┐  ┌──────────┐  ┌──────────┐
    │ PARALLEL │  │ PIPELINE │  │  CRITIC  │
    │  GROUP   │  │  CHAIN   │  │  LOOP    │
    └──────────┘  └──────────┘  └──────────┘
```

### 1. Parallel Group (dispatch_group)

Per task indipendenti che possono eseguire contemporaneamente:

```c
dispatch_group_t group = dispatch_group_create();
dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);

// Launch agents in parallel on P-cores
dispatch_group_async(group, queue, ^{ agent_baccio_analyze(input); });
dispatch_group_async(group, queue, ^{ agent_sofia_analyze(input); });
dispatch_group_async(group, queue, ^{ agent_omri_analyze(input); });

// Wait and converge
dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
```

**Use cases:**
- "Analyze this project" → Baccio (tech) || Sofia (marketing) || Omri (data)
- "Review this code" → Luca (security) || Thor (quality) || Dan (engineering)

### 2. Pipeline Chain (sequential)

Per task con dipendenze dove l'output di uno alimenta il successivo:

```c
// Sequential execution with dependency
char* strategy = agent_domik_decide(input);          // First: strategic decision
char* plan = agent_matteo_plan(strategy);            // Then: business plan
char* architecture = agent_baccio_design(plan);       // Then: tech architecture
char* timeline = agent_davide_schedule(architecture); // Finally: project plan
```

**Use cases:**
- Product launch: Strategy → Plan → Architecture → Schedule
- Decision making: Analysis → Options → Recommendation → Validation

### 3. Critic Loop (iterative refinement)

Per task che richiedono validazione e raffinamento:

```c
char* draft = agent_create(input);
int iterations = 0;
bool approved = false;

while (!approved && iterations < MAX_ITERATIONS) {
    char* critique = agent_thor_review(draft);
    if (is_approved(critique)) {
        approved = true;
    } else {
        draft = agent_refine(draft, critique);
    }
    iterations++;
}
```

**Use cases:**
- Code review: Baccio writes → Thor critiques → Baccio refines
- Strategy: Antonio proposes → Socrates challenges → Antonio improves

## M3 Max Optimization

### Core Allocation

```c
// P-cores (10) for interactive work
dispatch_queue_attr_t attr_p = dispatch_queue_attr_make_with_qos_class(
    DISPATCH_QUEUE_CONCURRENT,
    QOS_CLASS_USER_INTERACTIVE,  // Maps to P-cores
    0
);

// E-cores (4) for background work
dispatch_queue_attr_t attr_e = dispatch_queue_attr_make_with_qos_class(
    DISPATCH_QUEUE_CONCURRENT,
    QOS_CLASS_UTILITY,  // Maps to E-cores
    0
);
```

### Load Distribution

| Work Type | Cores | QoS Class |
|-----------|-------|-----------|
| Active agents (user-facing) | P-cores | USER_INTERACTIVE |
| Background analysis | E-cores | UTILITY |
| Memory consolidation | E-cores | BACKGROUND |
| GPU embeddings | GPU (30 cores) | N/A |

## Cost Implications

Parallel execution = multiple Claude API calls simultaneously.

**Mitigation:**
1. Budget check before parallel dispatch
2. Cost estimation before execution
3. Automatic throttling if budget low
4. Priority-based agent selection

```c
if (!cost_can_afford(3, AVG_INPUT_TOKENS, AVG_OUTPUT_TOKENS)) {
    // Run sequentially or reduce agent count
    return execute_sequential(input);
}
```

## Consequences

**Positive:**
- Maximum M3 Max utilization
- Faster response for multi-agent tasks
- Flexible execution strategies

**Negative:**
- Higher peak API costs (parallel calls)
- More complex error handling
- Need careful synchronization

## Status

**APPROVED** - Implementing with GCD for parallel execution.

---

*This decision leverages M3 Max's 10 P-cores + 4 E-cores for optimal agent parallelization.*
