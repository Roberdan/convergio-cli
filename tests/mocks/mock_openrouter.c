/**
 * CONVERGIO MOCK OPENROUTER PROVIDER
 *
 * Specialized mock for OpenRouter API behavior simulation.
 * Simulates access to 300+ models through unified API.
 * OpenRouter uses OpenAI-compatible format but with provider-prefixed model IDs.
 */

#include "../mock_provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// OPENROUTER-SPECIFIC CONSTANTS
// ============================================================================

// OpenRouter rate limits
#define OPENROUTER_RPM_FREE    20       // 20 RPM for free tier
#define OPENROUTER_RPM_PAID    500      // 500 RPM for paid tier

// Token costs (per 1M tokens, Dec 2025 pricing)
#define DEEPSEEK_R1_INPUT_COST      0.55   // $0.55 per 1M input tokens
#define DEEPSEEK_R1_OUTPUT_COST     2.19   // $2.19 per 1M output tokens
#define LLAMA33_70B_INPUT_COST      0.40   // $0.40 per 1M input tokens
#define LLAMA33_70B_OUTPUT_COST     0.40   // $0.40 per 1M output tokens
#define MISTRAL_LARGE_INPUT_COST    2.0    // $2.00 per 1M input tokens
#define MISTRAL_LARGE_OUTPUT_COST   6.0    // $6.00 per 1M output tokens
#define QWEN_72B_INPUT_COST         0.35   // $0.35 per 1M input tokens
#define QWEN_72B_OUTPUT_COST        0.40   // $0.40 per 1M output tokens

// ============================================================================
// OPENROUTER-SPECIFIC RESPONSE HANDLER
// ============================================================================

static char* openrouter_response_handler(const char* model, const char* system,
                                         const char* user, void* ctx) {
    (void)ctx;
    (void)system;

    // Simulate OpenRouter's model routing
    const char* model_name = "OpenRouter Model";
    if (model) {
        if (strstr(model, "deepseek/deepseek-r1")) model_name = "DeepSeek R1";
        else if (strstr(model, "meta-llama/llama-3.3-70b")) model_name = "Llama 3.3 70B";
        else if (strstr(model, "mistralai/mistral-large")) model_name = "Mistral Large";
        else if (strstr(model, "qwen/qwen-2.5-72b")) model_name = "Qwen 2.5 72B";
        else if (strstr(model, "google/gemini")) model_name = "Gemini (via OR)";
        else if (strstr(model, "anthropic/claude")) model_name = "Claude (via OR)";
    }

    size_t len = strlen(model_name) + strlen(user) + 150;
    char* response = malloc(len);
    if (response) {
        snprintf(response, len,
                 "[%s via OpenRouter] Response to: \"%s\"",
                 model_name, user);
    }
    return response;
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Create mock configured like OpenRouter
 */
MockProvider* mock_openrouter_create(void) {
    MockProviderConfig config = {
        .default_response = "I'm responding via OpenRouter's unified API.",
        .echo_prompt = false,
        .response_delay_ms = 200,    // OpenRouter adds routing latency
        .tokens_per_word_input = 1,
        .tokens_per_word_output = 1,
        .simulate_errors = false,
        .error_rate = 0.0,
        .error_to_simulate = PROVIDER_ERR_UNKNOWN,
        .simulate_rate_limit = true,
        .requests_per_minute = OPENROUTER_RPM_PAID,
        .current_minute_requests = 0,
        .support_tools = true,       // OpenRouter supports tool calling (OpenAI format)
        .tool_response_json = NULL,
        .support_streaming = true,   // OpenRouter supports streaming
        .stream_chunk_size = 35,
        .stream_delay_ms = 30,
    };

    MockProvider* mock = mock_provider_create_with_config(&config);
    if (mock) {
        mock_set_response_handler(mock, openrouter_response_handler, NULL);
        mock->base.name = "Mock OpenRouter";
        mock->base.api_key_env = "OPENROUTER_API_KEY";
        mock->base.base_url = "https://openrouter.ai/api/v1";
    }
    return mock;
}

/**
 * Create mock configured as DeepSeek R1 (cheap reasoning)
 */
MockProvider* mock_openrouter_deepseek_r1(void) {
    MockProvider* mock = mock_openrouter_create();
    if (mock) {
        mock->config.response_delay_ms = 500;  // R1 takes time to reason
        mock_set_response(mock, "[DeepSeek R1] <think>reasoning process...</think> Conclusion based on analysis.");
    }
    return mock;
}

/**
 * Create mock configured as Llama 3.3 70B (fast, cheap)
 */
MockProvider* mock_openrouter_llama33(void) {
    MockProvider* mock = mock_openrouter_create();
    if (mock) {
        mock->config.response_delay_ms = 150;  // Llama is fast
        mock_set_response(mock, "[Llama 3.3 70B] Open-source response with broad capabilities.");
    }
    return mock;
}

/**
 * Create mock configured as Mistral Large (multilingual)
 */
MockProvider* mock_openrouter_mistral_large(void) {
    MockProvider* mock = mock_openrouter_create();
    if (mock) {
        mock->config.response_delay_ms = 200;
        mock_set_response(mock, "[Mistral Large] Response optimisée pour le français et autres langues européennes.");
    }
    return mock;
}

/**
 * Create mock configured as Qwen 2.5 72B (chinese/multilingual)
 */
MockProvider* mock_openrouter_qwen(void) {
    MockProvider* mock = mock_openrouter_create();
    if (mock) {
        mock->config.response_delay_ms = 180;
        mock_set_response(mock, "[Qwen 2.5 72B] Multilingual response with strong Chinese support.");
    }
    return mock;
}

/**
 * Create mock simulating OpenRouter rate limit
 */
MockProvider* mock_openrouter_rate_limited(void) {
    MockProvider* mock = mock_openrouter_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_RATE_LIMIT);
    }
    return mock;
}

/**
 * Create mock simulating OpenRouter auth error (invalid API key)
 */
MockProvider* mock_openrouter_auth_error(void) {
    MockProvider* mock = mock_openrouter_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_AUTH);
    }
    return mock;
}

/**
 * Create mock simulating OpenRouter model not found
 */
MockProvider* mock_openrouter_model_not_found(void) {
    MockProvider* mock = mock_openrouter_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_MODEL);
    }
    return mock;
}

/**
 * Create mock simulating OpenRouter credit exhausted
 */
MockProvider* mock_openrouter_no_credits(void) {
    MockProvider* mock = mock_openrouter_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_QUOTA);
    }
    return mock;
}
