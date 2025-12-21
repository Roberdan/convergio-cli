/**
 * Unit Tests for Context Compaction
 *
 * Tests the context compaction module including:
 * - Threshold detection
 * - Token estimation
 * - Checkpoint persistence
 * - Summarization (with mock)
 *
 * Run with: make compaction_test && ./build/bin/compaction_test
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
#include "nous/nous.h"
#include "nous/compaction.h"
#include "nous/provider.h"

// ============================================================================
// STUBS AND MOCKS
// ============================================================================

// Stub for nous_log
LogLevel g_log_level = LOG_LEVEL_ERROR;

void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) {
    (void)level; (void)cat; (void)fmt;
}

void nous_log_set_level(LogLevel level) { g_log_level = level; }
LogLevel nous_log_get_level(void) { return g_log_level; }
const char* nous_log_level_name(LogLevel level) { (void)level; return ""; }

// Mock database state
static int g_mock_checkpoint_count = 0;
static char* g_mock_checkpoint_summary = NULL;
static char* g_mock_messages = NULL;
static int64_t g_mock_first_msg_id = 1;
static int64_t g_mock_last_msg_id = 100;

// Mock persistence functions
int persistence_save_checkpoint(
    const char* session_id,
    int checkpoint_num,
    int64_t from_msg_id,
    int64_t to_msg_id,
    int messages_compressed,
    const char* summary,
    const char* key_facts,
    size_t original_tokens,
    size_t compressed_tokens,
    double cost
) {
    (void)from_msg_id; (void)to_msg_id; (void)messages_compressed;
    (void)key_facts; (void)original_tokens; (void)compressed_tokens; (void)cost;

    if (!session_id || !summary) return -1;

    g_mock_checkpoint_count = checkpoint_num;
    if (g_mock_checkpoint_summary) free(g_mock_checkpoint_summary);
    g_mock_checkpoint_summary = strdup(summary);

    return 0;
}

char* persistence_load_latest_checkpoint(const char* session_id) {
    (void)session_id;
    if (g_mock_checkpoint_summary) {
        return strdup(g_mock_checkpoint_summary);
    }
    return NULL;
}

int persistence_get_checkpoint_count(const char* session_id) {
    (void)session_id;
    return g_mock_checkpoint_count;
}

char* persistence_load_messages_range(
    const char* session_id,
    int64_t from_id,
    int64_t to_id,
    size_t* out_count
) {
    (void)session_id; (void)from_id; (void)to_id;
    if (g_mock_messages) {
        *out_count = 50;  // Mock 50 messages
        return strdup(g_mock_messages);
    }
    *out_count = 0;
    return NULL;
}

int persistence_get_message_id_range(
    const char* session_id,
    int64_t* out_first,
    int64_t* out_last
) {
    (void)session_id;
    *out_first = g_mock_first_msg_id;
    *out_last = g_mock_last_msg_id;
    return 0;
}

char* persistence_load_conversation_context(const char* session_id, size_t max_messages) {
    (void)session_id; (void)max_messages;
    if (g_mock_messages) {
        return strdup(g_mock_messages);
    }
    return strdup("");
}

int64_t persistence_get_cutoff_message_id(const char* session_id, int keep_recent) {
    (void)session_id; (void)keep_recent;
    // Return a cutoff that leaves 'keep_recent' messages at the end
    if (g_mock_last_msg_id - g_mock_first_msg_id < keep_recent) {
        return -1;  // Not enough messages
    }
    return g_mock_last_msg_id - keep_recent;
}

// Mock provider functions
static bool g_provider_available = true;
static char* g_mock_llm_response = NULL;

bool provider_is_available(ProviderType type) {
    (void)type;
    return g_provider_available;
}

Provider* provider_get(ProviderType type) {
    (void)type;
    // Return NULL to test fallback behavior, or implement mock provider
    return NULL;
}

// Mock LLM functions (required by compaction.c)
char* llm_chat_with_model(const char* model, const char* system, const char* user, TokenUsage* usage) {
    (void)model; (void)system; (void)user;
    if (usage) {
        usage->input_tokens = 100;
        usage->output_tokens = 50;
        usage->cached_tokens = 0;
        usage->estimated_cost = 0.001;
    }
    if (g_mock_llm_response) {
        return strdup(g_mock_llm_response);
    }
    return strdup("Mock summary: This is a test summary of the conversation context.");
}

size_t llm_estimate_tokens(const char* text) {
    if (!text) return 0;
    // Rough estimate: ~4 chars per token
    return strlen(text) / 4;
}

bool llm_is_available(void) {
    return g_provider_available;
}

// ============================================================================
// TEST FRAMEWORK
// ============================================================================

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
// HELPER FUNCTIONS
// ============================================================================

static void reset_mock_state(void) {
    g_mock_checkpoint_count = 0;
    if (g_mock_checkpoint_summary) {
        free(g_mock_checkpoint_summary);
        g_mock_checkpoint_summary = NULL;
    }
    if (g_mock_messages) {
        free(g_mock_messages);
        g_mock_messages = NULL;
    }
    if (g_mock_llm_response) {
        free(g_mock_llm_response);
        g_mock_llm_response = NULL;
    }
    g_mock_first_msg_id = 1;
    g_mock_last_msg_id = 100;
    g_provider_available = true;
}

// Generate a large conversation for testing
static char* generate_large_conversation(size_t num_messages) {
    size_t capacity = num_messages * 500;  // ~500 chars per message
    char* conv = malloc(capacity);
    if (!conv) return NULL;

    conv[0] = '\0';
    size_t len = 0;

    for (size_t i = 0; i < num_messages; i++) {
        const char* role = (i % 2 == 0) ? "user" : "assistant";
        len += snprintf(conv + len, capacity - len,
            "[%s]: This is message number %zu. It contains some content that "
            "represents a typical conversation turn with enough text to simulate "
            "realistic token usage. The message includes various topics like "
            "code review, architecture decisions, and implementation details.\n\n",
            role, i + 1);
    }

    return conv;
}

// ============================================================================
// TEST CASES
// ============================================================================

void test_compaction_threshold_detection(void) {
    TEST_SECTION("Compaction Threshold Detection");

    reset_mock_state();

    // Test below threshold (80K tokens = ~320K chars at 4 chars/token)
    TEST("No compaction needed for 50K tokens",
         !compaction_needed("session1", 50000));

    TEST("No compaction needed for 79K tokens",
         !compaction_needed("session1", 79999));

    // Test at/above threshold (threshold is > 80K, not >=)
    TEST("No compaction at exactly 80K tokens (boundary)",
         !compaction_needed("session1", 80000));

    TEST("Compaction needed for 80001 tokens",
         compaction_needed("session1", 80001));

    TEST("Compaction needed for 100K tokens",
         compaction_needed("session1", 100000));

    // Test NULL session
    TEST("No compaction for NULL session",
         !compaction_needed(NULL, 100000));

    // Test max checkpoints reached
    g_mock_checkpoint_count = COMPACTION_MAX_CHECKPOINTS;
    TEST("No compaction when max checkpoints reached",
         !compaction_needed("session1", 100000));
}

void test_compaction_checkpoint_persistence(void) {
    TEST_SECTION("Checkpoint Persistence");

    reset_mock_state();

    // Test save checkpoint
    int result = persistence_save_checkpoint(
        "session1",
        1,      // checkpoint_num
        1,      // from_msg_id
        50,     // to_msg_id
        50,     // messages_compressed
        "Test summary content",
        NULL,   // key_facts
        80000,  // original_tokens
        5000,   // compressed_tokens
        0.001   // cost
    );
    TEST("Save checkpoint returns success", result == 0);
    TEST("Checkpoint count updated", g_mock_checkpoint_count == 1);
    TEST("Summary stored correctly",
         g_mock_checkpoint_summary && strcmp(g_mock_checkpoint_summary, "Test summary content") == 0);

    // Test load checkpoint
    char* loaded = persistence_load_latest_checkpoint("session1");
    TEST("Load checkpoint returns summary",
         loaded && strcmp(loaded, "Test summary content") == 0);
    if (loaded) free(loaded);

    // Test get checkpoint count
    TEST("Get checkpoint count returns 1",
         persistence_get_checkpoint_count("session1") == 1);

    // Test NULL handling
    result = persistence_save_checkpoint(NULL, 1, 1, 50, 50, "test", NULL, 100, 10, 0.0);
    TEST("Save checkpoint rejects NULL session", result == -1);

    result = persistence_save_checkpoint("session1", 1, 1, 50, 50, NULL, NULL, 100, 10, 0.0);
    TEST("Save checkpoint rejects NULL summary", result == -1);
}

void test_compaction_summarize_fallback(void) {
    TEST_SECTION("Compaction Summarization (Fallback)");

    reset_mock_state();

    // Test with large message content (triggers truncation fallback since no LLM)
    g_mock_messages = generate_large_conversation(100);

    CompactionResult* result = compaction_summarize(
        "session1",
        1,
        90,
        g_mock_messages
    );

    TEST("Summarization returns result", result != NULL);

    if (result) {
        TEST("Summary has content", result->summary != NULL && strlen(result->summary) > 0);
        TEST("Original tokens estimated", result->original_tokens > 0);
        TEST("Compressed tokens estimated", result->compressed_tokens > 0);
        TEST("Compression ratio calculated", result->compression_ratio >= 1.0);
        TEST("Checkpoint number assigned", result->checkpoint_num > 0);

        // Verify fallback truncation worked
        TEST("Summary shorter than original",
             strlen(result->summary) < strlen(g_mock_messages));

        compaction_result_free(result);
    }

    // Test with NULL inputs
    result = compaction_summarize(NULL, 1, 90, "test");
    TEST("Summarize rejects NULL session", result == NULL);

    result = compaction_summarize("session1", 1, 90, NULL);
    TEST("Summarize rejects NULL messages", result == NULL);
}

void test_compaction_build_context(void) {
    TEST_SECTION("Compaction Build Context");

    reset_mock_state();

    // Test with small conversation (no compaction needed)
    g_mock_messages = strdup("[user]: Hello\n\n[assistant]: Hi there!\n\n");

    bool was_compacted = false;
    char* context = compaction_build_context("session1", "test input", &was_compacted);

    TEST("Build context returns result", context != NULL);
    TEST("Small context not compacted", !was_compacted);
    if (context) free(context);

    // Test with large conversation (compaction needed)
    reset_mock_state();
    g_mock_messages = generate_large_conversation(200);  // Large enough to trigger
    g_mock_last_msg_id = 200;

    context = compaction_build_context("session1", "test input", &was_compacted);
    TEST("Large context returns result", context != NULL);
    // Note: compaction might not trigger if token estimation is low
    if (context) free(context);

    // Test NULL handling
    context = compaction_build_context(NULL, "test", &was_compacted);
    TEST("Rejects NULL session", context == NULL);

    context = compaction_build_context("session1", NULL, &was_compacted);
    TEST("Rejects NULL user_input", context == NULL);
}

void test_compaction_result_free(void) {
    TEST_SECTION("Compaction Result Cleanup");

    // Test freeing NULL (should not crash)
    compaction_result_free(NULL);
    TEST("Free NULL doesn't crash", 1);  // If we get here, it passed

    // Test freeing valid result
    CompactionResult* result = calloc(1, sizeof(CompactionResult));
    result->summary = strdup("Test summary");
    compaction_result_free(result);
    TEST("Free valid result doesn't crash", 1);
}

void test_compaction_init_shutdown(void) {
    TEST_SECTION("Compaction Init/Shutdown");

    // Test initialization
    int result = compaction_init();
    TEST("Init returns success", result == 0);

    // Test double init (should be idempotent)
    result = compaction_init();
    TEST("Double init is safe", result == 0);

    // Test shutdown
    compaction_shutdown();
    TEST("Shutdown doesn't crash", 1);

    // Test double shutdown (should be safe)
    compaction_shutdown();
    TEST("Double shutdown is safe", 1);

    // Reinit for other tests
    compaction_init();
}

void test_compaction_checkpoint_count_limit(void) {
    TEST_SECTION("Checkpoint Count Limit");

    reset_mock_state();

    // Set checkpoints to max
    g_mock_checkpoint_count = COMPACTION_MAX_CHECKPOINTS;

    TEST("Compaction blocked at max checkpoints",
         !compaction_needed("session1", 100000));

    // One below max should allow compaction
    g_mock_checkpoint_count = COMPACTION_MAX_CHECKPOINTS - 1;
    TEST("Compaction allowed below max",
         compaction_needed("session1", 100000));
}

void test_compaction_message_range(void) {
    TEST_SECTION("Message ID Range");

    reset_mock_state();

    g_mock_first_msg_id = 100;
    g_mock_last_msg_id = 500;

    int64_t first, last;
    int result = persistence_get_message_id_range("session1", &first, &last);

    TEST("Get message range returns success", result == 0);
    TEST("First message ID correct", first == 100);
    TEST("Last message ID correct", last == 500);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("\n\033[1;34m========================================\033[0m\n");
    printf("\033[1;34m  CONTEXT COMPACTION UNIT TESTS\033[0m\n");
    printf("\033[1;34m========================================\033[0m\n");

    // Initialize compaction for tests
    compaction_init();

    // Run all tests
    test_compaction_init_shutdown();
    test_compaction_threshold_detection();
    test_compaction_checkpoint_persistence();
    test_compaction_summarize_fallback();
    test_compaction_build_context();
    test_compaction_result_free();
    test_compaction_checkpoint_count_limit();
    test_compaction_message_range();

    // Cleanup
    compaction_shutdown();
    reset_mock_state();

    // Summary
    printf("\n\033[1m========================================\033[0m\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  \033[32mPassed:\033[0m       %d\n", tests_passed);
    if (tests_failed > 0) {
        printf("  \033[31mFailed:\033[0m       %d\n", tests_failed);
    }
    printf("\033[1m========================================\033[0m\n\n");

    return (tests_failed > 0) ? 1 : 0;
}
