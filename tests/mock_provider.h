/**
 * CONVERGIO MOCK PROVIDER
 *
 * Mock LLM provider for testing purposes.
 * Simulates API responses without making actual network calls.
 *
 * Features:
 * - Configurable responses
 * - Latency simulation
 * - Error injection
 * - Request logging
 * - Rate limit simulation
 */

#ifndef CONVERGIO_MOCK_PROVIDER_H
#define CONVERGIO_MOCK_PROVIDER_H

#include "nous/provider.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// MOCK CONFIGURATION
// ============================================================================

typedef struct {
    // Response configuration
    const char* default_response;      // Default response text
    bool echo_prompt;                  // Include prompt in response
    int response_delay_ms;             // Simulated latency

    // Token counting
    uint64_t tokens_per_word_input;    // Tokens per word (input)
    uint64_t tokens_per_word_output;   // Tokens per word (output)

    // Error simulation
    bool simulate_errors;
    double error_rate;                 // 0.0 - 1.0
    ProviderError error_to_simulate;

    // Rate limiting
    bool simulate_rate_limit;
    int requests_per_minute;
    int current_minute_requests;

    // Tool calling
    bool support_tools;
    const char* tool_response_json;    // Pre-configured tool response

    // Streaming
    bool support_streaming;
    int stream_chunk_size;             // Characters per chunk
    int stream_delay_ms;               // Delay between chunks
} MockProviderConfig;

// ============================================================================
// REQUEST LOGGING
// ============================================================================

typedef struct MockRequest {
    char* model;
    char* system;
    char* user;
    char* tools_json;
    time_t timestamp;
    struct MockRequest* next;
} MockRequest;

typedef struct {
    MockRequest* requests;
    size_t request_count;
    uint64_t total_input_tokens;
    uint64_t total_output_tokens;
} MockRequestLog;

// ============================================================================
// MOCK PROVIDER STATE
// ============================================================================

typedef struct {
    Provider base;                     // Base provider interface
    MockProviderConfig config;
    MockRequestLog log;
    bool initialized;
} MockProvider;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Create a mock provider with default configuration
 */
MockProvider* mock_provider_create(void);

/**
 * Create a mock provider with custom configuration
 */
MockProvider* mock_provider_create_with_config(MockProviderConfig* config);

/**
 * Destroy a mock provider
 */
void mock_provider_destroy(MockProvider* mock);

/**
 * Get the base Provider pointer for use with provider API
 */
Provider* mock_provider_as_provider(MockProvider* mock);

// ============================================================================
// CONFIGURATION
// ============================================================================

/**
 * Set the default response text
 */
void mock_set_response(MockProvider* mock, const char* response);

/**
 * Set response delay in milliseconds
 */
void mock_set_latency(MockProvider* mock, int delay_ms);

/**
 * Configure error simulation
 */
void mock_set_error_simulation(MockProvider* mock, bool enabled,
                                double rate, ProviderError error);

/**
 * Configure rate limiting
 */
void mock_set_rate_limit(MockProvider* mock, bool enabled, int rpm);

/**
 * Configure streaming behavior
 */
void mock_set_streaming(MockProvider* mock, bool enabled,
                         int chunk_size, int delay_ms);

/**
 * Set tool calling response
 */
void mock_set_tool_response(MockProvider* mock, const char* tools_json);

/**
 * Set a response handler function for dynamic responses
 */
typedef char* (*MockResponseHandler)(const char* model, const char* system,
                                      const char* user, void* ctx);
void mock_set_response_handler(MockProvider* mock, MockResponseHandler handler, void* ctx);

// ============================================================================
// REQUEST LOGGING
// ============================================================================

/**
 * Get the request log
 */
MockRequestLog* mock_get_log(MockProvider* mock);

/**
 * Clear the request log
 */
void mock_clear_log(MockProvider* mock);

/**
 * Get the last request
 */
MockRequest* mock_get_last_request(MockProvider* mock);

/**
 * Get request count
 */
size_t mock_get_request_count(MockProvider* mock);

// ============================================================================
// ASSERTIONS (for testing)
// ============================================================================

/**
 * Assert that a specific number of requests were made
 */
bool mock_assert_request_count(MockProvider* mock, size_t expected);

/**
 * Assert that the last request used a specific model
 */
bool mock_assert_last_model(MockProvider* mock, const char* expected_model);

/**
 * Assert that the last request contained text in the prompt
 */
bool mock_assert_last_prompt_contains(MockProvider* mock, const char* text);

/**
 * Assert that the last request included tools
 */
bool mock_assert_last_had_tools(MockProvider* mock);

/**
 * Get total tokens processed
 */
void mock_get_total_tokens(MockProvider* mock, uint64_t* input, uint64_t* output);

// ============================================================================
// PRE-CONFIGURED MOCKS
// ============================================================================

/**
 * Create a mock that always returns success
 */
MockProvider* mock_provider_success(const char* response);

/**
 * Create a mock that always returns errors
 */
MockProvider* mock_provider_error(ProviderError error);

/**
 * Create a mock with realistic latency
 */
MockProvider* mock_provider_realistic(void);

/**
 * Create a mock that simulates rate limiting
 */
MockProvider* mock_provider_rate_limited(int rpm);

// ============================================================================
// PROVIDER-SPECIFIC MOCKS
// ============================================================================

// --- Anthropic ---
MockProvider* mock_anthropic_create(void);
MockProvider* mock_anthropic_opus(void);
MockProvider* mock_anthropic_sonnet(void);
MockProvider* mock_anthropic_rate_limited(void);
MockProvider* mock_anthropic_auth_error(void);

// --- OpenAI ---
MockProvider* mock_openai_create(void);
MockProvider* mock_openai_gpt4o(void);
MockProvider* mock_openai_gpt4o_mini(void);
MockProvider* mock_openai_o1(void);
MockProvider* mock_openai_rate_limited(void);
MockProvider* mock_openai_auth_error(void);
MockProvider* mock_openai_quota_exceeded(void);

// --- Gemini ---
MockProvider* mock_gemini_create(void);
MockProvider* mock_gemini_pro(void);
MockProvider* mock_gemini_flash(void);
MockProvider* mock_gemini_rate_limited(void);
MockProvider* mock_gemini_auth_error(void);

// --- OpenRouter ---
MockProvider* mock_openrouter_create(void);
MockProvider* mock_openrouter_deepseek_r1(void);
MockProvider* mock_openrouter_llama33(void);
MockProvider* mock_openrouter_mistral_large(void);
MockProvider* mock_openrouter_qwen(void);
MockProvider* mock_openrouter_rate_limited(void);
MockProvider* mock_openrouter_auth_error(void);
MockProvider* mock_openrouter_model_not_found(void);
MockProvider* mock_openrouter_no_credits(void);

// --- Ollama (Local) ---
MockProvider* mock_ollama_create(void);
MockProvider* mock_ollama_llama32(void);
MockProvider* mock_ollama_mistral(void);
MockProvider* mock_ollama_codellama(void);
MockProvider* mock_ollama_deepseek_coder(void);
MockProvider* mock_ollama_phi3(void);
MockProvider* mock_ollama_not_running(void);
MockProvider* mock_ollama_model_not_found(void);
MockProvider* mock_ollama_out_of_memory(void);
MockProvider* mock_ollama_slow_cpu(void);

// ============================================================================
// RESPONSE TEMPLATES
// ============================================================================

// Standard successful response
#define MOCK_RESPONSE_SUCCESS "I understand your request. Here is my response based on the input provided."

// Code completion response
#define MOCK_RESPONSE_CODE "```\nfunction example() {\n    return 'mock response';\n}\n```"

// Error message response
#define MOCK_RESPONSE_ERROR "I apologize, but I encountered an issue processing your request."

// Tool use response
#define MOCK_RESPONSE_TOOL_USE "{\"tool\":\"read_file\",\"arguments\":{\"path\":\"test.txt\"}}"

// Streaming chunks
#define MOCK_CHUNK_1 "This is "
#define MOCK_CHUNK_2 "a streaming "
#define MOCK_CHUNK_3 "response from "
#define MOCK_CHUNK_4 "the mock provider."

#endif // CONVERGIO_MOCK_PROVIDER_H
