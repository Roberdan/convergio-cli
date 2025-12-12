/**
 * CONVERGIO MODEL ROUTER UNIT TESTS
 *
 * Tests for model selection, budget tracking, and fallback logic.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Forward declarations for router functions (would normally come from header)
extern int router_init(void);
extern void router_shutdown(void);
extern int router_set_agent_model(const char* agent_name, const char* primary_model,
                                   const char* fallback_model);
extern void router_set_budget(double daily, double session);
extern void router_reset_session_budget(void);
extern void router_record_cost(double cost);
extern void router_get_stats(size_t* total, size_t* fallbacks, size_t* downgrades,
                              double* spent_daily, double* spent_session);

// ============================================================================
// TEST UTILITIES
// ============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  Running: %s... ", #name); \
    test_##name(); \
    tests_run++; \
    tests_passed++; \
    printf("PASSED\n"); \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        printf("FAILED at %s:%d - %s\n", __FILE__, __LINE__, #cond); \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NE(a, b) ASSERT_TRUE((a) != (b))
#define ASSERT_GT(a, b) ASSERT_TRUE((a) > (b))
#define ASSERT_GE(a, b) ASSERT_TRUE((a) >= (b))
#define ASSERT_LT(a, b) ASSERT_TRUE((a) < (b))
#define ASSERT_LE(a, b) ASSERT_TRUE((a) <= (b))

// ============================================================================
// INITIALIZATION TESTS
// ============================================================================

TEST(router_init_success) {
    int result = router_init();
    ASSERT_EQ(result, 0);
    router_shutdown();
}

TEST(router_double_init) {
    int result1 = router_init();
    int result2 = router_init();  // Should handle gracefully
    ASSERT_EQ(result1, 0);
    ASSERT_EQ(result2, 0);  // Already initialized is OK
    router_shutdown();
}

TEST(router_shutdown_without_init) {
    // Should not crash
    router_shutdown();
}

// ============================================================================
// BUDGET TESTS
// ============================================================================

TEST(router_set_budget) {
    router_init();
    router_set_budget(10.0, 5.0);  // $10 daily, $5 session

    double daily_spent, session_spent;
    size_t total, fallbacks, downgrades;
    router_get_stats(&total, &fallbacks, &downgrades, &daily_spent, &session_spent);

    ASSERT_EQ(daily_spent, 0.0);
    ASSERT_EQ(session_spent, 0.0);
    router_shutdown();
}

TEST(router_record_cost) {
    router_init();
    router_set_budget(10.0, 5.0);

    router_record_cost(1.50);
    router_record_cost(0.75);

    double daily_spent, session_spent;
    size_t total, fallbacks, downgrades;
    router_get_stats(&total, &fallbacks, &downgrades, &daily_spent, &session_spent);

    ASSERT_GE(daily_spent, 2.25);  // May have small floating point variance
    ASSERT_GE(session_spent, 2.25);
    router_shutdown();
}

TEST(router_reset_session_budget) {
    router_init();
    router_set_budget(10.0, 5.0);

    router_record_cost(2.00);
    router_reset_session_budget();

    double daily_spent, session_spent;
    size_t total, fallbacks, downgrades;
    router_get_stats(&total, &fallbacks, &downgrades, &daily_spent, &session_spent);

    ASSERT_GE(daily_spent, 2.00);   // Daily should still have the cost
    ASSERT_EQ(session_spent, 0.0);  // Session should be reset
    router_shutdown();
}

// ============================================================================
// AGENT CONFIGURATION TESTS
// ============================================================================

TEST(router_set_agent_model) {
    router_init();

    int result = router_set_agent_model(
        "test_agent",
        "anthropic/claude-sonnet-4.5",
        "openai/gpt-4o"
    );

    ASSERT_EQ(result, 0);
    router_shutdown();
}

TEST(router_set_agent_model_null_name) {
    router_init();

    int result = router_set_agent_model(
        NULL,
        "anthropic/claude-sonnet-4.5",
        "openai/gpt-4o"
    );

    ASSERT_NE(result, 0);  // Should fail with NULL agent name
    router_shutdown();
}

TEST(router_set_agent_model_null_primary) {
    router_init();

    int result = router_set_agent_model(
        "test_agent",
        NULL,  // NULL primary model
        "openai/gpt-4o"
    );

    ASSERT_NE(result, 0);  // Should fail with NULL primary model
    router_shutdown();
}

TEST(router_update_existing_agent) {
    router_init();

    // Set initial config
    router_set_agent_model("test_agent", "anthropic/claude-sonnet-4.5", "openai/gpt-4o");

    // Update same agent
    int result = router_set_agent_model("test_agent", "anthropic/claude-opus-4.5", "gemini/gemini-2-flash");

    ASSERT_EQ(result, 0);  // Should update existing config
    router_shutdown();
}

// ============================================================================
// STATISTICS TESTS
// ============================================================================

TEST(router_stats_initial) {
    router_init();

    size_t total, fallbacks, downgrades;
    double daily_spent, session_spent;

    router_get_stats(&total, &fallbacks, &downgrades, &daily_spent, &session_spent);

    ASSERT_EQ(total, 0);
    ASSERT_EQ(fallbacks, 0);
    ASSERT_EQ(downgrades, 0);
    router_shutdown();
}

TEST(router_stats_null_params) {
    router_init();

    // Should not crash with NULL parameters
    router_get_stats(NULL, NULL, NULL, NULL, NULL);

    size_t total;
    router_get_stats(&total, NULL, NULL, NULL, NULL);
    ASSERT_EQ(total, 0);

    router_shutdown();
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("\n=== Model Router Unit Tests ===\n\n");

    printf("Initialization Tests:\n");
    RUN_TEST(router_init_success);
    RUN_TEST(router_double_init);
    RUN_TEST(router_shutdown_without_init);

    printf("\nBudget Tests:\n");
    RUN_TEST(router_set_budget);
    RUN_TEST(router_record_cost);
    RUN_TEST(router_reset_session_budget);

    printf("\nAgent Configuration Tests:\n");
    RUN_TEST(router_set_agent_model);
    RUN_TEST(router_set_agent_model_null_name);
    RUN_TEST(router_set_agent_model_null_primary);
    RUN_TEST(router_update_existing_agent);

    printf("\nStatistics Tests:\n");
    RUN_TEST(router_stats_initial);
    RUN_TEST(router_stats_null_params);

    printf("\n=== Results: %d/%d tests passed ===\n\n", tests_passed, tests_run);

    return tests_passed == tests_run ? 0 : 1;
}
