/**
 * CONVERGIO GROUP CHAT
 *
 * AutoGen-inspired multi-agent conversation system
 * Supports round-robin, priority-based, and consensus-building modes
 */

#ifndef CONVERGIO_GROUP_CHAT_H
#define CONVERGIO_GROUP_CHAT_H

#include "nous/orchestrator.h"
#include "nous/nous.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// ============================================================================
// GROUP CHAT MODES
// ============================================================================

typedef enum {
    GROUP_CHAT_ROUND_ROBIN = 0,   // Take turns in order
    GROUP_CHAT_PRIORITY = 1,       // Priority-based speaking
    GROUP_CHAT_CONSENSUS = 2,      // Build consensus
    GROUP_CHAT_DEBATE = 3          // Structured debate
} GroupChatMode;

// ============================================================================
// GROUP CHAT STRUCTURE
// ============================================================================

typedef struct {
    uint64_t chat_id;
    SemanticID* participants;
    size_t participant_count;
    size_t participant_capacity;
    GroupChatMode mode;
    Message** message_history;
    size_t message_count;
    size_t message_capacity;
    int current_round;
    int max_rounds;
    bool consensus_reached;
    double consensus_threshold;
    time_t created_at;
    time_t last_message_at;
} GroupChat;

// ============================================================================
// GROUP CHAT OPERATIONS
// ============================================================================

// Create group chat
GroupChat* group_chat_create(SemanticID* participants, size_t count, GroupChatMode mode);

// Destroy group chat
void group_chat_destroy(GroupChat* chat);

// Add message to chat
int group_chat_add_message(GroupChat* chat, SemanticID sender, const char* content);

// Get next speaker based on mode
SemanticID group_chat_get_next_speaker(GroupChat* chat);

// Check if consensus reached
bool group_chat_check_consensus(GroupChat* chat, double threshold);

// Get chat summary
char* group_chat_get_summary(GroupChat* chat);

// ============================================================================
// CONSENSUS DETECTION
// ============================================================================

// Vote on a proposal
int group_chat_vote(GroupChat* chat, SemanticID voter, const char* proposal, bool approve);

// Get consensus status
bool group_chat_has_consensus(GroupChat* chat);

// Get agreement percentage
double group_chat_agreement_percentage(GroupChat* chat);

#endif // CONVERGIO_GROUP_CHAT_H

