/**
 * CONVERGIO PLAN DATABASE
 *
 * SQLite-backed persistent execution plans with thread-safe access.
 */

#include "nous/plan_db.h"
#include "nous/debug_mutex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <limits.h>
#include <uuid/uuid.h>
#include <pthread.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define PLAN_DB_BUSY_TIMEOUT_MS 5000
#define PLAN_DB_MAX_RETRIES 3
#define PLAN_DB_RETRY_DELAY_MS 100

// ============================================================================
// GLOBAL STATE
// ============================================================================

static sqlite3* g_db = NULL;
CONVERGIO_MUTEX_DECLARE(g_db_mutex);
static bool g_initialized = false;
static char g_db_path[PATH_MAX] = {0};

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static void generate_uuid(char* out) {
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, out);
}

static const char* status_to_string(PlanStatus status) {
    switch (status) {
        case PLAN_STATUS_PENDING: return "pending";
        case PLAN_STATUS_ACTIVE: return "active";
        case PLAN_STATUS_COMPLETED: return "completed";
        case PLAN_STATUS_FAILED: return "failed";
        case PLAN_STATUS_CANCELLED: return "cancelled";
        default: return "unknown";
    }
}

static PlanStatus string_to_status(const char* str) {
    if (!str) return PLAN_STATUS_PENDING;
    if (strcmp(str, "active") == 0) return PLAN_STATUS_ACTIVE;
    if (strcmp(str, "completed") == 0) return PLAN_STATUS_COMPLETED;
    if (strcmp(str, "failed") == 0) return PLAN_STATUS_FAILED;
    if (strcmp(str, "cancelled") == 0) return PLAN_STATUS_CANCELLED;
    return PLAN_STATUS_PENDING;
}

static const char* task_status_to_string(TaskDbStatus status) {
    switch (status) {
        case TASK_DB_STATUS_PENDING: return "pending";
        case TASK_DB_STATUS_IN_PROGRESS: return "in_progress";
        case TASK_DB_STATUS_COMPLETED: return "completed";
        case TASK_DB_STATUS_FAILED: return "failed";
        case TASK_DB_STATUS_BLOCKED: return "blocked";
        case TASK_DB_STATUS_SKIPPED: return "skipped";
        default: return "unknown";
    }
}

static TaskDbStatus string_to_task_status(const char* str) {
    if (!str) return TASK_DB_STATUS_PENDING;
    if (strcmp(str, "in_progress") == 0) return TASK_DB_STATUS_IN_PROGRESS;
    if (strcmp(str, "completed") == 0) return TASK_DB_STATUS_COMPLETED;
    if (strcmp(str, "failed") == 0) return TASK_DB_STATUS_FAILED;
    if (strcmp(str, "blocked") == 0) return TASK_DB_STATUS_BLOCKED;
    if (strcmp(str, "skipped") == 0) return TASK_DB_STATUS_SKIPPED;
    return TASK_DB_STATUS_PENDING;
}

static char* safe_strdup(const char* str) {
    return str ? strdup(str) : NULL;
}

static char* json_escape_string(const char* str) {
    if (!str) return strdup("");

    // Calculate needed size (worst case: every char needs escape)
    size_t len = strlen(str);
    size_t needed = len * 2 + 1;
    char* escaped = malloc(needed);
    if (!escaped) return strdup("");

    char* out = escaped;
    for (const char* p = str; *p; p++) {
        switch (*p) {
            case '"':  *out++ = '\\'; *out++ = '"'; break;
            case '\\': *out++ = '\\'; *out++ = '\\'; break;
            case '\n': *out++ = '\\'; *out++ = 'n'; break;
            case '\r': *out++ = '\\'; *out++ = 'r'; break;
            case '\t': *out++ = '\\'; *out++ = 't'; break;
            case '\b': *out++ = '\\'; *out++ = 'b'; break;
            case '\f': *out++ = '\\'; *out++ = 'f'; break;
            default:
                if ((unsigned char)*p < 32) {
                    // Control character - use \u00XX format
                    out += sprintf(out, "\\u%04x", (unsigned char)*p);
                } else {
                    *out++ = *p;
                }
                break;
        }
    }
    *out = '\0';
    return escaped;
}

// ============================================================================
// SCHEMA
// ============================================================================

static const char* SCHEMA_SQL =
    "PRAGMA journal_mode=WAL;\n"
    "PRAGMA busy_timeout=5000;\n"
    "PRAGMA synchronous=NORMAL;\n"
    "PRAGMA foreign_keys=ON;\n"
    "\n"
    "CREATE TABLE IF NOT EXISTS plans (\n"
    "    id TEXT PRIMARY KEY,\n"
    "    description TEXT NOT NULL,\n"
    "    context TEXT,\n"
    "    status TEXT DEFAULT 'pending' CHECK(status IN ('pending','active','completed','failed','cancelled')),\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    updated_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    completed_at INTEGER\n"
    ");\n"
    "\n"
    "CREATE TABLE IF NOT EXISTS tasks (\n"
    "    id TEXT PRIMARY KEY,\n"
    "    plan_id TEXT NOT NULL REFERENCES plans(id) ON DELETE CASCADE,\n"
    "    parent_task_id TEXT REFERENCES tasks(id) ON DELETE CASCADE,\n"
    "    description TEXT NOT NULL,\n"
    "    assigned_agent TEXT,\n"
    "    status TEXT DEFAULT 'pending' CHECK(status IN ('pending','in_progress','completed','failed','blocked','skipped')),\n"
    "    priority INTEGER DEFAULT 50 CHECK(priority >= 0 AND priority <= 100),\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    started_at INTEGER,\n"
    "    completed_at INTEGER,\n"
    "    output TEXT,\n"
    "    error TEXT,\n"
    "    retry_count INTEGER DEFAULT 0\n"
    ");\n"
    "\n"
    "CREATE INDEX IF NOT EXISTS idx_tasks_plan ON tasks(plan_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(plan_id, status);\n"
    "CREATE INDEX IF NOT EXISTS idx_tasks_agent ON tasks(assigned_agent);\n"
    "CREATE INDEX IF NOT EXISTS idx_tasks_parent ON tasks(parent_task_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_plans_status ON plans(status);\n"
    "\n"
    "CREATE TRIGGER IF NOT EXISTS update_plan_timestamp \n"
    "AFTER UPDATE ON plans\n"
    "BEGIN\n"
    "    UPDATE plans SET updated_at = strftime('%s','now') WHERE id = NEW.id;\n"
    "END;\n";

// ============================================================================
// INITIALIZATION
// ============================================================================

PlanDbError plan_db_init(const char* db_path) {
    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    if (g_initialized) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return PLAN_DB_OK;
    }

    // Determine database path
    if (db_path && db_path[0]) {
        snprintf(g_db_path, sizeof(g_db_path), "%s", db_path);
    } else {
        const char* home = getenv("HOME");
        if (!home) home = "/tmp";
        snprintf(g_db_path, sizeof(g_db_path), "%s/.convergio/plans.db", home);

        // Create directory if needed
        char dir_path[PATH_MAX];
        snprintf(dir_path, sizeof(dir_path), "%s/.convergio", home);
        mkdir(dir_path, 0755);
    }

    // Open database
    int rc = sqlite3_open_v2(g_db_path, &g_db,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                              SQLITE_OPEN_FULLMUTEX, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[plan_db] Failed to open database: %s\n", sqlite3_errmsg(g_db));
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return PLAN_DB_ERROR_INIT;
    }

    // Set busy timeout
    sqlite3_busy_timeout(g_db, PLAN_DB_BUSY_TIMEOUT_MS);

    // Execute schema
    char* err_msg = NULL;
    rc = sqlite3_exec(g_db, SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[plan_db] Schema error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(g_db);
        g_db = NULL;
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return PLAN_DB_ERROR_INIT;
    }

    g_initialized = true;
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
    return PLAN_DB_OK;
}

void plan_db_shutdown(void) {
    CONVERGIO_MUTEX_LOCK(&g_db_mutex);
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
    g_initialized = false;
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
}

bool plan_db_is_ready(void) {
    return g_initialized && g_db != NULL;
}

sqlite3* plan_db_get_handle(void) {
    return g_db;
}

// ============================================================================
// PLAN OPERATIONS
// ============================================================================

PlanDbError plan_db_create_plan(const char* description, const char* context, char* out_id) {
    if (!g_initialized || !description || !out_id) return PLAN_DB_ERROR_INVALID;

    generate_uuid(out_id);

    const char* sql = "INSERT INTO plans (id, description, context, status) VALUES (?, ?, ?, 'pending')";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, out_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, context, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? PLAN_DB_OK : PLAN_DB_ERROR_IO;
}

PlanDbError plan_db_get_plan(const char* plan_id, PlanRecord* out_plan) {
    if (!g_initialized || !plan_id || !out_plan) return PLAN_DB_ERROR_INVALID;

    memset(out_plan, 0, sizeof(PlanRecord));

    const char* sql =
        "SELECT p.id, p.description, p.context, p.status, p.created_at, p.updated_at, p.completed_at, "
        "       (SELECT COUNT(*) FROM tasks WHERE plan_id = p.id) as total, "
        "       (SELECT COUNT(*) FROM tasks WHERE plan_id = p.id AND status = 'completed') as completed, "
        "       (SELECT COUNT(*) FROM tasks WHERE plan_id = p.id AND status = 'failed') as failed "
        "FROM plans p WHERE p.id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, plan_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return PLAN_DB_ERROR_NOT_FOUND;
    }

    strncpy(out_plan->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(out_plan->id) - 1);
    out_plan->description = safe_strdup((const char*)sqlite3_column_text(stmt, 1));
    out_plan->context = safe_strdup((const char*)sqlite3_column_text(stmt, 2));
    out_plan->status = string_to_status((const char*)sqlite3_column_text(stmt, 3));
    out_plan->created_at = sqlite3_column_int64(stmt, 4);
    out_plan->updated_at = sqlite3_column_int64(stmt, 5);
    out_plan->completed_at = sqlite3_column_int64(stmt, 6);
    out_plan->total_tasks = sqlite3_column_int(stmt, 7);
    out_plan->completed_tasks = sqlite3_column_int(stmt, 8);
    out_plan->failed_tasks = sqlite3_column_int(stmt, 9);

    if (out_plan->total_tasks > 0) {
        out_plan->progress_percent = (double)out_plan->completed_tasks / out_plan->total_tasks * 100.0;
    }

    sqlite3_finalize(stmt);
    return PLAN_DB_OK;
}

PlanDbError plan_db_update_plan_status(const char* plan_id, PlanStatus status) {
    if (!g_initialized || !plan_id) return PLAN_DB_ERROR_INVALID;

    const char* sql;
    if (status == PLAN_STATUS_COMPLETED || status == PLAN_STATUS_FAILED || status == PLAN_STATUS_CANCELLED) {
        sql = "UPDATE plans SET status = ?, completed_at = strftime('%s','now') WHERE id = ?";
    } else {
        sql = "UPDATE plans SET status = ? WHERE id = ?";
    }

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, status_to_string(status), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, plan_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return PLAN_DB_ERROR_IO;
    if (sqlite3_changes(g_db) == 0) return PLAN_DB_ERROR_NOT_FOUND;

    return PLAN_DB_OK;
}

PlanDbError plan_db_delete_plan(const char* plan_id) {
    if (!g_initialized || !plan_id) return PLAN_DB_ERROR_INVALID;

    const char* sql = "DELETE FROM plans WHERE id = ?";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, plan_id, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return PLAN_DB_ERROR_IO;
    if (sqlite3_changes(g_db) == 0) return PLAN_DB_ERROR_NOT_FOUND;

    return PLAN_DB_OK;
}

PlanDbError plan_db_list_plans(int status, int limit, int offset,
                                PlanRecord* out_plans, int max_plans, int* out_count) {
    if (!g_initialized || !out_plans || !out_count) return PLAN_DB_ERROR_INVALID;

    *out_count = 0;

    char sql[512];
    if (status >= 0) {
        snprintf(sql, sizeof(sql),
            "SELECT id, description, context, status, created_at, updated_at, completed_at "
            "FROM plans WHERE status = ? ORDER BY updated_at DESC LIMIT ? OFFSET ?");
    } else {
        snprintf(sql, sizeof(sql),
            "SELECT id, description, context, status, created_at, updated_at, completed_at "
            "FROM plans ORDER BY updated_at DESC LIMIT ? OFFSET ?");
    }

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    int param = 1;
    if (status >= 0) {
        sqlite3_bind_text(stmt, param++, status_to_string((PlanStatus)status), -1, SQLITE_STATIC);
    }
    sqlite3_bind_int(stmt, param++, limit > 0 ? limit : 1000);
    sqlite3_bind_int(stmt, param++, offset);

    int count = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && count < max_plans) {
        PlanRecord* p = &out_plans[count];
        memset(p, 0, sizeof(PlanRecord));

        strncpy(p->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(p->id) - 1);
        p->description = safe_strdup((const char*)sqlite3_column_text(stmt, 1));
        p->context = safe_strdup((const char*)sqlite3_column_text(stmt, 2));
        p->status = string_to_status((const char*)sqlite3_column_text(stmt, 3));
        p->created_at = sqlite3_column_int64(stmt, 4);
        p->updated_at = sqlite3_column_int64(stmt, 5);
        p->completed_at = sqlite3_column_int64(stmt, 6);

        count++;
    }

    sqlite3_finalize(stmt);
    *out_count = count;
    return PLAN_DB_OK;
}

PlanDbError plan_db_get_active_plan(PlanRecord* out_plan) {
    if (!g_initialized || !out_plan) return PLAN_DB_ERROR_INVALID;

    memset(out_plan, 0, sizeof(PlanRecord));

    const char* sql =
        "SELECT id FROM plans WHERE status = 'active' ORDER BY updated_at DESC LIMIT 1";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return PLAN_DB_ERROR_NOT_FOUND;
    }

    const char* plan_id = (const char*)sqlite3_column_text(stmt, 0);
    char id_copy[64];
    strncpy(id_copy, plan_id, sizeof(id_copy) - 1);
    sqlite3_finalize(stmt);

    return plan_db_get_plan(id_copy, out_plan);
}

// ============================================================================
// TASK OPERATIONS
// ============================================================================

PlanDbError plan_db_add_task(const char* plan_id, const char* description,
                              const char* assigned_agent, int priority,
                              const char* parent_task_id, char* out_id) {
    if (!g_initialized || !plan_id || !description || !out_id) return PLAN_DB_ERROR_INVALID;

    generate_uuid(out_id);

    const char* sql =
        "INSERT INTO tasks (id, plan_id, description, assigned_agent, priority, parent_task_id) "
        "VALUES (?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, out_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, plan_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, assigned_agent, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, priority);
    sqlite3_bind_text(stmt, 6, parent_task_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_CONSTRAINT) return PLAN_DB_ERROR_CONSTRAINT;
    return (rc == SQLITE_DONE) ? PLAN_DB_OK : PLAN_DB_ERROR_IO;
}

PlanDbError plan_db_get_task(const char* task_id, TaskRecord* out_task) {
    if (!g_initialized || !task_id || !out_task) return PLAN_DB_ERROR_INVALID;

    memset(out_task, 0, sizeof(TaskRecord));

    const char* sql =
        "SELECT id, plan_id, parent_task_id, description, assigned_agent, status, "
        "       priority, created_at, started_at, completed_at, output, error, retry_count "
        "FROM tasks WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, task_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return PLAN_DB_ERROR_NOT_FOUND;
    }

    strncpy(out_task->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(out_task->id) - 1);
    strncpy(out_task->plan_id, (const char*)sqlite3_column_text(stmt, 1), sizeof(out_task->plan_id) - 1);

    const char* parent = (const char*)sqlite3_column_text(stmt, 2);
    if (parent) strncpy(out_task->parent_task_id, parent, sizeof(out_task->parent_task_id) - 1);

    out_task->description = safe_strdup((const char*)sqlite3_column_text(stmt, 3));
    out_task->assigned_agent = safe_strdup((const char*)sqlite3_column_text(stmt, 4));
    out_task->status = string_to_task_status((const char*)sqlite3_column_text(stmt, 5));
    out_task->priority = sqlite3_column_int(stmt, 6);
    out_task->created_at = sqlite3_column_int64(stmt, 7);
    out_task->started_at = sqlite3_column_int64(stmt, 8);
    out_task->completed_at = sqlite3_column_int64(stmt, 9);
    out_task->output = safe_strdup((const char*)sqlite3_column_text(stmt, 10));
    out_task->error = safe_strdup((const char*)sqlite3_column_text(stmt, 11));
    out_task->retry_count = sqlite3_column_int(stmt, 12);

    sqlite3_finalize(stmt);
    return PLAN_DB_OK;
}

PlanDbError plan_db_claim_task(const char* task_id, const char* agent) {
    if (!g_initialized || !task_id || !agent) return PLAN_DB_ERROR_INVALID;

    // Atomic claim: only update if status is pending
    const char* sql =
        "UPDATE tasks SET status = 'in_progress', assigned_agent = ?, started_at = strftime('%s','now') "
        "WHERE id = ? AND status = 'pending'";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, agent, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return PLAN_DB_ERROR_IO;
    if (sqlite3_changes(g_db) == 0) return PLAN_DB_ERROR_BUSY; // Already claimed or not found

    return PLAN_DB_OK;
}

PlanDbError plan_db_complete_task(const char* task_id, const char* output) {
    if (!g_initialized || !task_id) return PLAN_DB_ERROR_INVALID;

    const char* sql =
        "UPDATE tasks SET status = 'completed', output = ?, completed_at = strftime('%s','now') "
        "WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, output, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return PLAN_DB_ERROR_IO;
    if (sqlite3_changes(g_db) == 0) return PLAN_DB_ERROR_NOT_FOUND;

    return PLAN_DB_OK;
}

PlanDbError plan_db_fail_task(const char* task_id, const char* error) {
    if (!g_initialized || !task_id) return PLAN_DB_ERROR_INVALID;

    const char* sql =
        "UPDATE tasks SET status = 'failed', error = ?, completed_at = strftime('%s','now'), "
        "retry_count = retry_count + 1 WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, error, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? PLAN_DB_OK : PLAN_DB_ERROR_IO;
}

PlanDbError plan_db_block_task(const char* task_id, const char* blocked_by) {
    if (!g_initialized || !task_id) return PLAN_DB_ERROR_INVALID;

    const char* sql = "UPDATE tasks SET status = 'blocked', error = ? WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    char msg[256];
    snprintf(msg, sizeof(msg), "Blocked by task: %s", blocked_by ? blocked_by : "unknown");
    sqlite3_bind_text(stmt, 1, msg, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? PLAN_DB_OK : PLAN_DB_ERROR_IO;
}

PlanDbError plan_db_get_next_task(const char* plan_id, const char* agent, TaskRecord* out_task) {
    if (!g_initialized || !plan_id || !out_task) return PLAN_DB_ERROR_INVALID;

    memset(out_task, 0, sizeof(TaskRecord));

    // Priority: assigned to me > unassigned, then by priority desc
    const char* sql =
        "SELECT id FROM tasks "
        "WHERE plan_id = ? AND status = 'pending' "
        "ORDER BY (CASE WHEN assigned_agent = ? THEN 0 WHEN assigned_agent IS NULL THEN 1 ELSE 2 END), "
        "         priority DESC, created_at ASC "
        "LIMIT 1";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, plan_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, agent, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return PLAN_DB_ERROR_NOT_FOUND;
    }

    const char* task_id = (const char*)sqlite3_column_text(stmt, 0);
    char id_copy[64];
    strncpy(id_copy, task_id, sizeof(id_copy) - 1);
    sqlite3_finalize(stmt);

    return plan_db_get_task(id_copy, out_task);
}

PlanDbError plan_db_get_tasks(const char* plan_id, int status, TaskRecord** out_tasks) {
    if (!g_initialized || !plan_id || !out_tasks) return PLAN_DB_ERROR_INVALID;

    *out_tasks = NULL;

    char sql[512];
    if (status >= 0) {
        snprintf(sql, sizeof(sql),
            "SELECT id, plan_id, parent_task_id, description, assigned_agent, status, "
            "       priority, created_at, started_at, completed_at, output, error, retry_count "
            "FROM tasks WHERE plan_id = ? AND status = ? ORDER BY priority DESC, created_at ASC");
    } else {
        snprintf(sql, sizeof(sql),
            "SELECT id, plan_id, parent_task_id, description, assigned_agent, status, "
            "       priority, created_at, started_at, completed_at, output, error, retry_count "
            "FROM tasks WHERE plan_id = ? ORDER BY priority DESC, created_at ASC");
    }

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, plan_id, -1, SQLITE_STATIC);
    if (status >= 0) {
        sqlite3_bind_text(stmt, 2, task_status_to_string((TaskDbStatus)status), -1, SQLITE_STATIC);
    }

    TaskRecord* head = NULL;
    TaskRecord* tail = NULL;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        TaskRecord* task = calloc(1, sizeof(TaskRecord));
        if (!task) continue;

        strncpy(task->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(task->id) - 1);
        strncpy(task->plan_id, (const char*)sqlite3_column_text(stmt, 1), sizeof(task->plan_id) - 1);

        const char* parent = (const char*)sqlite3_column_text(stmt, 2);
        if (parent) strncpy(task->parent_task_id, parent, sizeof(task->parent_task_id) - 1);

        task->description = safe_strdup((const char*)sqlite3_column_text(stmt, 3));
        task->assigned_agent = safe_strdup((const char*)sqlite3_column_text(stmt, 4));
        task->status = string_to_task_status((const char*)sqlite3_column_text(stmt, 5));
        task->priority = sqlite3_column_int(stmt, 6);
        task->created_at = sqlite3_column_int64(stmt, 7);
        task->started_at = sqlite3_column_int64(stmt, 8);
        task->completed_at = sqlite3_column_int64(stmt, 9);
        task->output = safe_strdup((const char*)sqlite3_column_text(stmt, 10));
        task->error = safe_strdup((const char*)sqlite3_column_text(stmt, 11));
        task->retry_count = sqlite3_column_int(stmt, 12);

        task->next = NULL;
        if (!head) {
            head = tail = task;
        } else {
            tail->next = task;
            tail = task;
        }
    }

    sqlite3_finalize(stmt);
    *out_tasks = head;
    return PLAN_DB_OK;
}

PlanDbError plan_db_get_subtasks(const char* task_id, TaskRecord** out_tasks) {
    if (!g_initialized || !task_id || !out_tasks) return PLAN_DB_ERROR_INVALID;

    // Get plan_id first
    TaskRecord parent;
    PlanDbError err = plan_db_get_task(task_id, &parent);
    if (err != PLAN_DB_OK) return err;

    *out_tasks = NULL;

    const char* sql =
        "SELECT id, plan_id, parent_task_id, description, assigned_agent, status, "
        "       priority, created_at, started_at, completed_at, output, error, retry_count "
        "FROM tasks WHERE parent_task_id = ? ORDER BY priority DESC, created_at ASC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    task_record_free(&parent);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, task_id, -1, SQLITE_STATIC);

    TaskRecord* head = NULL;
    TaskRecord* tail = NULL;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        TaskRecord* task = calloc(1, sizeof(TaskRecord));
        if (!task) continue;

        strncpy(task->id, (const char*)sqlite3_column_text(stmt, 0), sizeof(task->id) - 1);
        strncpy(task->plan_id, (const char*)sqlite3_column_text(stmt, 1), sizeof(task->plan_id) - 1);

        const char* parent_id = (const char*)sqlite3_column_text(stmt, 2);
        if (parent_id) strncpy(task->parent_task_id, parent_id, sizeof(task->parent_task_id) - 1);

        task->description = safe_strdup((const char*)sqlite3_column_text(stmt, 3));
        task->assigned_agent = safe_strdup((const char*)sqlite3_column_text(stmt, 4));
        task->status = string_to_task_status((const char*)sqlite3_column_text(stmt, 5));
        task->priority = sqlite3_column_int(stmt, 6);
        task->created_at = sqlite3_column_int64(stmt, 7);
        task->started_at = sqlite3_column_int64(stmt, 8);
        task->completed_at = sqlite3_column_int64(stmt, 9);
        task->output = safe_strdup((const char*)sqlite3_column_text(stmt, 10));
        task->error = safe_strdup((const char*)sqlite3_column_text(stmt, 11));
        task->retry_count = sqlite3_column_int(stmt, 12);

        task->next = NULL;
        if (!head) {
            head = tail = task;
        } else {
            tail->next = task;
            tail = task;
        }
    }

    sqlite3_finalize(stmt);
    *out_tasks = head;
    return PLAN_DB_OK;
}

// ============================================================================
// PROGRESS & ANALYTICS
// ============================================================================

PlanDbError plan_db_get_progress(const char* plan_id, PlanProgress* out_progress) {
    if (!g_initialized || !plan_id || !out_progress) return PLAN_DB_ERROR_INVALID;

    memset(out_progress, 0, sizeof(PlanProgress));
    strncpy(out_progress->plan_id, plan_id, sizeof(out_progress->plan_id) - 1);

    const char* sql =
        "SELECT "
        "  COUNT(*) as total, "
        "  SUM(CASE WHEN status = 'pending' THEN 1 ELSE 0 END) as pending, "
        "  SUM(CASE WHEN status = 'in_progress' THEN 1 ELSE 0 END) as in_progress, "
        "  SUM(CASE WHEN status = 'completed' THEN 1 ELSE 0 END) as completed, "
        "  SUM(CASE WHEN status = 'failed' THEN 1 ELSE 0 END) as failed, "
        "  SUM(CASE WHEN status = 'blocked' THEN 1 ELSE 0 END) as blocked "
        "FROM tasks WHERE plan_id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return PLAN_DB_ERROR_IO;

    sqlite3_bind_text(stmt, 1, plan_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        out_progress->total = sqlite3_column_int(stmt, 0);
        out_progress->pending = sqlite3_column_int(stmt, 1);
        out_progress->in_progress = sqlite3_column_int(stmt, 2);
        out_progress->completed = sqlite3_column_int(stmt, 3);
        out_progress->failed = sqlite3_column_int(stmt, 4);
        out_progress->blocked = sqlite3_column_int(stmt, 5);

        if (out_progress->total > 0) {
            out_progress->percent_complete =
                (double)out_progress->completed / out_progress->total * 100.0;
        }
    }

    sqlite3_finalize(stmt);
    return PLAN_DB_OK;
}

bool plan_db_is_plan_complete(const char* plan_id) {
    PlanProgress progress;
    if (plan_db_get_progress(plan_id, &progress) != PLAN_DB_OK) return false;

    // Complete if no pending or in_progress tasks
    return (progress.pending == 0 && progress.in_progress == 0);
}

PlanDbError plan_db_refresh_plan_status(const char* plan_id) {
    if (!g_initialized || !plan_id) return PLAN_DB_ERROR_INVALID;

    PlanProgress progress;
    PlanDbError err = plan_db_get_progress(plan_id, &progress);
    if (err != PLAN_DB_OK) return err;

    PlanStatus new_status;
    if (progress.total == 0) {
        new_status = PLAN_STATUS_PENDING;
    } else if (progress.pending == 0 && progress.in_progress == 0) {
        // All done
        new_status = (progress.failed > 0) ? PLAN_STATUS_FAILED : PLAN_STATUS_COMPLETED;
    } else if (progress.in_progress > 0 || progress.completed > 0) {
        new_status = PLAN_STATUS_ACTIVE;
    } else {
        new_status = PLAN_STATUS_PENDING;
    }

    return plan_db_update_plan_status(plan_id, new_status);
}

// ============================================================================
// EXPORT
// ============================================================================

char* plan_db_generate_mermaid(const char* plan_id) {
    if (!g_initialized || !plan_id) return NULL;

    TaskRecord* tasks = NULL;
    if (plan_db_get_tasks(plan_id, -1, &tasks) != PLAN_DB_OK) return NULL;

    // Estimate buffer size
    size_t buf_size = 4096;
    char* buf = malloc(buf_size);
    if (!buf) {
        task_record_free_list(tasks);
        return NULL;
    }

    size_t pos = 0;
    pos += snprintf(buf + pos, buf_size - pos,
        "gantt\n"
        "    title Execution Plan Progress\n"
        "    dateFormat X\n"
        "    axisFormat %%H:%%M\n\n");

    for (TaskRecord* t = tasks; t; t = t->next) {
        const char* status_class;
        switch (t->status) {
            case TASK_DB_STATUS_COMPLETED: status_class = "done"; break;
            case TASK_DB_STATUS_IN_PROGRESS: status_class = "active"; break;
            case TASK_DB_STATUS_FAILED: status_class = "crit"; break;
            default: status_class = ""; break;
        }

        // Truncate description for display
        char short_desc[50];
        strncpy(short_desc, t->description ? t->description : "Task", sizeof(short_desc) - 1);
        short_desc[sizeof(short_desc) - 1] = '\0';
        // Replace special chars
        for (char* p = short_desc; *p; p++) {
            if (*p == ':' || *p == '\n' || *p == '\r') *p = ' ';
        }

        time_t start = t->started_at ? t->started_at : t->created_at;
        time_t end = t->completed_at ? t->completed_at : (start + 60); // Default 1 min

        if (pos < buf_size - 200) {
            pos += snprintf(buf + pos, buf_size - pos,
                "    %s :%s, %ld, %ld\n",
                short_desc, status_class, start, end);
        }
    }

    task_record_free_list(tasks);
    return buf;
}

PlanDbError plan_db_export_markdown(const char* plan_id, const char* out_path,
                                     bool include_mermaid) {
    if (!g_initialized || !plan_id || !out_path) return PLAN_DB_ERROR_INVALID;

    PlanRecord plan;
    PlanDbError err = plan_db_get_plan(plan_id, &plan);
    if (err != PLAN_DB_OK) return err;

    PlanProgress progress;
    plan_db_get_progress(plan_id, &progress);

    FILE* f = fopen(out_path, "w");
    if (!f) {
        plan_record_free(&plan);
        return PLAN_DB_ERROR_IO;
    }

    // Header
    fprintf(f, "# %s\n\n", plan.description ? plan.description : "Execution Plan");

    // Metadata
    char time_str[64];
    struct tm* tm_info = localtime(&plan.created_at);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(f, "**Created:** %s  \n", time_str);
    fprintf(f, "**Status:** %s  \n", status_to_string(plan.status));
    fprintf(f, "**ID:** `%s`\n\n", plan.id);

    // Progress bar (ASCII)
    fprintf(f, "## Progress\n\n");
    int bar_width = 20;
    int filled = (int)(progress.percent_complete / 100.0 * bar_width);
    fprintf(f, "```\n[");
    for (int i = 0; i < bar_width; i++) {
        fprintf(f, "%c", i < filled ? '#' : ' ');
    }
    fprintf(f, "] %.1f%% (%d/%d)\n```\n\n", progress.percent_complete,
            progress.completed, progress.total);

    fprintf(f, "- Pending: %d\n", progress.pending);
    fprintf(f, "- In Progress: %d\n", progress.in_progress);
    fprintf(f, "- Completed: %d\n", progress.completed);
    fprintf(f, "- Failed: %d\n", progress.failed);
    fprintf(f, "- Blocked: %d\n\n", progress.blocked);

    // Mermaid diagram
    if (include_mermaid) {
        char* mermaid = plan_db_generate_mermaid(plan_id);
        if (mermaid) {
            fprintf(f, "## Timeline\n\n```mermaid\n%s```\n\n", mermaid);
            free(mermaid);
        }
    }

    // Tasks
    fprintf(f, "## Tasks\n\n");

    TaskRecord* tasks = NULL;
    plan_db_get_tasks(plan_id, -1, &tasks);

    for (TaskRecord* t = tasks; t; t = t->next) {
        const char* emoji;
        switch (t->status) {
            case TASK_DB_STATUS_COMPLETED: emoji = "\xE2\x9C\x85"; break; // ✅
            case TASK_DB_STATUS_IN_PROGRESS: emoji = "\xF0\x9F\x94\x84"; break; // 🔄
            case TASK_DB_STATUS_FAILED: emoji = "\xE2\x9D\x8C"; break; // ❌
            case TASK_DB_STATUS_BLOCKED: emoji = "\xF0\x9F\x9A\xA7"; break; // 🚧
            default: emoji = "\xE2\x8F\xB3"; break; // ⏳
        }

        fprintf(f, "- %s **%s**", emoji, t->description ? t->description : "Task");
        if (t->assigned_agent) {
            fprintf(f, " → @%s", t->assigned_agent);
        }
        fprintf(f, "\n");

        if (t->output && t->status == TASK_DB_STATUS_COMPLETED) {
            fprintf(f, "  - Output: %s\n", t->output);
        }
        if (t->error && t->status == TASK_DB_STATUS_FAILED) {
            fprintf(f, "  - Error: %s\n", t->error);
        }
    }

    task_record_free_list(tasks);
    plan_record_free(&plan);
    fclose(f);

    return PLAN_DB_OK;
}

PlanDbError plan_db_export_json(const char* plan_id, char** out_json) {
    if (!g_initialized || !plan_id || !out_json) return PLAN_DB_ERROR_INVALID;

    PlanRecord plan;
    PlanDbError err = plan_db_get_plan(plan_id, &plan);
    if (err != PLAN_DB_OK) return err;

    PlanProgress progress;
    plan_db_get_progress(plan_id, &progress);

    TaskRecord* tasks = NULL;
    plan_db_get_tasks(plan_id, -1, &tasks);

    // Build JSON manually (or use cJSON if linked)
    size_t buf_size = 16384;
    char* buf = malloc(buf_size);
    if (!buf) {
        plan_record_free(&plan);
        task_record_free_list(tasks);
        return PLAN_DB_ERROR_IO;
    }

    // Escape user-provided strings for JSON safety
    char* escaped_desc = json_escape_string(plan.description);

    size_t pos = 0;
    pos += snprintf(buf + pos, buf_size - pos,
        "{\n"
        "  \"id\": \"%s\",\n"
        "  \"description\": \"%s\",\n"
        "  \"status\": \"%s\",\n"
        "  \"created_at\": %ld,\n"
        "  \"progress\": {\n"
        "    \"total\": %d,\n"
        "    \"completed\": %d,\n"
        "    \"percent\": %.1f\n"
        "  },\n"
        "  \"tasks\": [\n",
        plan.id,
        escaped_desc,
        status_to_string(plan.status),
        plan.created_at,
        progress.total,
        progress.completed,
        progress.percent_complete);

    free(escaped_desc);

    bool first = true;
    for (TaskRecord* t = tasks; t; t = t->next) {
        if (!first) pos += snprintf(buf + pos, buf_size - pos, ",\n");
        first = false;

        char* task_desc = json_escape_string(t->description);
        char* task_agent = json_escape_string(t->assigned_agent);

        pos += snprintf(buf + pos, buf_size - pos,
            "    {\n"
            "      \"id\": \"%s\",\n"
            "      \"description\": \"%s\",\n"
            "      \"status\": \"%s\",\n"
            "      \"agent\": \"%s\",\n"
            "      \"priority\": %d\n"
            "    }",
            t->id,
            task_desc,
            task_status_to_string(t->status),
            task_agent,
            t->priority);

        free(task_desc);
        free(task_agent);
    }

    pos += snprintf(buf + pos, buf_size - pos, "\n  ]\n}\n");

    task_record_free_list(tasks);
    plan_record_free(&plan);
    *out_json = buf;

    return PLAN_DB_OK;
}

// ============================================================================
// MAINTENANCE
// ============================================================================

int plan_db_cleanup_old(int days, int status) {
    if (!g_initialized || days < 0) return 0;

    char sql[256];
    if (status >= 0) {
        snprintf(sql, sizeof(sql),
            "DELETE FROM plans WHERE created_at < strftime('%%s','now','-%d days') AND status = ?",
            days);
    } else {
        snprintf(sql, sizeof(sql),
            "DELETE FROM plans WHERE created_at < strftime('%%s','now','-%d days')", days);
    }

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return 0;

    if (status >= 0) {
        sqlite3_bind_text(stmt, 1, status_to_string((PlanStatus)status), -1, SQLITE_STATIC);
    }

    rc = sqlite3_step(stmt);
    int deleted = sqlite3_changes(g_db);
    sqlite3_finalize(stmt);

    return deleted;
}

PlanDbError plan_db_vacuum(void) {
    if (!g_initialized) return PLAN_DB_ERROR_INVALID;

    char* err_msg = NULL;
    int rc = sqlite3_exec(g_db, "VACUUM", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        return PLAN_DB_ERROR_IO;
    }
    return PLAN_DB_OK;
}

char* plan_db_stats_json(void) {
    if (!g_initialized) return strdup("{\"error\": \"not initialized\"}");

    const char* sql =
        "SELECT "
        "  (SELECT COUNT(*) FROM plans) as total_plans, "
        "  (SELECT COUNT(*) FROM plans WHERE status = 'active') as active_plans, "
        "  (SELECT COUNT(*) FROM tasks) as total_tasks, "
        "  (SELECT COUNT(*) FROM tasks WHERE status = 'completed') as completed_tasks";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return strdup("{\"error\": \"query failed\"}");

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return strdup("{\"error\": \"no data\"}");
    }

    char* buf = malloc(256);
    snprintf(buf, 256,
        "{\n"
        "  \"total_plans\": %d,\n"
        "  \"active_plans\": %d,\n"
        "  \"total_tasks\": %d,\n"
        "  \"completed_tasks\": %d,\n"
        "  \"db_path\": \"%s\"\n"
        "}",
        sqlite3_column_int(stmt, 0),
        sqlite3_column_int(stmt, 1),
        sqlite3_column_int(stmt, 2),
        sqlite3_column_int(stmt, 3),
        g_db_path);

    sqlite3_finalize(stmt);
    return buf;
}

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

void plan_record_free(PlanRecord* record) {
    if (!record) return;
    free(record->description);
    free(record->context);
    record->description = NULL;
    record->context = NULL;
}

void task_record_free(TaskRecord* record) {
    if (!record) return;
    free(record->description);
    free(record->assigned_agent);
    free(record->output);
    free(record->error);
    record->description = NULL;
    record->assigned_agent = NULL;
    record->output = NULL;
    record->error = NULL;
}

void task_record_free_list(TaskRecord* head) {
    while (head) {
        TaskRecord* next = head->next;
        task_record_free(head);
        free(head);
        head = next;
    }
}
