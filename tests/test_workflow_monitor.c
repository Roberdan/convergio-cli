/**
 * TEST WORKFLOW MONITOR
 *
 * Tests for the ASCII workflow visualization
 * Uses mock data - NO LLM calls needed
 */

#include "nous/workflow_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Simple test framework
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, expr)                                                                           \
    do {                                                                                           \
        if (expr) {                                                                                \
            tests_passed++;                                                                        \
            printf("  ✓ %s\n", name);                                                              \
        } else {                                                                                   \
            tests_failed++;                                                                        \
            printf("  ✗ %s (FAILED)\n", name);                                                     \
        }                                                                                          \
    } while (0)

#define TEST_SECTION(name) printf("\n=== %s ===\n", name)

// ============================================================================
// UNIT TESTS
// ============================================================================

void test_monitor_create(void) {
    TEST_SECTION("Monitor Creation");

    WorkflowMonitor* monitor = workflow_monitor_create("test-workflow", false);
    TEST("Create monitor succeeds", monitor != NULL);
    TEST("Workflow name set", monitor && strcmp(monitor->workflow_name, "test-workflow") == 0);
    TEST("Agent count is 0", monitor && monitor->agent_count == 0);
    TEST("Not active initially", monitor && !monitor->is_active);

    workflow_monitor_free(monitor);
    TEST("Free monitor succeeds", true);  // No crash = success
}

void test_monitor_add_agents(void) {
    TEST_SECTION("Adding Agents");

    WorkflowMonitor* monitor = workflow_monitor_create("test", false);

    int idx1 = workflow_monitor_add_agent(monitor, "rex-code-reviewer", "Review code quality");
    TEST("Add first agent returns 0", idx1 == 0);
    TEST("Agent count is 1", monitor->agent_count == 1);

    int idx2 = workflow_monitor_add_agent(monitor, "baccio-tech-architect", "Analyze architecture");
    TEST("Add second agent returns 1", idx2 == 1);
    TEST("Agent count is 2", monitor->agent_count == 2);

    int idx3 = workflow_monitor_add_agent(monitor, "luca-security-expert", "Check security");
    TEST("Add third agent returns 2", idx3 == 2);
    TEST("Agent count is 3", monitor->agent_count == 3);

    workflow_monitor_free(monitor);
}

void test_monitor_status_updates(void) {
    TEST_SECTION("Status Updates");

    WorkflowMonitor* monitor = workflow_monitor_create("test", false);
    workflow_monitor_add_agent(monitor, "rex", "Task 1");
    workflow_monitor_add_agent(monitor, "baccio", "Task 2");

    TEST("Initial status is pending", monitor->agents[0].status == AGENT_STATUS_PENDING);

    workflow_monitor_set_status(monitor, 0, AGENT_STATUS_THINKING);
    TEST("Set status to thinking", monitor->agents[0].status == AGENT_STATUS_THINKING);

    workflow_monitor_set_status(monitor, 0, AGENT_STATUS_COMPLETED);
    TEST("Set status to completed", monitor->agents[0].status == AGENT_STATUS_COMPLETED);
    TEST("Duration is recorded", monitor->agents[0].duration_ms >= 0);

    workflow_monitor_set_status_by_name(monitor, "baccio", AGENT_STATUS_FAILED);
    TEST("Set status by name", monitor->agents[1].status == AGENT_STATUS_FAILED);

    workflow_monitor_free(monitor);
}

void test_monitor_status_helpers(void) {
    TEST_SECTION("Status Helpers");

    TEST("Pending icon", strcmp(workflow_monitor_status_icon(AGENT_STATUS_PENDING), "○") == 0);
    TEST("Thinking icon", strcmp(workflow_monitor_status_icon(AGENT_STATUS_THINKING), "◐") == 0);
    TEST("Completed icon", strcmp(workflow_monitor_status_icon(AGENT_STATUS_COMPLETED), "●") == 0);
    TEST("Failed icon", strcmp(workflow_monitor_status_icon(AGENT_STATUS_FAILED), "✗") == 0);

    TEST("Pending name", strcmp(workflow_monitor_status_name(AGENT_STATUS_PENDING), "pending") == 0);
    TEST("Thinking name", strcmp(workflow_monitor_status_name(AGENT_STATUS_THINKING), "thinking") == 0);
    TEST("Completed name", strcmp(workflow_monitor_status_name(AGENT_STATUS_COMPLETED), "completed") == 0);
    TEST("Failed name", strcmp(workflow_monitor_status_name(AGENT_STATUS_FAILED), "failed") == 0);
}

// ============================================================================
// VISUAL DEMO (not an automated test)
// ============================================================================

void demo_visual_workflow(void) {
    TEST_SECTION("Visual Demo (simulated workflow)");

    printf("\n--- Simulating a 3-agent delegation workflow ---\n\n");

    // Create monitor with ANSI enabled
    WorkflowMonitor* monitor = workflow_monitor_create("delegation", true);
    workflow_monitor_start(monitor);

    // Add agents
    workflow_monitor_add_agent(monitor, "rex-code-reviewer", "Analyze code quality");
    workflow_monitor_add_agent(monitor, "baccio-tech-architect", "Review architecture");
    workflow_monitor_add_agent(monitor, "luca-security-expert", "Check vulnerabilities");

    // Initial render
    workflow_monitor_render(monitor);
    printf("\n[Press enter to simulate agents starting...]\n");
    getchar();

    // Start all agents thinking
    workflow_monitor_set_status(monitor, 0, AGENT_STATUS_THINKING);
    workflow_monitor_set_status(monitor, 1, AGENT_STATUS_THINKING);
    workflow_monitor_set_status(monitor, 2, AGENT_STATUS_THINKING);
    workflow_monitor_render(monitor);
    printf("\n[Press enter to simulate rex completing...]\n");
    getchar();

    // Rex completes
    workflow_monitor_set_status(monitor, 0, AGENT_STATUS_COMPLETED);
    workflow_monitor_render(monitor);
    printf("\n[Press enter to simulate baccio completing...]\n");
    getchar();

    // Baccio completes
    workflow_monitor_set_status(monitor, 1, AGENT_STATUS_COMPLETED);
    workflow_monitor_render(monitor);
    printf("\n[Press enter to simulate luca failing...]\n");
    getchar();

    // Luca fails
    workflow_monitor_set_status(monitor, 2, AGENT_STATUS_FAILED);
    workflow_monitor_stop(monitor);
    workflow_monitor_render(monitor);

    // Show summary
    printf("\n");
    workflow_monitor_render_summary(monitor);

    workflow_monitor_free(monitor);
    printf("\n--- Demo complete ---\n");
}

// ============================================================================
// QUICK VISUAL TEST (non-interactive)
// ============================================================================

void test_render_output(void) {
    TEST_SECTION("Render Output");

    WorkflowMonitor* monitor = workflow_monitor_create("test-render", false);  // No ANSI for predictable output
    workflow_monitor_start(monitor);

    workflow_monitor_add_agent(monitor, "agent-1", "Task A");
    workflow_monitor_add_agent(monitor, "agent-2", "Task B");

    workflow_monitor_set_status(monitor, 0, AGENT_STATUS_THINKING);
    workflow_monitor_set_status(monitor, 1, AGENT_STATUS_COMPLETED);

    printf("\n--- Render output: ---\n");
    workflow_monitor_render(monitor);

    workflow_monitor_stop(monitor);
    workflow_monitor_render_summary(monitor);

    workflow_monitor_free(monitor);

    TEST("Render completes without crash", true);
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[]) {
    printf("\n╔═══════════════════════════════════════════╗\n");
    printf("║   WORKFLOW MONITOR TEST SUITE             ║\n");
    printf("╚═══════════════════════════════════════════╝\n");

    // Check for demo mode
    bool demo_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--demo") == 0 || strcmp(argv[i], "-d") == 0) {
            demo_mode = true;
        }
    }

    // Run unit tests
    test_monitor_create();
    test_monitor_add_agents();
    test_monitor_status_updates();
    test_monitor_status_helpers();
    test_render_output();

    // Summary
    printf("\n═══════════════════════════════════════════\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("═══════════════════════════════════════════\n");

    // Demo mode if requested
    if (demo_mode) {
        printf("\n[Demo mode enabled - interactive visual demo]\n");
        demo_visual_workflow();
    } else {
        printf("\nRun with --demo for interactive visual demonstration\n");
    }

    return tests_failed > 0 ? 1 : 0;
}
