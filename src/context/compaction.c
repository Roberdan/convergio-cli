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
#include "nous/provider.h"
#include "nous/orchestrator.h"
#include "nous/nous.h"
#include "nous/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External persistence functions
extern char* persistence_load_conversation_context(const char* session_id, size_t max_messages);

// ============================================================================
// INTERNAL STATE
// ============================================================================

static bool g_compaction_initialized = false;

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

    // Verify provider is available for summarization
    if (!provider_is_available(PROVIDER_ANTHROPIC)) {
        LOG_WARN(LOG_CAT_MEMORY, "Compaction: Anthropic provider not available, "
                 "summarization will use fallback truncation");
    }

    g_compaction_initialized = true;
    LOG_INFO(LOG_CAT_MEMORY, "Context compaction initialized (threshold: %d tokens)",
             COMPACTION_THRESHOLD_TOKENS);

    return 0;
}

void compaction_shutdown(void) {
    g_compaction_initialized = false;
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

    // Check threshold
    if (current_tokens > COMPACTION_THRESHOLD_TOKENS) {
        LOG_INFO(LOG_CAT_MEMORY, "Context size %zu exceeds threshold %d, compaction needed",
                 current_tokens, COMPACTION_THRESHOLD_TOKENS);
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

    // Estimate original tokens
    Provider* provider = provider_get(PROVIDER_ANTHROPIC);
    if (provider && provider->estimate_tokens) {
        result->original_tokens = provider->estimate_tokens(provider, messages_text);
    } else {
        // Fallback: estimate ~4 chars per token
        result->original_tokens = strlen(messages_text) / 4;
    }

    // Try to get LLM summary
    char* summary = NULL;
    TokenUsage usage = {0};

    if (provider && provider->chat) {
        // Build summarization prompt
        size_t prompt_len = strlen(SUMMARIZATION_PROMPT) + strlen(messages_text) + 100;
        char* full_prompt = malloc(prompt_len);
        if (full_prompt) {
            snprintf(full_prompt, prompt_len, SUMMARIZATION_PROMPT, messages_text);

            // Call cheap model for summarization
            summary = provider->chat(
                provider,
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
            summary = malloc(4100);
            if (summary) {
                strncpy(summary, messages_text, 2000);
                summary[2000] = '\0';
                strcat(summary, "\n\n[... conversation truncated ...]\n\n");
                strcat(summary, messages_text + msg_len - 2000);
            }
        } else {
            summary = strdup(messages_text);
        }
    }

    if (!summary) {
        free(result);
        return NULL;
    }

    // Calculate compressed tokens
    if (provider && provider->estimate_tokens) {
        result->compressed_tokens = provider->estimate_tokens(provider, summary);
    } else {
        result->compressed_tokens = strlen(summary) / 4;
    }

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

    // Estimate tokens
    Provider* provider = provider_get(PROVIDER_ANTHROPIC);
    size_t conv_tokens = 0;
    if (provider && provider->estimate_tokens) {
        conv_tokens = provider->estimate_tokens(provider, full_conv);
    } else {
        conv_tokens = strlen(full_conv) / 4;
    }

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

    // Calculate cutoff: summarize all but the last KEEP_RECENT messages
    int64_t cutoff_msg_id = last_msg_id - COMPACTION_KEEP_RECENT_MSGS;
    if (cutoff_msg_id <= first_msg_id) {
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
        len += snprintf(context + len, ctx_size - len,
            "## Previous Context (Summarized)\n%s\n\n", checkpoint_summary);
    }

    if (recent) {
        len += snprintf(context + len, ctx_size - len,
            "## Recent Conversation\n%s\n", recent);
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
