/**
 * CONVERGIO PLAN DATABASE
 *
 * SQLite-backed persistent execution plans with thread-safe access.
 * Enables multi-agent coordination with audit trail and progress tracking.
 *
 * Features:
 * - ACID transactions for plan/task updates
 * - WAL mode for concurrent read/write
 * - Automatic cleanup of old plans
 * - Export to Markdown with Mermaid diagrams
 * - Real-time progress queries
 */

#ifndef CONVERGIO_PLAN_DB_H
#define CONVERGIO_PLAN_DB_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <sqlite3.h>

// ============================================================================
// TYPES
// ============================================================================

typedef enum {
    PLAN_STATUS_PENDING = 0,
    PLAN_STATUS_ACTIVE,
    PLAN_STATUS_COMPLETED,
    PLAN_STATUS_FAILED,
    PLAN_STATUS_CANCELLED
} PlanStatus;

typedef enum {
    TASK_DB_STATUS_PENDING = 0,
    TASK_DB_STATUS_IN_PROGRESS,
    TASK_DB_STATUS_COMPLETED,
    TASK_DB_STATUS_FAILED,
    TASK_DB_STATUS_BLOCKED,
    TASK_DB_STATUS_SKIPPED
} TaskDbStatus;

typedef enum {
    PLAN_DB_OK = 0,
    PLAN_DB_ERROR_INIT,
    PLAN_DB_ERROR_NOT_FOUND,
    PLAN_DB_ERROR_CONSTRAINT,
    PLAN_DB_ERROR_BUSY,
    PLAN_DB_ERROR_IO,
    PLAN_DB_ERROR_INVALID
} PlanDbError;

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Execution Plan - High-level goal with associated tasks
 */
typedef struct {
    char id[64];              // UUID
    char* description;        // Human-readable goal
    char* context;            // Additional context/notes
    PlanStatus status;
    time_t created_at;
    time_t updated_at;
    time_t completed_at;
    int total_tasks;
    int completed_tasks;
    int failed_tasks;
    double progress_percent;
} PlanRecord;

/**
 * Task - Individual unit of work within a plan
 */
typedef struct TaskRecord {
    char id[64];              // UUID
    char plan_id[64];         // Parent plan
    char parent_task_id[64];  // For subtasks (empty if root)
    char* description;
    char* assigned_agent;     // Agent name/ID
    TaskDbStatus status;
    int priority;             // 0-100, higher = more important
    time_t created_at;
    time_t started_at;
    time_t completed_at;
    char* output;             // Result/notes from execution
    char* error;              // Error message if failed
    int retry_count;
    struct TaskRecord* next;  // For linked list queries
} TaskRecord;

/**
 * Plan progress summary
 */
typedef struct {
    char plan_id[64];
    int total;
    int pending;
    int in_progress;
    int completed;
    int failed;
    int blocked;
    double percent_complete;
    time_t estimated_completion;  // 0 if cannot estimate
} PlanProgress;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize the plan database
 * Creates schema if not exists, enables WAL mode
 *
 * @param db_path  Path to SQLite database (NULL for default ~/.convergio/plans.db)
 * @return         PLAN_DB_OK on success
 */
PlanDbError plan_db_init(const char* db_path);

/**
 * Shutdown and close database connection
 */
void plan_db_shutdown(void);

/**
 * Check if database is initialized
 */
bool plan_db_is_ready(void);

/**
 * Get raw SQLite handle (for advanced queries)
 * WARNING: Use with caution, prefer API functions
 */
sqlite3* plan_db_get_handle(void);

// ============================================================================
// PLAN OPERATIONS (CRUD)
// ============================================================================

/**
 * Create a new execution plan
 *
 * @param description  Human-readable goal
 * @param context      Optional additional context (can be NULL)
 * @param out_id       Output buffer for generated UUID (min 64 chars)
 * @return             PLAN_DB_OK on success
 */
PlanDbError plan_db_create_plan(const char* description, const char* context, char* out_id);

/**
 * Get plan by ID
 *
 * @param plan_id  Plan UUID
 * @param out_plan Output structure (caller must free strings with plan_record_free)
 * @return         PLAN_DB_OK on success, PLAN_DB_ERROR_NOT_FOUND if not exists
 */
PlanDbError plan_db_get_plan(const char* plan_id, PlanRecord* out_plan);

/**
 * Update plan status
 *
 * @param plan_id  Plan UUID
 * @param status   New status
 * @return         PLAN_DB_OK on success
 */
PlanDbError plan_db_update_plan_status(const char* plan_id, PlanStatus status);

/**
 * Delete plan and all associated tasks
 *
 * @param plan_id  Plan UUID
 * @return         PLAN_DB_OK on success
 */
PlanDbError plan_db_delete_plan(const char* plan_id);

/**
 * List all plans with optional filtering
 *
 * @param status      Filter by status (-1 for all)
 * @param limit       Maximum plans to return (0 for no limit)
 * @param offset      Offset for pagination
 * @param out_plans   Output array (caller allocates)
 * @param out_count   Number of plans returned
 * @return            PLAN_DB_OK on success
 */
PlanDbError plan_db_list_plans(int status, int limit, int offset,
                                PlanRecord* out_plans, int max_plans, int* out_count);

/**
 * Get active plan (most recent ACTIVE status)
 *
 * @param out_plan  Output structure
 * @return          PLAN_DB_OK on success, PLAN_DB_ERROR_NOT_FOUND if none active
 */
PlanDbError plan_db_get_active_plan(PlanRecord* out_plan);

// ============================================================================
// TASK OPERATIONS
// ============================================================================

/**
 * Add task to a plan
 *
 * @param plan_id        Parent plan UUID
 * @param description    Task description
 * @param assigned_agent Agent name (can be NULL for unassigned)
 * @param priority       Priority 0-100
 * @param parent_task_id Parent task for subtasks (NULL for root tasks)
 * @param out_id         Output buffer for generated UUID
 * @return               PLAN_DB_OK on success
 */
PlanDbError plan_db_add_task(const char* plan_id, const char* description,
                              const char* assigned_agent, int priority,
                              const char* parent_task_id, char* out_id);

/**
 * Get task by ID
 *
 * @param task_id   Task UUID
 * @param out_task  Output structure
 * @return          PLAN_DB_OK on success
 */
PlanDbError plan_db_get_task(const char* task_id, TaskRecord* out_task);

/**
 * Claim a task for execution (atomic status update)
 * Sets status to IN_PROGRESS and assigns agent
 *
 * @param task_id  Task UUID
 * @param agent    Agent claiming the task
 * @return         PLAN_DB_OK on success, PLAN_DB_ERROR_BUSY if already claimed
 */
PlanDbError plan_db_claim_task(const char* task_id, const char* agent);

/**
 * Complete a task with result
 *
 * @param task_id  Task UUID
 * @param output   Result/output from execution
 * @return         PLAN_DB_OK on success
 */
PlanDbError plan_db_complete_task(const char* task_id, const char* output);

/**
 * Fail a task with error
 *
 * @param task_id  Task UUID
 * @param error    Error message
 * @return         PLAN_DB_OK on success
 */
PlanDbError plan_db_fail_task(const char* task_id, const char* error);

/**
 * Block a task (waiting on dependency)
 *
 * @param task_id       Task UUID
 * @param blocked_by    ID of blocking task
 * @return              PLAN_DB_OK on success
 */
PlanDbError plan_db_block_task(const char* task_id, const char* blocked_by);

/**
 * Get next pending task for an agent
 * Returns highest priority unassigned or assigned-to-me task
 *
 * @param plan_id   Plan UUID
 * @param agent     Agent name (for assignment preference)
 * @param out_task  Output structure
 * @return          PLAN_DB_OK on success, PLAN_DB_ERROR_NOT_FOUND if none available
 */
PlanDbError plan_db_get_next_task(const char* plan_id, const char* agent, TaskRecord* out_task);

/**
 * Get all tasks for a plan
 *
 * @param plan_id    Plan UUID
 * @param status     Filter by status (-1 for all)
 * @param out_tasks  Linked list head (caller must free with task_record_free_list)
 * @return           PLAN_DB_OK on success
 */
PlanDbError plan_db_get_tasks(const char* plan_id, int status, TaskRecord** out_tasks);

/**
 * Get subtasks of a task
 */
PlanDbError plan_db_get_subtasks(const char* task_id, TaskRecord** out_tasks);

// ============================================================================
// PROGRESS & ANALYTICS
// ============================================================================

/**
 * Get progress summary for a plan
 */
PlanDbError plan_db_get_progress(const char* plan_id, PlanProgress* out_progress);

/**
 * Check if plan is complete (all tasks done or failed)
 */
bool plan_db_is_plan_complete(const char* plan_id);

/**
 * Auto-update plan status based on task completion
 * Call after completing/failing tasks
 */
PlanDbError plan_db_refresh_plan_status(const char* plan_id);

// ============================================================================
// EXPORT
// ============================================================================

/**
 * Export plan to Markdown with Mermaid diagram
 *
 * @param plan_id    Plan UUID
 * @param out_path   Output file path (caller provides)
 * @param include_mermaid  Include Mermaid timeline diagram
 * @return           PLAN_DB_OK on success
 */
PlanDbError plan_db_export_markdown(const char* plan_id, const char* out_path,
                                     bool include_mermaid);

/**
 * Export plan to JSON
 */
PlanDbError plan_db_export_json(const char* plan_id, char** out_json);

/**
 * Generate Mermaid gantt chart for plan
 */
char* plan_db_generate_mermaid(const char* plan_id);

// ============================================================================
// MAINTENANCE
// ============================================================================

/**
 * Delete plans older than specified days
 *
 * @param days       Age threshold
 * @param status     Only delete plans with this status (-1 for all)
 * @return           Number of plans deleted
 */
int plan_db_cleanup_old(int days, int status);

/**
 * Vacuum database to reclaim space
 */
PlanDbError plan_db_vacuum(void);

/**
 * Get database statistics
 */
char* plan_db_stats_json(void);

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

/**
 * Free strings in PlanRecord
 */
void plan_record_free(PlanRecord* record);

/**
 * Free TaskRecord and its strings
 */
void task_record_free(TaskRecord* record);

/**
 * Free linked list of TaskRecords
 */
void task_record_free_list(TaskRecord* head);

// ============================================================================
// THREAD SAFETY NOTES
// ============================================================================

/*
 * All functions are thread-safe due to:
 * 1. SQLite WAL mode enabling concurrent readers
 * 2. IMMEDIATE transactions for writes
 * 3. BUSY timeout handling with exponential backoff
 *
 * For high-contention scenarios, use plan_db_claim_task() which
 * atomically checks and updates task status.
 *
 * The database uses advisory locking via SQLite's built-in mechanisms.
 * No external file locking is required.
 */

#endif // CONVERGIO_PLAN_DB_H
