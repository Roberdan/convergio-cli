# Convergio Threading and Lock Hierarchy

**Document Version**: 1.0
**Last Updated**: 2025-12-14
**Author**: Roberto D'Angelo & AI Team

---

## Overview

Convergio uses POSIX threads (pthreads) for concurrent operations including:
- Provider API calls (parallel model queries)
- Streaming response handling
- Background status bar updates
- Database operations
- File locking for multi-instance coordination

This document describes the lock hierarchy to prevent deadlocks.

---

## Lock Hierarchy (Acquisition Order)

**CRITICAL**: Always acquire locks in this order to prevent deadlocks.

```
Level 1 (Highest - acquire first)
├── g_db_mutex (persistence.c)          - Database access
│
Level 2
├── g_registry_mutex (provider.c)       - Provider registry
├── g_lock_mutex (file_lock.c)          - File lock registry
│
Level 3
├── Provider-specific mutexes           - Individual provider state
│   ├── AnthropicProviderData.mutex
│   ├── OpenAIProviderData.mutex
│   ├── GeminiProviderData.mutex
│   ├── OllamaProviderData.mutex
│   ├── OpenRouterProviderData.mutex
│   └── g_mlx_mutex
│
Level 4
├── g_retry_mutex (retry.c)             - Retry state
├── g_status_mutex (statusbar.c)        - Status bar
├── terminal.mutex (terminal.c)         - Terminal output
│
Level 5 (Lowest - acquire last)
├── StreamContext.mutex (streaming.c)   - Per-stream state
├── MCPServer.lock (mcp_client.c)       - Per-server MCP state
├── CostOptimizer mutexes               - Cost tracking
└── g_wait_mutex (file_lock.c)          - Wait queue
```

---

## Lock Inventory

### Database Layer (Level 1)

| Lock | File | Purpose | Notes |
|------|------|---------|-------|
| `g_db_mutex` | persistence.c:29 | SQLite database access | Shared by todo.c, notify.c |

**Usage Pattern**:
```c
extern pthread_mutex_t g_db_mutex;
CONVERGIO_MUTEX_LOCK(&g_db_mutex);
// ... database operations ...
CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
```

### Provider Layer (Levels 2-3)

| Lock | File | Purpose | Notes |
|------|------|---------|-------|
| `g_registry_mutex` | provider.c:25 | Provider registry access | Guards provider list |
| `AnthropicProviderData.mutex` | anthropic.c:47 | Anthropic state | Per-provider |
| `OpenAIProviderData.mutex` | openai.c:48 | OpenAI state | Per-provider |
| `GeminiProviderData.mutex` | gemini.c:44 | Gemini state | Per-provider |
| `OllamaProviderData.mutex` | ollama.c:47 | Ollama state | Per-provider |
| `OpenRouterProviderData.mutex` | openrouter.c:47 | OpenRouter state | Per-provider |
| `g_mlx_mutex` | mlx.m:182 | MLX state | Static global |

**Pattern**: Provider mutexes are independent - no cross-provider locking required.

### UI Layer (Level 4)

| Lock | File | Purpose | Notes |
|------|------|---------|-------|
| `g_status_mutex` | statusbar.c:66 | Status bar updates | Static initializer |
| `terminal.mutex` | terminal.c:49 | Terminal output | Per-terminal instance |

### Infrastructure (Levels 4-5)

| Lock | File | Purpose | Notes |
|------|------|---------|-------|
| `g_retry_mutex` | retry.c:52 | Retry tracking | Short-held |
| `g_lock_mutex` | file_lock.c:24 | Lock registry | File coordination |
| `g_wait_mutex` | file_lock.c:489 | Wait queue | Always last |
| `StreamContext.mutex` | streaming.c:62 | Per-stream | Short-lived |
| `MCPServer.lock` | mcp_client.c:57 | Per-MCP server | Independent |

### Cost Optimizer (Level 5)

| Lock | File | Purpose | Notes |
|------|------|---------|-------|
| `CostBudget.mutex` | cost_optimizer.c:47 | Budget tracking | Per-budget |
| `CostAlert.mutex` | cost_optimizer.c:78 | Alert state | Per-alert |
| `CostOptimizer.mutex` | cost_optimizer.c:101 | Optimizer state | Main state |

---

## Deadlock Prevention Rules

### Rule 1: Follow the Hierarchy
Always acquire locks in the order specified above. Never acquire a higher-level lock while holding a lower-level lock.

```c
// CORRECT
pthread_mutex_lock(&g_db_mutex);       // Level 1
pthread_mutex_lock(&provider->mutex);  // Level 3
// ... work ...
pthread_mutex_unlock(&provider->mutex);
pthread_mutex_unlock(&g_db_mutex);

// WRONG - violates hierarchy
pthread_mutex_lock(&provider->mutex);  // Level 3
pthread_mutex_lock(&g_db_mutex);       // Level 1 - DEADLOCK RISK!
```

### Rule 2: Same-Level Locks
For locks at the same level, use a consistent ordering (e.g., alphabetical by name, or by memory address).

```c
// When locking multiple providers, use consistent order
Provider* providers[] = {anthropic, openai, gemini};
qsort(providers, 3, sizeof(Provider*), compare_by_address);
for (int i = 0; i < 3; i++) {
    pthread_mutex_lock(&providers[i]->data->mutex);
}
```

### Rule 3: Hold Locks Briefly
Minimize time spent holding locks. Don't perform I/O or long computations while holding locks.

```c
// CORRECT - copy data, release lock, then process
pthread_mutex_lock(&mutex);
char* data_copy = strdup(shared_data);
pthread_mutex_unlock(&mutex);
process(data_copy);  // Outside lock
free(data_copy);

// WRONG - holding lock during I/O
pthread_mutex_lock(&mutex);
send_to_api(shared_data);  // Network I/O while holding lock!
pthread_mutex_unlock(&mutex);
```

### Rule 4: No Lock Escalation
Don't try to acquire the same lock recursively unless using PTHREAD_MUTEX_RECURSIVE.

### Rule 5: Error Handling
Always release locks in error paths using goto cleanup or RAII patterns.

```c
int result = -1;
pthread_mutex_lock(&mutex);

if (!validate()) goto cleanup;
if (!process()) goto cleanup;
result = 0;

cleanup:
    pthread_mutex_unlock(&mutex);
    return result;
```

---

## Common Patterns

### Provider API Call
```c
Provider* provider = provider_get(PROVIDER_ANTHROPIC);
// Provider mutex is acquired internally by provider->chat()
char* response = provider->chat(provider, model, system, user, &usage);
// Mutex released before return
```

### Database Transaction
```c
CONVERGIO_MUTEX_LOCK(&g_db_mutex);
sqlite3_exec(g_db, "BEGIN TRANSACTION", ...);
// ... multiple operations ...
sqlite3_exec(g_db, "COMMIT", ...);
CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
```

### Streaming Response
```c
// StreamContext.mutex protects accumulated buffer
// Automatically managed by streaming.c
provider->stream_chat(provider, model, system, user, &handler, &usage);
```

---

## Testing for Deadlocks

### ThreadSanitizer (TSan)
Build with sanitizers to detect data races and potential deadlocks:

```bash
make debug  # Enables -fsanitize=address,undefined
```

### Helgrind (Valgrind)
```bash
valgrind --tool=helgrind ./build/bin/convergio
```

### Lock Logging (Debug)
Enable lock logging by setting:
```c
#define LOCK_DEBUG 1  // In persistence.c or target file
```

---

## Known Threading Constraints

1. **SQLite**: Single writer, multiple readers. All writes go through g_db_mutex.
2. **curl**: curl_global_init() must be called once before any threads (handled in main.c).
3. **MLX/Metal**: GPU operations are serialized by g_mlx_mutex.
4. **Terminal**: Output must be serialized to prevent garbled display.

---

## Adding New Locks

When adding a new mutex:

1. Determine its level in the hierarchy
2. Document it in this file
3. Ensure all acquisition paths follow the hierarchy
4. Add appropriate comments in code:

```c
// Lock hierarchy level: 3 (Provider layer)
// Must not hold: Level 1-2 locks
// May hold: Level 4-5 locks
static pthread_mutex_t my_new_mutex = PTHREAD_MUTEX_INITIALIZER;
```

---

## References

- [POSIX Threads Programming](https://computing.llnl.gov/tutorials/pthreads/)
- [ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html)
- [SQLite Threading Modes](https://www.sqlite.org/threadsafe.html)
