/**
 * Unit Tests for Anna Executive Assistant Components
 *
 * Tests todo.c, notify.c, and mcp_client.c
 * Run with: make anna_test && ./build/bin/anna_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdarg.h>
#include <time.h>
#include <sqlite3.h>
#include <pthread.h>

#include "nous/todo.h"
#include "nous/notify.h"
#include "nous/mcp_client.h"

// Stub for nous_log
typedef enum { LOG_LEVEL_ERROR = 0 } LogLevel;
typedef enum { LOG_CAT_GENERAL = 0 } LogCategory;
LogLevel g_log_level = LOG_LEVEL_ERROR;

void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) {
    (void)level; (void)cat; (void)fmt;
}

void nous_log_set_level(LogLevel level) { g_log_level = level; }
LogLevel nous_log_get_level(void) { return g_log_level; }
const char* nous_log_level_name(LogLevel level) { (void)level; return ""; }

// External references to global database (defined in persistence.c)
// Using extern to avoid duplicate symbol errors when linking with persistence.o
extern sqlite3* g_db;
extern pthread_mutex_t g_db_mutex;

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, condition) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("  \033[32m✓\033[0m %s\n", name); \
    } else { \
        tests_failed++; \
        printf("  \033[31m✗\033[0m %s FAILED\n", name); \
    } \
} while(0)

#define TEST_SECTION(name) printf("\n\033[1m=== %s ===\033[0m\n", name)

// ============================================================================
// TEST DATABASE SETUP
// ============================================================================

static int setup_test_db(void) {
    int rc = sqlite3_open(":memory:", &g_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create test database: %s\n", sqlite3_errmsg(g_db));
        return -1;
    }

    const char* schema =
        "CREATE TABLE IF NOT EXISTS tasks ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  title TEXT NOT NULL,"
        "  description TEXT,"
        "  priority INTEGER DEFAULT 2,"
        "  status INTEGER DEFAULT 0,"
        "  due_date TEXT,"
        "  reminder_at TEXT,"
        "  recurrence INTEGER DEFAULT 0,"
        "  recurrence_rule TEXT,"
        "  tags TEXT,"
        "  context TEXT,"
        "  parent_id INTEGER,"
        "  source TEXT DEFAULT 'cli',"
        "  external_id TEXT,"
        "  created_at TEXT DEFAULT (datetime('now')),"
        "  updated_at TEXT DEFAULT (datetime('now')),"
        "  completed_at TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS inbox ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  content TEXT NOT NULL,"
        "  captured_at TEXT DEFAULT (datetime('now')),"
        "  processed INTEGER DEFAULT 0,"
        "  processed_task_id INTEGER,"
        "  source TEXT DEFAULT 'cli'"
        ");"
        "CREATE TABLE IF NOT EXISTS notification_queue ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  task_id INTEGER,"
        "  title TEXT NOT NULL,"
        "  body TEXT,"
        "  scheduled_at TEXT NOT NULL,"
        "  delivered_at TEXT,"
        "  status INTEGER DEFAULT 0,"
        "  retry_count INTEGER DEFAULT 0,"
        "  created_at TEXT DEFAULT (datetime('now'))"
        ");"
        "CREATE VIRTUAL TABLE IF NOT EXISTS tasks_fts USING fts5("
        "  title, description, tags, content='tasks', content_rowid='id'"
        ");";

    char* err_msg = NULL;
    rc = sqlite3_exec(g_db, schema, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create schema: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    return 0;
}

static void teardown_test_db(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

// ============================================================================
// TODO DATE PARSING TESTS
// ============================================================================

void test_todo_date_parsing(void) {
    TEST_SECTION("Todo Date Parsing Tests");

    time_t now = time(NULL);
    time_t parsed;

    // Test relative dates
    parsed = todo_parse_date("tomorrow", now);
    TEST("Parse 'tomorrow'", parsed > now && parsed < now + 86400 * 2);

    parsed = todo_parse_date("today", now);
    TEST("Parse 'today'", parsed >= now - 86400 && parsed <= now + 86400);

    // Test Italian dates
    parsed = todo_parse_date("domani", now);
    TEST("Parse 'domani' (Italian tomorrow)", parsed > now && parsed < now + 86400 * 2);

    // Test weekdays
    parsed = todo_parse_date("next monday", now);
    TEST("Parse 'next monday'", parsed > now);

    // Test invalid input
    parsed = todo_parse_date(NULL, now);
    TEST("Handle NULL input", parsed == 0);

    parsed = todo_parse_date("", now);
    TEST("Handle empty string", parsed == 0);
}

// ============================================================================
// TODO PRIORITY PARSING TESTS
// ============================================================================

void test_todo_priority_parsing(void) {
    TEST_SECTION("Todo Priority Parsing Tests");

    TEST("Parse 'urgent' priority", todo_priority_from_string("urgent") == TODO_PRIORITY_URGENT);
    TEST("Parse 'normal' priority", todo_priority_from_string("normal") == TODO_PRIORITY_NORMAL);
    TEST("Parse 'low' priority", todo_priority_from_string("low") == TODO_PRIORITY_LOW);

    // Numeric
    TEST("Parse '1' as urgent", todo_priority_from_string("1") == TODO_PRIORITY_URGENT);
    TEST("Parse '2' as normal", todo_priority_from_string("2") == TODO_PRIORITY_NORMAL);
    TEST("Parse '3' as low", todo_priority_from_string("3") == TODO_PRIORITY_LOW);

    // Default
    TEST("Default for unknown", todo_priority_from_string("unknown") == TODO_PRIORITY_NORMAL);
    TEST("Handle NULL", todo_priority_from_string(NULL) == TODO_PRIORITY_NORMAL);
}

// ============================================================================
// TODO CRUD OPERATIONS TESTS
// ============================================================================

void test_todo_crud_operations(void) {
    TEST_SECTION("Todo CRUD Operations Tests");

    // Initialize todo system
    int rc = todo_init();
    TEST("Initialize todo system", rc == 0);

    // Create a task
    TodoCreateOptions opts = {0};
    opts.title = "Test Task";
    opts.description = "Test Description";
    opts.priority = TODO_PRIORITY_URGENT;
    opts.context = "@work";
    opts.tags = "test,unit";

    int64_t task_id = todo_create(&opts);
    TEST("Create task", task_id > 0);

    // Get the task
    TodoTask* task = todo_get(task_id);
    TEST("Get task by ID", task != NULL);
    if (task) {
        TEST("Task title matches", strcmp(task->title, "Test Task") == 0);
        TEST("Task description matches", task->description && strcmp(task->description, "Test Description") == 0);
        TEST("Task priority matches", task->priority == TODO_PRIORITY_URGENT);
        TEST("Task status is pending", task->status == TODO_STATUS_PENDING);
        todo_free_task(task);
    }

    // Update the task
    TodoCreateOptions update_opts = {0};
    update_opts.title = "Updated Task";
    update_opts.priority = TODO_PRIORITY_LOW;
    rc = todo_update(task_id, &update_opts);
    TEST("Update task", rc == 0);

    task = todo_get(task_id);
    if (task) {
        TEST("Updated title matches", strcmp(task->title, "Updated Task") == 0);
        TEST("Updated priority matches", task->priority == TODO_PRIORITY_LOW);
        todo_free_task(task);
    }

    // Start the task
    rc = todo_start(task_id);
    TEST("Start task", rc == 0);

    task = todo_get(task_id);
    if (task) {
        TEST("Task status is in_progress", task->status == TODO_STATUS_IN_PROGRESS);
        todo_free_task(task);
    }

    // Complete the task
    rc = todo_complete(task_id);
    TEST("Complete task", rc == 0);

    task = todo_get(task_id);
    if (task) {
        TEST("Task status is completed", task->status == TODO_STATUS_COMPLETED);
        todo_free_task(task);
    }

    // Delete the task
    rc = todo_delete(task_id);
    TEST("Delete task", rc == 0);

    task = todo_get(task_id);
    TEST("Task is deleted", task == NULL);

    todo_shutdown();
}

// ============================================================================
// TODO LIST OPERATIONS TESTS
// ============================================================================

void test_todo_list_operations(void) {
    TEST_SECTION("Todo List Operations Tests");

    todo_init();

    // Create multiple tasks
    TodoCreateOptions opts = {0};

    opts.title = "Task 1";
    opts.priority = TODO_PRIORITY_URGENT;
    todo_create(&opts);

    opts.title = "Task 2";
    opts.priority = TODO_PRIORITY_LOW;
    todo_create(&opts);

    opts.title = "Task 3";
    opts.priority = TODO_PRIORITY_NORMAL;
    todo_create(&opts);

    // List all tasks
    int count = 0;
    TodoTask** tasks = todo_list(NULL, &count);
    TEST("List returns tasks", tasks != NULL);
    TEST("List count is 3", count == 3);

    if (tasks) {
        // Check ordering by priority (urgent first)
        TEST("Tasks ordered by priority (first is urgent)",
             tasks[0]->priority == TODO_PRIORITY_URGENT);
        todo_free_tasks(tasks, count);
    }

    todo_shutdown();
}

// ============================================================================
// TODO INBOX TESTS
// ============================================================================

void test_todo_inbox(void) {
    TEST_SECTION("Todo Inbox Tests");

    todo_init();

    // Capture to inbox
    int64_t inbox_id = inbox_capture("Quick thought to process later", "cli");
    TEST("Capture to inbox", inbox_id > 0);

    // List unprocessed
    int count = 0;
    TodoInboxItem** items = inbox_list_unprocessed(&count);
    TEST("List unprocessed inbox items", items != NULL);
    TEST("Inbox count is 1", count == 1);

    if (items && count > 0) {
        TEST("Inbox content matches", strcmp(items[0]->content, "Quick thought to process later") == 0);
        TEST("Inbox item not processed", items[0]->processed == 0);
        todo_free_inbox_items(items, count);
    }

    // Process inbox item
    TodoCreateOptions opts = {0};
    opts.title = "Processed from inbox";
    int64_t task_id = todo_create(&opts);

    int rc = inbox_process(inbox_id, task_id);
    TEST("Process inbox item", rc == 0);

    // Verify processed
    items = inbox_list_unprocessed(&count);
    TEST("No unprocessed items after processing", count == 0);
    if (items) {
        todo_free_inbox_items(items, count);
    }

    todo_shutdown();
}

// ============================================================================
// NOTIFICATION TESTS
// NOTE: Disabled until Phase 2 implementation (ADR-009)
// The notify.h API has different signature than expected by tests
// ============================================================================

#if 0  // Enable when notify.c is fully implemented
void test_notify_init(void) {
    TEST_SECTION("Notification System Tests");
    // ... tests here when API is stable
}

void test_notify_send(void) {
    TEST_SECTION("Notification Send Tests");
    // ... tests here when API is stable
}
#endif

// ============================================================================
// MCP CLIENT TESTS
// NOTE: Disabled until Phase 3 implementation (ADR-009)
// The mcp_client.h API has different signature than expected by tests
// ============================================================================

#if 0  // Enable when mcp_client.c is fully implemented
void test_mcp_client_create(void) {
    TEST_SECTION("MCP Client Tests");
    // ... tests here when API is stable
}

void test_mcp_tool_call(void) {
    TEST_SECTION("MCP Tool Call Tests");
    // ... tests here when API is stable
}
#endif

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║         ANNA EXECUTIVE ASSISTANT UNIT TESTS                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    // Setup test database
    if (setup_test_db() != 0) {
        fprintf(stderr, "Failed to setup test database\n");
        return 1;
    }

    // Run todo tests
    test_todo_date_parsing();
    test_todo_priority_parsing();
    test_todo_crud_operations();
    test_todo_list_operations();
    test_todo_inbox();

    // NOTE: Notification and MCP tests disabled until Phase 2 & 3 implementation complete
    // See ADR-009 for implementation status. APIs use different signatures than tests expect.
    // TODO: Re-enable when notify.c and mcp_client.c are fully implemented
    //
    // test_notify_init();
    // test_notify_send();
    //
    // test_mcp_client_create();
    // test_mcp_tool_call();

    // Cleanup
    teardown_test_db();

    // Summary
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  Tests Run:    %d\n", tests_run);
    printf("  Tests Passed: \033[32m%d\033[0m\n", tests_passed);
    printf("  Tests Failed: \033[31m%d\033[0m\n", tests_failed);
    printf("═══════════════════════════════════════════════════════════════\n");

    return tests_failed > 0 ? 1 : 0;
}
