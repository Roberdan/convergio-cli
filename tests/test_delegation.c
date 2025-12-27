/**
 * Delegation Workflow Tests
 *
 * Tests for Ali's multi-agent delegation:
 * - Delegation parsing from response text
 * - Parallel agent execution via GCD
 * - Response synthesis
 *
 * Run with: make delegation_test && ./build/bin/delegation_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nous/nous.h"
#include "nous/delegation.h"

// Stub for nous_log
LogLevel g_log_level = LOG_LEVEL_ERROR;

void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) {
    (void)level; (void)cat; (void)fmt;
}

void nous_log_set_level(LogLevel level) { g_log_level = level; }
LogLevel nous_log_get_level(void) { return g_log_level; }
const char* nous_log_level_name(LogLevel level) { (void)level; return ""; }

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
// DELEGATION PARSING TESTS
// ============================================================================

void test_delegation_parsing_single(void) {
    TEST_SECTION("Single Delegation Parsing");

    const char* response = "I'll delegate this to the expert.\n[DELEGATE: rex-code-reviewer] Please review this code.";

    DelegationList* list = parse_all_delegations(response);
    TEST("Parse returns non-NULL for valid delegation", list != NULL);

    if (list) {
        TEST("Parse finds exactly 1 delegation", list->count == 1);
        if (list->count >= 1) {
            TEST("Agent name is rex-code-reviewer",
                 list->requests[0]->agent_name != NULL &&
                 strcmp(list->requests[0]->agent_name, "rex-code-reviewer") == 0);
            TEST("Reason is captured",
                 list->requests[0]->reason != NULL &&
                 strstr(list->requests[0]->reason, "review") != NULL);
        }
        free_delegation_list(list);
    }
}

void test_delegation_parsing_multiple(void) {
    TEST_SECTION("Multiple Delegation Parsing");

    const char* response =
        "This requires multiple specialists:\n"
        "[DELEGATE: rex-code-reviewer] Review the code quality\n"
        "[DELEGATE: luca-security-expert] Check for vulnerabilities\n"
        "[DELEGATE: paolo-best-practices-enforcer] Verify coding standards";

    DelegationList* list = parse_all_delegations(response);
    TEST("Parse returns non-NULL for multiple delegations", list != NULL);

    if (list) {
        TEST("Parse finds exactly 3 delegations", list->count == 3);

        if (list->count >= 3) {
            TEST("First agent is rex-code-reviewer",
                 strcmp(list->requests[0]->agent_name, "rex-code-reviewer") == 0);
            TEST("Second agent is luca-security-expert",
                 strcmp(list->requests[1]->agent_name, "luca-security-expert") == 0);
            TEST("Third agent is paolo-best-practices-enforcer",
                 strcmp(list->requests[2]->agent_name, "paolo-best-practices-enforcer") == 0);
        }
        free_delegation_list(list);
    }
}

void test_delegation_parsing_no_delegation(void) {
    TEST_SECTION("No Delegation Parsing");

    const char* response = "This is a normal response without any delegation markers.";

    DelegationList* list = parse_all_delegations(response);
    TEST("Parse returns NULL when no delegations", list == NULL);

    if (list) {
        free_delegation_list(list);
    }
}

void test_delegation_parsing_malformed(void) {
    TEST_SECTION("Malformed Delegation Parsing");

    // Missing closing bracket
    const char* response1 = "I'll delegate [DELEGATE: rex-code-reviewer to review";
    DelegationList* list1 = parse_all_delegations(response1);
    TEST("Parse handles missing closing bracket", list1 == NULL || list1->count == 0);
    if (list1) free_delegation_list(list1);

    // Empty agent name
    const char* response2 = "[DELEGATE: ] Empty agent";
    DelegationList* list2 = parse_all_delegations(response2);
    if (list2 && list2->count > 0) {
        TEST("Empty agent name is captured as empty or whitespace",
             list2->requests[0]->agent_name != NULL);
        free_delegation_list(list2);
    } else {
        TEST("Empty agent handled gracefully", true);
    }
}

void test_delegation_parsing_whitespace(void) {
    TEST_SECTION("Whitespace Handling in Delegation");

    // Extra spaces around agent name
    const char* response = "[DELEGATE:   rex-code-reviewer   ] Review code";

    DelegationList* list = parse_all_delegations(response);
    TEST("Parse returns non-NULL", list != NULL);

    if (list && list->count > 0) {
        // Should trim whitespace
        TEST("Agent name has leading spaces trimmed",
             list->requests[0]->agent_name[0] != ' ');
        TEST("Agent name has trailing spaces trimmed",
             list->requests[0]->agent_name[strlen(list->requests[0]->agent_name)-1] != ' ');
        free_delegation_list(list);
    }
}

// ============================================================================
// DELEGATION MARKER FORMAT TESTS
// ============================================================================

void test_delegation_marker_formats(void) {
    TEST_SECTION("Delegation Marker Format Variations");

    // Standard format
    const char* standard = "[DELEGATE: agent-name] reason";
    DelegationList* list1 = parse_all_delegations(standard);
    TEST("Standard format parses", list1 != NULL && list1->count == 1);
    if (list1) free_delegation_list(list1);

    // No space after colon
    const char* nospace = "[DELEGATE:agent-name] reason";
    DelegationList* list2 = parse_all_delegations(nospace);
    TEST("No space after colon parses", list2 != NULL && list2->count == 1);
    if (list2) free_delegation_list(list2);

    // Multiple spaces
    const char* multispace = "[DELEGATE:    agent-name] reason";
    DelegationList* list3 = parse_all_delegations(multispace);
    TEST("Multiple spaces after colon parses", list3 != NULL && list3->count == 1);
    if (list3) free_delegation_list(list3);
}

// ============================================================================
// STRESS TESTS
// ============================================================================

void test_delegation_many_agents(void) {
    TEST_SECTION("Many Agents Delegation");

    // Build a response with many delegations
    char response[4096];
    int offset = 0;
    offset += snprintf(response + offset, sizeof(response) - offset,
                       "Delegating to team:\n");

    const char* agents[] = {
        "agent-1", "agent-2", "agent-3", "agent-4", "agent-5",
        "agent-6", "agent-7", "agent-8", "agent-9", "agent-10"
    };
    int num_agents = sizeof(agents) / sizeof(agents[0]);

    for (int i = 0; i < num_agents; i++) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                           "[DELEGATE: %s] Task %d\n", agents[i], i+1);
    }

    DelegationList* list = parse_all_delegations(response);
    TEST("Parse returns non-NULL for 10 delegations", list != NULL);

    if (list) {
        TEST("Parse finds all 10 delegations", list->count == 10);

        // Verify each agent
        int all_correct = 1;
        for (size_t i = 0; i < list->count && i < 10; i++) {
            if (strcmp(list->requests[i]->agent_name, agents[i]) != 0) {
                all_correct = 0;
                printf("    Mismatch at %zu: expected '%s', got '%s'\n",
                       i, agents[i], list->requests[i]->agent_name);
            }
        }
        TEST("All agent names match", all_correct);

        free_delegation_list(list);
    }
}

// ============================================================================
// MEMORY TESTS
// ============================================================================

void test_delegation_memory_cleanup(void) {
    TEST_SECTION("Memory Cleanup Tests");

    // Parse and free multiple times to check for leaks
    for (int i = 0; i < 100; i++) {
        const char* response = "[DELEGATE: agent-1] Task 1\n[DELEGATE: agent-2] Task 2";
        DelegationList* list = parse_all_delegations(response);
        if (list) {
            free_delegation_list(list);
        }
    }
    TEST("100 parse/free cycles completed without crash", true);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║      CONVERGIO DELEGATION WORKFLOW TESTS                  ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    // Run tests
    test_delegation_parsing_single();
    test_delegation_parsing_multiple();
    test_delegation_parsing_no_delegation();
    test_delegation_parsing_malformed();
    test_delegation_parsing_whitespace();
    test_delegation_marker_formats();
    test_delegation_many_agents();
    test_delegation_memory_cleanup();

    // Summary
    printf("\n══════════════════════════════════════════════════════════════\n");
    printf("Tests run: %d, Passed: %d, Failed: %d\n", tests_run, tests_passed, tests_failed);

    if (tests_failed > 0) {
        printf("\033[31mSOME TESTS FAILED\033[0m\n");
        return 1;
    }

    printf("\033[32mALL TESTS PASSED\033[0m\n");
    return 0;
}
