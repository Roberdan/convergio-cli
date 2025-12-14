# ADR-009: Anna Executive Assistant Architecture

**Status**: Accepted (Phase 1 Implemented)
**Date**: 2025-12-14
**Author**: Roberto with AI Team
**Issue**: [#36](https://github.com/Roberdan/convergio-cli/issues/36)

---

## Context

Convergio CLI needs a personal productivity assistant that can:
1. Manage tasks and reminders locally
2. Integrate with external calendar/email services
3. Send native notifications for deadlines
4. Work with any LLM provider (online or local)

The market for AI executive assistants is growing rapidly ($3.35B → $21B by 2030, CAGR 44.5%), and there's a gap for privacy-first, local-first solutions.

## Decision

We will implement **Anna**, an Executive Assistant agent with three core components:

### 1. Native Todo Manager (IMPLEMENTED)

**Rationale**: Local-first, no external dependencies, works offline.

- SQLite persistence (extends existing `convergio.db`)
- Full CRUD operations for tasks
- Recurrence support (iCal RRULE format)
- Full-text search (FTS5)
- Quick capture inbox
- Natural language date parsing (English + Italian)

### 2. Generic MCP Client (PLANNED)

**Rationale**: Flexibility over hardcoded integrations. Users choose their services.

- JSON-RPC 2.0 over stdio and HTTP transports
- Configuration file (`~/.convergio/mcp.json`)
- Auto tool discovery
- No official C SDK → implement from [MCP Spec 2025-06-18](https://modelcontextprotocol.io/specification/2025-06-18)

### 3. Notification Daemon (PLANNED)

**Rationale**: Background service for timely reminders without keeping CLI running.

- launchd-managed daemon (auto-start, auto-restart)
- Multiple notification backends (terminal-notifier, osascript, log)
- Health monitoring and error recovery

---

## Architecture

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              CONVERGIO CLI                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │                           USER INTERFACE                                │ │
│  │                                                                         │ │
│  │   CLI Commands                    Natural Language (via any LLM)       │ │
│  │   /todo add "Review PR"           @anna "remind me to call Mario"      │ │
│  │   /remind "Meeting" tomorrow      @anna "what do I have to do today?"  │ │
│  │   /reminders week                                                       │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│                                     │                                        │
│                     ┌───────────────┴───────────────┐                       │
│                     ▼                               ▼                       │
│  ┌──────────────────────────────┐   ┌──────────────────────────────┐       │
│  │       ANNA AGENT             │   │      DIRECT CLI HANDLER      │       │
│  │   (requires LLM)             │   │    (no LLM required)         │       │
│  │                              │   │                              │       │
│  │  • Intent recognition        │   │  • Argument parsing          │       │
│  │  • Context awareness         │   │  • Direct API calls          │       │
│  │  • Multi-step planning       │   │  • Structured output         │       │
│  │  • NL reminder management    │   │                              │       │
│  └──────────────────────────────┘   └──────────────────────────────┘       │
│                     │                               │                       │
│                     └───────────────┬───────────────┘                       │
│                                     ▼                                        │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │                          CORE SERVICES                                  │ │
│  ├──────────────────┬───────────────────┬─────────────────────────────────┤ │
│  │                  │                   │                                  │ │
│  │  TODO MANAGER    │  NOTIFICATION     │  MCP CLIENT                     │ │
│  │  [IMPLEMENTED]   │  DAEMON           │                                  │ │
│  │                  │  [PLANNED]        │  [PLANNED]                      │ │
│  │  • CRUD ops      │  • Scheduler      │  • JSON-RPC 2.0                 │ │
│  │  • Recurrence    │  • Dispatcher     │  • stdio/HTTP transport         │ │
│  │  • Tags/Context  │  • Fallbacks      │  • Tool discovery               │ │
│  │  • FTS5 Search   │  • Health mon     │  • Multi-server                 │ │
│  │  • NL Dates      │                   │                                  │ │
│  │                  │                   │                                  │ │
│  └────────┬─────────┴─────────┬─────────┴───────────────┬─────────────────┘ │
│           │                   │                         │                    │
│           ▼                   ▼                         ▼                    │
│  ┌──────────────┐   ┌───────────────────┐   ┌────────────────────────┐     │
│  │   SQLite     │   │   macOS Native    │   │  External MCP Servers  │     │
│  │ convergio.db │   │   Notifications   │   │  (user configured)     │     │
│  └──────────────┘   └───────────────────┘   └────────────────────────┘     │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### File Structure

```
include/nous/
├── todo.h                    # Todo Manager public API [IMPLEMENTED]
├── notify.h                  # Notification API [PLANNED]
└── mcp_client.h              # MCP Client API [PLANNED]

src/
├── todo/
│   └── todo.c                # Todo Manager implementation [IMPLEMENTED]
├── notifications/
│   ├── notify.c              # Notification dispatcher [PLANNED]
│   ├── notify_macos.m        # macOS-specific implementation [PLANNED]
│   └── daemon.c              # Daemon main loop [PLANNED]
├── mcp/
│   ├── mcp_client.c          # Client lifecycle [PLANNED]
│   ├── mcp_jsonrpc.c         # JSON-RPC 2.0 parser [PLANNED]
│   ├── mcp_transport_stdio.c # stdio transport [PLANNED]
│   └── mcp_transport_http.c  # HTTP transport [PLANNED]
└── agents/definitions/
    └── anna-executive-assistant.md  # Agent definition [PLANNED]

docs/help/
├── todo.md                   # /todo command documentation [IMPLEMENTED]
└── remind.md                 # /remind command documentation [IMPLEMENTED]
```

---

## Implementation Details

### Phase 1: Todo Manager (IMPLEMENTED)

#### Database Schema

```sql
-- Tasks table with full metadata
CREATE TABLE IF NOT EXISTS tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    description TEXT,
    priority INTEGER DEFAULT 2 CHECK (priority BETWEEN 1 AND 3),
    status INTEGER DEFAULT 0,  -- 0=pending, 1=in_progress, 2=completed, 3=cancelled
    due_date INTEGER,          -- Unix timestamp
    reminder_at INTEGER,       -- Unix timestamp
    recurrence INTEGER DEFAULT 0,
    recurrence_rule TEXT,      -- iCal RRULE format
    tags TEXT,                 -- JSON array
    context TEXT,              -- project, person, etc.
    parent_id INTEGER REFERENCES tasks(id) ON DELETE CASCADE,
    source INTEGER DEFAULT 0,  -- 0=user, 1=agent, 2=mcp_sync
    external_id TEXT,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER DEFAULT (strftime('%s', 'now')),
    completed_at INTEGER
);

-- Notification queue for scheduled reminders
CREATE TABLE IF NOT EXISTS notification_queue (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id INTEGER NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    scheduled_at INTEGER NOT NULL,
    method INTEGER DEFAULT 0,  -- 0=native, 1=terminal, 2=sound, 3=log
    status INTEGER DEFAULT 0,  -- 0=pending, 1=sent, 2=failed, 3=acknowledged
    retry_count INTEGER DEFAULT 0,
    max_retries INTEGER DEFAULT 3,
    last_error TEXT,
    sent_at INTEGER,
    acknowledged_at INTEGER
);

-- Quick capture inbox
CREATE TABLE IF NOT EXISTS inbox (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    content TEXT NOT NULL,
    captured_at INTEGER DEFAULT (strftime('%s', 'now')),
    processed INTEGER DEFAULT 0,
    processed_task_id INTEGER REFERENCES tasks(id),
    source TEXT DEFAULT 'cli'
);

-- Full-text search for tasks
CREATE VIRTUAL TABLE IF NOT EXISTS tasks_fts USING fts5(
    title, description, tags, context,
    content='tasks', content_rowid='id'
);

-- Performance indexes
CREATE INDEX IF NOT EXISTS idx_tasks_due ON tasks(due_date) WHERE status = 0;
CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status);
CREATE INDEX IF NOT EXISTS idx_tasks_context ON tasks(context);
CREATE INDEX IF NOT EXISTS idx_notif_scheduled ON notification_queue(scheduled_at) WHERE status = 0;
```

#### Todo API (`include/nous/todo.h`)

```c
// Core types
typedef enum { TODO_PRIORITY_URGENT=1, TODO_PRIORITY_NORMAL=2, TODO_PRIORITY_LOW=3 } TodoPriority;
typedef enum { TODO_STATUS_PENDING=0, TODO_STATUS_IN_PROGRESS=1, TODO_STATUS_COMPLETED=2, TODO_STATUS_CANCELLED=3 } TodoStatus;
typedef enum { TODO_RECURRENCE_NONE=0, TODO_RECURRENCE_DAILY=1, TODO_RECURRENCE_WEEKLY=2, TODO_RECURRENCE_MONTHLY=3, TODO_RECURRENCE_YEARLY=4, TODO_RECURRENCE_CUSTOM=5 } TodoRecurrence;

// Task structure
typedef struct {
    int64_t id;
    char* title;
    char* description;
    TodoPriority priority;
    TodoStatus status;
    time_t due_date;
    time_t reminder_at;
    TodoRecurrence recurrence;
    char* recurrence_rule;
    char* tags;
    char* context;
    int64_t parent_id;
    TodoSource source;
    char* external_id;
    time_t created_at;
    time_t updated_at;
    time_t completed_at;
} TodoTask;

// CRUD Operations
int64_t todo_create(const TodoCreateOptions* options);
TodoTask* todo_get(int64_t id);
int todo_update(int64_t id, const TodoCreateOptions* options);
int todo_delete(int64_t id);

// Status changes
int todo_complete(int64_t id);
int todo_uncomplete(int64_t id);
int todo_start(int64_t id);
int todo_cancel(int64_t id);

// Listing & Search
TodoTask** todo_list(const TodoFilter* filter, int* count);
TodoTask** todo_list_today(int* count);
TodoTask** todo_list_overdue(int* count);
TodoTask** todo_list_upcoming(int days, int* count);
TodoTask** todo_search(const char* query, int* count);

// Inbox
int64_t inbox_capture(const char* content, const char* source);
TodoInboxItem** inbox_list_unprocessed(int* count);
int inbox_process(int64_t inbox_id, int64_t task_id);

// Statistics
TodoStats todo_get_stats(void);
```

#### Natural Language Date Parsing

The `todo_parse_date()` function supports:

| Format | Example | Result |
|--------|---------|--------|
| **Relative Days** | "today", "tomorrow", "yesterday" | Today/tomorrow/yesterday at current time |
| **Italian Days** | "oggi", "domani", "ieri" | Same as English |
| **Time of Day** | "tonight", "tomorrow morning" | 8pm today, 9am tomorrow |
| **Italian ToD** | "stasera", "domani mattina" | Same as English |
| **Weekdays** | "next monday", "thursday" | Next occurrence of that day |
| **Italian Weekdays** | "lunedi prossimo", "giovedi" | Same as English |
| **Relative Time** | "in 2 hours", "in 30 minutes" | Current time + duration |
| **Italian Relative** | "tra 2 ore", "tra 30 minuti" | Same as English |
| **Complex Relative** | "thursday in two weeks" | Thursday, 2 weeks from now |
| **Italian Complex** | "giovedi tra due settimane" | Same as English |
| **Specific Time** | "at 3pm", "at 15:00" | Today at that time (or tomorrow if past) |
| **Italian Time** | "alle 14", "alle 15:30" | Same as English |
| **Month + Day** | "dec 25", "december 15" | That date this/next year |
| **ISO Format** | "2025-12-25", "2025-12-25 14:30" | Exact date/time |

#### CLI Commands

**`/todo` - Task Manager**

```bash
# Add tasks
/todo add "Review PR" --due tomorrow --priority 1 --context work
/todo add "Call dentist" --due "next monday at 10am" --remind 1h

# List tasks
/todo list              # Pending tasks
/todo list today        # Today + overdue
/todo list overdue      # Only overdue
/todo list upcoming 14  # Next 14 days
/todo list all          # Including completed

# Manage tasks
/todo done 5            # Complete task #5
/todo start 3           # Mark as in progress
/todo delete 7          # Delete task

# Inbox
/todo inbox "Quick thought"  # Capture to inbox
/todo inbox                  # List inbox items

# Search & Stats
/todo search meeting    # Full-text search
/todo stats             # Show statistics
```

**`/remind` - Quick Reminders**

```bash
# Flexible syntax - message and time in any order
/remind "Call mom" tomorrow morning
/remind tonight "Buy groceries"
/remind "Team meeting" next tuesday at 10am
/remind in 2 hours "Take a break"

# With notes
/remind "Review PR" tomorrow --note "Check auth changes in #123"
```

**`/reminders` - View Reminders**

```bash
/reminders          # Today's reminders
/reminders week     # Next 7 days
/reminders all      # All scheduled
```

---

### Phase 2: Notification System (PLANNED)

#### Notification API

```c
typedef enum {
    NOTIFY_METHOD_NATIVE,      // terminal-notifier or osascript
    NOTIFY_METHOD_TERMINAL,    // Print to terminal if active
    NOTIFY_METHOD_SOUND,       // Sound only
    NOTIFY_METHOD_LOG          // Log file only (last resort)
} NotifyMethod;

// Send notification with automatic fallback chain
NotifyResult notify_send(const NotifyOptions* options);

// Daemon management
int notify_daemon_start(void);
int notify_daemon_stop(void);
bool notify_daemon_is_running(void);
NotifyHealth notify_get_health(void);
```

#### Daemon Design (Apple Silicon Optimized)

- **E-cores only**: QOS_CLASS_UTILITY for all daemon work
- **Adaptive polling**: 60s normal → 300s when no pending → 30s when many pending
- **Memory target**: < 8MB resident
- **Timer coalescing**: 10s leeway for power efficiency
- **Prepared statements**: Reuse SQLite queries

#### LaunchAgent

```xml
<plist version="1.0">
<dict>
    <key>Label</key><string>io.convergio.daemon</string>
    <key>RunAtLoad</key><true/>
    <key>ProcessType</key><string>Background</string>
    <key>LowPriorityIO</key><true/>
    <key>Nice</key><integer>10</integer>
</dict>
</plist>
```

---

### Phase 3: MCP Client (PLANNED)

#### Transport Support

| Transport | Use Case | Implementation |
|-----------|----------|----------------|
| **stdio** | Local MCP servers (npx, python) | fork + pipe |
| **HTTP** | Remote MCP servers | libcurl |
| **SSE** | Streaming responses | libcurl + chunked |

#### Configuration (`~/.convergio/mcp.json`)

```json
{
  "servers": {
    "google-calendar": {
      "enabled": true,
      "transport": "stdio",
      "command": "npx",
      "args": ["-y", "@anthropic/mcp-google-calendar"],
      "env": { "GOOGLE_CLIENT_ID": "${GOOGLE_CLIENT_ID}" }
    },
    "todoist": {
      "enabled": false,
      "transport": "http",
      "url": "https://ai.todoist.net/mcp",
      "headers": { "Authorization": "Bearer ${TODOIST_API_TOKEN}" }
    }
  }
}
```

---

### Phase 4: Anna Agent (PLANNED)

#### Agent Definition

```yaml
name: anna-executive-assistant
description: Personal Executive Assistant for tasks, calendar, and reminders
tools:
  - TodoRead, TodoWrite, TodoList, TodoComplete, TodoSearch
  - InboxCapture, InboxProcess
  - NotificationSchedule
  - MCPCall, MCPListTools
  - CalendarQuery, CalendarCreate (via MCP)
tier: Personal Assistant
reports_to: ali-chief-of-staff
```

#### Natural Language Capabilities

| Intent | Example | Action |
|--------|---------|--------|
| Create reminder | "remind me to call Mario tomorrow" | Create task + schedule notification |
| Query tasks | "what do I have to do today?" | List today's tasks |
| Complete task | "the thing about the report is done" | Find matching task, mark complete |
| Cancel reminder | "I don't need the dentist reminder anymore" | Find + delete with confirmation |
| Reschedule | "move the meeting reminder to Thursday" | Update task due date |
| Context capture | "remind me to finish this document" | Create task with file reference from chat |

---

## Alternatives Considered

### Alternative 1: Use External Todo Service (Todoist, etc.)

**Rejected because**:
- Requires internet connection
- Privacy concerns with sensitive task data
- Dependency on third-party service availability
- MCP integration can still provide this as an option

### Alternative 2: Hardcoded Calendar Integrations

**Rejected because**:
- Maintenance burden for multiple APIs
- No flexibility for user preferences
- MCP ecosystem already has mature servers

### Alternative 3: In-Process Notifications (no daemon)

**Rejected because**:
- Requires CLI to be running
- No reminders when user not at terminal
- Poor UX for time-sensitive notifications

---

## Model Compatibility

Critical requirement: System must work with ALL providers.

| Component | LLM Required | Online Required |
|-----------|--------------|-----------------|
| Todo CRUD | No | No |
| Todo CLI | No | No |
| Notifications | No | No |
| MCP Client | No | Depends on server |
| Anna NL Interface | Yes (any provider) | Depends on provider |

The NL layer uses existing provider router, supporting:
- Claude, OpenAI, Gemini, OpenRouter (online)
- Ollama, MLX (local)

---

## Security Considerations

1. **OAuth Tokens**: Stored in macOS Keychain, not config files
2. **MCP Trust**: User must explicitly enable each server
3. **Data Privacy**: All todo data local by default
4. **Daemon Permissions**: Runs as user, not root
5. **Notification Content**: Sensitive data redacted by default

---

## Performance Targets

| Metric | Target | Status |
|--------|--------|--------|
| Todo operation latency | < 50ms | Achieved |
| FTS5 search | < 100ms | Achieved |
| Date parsing | < 1ms | Achieved |
| Notification delivery | < 5s | Pending |
| MCP tool call | < 2s | Pending |
| Daemon memory | < 8MB | Pending |
| Daemon CPU | E-cores only | Pending |

---

## Implementation Status

| Phase | Status | Completion |
|-------|--------|------------|
| 1: Todo Manager | **IMPLEMENTED** | 95% |
| 2: Notifications | Not Started | 0% |
| 3: MCP Client | Not Started | 0% |
| 4: Anna Agent | Not Started | 0% |
| 5: Documentation | In Progress | 40% |

### Phase 1 Deliverables

- [x] Database schema migration (`persistence.c`)
- [x] Todo API (`include/nous/todo.h`, `src/todo/todo.c`)
- [x] CLI commands (`/todo`, `/remind`, `/reminders`)
- [x] Natural language date parsing (EN + IT)
- [x] Enhanced date parsing (time of day, complex relative)
- [x] Help documentation (`docs/help/todo.md`, `docs/help/remind.md`)
- [ ] Unit tests

---

## Consequences

### Positive

- Privacy-first: All core data local
- Flexible: Users choose MCP integrations
- Offline-capable: Todo + notifications work without internet
- Future-proof: New MCP servers work automatically
- Model-agnostic: Works with any LLM provider
- Bilingual: English + Italian date parsing

### Negative

- More code to maintain (MCP client in C)
- macOS-specific daemon (launchd)
- Notification reliability depends on fallback chain

### Neutral

- Users need to configure MCP servers
- Daemon needs to be running for reminders

---

## Future Enhancements

Ideas for future iterations:

1. **Smart Reminder Defaults**: Auto-set reminders based on priority/context
2. **Context Awareness**: Link reminders to files/PRs mentioned in chat
3. **Daily Briefing**: `/anna morning` for daily summary
4. **Pattern Learning**: Suggest recurring tasks from behavior
5. **Cross-MCP Intelligence**: Combine calendar + flights for travel reminders
6. **Voice Notes**: Audio capture with transcription

---

## References

- [MCP Specification 2025-06-18](https://modelcontextprotocol.io/specification/2025-06-18)
- [MCP November 2025 Release](http://blog.modelcontextprotocol.io/posts/2025-11-25-first-mcp-anniversary/)
- [Apple launchd Documentation](https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPSystemStartup/Chapters/CreatingLaunchdJobs.html)
- [AI Assistant Market Report](https://www.marketsandmarkets.com/Market-Reports/ai-assistant-market-40111511.html)
- [SQLite FTS5](https://www.sqlite.org/fts5.html)
