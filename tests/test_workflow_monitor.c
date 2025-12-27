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
    TEST("Skipped icon", strcmp(workflow_monitor_status_icon(AGENT_STATUS_SKIPPED), "⊘") == 0);
    TEST("Waiting icon", strcmp(workflow_monitor_status_icon(AGENT_STATUS_WAITING), "◷") == 0);

    TEST("Pending name", strcmp(workflow_monitor_status_name(AGENT_STATUS_PENDING), "pending") == 0);
    TEST("Thinking name", strcmp(workflow_monitor_status_name(AGENT_STATUS_THINKING), "thinking") == 0);
    TEST("Completed name", strcmp(workflow_monitor_status_name(AGENT_STATUS_COMPLETED), "completed") == 0);
    TEST("Failed name", strcmp(workflow_monitor_status_name(AGENT_STATUS_FAILED), "failed") == 0);
    TEST("Skipped name", strcmp(workflow_monitor_status_name(AGENT_STATUS_SKIPPED), "skipped") == 0);
    TEST("Waiting name", strcmp(workflow_monitor_status_name(AGENT_STATUS_WAITING), "waiting") == 0);

    TEST("Parallel type name", strcmp(workflow_monitor_type_name(WORKFLOW_PARALLEL), "parallel") == 0);
    TEST("Sequential type name", strcmp(workflow_monitor_type_name(WORKFLOW_SEQUENTIAL), "sequential") == 0);
    TEST("Pipeline type name", strcmp(workflow_monitor_type_name(WORKFLOW_PIPELINE), "pipeline") == 0);
    TEST("Conditional type name", strcmp(workflow_monitor_type_name(WORKFLOW_CONDITIONAL), "conditional") == 0);
}

// ============================================================================
// COMPLEX WORKFLOW TESTS
// ============================================================================

void test_sequential_workflow(void) {
    TEST_SECTION("Sequential Workflow");

    const char* agents[] = {"agent-1", "agent-2", "agent-3"};
    const char* tasks[] = {"Task A", "Task B", "Task C"};

    WorkflowMonitor* monitor = workflow_monitor_create_sequential("seq-test", agents, tasks, 3, false);
    TEST("Create sequential workflow succeeds", monitor != NULL);
    TEST("Workflow type is sequential", monitor && monitor->type == WORKFLOW_SEQUENTIAL);
    TEST("Agent count is 3", monitor && monitor->agent_count == 3);

    // Simulate sequential execution
    if (monitor) {
        workflow_monitor_start(monitor);

        workflow_monitor_set_status(monitor, 0, AGENT_STATUS_THINKING);
        TEST("First agent is thinking", monitor->agents[0].status == AGENT_STATUS_THINKING);

        workflow_monitor_set_status(monitor, 0, AGENT_STATUS_COMPLETED);
        workflow_monitor_set_status(monitor, 1, AGENT_STATUS_THINKING);
        TEST("First done, second thinking",
             monitor->agents[0].status == AGENT_STATUS_COMPLETED &&
             monitor->agents[1].status == AGENT_STATUS_THINKING);

        workflow_monitor_set_status(monitor, 1, AGENT_STATUS_COMPLETED);
        workflow_monitor_set_status(monitor, 2, AGENT_STATUS_THINKING);
        workflow_monitor_set_status(monitor, 2, AGENT_STATUS_COMPLETED);
        TEST("All completed",
             monitor->agents[0].status == AGENT_STATUS_COMPLETED &&
             monitor->agents[1].status == AGENT_STATUS_COMPLETED &&
             monitor->agents[2].status == AGENT_STATUS_COMPLETED);

        workflow_monitor_stop(monitor);
    }

    workflow_monitor_free(monitor);
}

void test_pipeline_workflow(void) {
    TEST_SECTION("Pipeline Workflow");

    const char* agents[] = {"input-parser", "processor", "output-formatter"};
    const char* tasks[] = {"Parse input data", "Process data", "Format output"};

    WorkflowMonitor* monitor = workflow_monitor_create_pipeline("pipe-test", agents, tasks, 3, false);
    TEST("Create pipeline workflow succeeds", monitor != NULL);
    TEST("Workflow type is pipeline", monitor && monitor->type == WORKFLOW_PIPELINE);

    if (monitor) {
        workflow_monitor_start(monitor);

        // Render pipeline (visual test)
        printf("\n--- Pipeline render: ---\n");
        workflow_monitor_render_complex(monitor);

        workflow_monitor_stop(monitor);
    }

    workflow_monitor_free(monitor);
    TEST("Pipeline render completes without crash", true);
}

void test_conditional_workflow(void) {
    TEST_SECTION("Conditional Workflow");

    WorkflowMonitor* monitor = workflow_monitor_create_typed("cond-test", WORKFLOW_CONDITIONAL, false);
    TEST("Create conditional workflow succeeds", monitor != NULL);
    TEST("Workflow type is conditional", monitor && monitor->type == WORKFLOW_CONDITIONAL);

    if (monitor) {
        // Create decision tree structure
        int decision = workflow_monitor_add_node(monitor, NODE_DECISION, "Check severity", -1);
        TEST("Add decision node succeeds", decision >= 0);

        workflow_monitor_set_condition(monitor, decision, "severity >= HIGH");
        TEST("Set condition succeeds", monitor->nodes[decision].condition != NULL);

        // Add branches
        int high_branch = workflow_monitor_add_node(monitor, NODE_AGENT, "luca-security-expert", decision);
        int low_branch = workflow_monitor_add_node(monitor, NODE_AGENT, "rex-code-reviewer", decision);
        TEST("Add high severity branch", high_branch >= 0);
        TEST("Add low severity branch", low_branch >= 0);

        // Simulate execution - high severity path
        workflow_monitor_set_node_status(monitor, decision, AGENT_STATUS_COMPLETED);
        workflow_monitor_set_node_status(monitor, high_branch, AGENT_STATUS_THINKING);
        workflow_monitor_set_node_status(monitor, low_branch, AGENT_STATUS_SKIPPED);

        TEST("Decision completed", monitor->nodes[decision].status == AGENT_STATUS_COMPLETED);
        TEST("High branch thinking", monitor->nodes[high_branch].status == AGENT_STATUS_THINKING);
        TEST("Low branch skipped", monitor->nodes[low_branch].status == AGENT_STATUS_SKIPPED);

        // Render conditional (visual test)
        printf("\n--- Conditional render: ---\n");
        workflow_monitor_render_complex(monitor);
    }

    workflow_monitor_free(monitor);
    TEST("Conditional render completes without crash", true);
}

void test_phased_workflow(void) {
    TEST_SECTION("Phased Workflow");

    WorkflowMonitor* monitor = workflow_monitor_create_typed("phased-test", WORKFLOW_PARALLEL, false);
    TEST("Create phased workflow succeeds", monitor != NULL);

    if (monitor) {
        // Add phases
        int phase1 = workflow_monitor_add_phase(monitor, "Analysis Phase");
        int phase2 = workflow_monitor_add_phase(monitor, "Implementation Phase");
        TEST("Add phase 1 succeeds", phase1 >= 0);
        TEST("Add phase 2 succeeds", phase2 >= 0);
        TEST("Phase count is 2", monitor->phase_count == 2);

        // Add agents to phases
        int a1 = workflow_monitor_add_agent_to_phase(monitor, phase1, "rex", "Code review");
        int a2 = workflow_monitor_add_agent_to_phase(monitor, phase1, "baccio", "Architecture review");
        int a3 = workflow_monitor_add_agent_to_phase(monitor, phase2, "paolo", "Implement fixes");
        TEST("Add agent to phase 1", a1 >= 0);
        TEST("Add agent 2 to phase 1", a2 >= 0);
        TEST("Add agent to phase 2", a3 >= 0);

        // Set current phase
        workflow_monitor_set_current_phase(monitor, 0);
        TEST("Set current phase", monitor->current_phase == 0);
    }

    workflow_monitor_free(monitor);
    TEST("Phased workflow cleanup succeeds", true);
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

    // Run complex workflow tests
    test_sequential_workflow();
    test_pipeline_workflow();
    test_conditional_workflow();
    test_phased_workflow();

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
