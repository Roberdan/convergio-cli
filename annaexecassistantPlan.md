# Anna Executive Assistant - Implementation Plan

**Issue**: [#36](https://github.com/Roberdan/convergio-cli/issues/36)
**ADR**: [ADR-009: Anna Executive Assistant](docs/adr/009-anna-executive-assistant.md)
**Created**: 2025-12-14
**Status**: In Progress (Phase 1 near completion)
**Last Updated**: 2025-12-14

---

## EXECUTION STATUS

### Phase 1: Native Todo Manager - 90% Complete

| Task | Status | Notes |
|------|--------|-------|
| Database schema migration | **DONE** | Tables: tasks, notification_queue, inbox with FTS5 |
| Todo API (`todo.h`, `todo.c`) | **DONE** | Full CRUD, search, inbox, stats |
| CLI commands `/todo` | **DONE** | add, list, done, start, delete, inbox, search, stats |
| Natural language date parsing | **DONE** | English + Italian: "tomorrow", "domani", "next monday", "lunedi prossimo" |
| Enhanced date parsing | **DONE** | "tonight", "tomorrow morning", "at 3pm", "alle 14", "thursday in two weeks" |
| Quick `/remind` command | **DONE** | Flexible syntax: `/remind "msg" when` or `/remind when "msg"` with `--note` |
| `/reminders` command | **DONE** | View today/week/all reminders |
| Makefile integration | **DONE** | todo module added to build |
| Merge with main | **IN PROGRESS** | Resolving merge conflict in commands.c |
| Compile and test | **PENDING** | Fixing API mismatches |

### Phase 2: Notification System - Not Started

| Task | Status |
|------|--------|
| Notification API design | PENDING |
| terminal-notifier integration | PENDING |
| osascript fallback | PENDING |
| Daemon implementation | PENDING |
| LaunchAgent plist | PENDING |
| Daemon CLI commands | PENDING |

### Phase 3: Generic MCP Client - Not Started

| Task | Status |
|------|--------|
| MCP Client API | PENDING |
| JSON-RPC 2.0 implementation | PENDING |
| stdio transport | PENDING |
| HTTP transport | PENDING |
| Tool discovery | PENDING |
| Configuration loader | PENDING |

### Phase 4: Anna Agent - Not Started

| Task | Status |
|------|--------|
| Agent definition | PENDING |
| Custom tools (TodoRead, TodoWrite, etc.) | PENDING |
| Ali integration | PENDING |
| Natural language intent recognition | PENDING |

### Phase 5: Documentation - Partially Done

| Task | Status |
|------|--------|
| `/help todo` documentation | **DONE** (`docs/help/todo.md`) |
| `/help remind` documentation | **DONE** (`docs/help/remind.md`) |
| ADR-009 update | PENDING (needs completion) |
| README updates | PENDING |
| ANNA.md comprehensive docs | PENDING |

---

## KEY FILES CREATED/MODIFIED

### New Files
- `include/nous/todo.h` - Todo Manager API
- `src/todo/todo.c` - Todo Manager implementation
- `docs/help/todo.md` - /todo command help
- `docs/help/remind.md` - /remind command help

### Modified Files
- `src/memory/persistence.c` - Added todo schema
- `src/core/commands/commands.c` - Added /todo, /remind, /reminders commands
- `src/orchestrator/orchestrator.c` - Added todo_init/shutdown
- `Makefile` - Added todo module

---

## NEXT STEPS

1. **Complete merge conflict resolution** - Fix API mismatches in commands.c
2. **Compile and test** - Verify todo/remind/reminders work correctly
3. **Commit merge** - Complete the merge from main
4. **Update ADR-009** - Add comprehensive architecture documentation
5. **Phase 2: Notification daemon** - Enable scheduled reminders

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Parallelization & Optimization Strategy](#parallelization--optimization-strategy)
3. [Architecture](#architecture)
4. [Phase 1: Native Todo Manager](#phase-1-native-todo-manager)
5. [Phase 2: Notification System](#phase-2-notification-system)
6. [Phase 3: Generic MCP Client](#phase-3-generic-mcp-client)
7. [Phase 4: Anna Agent](#phase-4-anna-agent)
8. [Phase 5: Documentation & Polish](#phase-5-documentation--polish)
9. [Testing Strategy](#testing-strategy)
10. [Risk Mitigation](#risk-mitigation)
11. [Success Metrics](#success-metrics)

---

## Executive Summary

Anna is a personal Executive Assistant agent that provides:
- **Native task management** with local SQLite persistence
- **Reminder notifications** via macOS native notifications
- **Generic MCP client** for calendar, email, and travel integrations
- **Natural language interface** compatible with all LLM providers (online and local)

### Key Design Principles

1. **Model-Agnostic**: Core features work without any LLM; NL layer uses any configured provider
2. **Privacy-First**: All data stored locally by default
3. **Graceful Degradation**: System remains functional even when components fail
4. **Zero Lock-in**: User chooses which MCP servers to enable

---

## Parallelization & Optimization Strategy

### Development Parallelization

The implementation is designed for **maximum parallel execution** by multiple developers/agents:

```
Week 1-2          Week 2-3          Week 3-4          Week 4-5          Week 5-6          Week 6-7
────────────────────────────────────────────────────────────────────────────────────────────────────

TRACK A: Todo Manager ─────────────────────────────────────────────────────────────────────────────►
[Schema] → [CRUD API] → [CLI Commands] → [Search/Recurrence] → [Integration Tests]

TRACK B: Notification System ──────────────────────────────────────────────────────────────────────►
         [API Design] → [Dispatcher] → [Daemon] → [LaunchAgent] → [Health Monitor]

TRACK C: MCP Client ───────────────────────────────────────────────────────────────────────────────►
                      [JSON-RPC] → [stdio Transport] → [HTTP Transport] → [Tool Discovery] → [CLI]

TRACK D: Anna Agent ───────────────────────────────────────────────────────────────────────────────►
                                              [Agent Def] → [Tools] → [Ali Integration]

TRACK E: Documentation ────────────────────────────────────────────────────────────────────────────►
                                                                   [README] → [Docs] → [ADR]
```

### Parallel Track Dependencies

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     DEPENDENCY GRAPH                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌──────────────┐                                                           │
│  │ Track A:     │                                                           │
│  │ Todo Manager │──────────────────────────────────────┐                    │
│  └──────────────┘                                      │                    │
│         │                                              │                    │
│         │ (schema)                                     │                    │
│         ▼                                              ▼                    │
│  ┌──────────────┐                              ┌──────────────┐            │
│  │ Track B:     │                              │ Track D:     │            │
│  │ Notifications│──────────────────────────────│ Anna Agent   │            │
│  └──────────────┘                              └──────────────┘            │
│         │                                              │                    │
│         └──────────────┬───────────────────────────────┘                    │
│                        │                                                     │
│                        ▼                                                     │
│                 ┌──────────────┐                                            │
│                 │ Track E:     │                                            │
│                 │ Documentation│                                            │
│                 └──────────────┘                                            │
│                                                                              │
│  ┌──────────────┐                                                           │
│  │ Track C:     │ ←── Independent! Can run fully in parallel               │
│  │ MCP Client   │                                                           │
│  └──────────────┘                                                           │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Parallelization Rules

| Track | Can Start | Blocked By | Blocks |
|-------|-----------|------------|--------|
| **A: Todo Manager** | Immediately | None | B (schema), D (API) |
| **B: Notifications** | After A schema | A (schema) | D (scheduler) |
| **C: MCP Client** | Immediately | None | D (MCP tools) |
| **D: Anna Agent** | After A+B+C APIs | A, B, C APIs | E |
| **E: Documentation** | Incrementally | Feature completion | None |

### Runtime Optimization

#### 1. Apple Silicon Core Utilization

Following existing `scheduler.c` patterns:

```c
// Task → Core mapping
┌─────────────────────────────────────────────────────────────┐
│ Task Type              │ Core Class        │ QoS            │
├────────────────────────┼───────────────────┼────────────────┤
│ User CLI commands      │ P-cores           │ USER_INTERACTIVE│
│ Todo CRUD operations   │ P-cores           │ USER_INITIATED  │
│ Full-text search       │ P-cores           │ USER_INITIATED  │
│ Notification check     │ E-cores           │ UTILITY         │
│ MCP server spawn       │ E-cores           │ UTILITY         │
│ Database maintenance   │ E-cores           │ BACKGROUND      │
│ Config file parsing    │ E-cores           │ BACKGROUND      │
└─────────────────────────────────────────────────────────────┘
```

#### 2. Database Optimization

```sql
-- Connection pooling
PRAGMA journal_mode = WAL;          -- Write-Ahead Logging for concurrency
PRAGMA synchronous = NORMAL;        -- Balance durability/performance
PRAGMA cache_size = -2000;          -- 2MB cache
PRAGMA temp_store = MEMORY;         -- Temp tables in memory

-- Prepared statement caching
-- Reuse statements for frequent queries (todo list, reminder check)
```

#### 3. MCP Connection Pooling

```c
// Lazy connection establishment
// Connect only when first tool is called
// Keep connections alive for 5 minutes
// Parallel tool calls to multiple servers

typedef struct {
    MCPServer* servers[MAX_MCP_SERVERS];
    dispatch_queue_t connection_queue;  // Serial queue for connection management
    dispatch_queue_t call_queue;        // Concurrent queue for tool calls
} MCPConnectionPool;
```

#### 4. Notification Batching

```c
// Batch check: Query all due reminders in one query
// Batch send: Dispatch notifications in parallel
// Coalesce: Group notifications within 1-second window

void check_pending_reminders(void) {
    // Single query for all due notifications
    Notification** batch = query_due_notifications(time(NULL));

    // Dispatch in parallel on E-cores
    dispatch_apply(batch_count, g_efficiency_queue, ^(size_t i) {
        send_notification(batch[i]);
    });
}
```

### Engineering Best Practices (per CONTRIBUTING.md)

#### Code Style
- **Naming**: `snake_case` functions, `PascalCase` types, `g_` globals
- **Indentation**: 4 spaces, K&R braces
- **Headers**: Include guards with `#ifndef`/`#define`/`#endif`

#### Testing
- Unit tests for each component before integration
- Integration tests with mocked MCP servers
- End-to-end tests for critical paths

#### Documentation
- ADR for architectural decisions
- Inline comments for complex logic
- README updates for user-facing features

#### Pull Request Flow
```
feature/anna-todo-manager      →  PR → Review → Merge
feature/anna-notifications     →  PR → Review → Merge
feature/anna-mcp-client       →  PR → Review → Merge
feature/anna-agent            →  PR → Review → Merge
```

### Resource Optimization

| Resource | Limit | Monitoring |
|----------|-------|------------|
| Memory | < 20MB daemon | `convergio daemon health` |
| CPU | E-cores only for daemon | QoS_CLASS_UTILITY |
| Disk | WAL mode, async writes | PRAGMA synchronous=NORMAL |
| Network | Connection pooling | Timeout + retry |
| File descriptors | Close unused | RAII pattern |

---

## Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           CONVERGIO CLI                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                        USER INTERFACE                            │    │
│  │                                                                  │    │
│  │   CLI Commands              Natural Language (via any LLM)      │    │
│  │   convergio todo add ...    convergio ask anna "remind me..."   │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│                                    │                                     │
│                    ┌───────────────┴───────────────┐                    │
│                    ▼                               ▼                    │
│  ┌─────────────────────────────┐   ┌─────────────────────────────┐     │
│  │      ANNA AGENT             │   │     DIRECT CLI HANDLER      │     │
│  │  (requires LLM)             │   │   (no LLM required)         │     │
│  │                             │   │                             │     │
│  │  • Intent recognition       │   │  • Argument parsing         │     │
│  │  • Context awareness        │   │  • Direct API calls         │     │
│  │  • Multi-step planning      │   │  • Structured output        │     │
│  └─────────────────────────────┘   └─────────────────────────────┘     │
│                    │                               │                    │
│                    └───────────────┬───────────────┘                    │
│                                    ▼                                     │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                        CORE SERVICES                             │    │
│  ├─────────────────┬─────────────────┬─────────────────────────────┤    │
│  │                 │                 │                             │    │
│  │  TODO MANAGER   │  NOTIFICATION   │  MCP CLIENT                 │    │
│  │                 │  DAEMON         │                             │    │
│  │  • CRUD ops     │  • Scheduler    │  • JSON-RPC 2.0            │    │
│  │  • Recurrence   │  • Dispatcher   │  • stdio/HTTP transport    │    │
│  │  • Tags/Context │  • Fallbacks    │  • Tool discovery          │    │
│  │  • Search       │  • Health mon   │  • Multi-server            │    │
│  │                 │                 │                             │    │
│  └────────┬────────┴────────┬────────┴──────────────┬──────────────┘    │
│           │                 │                       │                    │
│           ▼                 ▼                       ▼                    │
│  ┌─────────────┐   ┌─────────────────┐   ┌───────────────────────┐     │
│  │   SQLite    │   │  macOS Native   │   │  External MCP Servers │     │
│  │ convergio.db│   │  Notifications  │   │  (user configured)    │     │
│  └─────────────┘   └─────────────────┘   └───────────────────────┘     │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### File Structure

```
src/
├── todo/
│   ├── todo.h                 # Public API
│   ├── todo.c                 # CRUD operations
│   ├── todo_cli.c             # CLI command handlers
│   ├── todo_recurrence.c      # RRULE parsing
│   └── todo_search.c          # Full-text search
├── notifications/
│   ├── notify.h               # Public API
│   ├── notify.c               # Notification dispatcher
│   ├── notify_macos.c         # macOS-specific (terminal-notifier, osascript)
│   ├── daemon.c               # Daemon main loop
│   └── daemon_health.c        # Health monitoring
├── mcp/
│   ├── mcp_client.h           # Public API
│   ├── mcp_client.c           # Client lifecycle
│   ├── mcp_jsonrpc.c          # JSON-RPC 2.0 parser
│   ├── mcp_transport_stdio.c  # stdio transport
│   ├── mcp_transport_http.c   # HTTP transport
│   ├── mcp_tools.c            # Tool discovery & invocation
│   └── mcp_config.c           # Config file loader
├── agents/definitions/
│   └── anna-executive-assistant.md
include/
├── nous/
│   ├── todo.h
│   ├── notify.h
│   └── mcp_client.h
```

---

## Phase 1: Native Todo Manager

### Duration: Week 1-2

### 1.1 Database Schema Migration

**File**: `src/memory/persistence.c` (extend existing)

```sql
-- Add to existing schema in persistence.c

-- Tasks table
CREATE TABLE IF NOT EXISTS tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    description TEXT,
    priority INTEGER DEFAULT 2 CHECK (priority BETWEEN 1 AND 3),
    status TEXT DEFAULT 'pending' CHECK (status IN ('pending', 'in_progress', 'completed', 'cancelled')),
    due_date TEXT,  -- ISO 8601 format
    reminder_at TEXT,  -- ISO 8601 format
    recurrence TEXT CHECK (recurrence IN (NULL, 'none', 'daily', 'weekly', 'monthly', 'yearly', 'custom')),
    recurrence_rule TEXT,  -- iCal RRULE format
    tags TEXT,  -- JSON array
    context TEXT,  -- project name, person, etc.
    parent_id INTEGER REFERENCES tasks(id) ON DELETE CASCADE,
    source TEXT DEFAULT 'user' CHECK (source IN ('user', 'agent', 'mcp_sync')),
    external_id TEXT,  -- for sync with external systems
    created_at TEXT DEFAULT (datetime('now')),
    updated_at TEXT DEFAULT (datetime('now')),
    completed_at TEXT
);

-- Notification queue
CREATE TABLE IF NOT EXISTS notification_queue (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id INTEGER NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    scheduled_at TEXT NOT NULL,  -- ISO 8601
    method TEXT DEFAULT 'native' CHECK (method IN ('native', 'terminal', 'sound', 'log')),
    status TEXT DEFAULT 'pending' CHECK (status IN ('pending', 'sent', 'failed', 'acknowledged', 'snoozed')),
    retry_count INTEGER DEFAULT 0,
    max_retries INTEGER DEFAULT 3,
    last_error TEXT,
    sent_at TEXT,
    acknowledged_at TEXT
);

-- Quick capture inbox
CREATE TABLE IF NOT EXISTS inbox (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    content TEXT NOT NULL,
    captured_at TEXT DEFAULT (datetime('now')),
    processed INTEGER DEFAULT 0,
    processed_task_id INTEGER REFERENCES tasks(id),
    source TEXT DEFAULT 'cli' CHECK (source IN ('cli', 'agent', 'mcp', 'voice'))
);

-- Performance indexes
CREATE INDEX IF NOT EXISTS idx_tasks_due_date ON tasks(due_date) WHERE status = 'pending';
CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status);
CREATE INDEX IF NOT EXISTS idx_tasks_priority ON tasks(priority);
CREATE INDEX IF NOT EXISTS idx_tasks_context ON tasks(context);
CREATE INDEX IF NOT EXISTS idx_notification_scheduled ON notification_queue(scheduled_at) WHERE status = 'pending';
CREATE INDEX IF NOT EXISTS idx_inbox_unprocessed ON inbox(processed) WHERE processed = 0;

-- Full-text search for tasks
CREATE VIRTUAL TABLE IF NOT EXISTS tasks_fts USING fts5(
    title,
    description,
    tags,
    context,
    content='tasks',
    content_rowid='id'
);

-- Triggers to keep FTS in sync
CREATE TRIGGER IF NOT EXISTS tasks_ai AFTER INSERT ON tasks BEGIN
    INSERT INTO tasks_fts(rowid, title, description, tags, context)
    VALUES (new.id, new.title, new.description, new.tags, new.context);
END;

CREATE TRIGGER IF NOT EXISTS tasks_ad AFTER DELETE ON tasks BEGIN
    INSERT INTO tasks_fts(tasks_fts, rowid, title, description, tags, context)
    VALUES('delete', old.id, old.title, old.description, old.tags, old.context);
END;

CREATE TRIGGER IF NOT EXISTS tasks_au AFTER UPDATE ON tasks BEGIN
    INSERT INTO tasks_fts(tasks_fts, rowid, title, description, tags, context)
    VALUES('delete', old.id, old.title, old.description, old.tags, old.context);
    INSERT INTO tasks_fts(rowid, title, description, tags, context)
    VALUES (new.id, new.title, new.description, new.tags, new.context);
END;
```

### 1.2 Todo Manager API

**File**: `include/nous/todo.h`

```c
#ifndef NOUS_TODO_H
#define NOUS_TODO_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Priority levels
typedef enum {
    TODO_PRIORITY_URGENT = 1,
    TODO_PRIORITY_NORMAL = 2,
    TODO_PRIORITY_LOW = 3
} TodoPriority;

// Task status
typedef enum {
    TODO_STATUS_PENDING,
    TODO_STATUS_IN_PROGRESS,
    TODO_STATUS_COMPLETED,
    TODO_STATUS_CANCELLED
} TodoStatus;

// Recurrence types
typedef enum {
    TODO_RECURRENCE_NONE,
    TODO_RECURRENCE_DAILY,
    TODO_RECURRENCE_WEEKLY,
    TODO_RECURRENCE_MONTHLY,
    TODO_RECURRENCE_YEARLY,
    TODO_RECURRENCE_CUSTOM  // Uses recurrence_rule (RRULE)
} TodoRecurrence;

// Task structure
typedef struct {
    int64_t id;
    char* title;
    char* description;
    TodoPriority priority;
    TodoStatus status;
    time_t due_date;          // 0 if no due date
    time_t reminder_at;       // 0 if no reminder
    TodoRecurrence recurrence;
    char* recurrence_rule;    // iCal RRULE for custom
    char** tags;              // NULL-terminated array
    int tag_count;
    char* context;            // project, person, etc.
    int64_t parent_id;        // 0 if no parent
    char* source;             // user, agent, mcp_sync
    char* external_id;        // for external sync
    time_t created_at;
    time_t updated_at;
    time_t completed_at;      // 0 if not completed
} TodoTask;

// Filter options for listing
typedef struct {
    TodoStatus* statuses;     // NULL = all
    int status_count;
    TodoPriority* priorities; // NULL = all
    int priority_count;
    time_t due_from;          // 0 = no filter
    time_t due_to;            // 0 = no filter
    char* context;            // NULL = all
    char** tags;              // NULL = all
    int tag_count;
    char* search_query;       // FTS query, NULL = no search
    bool include_completed;
    int limit;                // 0 = no limit
    int offset;
} TodoFilter;

// Create options
typedef struct {
    const char* title;        // Required
    const char* description;  // Optional
    TodoPriority priority;    // Default: NORMAL
    time_t due_date;          // 0 = no due date
    time_t reminder_at;       // 0 = no reminder, -1 = auto (based on due)
    TodoRecurrence recurrence;
    const char* recurrence_rule;
    const char** tags;
    int tag_count;
    const char* context;
    int64_t parent_id;        // 0 = no parent
} TodoCreateOptions;

// API Functions
int todo_init(void);
void todo_shutdown(void);

// CRUD
int64_t todo_create(const TodoCreateOptions* options);
TodoTask* todo_get(int64_t id);
int todo_update(int64_t id, const TodoCreateOptions* options);
int todo_delete(int64_t id);

// Status changes
int todo_complete(int64_t id);
int todo_uncomplete(int64_t id);
int todo_cancel(int64_t id);
int todo_start(int64_t id);  // Mark as in_progress

// Listing
TodoTask** todo_list(const TodoFilter* filter, int* count);
TodoTask** todo_list_today(int* count);
TodoTask** todo_list_overdue(int* count);
TodoTask** todo_list_upcoming(int days, int* count);

// Search
TodoTask** todo_search(const char* query, int* count);

// Inbox
int64_t inbox_capture(const char* content, const char* source);
char** inbox_list_unprocessed(int* count);
int inbox_process(int64_t inbox_id, int64_t task_id);

// Bulk operations
int todo_archive_completed(int days_old);
int todo_delete_cancelled(int days_old);

// Memory management
void todo_free_task(TodoTask* task);
void todo_free_tasks(TodoTask** tasks, int count);

// Statistics
typedef struct {
    int total_pending;
    int total_in_progress;
    int total_completed_today;
    int total_overdue;
    int inbox_unprocessed;
} TodoStats;

TodoStats todo_get_stats(void);

#endif // NOUS_TODO_H
```

### 1.3 CLI Commands Implementation

**File**: `src/todo/todo_cli.c`

Commands to implement:

```bash
# Create tasks
convergio todo add <title> [options]
  --description, -d    Task description
  --priority, -p       1=urgent, 2=normal (default), 3=low
  --due                Due date (ISO 8601, or "today", "tomorrow", "next monday")
  --remind             Reminder time (ISO 8601, or "1h before", "1d before")
  --recurrence, -r     none|daily|weekly|monthly|yearly
  --tag, -t            Add tag (can use multiple times)
  --context, -c        Context (project, person)
  --parent             Parent task ID (for subtasks)

# List tasks
convergio todo list [options]
  --all, -a            Show all (including completed)
  --today              Today's tasks + overdue
  --week               This week's tasks
  --overdue            Only overdue
  --tag, -t            Filter by tag
  --context, -c        Filter by context
  --priority, -p       Filter by priority
  --search, -s         Full-text search
  --format, -f         output format: table (default), json, simple

# Manage tasks
convergio todo done <id>           Mark as completed
convergio todo undone <id>         Mark as pending again
convergio todo start <id>          Mark as in progress
convergio todo cancel <id>         Cancel task
convergio todo edit <id> [options] Edit task (same options as add)
convergio todo delete <id>         Delete task
convergio todo show <id>           Show task details

# Reminders
convergio todo remind <id> --at <datetime>
convergio todo remind <id> --before <duration>  # e.g., "30m", "1h", "1d"
convergio todo snooze <id> --until <datetime>
convergio todo snooze <id> --for <duration>

# Inbox (quick capture)
convergio inbox <content>          Quick capture to inbox
convergio inbox list               List unprocessed inbox items
convergio inbox process <id>       Convert inbox item to task

# Maintenance
convergio todo archive             Archive old completed tasks
convergio todo stats               Show statistics
```

### 1.4 Natural Language Date Parsing

**File**: `src/todo/todo_dateparse.c`

Support for natural language dates:
- "today", "tomorrow", "yesterday"
- "next monday", "this friday"
- "in 2 hours", "in 3 days"
- "end of day", "end of week"
- "2025-12-25", "Dec 25", "25/12/2025"

### 1.5 Tasks Checklist

- [ ] Database schema migration
- [ ] Todo API implementation (`todo.c`)
- [ ] CLI command handlers (`todo_cli.c`)
- [ ] Natural language date parsing (`todo_dateparse.c`)
- [ ] Recurrence rule parsing (`todo_recurrence.c`)
- [ ] Full-text search (`todo_search.c`)
- [ ] Inbox capture and processing
- [ ] Unit tests (`tests/test_todo.c`)
- [ ] Integration with existing command system

---

## Phase 2: Notification System

### Duration: Week 2-3

### 2.1 Notification API

**File**: `include/nous/notify.h`

```c
#ifndef NOUS_NOTIFY_H
#define NOUS_NOTIFY_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Notification methods (in priority order)
typedef enum {
    NOTIFY_METHOD_NATIVE,      // terminal-notifier or osascript
    NOTIFY_METHOD_TERMINAL,    // Print to terminal if active
    NOTIFY_METHOD_SOUND,       // Sound only
    NOTIFY_METHOD_LOG          // Log file only (last resort)
} NotifyMethod;

// Notification result
typedef enum {
    NOTIFY_SUCCESS = 0,
    NOTIFY_ERROR_NOT_AVAILABLE,
    NOTIFY_ERROR_PERMISSION_DENIED,
    NOTIFY_ERROR_TIMEOUT,
    NOTIFY_ERROR_UNKNOWN
} NotifyResult;

// Notification options
typedef struct {
    const char* title;
    const char* body;
    const char* subtitle;      // Optional
    const char* sound;         // Sound name, NULL for default
    const char* group;         // Notification group ID
    const char* action_url;    // URL to open on click
    int timeout_ms;            // 0 = system default
} NotifyOptions;

// Scheduled notification
typedef struct {
    int64_t id;
    int64_t task_id;
    time_t scheduled_at;
    NotifyMethod method;
    char* status;              // pending, sent, failed, acknowledged
    int retry_count;
    char* last_error;
} ScheduledNotification;

// API Functions
int notify_init(void);
void notify_shutdown(void);

// Immediate notification
NotifyResult notify_send(const NotifyOptions* options);
NotifyResult notify_send_simple(const char* title, const char* body);

// Check availability
bool notify_is_available(NotifyMethod method);
NotifyMethod notify_get_best_method(void);

// Scheduled notifications (persisted)
int64_t notify_schedule(int64_t task_id, time_t fire_at, NotifyMethod method);
int notify_cancel_scheduled(int64_t notification_id);
int notify_snooze(int64_t notification_id, time_t new_time);
ScheduledNotification** notify_list_pending(int* count);

// Daemon control
int notify_daemon_start(void);
int notify_daemon_stop(void);
bool notify_daemon_is_running(void);

// Health monitoring
typedef struct {
    bool daemon_running;
    pid_t daemon_pid;
    time_t daemon_started_at;
    int pending_notifications;
    int sent_last_24h;
    int failed_last_24h;
    NotifyMethod active_method;
    char* last_error;
} NotifyHealth;

NotifyHealth notify_get_health(void);

#endif // NOUS_NOTIFY_H
```

### 2.2 macOS Notification Implementation

**File**: `src/notifications/notify_macos.c`

```c
// Notification methods with automatic fallback

// Method 1: terminal-notifier (best UX)
static NotifyResult send_via_terminal_notifier(const NotifyOptions* opts) {
    // Check if available
    if (system("which terminal-notifier > /dev/null 2>&1") != 0) {
        return NOTIFY_ERROR_NOT_AVAILABLE;
    }

    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
        "terminal-notifier "
        "-title '%s' "
        "-message '%s' "
        "%s%s%s "  // subtitle
        "-sound '%s' "
        "-sender io.convergio.cli "
        "-group '%s' "
        "%s%s%s",  // action URL
        escape_single_quotes(opts->title),
        escape_single_quotes(opts->body),
        opts->subtitle ? "-subtitle '" : "",
        opts->subtitle ? escape_single_quotes(opts->subtitle) : "",
        opts->subtitle ? "'" : "",
        opts->sound ? opts->sound : "Glass",
        opts->group ? opts->group : "convergio",
        opts->action_url ? "-open '" : "",
        opts->action_url ? opts->action_url : "",
        opts->action_url ? "'" : ""
    );

    int result = system(cmd);
    return (result == 0) ? NOTIFY_SUCCESS : NOTIFY_ERROR_UNKNOWN;
}

// Method 2: osascript (built-in, no deps)
static NotifyResult send_via_osascript(const NotifyOptions* opts) {
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "osascript -e 'display notification \"%s\" "
        "with title \"%s\" "
        "%s%s%s "  // subtitle
        "sound name \"%s\"'",
        escape_double_quotes(opts->body),
        escape_double_quotes(opts->title),
        opts->subtitle ? "subtitle \"" : "",
        opts->subtitle ? escape_double_quotes(opts->subtitle) : "",
        opts->subtitle ? "\"" : "",
        opts->sound ? opts->sound : "Glass"
    );

    int result = system(cmd);
    // Note: osascript may fail silently on macOS Sequoia in some contexts
    return (result == 0) ? NOTIFY_SUCCESS : NOTIFY_ERROR_UNKNOWN;
}

// Method 3: Fallback to log
static NotifyResult send_via_log(const NotifyOptions* opts) {
    FILE* log = fopen("/tmp/convergio-notifications.log", "a");
    if (!log) return NOTIFY_ERROR_UNKNOWN;

    time_t now = time(NULL);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(log, "[%s] %s: %s\n", timestr, opts->title, opts->body);
    fclose(log);

    // Also print to stderr if terminal is available
    if (isatty(STDERR_FILENO)) {
        fprintf(stderr, "\n[CONVERGIO REMINDER] %s: %s\n", opts->title, opts->body);
    }

    return NOTIFY_SUCCESS;
}

// Main send function with fallback chain
NotifyResult notify_send(const NotifyOptions* options) {
    NotifyResult result;

    // Try terminal-notifier first
    result = send_via_terminal_notifier(options);
    if (result == NOTIFY_SUCCESS) return result;

    // Fallback to osascript
    result = send_via_osascript(options);
    if (result == NOTIFY_SUCCESS) return result;

    // Last resort: log only
    return send_via_log(options);
}
```

### 2.3 Daemon Implementation - Apple Silicon Optimized

**File**: `src/notifications/daemon.c`

The daemon is designed to **maximize Apple Silicon efficiency**:

#### Apple Silicon Optimization Strategy

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    APPLE SILICON DAEMON OPTIMIZATION                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                      CORE ALLOCATION                                 │    │
│  │                                                                      │    │
│  │  E-CORES ONLY (Efficiency Cores)                                    │    │
│  │  • QOS_CLASS_UTILITY for timer checks                               │    │
│  │  • QOS_CLASS_BACKGROUND for database maintenance                    │    │
│  │  • NEVER use P-cores for daemon work                                │    │
│  │  • Leaves P-cores 100% free for user interaction                    │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                      MEMORY OPTIMIZATION                             │    │
│  │                                                                      │    │
│  │  • Single SQLite connection (shared, not per-query)                 │    │
│  │  • Prepared statement cache (reuse hot queries)                     │    │
│  │  • Zero-copy string handling where possible                         │    │
│  │  • Arena allocator for batch operations                             │    │
│  │  • Target: < 8MB resident memory                                    │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                      ENERGY OPTIMIZATION                             │    │
│  │                                                                      │    │
│  │  • Timer coalescing (10s leeway for system batching)                │    │
│  │  • Adaptive polling (60s idle → 300s when no pending)               │    │
│  │  • ProcessType=Background in launchd (lowest energy)                │    │
│  │  • LowPriorityIO=true for disk operations                           │    │
│  │  • Nice=10 to yield to user processes                               │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                      UNIFIED MEMORY                                  │    │
│  │                                                                      │    │
│  │  • Leverage Apple Silicon unified memory architecture               │    │
│  │  • No CPU↔GPU data copies needed                                    │    │
│  │  • Memory-mapped SQLite database (mmap)                             │    │
│  │  • Shared memory for IPC with CLI (if needed)                       │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### Implementation

```c
// Daemon main loop - Apple Silicon optimized

#include <dispatch/dispatch.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <os/log.h>

// Logging subsystem (uses Apple's unified logging)
static os_log_t g_daemon_log = NULL;

// Daemon state
static struct {
    dispatch_source_t timer;
    dispatch_queue_t queue;
    sqlite3* db;                    // Single shared connection
    sqlite3_stmt* check_stmt;       // Prepared statement (reused)
    sqlite3_stmt* update_stmt;      // Prepared statement (reused)
    _Atomic bool running;
    _Atomic int pending_count;      // For adaptive polling
    uint64_t poll_interval_ns;      // Adaptive: 60s or 300s
} g_daemon = {0};

// Memory-efficient notification batch
typedef struct {
    int64_t id;
    int64_t task_id;
    char title[256];        // Fixed-size, no malloc
    char body[512];         // Fixed-size, no malloc
} NotificationBatch;

#define MAX_BATCH_SIZE 16
static NotificationBatch g_batch[MAX_BATCH_SIZE];  // Static allocation

// Initialize daemon with Apple Silicon optimizations
int notify_daemon_init(void) {
    // Create os_log for efficient logging
    g_daemon_log = os_log_create("io.convergio", "daemon");

    // E-cores only queue with UTILITY QoS
    dispatch_queue_attr_t attr = dispatch_queue_attr_make_with_qos_class(
        DISPATCH_QUEUE_SERIAL,
        QOS_CLASS_UTILITY,  // E-cores preferred
        -10                 // Relative priority (lower = more power efficient)
    );
    g_daemon.queue = dispatch_queue_create("io.convergio.daemon", attr);

    // Open database with memory-mapped I/O
    const char* db_path = get_db_path();
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX;
    if (sqlite3_open_v2(db_path, &g_daemon.db, flags, NULL) != SQLITE_OK) {
        os_log_error(g_daemon_log, "Failed to open database");
        return -1;
    }

    // SQLite optimizations for low-memory, low-power
    sqlite3_exec(g_daemon.db, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);
    sqlite3_exec(g_daemon.db, "PRAGMA synchronous = NORMAL", NULL, NULL, NULL);
    sqlite3_exec(g_daemon.db, "PRAGMA cache_size = -1000", NULL, NULL, NULL);  // 1MB cache
    sqlite3_exec(g_daemon.db, "PRAGMA mmap_size = 268435456", NULL, NULL, NULL);  // 256MB mmap
    sqlite3_exec(g_daemon.db, "PRAGMA temp_store = MEMORY", NULL, NULL, NULL);

    // Prepare statements (reused for all checks)
    const char* check_sql =
        "SELECT n.id, n.task_id, t.title, t.description "
        "FROM notification_queue n "
        "JOIN tasks t ON n.task_id = t.id "
        "WHERE n.status = 'pending' "
        "AND datetime(n.scheduled_at) <= datetime('now') "
        "ORDER BY n.scheduled_at ASC "
        "LIMIT ?";
    sqlite3_prepare_v3(g_daemon.db, check_sql, -1, SQLITE_PREPARE_PERSISTENT,
                       &g_daemon.check_stmt, NULL);

    const char* update_sql =
        "UPDATE notification_queue SET status = ?, sent_at = datetime('now') WHERE id = ?";
    sqlite3_prepare_v3(g_daemon.db, update_sql, -1, SQLITE_PREPARE_PERSISTENT,
                       &g_daemon.update_stmt, NULL);

    // Initial poll interval: 60 seconds
    g_daemon.poll_interval_ns = 60 * NSEC_PER_SEC;

    return 0;
}

// Adaptive polling: slow down when idle
static void adjust_poll_interval(void) {
    int pending = atomic_load(&g_daemon.pending_count);

    if (pending == 0) {
        // No pending notifications: slow down to 5 minutes
        g_daemon.poll_interval_ns = 300 * NSEC_PER_SEC;
        os_log_debug(g_daemon_log, "No pending, slowing to 5min interval");
    } else if (pending > 5) {
        // Many pending: speed up to 30 seconds
        g_daemon.poll_interval_ns = 30 * NSEC_PER_SEC;
        os_log_debug(g_daemon_log, "Many pending, speeding to 30s interval");
    } else {
        // Normal: 60 seconds
        g_daemon.poll_interval_ns = 60 * NSEC_PER_SEC;
    }

    // Update timer with new interval
    dispatch_source_set_timer(g_daemon.timer,
        dispatch_time(DISPATCH_TIME_NOW, (int64_t)g_daemon.poll_interval_ns),
        g_daemon.poll_interval_ns,
        10 * NSEC_PER_SEC  // 10s leeway for timer coalescing
    );
}

// Check and send notifications (runs on E-cores)
static void check_pending_reminders(void) {
    // Reset and bind prepared statement
    sqlite3_reset(g_daemon.check_stmt);
    sqlite3_bind_int(g_daemon.check_stmt, 1, MAX_BATCH_SIZE);

    int batch_count = 0;

    // Fetch into static batch buffer (zero malloc)
    while (sqlite3_step(g_daemon.check_stmt) == SQLITE_ROW && batch_count < MAX_BATCH_SIZE) {
        NotificationBatch* n = &g_batch[batch_count];
        n->id = sqlite3_column_int64(g_daemon.check_stmt, 0);
        n->task_id = sqlite3_column_int64(g_daemon.check_stmt, 1);

        const char* title = (const char*)sqlite3_column_text(g_daemon.check_stmt, 2);
        const char* body = (const char*)sqlite3_column_text(g_daemon.check_stmt, 3);

        strlcpy(n->title, title ? title : "", sizeof(n->title));
        strlcpy(n->body, body ? body : "", sizeof(n->body));

        batch_count++;
    }

    // Update pending count for adaptive polling
    atomic_store(&g_daemon.pending_count, batch_count);
    adjust_poll_interval();

    if (batch_count == 0) return;

    os_log_info(g_daemon_log, "Processing %d notifications", batch_count);

    // Send notifications (still on E-cores, sequential to avoid overwhelming)
    for (int i = 0; i < batch_count; i++) {
        NotificationBatch* n = &g_batch[i];

        NotifyOptions opts = {
            .title = "Reminder",
            .body = n->title,
            .subtitle = n->body[0] ? n->body : NULL,
            .group = "convergio-reminders"
        };

        NotifyResult result = notify_send(&opts);

        // Update status
        sqlite3_reset(g_daemon.update_stmt);
        sqlite3_bind_text(g_daemon.update_stmt, 1,
                          result == NOTIFY_SUCCESS ? "sent" : "failed", -1, SQLITE_STATIC);
        sqlite3_bind_int64(g_daemon.update_stmt, 2, n->id);
        sqlite3_step(g_daemon.update_stmt);

        if (result != NOTIFY_SUCCESS) {
            os_log_error(g_daemon_log, "Failed to send notification %lld", n->id);
        }
    }
}

// Start daemon with maximum power efficiency
int notify_daemon_start(void) {
    if (atomic_load(&g_daemon.running)) return 0;

    if (notify_daemon_init() != 0) return -1;

    // Create timer source on E-core queue
    g_daemon.timer = dispatch_source_create(
        DISPATCH_SOURCE_TYPE_TIMER, 0, 0, g_daemon.queue);

    // Initial timer: immediate first check, then poll_interval
    dispatch_source_set_timer(g_daemon.timer,
        dispatch_time(DISPATCH_TIME_NOW, 0),      // Fire immediately
        g_daemon.poll_interval_ns,                // Then every 60s
        10 * NSEC_PER_SEC                         // 10s leeway (power optimization)
    );

    dispatch_source_set_event_handler(g_daemon.timer, ^{
        @autoreleasepool {
            check_pending_reminders();
        }
    });

    // Handle graceful shutdown
    dispatch_source_set_cancel_handler(g_daemon.timer, ^{
        sqlite3_finalize(g_daemon.check_stmt);
        sqlite3_finalize(g_daemon.update_stmt);
        sqlite3_close(g_daemon.db);
        os_log_info(g_daemon_log, "Daemon stopped cleanly");
    });

    dispatch_resume(g_daemon.timer);
    atomic_store(&g_daemon.running, true);

    os_log_info(g_daemon_log, "Daemon started (E-cores, adaptive polling)");
    return 0;
}

// Memory footprint monitoring
size_t notify_daemon_get_memory_usage(void) {
    struct task_basic_info info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    kern_return_t kr = task_info(mach_task_self(), TASK_BASIC_INFO,
                                  (task_info_t)&info, &count);
    return (kr == KERN_SUCCESS) ? info.resident_size : 0;
}
```

#### Performance Characteristics

| Metric | Target | Implementation |
|--------|--------|----------------|
| Memory (resident) | < 8MB | Static buffers, prepared statements, mmap |
| Memory (virtual) | < 50MB | Lazy allocation, shared SQLite connection |
| CPU usage (idle) | < 0.1% | Timer coalescing, adaptive polling |
| CPU usage (active) | < 1% | E-cores only, no P-core usage |
| Energy Impact | "Low" in Activity Monitor | QOS_CLASS_UTILITY, LowPriorityIO |
| Wake frequency | 12/hour min | 5-minute idle interval |
| Battery impact | < 0.1%/hour | ProcessType=Background |

### 2.4 LaunchAgent Configuration

**File**: `resources/io.convergio.daemon.plist`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>io.convergio.daemon</string>

    <key>ProgramArguments</key>
    <array>
        <string>/usr/local/bin/convergio</string>
        <string>daemon</string>
        <string>run</string>
        <string>--foreground</string>
    </array>

    <!-- Start on login -->
    <key>RunAtLoad</key>
    <true/>

    <!-- Auto-restart on crash -->
    <key>KeepAlive</key>
    <dict>
        <key>SuccessfulExit</key>
        <false/>
        <key>Crashed</key>
        <true/>
    </dict>

    <!-- Restart throttle -->
    <key>ThrottleInterval</key>
    <integer>10</integer>

    <!-- Run as background process -->
    <key>ProcessType</key>
    <string>Background</string>

    <!-- Low I/O priority -->
    <key>LowPriorityIO</key>
    <true/>

    <!-- Nice value (lower priority) -->
    <key>Nice</key>
    <integer>10</integer>

    <!-- Logging -->
    <key>StandardOutPath</key>
    <string>/tmp/convergio-daemon.log</string>

    <key>StandardErrorPath</key>
    <string>/tmp/convergio-daemon.err</string>

    <!-- Environment -->
    <key>EnvironmentVariables</key>
    <dict>
        <key>PATH</key>
        <string>/usr/local/bin:/usr/bin:/bin</string>
    </dict>
</dict>
</plist>
```

### 2.5 Daemon CLI Commands

```bash
# Daemon management
convergio daemon start      # Install LaunchAgent and start
convergio daemon stop       # Stop daemon
convergio daemon restart    # Restart daemon
convergio daemon status     # Show status
convergio daemon logs       # Show recent logs
convergio daemon health     # Full diagnostic
convergio daemon uninstall  # Remove LaunchAgent completely
```

### 2.6 Tasks Checklist

- [ ] Notification API implementation
- [ ] terminal-notifier integration
- [ ] osascript fallback
- [ ] Log-only fallback
- [ ] Daemon main loop with dispatch timer
- [ ] LaunchAgent plist file
- [ ] Daemon CLI commands (start, stop, status, etc.)
- [ ] Health monitoring
- [ ] Error recovery logic (retries, exponential backoff)
- [ ] Installation script for LaunchAgent
- [ ] Unit tests

---

## Phase 3: Generic MCP Client

### Duration: Week 3-5

### 3.1 MCP Client API

**File**: `include/nous/mcp_client.h`

```c
#ifndef NOUS_MCP_CLIENT_H
#define NOUS_MCP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <cjson/cJSON.h>

// Transport types
typedef enum {
    MCP_TRANSPORT_STDIO,
    MCP_TRANSPORT_HTTP,
    MCP_TRANSPORT_SSE
} MCPTransportType;

// Connection status
typedef enum {
    MCP_STATUS_DISCONNECTED,
    MCP_STATUS_CONNECTING,
    MCP_STATUS_CONNECTED,
    MCP_STATUS_ERROR
} MCPConnectionStatus;

// Tool definition
typedef struct {
    char* name;
    char* description;
    cJSON* input_schema;       // JSON Schema for parameters
    bool requires_confirmation;
} MCPTool;

// Resource definition
typedef struct {
    char* uri;
    char* name;
    char* description;
    char* mime_type;
} MCPResource;

// Server definition
typedef struct {
    char* name;
    MCPTransportType transport;
    MCPConnectionStatus status;

    // Transport-specific
    union {
        struct {
            pid_t pid;
            int stdin_fd;
            int stdout_fd;
            int stderr_fd;
        } stdio;
        struct {
            char* url;
            char** headers;
            int header_count;
        } http;
    } transport_data;

    // Capabilities
    MCPTool* tools;
    int tool_count;
    MCPResource* resources;
    int resource_count;

    // Protocol state
    int64_t next_request_id;
    char* protocol_version;

    // Error tracking
    char* last_error;
    int consecutive_errors;
    time_t last_success;
} MCPServer;

// Configuration
typedef struct {
    char* name;
    bool enabled;
    MCPTransportType transport;
    char* command;             // For stdio
    char** args;
    int arg_count;
    char** env;                // KEY=VALUE pairs
    int env_count;
    char* url;                 // For HTTP
    char** headers;
    int header_count;
    int timeout_ms;
    int retry_count;
    int retry_delay_ms;
} MCPServerConfig;

// Callback types
typedef void (*MCPErrorCallback)(const char* server_name, int code, const char* message);
typedef void (*MCPToolResultCallback)(const char* server_name, const char* tool_name, cJSON* result);

// Client lifecycle
int mcp_client_init(void);
void mcp_client_shutdown(void);

// Configuration
int mcp_load_config(const char* config_path);  // NULL = default path
int mcp_save_config(const char* config_path);
MCPServerConfig* mcp_get_server_config(const char* name);
int mcp_add_server_config(const MCPServerConfig* config);
int mcp_remove_server_config(const char* name);

// Server management
int mcp_connect(const char* server_name);
int mcp_disconnect(const char* server_name);
int mcp_reconnect(const char* server_name);
MCPServer* mcp_get_server(const char* server_name);
char** mcp_list_servers(int* count);
char** mcp_list_connected_servers(int* count);

// Tool discovery (automatic on connect)
int mcp_refresh_tools(const char* server_name);
MCPTool* mcp_get_tool(const char* server_name, const char* tool_name);
MCPTool** mcp_list_tools(const char* server_name, int* count);
MCPTool** mcp_list_all_tools(int* count);  // All connected servers

// Tool invocation
cJSON* mcp_call_tool(const char* server_name, const char* tool_name, cJSON* arguments);
cJSON* mcp_call_tool_async(const char* server_name, const char* tool_name,
                           cJSON* arguments, MCPToolResultCallback callback);

// Resource access
MCPResource** mcp_list_resources(const char* server_name, int* count);
cJSON* mcp_read_resource(const char* server_name, const char* uri);

// Error handling
void mcp_set_error_callback(MCPErrorCallback callback);
const char* mcp_get_last_error(const char* server_name);

// Health
typedef struct {
    int total_servers;
    int connected_servers;
    int servers_with_errors;
    struct {
        char* name;
        MCPConnectionStatus status;
        time_t last_success;
        char* last_error;
    }* server_status;
} MCPHealth;

MCPHealth mcp_get_health(void);
void mcp_free_health(MCPHealth* health);

#endif // NOUS_MCP_CLIENT_H
```

### 3.2 JSON-RPC Implementation

**File**: `src/mcp/mcp_jsonrpc.c`

Based on [MCP Specification](https://modelcontextprotocol.io/specification/2025-06-18):

```c
// JSON-RPC 2.0 message format

// Request
typedef struct {
    int64_t id;
    const char* method;
    cJSON* params;
} JSONRPCRequest;

// Response
typedef struct {
    int64_t id;
    cJSON* result;
    struct {
        int code;
        char* message;
        cJSON* data;
    } error;
    bool is_error;
} JSONRPCResponse;

// Serialize request to JSON string (newline-delimited for stdio)
char* jsonrpc_serialize_request(const JSONRPCRequest* req) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(json, "id", req->id);
    cJSON_AddStringToObject(json, "method", req->method);
    if (req->params) {
        cJSON_AddItemToObject(json, "params", cJSON_Duplicate(req->params, true));
    }

    char* str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    // Append newline for stdio transport
    size_t len = strlen(str);
    str = realloc(str, len + 2);
    str[len] = '\n';
    str[len + 1] = '\0';

    return str;
}

// Parse response from JSON string
JSONRPCResponse* jsonrpc_parse_response(const char* json_str) {
    cJSON* json = cJSON_Parse(json_str);
    if (!json) return NULL;

    JSONRPCResponse* resp = calloc(1, sizeof(JSONRPCResponse));

    cJSON* id = cJSON_GetObjectItem(json, "id");
    resp->id = (int64_t)cJSON_GetNumberValue(id);

    cJSON* error = cJSON_GetObjectItem(json, "error");
    if (error) {
        resp->is_error = true;
        resp->error.code = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(error, "code"));
        resp->error.message = strdup(cJSON_GetStringValue(cJSON_GetObjectItem(error, "message")));
        resp->error.data = cJSON_Duplicate(cJSON_GetObjectItem(error, "data"), true);
    } else {
        resp->result = cJSON_Duplicate(cJSON_GetObjectItem(json, "result"), true);
    }

    cJSON_Delete(json);
    return resp;
}
```

### 3.3 stdio Transport

**File**: `src/mcp/mcp_transport_stdio.c`

```c
// Launch MCP server as subprocess and communicate via stdin/stdout

#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <poll.h>

typedef struct {
    pid_t pid;
    int stdin_fd;   // Write to server
    int stdout_fd;  // Read from server
    int stderr_fd;  // For error logging
} StdioConnection;

StdioConnection* stdio_connect(const MCPServerConfig* config) {
    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];

    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
        return NULL;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return NULL;
    }

    if (pid == 0) {
        // Child process
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        // Set environment
        for (int i = 0; i < config->env_count; i++) {
            putenv(config->env[i]);
        }

        // Build argv
        char** argv = malloc((config->arg_count + 2) * sizeof(char*));
        argv[0] = (char*)config->command;
        for (int i = 0; i < config->arg_count; i++) {
            argv[i + 1] = config->args[i];
        }
        argv[config->arg_count + 1] = NULL;

        execvp(config->command, argv);
        _exit(127);  // exec failed
    }

    // Parent process
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    StdioConnection* conn = malloc(sizeof(StdioConnection));
    conn->pid = pid;
    conn->stdin_fd = stdin_pipe[1];
    conn->stdout_fd = stdout_pipe[0];
    conn->stderr_fd = stderr_pipe[0];

    // Set non-blocking
    fcntl(conn->stdout_fd, F_SETFL, O_NONBLOCK);
    fcntl(conn->stderr_fd, F_SETFL, O_NONBLOCK);

    return conn;
}

// Send request and receive response
JSONRPCResponse* stdio_send_request(StdioConnection* conn, const JSONRPCRequest* req, int timeout_ms) {
    char* request_str = jsonrpc_serialize_request(req);

    // Write request
    write(conn->stdin_fd, request_str, strlen(request_str));
    free(request_str);

    // Read response (newline-delimited)
    char buffer[65536];
    size_t buffer_pos = 0;

    struct pollfd pfd = { .fd = conn->stdout_fd, .events = POLLIN };

    while (1) {
        int poll_result = poll(&pfd, 1, timeout_ms);
        if (poll_result <= 0) {
            return NULL;  // Timeout or error
        }

        ssize_t n = read(conn->stdout_fd, buffer + buffer_pos, sizeof(buffer) - buffer_pos - 1);
        if (n <= 0) break;

        buffer_pos += n;
        buffer[buffer_pos] = '\0';

        // Check for newline (message delimiter)
        char* newline = strchr(buffer, '\n');
        if (newline) {
            *newline = '\0';
            return jsonrpc_parse_response(buffer);
        }
    }

    return NULL;
}
```

### 3.4 HTTP Transport

**File**: `src/mcp/mcp_transport_http.c`

```c
// HTTP transport using libcurl

#include <curl/curl.h>

typedef struct {
    char* url;
    struct curl_slist* headers;
    CURL* curl;
} HTTPConnection;

HTTPConnection* http_connect(const MCPServerConfig* config) {
    HTTPConnection* conn = calloc(1, sizeof(HTTPConnection));
    conn->url = strdup(config->url);
    conn->curl = curl_easy_init();

    // Set headers
    for (int i = 0; i < config->header_count; i++) {
        conn->headers = curl_slist_append(conn->headers, config->headers[i]);
    }
    conn->headers = curl_slist_append(conn->headers, "Content-Type: application/json");

    curl_easy_setopt(conn->curl, CURLOPT_HTTPHEADER, conn->headers);
    curl_easy_setopt(conn->curl, CURLOPT_TIMEOUT_MS, config->timeout_ms);

    return conn;
}

// Response buffer callback
static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t realsize = size * nmemb;
    char** response = (char**)userdata;

    *response = realloc(*response, strlen(*response) + realsize + 1);
    strncat(*response, ptr, realsize);

    return realsize;
}

JSONRPCResponse* http_send_request(HTTPConnection* conn, const JSONRPCRequest* req) {
    char* request_str = jsonrpc_serialize_request(req);
    char* response_str = strdup("");

    curl_easy_setopt(conn->curl, CURLOPT_URL, conn->url);
    curl_easy_setopt(conn->curl, CURLOPT_POSTFIELDS, request_str);
    curl_easy_setopt(conn->curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(conn->curl, CURLOPT_WRITEDATA, &response_str);

    CURLcode res = curl_easy_perform(conn->curl);
    free(request_str);

    if (res != CURLE_OK) {
        free(response_str);
        return NULL;
    }

    JSONRPCResponse* resp = jsonrpc_parse_response(response_str);
    free(response_str);

    return resp;
}
```

### 3.5 MCP Configuration File

**Default path**: `~/.convergio/mcp.json`

```json
{
  "version": "1.0",
  "defaults": {
    "timeout_ms": 30000,
    "retry_count": 3,
    "retry_delay_ms": 1000
  },
  "servers": {
    "google-calendar": {
      "enabled": true,
      "transport": "stdio",
      "command": "npx",
      "args": ["-y", "@anthropic/mcp-google-calendar"],
      "env": {
        "GOOGLE_CLIENT_ID": "${GOOGLE_CLIENT_ID}",
        "GOOGLE_CLIENT_SECRET": "${GOOGLE_CLIENT_SECRET}"
      },
      "timeout_ms": 30000
    },
    "outlook": {
      "enabled": false,
      "transport": "http",
      "url": "https://mcp.microsoft.com/outlook",
      "headers": {
        "Authorization": "Bearer ${AZURE_ACCESS_TOKEN}"
      }
    },
    "gmail": {
      "enabled": false,
      "transport": "stdio",
      "command": "npx",
      "args": ["-y", "@anthropic/mcp-gmail"]
    },
    "todoist": {
      "enabled": false,
      "transport": "http",
      "url": "https://ai.todoist.net/mcp",
      "headers": {
        "Authorization": "Bearer ${TODOIST_API_TOKEN}"
      }
    },
    "flights-kiwi": {
      "enabled": false,
      "transport": "stdio",
      "command": "npx",
      "args": ["-y", "@kiwi/mcp-flights"]
    },
    "expedia": {
      "enabled": false,
      "transport": "http",
      "url": "https://mcp.expedia.com/v1",
      "headers": {
        "X-API-Key": "${EXPEDIA_API_KEY}"
      }
    }
  }
}
```

### 3.6 MCP CLI Commands

```bash
# Server management
convergio mcp list                      # List all configured servers
convergio mcp status                    # Show connection status
convergio mcp connect <server>          # Connect to server
convergio mcp disconnect <server>       # Disconnect from server

# Configuration
convergio mcp add <name> <command> [args...]  # Add stdio server
convergio mcp add <name> --http <url>         # Add HTTP server
convergio mcp remove <name>                    # Remove server
convergio mcp enable <name>                    # Enable server
convergio mcp disable <name>                   # Disable server

# Tool discovery
convergio mcp tools                     # List all available tools
convergio mcp tools <server>            # List tools from specific server
convergio mcp describe <server> <tool>  # Show tool details

# Direct tool invocation (advanced)
convergio mcp call <server> <tool> [json_args]
```

### 3.7 Tasks Checklist

- [ ] MCP Client API design
- [ ] JSON-RPC 2.0 serializer/parser
- [ ] stdio transport implementation
- [ ] HTTP transport implementation
- [ ] Configuration file loader (JSON)
- [ ] Environment variable expansion
- [ ] Tool discovery (tools/list method)
- [ ] Tool invocation (tools/call method)
- [ ] Connection management (connect, disconnect, reconnect)
- [ ] Error handling and retries
- [ ] MCP CLI commands
- [ ] Unit tests
- [ ] Integration tests with real MCP servers

---

## Phase 4: Anna Agent

### Duration: Week 5-6

### 4.1 Agent Definition

**File**: `src/agents/definitions/anna-executive-assistant.md`

```markdown
---
name: anna-executive-assistant
description: Personal Executive Assistant managing tasks, calendar, reminders, and productivity through native todo manager and MCP integrations
tools: ["Task", "Read", "Write", "Bash", "WebSearch", "TodoRead", "TodoWrite", "TodoList", "TodoComplete", "TodoSearch", "InboxCapture", "InboxProcess", "MCPCall", "MCPListTools", "NotificationSchedule", "CalendarQuery", "CalendarCreate"]
color: "#E91E63"
tier: "Personal Assistant"
reports_to: "ali-chief-of-staff"
---

You are **Anna**, a highly efficient Personal Executive Assistant in the Convergio ecosystem. You help users manage their personal productivity through task management, calendar coordination, reminders, and proactive assistance.

## Core Capabilities

### 1. Task Management (Native Todo System)
- Create, update, and track tasks with priorities and deadlines
- Process natural language task requests
- Manage recurring tasks and subtasks
- Tag and categorize tasks by context (project, person, etc.)
- Quick capture to inbox for later processing

### 2. Reminder System
- Schedule reminders for tasks and events
- Send native macOS notifications
- Support relative reminders ("remind me 1 hour before")
- Snooze and reschedule reminders

### 3. Calendar Integration (via MCP)
- Query calendar events (if MCP server configured)
- Create calendar events
- Find free time slots
- Coordinate meeting scheduling

### 4. Travel Assistance (via MCP)
- Search for flights (if MCP server configured)
- Search for hotels
- Compare options and prices

## Interaction Style

- **Proactive**: Anticipate needs based on context
- **Concise**: Provide clear, actionable information
- **Organized**: Structure responses with priorities
- **Respectful**: Handle sensitive information carefully

## Example Interactions

**User**: "Remind me to call Mario tomorrow at 10am"
**Anna**: Creates task "Call Mario" with due date tomorrow 10:00 and schedules reminder.

**User**: "What do I have to do today?"
**Anna**: Lists today's tasks and overdue items, prioritized by urgency.

**User**: "Schedule a meeting with the team next Tuesday"
**Anna**: Checks calendar availability via MCP, suggests times, creates event.

**User**: "Find me a flight to Milan next week"
**Anna**: Searches flights via MCP, presents options with prices.

## Integration with Ali

Anna reports to Ali (Chief of Staff) for:
- Daily/weekly productivity summaries
- Escalation of blocked tasks
- Coordination with other agents when needed

Ali delegates personal productivity tasks to Anna when users ask about:
- Task management
- Calendar coordination
- Reminders and deadlines
- Travel planning
```

### 4.2 Custom Tools for Anna

**File**: `src/tools/todo_tools.c`

```c
// Tool: TodoRead - Read task details
// Tool: TodoWrite - Create or update task
// Tool: TodoList - List tasks with filters
// Tool: TodoComplete - Mark task as done
// Tool: TodoSearch - Full-text search tasks
// Tool: InboxCapture - Quick capture to inbox
// Tool: InboxProcess - Process inbox item to task
// Tool: NotificationSchedule - Schedule a reminder
// Tool: MCPCall - Call any MCP tool
// Tool: MCPListTools - List available MCP tools
```

### 4.3 Natural Language Processing

Anna uses the configured LLM (any provider) to:

1. **Intent Recognition**
   - Task creation: "remind me to...", "add task...", "I need to..."
   - Task query: "what do I have to do...", "show my tasks..."
   - Calendar: "schedule a meeting...", "what's on my calendar..."
   - Reminder: "remind me...", "set an alarm..."
   - Travel: "find a flight...", "book a hotel..."

2. **Entity Extraction**
   - Dates and times (natural language)
   - People names
   - Project/context names
   - Priorities

3. **Response Generation**
   - Confirmation messages
   - Task summaries
   - Suggestions and follow-ups

### 4.4 Ali Integration Protocol

Update Ali's agent definition to include Anna in the ecosystem:

```markdown
### Personal Assistant Tier (1 Agent)
- **Anna** (anna-executive-assistant): Personal productivity, task management, calendar coordination, reminders

### RACI Matrix Update

| Activity | Anna | Ali | Other Agents |
|----------|------|-----|--------------|
| Personal task management | **R** | A | I |
| Calendar coordination | **R** | A | C (relevant agents) |
| Meeting scheduling | **R** | A | C (attendees) |
| Reminder management | **R** | I | - |
| Travel booking research | **R** | A | C (Amy for budget) |
| Daily/weekly reviews | **R** | A | I |
```

### 4.5 Tasks Checklist

- [ ] Anna agent definition markdown
- [ ] Custom tools implementation (TodoRead, TodoWrite, etc.)
- [ ] Tool registration with agent system
- [ ] Natural language intent recognition prompts
- [ ] Entity extraction for dates, people, projects
- [ ] Integration tests with Anna agent
- [ ] Ali integration (RACI update)
- [ ] Handoff protocols between Anna and Ali

---

## Phase 5: Documentation & Polish

### Duration: Week 6-7

### 5.1 User Documentation

**Files to update**:

- [ ] `README.md` - Add Anna feature section
- [ ] `docs/ANNA.md` - Comprehensive Anna documentation (NEW)
- [ ] `docs/TODO.md` - Todo manager documentation (NEW)
- [ ] `docs/MCP.md` - MCP client documentation (NEW)
- [ ] `docs/DAEMON.md` - Daemon documentation (NEW)
- [ ] `docs/agents/anna-executive-assistant.md` - Agent reference

### 5.2 README.md Updates

Add sections:

```markdown
## Personal Assistant: Anna

Anna is your AI-powered executive assistant that helps manage:
- **Tasks & Reminders**: Native todo list with notifications
- **Calendar**: Integration with Google Calendar, Outlook (via MCP)
- **Travel**: Flight and hotel search (via MCP)

### Quick Start

\`\`\`bash
# Add a task
convergio todo add "Prepare presentation" --due friday --priority 1

# Natural language
convergio ask anna "remind me to call the accountant monday morning"

# List today's tasks
convergio todo list --today
\`\`\`

### Notification Daemon

\`\`\`bash
# Start the reminder daemon
convergio daemon start

# Check status
convergio daemon status
\`\`\`

### MCP Integrations

Configure external services in `~/.convergio/mcp.json`:

\`\`\`bash
# List available MCP servers
convergio mcp list

# Enable Google Calendar
convergio mcp enable google-calendar
\`\`\`
```

### 5.3 Setup Wizard

**File**: `src/core/commands/setup_anna.c`

Interactive wizard for first-time setup:

```bash
convergio setup anna
```

Steps:
1. Check/install terminal-notifier (optional)
2. Start notification daemon
3. Configure MCP servers (optional)
4. Test notification
5. Create first task

### 5.4 Error Messages and Troubleshooting

Create helpful error messages for common issues:

| Issue | Error Message | Solution |
|-------|--------------|----------|
| Daemon not running | "Reminder daemon is not running. Start it with `convergio daemon start`" | Auto-suggest fix |
| MCP auth failed | "Authentication failed for [server]. Run `convergio mcp setup [server]` to re-authenticate" | Guide to re-auth |
| Notification failed | "Could not send notification. Check `convergio daemon health` for details" | Health check |
| Database locked | "Database is busy. Retrying..." | Auto-retry |

### 5.5 Tasks Checklist

- [ ] README.md updates
- [ ] docs/ANNA.md - Full documentation
- [ ] docs/TODO.md - Todo system docs
- [ ] docs/MCP.md - MCP client docs
- [ ] docs/DAEMON.md - Daemon docs
- [ ] Setup wizard (`convergio setup anna`)
- [ ] Error messages and troubleshooting guide
- [ ] Help command updates (`convergio help anna`, `convergio help todo`, etc.)
- [ ] Man page updates (if applicable)
- [ ] CHANGELOG.md update

---

## Testing Strategy

### Unit Tests

| Component | Test File | Coverage Target |
|-----------|-----------|-----------------|
| Todo CRUD | `tests/test_todo.c` | 90% |
| Date parsing | `tests/test_dateparse.c` | 95% |
| Recurrence | `tests/test_recurrence.c` | 90% |
| Notification | `tests/test_notify.c` | 85% |
| JSON-RPC | `tests/test_jsonrpc.c` | 95% |
| MCP Client | `tests/test_mcp.c` | 80% |

### Integration Tests

| Test | Description |
|------|-------------|
| Todo CLI | Full CLI workflow (add, list, done, delete) |
| Daemon lifecycle | Start, health check, stop |
| MCP connection | Connect to test MCP server |
| Anna agent | Natural language → task creation |

### End-to-End Tests

| Test | Description |
|------|-------------|
| Full reminder flow | Create task → wait → receive notification |
| Calendar integration | Query/create events via MCP |
| Anna conversation | Multi-turn conversation with task management |

---

## Risk Mitigation

### Technical Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| No C MCP SDK | High | Implement from spec (JSON-RPC is simple) |
| macOS notification issues on Sequoia | Medium | Multiple fallback methods |
| OAuth token management complexity | Medium | Use system keychain |
| Daemon stability | Medium | launchd auto-restart + health monitoring |

### Operational Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| User doesn't start daemon | Medium | Prompt on first todo command |
| MCP server unavailable | Low | Graceful degradation, offline mode |
| Database corruption | Low | Regular backups, integrity checks |

---

## Success Metrics

### Performance

- [ ] Todo operations < 50ms latency
- [ ] Notification delivery < 5s from scheduled time
- [ ] MCP tool call < 2s response time
- [ ] Daemon memory usage < 20MB

### Reliability

- [ ] Notification delivery rate > 99%
- [ ] Daemon uptime > 99.9%
- [ ] MCP connection success rate > 95%
- [ ] Zero data loss in todo system

### User Experience

- [ ] Natural language recognition accuracy > 90%
- [ ] Setup wizard completion rate > 80%
- [ ] User satisfaction score > 4.5/5

---

## Appendix

### A. MCP Protocol Reference

- [MCP Specification 2025-06-18](https://modelcontextprotocol.io/specification/2025-06-18)
- [MCP November 2025 Release Notes](http://blog.modelcontextprotocol.io/posts/2025-11-25-first-mcp-anniversary/)
- [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification)

### B. macOS Notification Reference

- [terminal-notifier GitHub](https://github.com/julienXX/terminal-notifier)
- [Apple launchd Documentation](https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPSystemStartup/Chapters/CreatingLaunchdJobs.html)

### C. Market Research

- [AI Assistant Market Report](https://www.marketsandmarkets.com/Market-Reports/ai-assistant-market-40111511.html)
- [AI Productivity Tools Market](https://www.grandviewresearch.com/industry-analysis/ai-productivity-tools-market-report)
