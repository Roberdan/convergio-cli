/**
 * CONVERGIO CONTEXT COMPACTION
 *
 * Automatic context compression using LLM summarization.
 * When conversation context exceeds COMPACTION_THRESHOLD_TOKENS,
 * older messages are summarized into a checkpoint while keeping
 * recent messages intact.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/compaction.h"
#include "nous/orchestrator.h"  // For llm_chat, llm_estimate_tokens, etc.
#include "nous/nous.h"
#include "nous/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External persistence functions
extern char* persistence_load_conversation_context(const char* session_id, size_t max_messages);
extern int persistence_get_message_id_range(const char* session_id, int64_t* out_first, int64_t* out_last);
extern char* persistence_load_messages_range(const char* session_id, int64_t from_id, int64_t to_id, size_t* out_count);
extern char* persistence_load_latest_checkpoint(const char* session_id);
extern int persistence_get_checkpoint_count(const char* session_id);
extern int persistence_save_checkpoint(const char* session_id, int checkpoint_num, int64_t from_msg_id,
                                       int64_t to_msg_id, int messages_compressed, const char* summary,
                                       const char* key_facts, size_t original_tokens, size_t compressed_tokens,
                                       double cost);
extern int64_t persistence_get_cutoff_message_id(const char* session_id, int keep_recent);

// ============================================================================
// INTERNAL STATE
// ============================================================================

static bool g_compaction_initialized = false;
static size_t g_dynamic_threshold = COMPACTION_THRESHOLD_TOKENS;  // Dynamic threshold

// Summarization prompt template
static const char* SUMMARIZATION_PROMPT =
    "You are a conversation summarizer. Your task is to compress the following "
    "conversation into a concise summary that preserves:\n\n"
    "1. Key decisions made\n"
    "2. Important facts learned\n"
    "3. Current state of any tasks\n"
    "4. User preferences expressed\n"
    "5. Any errors encountered and how they were resolved\n\n"
    "Be extremely concise. Use bullet points. Maximum 500 tokens.\n"
    "Focus on information that would be useful for continuing the conversation.\n\n"
    "CONVERSATION TO SUMMARIZE:\n%s\n\n"
    "SUMMARY:";

// ============================================================================
// INITIALIZATION
// ============================================================================

int compaction_init(void) {
    if (g_compaction_initialized) {
        return 0;  // Already initialized
    }

    // Verify LLM is available for summarization
    if (!llm_is_available()) {
        LOG_WARN(LOG_CAT_MEMORY, "Compaction: No LLM provider available, "
                 "summarization will use fallback truncation");
    }

    g_compaction_initialized = true;
    LOG_INFO(LOG_CAT_MEMORY, "Context compaction initialized (threshold: %d tokens)",
             COMPACTION_THRESHOLD_TOKENS);

    return 0;
}

void compaction_shutdown(void) {
    g_compaction_initialized = false;
    g_dynamic_threshold = COMPACTION_THRESHOLD_TOKENS;  // Reset to default
}

// ============================================================================
// DYNAMIC THRESHOLD MANAGEMENT
// ============================================================================

size_t compaction_set_dynamic_threshold(size_t model_context_window) {
    if (model_context_window == 0) {
        // No valid context window, use default
        g_dynamic_threshold = COMPACTION_THRESHOLD_TOKENS;
        return g_dynamic_threshold;
    }

    // Calculate threshold as percentage of context window
    size_t calculated = (size_t)(model_context_window * COMPACTION_THRESHOLD_RATIO);

    // Clamp to min/max bounds
    if (calculated < COMPACTION_MIN_THRESHOLD) {
        calculated = COMPACTION_MIN_THRESHOLD;
    }
    if (calculated > COMPACTION_MAX_THRESHOLD) {
        calculated = COMPACTION_MAX_THRESHOLD;
    }

    g_dynamic_threshold = calculated;

    LOG_INFO(LOG_CAT_MEMORY, "Dynamic compaction threshold set to %zu tokens "
             "(model context: %zu, ratio: %.0f%%)",
             g_dynamic_threshold, model_context_window,
             COMPACTION_THRESHOLD_RATIO * 100);

    return g_dynamic_threshold;
}

size_t compaction_get_threshold(void) {
    return g_dynamic_threshold;
}

void compaction_reset_threshold(void) {
    g_dynamic_threshold = COMPACTION_THRESHOLD_TOKENS;
    LOG_DEBUG(LOG_CAT_MEMORY, "Compaction threshold reset to default: %zu tokens",
              g_dynamic_threshold);
}

// ============================================================================
// THRESHOLD CHECK
// ============================================================================

bool compaction_needed(const char* session_id, size_t current_tokens) {
    if (!session_id) return false;

    // Check if we've already hit max checkpoints
    int checkpoint_count = compaction_get_checkpoint_count(session_id);
    if (checkpoint_count >= COMPACTION_MAX_CHECKPOINTS) {
        LOG_DEBUG(LOG_CAT_MEMORY, "Max checkpoints reached (%d), skipping compaction",
                  COMPACTION_MAX_CHECKPOINTS);
        return false;
    }

    // Check threshold (uses dynamic threshold which may be adjusted for MLX models)
    if (current_tokens > g_dynamic_threshold) {
        LOG_INFO(LOG_CAT_MEMORY, "Context size %zu exceeds threshold %zu, compaction needed",
                 current_tokens, g_dynamic_threshold);
        return true;
    }

    return false;
}

// ============================================================================
// SUMMARIZATION
// ============================================================================

CompactionResult* compaction_summarize(
    const char* session_id,
    int64_t from_msg_id,
    int64_t to_msg_id,
    const char* messages_text
) {
    if (!session_id || !messages_text) return NULL;

    CompactionResult* result = calloc(1, sizeof(CompactionResult));
    if (!result) return NULL;

    // Estimate original tokens using LLM facade
    result->original_tokens = llm_estimate_tokens(messages_text);

    // Try to get LLM summary
    char* summary = NULL;
    TokenUsage usage = {0};

    if (llm_is_available()) {
        // Build summarization prompt
        size_t prompt_len = strlen(SUMMARIZATION_PROMPT) + strlen(messages_text) + 100;
        char* full_prompt = malloc(prompt_len);
        if (full_prompt) {
            // Suppress format-nonliteral warning - SUMMARIZATION_PROMPT is a trusted constant
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wformat-nonliteral"
            snprintf(full_prompt, prompt_len, SUMMARIZATION_PROMPT, messages_text);
            #pragma clang diagnostic pop

            // Call LLM via facade for summarization (uses best available provider)
            summary = llm_chat_with_model(
                COMPACTION_MODEL,
                "You are a precise and concise summarizer.",
                full_prompt,
                &usage
            );

            free(full_prompt);
        }
    }

    // Fallback to truncation if summarization failed
    if (!summary) {
        LOG_WARN(LOG_CAT_MEMORY, "LLM summarization failed, using truncation fallback");

        // Create truncated summary (first 2000 chars + last 2000 chars)
        size_t msg_len = strlen(messages_text);
        if (msg_len > 4000) {
            static const char TRUNCATE_MARKER[] = "\n\n[... conversation truncated ...]\n\n";
            #define TRUNCATE_CHUNK 2000

            size_t marker_len = sizeof(TRUNCATE_MARKER) - 1;  // Exclude null terminator
            size_t summary_len = TRUNCATE_CHUNK + marker_len + TRUNCATE_CHUNK;
            summary = malloc(summary_len + 1);
            if (summary) {
                memcpy(summary, messages_text, TRUNCATE_CHUNK);
                memcpy(summary + TRUNCATE_CHUNK, TRUNCATE_MARKER, marker_len);
                memcpy(summary + TRUNCATE_CHUNK + marker_len,
                       messages_text + msg_len - TRUNCATE_CHUNK, TRUNCATE_CHUNK);
                summary[summary_len] = '\0';
            }

            #undef TRUNCATE_CHUNK
        } else {
            summary = strdup(messages_text);
        }
    }

    if (!summary) {
        free(result);
        return NULL;
    }

    // Calculate compressed tokens using LLM facade
    result->compressed_tokens = llm_estimate_tokens(summary);

    // Fill result
    result->summary = summary;
    result->compression_ratio = (result->compressed_tokens > 0)
        ? (double)result->original_tokens / result->compressed_tokens
        : 1.0;
    result->cost_usd = usage.estimated_cost;

    // Get checkpoint number
    int checkpoint_num = persistence_get_checkpoint_count(session_id) + 1;
    result->checkpoint_num = checkpoint_num;

    // Count messages (estimate from text)
    int messages_compressed = 0;
    const char* p = messages_text;
    while ((p = strstr(p, "\n\n")) != NULL) {
        messages_compressed++;
        p += 2;
    }

    // Save checkpoint to database
    int save_result = persistence_save_checkpoint(
        session_id,
        checkpoint_num,
        from_msg_id,
        to_msg_id,
        messages_compressed,
        summary,
        NULL,  // key_facts (could extract later)
        result->original_tokens,
        result->compressed_tokens,
        result->cost_usd
    );

    if (save_result != 0) {
        LOG_WARN(LOG_CAT_MEMORY, "Failed to save checkpoint to database");
    }

    LOG_INFO(LOG_CAT_MEMORY, "Compaction complete: %zu -> %zu tokens (%.1fx compression), cost: $%.6f",
             result->original_tokens, result->compressed_tokens,
             result->compression_ratio, result->cost_usd);

    return result;
}

// ============================================================================
// CHECKPOINT ACCESS
// ============================================================================

char* compaction_get_checkpoint(const char* session_id) {
    if (!session_id) return NULL;
    return persistence_load_latest_checkpoint(session_id);
}

int compaction_get_checkpoint_count(const char* session_id) {
    if (!session_id) return 0;
    return persistence_get_checkpoint_count(session_id);
}

// ============================================================================
// CONTEXT BUILDING WITH COMPACTION
// ============================================================================

char* compaction_build_context(
    const char* session_id,
    const char* user_input,
    bool* out_was_compacted
) {
    if (!session_id || !user_input) return NULL;
    if (out_was_compacted) *out_was_compacted = false;

    // Load full conversation first to estimate size
    char* full_conv = persistence_load_conversation_context(session_id, 100);
    if (!full_conv) {
        return strdup("");  // No conversation yet
    }

    // Estimate tokens using LLM facade
    size_t conv_tokens = llm_estimate_tokens(full_conv);

    // Check if compaction needed
    if (!compaction_needed(session_id, conv_tokens)) {
        // Return full conversation as-is
        return full_conv;
    }

    // Compaction needed - get message ID range
    int64_t first_msg_id = 0, last_msg_id = 0;
    if (persistence_get_message_id_range(session_id, &first_msg_id, &last_msg_id) != 0) {
        return full_conv;  // Can't get range, return as-is
    }

    // Calculate cutoff: get the actual message ID that is KEEP_RECENT messages from the end
    // This properly handles non-consecutive message IDs
    int64_t cutoff_msg_id = persistence_get_cutoff_message_id(session_id, COMPACTION_KEEP_RECENT_MSGS);
    if (cutoff_msg_id < 0 || cutoff_msg_id <= first_msg_id) {
        return full_conv;  // Not enough messages to compact
    }

    // Check for existing checkpoint
    char* existing_checkpoint = compaction_get_checkpoint(session_id);

    // Load messages to summarize (those before cutoff)
    size_t old_count = 0;
    char* old_messages = persistence_load_messages_range(
        session_id,
        first_msg_id,
        cutoff_msg_id,
        &old_count
    );

    char* checkpoint_summary = existing_checkpoint;

    // If no existing checkpoint or we have new messages to summarize
    if (!checkpoint_summary && old_messages) {
        CompactionResult* result = compaction_summarize(
            session_id,
            first_msg_id,
            cutoff_msg_id,
            old_messages
        );
        if (result) {
            checkpoint_summary = strdup(result->summary);
            compaction_result_free(result);
            if (out_was_compacted) *out_was_compacted = true;
        }
    }

    if (old_messages) free(old_messages);

    // Load recent messages
    char* recent = persistence_load_conversation_context(
        session_id,
        COMPACTION_KEEP_RECENT_MSGS
    );

    // Build final context
    size_t ctx_size = 65536;
    char* context = malloc(ctx_size);
    if (!context) {
        if (checkpoint_summary && checkpoint_summary != existing_checkpoint) {
            free(checkpoint_summary);
        }
        if (existing_checkpoint) free(existing_checkpoint);
        if (recent) free(recent);
        free(full_conv);
        return NULL;
    }

    size_t len = 0;
    if (checkpoint_summary) {
        int written = snprintf(context + len, ctx_size - len,
            "## Previous Context (Summarized)\n%s\n\n", checkpoint_summary);
        if (written > 0) {
            len += (size_t)written;
        }
    }

    if (recent) {
        int written = snprintf(context + len, ctx_size - len,
            "## Recent Conversation\n%s\n", recent);
        if (written > 0) {
            len += (size_t)written;
        }
        free(recent);
    }

    // Cleanup
    if (checkpoint_summary && checkpoint_summary != existing_checkpoint) {
        free(checkpoint_summary);
    }
    if (existing_checkpoint) free(existing_checkpoint);
    free(full_conv);

    return context;
}

// ============================================================================
// CLEANUP
// ============================================================================

void compaction_result_free(CompactionResult* result) {
    if (!result) return;
    if (result->summary) free(result->summary);
    free(result);
}

// ============================================================================
// STATISTICS
// ============================================================================

int compaction_get_stats(
    const char* session_id,
    size_t* out_total_saved,
    double* out_total_cost
) {
    // This would query the checkpoint_summaries table for aggregated stats
    // For now, return 0 indicating no stats available
    if (out_total_saved) *out_total_saved = 0;
    if (out_total_cost) *out_total_cost = 0.0;
    return 0;
}
