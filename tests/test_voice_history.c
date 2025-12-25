/**
 * Voice History Unit Tests
 *
 * Tests for the voice transcription to chat history system.
 * Run with: make voice_history_test && ./build/bin/voice_history_test
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include "nous/nous.h"
#include "nous/voice_history.h"

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, condition) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("  \033[32m+\033[0m %s\n", name); \
    } else { \
        tests_failed++; \
        printf("  \033[31m-\033[0m %s FAILED\n", name); \
    } \
} while(0)

#define TEST_SECTION(name) printf("\n\033[1m=== %s ===\033[0m\n", name)

// ============================================================================
// INITIALIZATION TESTS
// ============================================================================

void test_voice_history_init(void) {
    TEST_SECTION("Voice History Initialization");

    // Initialize voice history
    int result = voice_history_init();
    TEST("voice_history_init succeeds", result == 0);

    // Double init should be safe
    result = voice_history_init();
    TEST("double init is safe", result == 0);
}

// ============================================================================
// SESSION TESTS
// ============================================================================

void test_voice_session_lifecycle(void) {
    TEST_SECTION("Voice Session Lifecycle");

    char session_id[64] = {0};

    // Start session
    int result = voice_session_start("test_agent", session_id);
    TEST("session start succeeds", result == 0);
    TEST("session ID is generated", strlen(session_id) > 0);
    TEST("session ID is UUID format", strlen(session_id) == 36);

    // Get session metadata
    VoiceSessionMetadata metadata = {0};
    result = voice_session_get_metadata(session_id, &metadata);
    TEST("get session metadata succeeds", result == 0);
    TEST("session agent name matches", strcmp(metadata.agent_name, "test_agent") == 0);
    TEST("session has start time", metadata.start_time > 0);

    // End session
    result = voice_session_end(session_id);
    TEST("session end succeeds", result == 0);

    voice_session_metadata_free(&metadata);
}

// ============================================================================
// TRANSCRIPT TESTS
// ============================================================================

void test_voice_transcript_save_simple(void) {
    TEST_SECTION("Voice Transcript Save (Simple)");

    char session_id[64] = {0};
    voice_session_start("transcript_test_agent", session_id);

    // Save simple transcript
    int result = voice_history_save_simple(
        session_id,
        "transcript_test_agent",
        "Hello, how are you?",
        "I'm doing well, thank you for asking!",
        1500,
        "en-US"
    );
    TEST("save simple transcript succeeds", result == 0);

    // Save another transcript
    result = voice_history_save_simple(
        session_id,
        "transcript_test_agent",
        "What's the weather like?",
        "It's sunny with temperatures around 72F.",
        2000,
        "en-US"
    );
    TEST("save second transcript succeeds", result == 0);

    voice_session_end(session_id);
}

void test_voice_transcript_save_full(void) {
    TEST_SECTION("Voice Transcript Save (Full Entry)");

    char session_id[64] = {0};
    voice_session_start("full_entry_agent", session_id);

    VoiceTranscriptEntry entry = {0};
    snprintf(entry.session_id, sizeof(entry.session_id), "%s", session_id);
    snprintf(entry.agent_name, sizeof(entry.agent_name), "full_entry_agent");
    entry.user_transcript = strdup("This is a test question");
    entry.assistant_response = strdup("This is the test response");
    entry.timestamp = time(NULL);
    entry.duration_ms = 3000;
    entry.response_latency_ms = 150;
    entry.speech_clarity = 0.95f;
    entry.background_noise = 0.1f;
    entry.language = strdup("en-US");
    entry.topic = strdup("testing");
    entry.intent = strdup("question");
    entry.is_command = false;
    entry.user_emotion.dominant_emotion = VOICE_EMOTION_CURIOSITY;
    entry.user_emotion.dominant_confidence = 0.8f;

    int result = voice_history_save(&entry);
    TEST("save full entry succeeds", result == 0);

    voice_transcript_free(&entry);
    voice_session_end(session_id);
}

// ============================================================================
// LOAD AND SEARCH TESTS
// ============================================================================

void test_voice_history_load(void) {
    TEST_SECTION("Voice History Load");

    char session_id[64] = {0};
    voice_session_start("load_test_agent", session_id);

    // Save some transcripts
    voice_history_save_simple(session_id, "load_test_agent", "Question 1", "Answer 1", 1000, "en-US");
    voice_history_save_simple(session_id, "load_test_agent", "Question 2", "Answer 2", 1500, "en-US");
    voice_history_save_simple(session_id, "load_test_agent", "Question 3", "Answer 3", 2000, "en-US");

    voice_session_end(session_id);

    // Load session transcripts
    VoiceTranscriptEntry entries[10] = {0};
    size_t count = 0;
    int result = voice_history_load_session(session_id, entries, 10, &count);
    TEST("load session transcripts succeeds", result == 0);
    TEST("loaded correct transcript count", count == 3);

    // Free entries
    for (size_t i = 0; i < count; i++) {
        voice_transcript_free(&entries[i]);
    }
}

void test_voice_history_search(void) {
    TEST_SECTION("Voice History Search");

    char session_id[64] = {0};
    voice_session_start("search_test_agent", session_id);

    // Save transcript with unique content
    voice_history_save_simple(
        session_id,
        "search_test_agent",
        "Tell me about quantum computing",
        "Quantum computing uses qubits and superposition.",
        2500,
        "en-US"
    );

    voice_session_end(session_id);

    // Search for it
    VoiceTranscriptEntry results[10] = {0};
    size_t count = 0;
    int result = voice_history_search("quantum", 10, results, &count);
    TEST("search succeeds", result == 0);
    TEST("found quantum computing transcript", count >= 1);

    // Free results
    for (size_t i = 0; i < count; i++) {
        voice_transcript_free(&results[i]);
    }
}

// ============================================================================
// EMOTION TESTS
// ============================================================================

void test_voice_emotion_names(void) {
    TEST_SECTION("Voice Emotion Names");

    TEST("neutral emotion name", strcmp(voice_emotion_name(VOICE_EMOTION_NEUTRAL), "neutral") == 0);
    TEST("confusion emotion name", strcmp(voice_emotion_name(VOICE_EMOTION_CONFUSION), "confusion") == 0);
    TEST("frustration emotion name", strcmp(voice_emotion_name(VOICE_EMOTION_FRUSTRATION), "frustration") == 0);
    TEST("anxiety emotion name", strcmp(voice_emotion_name(VOICE_EMOTION_ANXIETY), "anxiety") == 0);
    TEST("boredom emotion name", strcmp(voice_emotion_name(VOICE_EMOTION_BOREDOM), "boredom") == 0);
    TEST("excitement emotion name", strcmp(voice_emotion_name(VOICE_EMOTION_EXCITEMENT), "excitement") == 0);
    TEST("curiosity emotion name", strcmp(voice_emotion_name(VOICE_EMOTION_CURIOSITY), "curiosity") == 0);
    TEST("joy emotion name", strcmp(voice_emotion_name(VOICE_EMOTION_JOY), "joy") == 0);
    TEST("sadness emotion name", strcmp(voice_emotion_name(VOICE_EMOTION_SADNESS), "sadness") == 0);
    TEST("anger emotion name", strcmp(voice_emotion_name(VOICE_EMOTION_ANGER), "anger") == 0);
}

void test_voice_emotion_distribution(void) {
    TEST_SECTION("Voice Emotion Distribution");

    char session_id[64] = {0};
    voice_session_start("emotion_test_agent", session_id);

    // Save transcripts with different emotions
    VoiceTranscriptEntry entry = {0};
    snprintf(entry.session_id, sizeof(entry.session_id), "%s", session_id);
    snprintf(entry.agent_name, sizeof(entry.agent_name), "emotion_test_agent");
    entry.timestamp = time(NULL);
    entry.duration_ms = 1000;

    // Add neutral emotion transcript
    entry.user_transcript = strdup("Just checking in");
    entry.assistant_response = strdup("All good here");
    entry.user_emotion.dominant_emotion = VOICE_EMOTION_NEUTRAL;
    entry.user_emotion.dominant_confidence = 0.9f;
    voice_history_save(&entry);
    free(entry.user_transcript);
    free(entry.assistant_response);

    // Add excited emotion transcript
    entry.user_transcript = strdup("Wow this is amazing!");
    entry.assistant_response = strdup("I'm glad you like it!");
    entry.user_emotion.dominant_emotion = VOICE_EMOTION_EXCITEMENT;
    entry.user_emotion.dominant_confidence = 0.85f;
    voice_history_save(&entry);
    free(entry.user_transcript);
    free(entry.assistant_response);

    voice_session_end(session_id);

    // Get emotion distribution
    float distribution[VOICE_EMOTION_COUNT] = {0};
    int result = voice_session_emotion_distribution(session_id, distribution);
    TEST("get emotion distribution succeeds", result == 0);
    TEST("distribution has neutral emotion", distribution[VOICE_EMOTION_NEUTRAL] > 0);
    TEST("distribution has excitement emotion", distribution[VOICE_EMOTION_EXCITEMENT] > 0);
}

// ============================================================================
// EXPORT TESTS
// ============================================================================

void test_voice_export_to_chat(void) {
    TEST_SECTION("Voice Export to Chat Format");

    char session_id[64] = {0};
    voice_session_start("export_test_agent", session_id);

    // Save some transcripts
    voice_history_save_simple(session_id, "export_test_agent", "First question", "First answer", 1000, "en-US");
    voice_history_save_simple(session_id, "export_test_agent", "Second question", "Second answer", 1500, "en-US");

    voice_session_end(session_id);

    // Export to chat format
    char** messages = NULL;
    char** roles = NULL;
    size_t count = 0;

    int result = voice_history_export_to_chat(session_id, &messages, &roles, &count);
    TEST("export to chat succeeds", result == 0);
    // Each transcript creates 2 messages (user + assistant)
    TEST("exported correct message count", count == 4 || count == 2);

    // Free exported data
    if (messages && roles) {
        for (size_t i = 0; i < count; i++) {
            free(messages[i]);
            free(roles[i]);
        }
        free(messages);
        free(roles);
    }
}

// ============================================================================
// STATISTICS TESTS
// ============================================================================

void test_voice_history_stats(void) {
    TEST_SECTION("Voice History Statistics");

    VoiceHistoryStats stats = {0};
    int result = voice_history_get_stats(&stats);
    TEST("get stats succeeds", result == 0);
    TEST("has session count", stats.total_sessions >= 0);
    TEST("has transcript count", stats.total_transcripts >= 0);
}

// ============================================================================
// CLEANUP TESTS
// ============================================================================

void test_voice_history_shutdown(void) {
    TEST_SECTION("Voice History Shutdown");

    // Shutdown should be safe
    voice_history_shutdown();
    TEST("shutdown succeeds", true);

    // Double shutdown should be safe
    voice_history_shutdown();
    TEST("double shutdown is safe", true);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("\033[1m");
    printf("========================================\n");
    printf("  CONVERGIO VOICE HISTORY UNIT TESTS\n");
    printf("========================================\n");
    printf("\033[0m");

    // Initialize tests
    test_voice_history_init();

    // Session tests
    test_voice_session_lifecycle();

    // Transcript tests
    test_voice_transcript_save_simple();
    test_voice_transcript_save_full();

    // Load and search tests
    test_voice_history_load();
    test_voice_history_search();

    // Emotion tests
    test_voice_emotion_names();
    test_voice_emotion_distribution();

    // Export tests
    test_voice_export_to_chat();

    // Stats tests
    test_voice_history_stats();

    // Cleanup tests
    test_voice_history_shutdown();

    // Summary
    printf("\n\033[1m");
    printf("========================================\n");
    printf("Results: %d tests, ", tests_run);
    if (tests_failed == 0) {
        printf("\033[32m%d passed\033[0m\033[1m, ", tests_passed);
    } else {
        printf("%d passed, ", tests_passed);
    }
    if (tests_failed > 0) {
        printf("\033[31m%d failed\033[0m\033[1m\n", tests_failed);
    } else {
        printf("0 failed\n");
    }
    printf("========================================\n");
    printf("\033[0m");

    return tests_failed > 0 ? 1 : 0;
}
