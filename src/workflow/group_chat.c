/**
 * CONVERGIO GROUP CHAT
 *
 * Multi-agent conversation with consensus building
 * Thread-safe implementation with fair agent selection
 */

#include "nous/group_chat.h"
#include "nous/orchestrator.h"
#include "nous/nous.h"
#include "nous/debug_mutex.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdatomic.h>

// workflow_strdup is defined in workflow_types.c
extern char* workflow_strdup(const char* str);

// FIX-02: Atomic chat ID generation to prevent race conditions
static _Atomic uint64_t g_next_chat_id = 1;

// FIX-02: Thread-safe atomic chat ID allocation
static uint64_t allocate_chat_id(void) {
    return atomic_fetch_add(&g_next_chat_id, 1);
}

// ============================================================================
// GROUP CHAT CREATION
// ============================================================================

GroupChat* group_chat_create(SemanticID* participants, size_t count, GroupChatMode mode) {
    if (!participants || count == 0) {
        return NULL;
    }
    
    GroupChat* chat = calloc(1, sizeof(GroupChat));
    if (!chat) {
        return NULL;
    }
    
    // MEDIUM-03: Use thread-safe ID allocation
    chat->chat_id = allocate_chat_id();
    chat->mode = mode;
    chat->current_round = 0;
    chat->max_rounds = 10;
    chat->consensus_reached = false;
    chat->consensus_threshold = 0.75; // 75% agreement
    chat->created_at = time(NULL);
    chat->last_message_at = chat->created_at;
    
    chat->participant_capacity = count;
    chat->participants = calloc(count, sizeof(SemanticID));
    if (!chat->participants) {
        free(chat);
        return NULL;
    }
    
    memcpy(chat->participants, participants, count * sizeof(SemanticID));
    chat->participant_count = count;
    
    chat->message_capacity = 64;
    chat->message_history = calloc(chat->message_capacity, sizeof(Message*));
    if (!chat->message_history) {
        free(chat->participants);
        free(chat);
        return NULL;
    }

    chat->message_count = 0;

    // Fair agent selection: Initialize participation tracking
    chat->participation_count = calloc(count, sizeof(size_t));
    if (!chat->participation_count) {
        free(chat->message_history);
        free(chat->participants);
        free(chat);
        return NULL;
    }
    chat->total_participations = 0;

    return chat;
}

void group_chat_destroy(GroupChat* chat) {
    if (!chat) {
        return;
    }

    if (chat->participants) {
        free(chat->participants);
        chat->participants = NULL;
    }

    if (chat->message_history) {
        // Messages are managed by orchestrator, don't free here
        free(chat->message_history);
        chat->message_history = NULL;
    }

    // Free participation tracking
    if (chat->participation_count) {
        free(chat->participation_count);
        chat->participation_count = NULL;
    }

    free(chat);
    chat = NULL;
}

// ============================================================================
// MESSAGE MANAGEMENT
// ============================================================================

int group_chat_add_message(GroupChat* chat, SemanticID sender, const char* content) {
    if (!chat || !content) {
        return -1;
    }
    
    // Verify sender is participant
    bool is_participant = false;
    for (size_t i = 0; i < chat->participant_count; i++) {
        if (chat->participants[i] == sender) {
            is_participant = true;
            break;
        }
    }
    
    if (!is_participant) {
        return -1;
    }
    
    // Create message
    Message* msg = message_create(MSG_TYPE_AGENT_RESPONSE, sender, 0, content);
    if (!msg) {
        return -1;
    }
    
    // Add to history
    if (chat->message_count >= chat->message_capacity) {
        size_t new_capacity = chat->message_capacity * 2;
        Message** new_history = realloc(chat->message_history, new_capacity * sizeof(Message*));
        if (!new_history) {
            // Message will be cleaned up by orchestrator
            return -1;
        }
        chat->message_history = new_history;
        chat->message_capacity = new_capacity;
    }
    
    chat->message_history[chat->message_count++] = msg;
    chat->last_message_at = time(NULL);

    // Fair agent selection: Track participation
    for (size_t i = 0; i < chat->participant_count; i++) {
        if (chat->participants[i] == sender) {
            chat->participation_count[i]++;
            chat->total_participations++;
            break;
        }
    }

    // Send message via message bus (only if orchestrator is active)
    // Note: message_send destroys the message if orchestrator is NULL,
    // but we've already stored it in our history, so skip if no orchestrator
    Orchestrator* orch = orchestrator_get();
    if (orch) {
        message_send(msg);
    }

    return 0;
}

// ============================================================================
// TURN-TAKING
// ============================================================================

SemanticID group_chat_get_next_speaker(GroupChat* chat) {
    if (!chat || chat->participant_count == 0) {
        return 0;
    }
    
    switch (chat->mode) {
        case GROUP_CHAT_ROUND_ROBIN: {
            // Round-robin: cycle through participants
            int speaker_idx = chat->current_round % (int)chat->participant_count;
            return chat->participants[speaker_idx];
        }
        
        case GROUP_CHAT_PRIORITY: {
            // Priority: return first participant (simplified)
            // Full implementation would use priority queue
            return chat->participants[0];
        }
        
        case GROUP_CHAT_CONSENSUS:
        case GROUP_CHAT_DEBATE: {
            // Fair agent selection with bias prevention
            // Prioritize participants who have spoken least to ensure fair distribution
            if (chat->message_count == 0 || !chat->participation_count) {
                return chat->participants[0];
            }

            // Find participant with minimum participation count (bias prevention)
            size_t min_count = SIZE_MAX;
            size_t min_idx = 0;

            // Calculate expected participation per agent
            double expected = (double)chat->total_participations / (double)chat->participant_count;

            for (size_t i = 0; i < chat->participant_count; i++) {
                size_t count = chat->participation_count[i];
                // Bias prevention: strongly prefer underrepresented participants
                if (count < min_count) {
                    min_count = count;
                    min_idx = i;
                }
                // If counts are equal, check deviation from expected (secondary bias check)
                else if (count == min_count && (double)count < expected) {
                    min_idx = i;
                }
            }

            return chat->participants[min_idx];
        }
        
        default:
            return chat->participants[0];
    }
}

// ============================================================================
// CONSENSUS DETECTION
// ============================================================================

bool group_chat_check_consensus(GroupChat* chat, double threshold) {
    if (!chat || chat->message_count < chat->participant_count) {
        return false;
    }
    
    // Simple consensus: check if recent messages express agreement
    // Full implementation would use LLM to analyze sentiment
    
    size_t agreement_count = 0;
    size_t recent_messages = chat->participant_count;
    if (recent_messages > chat->message_count) {
        recent_messages = chat->message_count;
    }
    
    // Check last N messages (one per participant)
    for (size_t i = chat->message_count - recent_messages; i < chat->message_count; i++) {
        if (chat->message_history[i] && chat->message_history[i]->content) {
            const char* content = chat->message_history[i]->content;
            // Simple keyword-based agreement detection
            if (strstr(content, "agree") || strstr(content, "yes") || 
                strstr(content, "approve") || strstr(content, "consensus")) {
                agreement_count++;
            }
        }
    }
    
    double agreement_ratio = (double)agreement_count / (double)recent_messages;
    chat->consensus_reached = (agreement_ratio >= threshold);
    
    return chat->consensus_reached;
}

bool group_chat_has_consensus(GroupChat* chat) {
    if (!chat) {
        return false;
    }
    return group_chat_check_consensus(chat, chat->consensus_threshold);
}

double group_chat_agreement_percentage(GroupChat* chat) {
    if (!chat || chat->message_count == 0) {
        return 0.0;
    }
    
    size_t agreement_count = 0;
    for (size_t i = 0; i < chat->message_count; i++) {
        if (chat->message_history[i] && chat->message_history[i]->content) {
            const char* content = chat->message_history[i]->content;
            if (strstr(content, "agree") || strstr(content, "yes") || 
                strstr(content, "approve")) {
                agreement_count++;
            }
        }
    }
    
    return (double)agreement_count / (double)chat->message_count;
}

int group_chat_vote(GroupChat* chat, SemanticID voter, const char* proposal, bool approve) {
    if (!chat || !proposal) {
        return -1;
    }
    
    // Add vote as message
    char vote_msg[512];
    snprintf(vote_msg, sizeof(vote_msg), "Vote on proposal: %s - %s", 
             proposal, approve ? "APPROVE" : "REJECT");
    
    return group_chat_add_message(chat, voter, vote_msg);
}

// ============================================================================
// SUMMARY
// ============================================================================

char* group_chat_get_summary(GroupChat* chat) {
    if (!chat || chat->message_count == 0) {
        return workflow_strdup("No messages in chat");
    }
    
    // Build summary from messages
    size_t summary_len = 100; // Base length
    for (size_t i = 0; i < chat->message_count; i++) {
        if (chat->message_history[i] && chat->message_history[i]->content) {
            summary_len += strlen(chat->message_history[i]->content) + 2; // +2 for "\n"
        }
    }
    
    char* summary = malloc(summary_len);
    if (!summary) {
        return NULL;
    }
    
    size_t pos = 0;
    int written = snprintf(summary + pos, summary_len - pos, "Group Chat Summary:\n");
    if (written > 0) pos += (size_t)written;

    for (size_t i = 0; i < chat->message_count && pos < summary_len - 1; i++) {
        if (chat->message_history[i] && chat->message_history[i]->content) {
            written = snprintf(summary + pos, summary_len - pos, "%s\n",
                           chat->message_history[i]->content);
            if (written > 0) pos += (size_t)written;
        }
    }
    
    return summary;
}

