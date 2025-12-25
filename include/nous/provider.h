/**
 * CONVERGIO MULTI-MODEL PROVIDER ABSTRACTION LAYER
 *
 * Unified interface for multiple LLM providers:
 * - Anthropic (Claude)
 * - OpenAI (GPT)
 * - Google (Gemini)
 * - Ollama (Local models)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_PROVIDER_H
#define CONVERGIO_PROVIDER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// ============================================================================
// PROVIDER TYPES
// ============================================================================

typedef enum {
    PROVIDER_ANTHROPIC   = 0,
    PROVIDER_OPENAI      = 1,
    PROVIDER_GEMINI      = 2,
    PROVIDER_OPENROUTER  = 3,
    PROVIDER_OLLAMA      = 4,
    PROVIDER_MLX         = 5,   // Local MLX inference (Apple Silicon native)
    PROVIDER_APPLE_FOUNDATION = 6,  // Apple Foundation Models (macOS 26+)
    PROVIDER_COUNT       = 7
} ProviderType;

// Cost tier for model selection
typedef enum {
    COST_TIER_CHEAP,    // < $2/MTok
    COST_TIER_MID,      // $2-10/MTok
    COST_TIER_PREMIUM   // > $10/MTok
} CostTier;

// ============================================================================
// MODEL CONFIGURATION
// ============================================================================

typedef struct {
    const char* id;                     // e.g., "claude-opus-4.5"
    const char* display_name;           // e.g., "Claude Opus 4.5"
    ProviderType provider;
    double input_cost_per_mtok;         // $/million input tokens
    double output_cost_per_mtok;        // $/million output tokens
    double thinking_cost_per_mtok;      // $/million thinking tokens (if applicable)
    size_t context_window;              // Maximum context size
    size_t max_output;                  // Maximum output tokens
    bool supports_tools;                // Function/tool calling support
    bool supports_vision;               // Image input support
    bool supports_streaming;            // Streaming response support
    CostTier tier;                      // Cost tier classification
    const char* released;               // Release date (YYYY-MM-DD)
    bool deprecated;                    // Is this model deprecated?
} ModelConfig;

// ============================================================================
// PROVIDER ERROR HANDLING
// ============================================================================

typedef enum {
    PROVIDER_OK = 0,
    PROVIDER_ERR_AUTH,              // Invalid/expired API key
    PROVIDER_ERR_RATE_LIMIT,        // Too many requests
    PROVIDER_ERR_QUOTA,             // Quota exceeded
    PROVIDER_ERR_CONTEXT_LENGTH,    // Input too long
    PROVIDER_ERR_CONTENT_FILTER,    // Content policy violation
    PROVIDER_ERR_MODEL_NOT_FOUND,   // Model doesn't exist
    PROVIDER_ERR_OVERLOADED,        // Server overloaded
    PROVIDER_ERR_TIMEOUT,           // Request timeout
    PROVIDER_ERR_NETWORK,           // Network error
    PROVIDER_ERR_INVALID_REQUEST,   // Malformed request
    PROVIDER_ERR_NOT_INITIALIZED,   // Provider not initialized
    PROVIDER_ERR_UNKNOWN,           // Unknown error
} ProviderError;

typedef struct {
    ProviderError code;
    char* message;                  // Human-readable message
    char* provider_code;            // Original provider error code
    int http_status;
    bool is_retryable;
    int retry_after_ms;             // Hint from rate limit headers
} ProviderErrorInfo;

// ============================================================================
// TOKEN USAGE TRACKING
// ============================================================================

typedef struct {
    size_t input_tokens;
    size_t output_tokens;
    size_t cached_tokens;
    double estimated_cost;
} TokenUsage;

// ============================================================================
// STREAMING CALLBACK
// ============================================================================

typedef void (*StreamCallback)(const char* chunk, bool is_done, void* ctx);

typedef struct {
    StreamCallback on_chunk;
    void (*on_error)(const char* error, void* ctx);
    void (*on_complete)(const char* full_response, void* ctx);
    void* user_ctx;
} StreamHandler;

// ============================================================================
// TOOL/FUNCTION CALLING
// ============================================================================

typedef struct {
    const char* name;
    const char* description;
    const char* parameters_json;    // JSON Schema
} ToolDefinition;

typedef struct {
    char* tool_name;
    char* tool_id;
    char* arguments_json;
} ToolCall;

// ============================================================================
// PROVIDER INTERFACE (ADAPTER PATTERN)
// ============================================================================

typedef struct Provider Provider;

struct Provider {
    ProviderType type;
    const char* name;
    const char* api_key_env;        // Environment variable name for API key
    const char* base_url;
    bool initialized;

    // ========================================================================
    // CORE OPERATIONS
    // ========================================================================

    /**
     * Initialize the provider with API credentials
     * @return PROVIDER_OK on success, error code otherwise
     */
    ProviderError (*init)(Provider* self);

    /**
     * Shutdown and cleanup the provider
     */
    void (*shutdown)(Provider* self);

    /**
     * Validate API key is set and valid
     * @return true if API key is valid
     */
    bool (*validate_key)(Provider* self);

    // ========================================================================
    // CHAT OPERATIONS
    // ========================================================================

    /**
     * Send a chat request and get a response
     * @param model Model ID to use
     * @param system System prompt
     * @param user User message
     * @param usage Output parameter for token usage (can be NULL)
     * @return Response string (caller must free) or NULL on error
     */
    char* (*chat)(Provider* self, const char* model, const char* system,
                  const char* user, TokenUsage* usage);

    /**
     * Send a chat request with tool definitions
     * @param model Model ID to use
     * @param system System prompt
     * @param user User message
     * @param tools Array of tool definitions
     * @param tool_count Number of tools
     * @param out_tool_calls Output array of tool calls (caller must free)
     * @param out_tool_count Number of tool calls returned
     * @param usage Output parameter for token usage (can be NULL)
     * @return Response string (caller must free) or NULL on error
     */
    char* (*chat_with_tools)(Provider* self, const char* model, const char* system,
                             const char* user, ToolDefinition* tools, size_t tool_count,
                             ToolCall** out_tool_calls, size_t* out_tool_count,
                             TokenUsage* usage);

    /**
     * Stream a chat response
     * @param model Model ID to use
     * @param system System prompt
     * @param user User message
     * @param handler Streaming callbacks
     * @param usage Output parameter for token usage (can be NULL)
     * @return PROVIDER_OK on success, error code otherwise
     */
    ProviderError (*stream_chat)(Provider* self, const char* model, const char* system,
                                 const char* user, StreamHandler* handler, TokenUsage* usage);

    // ========================================================================
    // UTILITY OPERATIONS
    // ========================================================================

    /**
     * Estimate token count for text
     * @param text Text to tokenize
     * @return Estimated token count
     */
    size_t (*estimate_tokens)(Provider* self, const char* text);

    /**
     * Get the last error information
     * @return Error info struct (do not free)
     */
    ProviderErrorInfo* (*get_last_error)(Provider* self);

    /**
     * List available models for this provider
     * @param out_models Output array of model configs
     * @param out_count Number of models
     * @return PROVIDER_OK on success
     */
    ProviderError (*list_models)(Provider* self, ModelConfig** out_models, size_t* out_count);

    // ========================================================================
    // PROVIDER-SPECIFIC DATA
    // ========================================================================

    void* impl_data;                // Provider-specific implementation data
};

// ============================================================================
// PROVIDER REGISTRY
// ============================================================================

/**
 * Initialize the provider registry
 * Loads model configurations and initializes available providers
 * @return PROVIDER_OK on success
 */
ProviderError provider_registry_init(void);

// Map HTTP status code to ProviderError (for consistent error handling)
ProviderError provider_map_http_error(long http_code);

/**
 * Shutdown all providers and free resources
 */
void provider_registry_shutdown(void);

/**
 * Get a provider by type
 * @param type Provider type
 * @return Provider instance or NULL if not available
 */
Provider* provider_get(ProviderType type);

/**
 * Check if a provider is available (API key configured)
 * @param type Provider type
 * @return true if provider is available
 */
bool provider_is_available(ProviderType type);

/**
 * Get provider name string
 * @param type Provider type
 * @return Provider name
 */
const char* provider_name(ProviderType type);

// ============================================================================
// MODEL REGISTRY
// ============================================================================

/**
 * Get model configuration by ID
 * @param model_id Full model ID (e.g., "anthropic/claude-opus-4.5" or "claude-opus-4.5")
 * @return Model config or NULL if not found
 */
const ModelConfig* model_get_config(const char* model_id);

/**
 * Get all models for a provider
 * @param type Provider type
 * @param out_count Number of models returned
 * @return Array of model configs (do not free)
 */
const ModelConfig* model_get_by_provider(ProviderType type, size_t* out_count);

/**
 * Get models by cost tier
 * @param tier Cost tier
 * @param out_count Number of models returned
 * @return Array of model configs (do not free)
 */
const ModelConfig* model_get_by_tier(CostTier tier, size_t* out_count);

/**
 * Get the cheapest model for a provider
 * @param type Provider type
 * @return Model config or NULL if no models available
 */
const ModelConfig* model_get_cheapest(ProviderType type);

/**
 * Calculate estimated cost for a request
 * @param model_id Model ID
 * @param input_tokens Number of input tokens
 * @param output_tokens Number of output tokens
 * @return Estimated cost in USD
 */
double model_estimate_cost(const char* model_id, size_t input_tokens, size_t output_tokens);

// ============================================================================
// ERROR HANDLING UTILITIES
// ============================================================================

/**
 * Get user-friendly error message
 * @param code Error code
 * @return Human-readable error message
 */
const char* provider_error_message(ProviderError code);

/**
 * Check if error is retryable
 * @param code Error code
 * @return true if operation should be retried
 */
bool provider_error_is_retryable(ProviderError code);

/**
 * Free error info structure
 * @param info Error info to free
 */
void provider_error_free(ProviderErrorInfo* info);

// ============================================================================
// TOOL CALL UTILITIES
// ============================================================================

/**
 * Free tool call array
 * @param calls Array of tool calls
 * @param count Number of tool calls
 */
void tool_calls_free(ToolCall* calls, size_t count);

/**
 * Parse tool calls from Anthropic API response
 * @param response JSON response string
 * @param count Output: number of tool calls found
 * @return Array of ToolCall structs (caller must free with tool_calls_free)
 */
ToolCall* parse_anthropic_tool_calls(const char* response, size_t* count);

/**
 * Parse tool calls from OpenAI API response
 * @param response JSON response string
 * @param count Output: number of tool calls found
 * @return Array of ToolCall structs (caller must free with tool_calls_free)
 */
ToolCall* parse_openai_tool_calls(const char* response, size_t* count);

/**
 * Parse tool calls from Gemini API response
 * @param response JSON response string
 * @param count Output: number of tool calls found
 * @return Array of ToolCall structs (caller must free with tool_calls_free)
 */
ToolCall* parse_gemini_tool_calls(const char* response, size_t* count);

/**
 * Build tools JSON array for Anthropic API
 * @param tools Array of tool definitions
 * @param count Number of tools
 * @return JSON string (caller must free)
 */
char* build_anthropic_tools_json(ToolDefinition* tools, size_t count);

/**
 * Build tools JSON array for OpenAI API
 * @param tools Array of tool definitions
 * @param count Number of tools
 * @return JSON string (caller must free)
 */
char* build_openai_tools_json(ToolDefinition* tools, size_t count);

/**
 * Build tools JSON array for Gemini API
 * @param tools Array of tool definitions
 * @param count Number of tools
 * @return JSON string (caller must free)
 */
char* build_gemini_tools_json(ToolDefinition* tools, size_t count);

// ============================================================================
// RETRY CONFIGURATION
// ============================================================================

typedef struct {
    int max_retries;                // Default: 3
    int base_delay_ms;              // Default: 1000
    int max_delay_ms;               // Default: 60000
    double jitter_factor;           // Default: 0.2
    bool retry_on_rate_limit;       // Default: true
    bool retry_on_server_error;     // Default: true
} RetryConfig;

/**
 * Get default retry configuration
 * @return Default retry config
 */
RetryConfig retry_config_default(void);

/**
 * Calculate delay with exponential backoff + jitter
 * @param cfg Retry configuration
 * @param attempt Attempt number (0-indexed)
 * @return Delay in milliseconds
 */
int retry_calculate_delay(const RetryConfig* cfg, int attempt);

// ============================================================================
// STREAMING INFRASTRUCTURE
// ============================================================================

// Opaque stream context type
typedef struct StreamContext StreamContext;

/**
 * Create a new streaming context for a provider
 * @param provider Provider type
 * @return New stream context or NULL on error
 */
StreamContext* stream_context_create(ProviderType provider);

/**
 * Destroy a stream context and free resources
 * @param ctx Stream context to destroy
 */
void stream_context_destroy(StreamContext* ctx);

/**
 * Set callbacks for streaming events
 * @param ctx Stream context
 * @param on_chunk Called for each text chunk
 * @param on_complete Called when streaming completes
 * @param on_error Called on error
 * @param user_ctx User data passed to callbacks
 */
void stream_set_callbacks(StreamContext* ctx,
                          void (*on_chunk)(const char*, void*),
                          void (*on_complete)(const char*, TokenUsage*, void*),
                          void (*on_error)(ProviderError, const char*, void*),
                          void* user_ctx);

/**
 * Execute a streaming request
 * @param ctx Stream context
 * @param url API endpoint URL
 * @param body Request body JSON
 * @param api_key API key for authentication
 * @return 0 on success, 1 if cancelled, -1 on error
 */
int stream_execute(StreamContext* ctx, const char* url, const char* body,
                   const char* api_key);

/**
 * Cancel an ongoing stream
 * @param ctx Stream context
 */
void stream_cancel(StreamContext* ctx);

/**
 * Check if stream was cancelled
 * @param ctx Stream context
 * @return true if cancelled
 */
bool stream_is_cancelled(StreamContext* ctx);

/**
 * Get the accumulated response from a completed stream
 * @param ctx Stream context
 * @return Full response text (do not free)
 */
const char* stream_get_response(StreamContext* ctx);

/**
 * Unescape JSON string content (handles \n, \t, \", etc.)
 * @param input JSON-escaped string
 * @return Unescaped string (caller must free) or NULL on error
 */
char* stream_unescape_json(const char* input);

// ============================================================================
// PROVIDER FACTORY FUNCTIONS
// ============================================================================

/**
 * Create OpenRouter provider instance
 * OpenRouter provides access to 300+ models via OpenAI-compatible API
 * @return Provider instance (caller must free via shutdown)
 */
Provider* openrouter_provider_create(void);

/**
 * Create Ollama provider instance
 * Ollama runs local models with zero API costs
 * @return Provider instance (caller must free via shutdown)
 */
Provider* ollama_provider_create(void);

/**
 * Create MLX provider instance
 * MLX runs local models natively on Apple Silicon (Metal + Neural Engine)
 * No external dependencies - 100% offline capable
 * @return Provider instance (caller must free via shutdown)
 */
Provider* mlx_provider_create(void);

/**
 * Get API key environment variable name for a provider
 * @param type Provider type
 * @return Environment variable name or NULL for local providers
 */
const char* provider_get_api_key_env(ProviderType type);

/**
 * Get human-readable name for a provider
 * @param type Provider type
 * @return Provider display name
 */
const char* provider_get_name(ProviderType type);

#endif // CONVERGIO_PROVIDER_H
