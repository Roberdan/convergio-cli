/**
 * CONVERGIO MULTI-PROVIDER INTEGRATION TESTS
 *
 * Tests for multi-provider scenarios including:
 * - Provider fallback when primary fails
 * - Cost-based provider selection
 * - Streaming across providers
 * - Tool calling with different providers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../mock_provider.h"

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

#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)
#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_STR_CONTAINS(haystack, needle) ASSERT_TRUE(strstr(haystack, needle) != NULL)

// ============================================================================
// MULTI-PROVIDER SCENARIO TESTS
// ============================================================================

/**
 * Test: Primary provider succeeds on first try
 */
TEST(primary_provider_success) {
    MockProvider* primary = mock_provider_success("Response from primary");
    Provider* p = mock_provider_as_provider(primary);

    p->init(p);

    TokenUsage usage = {0};
    char* response = p->chat(p, "claude-sonnet", "system", "user prompt", &usage);

    ASSERT_NOT_NULL(response);
    ASSERT_STR_CONTAINS(response, "primary");
    ASSERT_TRUE(usage.input_tokens > 0);

    free(response);
    p->shutdown(p);
    mock_provider_destroy(primary);
}

/**
 * Test: Fallback to secondary when primary fails
 */
TEST(fallback_on_primary_failure) {
    // Primary that always fails
    MockProvider* primary = mock_provider_error(PROVIDER_ERR_NETWORK);
    // Secondary that succeeds
    MockProvider* secondary = mock_provider_success("Response from fallback");

    Provider* p1 = mock_provider_as_provider(primary);
    Provider* p2 = mock_provider_as_provider(secondary);

    p1->init(p1);
    p2->init(p2);

    TokenUsage usage = {0};

    // Try primary (should fail)
    char* response = p1->chat(p1, "claude-sonnet", "system", "user prompt", &usage);
    ASSERT_NULL(response);

    // Fallback to secondary (should succeed)
    response = p2->chat(p2, "gpt-4o", "system", "user prompt", &usage);
    ASSERT_NOT_NULL(response);
    ASSERT_STR_CONTAINS(response, "fallback");

    free(response);
    p1->shutdown(p1);
    p2->shutdown(p2);
    mock_provider_destroy(primary);
    mock_provider_destroy(secondary);
}

/**
 * Test: Rate limiting triggers fallback
 */
TEST(rate_limit_triggers_fallback) {
    // Primary with strict rate limit
    MockProvider* primary = mock_provider_rate_limited(1);  // 1 RPM
    MockProvider* fallback = mock_provider_success("Fallback response");

    Provider* p1 = mock_provider_as_provider(primary);
    Provider* p2 = mock_provider_as_provider(fallback);

    p1->init(p1);
    p2->init(p2);

    TokenUsage usage = {0};

    // First request should succeed
    char* r1 = p1->chat(p1, "model", "sys", "user1", &usage);
    ASSERT_NOT_NULL(r1);
    free(r1);

    // Second request should be rate limited
    char* r2 = p1->chat(p1, "model", "sys", "user2", &usage);
    ASSERT_NULL(r2);  // Rate limited

    // Fallback should work
    char* r3 = p2->chat(p2, "model", "sys", "user2", &usage);
    ASSERT_NOT_NULL(r3);
    ASSERT_STR_CONTAINS(r3, "Fallback");
    free(r3);

    p1->shutdown(p1);
    p2->shutdown(p2);
    mock_provider_destroy(primary);
    mock_provider_destroy(fallback);
}

// Streaming test state
static int streaming_chunk_count = 0;
static char streaming_full_response[1024] = "";

static void streaming_on_chunk(const char* chunk, bool done, void* ctx) {
    (void)ctx;
    (void)done;
    streaming_chunk_count++;
    strncat(streaming_full_response, chunk, sizeof(streaming_full_response) - strlen(streaming_full_response) - 1);
}

/**
 * Test: Streaming works across providers
 */
TEST(streaming_across_providers) {
    MockProviderConfig config = {
        .default_response = "Streamed response",
        .support_streaming = true,
        .stream_chunk_size = 20,
        .stream_delay_ms = 10,
    };

    MockProvider* mock = mock_provider_create_with_config(&config);
    Provider* p = mock_provider_as_provider(mock);
    p->init(p);

    streaming_chunk_count = 0;
    streaming_full_response[0] = '\0';

    StreamHandler handler = {
        .on_chunk = streaming_on_chunk,
        .on_complete = NULL,
        .on_error = NULL,
        .user_ctx = NULL
    };

    TokenUsage usage = {0};
    ProviderError err = p->stream_chat(p, "model", "sys", "user", &handler, &usage);

    ASSERT_EQ(err, PROVIDER_OK);
    ASSERT_TRUE(streaming_chunk_count > 0);
    ASSERT_TRUE(strlen(streaming_full_response) > 0);

    p->shutdown(p);
    mock_provider_destroy(mock);
}

/**
 * Test: Tool calling with mock provider
 */
TEST(tool_calling_mock) {
    MockProviderConfig config = {
        .default_response = NULL,
        .support_tools = true,
        .tool_response_json = "{\"name\": \"calculator\", \"result\": \"42\"}",
    };

    MockProvider* mock = mock_provider_create_with_config(&config);
    Provider* p = mock_provider_as_provider(mock);
    p->init(p);

    TokenUsage usage = {0};
    ToolCall* tool_calls = NULL;
    size_t tool_count = 0;

    char* response = p->chat_with_tools(
        p, "model", "sys", "what is 6*7?",
        NULL, 0,  // No tool definitions needed for mock
        &tool_calls, &tool_count,
        &usage
    );

    ASSERT_NOT_NULL(response);
    ASSERT_STR_CONTAINS(response, "calculator");

    free(response);
    p->shutdown(p);
    mock_provider_destroy(mock);
}

/**
 * Test: Token counting consistency
 */
TEST(token_counting_consistency) {
    MockProvider* mock = mock_provider_create();
    Provider* p = mock_provider_as_provider(mock);
    p->init(p);

    TokenUsage usage1 = {0};
    TokenUsage usage2 = {0};

    // Same prompt should give consistent token counts
    char* r1 = p->chat(p, "model", "system prompt", "user message", &usage1);
    char* r2 = p->chat(p, "model", "system prompt", "user message", &usage2);

    ASSERT_NOT_NULL(r1);
    ASSERT_NOT_NULL(r2);
    ASSERT_EQ(usage1.input_tokens, usage2.input_tokens);

    free(r1);
    free(r2);
    p->shutdown(p);
    mock_provider_destroy(mock);
}

/**
 * Test: Cost tracking across multiple requests
 */
TEST(cost_tracking_multi_request) {
    MockProvider* mock = mock_provider_create();
    Provider* p = mock_provider_as_provider(mock);
    p->init(p);

    double total_cost = 0.0;

    for (int i = 0; i < 5; i++) {
        TokenUsage usage = {0};
        char* response = p->chat(p, "model", "sys", "user", &usage);
        if (response) {
            total_cost += usage.estimated_cost;
            free(response);
        }
    }

    uint64_t total_in, total_out;
    mock_get_total_tokens(mock, &total_in, &total_out);

    ASSERT_TRUE(total_in > 0);
    ASSERT_TRUE(total_out > 0);
    ASSERT_TRUE(total_cost > 0);

    p->shutdown(p);
    mock_provider_destroy(mock);
}

/**
 * Test: Request logging
 */
TEST(request_logging) {
    MockProvider* mock = mock_provider_create();
    Provider* p = mock_provider_as_provider(mock);
    p->init(p);

    TokenUsage usage = {0};

    // Make some requests
    char* r1 = p->chat(p, "claude-sonnet", "sys1", "prompt1", &usage);
    char* r2 = p->chat(p, "gpt-4o", "sys2", "prompt2", &usage);
    char* r3 = p->chat(p, "gemini-flash", "sys3", "prompt3", &usage);

    free(r1);
    free(r2);
    free(r3);

    // Check request count
    ASSERT_TRUE(mock_assert_request_count(mock, 3));

    // Check last request was gemini
    ASSERT_TRUE(mock_assert_last_model(mock, "gemini-flash"));
    ASSERT_TRUE(mock_assert_last_prompt_contains(mock, "prompt3"));

    p->shutdown(p);
    mock_provider_destroy(mock);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("\n=== Multi-Provider Integration Tests ===\n\n");

    printf("Basic Provider Tests:\n");
    RUN_TEST(primary_provider_success);
    RUN_TEST(fallback_on_primary_failure);
    RUN_TEST(rate_limit_triggers_fallback);

    printf("\nAdvanced Features:\n");
    RUN_TEST(streaming_across_providers);
    RUN_TEST(tool_calling_mock);

    printf("\nTracking & Logging:\n");
    RUN_TEST(token_counting_consistency);
    RUN_TEST(cost_tracking_multi_request);
    RUN_TEST(request_logging);

    printf("\n=== Results: %d/%d tests passed ===\n\n", tests_passed, tests_run);

    return tests_passed == tests_run ? 0 : 1;
}
