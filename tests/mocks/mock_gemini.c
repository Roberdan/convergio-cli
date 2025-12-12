/**
 * CONVERGIO MOCK GEMINI PROVIDER
 *
 * Specialized mock for Google Gemini API behavior simulation.
 * Simulates Gemini-specific response formats, error codes, and rate limits.
 */

#include "../mock_provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// GEMINI-SPECIFIC CONSTANTS
// ============================================================================

// Gemini rate limits (Dec 2025)
#define GEMINI_RPM_FREE 15         // Free tier: 15 RPM
#define GEMINI_RPM_PAID 1000       // Paid tier: 1000 RPM

// Token costs (per 1M tokens, Dec 2025 pricing)
// Gemini 2.0 pricing
#define GEMINI_2_FLASH_INPUT_COST      0.10   // $0.10 per 1M input tokens
#define GEMINI_2_FLASH_OUTPUT_COST     0.40   // $0.40 per 1M output tokens
#define GEMINI_1_5_PRO_INPUT_COST      1.25   // $1.25 per 1M input tokens
#define GEMINI_1_5_PRO_OUTPUT_COST     5.00   // $5.00 per 1M output tokens
#define GEMINI_1_5_FLASH_INPUT_COST    0.075  // $0.075 per 1M input tokens
#define GEMINI_1_5_FLASH_OUTPUT_COST   0.30   // $0.30 per 1M output tokens

// ============================================================================
// GEMINI-SPECIFIC RESPONSE HANDLER
// ============================================================================

static char* gemini_response_handler(const char* model, const char* system,
                                      const char* user, void* ctx) {
    (void)ctx;
    (void)system;

    // Simulate Gemini's typical response style
    const char* model_name = "Gemini";
    if (model) {
        if (strstr(model, "gemini-2.0-flash")) model_name = "Gemini 2.0 Flash";
        else if (strstr(model, "gemini-1.5-pro")) model_name = "Gemini 1.5 Pro";
        else if (strstr(model, "gemini-1.5-flash")) model_name = "Gemini 1.5 Flash";
        else if (strstr(model, "gemini-exp")) model_name = "Gemini Experimental";
    }

    size_t len = strlen(model_name) + strlen(user) + 100;
    char* response = malloc(len);
    if (response) {
        snprintf(response, len,
                 "Hello! I'm %s. Let me help you with: \"%s\"",
                 model_name, user);
    }
    return response;
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Create mock configured like Google Gemini
 */
MockProvider* mock_gemini_create(void) {
    MockProviderConfig config = {
        .default_response = "Hello! I'm Gemini, Google's AI assistant.",
        .echo_prompt = false,
        .response_delay_ms = 100,    // Gemini Flash is very fast
        .tokens_per_word_input = 1,
        .tokens_per_word_output = 1,
        .simulate_errors = false,
        .error_rate = 0.0,
        .error_to_simulate = PROVIDER_ERR_UNKNOWN,
        .simulate_rate_limit = true,
        .requests_per_minute = GEMINI_RPM_PAID,
        .current_minute_requests = 0,
        .support_tools = true,       // Gemini supports function calling
        .tool_response_json = NULL,
        .support_streaming = true,   // Gemini supports streaming
        .stream_chunk_size = 60,
        .stream_delay_ms = 20,
    };

    MockProvider* mock = mock_provider_create_with_config(&config);
    if (mock) {
        mock_set_response_handler(mock, gemini_response_handler, NULL);
        mock->base.name = "Mock Gemini";
        mock->base.api_key_env = "GOOGLE_API_KEY";
        mock->base.base_url = "https://generativelanguage.googleapis.com/v1beta";
    }
    return mock;
}

/**
 * Create mock configured as Gemini 2.0 Flash (latest, fastest)
 */
MockProvider* mock_gemini_2_flash(void) {
    MockProvider* mock = mock_gemini_create();
    if (mock) {
        mock->config.response_delay_ms = 50;  // 2.0 Flash is extremely fast
        mock_set_response(mock, "[Gemini 2.0 Flash] Lightning-fast multimodal response with thinking.");
    }
    return mock;
}

/**
 * Create mock configured as Gemini 1.5 Pro (powerful)
 */
MockProvider* mock_gemini_1_5_pro(void) {
    MockProvider* mock = mock_gemini_create();
    if (mock) {
        mock->config.response_delay_ms = 300;  // Pro is more thorough
        mock_set_response(mock, "[Gemini 1.5 Pro] Comprehensive analysis with 2M context window.");
    }
    return mock;
}

/**
 * Create mock configured as Gemini 1.5 Flash (balanced)
 */
MockProvider* mock_gemini_1_5_flash(void) {
    MockProvider* mock = mock_gemini_create();
    if (mock) {
        mock->config.response_delay_ms = 80;
        mock_set_response(mock, "[Gemini 1.5 Flash] Fast and efficient with 1M context window.");
    }
    return mock;
}

/**
 * Create mock simulating Gemini free tier (strict rate limits)
 */
MockProvider* mock_gemini_free_tier(void) {
    MockProvider* mock = mock_gemini_create();
    if (mock) {
        mock_set_rate_limit(mock, true, GEMINI_RPM_FREE);
        mock_set_response(mock, "[Gemini Free] Response from free tier.");
    }
    return mock;
}

/**
 * Create mock simulating Gemini rate limit (429)
 */
MockProvider* mock_gemini_rate_limited(void) {
    MockProvider* mock = mock_gemini_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_RATE_LIMIT);
    }
    return mock;
}

/**
 * Create mock simulating Gemini auth error
 */
MockProvider* mock_gemini_auth_error(void) {
    MockProvider* mock = mock_gemini_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_AUTH);
    }
    return mock;
}

/**
 * Create mock simulating Gemini safety filter block
 */
MockProvider* mock_gemini_safety_blocked(void) {
    MockProvider* mock = mock_gemini_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_CONTENT_FILTER);
    }
    return mock;
}
