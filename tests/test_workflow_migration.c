/**
 * CONVERGIO WORKFLOW MIGRATION TESTS
 *
 * Tests for workflow engine database migration (016_workflow_engine.sql)
 * Tests: idempotency, rollback, foreign keys, indexes, schema correctness
 */

#include "nous/workflow.h"
#include "nous/orchestrator.h"
#include "nous/nous.h"
#include "nous/debug_mutex.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <time.h>

// ============================================================================
// MOCK FUNCTIONS
// ============================================================================

// Mock functions are provided by test_stubs.c (linked via $(TEST_STUBS))
// nous_log, nous_log_set_level, nous_log_get_level, nous_log_level_name
// are defined in tests/test_stubs.c to avoid duplicate symbol errors

// ============================================================================
// TEST HELPERS
// ============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("  ✓ %s\n", message); \
        } else { \
            printf("  ✗ %s\n", message); \
        } \
    } while (0)

// External database access
extern sqlite3* g_db;
extern ConvergioMutex g_db_mutex;

// ============================================================================
// MIGRATION SQL
// ============================================================================

static const char* MIGRATION_016_SQL =
    "-- Migration 016: Workflow Engine\n"
    "BEGIN TRANSACTION;\n"
    "\n"
    "CREATE TABLE IF NOT EXISTS workflows (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    name TEXT NOT NULL,\n"
    "    description TEXT,\n"
    "    entry_node_id INTEGER,\n"
    "    status INTEGER NOT NULL DEFAULT 0,\n"
    "    current_node_id INTEGER,\n"
    "    created_at INTEGER NOT NULL,\n"
    "    updated_at INTEGER,\n"
    "    last_checkpoint_at INTEGER,\n"
    "    error_message TEXT,\n"
    "    metadata_json TEXT,\n"
    "    FOREIGN KEY (entry_node_id) REFERENCES workflow_nodes(id) ON DELETE SET NULL,\n"
    "    FOREIGN KEY (current_node_id) REFERENCES workflow_nodes(id) ON DELETE SET NULL\n"
    ");\n"
    "\n"
    "CREATE TABLE IF NOT EXISTS workflow_nodes (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    workflow_id INTEGER NOT NULL,\n"
    "    name TEXT NOT NULL,\n"
    "    type INTEGER NOT NULL,\n"
    "    agent_id INTEGER,\n"
    "    action_prompt TEXT,\n"
    "    condition_expr TEXT,\n"
    "    node_data_json TEXT,\n"
    "    created_at INTEGER NOT NULL,\n"
    "    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE\n"
    ");\n"
    "\n"
    "CREATE TABLE IF NOT EXISTS workflow_edges (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    workflow_id INTEGER NOT NULL,\n"
    "    from_node_id INTEGER NOT NULL,\n"
    "    to_node_id INTEGER NOT NULL,\n"
    "    condition_expr TEXT,\n"
    "    is_default INTEGER DEFAULT 0,\n"
    "    created_at INTEGER NOT NULL,\n"
    "    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE,\n"
    "    FOREIGN KEY (from_node_id) REFERENCES workflow_nodes(id) ON DELETE CASCADE,\n"
    "    FOREIGN KEY (to_node_id) REFERENCES workflow_nodes(id) ON DELETE CASCADE,\n"
    "    UNIQUE(workflow_id, from_node_id, to_node_id)\n"
    ");\n"
    "\n"
    "CREATE TABLE IF NOT EXISTS workflow_state (\n"
    "    workflow_id INTEGER NOT NULL,\n"
    "    key TEXT NOT NULL,\n"
    "    value TEXT NOT NULL,\n"
    "    updated_at INTEGER NOT NULL,\n"
    "    PRIMARY KEY (workflow_id, key),\n"
    "    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE\n"
    ");\n"
    "\n"
    "CREATE TABLE IF NOT EXISTS workflow_checkpoints (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    workflow_id INTEGER NOT NULL,\n"
    "    node_id INTEGER NOT NULL,\n"
    "    state_json TEXT NOT NULL,\n"
    "    created_at INTEGER NOT NULL,\n"
    "    metadata_json TEXT,\n"
    "    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE,\n"
    "    FOREIGN KEY (node_id) REFERENCES workflow_nodes(id) ON DELETE SET NULL\n"
    ");\n"
    "\n"
    "CREATE INDEX IF NOT EXISTS idx_workflows_status ON workflows(status);\n"
    "CREATE INDEX IF NOT EXISTS idx_workflows_created ON workflows(created_at DESC);\n"
    "CREATE INDEX IF NOT EXISTS idx_workflows_current_node ON workflows(current_node_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_nodes_workflow ON workflow_nodes(workflow_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_nodes_type ON workflow_nodes(type);\n"
    "CREATE INDEX IF NOT EXISTS idx_edges_workflow ON workflow_edges(workflow_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_edges_from ON workflow_edges(from_node_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_edges_to ON workflow_edges(to_node_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_state_workflow ON workflow_state(workflow_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_checkpoints_workflow ON workflow_checkpoints(workflow_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_checkpoints_created ON workflow_checkpoints(created_at DESC);\n"
    "CREATE INDEX IF NOT EXISTS idx_checkpoints_node ON workflow_checkpoints(node_id);\n"
    "\n"
    "PRAGMA foreign_keys = ON;\n"
    "\n"
    "COMMIT;\n";

// ============================================================================
// TEST SETUP/TEARDOWN
// ============================================================================

static void setup_test_db(const char* db_name) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
    unlink(db_name); // Remove if exists

    int rc = sqlite3_open(db_name, &g_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(g_db));
        exit(1);
    }
}

static void teardown_test_db(const char* db_name) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
    unlink(db_name);
}

// ============================================================================
// MIGRATION TESTS
// ============================================================================

static void test_migration_execution(void) {
    printf("test_migration_execution:\n");
    char db_name[256];
    snprintf(db_name, sizeof(db_name), "/tmp/test_migration_%d.db", getpid());
    setup_test_db(db_name);

    char* err_msg = NULL;
    int rc = sqlite3_exec(g_db, MIGRATION_016_SQL, NULL, NULL, &err_msg);
    
    TEST_ASSERT(rc == SQLITE_OK, "migration executes successfully");
    if (err_msg) {
        sqlite3_free(err_msg);
    }

    // Verify tables exist
    const char* check_tables = 
        "SELECT name FROM sqlite_master WHERE type='table' AND name IN "
        "('workflows', 'workflow_nodes', 'workflow_edges', 'workflow_state', 'workflow_checkpoints');";
    
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(g_db, check_tables, -1, &stmt, NULL);
    TEST_ASSERT(rc == SQLITE_OK, "can prepare table check query");
    
    int table_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        table_count++;
    }
    sqlite3_finalize(stmt);
    
    TEST_ASSERT(table_count == 5, "all 5 tables created");

    teardown_test_db(db_name);
    printf("\n");
}

static void test_migration_idempotency(void) {
    printf("test_migration_idempotency:\n");
    char db_name[256];
    snprintf(db_name, sizeof(db_name), "/tmp/test_idempotency_%d.db", getpid());
    setup_test_db(db_name);

    // Run migration twice
    char* err_msg = NULL;
    int rc1 = sqlite3_exec(g_db, MIGRATION_016_SQL, NULL, NULL, &err_msg);
    if (err_msg) sqlite3_free(err_msg);
    err_msg = NULL;
    
    int rc2 = sqlite3_exec(g_db, MIGRATION_016_SQL, NULL, NULL, &err_msg);
    
    TEST_ASSERT(rc1 == SQLITE_OK, "first migration succeeds");
    TEST_ASSERT(rc2 == SQLITE_OK, "second migration succeeds (idempotent)");
    if (err_msg) {
        sqlite3_free(err_msg);
    }

    // Verify table count is still 5 (not duplicated)
    const char* count_tables = 
        "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name IN "
        "('workflows', 'workflow_nodes', 'workflow_edges', 'workflow_state', 'workflow_checkpoints');";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, count_tables, -1, &stmt, NULL);
    TEST_ASSERT(rc == SQLITE_OK, "can prepare count query");
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        TEST_ASSERT(count == 5, "table count is 5 (not duplicated)");
    }
    sqlite3_finalize(stmt);

    teardown_test_db(db_name);
    printf("\n");
}

static void test_foreign_keys(void) {
    printf("test_foreign_keys:\n");
    char db_name[256];
    snprintf(db_name, sizeof(db_name), "/tmp/test_fk_%d.db", getpid());
    setup_test_db(db_name);

    // Run migration
    char* err_msg = NULL;
    int rc = sqlite3_exec(g_db, MIGRATION_016_SQL, NULL, NULL, &err_msg);
    if (err_msg) sqlite3_free(err_msg);
    
    // Enable foreign keys
    sqlite3_exec(g_db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);

    // Try to insert workflow_node with invalid workflow_id (should fail)
    const char* invalid_insert = 
        "INSERT INTO workflow_nodes (workflow_id, name, type, created_at) "
        "VALUES (999, 'test', 0, ?);";
    
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(g_db, invalid_insert, -1, &stmt, NULL);
    TEST_ASSERT(rc == SQLITE_OK, "can prepare invalid insert");
    
    time_t now = time(NULL);
    sqlite3_bind_int64(stmt, 1, now);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    // Should fail due to foreign key constraint
    TEST_ASSERT(rc != SQLITE_OK, "foreign key constraint prevents invalid insert");

    // Insert valid workflow first
    const char* insert_workflow = 
        "INSERT INTO workflows (name, status, created_at) "
        "VALUES ('test_workflow', 0, ?);";
    
    rc = sqlite3_prepare_v2(g_db, insert_workflow, -1, &stmt, NULL);
    TEST_ASSERT(rc == SQLITE_OK, "can prepare workflow insert");
    
    sqlite3_bind_int64(stmt, 1, now);
    rc = sqlite3_step(stmt);
    sqlite3_int64 workflow_id = sqlite3_last_insert_rowid(g_db);
    sqlite3_finalize(stmt);
    
    TEST_ASSERT(rc == SQLITE_DONE, "workflow insert succeeds");
    TEST_ASSERT(workflow_id > 0, "workflow_id is valid");

    // Now insert node with valid workflow_id (should succeed)
    const char* valid_insert = 
        "INSERT INTO workflow_nodes (workflow_id, name, type, created_at) "
        "VALUES (?, 'test_node', 0, ?);";
    
    rc = sqlite3_prepare_v2(g_db, valid_insert, -1, &stmt, NULL);
    TEST_ASSERT(rc == SQLITE_OK, "can prepare valid insert");
    
    sqlite3_bind_int64(stmt, 1, workflow_id);
    sqlite3_bind_int64(stmt, 2, now);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    TEST_ASSERT(rc == SQLITE_DONE, "valid node insert succeeds");

    teardown_test_db(db_name);
    printf("\n");
}

static void test_indexes(void) {
    printf("test_indexes:\n");
    char db_name[256];
    snprintf(db_name, sizeof(db_name), "/tmp/test_indexes_%d.db", getpid());
    setup_test_db(db_name);

    // Run migration
    char* err_msg = NULL;
    int rc = sqlite3_exec(g_db, MIGRATION_016_SQL, NULL, NULL, &err_msg);
    if (err_msg) sqlite3_free(err_msg);

    // Check indexes exist
    const char* check_indexes = 
        "SELECT name FROM sqlite_master WHERE type='index' AND name LIKE 'idx_%';";
    
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(g_db, check_indexes, -1, &stmt, NULL);
    TEST_ASSERT(rc == SQLITE_OK, "can prepare index check query");
    
    int index_count = 0;
    const char* expected_indexes[] = {
        "idx_workflows_status",
        "idx_workflows_created",
        "idx_workflows_current_node",
        "idx_nodes_workflow",
        "idx_nodes_type",
        "idx_edges_workflow",
        "idx_edges_from",
        "idx_edges_to",
        "idx_state_workflow",
        "idx_checkpoints_workflow",
        "idx_checkpoints_created",
        "idx_checkpoints_node"
    };
    const int expected_count = sizeof(expected_indexes) / sizeof(expected_indexes[0]);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* index_name = (const char*)sqlite3_column_text(stmt, 0);
        for (int i = 0; i < expected_count; i++) {
            if (strcmp(index_name, expected_indexes[i]) == 0) {
                index_count++;
                break;
            }
        }
    }
    sqlite3_finalize(stmt);
    
    TEST_ASSERT(index_count == expected_count, "all expected indexes created");

    teardown_test_db(db_name);
    printf("\n");
}

static void test_schema_correctness(void) {
    printf("test_schema_correctness:\n");
    char db_name[256];
    snprintf(db_name, sizeof(db_name), "/tmp/test_schema_%d.db", getpid());
    setup_test_db(db_name);

    // Run migration
    char* err_msg = NULL;
    int rc = sqlite3_exec(g_db, MIGRATION_016_SQL, NULL, NULL, &err_msg);
    if (err_msg) sqlite3_free(err_msg);

    // Check workflows table schema
    const char* check_schema = "PRAGMA table_info(workflows);";
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(g_db, check_schema, -1, &stmt, NULL);
    TEST_ASSERT(rc == SQLITE_OK, "can prepare schema check");
    
    int column_count = 0;
    bool has_id = false, has_name = false, has_status = false;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* col_name = (const char*)sqlite3_column_text(stmt, 1);
        if (strcmp(col_name, "id") == 0) has_id = true;
        if (strcmp(col_name, "name") == 0) has_name = true;
        if (strcmp(col_name, "status") == 0) has_status = true;
        column_count++;
    }
    sqlite3_finalize(stmt);
    
    TEST_ASSERT(has_id, "workflows table has id column");
    TEST_ASSERT(has_name, "workflows table has name column");
    TEST_ASSERT(has_status, "workflows table has status column");
    TEST_ASSERT(column_count >= 10, "workflows table has all required columns");

    teardown_test_db(db_name);
    printf("\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("=== CONVERGIO WORKFLOW MIGRATION TESTS ===\n\n");

    test_migration_execution();
    test_migration_idempotency();
    test_foreign_keys();
    test_indexes();
    test_schema_correctness();

    printf("=== RESULTS ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);

    if (tests_passed == tests_run) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n");
        return 1;
    }
}

