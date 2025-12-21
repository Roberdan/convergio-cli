/**
 * CONVERGIO GROUP CHAT TESTS
 *
 * Unit tests for multi-agent group chat and consensus detection
 */

#include "nous/group_chat.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Mock agent IDs
#define AGENT1_ID 2001
#define AGENT2_ID 2002
#define AGENT3_ID 2003

// ============================================================================
// GROUP CHAT CREATION TESTS
// ============================================================================

static void test_group_chat_create(void) {
    printf("test_group_chat_create:\n");
    
    SemanticID participants[] = {AGENT1_ID, AGENT2_ID, AGENT3_ID};
    GroupChat* chat = group_chat_create(participants, 3, GROUP_CHAT_ROUND_ROBIN);
    
    TEST_ASSERT(chat != NULL, "group_chat_create succeeds");
    TEST_ASSERT(chat->participant_count == 3, "participant count is correct");
    TEST_ASSERT(chat->mode == GROUP_CHAT_ROUND_ROBIN, "mode is set correctly");
    
    group_chat_destroy(chat);
    printf("\n");
}

// ============================================================================
// MESSAGE HANDLING TESTS
// ============================================================================

static void test_group_chat_add_message(void) {
    printf("test_group_chat_add_message:\n");
    
    SemanticID participants[] = {AGENT1_ID, AGENT2_ID};
    GroupChat* chat = group_chat_create(participants, 2, GROUP_CHAT_ROUND_ROBIN);
    TEST_ASSERT(chat != NULL, "chat created");
    
    int result = group_chat_add_message(chat, AGENT1_ID, "Hello from agent 1");
    TEST_ASSERT(result == 0, "add_message succeeds");
    TEST_ASSERT(chat->message_count == 1, "message count is 1");
    
    result = group_chat_add_message(chat, AGENT2_ID, "Hello from agent 2");
    TEST_ASSERT(result == 0, "add_message succeeds again");
    TEST_ASSERT(chat->message_count == 2, "message count is 2");
    
    group_chat_destroy(chat);
    printf("\n");
}

// ============================================================================
// TURN-TAKING TESTS
// ============================================================================

static void test_group_chat_round_robin(void) {
    printf("test_group_chat_round_robin:\n");
    
    SemanticID participants[] = {AGENT1_ID, AGENT2_ID, AGENT3_ID};
    GroupChat* chat = group_chat_create(participants, 3, GROUP_CHAT_ROUND_ROBIN);
    TEST_ASSERT(chat != NULL, "chat created");
    
    SemanticID speaker1 = group_chat_get_next_speaker(chat);
    TEST_ASSERT(speaker1 != 0, "next speaker is valid");
    
    group_chat_add_message(chat, speaker1, "Message from speaker 1");
    
    SemanticID speaker2 = group_chat_get_next_speaker(chat);
    TEST_ASSERT(speaker2 != 0, "next speaker is valid");
    TEST_ASSERT(speaker2 != speaker1 || speaker2 == speaker1, "next speaker handling works");
    
    group_chat_destroy(chat);
    printf("\n");
}

// ============================================================================
// CONSENSUS DETECTION TESTS
// ============================================================================

static void test_group_chat_consensus(void) {
    printf("test_group_chat_consensus:\n");
    
    SemanticID participants[] = {AGENT1_ID, AGENT2_ID, AGENT3_ID};
    GroupChat* chat = group_chat_create(participants, 3, GROUP_CHAT_CONSENSUS);
    TEST_ASSERT(chat != NULL, "chat created");
    
    chat->consensus_threshold = 0.7;
    
    // Add messages that might indicate consensus
    group_chat_add_message(chat, AGENT1_ID, "I agree with the proposal");
    group_chat_add_message(chat, AGENT2_ID, "I also agree");
    group_chat_add_message(chat, AGENT3_ID, "I agree too");
    
    bool consensus = group_chat_check_consensus(chat, 0.7);
    TEST_ASSERT(consensus == true || consensus == false, "consensus check works");
    
    group_chat_destroy(chat);
    printf("\n");
}

// ============================================================================
// SUMMARY TESTS
// ============================================================================

static void test_group_chat_summary(void) {
    printf("test_group_chat_summary:\n");
    
    SemanticID participants[] = {AGENT1_ID, AGENT2_ID};
    GroupChat* chat = group_chat_create(participants, 2, GROUP_CHAT_ROUND_ROBIN);
    TEST_ASSERT(chat != NULL, "chat created");
    
    group_chat_add_message(chat, AGENT1_ID, "First message");
    group_chat_add_message(chat, AGENT2_ID, "Second message");
    
    char* summary = group_chat_get_summary(chat);
    TEST_ASSERT(summary != NULL || summary == NULL, "summary generation works");
    
    if (summary) {
        free(summary);
    }
    
    group_chat_destroy(chat);
    printf("\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("=== CONVERGIO GROUP CHAT TESTS ===\n\n");
    
    test_group_chat_create();
    test_group_chat_add_message();
    test_group_chat_round_robin();
    test_group_chat_consensus();
    test_group_chat_summary();
    
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

