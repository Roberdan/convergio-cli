/**
 * CONVERGIO GROUP CHAT
 *
 * Multi-agent conversation with consensus building
 */

#include "nous/group_chat.h"
#include "nous/orchestrator.h"
#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// workflow_strdup is defined in workflow_types.c
extern char* workflow_strdup(const char* str);

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
    
    static uint64_t next_chat_id = 1;
    chat->chat_id = next_chat_id++;
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
    
    // Send message via message bus
    message_send(msg);
    
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
            // For consensus/debate, any participant can speak
            // Return participant who hasn't spoken recently
            if (chat->message_count == 0) {
                return chat->participants[0];
            }
            
            // Find participant who spoke least recently
            SemanticID least_recent = chat->participants[0];
            time_t oldest_time = time(NULL);
            
            for (int i = (int)chat->message_count - 1; i >= 0 && i >= (int)chat->message_count - (int)chat->participant_count; i--) {
                if (chat->message_history[i]) {
                    time_t msg_time = chat->message_history[i]->timestamp;
                    if (msg_time < oldest_time) {
                        oldest_time = msg_time;
                        least_recent = chat->message_history[i]->sender;
                    }
                }
            }
            
            return least_recent;
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

