/**
 * CONVERGIO PROJECTS MODULE TESTS
 *
 * Unit tests for project management functionality
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>
#include "../src/projects/projects.h"
#include "nous/nous.h"

// ============================================================================
// STUBS FOR TEST ISOLATION
// ============================================================================

// Logging stub
LogLevel g_log_level = LOG_LEVEL_ERROR;
void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) {
    (void)level; (void)cat; (void)fmt;
}
void nous_log_set_level(LogLevel level) { g_log_level = level; }
LogLevel nous_log_get_level(void) { return g_log_level; }
const char* nous_log_level_name(LogLevel level) { (void)level; return ""; }

// Keychain stubs
int convergio_keychain_store(const char* service, const char* account, const char* data) {
    (void)service; (void)account; (void)data;
    return 0;
}
char* convergio_keychain_read(const char* service, const char* account) {
    (void)service; (void)account;
    return NULL;
}
int convergio_keychain_delete(const char* service, const char* account) {
    (void)service; (void)account;
    return 0;
}

// ============================================================================
// TEST HELPERS
// ============================================================================

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "TEST FAILED: %s\n", msg); \
            exit(1); \
        } \
    } while(0)

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name, condition) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("  ✓ %s\n", name); \
    } else { \
        printf("  ✗ %s FAILED\n", name); \
    } \
} while(0)

// ============================================================================
// TEST PROJECT INITIALIZATION
// ============================================================================

static void test_project_init(void) {
    printf("\n=== Project Initialization Tests ===\n");
    
    // Test initialization (returns bool: true on success)
    bool result = projects_init();
    TEST("Projects init succeeds", result == true);
    
    // Test shutdown
    projects_shutdown();
    TEST("Projects shutdown succeeds", 1);  // No return value to check
}

// ============================================================================
// TEST PROJECT CREATION
// ============================================================================

static void test_project_creation(void) {
    printf("\n=== Project Creation Tests ===\n");
    
    // Re-init for tests
    projects_init();
    
    // Create a test project
    const char* project_name = "test-project-123";
    ConvergioProject* proj = project_create(project_name, "Test project description", NULL, NULL);
    
    TEST("Project creation succeeds", proj != NULL);
    if (proj) {
        TEST("Project name matches", strcmp(proj->name, project_name) == 0);
        TEST("Project purpose matches", strcmp(proj->purpose, "Test project description") == 0);
        // Note: Don't call project_free(proj) here - projects_shutdown() handles cleanup
        // of all projects in the manager's all_projects array
    }

    projects_shutdown();
}

// ============================================================================
// TEST PROJECT TEMPLATES
// ============================================================================

static void test_project_templates(void) {
    printf("\n=== Project Templates Tests ===\n");
    
    projects_init();
    
    // Test template listing
    size_t template_count = 0;
    const ProjectTemplate* templates = project_get_templates(&template_count);
    
    TEST("Templates list succeeds", templates != NULL);
    TEST("Template count > 0", template_count > 0);
    
    if (templates && template_count > 0) {
        TEST("First template has name", templates[0].name != NULL);
        TEST("First template has description", templates[0].description != NULL);
    }
    
    projects_shutdown();
}

// ============================================================================
// TEST PROJECT VALIDATION
// ============================================================================

static void test_project_validation(void) {
    printf("\n=== Project Validation Tests ===\n");
    
    // Test invalid project names
    TEST("Empty name rejected", project_create("", "desc", NULL, NULL) == NULL);
    TEST("NULL name rejected", project_create(NULL, "desc", NULL, NULL) == NULL);
    
    // Test valid project name format
    projects_init();
    ConvergioProject* valid = project_create("valid-project-123", "Valid project", NULL, NULL);
    TEST("Valid project name accepted", valid != NULL);
    // Note: Don't call project_free() - projects_shutdown() handles cleanup
    projects_shutdown();
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║       CONVERGIO PROJECTS MODULE TESTS             ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");
    
    test_project_init();
    test_project_creation();
    test_project_templates();
    test_project_validation();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║       Test Results: %d/%d passed                  ║\n", tests_passed, tests_run);
    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}

