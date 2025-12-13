/**
 * CONVERGIO MOCK OLLAMA PROVIDER
 *
 * Specialized mock for Ollama local API behavior simulation.
 * Simulates local model inference with no API costs.
 * Ollama runs at localhost:11434 and has its own JSON format.
 */

#include "../mock_provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// OLLAMA-SPECIFIC CONSTANTS
// ============================================================================

// Ollama has no rate limits (local), but has hardware constraints
#define OLLAMA_TYPICAL_TPS       30      // Typical tokens per second on M1
#define OLLAMA_FAST_TPS          50      // Fast on M3 Pro/Max

// Token costs (all zero - local inference)
#define OLLAMA_INPUT_COST        0.0     // Free
#define OLLAMA_OUTPUT_COST       0.0     // Free

// Model sizes (GB VRAM)
#define LLAMA32_7B_VRAM          5.0     // 5GB for 7B model
#define LLAMA32_70B_VRAM         40.0    // 40GB for 70B model
#define MISTRAL_7B_VRAM          5.0     // 5GB
#define CODELLAMA_7B_VRAM        5.0     // 5GB
#define DEEPSEEK_CODER_VRAM      10.0    // 10GB for 16B model

// ============================================================================
// OLLAMA-SPECIFIC RESPONSE HANDLER
// ============================================================================

static char* ollama_response_handler(const char* model, const char* system,
                                     const char* user, void* ctx) {
    (void)ctx;
    (void)system;

    // Simulate Ollama's local model response
    const char* model_name = "Local Model";
    if (model) {
        if (strstr(model, "llama3.2")) model_name = "Llama 3.2 (Local)";
        else if (strstr(model, "llama3:70b")) model_name = "Llama 3 70B (Local)";
        else if (strstr(model, "mistral")) model_name = "Mistral 7B (Local)";
        else if (strstr(model, "codellama")) model_name = "Code Llama (Local)";
        else if (strstr(model, "deepseek-coder")) model_name = "DeepSeek Coder (Local)";
        else if (strstr(model, "phi3")) model_name = "Phi-3 (Local)";
        else if (strstr(model, "qwen")) model_name = "Qwen (Local)";
    }

    size_t len = strlen(model_name) + strlen(user) + 150;
    char* response = malloc(len);
    if (response) {
        snprintf(response, len,
                 "[%s] Local inference: \"%s\" - No API cost!",
                 model_name, user);
    }
    return response;
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Create mock configured like Ollama (local)
 */
MockProvider* mock_ollama_create(void) {
    MockProviderConfig config = {
        .default_response = "I'm running locally via Ollama. No API costs!",
        .echo_prompt = false,
        .response_delay_ms = 100,    // Local is typically fast
        .tokens_per_word_input = 1,
        .tokens_per_word_output = 1,
        .simulate_errors = false,
        .error_rate = 0.0,
        .error_to_simulate = PROVIDER_ERR_UNKNOWN,
        .simulate_rate_limit = false,  // No rate limits for local
        .requests_per_minute = 0,      // Unlimited
        .current_minute_requests = 0,
        .support_tools = false,        // Ollama doesn't support native tool calling
        .tool_response_json = NULL,
        .support_streaming = true,     // Ollama supports streaming
        .stream_chunk_size = 20,       // Smaller chunks for local
        .stream_delay_ms = 20,
    };

    MockProvider* mock = mock_provider_create_with_config(&config);
    if (mock) {
        mock_set_response_handler(mock, ollama_response_handler, NULL);
        mock->base.name = "Mock Ollama";
        mock->base.api_key_env = NULL;  // No API key needed
        mock->base.base_url = "http://localhost:11434";
    }
    return mock;
}

/**
 * Create mock configured as Llama 3.2 (default local model)
 */
MockProvider* mock_ollama_llama32(void) {
    MockProvider* mock = mock_ollama_create();
    if (mock) {
        mock->config.response_delay_ms = 100;
        mock_set_response(mock, "[Llama 3.2 Local] Efficient local inference with good quality.");
    }
    return mock;
}

/**
 * Create mock configured as Mistral 7B (fast, lightweight)
 */
MockProvider* mock_ollama_mistral(void) {
    MockProvider* mock = mock_ollama_create();
    if (mock) {
        mock->config.response_delay_ms = 80;  // Mistral 7B is fast
        mock_set_response(mock, "[Mistral 7B Local] Quick local response.");
    }
    return mock;
}

/**
 * Create mock configured as Code Llama (coding specialist)
 */
MockProvider* mock_ollama_codellama(void) {
    MockProvider* mock = mock_ollama_create();
    if (mock) {
        mock->config.response_delay_ms = 120;
        mock_set_response(mock, "[Code Llama Local] ```python\ndef hello():\n    print('Hello from local!')\n```");
    }
    return mock;
}

/**
 * Create mock configured as DeepSeek Coder V2 (larger coding model)
 */
MockProvider* mock_ollama_deepseek_coder(void) {
    MockProvider* mock = mock_ollama_create();
    if (mock) {
        mock->config.response_delay_ms = 200;  // Larger model, slower
        mock_set_response(mock, "[DeepSeek Coder V2 Local] Advanced code generation locally.");
    }
    return mock;
}

/**
 * Create mock configured as Phi-3 (small, efficient)
 */
MockProvider* mock_ollama_phi3(void) {
    MockProvider* mock = mock_ollama_create();
    if (mock) {
        mock->config.response_delay_ms = 50;  // Phi-3 is very fast
        mock_set_response(mock, "[Phi-3 Local] Ultra-fast local inference.");
    }
    return mock;
}

/**
 * Create mock simulating Ollama not running (connection refused)
 */
MockProvider* mock_ollama_not_running(void) {
    MockProvider* mock = mock_ollama_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_NETWORK);
    }
    return mock;
}

/**
 * Create mock simulating model not pulled
 */
MockProvider* mock_ollama_model_not_found(void) {
    MockProvider* mock = mock_ollama_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_MODEL);
    }
    return mock;
}

/**
 * Create mock simulating out of memory (model too large)
 */
MockProvider* mock_ollama_out_of_memory(void) {
    MockProvider* mock = mock_ollama_create();
    if (mock) {
        // Simulate OOM as a generic error
        mock_set_error_simulation(mock, true, 1.0, PROVIDER_ERR_UNKNOWN);
    }
    return mock;
}

/**
 * Create mock simulating slow hardware (e.g., no GPU)
 */
MockProvider* mock_ollama_slow_cpu(void) {
    MockProvider* mock = mock_ollama_create();
    if (mock) {
        mock->config.response_delay_ms = 2000;  // Very slow on CPU
        mock->config.stream_delay_ms = 200;
        mock_set_response(mock, "[Llama Local - CPU] Running on CPU, please wait...");
    }
    return mock;
}
