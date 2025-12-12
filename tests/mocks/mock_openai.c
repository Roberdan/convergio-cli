/**
 * CONVERGIO MOCK OPENAI PROVIDER
 *
 * Specialized mock for OpenAI GPT API behavior simulation.
 * Simulates GPT-specific response formats, error codes, and rate limits.
 */

#include "../mock_provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// OPENAI-SPECIFIC CONSTANTS
// ============================================================================

// OpenAI rate limits (Dec 2025 Tier 1)
#define OPENAI_RPM_TIER1 500       // 500 RPM for Tier 1
#define OPENAI_TPM_TIER1 200000    // 200K TPM for Tier 1

// Token costs (per 1M tokens, Dec 2025 pricing)
#define GPT4O_INPUT_COST      2.50   // $2.50 per 1M input tokens
#define GPT4O_OUTPUT_COST     10.0   // $10.00 per 1M output tokens
#define GPT4O_MINI_INPUT_COST 0.15   // $0.15 per 1M input tokens
#define GPT4O_MINI_OUTPUT_COST 0.60  // $0.60 per 1M output tokens
#define O1_INPUT_COST         15.0   // $15.00 per 1M input tokens
#define O1_OUTPUT_COST        60.0   // $60.00 per 1M output tokens

// ============================================================================
// OPENAI-SPECIFIC RESPONSE HANDLER
// ============================================================================

static char* openai_response_handler(const char* model, const char* system,
                                      const char* user, void* ctx) {
    (void)ctx;
    (void)system;

    // Simulate GPT's typical response style
    const char* model_name = "GPT";
    if (model) {
        if (strstr(model, "gpt-4o-mini")) model_name = "GPT-4o mini";
        else if (strstr(model, "gpt-4o")) model_name = "GPT-4o";
        else if (strstr(model, "o1-preview")) model_name = "o1-preview";
        else if (strstr(model, "o1-mini")) model_name = "o1-mini";
        else if (strstr(model, "o3")) model_name = "o3";
    }

    size_t len = strlen(model_name) + strlen(user) + 100;
    char* response = malloc(len);
    if (response) {
        snprintf(response, len,
                 "Based on your input \"%s\", here's my response from %s.",
                 user, model_name);
    }
    return response;
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Create mock configured like OpenAI GPT
 */
MockProvider* mock_openai_create(void) {
    MockProviderConfig config = {
        .default_response = "I'm ChatGPT, an AI assistant by OpenAI.",
        .echo_prompt = false,
        .response_delay_ms = 150,    // OpenAI is typically fast
        .tokens_per_word_input = 1,
        .tokens_per_word_output = 1,
        .simulate_errors = false,
        .error_rate = 0.0,
        .error_to_simulate = PROVIDER_ERR_UNKNOWN,
        .simulate_rate_limit = true,
        .requests_per_minute = OPENAI_RPM_TIER1,
        .current_minute_requests = 0,
        .support_tools = true,       // GPT supports function calling
        .tool_response_json = NULL,
        .support_streaming = true,   // GPT supports streaming
        .stream_chunk_size = 40,
        .stream_delay_ms = 25,
    };

    MockProvider* mock = mock_provider_create_with_config(&config);
    if (mock) {
        mock_set_response_handler(mock, openai_response_handler, NULL);
        mock->base.name = "Mock OpenAI";
        mock->base.api_key_env = "OPENAI_API_KEY";
        mock->base.base_url = "https://api.openai.com/v1";
    }
    return mock;
}

/**
 * Create mock configured as GPT-4o
 */
MockProvider* mock_openai_gpt4o(void) {
    MockProvider* mock = mock_openai_create();
    if (mock) {
        mock_set_response(mock, "[GPT-4o] Multimodal response with vision and audio capabilities.");
    }
    return mock;
}

/**
 * Create mock configured as GPT-4o-mini (fast, cheap)
 */
MockProvider* mock_openai_gpt4o_mini(void) {
    MockProvider* mock = mock_openai_create();
    if (mock) {
        mock->config.response_delay_ms = 80;  // Mini is faster
        mock_set_response(mock, "[GPT-4o-mini] Quick and efficient response.");
    }
    return mock;
}

/**
 * Create mock configured as o1-preview (reasoning model)
 */
MockProvider* mock_openai_o1(void) {
    MockProvider* mock = mock_openai_create();
    if (mock) {
        mock->config.response_delay_ms = 2000;  // o1 takes longer (reasoning)
        mock->config.support_streaming = false; // o1 doesn't stream
        mock_set_response(mock, "[o1-preview] <thinking>reasoning...</thinking> Conclusion after deep analysis.");
    }
    return mock;
}

/**
 * Create mock simulating OpenAI rate limit (429)
 */
MockProvider* mock_openai_rate_limited(void) {
    MockProvider* mock = mock_openai_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_RATE_LIMIT);
    }
    return mock;
}

/**
 * Create mock simulating OpenAI auth error (401)
 */
MockProvider* mock_openai_auth_error(void) {
    MockProvider* mock = mock_openai_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_AUTH);
    }
    return mock;
}

/**
 * Create mock simulating OpenAI quota exceeded
 */
MockProvider* mock_openai_quota_exceeded(void) {
    MockProvider* mock = mock_openai_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_QUOTA);
    }
    return mock;
}
