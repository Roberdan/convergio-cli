/**
 * CONVERGIO TOKEN COUNTER
 *
 * Token estimation and counting for multiple providers:
 * - BPE-based estimation
 * - Provider-specific adjustments
 * - Cost calculation
 * - Context window management
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// TOKEN ESTIMATION CONSTANTS
// ============================================================================

// Average tokens per character for different content types
// These are empirical values based on typical English text
#define CHARS_PER_TOKEN_ENGLISH 4.0
#define CHARS_PER_TOKEN_CODE 3.5
#define CHARS_PER_TOKEN_JSON 3.0
#define CHARS_PER_TOKEN_UNICODE 2.5

// Provider-specific multipliers (relative to baseline)
#define MULTIPLIER_ANTHROPIC 1.0
#define MULTIPLIER_OPENAI 0.95 // GPT tokenizers slightly more efficient
#define MULTIPLIER_GEMINI 1.05 // Gemini slightly less efficient

// ============================================================================
// CONTENT TYPE DETECTION
// ============================================================================

typedef enum {
    CONTENT_ENGLISH,
    CONTENT_CODE,
    CONTENT_JSON,
    CONTENT_MIXED,
    CONTENT_UNICODE,
} ContentType;

static ContentType detect_content_type(const char* text) {
    if (!text || !*text)
        return CONTENT_ENGLISH;

    size_t len = strlen(text);
    size_t code_chars = 0;
    size_t json_chars = 0;
    size_t unicode_chars = 0;
    size_t sample_size = len > 1000 ? 1000 : len;

    for (size_t i = 0; i < sample_size; i++) {
        unsigned char c = (unsigned char)text[i];

        // Check for code-like characters
        if (c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')' || c == ';' ||
            c == '=' || c == '<' || c == '>' || c == '&' || c == '|') {
            code_chars++;
        }

        // Check for JSON structure
        if (c == ':' || c == '"' || c == ',' || c == '{' || c == '[') {
            json_chars++;
        }

        // Check for non-ASCII (Unicode)
        if (c > 127) {
            unicode_chars++;
        }
    }

    double code_ratio = (double)code_chars / sample_size;
    double json_ratio = (double)json_chars / sample_size;
    double unicode_ratio = (double)unicode_chars / sample_size;

    if (unicode_ratio > 0.2)
        return CONTENT_UNICODE;
    if (json_ratio > 0.15 && code_ratio > 0.1)
        return CONTENT_JSON;
    if (code_ratio > 0.1)
        return CONTENT_CODE;

    return CONTENT_ENGLISH;
}

// ============================================================================
// TOKEN ESTIMATION
// ============================================================================

/**
 * Estimate token count for text
 */
uint64_t tokens_estimate(const char* text, ProviderType provider) {
    if (!text || !*text)
        return 0;

    size_t len = strlen(text);
    ContentType type = detect_content_type(text);

    // Base estimation
    double chars_per_token;
    switch (type) {
    case CONTENT_ENGLISH:
        chars_per_token = CHARS_PER_TOKEN_ENGLISH;
        break;
    case CONTENT_CODE:
        chars_per_token = CHARS_PER_TOKEN_CODE;
        break;
    case CONTENT_JSON:
        chars_per_token = CHARS_PER_TOKEN_JSON;
        break;
    case CONTENT_UNICODE:
        chars_per_token = CHARS_PER_TOKEN_UNICODE;
        break;
    default:
        chars_per_token = CHARS_PER_TOKEN_ENGLISH;
        break;
    }

    double base_tokens = (double)len / chars_per_token;

    // Apply provider multiplier
    double multiplier;
    switch (provider) {
    case PROVIDER_ANTHROPIC:
        multiplier = MULTIPLIER_ANTHROPIC;
        break;
    case PROVIDER_OPENAI:
        multiplier = MULTIPLIER_OPENAI;
        break;
    case PROVIDER_GEMINI:
        multiplier = MULTIPLIER_GEMINI;
        break;
    default:
        multiplier = 1.0;
        break;
    }

    return (uint64_t)ceil(base_tokens * multiplier);
}

/**
 * Estimate tokens for a chat message
 */
uint64_t tokens_estimate_message(const char* role, const char* content, ProviderType provider) {
    if (!content)
        return 0;

    uint64_t content_tokens = tokens_estimate(content, provider);

    // Add overhead for message structure
    uint64_t overhead;
    switch (provider) {
    case PROVIDER_ANTHROPIC:
        // Anthropic: <role>content</role>
        overhead = 4;
        break;
    case PROVIDER_OPENAI:
        // OpenAI: {"role": "...", "content": "..."}
        overhead = 7;
        break;
    case PROVIDER_GEMINI:
        // Gemini: {"parts": [{"text": "..."}]}
        overhead = 8;
        break;
    default:
        overhead = 5;
    }

    // Add role tokens
    if (role) {
        overhead += strlen(role) / 4 + 1;
    }

    return content_tokens + overhead;
}

/**
 * Estimate tokens for a full conversation
 */
uint64_t tokens_estimate_conversation(const char* system, const char** messages, const char** roles,
                                      size_t message_count, ProviderType provider) {
    uint64_t total = 0;

    // System prompt
    if (system) {
        total += tokens_estimate_message("system", system, provider);
    }

    // Messages
    for (size_t i = 0; i < message_count; i++) {
        const char* role = roles ? roles[i] : "user";
        total += tokens_estimate_message(role, messages[i], provider);
    }

    // Add conversation overhead
    switch (provider) {
    case PROVIDER_ANTHROPIC:
        total += 10; // Anthropic message format overhead
        break;
    case PROVIDER_OPENAI:
        total += 3 * (message_count + 1); // ~3 tokens per message separator
        break;
    case PROVIDER_GEMINI:
        total += 5; // Gemini structure overhead
        break;
    default:
        total += 5;
    }

    return total;
}

// ============================================================================
// COST CALCULATION
// ============================================================================

/**
 * Calculate cost for token usage
 */
double tokens_calculate_cost(uint64_t input_tokens, uint64_t output_tokens, const char* model) {
    // Model pricing (per 1M tokens)
    double input_cost_per_m = 0;
    double output_cost_per_m = 0;

    // Anthropic models
    if (strstr(model, "opus")) {
        input_cost_per_m = 15.0;
        output_cost_per_m = 75.0;
    } else if (strstr(model, "sonnet")) {
        input_cost_per_m = 3.0;
        output_cost_per_m = 15.0;
    } else if (strstr(model, "haiku")) {
        input_cost_per_m = 1.0;
        output_cost_per_m = 5.0;
    }
    // OpenAI models
    else if (strstr(model, "gpt-4o")) {
        input_cost_per_m = 5.0;
        output_cost_per_m = 20.0;
    } else if (strstr(model, "gpt-4o") || strstr(model, "gpt-4o")) {
        input_cost_per_m = 1.25;
        output_cost_per_m = 10.0;
    } else if (strstr(model, "nano")) {
        input_cost_per_m = 0.05;
        output_cost_per_m = 0.40;
    } else if (strstr(model, "o3") || strstr(model, "o4")) {
        input_cost_per_m = 10.0;
        output_cost_per_m = 40.0;
    }
    // Gemini models
    else if (strstr(model, "ultra")) {
        input_cost_per_m = 7.0;
        output_cost_per_m = 21.0;
    } else if (strstr(model, "pro")) {
        input_cost_per_m = 2.0;
        output_cost_per_m = 12.0;
    } else if (strstr(model, "flash")) {
        input_cost_per_m = 0.075;
        output_cost_per_m = 0.30;
    }
    // Default
    else {
        input_cost_per_m = 3.0;
        output_cost_per_m = 15.0;
    }

    double input_cost = (double)input_tokens / 1000000.0 * input_cost_per_m;
    double output_cost = (double)output_tokens / 1000000.0 * output_cost_per_m;

    return input_cost + output_cost;
}

// ============================================================================
// CONTEXT WINDOW MANAGEMENT
// ============================================================================

typedef struct {
    const char* model;
    uint64_t context_window;
    uint64_t max_output;
} ModelLimits;

static const ModelLimits g_model_limits[] = {
    // Anthropic
    {"claude-opus-4", 200000, 32000},
    {"claude-sonnet-4", 1000000, 64000},
    {"claude-sonnet-4", 200000, 32000},
    {"claude-haiku-4.5", 200000, 32000},

    // OpenAI
    {"gpt-4o", 400000, 32000},
    {"o1", 400000, 32000},
    {"gpt-4o", 256000, 32000},
    {"gpt-4o", 128000, 16000},
    {"o3", 128000, 32000},
    {"o4-mini", 128000, 32000},
    {"gpt-4o-mini", 128000, 16000},

    // Gemini
    {"gemini-1.5-pro", 2000000, 65536},
    {"gemini-1.5-pro", 2000000, 65536},
    {"gemini-1.5-flash", 1000000, 65536},

    {NULL, 0, 0} // Sentinel
};

/**
 * Get context window size for a model
 */
uint64_t tokens_get_context_window(const char* model) {
    if (!model)
        return 128000; // Default

    for (int i = 0; g_model_limits[i].model != NULL; i++) {
        if (strstr(model, g_model_limits[i].model)) {
            return g_model_limits[i].context_window;
        }
    }

    return 128000; // Default
}

/**
 * Get max output tokens for a model
 */
uint64_t tokens_get_max_output(const char* model) {
    if (!model)
        return 16000; // Default

    for (int i = 0; g_model_limits[i].model != NULL; i++) {
        if (strstr(model, g_model_limits[i].model)) {
            return g_model_limits[i].max_output;
        }
    }

    return 16000; // Default
}

/**
 * Check if input fits in context window
 */
bool tokens_fits_context(uint64_t input_tokens, uint64_t reserved_output, const char* model) {
    uint64_t window = tokens_get_context_window(model);
    return input_tokens + reserved_output <= window;
}

/**
 * Calculate available tokens for output
 */
uint64_t tokens_available_for_output(uint64_t input_tokens, const char* model) {
    uint64_t window = tokens_get_context_window(model);
    uint64_t max_output = tokens_get_max_output(model);

    if (input_tokens >= window)
        return 0;

    uint64_t remaining = window - input_tokens;
    return remaining < max_output ? remaining : max_output;
}

// ============================================================================
// TRUNCATION
// ============================================================================

/**
 * Truncate text to fit within token limit
 */
char* tokens_truncate(const char* text, uint64_t max_tokens, ProviderType provider) {
    if (!text)
        return NULL;

    uint64_t current_tokens = tokens_estimate(text, provider);
    if (current_tokens <= max_tokens) {
        return strdup(text);
    }

    // Estimate characters to keep
    ContentType type = detect_content_type(text);
    double chars_per_token;
    switch (type) {
    case CONTENT_ENGLISH:
        chars_per_token = CHARS_PER_TOKEN_ENGLISH;
        break;
    case CONTENT_CODE:
        chars_per_token = CHARS_PER_TOKEN_CODE;
        break;
    case CONTENT_JSON:
        chars_per_token = CHARS_PER_TOKEN_JSON;
        break;
    case CONTENT_UNICODE:
        chars_per_token = CHARS_PER_TOKEN_UNICODE;
        break;
    default:
        chars_per_token = CHARS_PER_TOKEN_ENGLISH;
        break;
    }

    size_t max_chars = (size_t)(max_tokens * chars_per_token * 0.95); // 5% safety margin
    size_t len = strlen(text);

    if (max_chars >= len) {
        return strdup(text);
    }

    // Truncate at word boundary if possible
    size_t truncate_at = max_chars;
    while (truncate_at > 0 && !isspace((unsigned char)text[truncate_at])) {
        truncate_at--;
    }

    if (truncate_at == 0) {
        truncate_at = max_chars; // No word boundary found
    }

    char* result = malloc(truncate_at + 4);
    if (!result)
        return NULL;

    memcpy(result, text, truncate_at);
    memcpy(result + truncate_at, "...", 4); // includes null terminator

    return result;
}

// ============================================================================
// UTILITIES
// ============================================================================

/**
 * Format token count for display
 */
char* tokens_format(uint64_t tokens) {
    char* buf = malloc(32);
    if (!buf)
        return NULL;

    if (tokens >= 1000000) {
        snprintf(buf, 32, "%.1fM", (double)tokens / 1000000);
    } else if (tokens >= 1000) {
        snprintf(buf, 32, "%.1fK", (double)tokens / 1000);
    } else {
        snprintf(buf, 32, "%llu", (unsigned long long)tokens);
    }

    return buf;
}

/**
 * Format cost for display
 */
char* tokens_format_cost(double cost) {
    char* buf = malloc(32);
    if (!buf)
        return NULL;

    if (cost >= 1.0) {
        snprintf(buf, 32, "$%.2f", cost);
    } else if (cost >= 0.01) {
        snprintf(buf, 32, "$%.3f", cost);
    } else {
        snprintf(buf, 32, "$%.4f", cost);
    }

    return buf;
}

/**
 * Get token usage summary as JSON
 */
char* tokens_usage_json(TokenUsage* usage) {
    if (!usage)
        return strdup("{}");

    char* json = malloc(256);
    if (!json)
        return NULL;

    snprintf(json, 256,
             "{"
             "\"input_tokens\":%zu,"
             "\"output_tokens\":%zu,"
             "\"cached_tokens\":%zu,"
             "\"estimated_cost\":%.6f"
             "}",
             usage->input_tokens, usage->output_tokens, usage->cached_tokens,
             usage->estimated_cost);

    return json;
}
