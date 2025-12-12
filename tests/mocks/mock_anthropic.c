/**
 * CONVERGIO MOCK ANTHROPIC PROVIDER
 *
 * Specialized mock for Anthropic Claude API behavior simulation.
 * Simulates Claude-specific response formats, error codes, and rate limits.
 */

#include "../mock_provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// ANTHROPIC-SPECIFIC CONSTANTS
// ============================================================================

// Claude response format simulation
#define CLAUDE_RESPONSE_PREFIX "[Claude]: "

// Anthropic rate limits (as of Dec 2025)
#define ANTHROPIC_RPM_TIER1 60      // Tier 1: 60 RPM
#define ANTHROPIC_RPM_TIER2 1000    // Tier 2: 1000 RPM
#define ANTHROPIC_RPM_TIER3 4000    // Tier 3: 4000 RPM

// Token costs (per 1M tokens, Dec 2025 pricing)
#define CLAUDE_SONNET_INPUT_COST   3.0   // $3.00 per 1M input tokens
#define CLAUDE_SONNET_OUTPUT_COST  15.0  // $15.00 per 1M output tokens
#define CLAUDE_HAIKU_INPUT_COST    0.25  // $0.25 per 1M input tokens
#define CLAUDE_HAIKU_OUTPUT_COST   1.25  // $1.25 per 1M output tokens
#define CLAUDE_OPUS_INPUT_COST     15.0  // $15.00 per 1M input tokens
#define CLAUDE_OPUS_OUTPUT_COST    75.0  // $75.00 per 1M output tokens

// ============================================================================
// ANTHROPIC-SPECIFIC RESPONSE HANDLER
// ============================================================================

static char* anthropic_response_handler(const char* model, const char* system,
                                         const char* user, void* ctx) {
    (void)ctx;
    (void)system;

    // Simulate Claude's typical response style
    const char* model_name = "Claude";
    if (model) {
        if (strstr(model, "haiku")) model_name = "Claude Haiku";
        else if (strstr(model, "sonnet")) model_name = "Claude Sonnet";
        else if (strstr(model, "opus")) model_name = "Claude Opus";
    }

    size_t len = strlen(model_name) + strlen(user) + 100;
    char* response = malloc(len);
    if (response) {
        snprintf(response, len,
                 "I understand your request: \"%s\". As %s, I'm happy to help.",
                 user, model_name);
    }
    return response;
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Create mock configured like Anthropic Claude
 */
MockProvider* mock_anthropic_create(void) {
    MockProviderConfig config = {
        .default_response = "I'm Claude, an AI assistant by Anthropic.",
        .echo_prompt = false,
        .response_delay_ms = 200,    // Realistic latency
        .tokens_per_word_input = 1,  // Claude is efficient
        .tokens_per_word_output = 1,
        .simulate_errors = false,
        .error_rate = 0.0,
        .error_to_simulate = PROVIDER_ERR_UNKNOWN,
        .simulate_rate_limit = true,
        .requests_per_minute = ANTHROPIC_RPM_TIER1,
        .current_minute_requests = 0,
        .support_tools = true,       // Claude supports tools
        .tool_response_json = NULL,
        .support_streaming = true,   // Claude supports streaming
        .stream_chunk_size = 50,
        .stream_delay_ms = 30,
    };

    MockProvider* mock = mock_provider_create_with_config(&config);
    if (mock) {
        mock_set_response_handler(mock, anthropic_response_handler, NULL);
        mock->base.name = "Mock Anthropic";
        mock->base.api_key_env = "ANTHROPIC_API_KEY";
        mock->base.base_url = "https://api.anthropic.com/v1";
    }
    return mock;
}

/**
 * Create mock configured as Claude Sonnet
 */
MockProvider* mock_anthropic_sonnet(void) {
    MockProvider* mock = mock_anthropic_create();
    if (mock) {
        mock_set_response(mock, "[Sonnet] I'm Claude 3.5 Sonnet, optimized for balanced performance.");
    }
    return mock;
}

/**
 * Create mock configured as Claude Haiku (fast, cheap)
 */
MockProvider* mock_anthropic_haiku(void) {
    MockProvider* mock = mock_anthropic_create();
    if (mock) {
        mock->config.response_delay_ms = 50;  // Haiku is faster
        mock_set_response(mock, "[Haiku] Quick response from Claude Haiku.");
    }
    return mock;
}

/**
 * Create mock configured as Claude Opus (powerful, expensive)
 */
MockProvider* mock_anthropic_opus(void) {
    MockProvider* mock = mock_anthropic_create();
    if (mock) {
        mock->config.response_delay_ms = 500;  // Opus is slower but more thorough
        mock_set_response(mock, "[Opus] Comprehensive analysis from Claude Opus 4.5.");
    }
    return mock;
}

/**
 * Create mock simulating Anthropic overload errors
 */
MockProvider* mock_anthropic_overloaded(void) {
    MockProvider* mock = mock_anthropic_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_RATE_LIMIT);
    }
    return mock;
}

/**
 * Create mock simulating Anthropic auth errors
 */
MockProvider* mock_anthropic_auth_error(void) {
    MockProvider* mock = mock_anthropic_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_AUTH);
    }
    return mock;
}
