/**
 * CONVERGIO MOCK PROVIDER
 *
 * Implementation of mock LLM provider for testing.
 */

#include "mock_provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// ============================================================================
// INTERNAL STATE
// ============================================================================

static MockResponseHandler g_response_handler = NULL;
static void* g_response_handler_ctx = NULL;

// ============================================================================
// HELPERS
// ============================================================================

static char* safe_strdup(const char* s) {
    return s ? strdup(s) : NULL;
}

static void simulate_delay(int delay_ms) {
    if (delay_ms > 0) {
        usleep(delay_ms * 1000);
    }
}

static uint64_t count_words(const char* text) {
    if (!text) return 0;

    uint64_t count = 0;
    bool in_word = false;

    while (*text) {
        if (*text == ' ' || *text == '\n' || *text == '\t') {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            count++;
        }
        text++;
    }

    return count;
}

static bool should_error(MockProvider* mock) {
    if (!mock->config.simulate_errors) return false;
    if (mock->config.error_rate <= 0.0) return false;

    double r = (double)rand() / RAND_MAX;
    return r < mock->config.error_rate;
}

static bool check_rate_limit(MockProvider* mock) {
    if (!mock->config.simulate_rate_limit) return false;

    // Simple rate limiting - reset every minute
    static time_t last_reset = 0;
    time_t now = time(NULL);

    if (now - last_reset >= 60) {
        mock->config.current_minute_requests = 0;
        last_reset = now;
    }

    if (mock->config.current_minute_requests >= mock->config.requests_per_minute) {
        return true;  // Rate limited
    }

    mock->config.current_minute_requests++;
    return false;
}

static void log_request(MockProvider* mock, const char* model,
                        const char* system, const char* user,
                        const char* tools_json) {
    MockRequest* req = calloc(1, sizeof(MockRequest));
    if (!req) return;

    req->model = safe_strdup(model);
    req->system = safe_strdup(system);
    req->user = safe_strdup(user);
    req->tools_json = safe_strdup(tools_json);
    req->timestamp = time(NULL);

    // Add to front of list
    req->next = mock->log.requests;
    mock->log.requests = req;
    mock->log.request_count++;
}

static void free_request(MockRequest* req) {
    if (!req) return;
    free(req->model);
    free(req->system);
    free(req->user);
    free(req->tools_json);
    free(req);
}

// ============================================================================
// PROVIDER INTERFACE IMPLEMENTATIONS
// ============================================================================

static ProviderError mock_init(Provider* self) {
    MockProvider* mock = (MockProvider*)self;
    mock->initialized = true;
    self->initialized = true;
    return PROVIDER_OK;
}

static void mock_shutdown(Provider* self) {
    MockProvider* mock = (MockProvider*)self;
    mock_clear_log(mock);
    mock->initialized = false;
    self->initialized = false;
}

static bool mock_validate_key(Provider* self) {
    (void)self;
    return true;  // Mock always has valid "key"
}

static char* mock_chat(Provider* self, const char* model, const char* system,
                       const char* user, TokenUsage* usage) {
    MockProvider* mock = (MockProvider*)self;

    // Log request
    log_request(mock, model, system, user, NULL);

    // Check rate limit
    if (check_rate_limit(mock)) {
        if (usage) usage->estimated_cost = 0;
        return NULL;  // Rate limited
    }

    // Simulate error if configured
    if (should_error(mock)) {
        if (usage) usage->estimated_cost = 0;
        return NULL;
    }

    // Simulate latency
    simulate_delay(mock->config.response_delay_ms);

    // Generate response
    char* response = NULL;

    if (g_response_handler) {
        response = g_response_handler(model, system, user, g_response_handler_ctx);
    }

    if (!response && mock->config.echo_prompt) {
        // Echo the prompt with prefix
        size_t len = strlen(user) + 64;
        response = malloc(len);
        if (response) {
            snprintf(response, len, "[MOCK] Received: %s", user);
        }
    }

    if (!response && mock->config.default_response) {
        response = strdup(mock->config.default_response);
    }

    if (!response) {
        response = strdup(MOCK_RESPONSE_SUCCESS);
    }

    // Calculate tokens
    if (usage) {
        uint64_t input_words = count_words(system) + count_words(user);
        uint64_t output_words = count_words(response);

        usage->input_tokens = input_words * mock->config.tokens_per_word_input;
        usage->output_tokens = output_words * mock->config.tokens_per_word_output;
        usage->cached_tokens = 0;

        // Estimate cost (using mock pricing)
        usage->estimated_cost = (usage->input_tokens * 0.001 + usage->output_tokens * 0.003) / 1000;

        // Update log totals
        mock->log.total_input_tokens += usage->input_tokens;
        mock->log.total_output_tokens += usage->output_tokens;
    }

    return response;
}

static char* mock_chat_with_tools(Provider* self, const char* model,
                                   const char* system, const char* user,
                                   ToolDefinition* tools, size_t tool_count,
                                   ToolCall** out_tool_calls, size_t* out_tool_count,
                                   TokenUsage* usage) {
    MockProvider* mock = (MockProvider*)self;
    (void)tools;
    (void)tool_count;

    // Log request with tools indicator
    log_request(mock, model, system, user, tool_count > 0 ? "[tools]" : NULL);

    // Initialize output params
    if (out_tool_calls) *out_tool_calls = NULL;
    if (out_tool_count) *out_tool_count = 0;

    // If tool response is configured, return it
    if (mock->config.support_tools && mock->config.tool_response_json) {
        if (usage) {
            usage->input_tokens = 100;
            usage->output_tokens = 50;
            usage->cached_tokens = 0;
            usage->estimated_cost = 0.0005;
        }
        simulate_delay(mock->config.response_delay_ms);
        return strdup(mock->config.tool_response_json);
    }

    // Otherwise use regular chat
    return mock_chat(self, model, system, user, usage);
}

static ProviderError mock_stream_chat(Provider* self, const char* model,
                                       const char* system, const char* user,
                                       StreamHandler* handler, TokenUsage* usage) {
    MockProvider* mock = (MockProvider*)self;

    if (!mock->config.support_streaming || !handler || !handler->on_chunk) {
        // Fall back to non-streaming
        char* response = mock_chat(self, model, system, user, usage);
        if (response) {
            if (handler && handler->on_chunk) {
                handler->on_chunk(response, true, handler->user_ctx);
            }
            if (handler && handler->on_complete) {
                handler->on_complete(response, handler->user_ctx);
            }
            free(response);
            return PROVIDER_OK;
        }
        return PROVIDER_ERR_UNKNOWN;
    }

    // Log request
    log_request(mock, model, system, user, NULL);

    // Simulate streaming
    const char* chunks[] = {MOCK_CHUNK_1, MOCK_CHUNK_2, MOCK_CHUNK_3, MOCK_CHUNK_4};
    size_t chunk_count = sizeof(chunks) / sizeof(chunks[0]);

    simulate_delay(mock->config.response_delay_ms);

    // Build full response for on_complete callback
    char full_response[1024] = "";
    for (size_t i = 0; i < chunk_count; i++) {
        bool is_done = (i == chunk_count - 1);
        handler->on_chunk(chunks[i], is_done, handler->user_ctx);
        strncat(full_response, chunks[i], sizeof(full_response) - strlen(full_response) - 1);

        if (!is_done) {
            simulate_delay(mock->config.stream_delay_ms);
        }
    }

    if (handler->on_complete) {
        handler->on_complete(full_response, handler->user_ctx);
    }

    if (usage) {
        usage->input_tokens = 50;
        usage->output_tokens = 20;
        usage->cached_tokens = 0;
        usage->estimated_cost = 0.0002;
    }

    return PROVIDER_OK;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

MockProvider* mock_provider_create(void) {
    MockProviderConfig config = {
        .default_response = MOCK_RESPONSE_SUCCESS,
        .echo_prompt = false,
        .response_delay_ms = 0,
        .tokens_per_word_input = 2,
        .tokens_per_word_output = 2,
        .simulate_errors = false,
        .error_rate = 0.0,
        .error_to_simulate = PROVIDER_ERR_UNKNOWN,
        .simulate_rate_limit = false,
        .requests_per_minute = 100,
        .current_minute_requests = 0,
        .support_tools = false,
        .tool_response_json = NULL,
        .support_streaming = false,
        .stream_chunk_size = 20,
        .stream_delay_ms = 50,
    };

    return mock_provider_create_with_config(&config);
}

MockProvider* mock_provider_create_with_config(MockProviderConfig* config) {
    MockProvider* mock = calloc(1, sizeof(MockProvider));
    if (!mock) return NULL;

    // Copy config
    mock->config = *config;
    mock->config.default_response = safe_strdup(config->default_response);
    mock->config.tool_response_json = safe_strdup(config->tool_response_json);

    // Initialize base provider
    mock->base.type = PROVIDER_COUNT;  // Special value for mock
    mock->base.name = "Mock Provider";
    mock->base.api_key_env = "MOCK_API_KEY";
    mock->base.base_url = "http://mock.local";
    mock->base.initialized = false;

    // Set function pointers
    mock->base.init = mock_init;
    mock->base.shutdown = mock_shutdown;
    mock->base.validate_key = mock_validate_key;
    mock->base.chat = mock_chat;
    mock->base.chat_with_tools = mock_chat_with_tools;
    mock->base.stream_chat = mock_stream_chat;

    // Initialize log
    mock->log.requests = NULL;
    mock->log.request_count = 0;
    mock->log.total_input_tokens = 0;
    mock->log.total_output_tokens = 0;

    return mock;
}

void mock_provider_destroy(MockProvider* mock) {
    if (!mock) return;

    mock_clear_log(mock);
    free((char*)mock->config.default_response);
    free((char*)mock->config.tool_response_json);
    free(mock);
}

Provider* mock_provider_as_provider(MockProvider* mock) {
    return mock ? &mock->base : NULL;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void mock_set_response(MockProvider* mock, const char* response) {
    if (!mock) return;
    free((char*)mock->config.default_response);
    mock->config.default_response = safe_strdup(response);
}

void mock_set_latency(MockProvider* mock, int delay_ms) {
    if (!mock) return;
    mock->config.response_delay_ms = delay_ms;
}

void mock_set_error_simulation(MockProvider* mock, bool enabled,
                                double rate, ProviderError error) {
    if (!mock) return;
    mock->config.simulate_errors = enabled;
    mock->config.error_rate = rate;
    mock->config.error_to_simulate = error;
}

void mock_set_rate_limit(MockProvider* mock, bool enabled, int rpm) {
    if (!mock) return;
    mock->config.simulate_rate_limit = enabled;
    mock->config.requests_per_minute = rpm;
    mock->config.current_minute_requests = 0;
}

void mock_set_streaming(MockProvider* mock, bool enabled,
                         int chunk_size, int delay_ms) {
    if (!mock) return;
    mock->config.support_streaming = enabled;
    mock->config.stream_chunk_size = chunk_size;
    mock->config.stream_delay_ms = delay_ms;
}

void mock_set_tool_response(MockProvider* mock, const char* tools_json) {
    if (!mock) return;
    free((char*)mock->config.tool_response_json);
    mock->config.tool_response_json = safe_strdup(tools_json);
    mock->config.support_tools = (tools_json != NULL);
}

void mock_set_response_handler(MockProvider* mock, MockResponseHandler handler, void* ctx) {
    (void)mock;
    g_response_handler = handler;
    g_response_handler_ctx = ctx;
}

// ============================================================================
// REQUEST LOGGING
// ============================================================================

MockRequestLog* mock_get_log(MockProvider* mock) {
    return mock ? &mock->log : NULL;
}

void mock_clear_log(MockProvider* mock) {
    if (!mock) return;

    MockRequest* req = mock->log.requests;
    while (req) {
        MockRequest* next = req->next;
        free_request(req);
        req = next;
    }

    mock->log.requests = NULL;
    mock->log.request_count = 0;
    mock->log.total_input_tokens = 0;
    mock->log.total_output_tokens = 0;
}

MockRequest* mock_get_last_request(MockProvider* mock) {
    return mock ? mock->log.requests : NULL;
}

size_t mock_get_request_count(MockProvider* mock) {
    return mock ? mock->log.request_count : 0;
}

// ============================================================================
// ASSERTIONS
// ============================================================================

bool mock_assert_request_count(MockProvider* mock, size_t expected) {
    return mock && mock->log.request_count == expected;
}

bool mock_assert_last_model(MockProvider* mock, const char* expected_model) {
    if (!mock || !mock->log.requests || !expected_model) return false;
    return mock->log.requests->model &&
           strcmp(mock->log.requests->model, expected_model) == 0;
}

bool mock_assert_last_prompt_contains(MockProvider* mock, const char* text) {
    if (!mock || !mock->log.requests || !text) return false;

    MockRequest* req = mock->log.requests;
    if (req->system && strstr(req->system, text)) return true;
    if (req->user && strstr(req->user, text)) return true;

    return false;
}

bool mock_assert_last_had_tools(MockProvider* mock) {
    if (!mock || !mock->log.requests) return false;
    return mock->log.requests->tools_json != NULL;
}

void mock_get_total_tokens(MockProvider* mock, uint64_t* input, uint64_t* output) {
    if (!mock) return;
    if (input) *input = mock->log.total_input_tokens;
    if (output) *output = mock->log.total_output_tokens;
}

// ============================================================================
// PRE-CONFIGURED MOCKS
// ============================================================================

MockProvider* mock_provider_success(const char* response) {
    MockProvider* mock = mock_provider_create();
    if (mock) {
        mock_set_response(mock, response);
    }
    return mock;
}

MockProvider* mock_provider_error(ProviderError error) {
    MockProvider* mock = mock_provider_create();
    if (mock) {
        mock_set_error_simulation(mock, true, 1.0, error);
    }
    return mock;
}

MockProvider* mock_provider_realistic(void) {
    MockProviderConfig config = {
        .default_response = MOCK_RESPONSE_SUCCESS,
        .echo_prompt = false,
        .response_delay_ms = 500,  // 500ms latency
        .tokens_per_word_input = 2,
        .tokens_per_word_output = 2,
        .simulate_errors = true,
        .error_rate = 0.01,  // 1% error rate
        .error_to_simulate = PROVIDER_ERR_NETWORK,
        .simulate_rate_limit = true,
        .requests_per_minute = 60,
        .support_tools = true,
        .tool_response_json = NULL,
        .support_streaming = true,
        .stream_chunk_size = 20,
        .stream_delay_ms = 50,
    };

    return mock_provider_create_with_config(&config);
}

MockProvider* mock_provider_rate_limited(int rpm) {
    MockProvider* mock = mock_provider_create();
    if (mock) {
        mock_set_rate_limit(mock, true, rpm);
    }
    return mock;
}
