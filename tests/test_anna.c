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

#include "nous/nous.h"  // For LogLevel, LogCategory
#include "nous/todo.h"
#include "nous/notify.h"
#include "nous/mcp_client.h"

// Log stubs are provided by test_stubs.c (linked via Makefile)

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
        "  scheduled_at TEXT NOT NULL,"
        "  method INTEGER DEFAULT 0,"
        "  status INTEGER DEFAULT 0,"
        "  retry_count INTEGER DEFAULT 0,"
        "  max_retries INTEGER DEFAULT 3,"
        "  last_error TEXT,"
        "  sent_at TEXT,"
        "  acknowledged_at TEXT"
        ");"
        "CREATE VIRTUAL TABLE IF NOT EXISTS tasks_fts USING fts5("
        "  title, description, tags, context, content='tasks', content_rowid='id'"
        ");"
        "CREATE TRIGGER IF NOT EXISTS tasks_fts_insert AFTER INSERT ON tasks BEGIN "
        "  INSERT INTO tasks_fts(rowid, title, description, tags, context) "
        "  VALUES (new.id, new.title, new.description, new.tags, new.context); "
        "END;"
        "CREATE TRIGGER IF NOT EXISTS tasks_fts_delete AFTER DELETE ON tasks BEGIN "
        "  INSERT INTO tasks_fts(tasks_fts, rowid, title, description, tags, context) "
        "  VALUES('delete', old.id, old.title, old.description, old.tags, old.context); "
        "END;"
        "CREATE TRIGGER IF NOT EXISTS tasks_fts_update AFTER UPDATE ON tasks BEGIN "
        "  INSERT INTO tasks_fts(tasks_fts, rowid, title, description, tags, context) "
        "  VALUES('delete', old.id, old.title, old.description, old.tags, old.context); "
        "  INSERT INTO tasks_fts(rowid, title, description, tags, context) "
        "  VALUES (new.id, new.title, new.description, new.tags, new.context); "
        "END;";

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

static void reset_tables(void) {
    const char* reset_sql =
        "DELETE FROM tasks; DELETE FROM tasks_fts; DELETE FROM inbox; DELETE FROM notification_queue;";
    sqlite3_exec(g_db, reset_sql, NULL, NULL, NULL);
    todo_shutdown();
    todo_init();
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

void test_iso8601_parsing(void) {
    TEST_SECTION("ISO8601 Parsing Tests");

    struct tm tm = {0};
    time_t parsed = 0;

    tm.tm_year = 2024 - 1900;
    tm.tm_mon = 4;
    tm.tm_mday = 10;
    tm.tm_hour = 12;
    tm.tm_min = 30;
    tm.tm_sec = 45;
    time_t expected_utc = timegm(&tm);

    TEST("Parse full timestamp with 'T' and Z", todo_parse_iso8601("2024-05-10T12:30:45Z", &parsed) == TODO_ISO8601_OK && parsed == expected_utc);

    tm.tm_hour = 10;
    tm.tm_min = 30;
    tm.tm_sec = 45;
    time_t expected_offset = timegm(&tm);
    TEST("Parse timestamp with timezone offset", todo_parse_iso8601("2024-05-10 12:30:45+02:00", &parsed) == TODO_ISO8601_OK && parsed == expected_offset);

    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    time_t expected_date_only = timegm(&tm);
    TEST("Parse date-only input", todo_parse_iso8601("2024-05-10", &parsed) == TODO_ISO8601_OK && parsed == expected_date_only);

    parsed = 123;
    TEST("Reject invalid timestamp", todo_parse_iso8601("2024-99-99T00:00:00", &parsed) == TODO_ISO8601_INVALID && parsed == 0);

    parsed = 123;
    TEST("Handle empty input gracefully", todo_parse_iso8601("", &parsed) == TODO_ISO8601_EMPTY && parsed == 0);
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

void test_todo_queries_and_search(void) {
    TEST_SECTION("Todo List/FTS Query Tests");

    reset_tables();

    time_t now = time(NULL);

    TodoCreateOptions opts = {0};
    opts.title = "Urgent Today";
    opts.priority = TODO_PRIORITY_URGENT;
    opts.due_date = now;
    todo_create(&opts);

    opts.title = "Normal No Due";
    opts.priority = TODO_PRIORITY_NORMAL;
    opts.due_date = 0;
    todo_create(&opts);

    opts.title = "Low Overdue";
    opts.priority = TODO_PRIORITY_LOW;
    opts.due_date = now - 86400;
    todo_create(&opts);

    opts.title = "Future Task";
    opts.priority = TODO_PRIORITY_NORMAL;
    opts.due_date = now + 86400;
    todo_create(&opts);

    int count = 0;
    TodoTask** today = todo_list_today(&count);
    TEST("list_today returns only current tasks", today && count == 3);
    if (today && count == 3) {
        TEST("list_today orders by priority then due", today[0]->priority == TODO_PRIORITY_URGENT &&
             today[1]->priority == TODO_PRIORITY_NORMAL && today[2]->priority == TODO_PRIORITY_LOW &&
             today[2]->due_date < now);
        todo_free_tasks(today, count);
    }

    TodoTask** overdue = todo_list_overdue(&count);
    TEST("list_overdue surfaces past-due tasks", overdue && count == 1);
    if (overdue && count == 1) {
        TEST("overdue task is older than now", overdue[0]->due_date < now);
        todo_free_tasks(overdue, count);
    }

    // Limit enforcement for list_today (LIMIT 100)
    for (int i = 0; i < 120; i++) {
        char title[32];
        snprintf(title, sizeof(title), "Limit Task %d", i);
        opts.title = title;
        opts.priority = TODO_PRIORITY_NORMAL;
        opts.due_date = now;
        todo_create(&opts);
    }

    TodoTask** limited = todo_list_today(&count);
    TEST("list_today respects 100 row cap", limited && count == 100);
    if (limited) {
        todo_free_tasks(limited, count);
    }

    // FTS search ordering/limit
    reset_tables();
    for (int i = 0; i < 60; i++) {
        char title[48];
        snprintf(title, sizeof(title), "Searchable Task %d", i);
        opts.title = title;
        opts.description = "needle in haystack";
        opts.priority = TODO_PRIORITY_NORMAL;
        opts.due_date = now;
        todo_create(&opts);
    }

    TodoTask** search_results = todo_search("needle", &count);
    TEST("FTS search returns capped results", search_results && count == 50);
    if (search_results) {
        TEST("FTS search keeps pending status only", search_results[0]->status == TODO_STATUS_PENDING);
        todo_free_tasks(search_results, count);
    }

    todo_shutdown();
}

void test_todo_stats_and_error_handling(void) {
    TEST_SECTION("Todo Stats and SQLite Error Handling");

    reset_tables();

    time_t now = time(NULL);

    TodoCreateOptions opts = {0};
    opts.title = "Pending";
    opts.priority = TODO_PRIORITY_NORMAL;
    opts.due_date = now - 3600;
    todo_create(&opts);

    opts.title = "In Progress";
    int64_t in_progress_id = todo_create(&opts);
    todo_start(in_progress_id);

    opts.title = "Completed";
    int64_t completed_id = todo_create(&opts);
    todo_complete(completed_id);

    inbox_capture("Process me", "cli");

    TodoStats stats = todo_get_stats();
    TEST("Stats counts pending", stats.total_pending == 1);
    TEST("Stats counts in-progress", stats.total_in_progress == 1);
    TEST("Stats counts completed today", stats.total_completed_today >= 1);
    TEST("Stats counts overdue", stats.total_overdue == 2);
    TEST("Stats counts inbox backlog", stats.inbox_unprocessed == 1);

    // Simulate missing prepared statement and ensure graceful recovery
    todo_invalidate_stats_statement();
    TodoStats stats_after = todo_get_stats();
    TEST("Stats recompute after invalidation", stats_after.total_pending == stats.total_pending);
    TEST("Stats overdue persists after reprepare", stats_after.total_overdue == stats.total_overdue);

    todo_shutdown();
}

void test_notifications_and_mcp_integration(void) {
    TEST_SECTION("Notification Queue and MCP Integration");

    reset_tables();

    time_t now = time(NULL);
    int64_t notif1 = notify_schedule(1, now + 60, NOTIFY_METHOD_TERMINAL);
    int64_t notif2 = notify_schedule(2, now + 120, NOTIFY_METHOD_LOG);

    TEST("Notification scheduling returns IDs", notif1 > 0 && notif2 > 0);

    int ncount = 0;
    ScheduledNotification** pending = notify_list_pending(&ncount);
    TEST("Pending notification list populated", pending && ncount == 2);
    if (pending && ncount == 2) {
        TEST("Notifications ordered by schedule", pending[0]->scheduled_at <= pending[1]->scheduled_at);
    }

    notify_snooze_for(notif1, 5);
    ScheduledNotification* snoozed = notify_get(notif1);
    TEST("Snoozed notification updates status", snoozed && snoozed->status == NOTIFY_STATUS_SNOOZED);
    notify_free(snoozed);

    notify_cancel(notif2);
    ScheduledNotification* cancelled = notify_get(notif2);
    TEST("Cancelled notification removed", cancelled == NULL);

    if (pending) {
        notify_free_list(pending, ncount);
    }

    // MCP network failure simulation
    mcp_init();
    MCPServerConfig cfg = {0};
    cfg.name = "netfail";
    cfg.enabled = true;
    cfg.transport = MCP_TRANSPORT_HTTP;
    cfg.url = "http://127.0.0.1:9";  // closed port to force connection failure
    cfg.timeout_ms = 200;

    int add_rc = mcp_add_server(&cfg);
    TEST("Added MCP server config", add_rc == 0);

    int connect_rc = mcp_connect("netfail");
    TEST("MCP connect fails fast on network error", connect_rc != MCP_OK);

    const char* last_err = mcp_get_last_error("netfail");
    TEST("MCP records last error", last_err != NULL && strlen(last_err) > 0);

    mcp_shutdown();
}

// Note: Notification and MCP tests are now integrated in test_notifications_and_mcp_integration()
// Phase 2 (Notifications) and Phase 3 (MCP Client) are fully implemented in v4.2.0

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
    test_iso8601_parsing();
    test_todo_priority_parsing();
    test_todo_crud_operations();
    test_todo_list_operations();
    test_todo_inbox();
    test_todo_queries_and_search();
    test_todo_stats_and_error_handling();
    test_notifications_and_mcp_integration();

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
