/**
 * Unit Tests for Convergio Core Components
 *
 * Tests persistence layer and tool sandbox functionality
 * Run with: make unit_test && ./build/bin/unit_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

// Safe path helper (from safe_path.c)
#include "nous/safe_path.h"

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
// SAFE PATH TESTS
// ============================================================================

void test_safe_path_resolve(void) {
    TEST_SECTION("Safe Path Resolution Tests");

    char resolved[PATH_MAX];
    SafePathResult result;

    // Test NULL inputs
    result = safe_path_resolve(NULL, "/tmp", resolved, sizeof(resolved));
    TEST("Reject NULL path", result == SAFE_PATH_NULL_INPUT);

    result = safe_path_resolve("/tmp/test", "/tmp", NULL, sizeof(resolved));
    TEST("Reject NULL output buffer", result == SAFE_PATH_NULL_INPUT);

    // Test boundary checking
    result = safe_path_resolve("/tmp/test.txt", "/tmp", resolved, sizeof(resolved));
    // May fail if /tmp/test.txt doesn't exist, but shouldn't crash
    TEST("Handle non-existent file gracefully", result == SAFE_PATH_OK || result == SAFE_PATH_RESOLVE_FAILED);

    // Test path outside boundary detection
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        // Try to resolve a path outside current directory
        result = safe_path_resolve("/etc/passwd", cwd, resolved, sizeof(resolved));
        TEST("Block path outside boundary", result == SAFE_PATH_OUTSIDE_BOUNDARY || result == SAFE_PATH_RESOLVE_FAILED);
    }
}

void test_safe_path_boundary_weak(void) {
    TEST_SECTION("Weak Boundary Check Tests");

    // Test path traversal detection
    TEST("Block .. traversal", !safe_path_within_boundary_weak("../etc/passwd", "/home/user"));
    TEST("Block absolute escape", !safe_path_within_boundary_weak("/etc/passwd", "/home/user"));
    TEST("Block embedded ..", !safe_path_within_boundary_weak("foo/../../../etc/passwd", "/home/user"));

    // Test valid paths
    TEST("Allow relative path", safe_path_within_boundary_weak("foo/bar.txt", "/home/user"));
    TEST("Allow nested path", safe_path_within_boundary_weak("foo/bar/baz.txt", "/home/user"));

    // Test NULL handling
    TEST("Reject NULL path", !safe_path_within_boundary_weak(NULL, "/home/user"));
    TEST("Reject NULL boundary", !safe_path_within_boundary_weak("foo.txt", NULL));
}

void test_safe_path_join(void) {
    TEST_SECTION("Safe Path Join Tests");

    char result[PATH_MAX];
    SafePathResult res;

    // Test basic join
    res = safe_path_join("/home/user", "file.txt", result, sizeof(result));
    TEST("Join basic path", res == SAFE_PATH_OK && strcmp(result, "/home/user/file.txt") == 0);

    // Test trailing slash
    res = safe_path_join("/home/user/", "file.txt", result, sizeof(result));
    TEST("Handle trailing slash", res == SAFE_PATH_OK && strcmp(result, "/home/user/file.txt") == 0);

    // Test component starting with /
    res = safe_path_join("/home/user", "/etc/passwd", result, sizeof(result));
    TEST("Block absolute component", res == SAFE_PATH_OUTSIDE_BOUNDARY);

    // Test traversal in component
    res = safe_path_join("/home/user", "../etc/passwd", result, sizeof(result));
    TEST("Block traversal in component", res == SAFE_PATH_OUTSIDE_BOUNDARY);

    // Test buffer size
    char small[10];
    res = safe_path_join("/home/user", "very_long_filename.txt", small, sizeof(small));
    TEST("Detect buffer overflow", res == SAFE_PATH_TOO_LONG);

    // Test NULL handling
    res = safe_path_join(NULL, "file.txt", result, sizeof(result));
    TEST("Reject NULL base", res == SAFE_PATH_NULL_INPUT);

    res = safe_path_join("/home", NULL, result, sizeof(result));
    TEST("Reject NULL component", res == SAFE_PATH_NULL_INPUT);
}

void test_safe_path_boundaries(void) {
    TEST_SECTION("Boundary Helper Tests");

    const char* user_boundary = safe_path_get_user_boundary();
    TEST("User boundary not NULL", user_boundary != NULL);
    TEST("User boundary contains .convergio", user_boundary && strstr(user_boundary, ".convergio") != NULL);

    const char* cwd_boundary = safe_path_get_cwd_boundary();
    TEST("CWD boundary not NULL", cwd_boundary != NULL);
    TEST("CWD boundary is absolute", cwd_boundary && cwd_boundary[0] == '/');
}

// ============================================================================
// TOOL SANDBOX TESTS
// ============================================================================

// External function declarations for tool testing
extern bool tools_is_command_safe(const char* command);
extern bool tools_is_path_safe(const char* path);

void test_tool_command_sandbox(void) {
    TEST_SECTION("Command Sandbox Tests");

    // These tests check the external tool sandbox functions
    // They may fail if tools.c isn't linked, which is expected

    // Safe commands
    TEST("Allow ls", tools_is_command_safe("ls -la"));
    TEST("Allow pwd", tools_is_command_safe("pwd"));
    TEST("Allow cat", tools_is_command_safe("cat file.txt"));
    TEST("Allow grep", tools_is_command_safe("grep pattern file.txt"));

    // Dangerous commands
    TEST("Block rm -rf /", !tools_is_command_safe("rm -rf /"));
    TEST("Block rm -rf /*", !tools_is_command_safe("rm -rf /*"));
    TEST("Block sudo", !tools_is_command_safe("sudo rm -rf /"));
    TEST("Block sh -c", !tools_is_command_safe("sh -c 'rm -rf /'"));

    // Injection attempts
    TEST("Block backtick", !tools_is_command_safe("ls `whoami`"));
    TEST("Block $(...)", !tools_is_command_safe("ls $(whoami)"));
    TEST("Block semicolon", !tools_is_command_safe("ls; rm -rf /"));
    TEST("Block &&", !tools_is_command_safe("ls && rm -rf /"));
    TEST("Block ||", !tools_is_command_safe("false || rm -rf /"));
}

void test_tool_path_sandbox(void) {
    TEST_SECTION("Path Sandbox Tests");

    // System paths should be blocked
    TEST("Block /etc", !tools_is_path_safe("/etc/passwd"));
    TEST("Block /var", !tools_is_path_safe("/var/log/system.log"));
    TEST("Block /System", !tools_is_path_safe("/System/Library/"));
    TEST("Block /Library", !tools_is_path_safe("/Library/"));
    TEST("Block /bin", !tools_is_path_safe("/bin/sh"));
    TEST("Block /sbin", !tools_is_path_safe("/sbin/mount"));
    TEST("Block /usr", !tools_is_path_safe("/usr/bin/ls"));

    // NULL path
    TEST("Block NULL", !tools_is_path_safe(NULL));

    // Empty path
    TEST("Block empty", !tools_is_path_safe(""));
}

// ============================================================================
// STRERROR TESTS
// ============================================================================

void test_safe_path_strerror(void) {
    TEST_SECTION("Error String Tests");

    TEST("SAFE_PATH_OK message", strlen(safe_path_strerror(SAFE_PATH_OK)) > 0);
    TEST("SAFE_PATH_NULL_INPUT message", strlen(safe_path_strerror(SAFE_PATH_NULL_INPUT)) > 0);
    TEST("SAFE_PATH_TOO_LONG message", strlen(safe_path_strerror(SAFE_PATH_TOO_LONG)) > 0);
    TEST("SAFE_PATH_RESOLVE_FAILED message", strlen(safe_path_strerror(SAFE_PATH_RESOLVE_FAILED)) > 0);
    TEST("SAFE_PATH_OUTSIDE_BOUNDARY message", strlen(safe_path_strerror(SAFE_PATH_OUTSIDE_BOUNDARY)) > 0);
    TEST("Unknown error message", strlen(safe_path_strerror(999)) > 0);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("\n\033[1;36m╔══════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m║     CONVERGIO UNIT TESTS                     ║\033[0m\n");
    printf("\033[1;36m╚══════════════════════════════════════════════╝\033[0m\n");

    // Safe path tests
    test_safe_path_resolve();
    test_safe_path_boundary_weak();
    test_safe_path_join();
    test_safe_path_boundaries();
    test_safe_path_strerror();

    // Tool sandbox tests (require full binary linkage)
    test_tool_command_sandbox();
    test_tool_path_sandbox();

    // Summary
    printf("\n\033[1m══════════════════════════════════════════════\033[0m\n");
    printf("Tests run: %d\n", tests_run);
    printf("\033[32mPassed: %d\033[0m\n", tests_passed);
    if (tests_failed > 0) {
        printf("\033[31mFailed: %d\033[0m\n", tests_failed);
    }

    if (tests_failed == 0) {
        printf("\n\033[32m✓ All tests passed!\033[0m\n\n");
        return 0;
    } else {
        printf("\n\033[31m✗ Some tests failed\033[0m\n\n");
        return 1;
    }
}
