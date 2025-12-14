/**
 * CONVERGIO TODO MANAGER - Implementation
 *
 * SQLite-backed task management with:
 * - CRUD operations
 * - Full-text search (FTS5)
 * - Recurrence support
 * - Inbox quick capture
 *
 * Performance optimizations:
 * - Prepared statement caching
 * - Single shared DB connection
 * - Efficient batch operations
 */

#include "nous/todo.h"
#include "nous/nous.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>

// External database handle from persistence.c
extern sqlite3* g_db;
extern pthread_mutex_t g_db_mutex;

// Prepared statement cache
typedef enum {
    STMT_INSERT_TASK = 0,
    STMT_GET_TASK,
    STMT_UPDATE_TASK,
    STMT_DELETE_TASK,
    STMT_LIST_TASKS,
    STMT_LIST_TODAY,
    STMT_LIST_OVERDUE,
    STMT_COMPLETE_TASK,
    STMT_INSERT_INBOX,
    STMT_LIST_INBOX,
    STMT_SEARCH_FTS,
    STMT_GET_STATS,
    STMT_CACHE_SIZE
} StmtIndex;

static sqlite3_stmt* g_stmts[STMT_CACHE_SIZE] = {0};
static bool g_todo_initialized = false;

static const char* SQL_STATS =
    "SELECT "
    "(SELECT COUNT(*) FROM tasks WHERE status = 0), "
    "(SELECT COUNT(*) FROM tasks WHERE status = 1), "
    "(SELECT COUNT(*) FROM tasks WHERE status = 2 AND date(completed_at) = date('now')), "
    "(SELECT COUNT(*) FROM tasks WHERE status = 2 AND date(completed_at) >= date('now', '-7 days')), "
    "(SELECT COUNT(*) FROM tasks WHERE status IN (0,1) AND due_date IS NOT NULL AND datetime(due_date) < datetime('now')), "
    "(SELECT COUNT(*) FROM inbox WHERE processed = 0)";

void todo_invalidate_stats_statement(void) {
    pthread_mutex_lock(&g_db_mutex);
    if (g_stmts[STMT_GET_STATS]) {
        sqlite3_finalize(g_stmts[STMT_GET_STATS]);
        g_stmts[STMT_GET_STATS] = NULL;
    }
    pthread_mutex_unlock(&g_db_mutex);
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static char* safe_strdup(const char* s) {
    return s ? strdup(s) : NULL;
}

static time_t parse_iso8601(const char* str) {
    if (!str || !*str) return 0;

    struct tm tm = {0};
    // Try ISO 8601 format: YYYY-MM-DD HH:MM:SS
    if (sscanf(str, "%d-%d-%d %d:%d:%d",
               &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
               &tm.tm_hour, &tm.tm_min, &tm.tm_sec) >= 3) {
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        return mktime(&tm);
    }
    return 0;
}

static void format_iso8601(time_t t, char* buf, size_t size) {
    if (t == 0) {
        buf[0] = '\0';
        return;
    }
    struct tm* tm = localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm);
}

static TodoTask* task_from_row(sqlite3_stmt* stmt) {
    TodoTask* task = calloc(1, sizeof(TodoTask));
    if (!task) return NULL;

    int col = 0;
    task->id = sqlite3_column_int64(stmt, col++);
    task->title = safe_strdup((const char*)sqlite3_column_text(stmt, col++));
    task->description = safe_strdup((const char*)sqlite3_column_text(stmt, col++));
    task->priority = (TodoPriority)sqlite3_column_int(stmt, col++);
    task->status = (TodoStatus)sqlite3_column_int(stmt, col++);
    task->due_date = parse_iso8601((const char*)sqlite3_column_text(stmt, col++));
    task->reminder_at = parse_iso8601((const char*)sqlite3_column_text(stmt, col++));
    task->recurrence = (TodoRecurrence)sqlite3_column_int(stmt, col++);
    task->recurrence_rule = safe_strdup((const char*)sqlite3_column_text(stmt, col++));
    task->tags = safe_strdup((const char*)sqlite3_column_text(stmt, col++));
    task->context = safe_strdup((const char*)sqlite3_column_text(stmt, col++));
    task->parent_id = sqlite3_column_int64(stmt, col++);
    task->source = (TodoSource)sqlite3_column_int(stmt, col++);
    task->external_id = safe_strdup((const char*)sqlite3_column_text(stmt, col++));
    task->created_at = parse_iso8601((const char*)sqlite3_column_text(stmt, col++));
    task->updated_at = parse_iso8601((const char*)sqlite3_column_text(stmt, col++));
    task->completed_at = parse_iso8601((const char*)sqlite3_column_text(stmt, col++));

    return task;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

int todo_init(void) {
    if (g_todo_initialized) return 0;
    if (!g_db) return -1;

    // Prepare frequently used statements
    const char* sql_insert =
        "INSERT INTO tasks (title, description, priority, status, due_date, "
        "reminder_at, recurrence, recurrence_rule, tags, context, parent_id, "
        "source, external_id) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    const char* sql_get =
        "SELECT id, title, description, priority, status, due_date, reminder_at, "
        "recurrence, recurrence_rule, tags, context, parent_id, source, external_id, "
        "created_at, updated_at, completed_at FROM tasks WHERE id = ?";

    const char* sql_list_today =
        "SELECT id, title, description, priority, status, due_date, reminder_at, "
        "recurrence, recurrence_rule, tags, context, parent_id, source, external_id, "
        "created_at, updated_at, completed_at FROM tasks "
        "WHERE status IN (0, 1) AND (due_date IS NULL OR date(due_date) <= date('now')) "
        "ORDER BY priority ASC, due_date ASC LIMIT 100";

    const char* sql_list_overdue =
        "SELECT id, title, description, priority, status, due_date, reminder_at, "
        "recurrence, recurrence_rule, tags, context, parent_id, source, external_id, "
        "created_at, updated_at, completed_at FROM tasks "
        "WHERE status IN (0, 1) AND due_date IS NOT NULL AND datetime(due_date) < datetime('now') "
        "ORDER BY due_date ASC LIMIT 100";

    const char* sql_complete =
        "UPDATE tasks SET status = 2, completed_at = datetime('now'), "
        "updated_at = datetime('now') WHERE id = ?";

    const char* sql_insert_inbox =
        "INSERT INTO inbox (content, source) VALUES (?, ?)";

    const char* sql_list_inbox =
        "SELECT id, content, captured_at, processed, processed_task_id, source "
        "FROM inbox WHERE processed = 0 ORDER BY captured_at DESC LIMIT 50";

    const char* sql_search =
        "SELECT t.id, t.title, t.description, t.priority, t.status, t.due_date, "
        "t.reminder_at, t.recurrence, t.recurrence_rule, t.tags, t.context, "
        "t.parent_id, t.source, t.external_id, t.created_at, t.updated_at, t.completed_at "
        "FROM tasks t JOIN tasks_fts f ON t.id = f.rowid "
        "WHERE tasks_fts MATCH ? AND t.status IN (0, 1) "
        "ORDER BY rank LIMIT 50";

    pthread_mutex_lock(&g_db_mutex);

    int rc = 0;
    rc |= sqlite3_prepare_v3(g_db, sql_insert, -1, SQLITE_PREPARE_PERSISTENT, &g_stmts[STMT_INSERT_TASK], NULL);
    rc |= sqlite3_prepare_v3(g_db, sql_get, -1, SQLITE_PREPARE_PERSISTENT, &g_stmts[STMT_GET_TASK], NULL);
    rc |= sqlite3_prepare_v3(g_db, sql_list_today, -1, SQLITE_PREPARE_PERSISTENT, &g_stmts[STMT_LIST_TODAY], NULL);
    rc |= sqlite3_prepare_v3(g_db, sql_list_overdue, -1, SQLITE_PREPARE_PERSISTENT, &g_stmts[STMT_LIST_OVERDUE], NULL);
    rc |= sqlite3_prepare_v3(g_db, sql_complete, -1, SQLITE_PREPARE_PERSISTENT, &g_stmts[STMT_COMPLETE_TASK], NULL);
    rc |= sqlite3_prepare_v3(g_db, sql_insert_inbox, -1, SQLITE_PREPARE_PERSISTENT, &g_stmts[STMT_INSERT_INBOX], NULL);
    rc |= sqlite3_prepare_v3(g_db, sql_list_inbox, -1, SQLITE_PREPARE_PERSISTENT, &g_stmts[STMT_LIST_INBOX], NULL);
    rc |= sqlite3_prepare_v3(g_db, sql_search, -1, SQLITE_PREPARE_PERSISTENT, &g_stmts[STMT_SEARCH_FTS], NULL);
    rc |= sqlite3_prepare_v3(g_db, SQL_STATS, -1, SQLITE_PREPARE_PERSISTENT, &g_stmts[STMT_GET_STATS], NULL);

    pthread_mutex_unlock(&g_db_mutex);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "todo_init: Failed to prepare statements\n");
        return -1;
    }

    g_todo_initialized = true;
    return 0;
}

void todo_shutdown(void) {
    if (!g_todo_initialized) return;

    pthread_mutex_lock(&g_db_mutex);
    for (int i = 0; i < STMT_CACHE_SIZE; i++) {
        if (g_stmts[i]) {
            sqlite3_finalize(g_stmts[i]);
            g_stmts[i] = NULL;
        }
    }
    pthread_mutex_unlock(&g_db_mutex);

    g_todo_initialized = false;
}

// ============================================================================
// TASK CRUD
// ============================================================================

int64_t todo_create(const TodoCreateOptions* options) {
    if (!options || !options->title || !*options->title) return -1;
    if (!g_todo_initialized && todo_init() != 0) return -1;

    char due_buf[32] = {0};
    char remind_buf[32] = {0};
    format_iso8601(options->due_date, due_buf, sizeof(due_buf));
    format_iso8601(options->reminder_at, remind_buf, sizeof(remind_buf));

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = g_stmts[STMT_INSERT_TASK];
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    sqlite3_bind_text(stmt, 1, options->title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, options->description, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, options->priority ? options->priority : TODO_PRIORITY_NORMAL);
    sqlite3_bind_int(stmt, 4, TODO_STATUS_PENDING);
    sqlite3_bind_text(stmt, 5, due_buf[0] ? due_buf : NULL, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, remind_buf[0] ? remind_buf : NULL, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, options->recurrence);
    sqlite3_bind_text(stmt, 8, options->recurrence_rule, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, options->tags, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, options->context, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 11, options->parent_id);
    sqlite3_bind_int(stmt, 12, options->source);
    sqlite3_bind_text(stmt, 13, options->external_id, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    int64_t id = -1;

    if (rc == SQLITE_DONE) {
        id = sqlite3_last_insert_rowid(g_db);
    }

    pthread_mutex_unlock(&g_db_mutex);
    return id;
}

TodoTask* todo_get(int64_t id) {
    if (!g_todo_initialized && todo_init() != 0) return NULL;

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = g_stmts[STMT_GET_TASK];
    sqlite3_reset(stmt);
    sqlite3_bind_int64(stmt, 1, id);

    TodoTask* task = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        task = task_from_row(stmt);
    }

    pthread_mutex_unlock(&g_db_mutex);
    return task;
}

int todo_update(int64_t id, const TodoCreateOptions* options) {
    if (!options) return -1;
    if (!g_todo_initialized && todo_init() != 0) return -1;

    // Build dynamic UPDATE query with parameter placeholders
    char sql[1024];
    int len = snprintf(sql, sizeof(sql), "UPDATE tasks SET updated_at = datetime('now')");

    if (options->title)
        len += snprintf(sql + len, sizeof(sql) - len, ", title = ?");
    if (options->description)
        len += snprintf(sql + len, sizeof(sql) - len, ", description = ?");
    if (options->priority)
        len += snprintf(sql + len, sizeof(sql) - len, ", priority = ?");
    if (options->due_date)
        len += snprintf(sql + len, sizeof(sql) - len, ", due_date = ?");
    if (options->reminder_at)
        len += snprintf(sql + len, sizeof(sql) - len, ", reminder_at = ?");
    if (options->context)
        len += snprintf(sql + len, sizeof(sql) - len, ", context = ?");
    if (options->tags)
        len += snprintf(sql + len, sizeof(sql) - len, ", tags = ?");

    snprintf(sql + len, sizeof(sql) - len, " WHERE id = ?");

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return -1;
    }

    // Bind parameters in order
    int param_idx = 1;
    if (options->title)
        sqlite3_bind_text(stmt, param_idx++, options->title, -1, SQLITE_TRANSIENT);
    if (options->description)
        sqlite3_bind_text(stmt, param_idx++, options->description, -1, SQLITE_TRANSIENT);
    if (options->priority)
        sqlite3_bind_int(stmt, param_idx++, options->priority);
    if (options->due_date) {
        char buf[32];
        format_iso8601(options->due_date, buf, sizeof(buf));
        sqlite3_bind_text(stmt, param_idx++, buf, -1, SQLITE_TRANSIENT);
    }
    if (options->reminder_at) {
        char buf[32];
        format_iso8601(options->reminder_at, buf, sizeof(buf));
        sqlite3_bind_text(stmt, param_idx++, buf, -1, SQLITE_TRANSIENT);
    }
    if (options->context)
        sqlite3_bind_text(stmt, param_idx++, options->context, -1, SQLITE_TRANSIENT);
    if (options->tags)
        sqlite3_bind_text(stmt, param_idx++, options->tags, -1, SQLITE_TRANSIENT);

    sqlite3_bind_int64(stmt, param_idx, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int todo_delete(int64_t id) {
    if (!g_todo_initialized && todo_init() != 0) return -1;

    const char* sql = "DELETE FROM tasks WHERE id = ?";

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ============================================================================
// STATUS CHANGES
// ============================================================================

int todo_complete(int64_t id) {
    if (!g_todo_initialized && todo_init() != 0) return -1;

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = g_stmts[STMT_COMPLETE_TASK];
    sqlite3_reset(stmt);
    sqlite3_bind_int64(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    pthread_mutex_unlock(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int todo_uncomplete(int64_t id) {
    if (!g_todo_initialized && todo_init() != 0) return -1;

    const char* sql = "UPDATE tasks SET status = 0, completed_at = NULL, "
                      "updated_at = datetime('now') WHERE id = ?";

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int todo_start(int64_t id) {
    if (!g_todo_initialized && todo_init() != 0) return -1;

    const char* sql = "UPDATE tasks SET status = 1, updated_at = datetime('now') WHERE id = ?";

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int todo_cancel(int64_t id) {
    if (!g_todo_initialized && todo_init() != 0) return -1;

    const char* sql = "UPDATE tasks SET status = 3, updated_at = datetime('now') WHERE id = ?";

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ============================================================================
// LISTING
// ============================================================================

static TodoTask** execute_list_query(sqlite3_stmt* stmt, int* count) {
    TodoTask** tasks = NULL;
    int capacity = 16;
    int n = 0;

    tasks = malloc(capacity * sizeof(TodoTask*));
    if (!tasks) {
        *count = 0;
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (n >= capacity) {
            capacity *= 2;
            TodoTask** new_tasks = realloc(tasks, capacity * sizeof(TodoTask*));
            if (!new_tasks) break;
            tasks = new_tasks;
        }
        tasks[n++] = task_from_row(stmt);
    }

    *count = n;
    return tasks;
}

TodoTask** todo_list_today(int* count) {
    if (!g_todo_initialized && todo_init() != 0) {
        *count = 0;
        return NULL;
    }

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = g_stmts[STMT_LIST_TODAY];
    sqlite3_reset(stmt);

    TodoTask** tasks = execute_list_query(stmt, count);

    pthread_mutex_unlock(&g_db_mutex);
    return tasks;
}

TodoTask** todo_list_overdue(int* count) {
    if (!g_todo_initialized && todo_init() != 0) {
        *count = 0;
        return NULL;
    }

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = g_stmts[STMT_LIST_OVERDUE];
    sqlite3_reset(stmt);

    TodoTask** tasks = execute_list_query(stmt, count);

    pthread_mutex_unlock(&g_db_mutex);
    return tasks;
}

TodoTask** todo_list_upcoming(int days, int* count) {
    if (!g_todo_initialized && todo_init() != 0) {
        *count = 0;
        return NULL;
    }

    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, title, description, priority, status, due_date, reminder_at, "
             "recurrence, recurrence_rule, tags, context, parent_id, source, external_id, "
             "created_at, updated_at, completed_at FROM tasks "
             "WHERE status IN (0, 1) AND due_date IS NOT NULL "
             "AND date(due_date) <= date('now', '+%d days') "
             "ORDER BY due_date ASC LIMIT 100", days);

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);

    TodoTask** tasks = execute_list_query(stmt, count);

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);

    return tasks;
}

TodoTask** todo_search(const char* query, int* count) {
    if (!query || !*query) {
        *count = 0;
        return NULL;
    }
    if (!g_todo_initialized && todo_init() != 0) {
        *count = 0;
        return NULL;
    }

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = g_stmts[STMT_SEARCH_FTS];
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, query, -1, SQLITE_TRANSIENT);

    TodoTask** tasks = execute_list_query(stmt, count);

    pthread_mutex_unlock(&g_db_mutex);
    return tasks;
}

TodoTask** todo_list(const TodoFilter* filter, int* count) {
    if (!g_todo_initialized && todo_init() != 0) {
        *count = 0;
        return NULL;
    }

    // Build dynamic query based on filter with parameter placeholders
    char sql[1024];
    int len = snprintf(sql, sizeof(sql),
                       "SELECT id, title, description, priority, status, due_date, reminder_at, "
                       "recurrence, recurrence_rule, tags, context, parent_id, source, external_id, "
                       "created_at, updated_at, completed_at FROM tasks WHERE 1=1");

    // Track which params to bind
    int has_context = 0, has_due_from = 0, has_due_to = 0;
    char due_from_buf[32] = {0}, due_to_buf[32] = {0};

    if (filter) {
        if (!filter->include_completed && !filter->include_cancelled) {
            len += snprintf(sql + len, sizeof(sql) - len, " AND status IN (0, 1)");
        } else if (!filter->include_completed) {
            len += snprintf(sql + len, sizeof(sql) - len, " AND status != 2");
        } else if (!filter->include_cancelled) {
            len += snprintf(sql + len, sizeof(sql) - len, " AND status != 3");
        }

        if (filter->context) {
            len += snprintf(sql + len, sizeof(sql) - len, " AND context = ?");
            has_context = 1;
        }

        if (filter->due_from) {
            format_iso8601(filter->due_from, due_from_buf, sizeof(due_from_buf));
            len += snprintf(sql + len, sizeof(sql) - len, " AND due_date >= ?");
            has_due_from = 1;
        }

        if (filter->due_to) {
            format_iso8601(filter->due_to, due_to_buf, sizeof(due_to_buf));
            len += snprintf(sql + len, sizeof(sql) - len, " AND due_date <= ?");
            has_due_to = 1;
        }

        int limit = filter->limit > 0 ? filter->limit : 100;
        len += snprintf(sql + len, sizeof(sql) - len, " ORDER BY priority ASC, due_date ASC LIMIT %d", limit);

        if (filter->offset > 0) {
            len += snprintf(sql + len, sizeof(sql) - len, " OFFSET %d", filter->offset);
        }
    } else {
        len += snprintf(sql + len, sizeof(sql) - len,
                        " AND status IN (0, 1) ORDER BY priority ASC, due_date ASC LIMIT 100");
    }

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        *count = 0;
        return NULL;
    }

    // Bind parameters in order
    int param_idx = 1;
    if (has_context)
        sqlite3_bind_text(stmt, param_idx++, filter->context, -1, SQLITE_TRANSIENT);
    if (has_due_from)
        sqlite3_bind_text(stmt, param_idx++, due_from_buf, -1, SQLITE_TRANSIENT);
    if (has_due_to)
        sqlite3_bind_text(stmt, param_idx++, due_to_buf, -1, SQLITE_TRANSIENT);

    TodoTask** tasks = execute_list_query(stmt, count);

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);

    return tasks;
}

// ============================================================================
// INBOX
// ============================================================================

int64_t inbox_capture(const char* content, const char* source) {
    if (!content || !*content) return -1;
    if (!g_todo_initialized && todo_init() != 0) return -1;

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = g_stmts[STMT_INSERT_INBOX];
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, content, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, source ? source : "cli", -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    int64_t id = -1;

    if (rc == SQLITE_DONE) {
        id = sqlite3_last_insert_rowid(g_db);
    }

    pthread_mutex_unlock(&g_db_mutex);
    return id;
}

TodoInboxItem** inbox_list_unprocessed(int* count) {
    if (!g_todo_initialized && todo_init() != 0) {
        *count = 0;
        return NULL;
    }

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = g_stmts[STMT_LIST_INBOX];
    sqlite3_reset(stmt);

    TodoInboxItem** items = NULL;
    int capacity = 16;
    int n = 0;

    items = malloc(capacity * sizeof(TodoInboxItem*));
    if (!items) {
        pthread_mutex_unlock(&g_db_mutex);
        *count = 0;
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (n >= capacity) {
            capacity *= 2;
            TodoInboxItem** new_items = realloc(items, capacity * sizeof(TodoInboxItem*));
            if (!new_items) break;
            items = new_items;
        }

        TodoInboxItem* item = calloc(1, sizeof(TodoInboxItem));
        item->id = sqlite3_column_int64(stmt, 0);
        item->content = safe_strdup((const char*)sqlite3_column_text(stmt, 1));
        item->captured_at = parse_iso8601((const char*)sqlite3_column_text(stmt, 2));
        item->processed = sqlite3_column_int(stmt, 3);
        item->processed_task_id = sqlite3_column_int64(stmt, 4);
        item->source = safe_strdup((const char*)sqlite3_column_text(stmt, 5));

        items[n++] = item;
    }

    pthread_mutex_unlock(&g_db_mutex);
    *count = n;
    return items;
}

int inbox_process(int64_t inbox_id, int64_t task_id) {
    if (!g_todo_initialized && todo_init() != 0) return -1;

    const char* sql = "UPDATE inbox SET processed = 1, processed_task_id = ? WHERE id = ?";

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, task_id);
    sqlite3_bind_int64(stmt, 2, inbox_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int inbox_delete(int64_t inbox_id) {
    if (!g_todo_initialized && todo_init() != 0) return -1;

    const char* sql = "DELETE FROM inbox WHERE id = ?";

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, inbox_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ============================================================================
// STATISTICS
// ============================================================================

TodoStats todo_get_stats(void) {
    TodoStats stats = {0};
    if (!g_todo_initialized && todo_init() != 0) return stats;

    pthread_mutex_lock(&g_db_mutex);
    sqlite3_stmt* stmt = g_stmts[STMT_GET_STATS];
    if (!stmt) {
        if (sqlite3_prepare_v3(g_db, SQL_STATS, -1, SQLITE_PREPARE_PERSISTENT,
                               &g_stmts[STMT_GET_STATS], NULL) != SQLITE_OK) {
            pthread_mutex_unlock(&g_db_mutex);
            return stats;
        }
        stmt = g_stmts[STMT_GET_STATS];
    }

    sqlite3_reset(stmt);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stats.total_pending = sqlite3_column_int(stmt, 0);
        stats.total_in_progress = sqlite3_column_int(stmt, 1);
        stats.total_completed_today = sqlite3_column_int(stmt, 2);
        stats.total_completed_week = sqlite3_column_int(stmt, 3);
        stats.total_overdue = sqlite3_column_int(stmt, 4);
        stats.inbox_unprocessed = sqlite3_column_int(stmt, 5);
    }

    pthread_mutex_unlock(&g_db_mutex);
    return stats;
}

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

void todo_free_task(TodoTask* task) {
    if (!task) return;
    free(task->title);
    free(task->description);
    free(task->recurrence_rule);
    free(task->tags);
    free(task->context);
    free(task->external_id);
    free(task);
}

void todo_free_tasks(TodoTask** tasks, int count) {
    if (!tasks) return;
    for (int i = 0; i < count; i++) {
        todo_free_task(tasks[i]);
    }
    free(tasks);
}

void todo_free_inbox_item(TodoInboxItem* item) {
    if (!item) return;
    free(item->content);
    free(item->source);
    free(item);
}

void todo_free_inbox_items(TodoInboxItem** items, int count) {
    if (!items) return;
    for (int i = 0; i < count; i++) {
        todo_free_inbox_item(items[i]);
    }
    free(items);
}

// ============================================================================
// UTILITY
// ============================================================================

const char* todo_status_to_string(TodoStatus status) {
    switch (status) {
        case TODO_STATUS_PENDING: return "pending";
        case TODO_STATUS_IN_PROGRESS: return "in_progress";
        case TODO_STATUS_COMPLETED: return "completed";
        case TODO_STATUS_CANCELLED: return "cancelled";
        default: return "unknown";
    }
}

const char* todo_priority_to_string(TodoPriority priority) {
    switch (priority) {
        case TODO_PRIORITY_URGENT: return "urgent";
        case TODO_PRIORITY_NORMAL: return "normal";
        case TODO_PRIORITY_LOW: return "low";
        default: return "normal";
    }
}

TodoStatus todo_status_from_string(const char* str) {
    if (!str) return TODO_STATUS_PENDING;
    if (strcasecmp(str, "pending") == 0) return TODO_STATUS_PENDING;
    if (strcasecmp(str, "in_progress") == 0) return TODO_STATUS_IN_PROGRESS;
    if (strcasecmp(str, "completed") == 0) return TODO_STATUS_COMPLETED;
    if (strcasecmp(str, "cancelled") == 0) return TODO_STATUS_CANCELLED;
    return TODO_STATUS_PENDING;
}

TodoPriority todo_priority_from_string(const char* str) {
    if (!str) return TODO_PRIORITY_NORMAL;
    if (strcasecmp(str, "urgent") == 0 || strcmp(str, "1") == 0) return TODO_PRIORITY_URGENT;
    if (strcasecmp(str, "normal") == 0 || strcmp(str, "2") == 0) return TODO_PRIORITY_NORMAL;
    if (strcasecmp(str, "low") == 0 || strcmp(str, "3") == 0) return TODO_PRIORITY_LOW;
    return TODO_PRIORITY_NORMAL;
}

// ============================================================================
// DATE PARSING (Natural Language)
// ============================================================================

// Helper: Parse weekday name to number (0=Sunday, 1=Monday, etc.)
static int parse_weekday(const char* str) {
    if (!str) return -1;
    if (strncmp(str, "sun", 3) == 0) return 0;
    if (strncmp(str, "mon", 3) == 0) return 1;
    if (strncmp(str, "tue", 3) == 0) return 2;
    if (strncmp(str, "wed", 3) == 0) return 3;
    if (strncmp(str, "thu", 3) == 0) return 4;
    if (strncmp(str, "fri", 3) == 0) return 5;
    if (strncmp(str, "sat", 3) == 0) return 6;
    // Italian
    if (strncmp(str, "dom", 3) == 0) return 0;
    if (strncmp(str, "lun", 3) == 0) return 1;
    if (strncmp(str, "mar", 3) == 0) return 2;
    if (strncmp(str, "mer", 3) == 0) return 3;
    if (strncmp(str, "gio", 3) == 0) return 4;
    if (strncmp(str, "ven", 3) == 0) return 5;
    if (strncmp(str, "sab", 3) == 0) return 6;
    return -1;
}

// Helper: Parse time of day (returns hour, -1 if not found)
static int parse_time_of_day(const char* str, int* out_min) {
    if (!str) return -1;
    *out_min = 0;

    // Time keywords
    if (strstr(str, "morning") || strstr(str, "mattina")) return 9;
    if (strstr(str, "noon") || strstr(str, "mezzogiorno")) return 12;
    if (strstr(str, "afternoon") || strstr(str, "pomeriggio")) return 14;
    if (strstr(str, "evening") || strstr(str, "sera")) return 19;
    if (strstr(str, "tonight") || strstr(str, "stasera")) return 20;
    if (strstr(str, "night") || strstr(str, "notte")) return 21;

    // Parse "at Xpm", "at X:YY", "at X am", "alle X"
    const char* at_pos = strstr(str, "at ");
    if (!at_pos) at_pos = strstr(str, "alle ");
    if (!at_pos) at_pos = strstr(str, "@ ");

    if (at_pos) {
        // Skip "at " or "alle " or "@ "
        const char* time_str = at_pos + (at_pos[0] == '@' ? 2 : (at_pos[1] == 't' ? 3 : 5));
        while (*time_str == ' ') time_str++;

        int hour = 0, min = 0;
        char ampm[8] = {0};

        // Try "HH:MM am/pm" or "HH:MM"
        if (sscanf(time_str, "%d:%d %7s", &hour, &min, ampm) >= 2 ||
            sscanf(time_str, "%d:%d%7s", &hour, &min, ampm) >= 2) {
            if (ampm[0] == 'p' || ampm[0] == 'P') {
                if (hour < 12) hour += 12;
            } else if ((ampm[0] == 'a' || ampm[0] == 'A') && hour == 12) {
                hour = 0;
            }
            *out_min = min;
            return hour;
        }

        // Try "Xpm", "X pm", "Xam", "X am"
        if (sscanf(time_str, "%d %7s", &hour, ampm) >= 1 ||
            sscanf(time_str, "%d%7s", &hour, ampm) >= 1) {
            if (ampm[0] == 'p' || ampm[0] == 'P') {
                if (hour < 12) hour += 12;
            } else if ((ampm[0] == 'a' || ampm[0] == 'A') && hour == 12) {
                hour = 0;
            }
            return hour;
        }
    }

    return -1;
}

/**
 * Parse a natural language date string.
 * Supports:
 *   - Keywords: today, tomorrow, tonight, this evening
 *   - Time of day: morning, afternoon, evening, night
 *   - Relative: next monday, in 2 hours, in 3 days
 *   - Complex: thursday in two weeks, monday in 3 weeks
 *   - Specific time: at 3pm, at 15:00, tomorrow at 9am
 *   - Dates: Dec 25, 2025-12-25, december 15
 *   - Italian: domani, stasera, lunedi prossimo
 */
time_t todo_parse_date(const char* input, time_t base_time) {
    if (!input || !*input) return 0;

    time_t base = base_time > 0 ? base_time : time(NULL);
    struct tm tm_copy;
    struct tm* tm_ptr = localtime(&base);
    memcpy(&tm_copy, tm_ptr, sizeof(struct tm));
    struct tm* tm = &tm_copy;

    // Make a mutable copy for parsing
    char buf[256];
    strncpy(buf, input, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    // Convert to lowercase for matching
    for (char* p = buf; *p; p++) *p = tolower(*p);

    // Default time is end of day
    int target_hour = 23;
    int target_min = 59;

    // First, extract any time specification
    int time_min = 0;
    int parsed_hour = parse_time_of_day(buf, &time_min);
    if (parsed_hour >= 0) {
        target_hour = parsed_hour;
        target_min = time_min;
    }

    // ============================================================
    // TIME-OF-DAY KEYWORDS (no date change)
    // ============================================================

    if (strcmp(buf, "tonight") == 0 || strcmp(buf, "stasera") == 0) {
        tm->tm_hour = 20;
        tm->tm_min = 0;
        tm->tm_sec = 0;
        return mktime(tm);
    }

    if (strcmp(buf, "now") == 0 || strcmp(buf, "adesso") == 0) {
        return base + 60;  // 1 minute from now
    }

    // ============================================================
    // SIMPLE DATE KEYWORDS
    // ============================================================

    if (strcmp(buf, "today") == 0 || strcmp(buf, "oggi") == 0) {
        tm->tm_hour = target_hour;
        tm->tm_min = target_min;
        tm->tm_sec = 0;
        return mktime(tm);
    }

    if (strcmp(buf, "tomorrow") == 0 || strcmp(buf, "domani") == 0 ||
        strncmp(buf, "tomorrow ", 9) == 0 || strncmp(buf, "domani ", 7) == 0) {
        tm->tm_mday += 1;
        tm->tm_hour = target_hour;
        tm->tm_min = target_min;
        tm->tm_sec = 0;
        return mktime(tm);
    }

    // "tomorrow morning", "domani mattina"
    if (strstr(buf, "tomorrow") || strstr(buf, "domani")) {
        tm->tm_mday += 1;
        tm->tm_hour = target_hour;
        tm->tm_min = target_min;
        tm->tm_sec = 0;
        return mktime(tm);
    }

    // ============================================================
    // "NEXT <weekday>" or "<weekday> prossimo"
    // ============================================================

    if (strncmp(buf, "next ", 5) == 0) {
        const char* rest = buf + 5;

        if (strncmp(rest, "week", 4) == 0) {
            tm->tm_mday += 7;
            tm->tm_hour = target_hour;
            tm->tm_min = target_min;
            tm->tm_sec = 0;
            return mktime(tm);
        }

        int target_wday = parse_weekday(rest);
        if (target_wday >= 0) {
            int days_ahead = target_wday - tm->tm_wday;
            if (days_ahead <= 0) days_ahead += 7;
            tm->tm_mday += days_ahead;
            tm->tm_hour = target_hour;
            tm->tm_min = target_min;
            tm->tm_sec = 0;
            return mktime(tm);
        }
    }

    // Italian: "lunedi prossimo", "martedi prossimo"
    if (strstr(buf, "prossim")) {
        int target_wday = parse_weekday(buf);
        if (target_wday >= 0) {
            int days_ahead = target_wday - tm->tm_wday;
            if (days_ahead <= 0) days_ahead += 7;
            tm->tm_mday += days_ahead;
            tm->tm_hour = target_hour;
            tm->tm_min = target_min;
            tm->tm_sec = 0;
            return mktime(tm);
        }
    }

    // ============================================================
    // "<weekday> in N weeks" - e.g., "thursday in two weeks"
    // ============================================================

    {
        int target_wday = parse_weekday(buf);
        if (target_wday >= 0) {
            // Check for "in N week(s)" or "tra N settiman"
            int weeks = 0;
            char* in_pos = strstr(buf, " in ");
            char* tra_pos = strstr(buf, " tra ");

            if (in_pos || tra_pos) {
                char* num_start = in_pos ? in_pos + 4 : tra_pos + 5;

                // Parse number (including words)
                if (strncmp(num_start, "two", 3) == 0 || strncmp(num_start, "due", 3) == 0) weeks = 2;
                else if (strncmp(num_start, "three", 5) == 0 || strncmp(num_start, "tre", 3) == 0) weeks = 3;
                else if (strncmp(num_start, "four", 4) == 0 || strncmp(num_start, "quattro", 7) == 0) weeks = 4;
                else sscanf(num_start, "%d", &weeks);

                if (weeks > 0) {
                    int days_ahead = target_wday - tm->tm_wday;
                    if (days_ahead <= 0) days_ahead += 7;
                    tm->tm_mday += days_ahead + (weeks - 1) * 7;
                    tm->tm_hour = target_hour;
                    tm->tm_min = target_min;
                    tm->tm_sec = 0;
                    return mktime(tm);
                }
            }

            // Just a weekday name - assume this week or next
            int days_ahead = target_wday - tm->tm_wday;
            if (days_ahead <= 0) days_ahead += 7;
            tm->tm_mday += days_ahead;
            tm->tm_hour = target_hour;
            tm->tm_min = target_min;
            tm->tm_sec = 0;
            return mktime(tm);
        }
    }

    // ============================================================
    // "IN N <unit>" - relative time
    // ============================================================

    if (strncmp(buf, "in ", 3) == 0 || strncmp(buf, "tra ", 4) == 0) {
        int offset = (buf[0] == 'i') ? 3 : 4;
        int n = 0;
        char unit[32] = {0};

        if (sscanf(buf + offset, "%d %31s", &n, unit) >= 2) {
            if (strncmp(unit, "hour", 4) == 0 || strncmp(unit, "or", 2) == 0) {
                return base + n * 3600;
            } else if (strncmp(unit, "day", 3) == 0 || strncmp(unit, "giorn", 5) == 0) {
                tm->tm_mday += n;
                tm->tm_hour = target_hour;
                tm->tm_min = target_min;
                tm->tm_sec = 0;
                return mktime(tm);
            } else if (strncmp(unit, "week", 4) == 0 || strncmp(unit, "settiman", 8) == 0) {
                tm->tm_mday += n * 7;
                tm->tm_hour = target_hour;
                tm->tm_min = target_min;
                tm->tm_sec = 0;
                return mktime(tm);
            } else if (strncmp(unit, "min", 3) == 0) {
                return base + n * 60;
            } else if (strncmp(unit, "month", 5) == 0 || strncmp(unit, "mes", 3) == 0) {
                tm->tm_mon += n;
                tm->tm_hour = target_hour;
                tm->tm_min = target_min;
                tm->tm_sec = 0;
                return mktime(tm);
            }
        }
    }

    // ============================================================
    // ISO FORMAT: YYYY-MM-DD [HH:MM]
    // ============================================================

    int year, month, day, hour = -1, min = 0;
    if (sscanf(input, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &min) >= 3) {
        tm->tm_year = year - 1900;
        tm->tm_mon = month - 1;
        tm->tm_mday = day;
        tm->tm_hour = (hour >= 0) ? hour : target_hour;
        tm->tm_min = (hour >= 0) ? min : target_min;
        tm->tm_sec = 0;
        return mktime(tm);
    }

    // ============================================================
    // MONTH DAY FORMAT: "Dec 25", "december 15", "15 december"
    // ============================================================

    const char* months[] = {"jan", "feb", "mar", "apr", "may", "jun",
                            "jul", "aug", "sep", "oct", "nov", "dec"};
    const char* months_it[] = {"gen", "feb", "mar", "apr", "mag", "giu",
                               "lug", "ago", "set", "ott", "nov", "dic"};

    for (int m = 0; m < 12; m++) {
        if (strstr(buf, months[m]) || strstr(buf, months_it[m])) {
            int d = 0;
            // Try "Dec 25" format
            if (sscanf(buf + 3, "%d", &d) == 1 || sscanf(buf + 4, "%d", &d) == 1) {
                // Found
            } else {
                // Try "25 December" format
                sscanf(buf, "%d", &d);
            }

            if (d >= 1 && d <= 31) {
                tm->tm_mon = m;
                tm->tm_mday = d;
                tm->tm_hour = target_hour;
                tm->tm_min = target_min;
                tm->tm_sec = 0;
                // If the date is in the past, assume next year
                time_t result = mktime(tm);
                if (result < base) {
                    tm->tm_year += 1;
                    result = mktime(tm);
                }
                return result;
            }
        }
    }

    // ============================================================
    // JUST A TIME: "at 3pm", "15:00"
    // ============================================================

    if (parsed_hour >= 0) {
        // If we only parsed a time, assume today (or tomorrow if time has passed)
        tm->tm_hour = parsed_hour;
        tm->tm_min = time_min;
        tm->tm_sec = 0;
        time_t result = mktime(tm);
        if (result <= base) {
            // Time has passed, assume tomorrow
            tm->tm_mday += 1;
            result = mktime(tm);
        }
        return result;
    }

    return 0;  // Parse failed
}

/**
 * Parse a duration string for reminders.
 * Supports: "30m", "1h", "2d", "1w", etc.
 */
int64_t todo_parse_duration(const char* input) {
    if (!input || !*input) return 0;

    int n = 0;
    char unit = 'm';

    if (sscanf(input, "%d%c", &n, &unit) >= 1) {
        switch (tolower(unit)) {
            case 's': return n;
            case 'm': return n * 60;
            case 'h': return n * 3600;
            case 'd': return n * 86400;
            case 'w': return n * 604800;
            default: return n * 60;  // Default to minutes
        }
    }

    return 0;
}

/**
 * Format a timestamp for display.
 */
void todo_format_date(time_t timestamp, char* buffer, size_t buffer_size, bool relative) {
    if (!buffer || buffer_size == 0) return;
    buffer[0] = '\0';

    if (timestamp == 0) return;

    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    int today_year = tm_now->tm_year;
    int today_yday = tm_now->tm_yday;

    struct tm* tm_ts = localtime(&timestamp);
    int ts_year = tm_ts->tm_year;
    int ts_yday = tm_ts->tm_yday;

    if (relative) {
        int64_t diff = timestamp - now;

        // Past
        if (diff < 0) {
            diff = -diff;
            if (diff < 60) {
                snprintf(buffer, buffer_size, "just now");
            } else if (diff < 3600) {
                snprintf(buffer, buffer_size, "%lld min ago", diff / 60);
            } else if (diff < 86400 && ts_yday == today_yday) {
                snprintf(buffer, buffer_size, "today");
            } else if (diff < 172800) {
                snprintf(buffer, buffer_size, "yesterday");
            } else if (diff < 604800) {
                snprintf(buffer, buffer_size, "%lld days ago", diff / 86400);
            } else {
                strftime(buffer, buffer_size, "%b %d", tm_ts);
            }
        }
        // Future
        else {
            if (diff < 60) {
                snprintf(buffer, buffer_size, "now");
            } else if (diff < 3600) {
                snprintf(buffer, buffer_size, "in %lld min", diff / 60);
            } else if (diff < 86400 && ts_yday == today_yday) {
                snprintf(buffer, buffer_size, "today");
            } else if (ts_year == today_year && ts_yday == today_yday + 1) {
                snprintf(buffer, buffer_size, "tomorrow");
            } else if (diff < 604800) {
                snprintf(buffer, buffer_size, "in %lld days", diff / 86400);
            } else {
                strftime(buffer, buffer_size, "%b %d", tm_ts);
            }
        }
    } else {
        // Absolute format
        if (ts_year == today_year) {
            strftime(buffer, buffer_size, "%b %d %H:%M", tm_ts);
        } else {
            strftime(buffer, buffer_size, "%Y-%m-%d %H:%M", tm_ts);
        }
    }
}
