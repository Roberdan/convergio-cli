/**
 * CONVERGIO TODO MANAGER
 *
 * Native task management with:
 * - SQLite persistence
 * - Recurrence support (iCal RRULE)
 * - Full-text search
 * - Reminder scheduling
 *
 * Part of Anna Executive Assistant feature.
 * See: annaexecassistantPlan.md, ADR-009
 */

#ifndef NOUS_TODO_H
#define NOUS_TODO_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ENUMS
// ============================================================================

// Priority levels
typedef enum {
    TODO_PRIORITY_URGENT = 1,
    TODO_PRIORITY_NORMAL = 2,
    TODO_PRIORITY_LOW = 3
} TodoPriority;

// Task status
typedef enum {
    TODO_STATUS_PENDING = 0,
    TODO_STATUS_IN_PROGRESS = 1,
    TODO_STATUS_COMPLETED = 2,
    TODO_STATUS_CANCELLED = 3
} TodoStatus;

// Recurrence types
typedef enum {
    TODO_RECURRENCE_NONE = 0,
    TODO_RECURRENCE_DAILY = 1,
    TODO_RECURRENCE_WEEKLY = 2,
    TODO_RECURRENCE_MONTHLY = 3,
    TODO_RECURRENCE_YEARLY = 4,
    TODO_RECURRENCE_CUSTOM = 5  // Uses recurrence_rule (RRULE)
} TodoRecurrence;

// Task source
typedef enum {
    TODO_SOURCE_USER = 0,
    TODO_SOURCE_AGENT = 1,
    TODO_SOURCE_MCP_SYNC = 2
} TodoSource;

// ============================================================================
// STRUCTURES
// ============================================================================

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
    char* tags;               // JSON array string
    char* context;            // project, person, etc.
    int64_t parent_id;        // 0 if no parent (for subtasks)
    TodoSource source;
    char* external_id;        // for external sync
    time_t created_at;
    time_t updated_at;
    time_t completed_at;      // 0 if not completed
} TodoTask;

// Filter options for listing
typedef struct {
    TodoStatus* statuses;     // NULL = all statuses
    int status_count;
    TodoPriority* priorities; // NULL = all priorities
    int priority_count;
    time_t due_from;          // 0 = no filter
    time_t due_to;            // 0 = no filter
    const char* context;      // NULL = all contexts
    const char* tag;          // NULL = all tags
    const char* search_query; // FTS query, NULL = no search
    bool include_completed;
    bool include_cancelled;
    int limit;                // 0 = no limit
    int offset;               // For pagination
} TodoFilter;

// Create/Update options
typedef struct {
    const char* title;        // Required
    const char* description;  // Optional
    TodoPriority priority;    // Default: NORMAL
    time_t due_date;          // 0 = no due date
    time_t reminder_at;       // 0 = no reminder
    TodoRecurrence recurrence;
    const char* recurrence_rule;
    const char* tags;         // JSON array string
    const char* context;
    int64_t parent_id;        // 0 = no parent
    TodoSource source;
    const char* external_id;
} TodoCreateOptions;

// Statistics
typedef struct {
    int total_pending;
    int total_in_progress;
    int total_completed_today;
    int total_completed_week;
    int total_overdue;
    int inbox_unprocessed;
} TodoStats;

// Inbox item
typedef struct {
    int64_t id;
    char* content;
    time_t captured_at;
    bool processed;
    int64_t processed_task_id;
    char* source;
} TodoInboxItem;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize the todo subsystem.
 * Must be called after persistence_init().
 * @return 0 on success, -1 on error
 */
int todo_init(void);

/**
 * Shutdown the todo subsystem.
 * Releases prepared statements and resources.
 */
void todo_shutdown(void);

// ============================================================================
// TASK CRUD
// ============================================================================

/**
 * Create a new task.
 * @param options Task creation options (title is required)
 * @return Task ID on success, -1 on error
 */
int64_t todo_create(const TodoCreateOptions* options);

/**
 * Get a task by ID.
 * @param id Task ID
 * @return Task struct (caller must free with todo_free_task), NULL if not found
 */
TodoTask* todo_get(int64_t id);

/**
 * Update an existing task.
 * Only non-NULL/non-zero fields in options are updated.
 * @param id Task ID
 * @param options Fields to update
 * @return 0 on success, -1 on error
 */
int todo_update(int64_t id, const TodoCreateOptions* options);

/**
 * Delete a task and its subtasks.
 * @param id Task ID
 * @return 0 on success, -1 on error
 */
int todo_delete(int64_t id);

// ============================================================================
// STATUS CHANGES
// ============================================================================

/**
 * Mark a task as completed.
 * Sets completed_at timestamp.
 * If task has recurrence, creates next occurrence.
 * @param id Task ID
 * @return 0 on success, -1 on error
 */
int todo_complete(int64_t id);

/**
 * Mark a completed task as pending again.
 * @param id Task ID
 * @return 0 on success, -1 on error
 */
int todo_uncomplete(int64_t id);

/**
 * Mark a task as in progress.
 * @param id Task ID
 * @return 0 on success, -1 on error
 */
int todo_start(int64_t id);

/**
 * Cancel a task.
 * @param id Task ID
 * @return 0 on success, -1 on error
 */
int todo_cancel(int64_t id);

// ============================================================================
// LISTING & SEARCH
// ============================================================================

/**
 * List tasks with optional filtering.
 * @param filter Filter options (NULL for defaults)
 * @param count Output: number of tasks returned
 * @return Array of tasks (caller must free with todo_free_tasks), NULL on error
 */
TodoTask** todo_list(const TodoFilter* filter, int* count);

/**
 * List today's tasks and overdue items.
 * @param count Output: number of tasks
 * @return Array of tasks
 */
TodoTask** todo_list_today(int* count);

/**
 * List overdue tasks only.
 * @param count Output: number of tasks
 * @return Array of tasks
 */
TodoTask** todo_list_overdue(int* count);

/**
 * List tasks due within N days.
 * @param days Number of days to look ahead
 * @param count Output: number of tasks
 * @return Array of tasks
 */
TodoTask** todo_list_upcoming(int days, int* count);

/**
 * Full-text search across tasks.
 * @param query Search query
 * @param count Output: number of results
 * @return Array of matching tasks
 */
TodoTask** todo_search(const char* query, int* count);

/**
 * List subtasks of a parent task.
 * @param parent_id Parent task ID
 * @param count Output: number of subtasks
 * @return Array of subtasks
 */
TodoTask** todo_list_subtasks(int64_t parent_id, int* count);

// ============================================================================
// INBOX (Quick Capture)
// ============================================================================

/**
 * Capture content to inbox for later processing.
 * @param content Raw text to capture
 * @param source Source identifier (cli, agent, voice)
 * @return Inbox item ID, -1 on error
 */
int64_t inbox_capture(const char* content, const char* source);

/**
 * List unprocessed inbox items.
 * @param count Output: number of items
 * @return Array of inbox items (caller must free)
 */
TodoInboxItem** inbox_list_unprocessed(int* count);

/**
 * Mark inbox item as processed and link to task.
 * @param inbox_id Inbox item ID
 * @param task_id Created task ID (0 if discarded)
 * @return 0 on success, -1 on error
 */
int inbox_process(int64_t inbox_id, int64_t task_id);

/**
 * Delete an inbox item.
 * @param inbox_id Inbox item ID
 * @return 0 on success, -1 on error
 */
int inbox_delete(int64_t inbox_id);

// ============================================================================
// MAINTENANCE
// ============================================================================

/**
 * Archive completed tasks older than specified days.
 * Moves to archive table (preserves data).
 * @param days_old Archive tasks completed more than N days ago
 * @return Number of tasks archived, -1 on error
 */
int todo_archive_completed(int days_old);

/**
 * Permanently delete cancelled tasks older than specified days.
 * @param days_old Delete tasks cancelled more than N days ago
 * @return Number of tasks deleted, -1 on error
 */
int todo_delete_cancelled(int days_old);

/**
 * Get todo statistics.
 * @return Statistics struct
 */
TodoStats todo_get_stats(void);

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

/**
 * Free a single task.
 */
void todo_free_task(TodoTask* task);

/**
 * Free an array of tasks.
 */
void todo_free_tasks(TodoTask** tasks, int count);

/**
 * Free a single inbox item.
 */
void todo_free_inbox_item(TodoInboxItem* item);

/**
 * Free an array of inbox items.
 */
void todo_free_inbox_items(TodoInboxItem** items, int count);

// ============================================================================
// DATE PARSING (Natural Language)
// ============================================================================

/**
 * Parse a natural language date string.
 * Supports: "today", "tomorrow", "next monday", "in 2 hours",
 *           "Dec 25", "2025-12-25", etc.
 * @param input Natural language date string
 * @param base_time Reference time (0 = now)
 * @return Parsed timestamp, 0 on parse error
 */
time_t todo_parse_date(const char* input, time_t base_time);

/**
 * Parse a duration string for reminders.
 * Supports: "30m", "1h", "2d", "1w", etc.
 * @param input Duration string
 * @return Duration in seconds, 0 on parse error
 */
int64_t todo_parse_duration(const char* input);

/**
 * Format a timestamp for display.
 * @param timestamp Time to format
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @param relative If true, use relative format ("in 2 hours", "yesterday")
 */
void todo_format_date(time_t timestamp, char* buffer, size_t buffer_size, bool relative);

// ============================================================================
// UTILITY
// ============================================================================

/**
 * Get status as string.
 */
const char* todo_status_to_string(TodoStatus status);

/**
 * Get priority as string.
 */
const char* todo_priority_to_string(TodoPriority priority);

/**
 * Parse status from string.
 */
TodoStatus todo_status_from_string(const char* str);

/**
 * Parse priority from string.
 */
TodoPriority todo_priority_from_string(const char* str);

#ifdef __cplusplus
}
#endif

#endif // NOUS_TODO_H
