/**
 * CONVERGIO CONTEXT COMPACTION
 *
 * Automatic context compression system that summarizes long conversations
 * when they exceed a token threshold. Uses an economical LLM model (Haiku)
 * to create concise summaries while preserving key information.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_COMPACTION_H
#define CONVERGIO_COMPACTION_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define COMPACTION_THRESHOLD_TOKENS   80000   // Trigger compaction above this
#define COMPACTION_KEEP_RECENT_MSGS   10      // Keep last N messages uncompacted
#define COMPACTION_MODEL              "claude-haiku-4.5"
#define COMPACTION_MAX_SUMMARY_TOKENS 500     // Max tokens in summary
#define COMPACTION_MAX_CHECKPOINTS    5       // Max checkpoints per session

// ============================================================================
// COMPACTION RESULT
// ============================================================================

typedef struct {
    char* summary;              // The compressed summary text
    size_t original_tokens;     // Tokens in original messages
    size_t compressed_tokens;   // Tokens in the summary
    double compression_ratio;   // original / compressed
    double cost_usd;            // Cost of summarization call
    int checkpoint_num;         // Checkpoint sequence number
} CompactionResult;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Initialize the compaction system
 * Must be called after persistence_init()
 * @return 0 on success, -1 on error
 */
int compaction_init(void);

/**
 * Shutdown and cleanup compaction resources
 */
void compaction_shutdown(void);

/**
 * Check if compaction is needed for current context
 * @param session_id Current session ID
 * @param current_tokens Estimated tokens in current context
 * @return true if compaction should be triggered
 */
bool compaction_needed(const char* session_id, size_t current_tokens);

/**
 * Perform context compaction (summarization)
 * Creates a summary of older messages and saves it as a checkpoint
 *
 * @param session_id Current session ID
 * @param from_msg_id First message ID to include in summary
 * @param to_msg_id Last message ID to include in summary
 * @param messages_text Full text of messages to summarize
 * @return CompactionResult pointer (caller must free with compaction_result_free)
 *         Returns NULL on error
 */
CompactionResult* compaction_summarize(
    const char* session_id,
    int64_t from_msg_id,
    int64_t to_msg_id,
    const char* messages_text
);

/**
 * Get the latest checkpoint summary for a session
 * @param session_id Session ID to lookup
 * @return Summary text (caller must free) or NULL if no checkpoint exists
 */
char* compaction_get_checkpoint(const char* session_id);

/**
 * Get number of checkpoints for a session
 * @param session_id Session ID to lookup
 * @return Number of checkpoints, 0 if none
 */
int compaction_get_checkpoint_count(const char* session_id);

/**
 * Build context with compaction applied
 * This is the main integration point - call this instead of loading
 * full conversation when context exceeds threshold
 *
 * @param session_id Current session ID
 * @param user_input Current user input (for relevance)
 * @param out_was_compacted Output: set to true if compaction was applied
 * @return Context string (caller must free)
 */
char* compaction_build_context(
    const char* session_id,
    const char* user_input,
    bool* out_was_compacted
);

/**
 * Free a CompactionResult structure
 * @param result Result to free (can be NULL)
 */
void compaction_result_free(CompactionResult* result);

/**
 * Get compaction statistics for a session
 * @param session_id Session ID
 * @param out_total_saved Output: total tokens saved by compaction
 * @param out_total_cost Output: total cost of compaction calls
 * @return Number of compaction events
 */
int compaction_get_stats(
    const char* session_id,
    size_t* out_total_saved,
    double* out_total_cost
);

// ============================================================================
// PERSISTENCE FUNCTIONS (called by compaction.c, implemented in persistence.c)
// ============================================================================

/**
 * Save a checkpoint summary to database
 * @return 0 on success, -1 on error
 */
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
);

/**
 * Load the latest checkpoint for a session
 * @return Summary text (caller must free) or NULL
 */
char* persistence_load_latest_checkpoint(const char* session_id);

/**
 * Get checkpoint count for a session
 * @return Number of checkpoints
 */
int persistence_get_checkpoint_count(const char* session_id);

/**
 * Load messages in a range for summarization
 * @param session_id Session ID
 * @param from_id First message ID
 * @param to_id Last message ID
 * @param out_count Output: number of messages
 * @return Formatted conversation text (caller must free)
 */
char* persistence_load_messages_range(
    const char* session_id,
    int64_t from_id,
    int64_t to_id,
    size_t* out_count
);

/**
 * Get first and last message IDs for a session
 * @param session_id Session ID
 * @param out_first Output: first message ID
 * @param out_last Output: last message ID
 * @return 0 on success, -1 if no messages
 */
int persistence_get_message_id_range(
    const char* session_id,
    int64_t* out_first,
    int64_t* out_last
);

#endif // CONVERGIO_COMPACTION_H
