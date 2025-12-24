/**
 * CONVERGIO PROVIDER REGISTRY
 *
 * Central registry for managing multiple LLM providers
 * Handles initialization, model lookup, and provider selection
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include "nous/model_loader.h"
#include "nous/nous.h"
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// PROVIDER REGISTRY STATE
// ============================================================================

static Provider* g_providers[PROVIDER_COUNT] = {NULL};
static bool g_registry_initialized = false;
static pthread_mutex_t g_registry_mutex = PTHREAD_MUTEX_INITIALIZER;

// ============================================================================
// BUILT-IN MODEL CONFIGURATIONS (December 2025)
// ============================================================================

// Anthropic Models (December 2025)
static ModelConfig g_anthropic_models[] = {{.id = "claude-opus-4.5",
                                            .display_name = "Claude Opus 4.5",
                                            .provider = PROVIDER_ANTHROPIC,
                                            .input_cost_per_mtok = 15.0,
                                            .output_cost_per_mtok = 75.0,
                                            .thinking_cost_per_mtok = 40.0,
                                            .context_window = 200000,
                                            .max_output = 32000,
                                            .supports_tools = true,
                                            .supports_vision = true,
                                            .supports_streaming = true,
                                            .tier = COST_TIER_PREMIUM,
                                            .released = "2025-11-01",
                                            .deprecated = false},
                                           {.id = "claude-sonnet-4.5",
                                            .display_name = "Claude Sonnet 4.5",
                                            .provider = PROVIDER_ANTHROPIC,
                                            .input_cost_per_mtok = 3.0,
                                            .output_cost_per_mtok = 15.0,
                                            .thinking_cost_per_mtok = 0.0,
                                            .context_window = 1000000,
                                            .max_output = 64000,
                                            .supports_tools = true,
                                            .supports_vision = true,
                                            .supports_streaming = true,
                                            .tier = COST_TIER_MID,
                                            .released = "2025-09-29",
                                            .deprecated = false},
                                           {.id = "claude-haiku-4.5",
                                            .display_name = "Claude Haiku 4.5",
                                            .provider = PROVIDER_ANTHROPIC,
                                            .input_cost_per_mtok = 1.0,
                                            .output_cost_per_mtok = 5.0,
                                            .thinking_cost_per_mtok = 0.0,
                                            .context_window = 200000,
                                            .max_output = 8192,
                                            .supports_tools = true,
                                            .supports_vision = true,
                                            .supports_streaming = true,
                                            .tier = COST_TIER_CHEAP,
                                            .released = "2025-10-01",
                                            .deprecated = false}};
static size_t g_anthropic_model_count = sizeof(g_anthropic_models) / sizeof(g_anthropic_models[0]);

// OpenAI Models (December 2025) - IDs match JSON config
static ModelConfig g_openai_models[] = {{.id = "gpt-5.2-pro",
                                         .display_name = "GPT-5.2 Pro",
                                         .provider = PROVIDER_OPENAI,
                                         .input_cost_per_mtok = 5.0,
                                         .output_cost_per_mtok = 30.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 400000,
                                         .max_output = 128000,
                                         .supports_tools = true,
                                         .supports_vision = true,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_PREMIUM,
                                         .released = "2025-12-11",
                                         .deprecated = false},
                                        {.id = "gpt-5.2",
                                         .display_name = "GPT-5.2 Thinking",
                                         .provider = PROVIDER_OPENAI,
                                         .input_cost_per_mtok = 1.75,
                                         .output_cost_per_mtok = 14.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 400000,
                                         .max_output = 128000,
                                         .supports_tools = true,
                                         .supports_vision = true,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_MID,
                                         .released = "2025-12-11",
                                         .deprecated = false},
                                        {.id = "gpt-5.2-instant",
                                         .display_name = "GPT-5.2 Instant",
                                         .provider = PROVIDER_OPENAI,
                                         .input_cost_per_mtok = 0.50,
                                         .output_cost_per_mtok = 2.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 400000,
                                         .max_output = 128000,
                                         .supports_tools = true,
                                         .supports_vision = true,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_CHEAP,
                                         .released = "2025-12-11",
                                         .deprecated = false},
                                        {.id = "o3",
                                         .display_name = "o3",
                                         .provider = PROVIDER_OPENAI,
                                         .input_cost_per_mtok = 10.0,
                                         .output_cost_per_mtok = 40.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 200000,
                                         .max_output = 100000,
                                         .supports_tools = true,
                                         .supports_vision = true,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_PREMIUM,
                                         .released = "2025-04-16",
                                         .deprecated = false},
                                        {.id = "o3-mini",
                                         .display_name = "o3-mini",
                                         .provider = PROVIDER_OPENAI,
                                         .input_cost_per_mtok = 1.10,
                                         .output_cost_per_mtok = 4.40,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 200000,
                                         .max_output = 100000,
                                         .supports_tools = true,
                                         .supports_vision = false,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_MID,
                                         .released = "2025-01-31",
                                         .deprecated = false},
                                        {.id = "o4-mini",
                                         .display_name = "o4-mini",
                                         .provider = PROVIDER_OPENAI,
                                         .input_cost_per_mtok = 1.10,
                                         .output_cost_per_mtok = 4.40,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 200000,
                                         .max_output = 100000,
                                         .supports_tools = true,
                                         .supports_vision = true,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_MID,
                                         .released = "2025-04-16",
                                         .deprecated = false},
                                        {.id = "gpt-4.1",
                                         .display_name = "GPT-4.1",
                                         .provider = PROVIDER_OPENAI,
                                         .input_cost_per_mtok = 2.0,
                                         .output_cost_per_mtok = 8.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 1000000,
                                         .max_output = 32768,
                                         .supports_tools = true,
                                         .supports_vision = true,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_MID,
                                         .released = "2025-04-14",
                                         .deprecated = false},
                                        {.id = "gpt-4.1-mini",
                                         .display_name = "GPT-4.1 mini",
                                         .provider = PROVIDER_OPENAI,
                                         .input_cost_per_mtok = 0.40,
                                         .output_cost_per_mtok = 1.60,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 1000000,
                                         .max_output = 32768,
                                         .supports_tools = true,
                                         .supports_vision = true,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_CHEAP,
                                         .released = "2025-04-14",
                                         .deprecated = false}};
static size_t g_openai_model_count = sizeof(g_openai_models) / sizeof(g_openai_models[0]);

// Gemini Models (Dec 2025)
static ModelConfig g_gemini_models[] = {{.id = "gemini-3.0-pro",
                                         .display_name = "Gemini 3.0 Pro",
                                         .provider = PROVIDER_GEMINI,
                                         .input_cost_per_mtok = 1.25,
                                         .output_cost_per_mtok = 5.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 2000000,
                                         .max_output = 8192,
                                         .supports_tools = true,
                                         .supports_vision = true,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_MID,
                                         .released = "2025-11-18",
                                         .deprecated = false},
                                        {.id = "gemini-3.0-deep-think",
                                         .display_name = "Gemini 3.0 Deep Think",
                                         .provider = PROVIDER_GEMINI,
                                         .input_cost_per_mtok = 5.0,
                                         .output_cost_per_mtok = 20.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 1000000,
                                         .max_output = 32768,
                                         .supports_tools = true,
                                         .supports_vision = true,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_PREMIUM,
                                         .released = "2025-12-04",
                                         .deprecated = false},
                                        {.id = "gemini-2.0-flash",
                                         .display_name = "Gemini 2.0 Flash",
                                         .provider = PROVIDER_GEMINI,
                                         .input_cost_per_mtok = 0.10,
                                         .output_cost_per_mtok = 0.40,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 1000000,
                                         .max_output = 8192,
                                         .supports_tools = true,
                                         .supports_vision = true,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_CHEAP,
                                         .released = "2025-01-30",
                                         .deprecated = false}};
static size_t g_gemini_model_count = sizeof(g_gemini_models) / sizeof(g_gemini_models[0]);

// OpenRouter Models (access to 300+ models via OpenAI-compatible API)
static ModelConfig g_openrouter_models[] = {{.id = "deepseek/deepseek-r1",
                                             .display_name = "DeepSeek R1",
                                             .provider = PROVIDER_OPENROUTER,
                                             .input_cost_per_mtok = 0.55,
                                             .output_cost_per_mtok = 2.19,
                                             .thinking_cost_per_mtok = 0.0,
                                             .context_window = 64000,
                                             .max_output = 8192,
                                             .supports_tools = true,
                                             .supports_vision = false,
                                             .supports_streaming = true,
                                             .tier = COST_TIER_CHEAP,
                                             .released = "2025-01-01",
                                             .deprecated = false},
                                            {.id = "deepseek/deepseek-chat",
                                             .display_name = "DeepSeek V3",
                                             .provider = PROVIDER_OPENROUTER,
                                             .input_cost_per_mtok = 0.14,
                                             .output_cost_per_mtok = 0.28,
                                             .thinking_cost_per_mtok = 0.0,
                                             .context_window = 64000,
                                             .max_output = 8192,
                                             .supports_tools = true,
                                             .supports_vision = false,
                                             .supports_streaming = true,
                                             .tier = COST_TIER_CHEAP,
                                             .released = "2024-12-01",
                                             .deprecated = false},
                                            {.id = "mistralai/mistral-large-2411",
                                             .display_name = "Mistral Large",
                                             .provider = PROVIDER_OPENROUTER,
                                             .input_cost_per_mtok = 2.0,
                                             .output_cost_per_mtok = 6.0,
                                             .thinking_cost_per_mtok = 0.0,
                                             .context_window = 128000,
                                             .max_output = 8192,
                                             .supports_tools = true,
                                             .supports_vision = false,
                                             .supports_streaming = true,
                                             .tier = COST_TIER_MID,
                                             .released = "2024-11-01",
                                             .deprecated = false},
                                            {.id = "meta-llama/llama-3.3-70b-instruct",
                                             .display_name = "Llama 3.3 70B",
                                             .provider = PROVIDER_OPENROUTER,
                                             .input_cost_per_mtok = 0.40,
                                             .output_cost_per_mtok = 0.40,
                                             .thinking_cost_per_mtok = 0.0,
                                             .context_window = 131072,
                                             .max_output = 8192,
                                             .supports_tools = true,
                                             .supports_vision = false,
                                             .supports_streaming = true,
                                             .tier = COST_TIER_CHEAP,
                                             .released = "2024-12-01",
                                             .deprecated = false},
                                            {.id = "qwen/qwen-2.5-72b-instruct",
                                             .display_name = "Qwen 2.5 72B",
                                             .provider = PROVIDER_OPENROUTER,
                                             .input_cost_per_mtok = 0.35,
                                             .output_cost_per_mtok = 0.40,
                                             .thinking_cost_per_mtok = 0.0,
                                             .context_window = 131072,
                                             .max_output = 8192,
                                             .supports_tools = true,
                                             .supports_vision = false,
                                             .supports_streaming = true,
                                             .tier = COST_TIER_CHEAP,
                                             .released = "2024-09-01",
                                             .deprecated = false},
                                            {.id = "google/gemini-2.0-flash-exp:free",
                                             .display_name = "Gemini 2.0 Flash (Free)",
                                             .provider = PROVIDER_OPENROUTER,
                                             .input_cost_per_mtok = 0.0,
                                             .output_cost_per_mtok = 0.0,
                                             .thinking_cost_per_mtok = 0.0,
                                             .context_window = 1000000,
                                             .max_output = 8192,
                                             .supports_tools = true,
                                             .supports_vision = true,
                                             .supports_streaming = true,
                                             .tier = COST_TIER_CHEAP,
                                             .released = "2024-12-01",
                                             .deprecated = false}};
static size_t g_openrouter_model_count =
    sizeof(g_openrouter_models) / sizeof(g_openrouter_models[0]);

// Ollama Models (local models with zero API costs)
static ModelConfig g_ollama_models[] = {{.id = "llama3.2",
                                         .display_name = "Llama 3.2 (Local)",
                                         .provider = PROVIDER_OLLAMA,
                                         .input_cost_per_mtok = 0.0,
                                         .output_cost_per_mtok = 0.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 131072,
                                         .max_output = 8192,
                                         .supports_tools = false,
                                         .supports_vision = false,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_CHEAP,
                                         .released = "2024-09-01",
                                         .deprecated = false},
                                        {.id = "llama3.2:1b",
                                         .display_name = "Llama 3.2 1B (Local)",
                                         .provider = PROVIDER_OLLAMA,
                                         .input_cost_per_mtok = 0.0,
                                         .output_cost_per_mtok = 0.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 131072,
                                         .max_output = 8192,
                                         .supports_tools = false,
                                         .supports_vision = false,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_CHEAP,
                                         .released = "2024-09-01",
                                         .deprecated = false},
                                        {.id = "mistral",
                                         .display_name = "Mistral 7B (Local)",
                                         .provider = PROVIDER_OLLAMA,
                                         .input_cost_per_mtok = 0.0,
                                         .output_cost_per_mtok = 0.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 32768,
                                         .max_output = 8192,
                                         .supports_tools = false,
                                         .supports_vision = false,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_CHEAP,
                                         .released = "2024-01-01",
                                         .deprecated = false},
                                        {.id = "codellama",
                                         .display_name = "Code Llama (Local)",
                                         .provider = PROVIDER_OLLAMA,
                                         .input_cost_per_mtok = 0.0,
                                         .output_cost_per_mtok = 0.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 16384,
                                         .max_output = 8192,
                                         .supports_tools = false,
                                         .supports_vision = false,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_CHEAP,
                                         .released = "2024-01-01",
                                         .deprecated = false},
                                        {.id = "deepseek-coder-v2",
                                         .display_name = "DeepSeek Coder V2 (Local)",
                                         .provider = PROVIDER_OLLAMA,
                                         .input_cost_per_mtok = 0.0,
                                         .output_cost_per_mtok = 0.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 128000,
                                         .max_output = 8192,
                                         .supports_tools = false,
                                         .supports_vision = false,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_CHEAP,
                                         .released = "2024-06-01",
                                         .deprecated = false},
                                        {.id = "qwen2.5-coder",
                                         .display_name = "Qwen 2.5 Coder (Local)",
                                         .provider = PROVIDER_OLLAMA,
                                         .input_cost_per_mtok = 0.0,
                                         .output_cost_per_mtok = 0.0,
                                         .thinking_cost_per_mtok = 0.0,
                                         .context_window = 131072,
                                         .max_output = 8192,
                                         .supports_tools = false,
                                         .supports_vision = false,
                                         .supports_streaming = true,
                                         .tier = COST_TIER_CHEAP,
                                         .released = "2024-09-01",
                                         .deprecated = false}};
static size_t g_ollama_model_count = sizeof(g_ollama_models) / sizeof(g_ollama_models[0]);

// MLX Models (Apple Silicon native - 100% offline)
static ModelConfig g_mlx_models[] = {{.id = "llama-3.2-1b",
                                      .display_name = "Llama 3.2 1B (MLX)",
                                      .provider = PROVIDER_MLX,
                                      .input_cost_per_mtok = 0.0,
                                      .output_cost_per_mtok = 0.0,
                                      .thinking_cost_per_mtok = 0.0,
                                      .context_window = 131072,
                                      .max_output = 8192,
                                      .supports_tools = true,
                                      .supports_vision = false,
                                      .supports_streaming = true,
                                      .tier = COST_TIER_CHEAP,
                                      .released = "2024-09-01",
                                      .deprecated = false},
                                     {.id = "llama-3.2-3b",
                                      .display_name = "Llama 3.2 3B (MLX)",
                                      .provider = PROVIDER_MLX,
                                      .input_cost_per_mtok = 0.0,
                                      .output_cost_per_mtok = 0.0,
                                      .thinking_cost_per_mtok = 0.0,
                                      .context_window = 131072,
                                      .max_output = 8192,
                                      .supports_tools = true,
                                      .supports_vision = false,
                                      .supports_streaming = true,
                                      .tier = COST_TIER_CHEAP,
                                      .released = "2024-09-01",
                                      .deprecated = false},
                                     {.id = "phi-3-mini",
                                      .display_name = "Phi-3 Mini (MLX)",
                                      .provider = PROVIDER_MLX,
                                      .input_cost_per_mtok = 0.0,
                                      .output_cost_per_mtok = 0.0,
                                      .thinking_cost_per_mtok = 0.0,
                                      .context_window = 128000,
                                      .max_output = 8192,
                                      .supports_tools = true,
                                      .supports_vision = false,
                                      .supports_streaming = true,
                                      .tier = COST_TIER_CHEAP,
                                      .released = "2024-04-01",
                                      .deprecated = false},
                                     {.id = "mistral-7b-q4",
                                      .display_name = "Mistral 7B Q4 (MLX)",
                                      .provider = PROVIDER_MLX,
                                      .input_cost_per_mtok = 0.0,
                                      .output_cost_per_mtok = 0.0,
                                      .thinking_cost_per_mtok = 0.0,
                                      .context_window = 32768,
                                      .max_output = 8192,
                                      .supports_tools = true,
                                      .supports_vision = false,
                                      .supports_streaming = true,
                                      .tier = COST_TIER_CHEAP,
                                      .released = "2024-01-01",
                                      .deprecated = false},
                                     {.id = "llama-3.1-8b-q4",
                                      .display_name = "Llama 3.1 8B Q4 (MLX)",
                                      .provider = PROVIDER_MLX,
                                      .input_cost_per_mtok = 0.0,
                                      .output_cost_per_mtok = 0.0,
                                      .thinking_cost_per_mtok = 0.0,
                                      .context_window = 131072,
                                      .max_output = 8192,
                                      .supports_tools = true,
                                      .supports_vision = false,
                                      .supports_streaming = true,
                                      .tier = COST_TIER_CHEAP,
                                      .released = "2024-07-01",
                                      .deprecated = false},
                                     {.id = "deepseek-r1-1.5b",
                                      .display_name = "DeepSeek R1 Distill 1.5B (MLX)",
                                      .provider = PROVIDER_MLX,
                                      .input_cost_per_mtok = 0.0,
                                      .output_cost_per_mtok = 0.0,
                                      .thinking_cost_per_mtok = 0.0,
                                      .context_window = 64000,
                                      .max_output = 8192,
                                      .supports_tools = true,
                                      .supports_vision = false,
                                      .supports_streaming = true,
                                      .tier = COST_TIER_CHEAP,
                                      .released = "2025-01-01",
                                      .deprecated = false},
                                     {.id = "deepseek-r1-7b",
                                      .display_name = "DeepSeek R1 Distill 7B (MLX)",
                                      .provider = PROVIDER_MLX,
                                      .input_cost_per_mtok = 0.0,
                                      .output_cost_per_mtok = 0.0,
                                      .thinking_cost_per_mtok = 0.0,
                                      .context_window = 64000,
                                      .max_output = 8192,
                                      .supports_tools = true,
                                      .supports_vision = false,
                                      .supports_streaming = true,
                                      .tier = COST_TIER_CHEAP,
                                      .released = "2025-01-01",
                                      .deprecated = false},
                                     {.id = "deepseek-r1-14b",
                                      .display_name = "DeepSeek R1 Distill 14B (MLX)",
                                      .provider = PROVIDER_MLX,
                                      .input_cost_per_mtok = 0.0,
                                      .output_cost_per_mtok = 0.0,
                                      .thinking_cost_per_mtok = 0.0,
                                      .context_window = 64000,
                                      .max_output = 8192,
                                      .supports_tools = true,
                                      .supports_vision = false,
                                      .supports_streaming = true,
                                      .tier = COST_TIER_CHEAP,
                                      .released = "2025-01-01",
                                      .deprecated = false},
                                     {.id = "qwen2.5-coder-7b",
                                      .display_name = "Qwen 2.5 Coder 7B (MLX)",
                                      .provider = PROVIDER_MLX,
                                      .input_cost_per_mtok = 0.0,
                                      .output_cost_per_mtok = 0.0,
                                      .thinking_cost_per_mtok = 0.0,
                                      .context_window = 131072,
                                      .max_output = 8192,
                                      .supports_tools = true,
                                      .supports_vision = false,
                                      .supports_streaming = true,
                                      .tier = COST_TIER_CHEAP,
                                      .released = "2024-11-01",
                                      .deprecated = false}};
static size_t g_mlx_model_count = sizeof(g_mlx_models) / sizeof(g_mlx_models[0]);

// ============================================================================
// PROVIDER NAME MAPPING
// ============================================================================

static const char* g_provider_names[] = {
    [PROVIDER_ANTHROPIC] = "anthropic", [PROVIDER_OPENAI] = "openai",
    [PROVIDER_GEMINI] = "gemini",       [PROVIDER_OPENROUTER] = "openrouter",
    [PROVIDER_OLLAMA] = "ollama",       [PROVIDER_MLX] = "mlx"};

__attribute__((unused)) static const char* g_provider_display_names[] = {
    [PROVIDER_ANTHROPIC] = "Anthropic",   [PROVIDER_OPENAI] = "OpenAI",
    [PROVIDER_GEMINI] = "Google Gemini",  [PROVIDER_OPENROUTER] = "OpenRouter",
    [PROVIDER_OLLAMA] = "Ollama (Local)", [PROVIDER_MLX] = "MLX (Apple Silicon)"};

static const char* g_provider_api_key_envs[] = {
    [PROVIDER_ANTHROPIC] = "ANTHROPIC_API_KEY",
    [PROVIDER_OPENAI] = "OPENAI_API_KEY",
    [PROVIDER_GEMINI] = "GEMINI_API_KEY",
    [PROVIDER_OPENROUTER] = "OPENROUTER_API_KEY",
    [PROVIDER_OLLAMA] = NULL, // No API key needed for local
    [PROVIDER_MLX] = NULL     // No API key needed - 100% local
};

// ============================================================================
// HTTP ERROR CODE MAPPING
// ============================================================================

/**
 * Map HTTP status code to ProviderError
 * This provides consistent error handling across all providers
 */
ProviderError provider_map_http_error(long http_code) {
    if (http_code == 200) {
        return PROVIDER_OK;
    } else if (http_code == 401) {
        return PROVIDER_ERR_AUTH;
    } else if (http_code == 403) {
        return PROVIDER_ERR_AUTH; // Forbidden usually means auth issue
    } else if (http_code == 404) {
        return PROVIDER_ERR_MODEL_NOT_FOUND;
    } else if (http_code == 413) {
        return PROVIDER_ERR_CONTEXT_LENGTH; // Payload too large
    } else if (http_code == 429) {
        return PROVIDER_ERR_RATE_LIMIT;
    } else if (http_code == 500) {
        return PROVIDER_ERR_OVERLOADED;
    } else if (http_code == 502 || http_code == 503 || http_code == 504) {
        return PROVIDER_ERR_OVERLOADED; // Bad gateway, service unavailable, gateway timeout
    } else {
        return PROVIDER_ERR_UNKNOWN;
    }
}

// ============================================================================
// ERROR MESSAGES
// ============================================================================

static const char* g_error_messages[] = {
    [PROVIDER_OK] = "Success",
    [PROVIDER_ERR_AUTH] = "API key invalid or expired. Run 'convergio setup' to reconfigure.",
    [PROVIDER_ERR_RATE_LIMIT] = "Rate limit exceeded. Retrying automatically...",
    [PROVIDER_ERR_QUOTA] = "API quota exceeded. Check your provider dashboard.",
    [PROVIDER_ERR_CONTEXT_LENGTH] =
        "Input too long for this model. Consider using a model with larger context.",
    [PROVIDER_ERR_CONTENT_FILTER] = "Content was filtered by the provider's safety system.",
    [PROVIDER_ERR_MODEL_NOT_FOUND] =
        "Model not found. Run 'convergio models' to see available models.",
    [PROVIDER_ERR_OVERLOADED] = "Provider service is overloaded. Retrying...",
    [PROVIDER_ERR_TIMEOUT] = "Request timed out. Please try again.",
    [PROVIDER_ERR_NETWORK] = "Network error. Check your internet connection.",
    [PROVIDER_ERR_INVALID_REQUEST] = "Invalid request. This may be a bug - please report it.",
    [PROVIDER_ERR_NOT_INITIALIZED] =
        "Provider not initialized. Call provider_registry_init() first.",
    [PROVIDER_ERR_UNKNOWN] = "An unexpected error occurred."};

// ============================================================================
// PROVIDER REGISTRY IMPLEMENTATION
// ============================================================================

// Forward declarations for provider adapters
extern Provider* anthropic_provider_create(void);
extern Provider* openai_provider_create(void);
extern Provider* gemini_provider_create(void);
extern Provider* openrouter_provider_create(void);
extern Provider* ollama_provider_create(void);
extern Provider* mlx_provider_create(void);

ProviderError provider_registry_init(void) {
    pthread_mutex_lock(&g_registry_mutex);

    if (g_registry_initialized) {
        pthread_mutex_unlock(&g_registry_mutex);
        return PROVIDER_OK;
    }

    LOG_INFO(LOG_CAT_SYSTEM, "Initializing provider registry...");

    // Create provider instances (they don't need to be fully initialized yet)
    // Full initialization happens when the provider is first used and has valid API key

    // Anthropic provider
    g_providers[PROVIDER_ANTHROPIC] = anthropic_provider_create();
    if (g_providers[PROVIDER_ANTHROPIC]) {
        LOG_DEBUG(LOG_CAT_SYSTEM, "Anthropic provider created");
    }

    // OpenAI provider
    g_providers[PROVIDER_OPENAI] = openai_provider_create();
    if (g_providers[PROVIDER_OPENAI]) {
        LOG_DEBUG(LOG_CAT_SYSTEM, "OpenAI provider created");
    }

    // Gemini provider
    g_providers[PROVIDER_GEMINI] = gemini_provider_create();
    if (g_providers[PROVIDER_GEMINI]) {
        LOG_DEBUG(LOG_CAT_SYSTEM, "Gemini provider created");
    }

    // OpenRouter provider (access to 300+ models)
    g_providers[PROVIDER_OPENROUTER] = openrouter_provider_create();
    if (g_providers[PROVIDER_OPENROUTER]) {
        LOG_DEBUG(LOG_CAT_SYSTEM, "OpenRouter provider created");
    }

    // Ollama provider (local models)
    g_providers[PROVIDER_OLLAMA] = ollama_provider_create();
    if (g_providers[PROVIDER_OLLAMA]) {
        LOG_DEBUG(LOG_CAT_SYSTEM, "Ollama provider created");
    }

    // MLX provider (Apple Silicon native - 100% offline)
    g_providers[PROVIDER_MLX] = mlx_provider_create();
    if (g_providers[PROVIDER_MLX]) {
        LOG_DEBUG(LOG_CAT_SYSTEM, "MLX provider created (Apple Silicon)");
    }

    g_registry_initialized = true;
    pthread_mutex_unlock(&g_registry_mutex);

    LOG_INFO(LOG_CAT_SYSTEM, "Provider registry initialized");
    return PROVIDER_OK;
}

void provider_registry_shutdown(void) {
    pthread_mutex_lock(&g_registry_mutex);

    if (!g_registry_initialized) {
        pthread_mutex_unlock(&g_registry_mutex);
        return;
    }

    LOG_INFO(LOG_CAT_SYSTEM, "Shutting down provider registry...");

    for (int i = 0; i < PROVIDER_COUNT; i++) {
        if (g_providers[i]) {
            if (g_providers[i]->shutdown) {
                g_providers[i]->shutdown(g_providers[i]);
            }
            free(g_providers[i]);
            g_providers[i] = NULL;
        }
    }

    g_registry_initialized = false;
    pthread_mutex_unlock(&g_registry_mutex);

    LOG_INFO(LOG_CAT_SYSTEM, "Provider registry shutdown complete");
}

Provider* provider_get(ProviderType type) {
    if (type < 0 || type >= PROVIDER_COUNT) {
        return NULL;
    }

    pthread_mutex_lock(&g_registry_mutex);
    Provider* provider = g_providers[type];
    pthread_mutex_unlock(&g_registry_mutex);

    return provider;
}

bool provider_is_available(ProviderType type) {
    Provider* provider = provider_get(type);
    if (!provider) {
        return false;
    }

    // Check if API key is set
    const char* env_var = g_provider_api_key_envs[type];
    if (env_var) {
        const char* api_key = getenv(env_var);
        if (!api_key || strlen(api_key) == 0) {
            return false;
        }
    }

    // Validate key if provider is initialized
    if (provider->initialized && provider->validate_key) {
        return provider->validate_key(provider);
    }

    return true;
}

const char* provider_name(ProviderType type) {
    if (type < 0 || type >= PROVIDER_COUNT) {
        return "unknown";
    }
    return g_provider_names[type];
}

// ============================================================================
// MODEL REGISTRY IMPLEMENTATION
// ============================================================================

static ModelConfig* find_model_in_array(const char* model_id, ModelConfig* models, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (strcmp(models[i].id, model_id) == 0) {
            return &models[i];
        }
    }
    return NULL;
}

// Convert provider name string to ProviderType
static ProviderType provider_name_to_type(const char* name) {
    if (!name)
        return PROVIDER_COUNT;
    if (strcmp(name, "anthropic") == 0)
        return PROVIDER_ANTHROPIC;
    if (strcmp(name, "openai") == 0)
        return PROVIDER_OPENAI;
    if (strcmp(name, "gemini") == 0)
        return PROVIDER_GEMINI;
    if (strcmp(name, "openrouter") == 0)
        return PROVIDER_OPENROUTER;
    if (strcmp(name, "ollama") == 0)
        return PROVIDER_OLLAMA;
    return PROVIDER_COUNT;
}

// Convert tier string to CostTier
static CostTier tier_string_to_enum(const char* tier) {
    if (!tier)
        return COST_TIER_MID;
    if (strcmp(tier, "premium") == 0)
        return COST_TIER_PREMIUM;
    if (strcmp(tier, "mid") == 0)
        return COST_TIER_MID;
    if (strcmp(tier, "cheap") == 0)
        return COST_TIER_CHEAP;
    return COST_TIER_MID;
}

// Static storage for JSON-loaded model (converted to ModelConfig)
// We use a small cache to avoid repeated conversions
#define JSON_MODEL_CACHE_SIZE 8
static ModelConfig g_json_model_cache[JSON_MODEL_CACHE_SIZE];
static size_t g_json_cache_index = 0;

// Convert JsonModelConfig to ModelConfig (returns pointer to static storage)
static const ModelConfig* json_to_model_config(const JsonModelConfig* json,
                                               const char* provider_name) {
    if (!json)
        return NULL;

    // Use circular buffer for cache
    ModelConfig* cfg = &g_json_model_cache[g_json_cache_index];
    g_json_cache_index = (g_json_cache_index + 1) % JSON_MODEL_CACHE_SIZE;

    // Copy data (note: id and display_name point to JSON loader's memory)
    cfg->id = json->id;
    cfg->display_name = json->display_name;
    cfg->provider = provider_name_to_type(provider_name);
    cfg->input_cost_per_mtok = json->input_cost;
    cfg->output_cost_per_mtok = json->output_cost;
    cfg->thinking_cost_per_mtok = json->thinking_cost;
    cfg->context_window = json->context_window;
    cfg->max_output = json->max_output;
    cfg->supports_tools = json->supports_tools;
    cfg->supports_vision = json->supports_vision;
    cfg->supports_streaming = json->supports_streaming;
    cfg->tier = tier_string_to_enum(json->tier);
    cfg->released = json->released;
    cfg->deprecated = json->deprecated;

    return cfg;
}

const ModelConfig* model_get_config(const char* model_id) {
    if (!model_id)
        return NULL;

    // Handle prefixed model IDs (e.g., "anthropic/claude-opus-4")
    const char* slash = strchr(model_id, '/');
    const char* actual_id = slash ? slash + 1 : model_id;

    // FIRST: Try JSON loader (single source of truth)
    const JsonModelConfig* json_model = models_get_json_model(actual_id);
    if (json_model) {
        const char* provider_name = models_get_model_provider(actual_id);
        return json_to_model_config(json_model, provider_name);
    }

    // FALLBACK: Search in hardcoded arrays (for when JSON is not available)
    // Determine provider from prefix if present
    ProviderType hint = PROVIDER_COUNT; // Invalid, search all
    if (slash) {
        size_t prefix_len = (size_t)(slash - model_id);
        for (int i = 0; i < PROVIDER_COUNT; i++) {
            if (strncmp(model_id, g_provider_names[i], prefix_len) == 0 &&
                strlen(g_provider_names[i]) == prefix_len) {
                hint = (ProviderType)i;
                break;
            }
        }
    }

    // Search in appropriate provider(s)
    ModelConfig* result = NULL;

    if (hint == PROVIDER_ANTHROPIC || hint == PROVIDER_COUNT) {
        result = find_model_in_array(actual_id, g_anthropic_models, g_anthropic_model_count);
        if (result)
            return result;
    }

    if (hint == PROVIDER_OPENAI || hint == PROVIDER_COUNT) {
        result = find_model_in_array(actual_id, g_openai_models, g_openai_model_count);
        if (result)
            return result;
    }

    if (hint == PROVIDER_GEMINI || hint == PROVIDER_COUNT) {
        result = find_model_in_array(actual_id, g_gemini_models, g_gemini_model_count);
        if (result)
            return result;
    }

    if (hint == PROVIDER_OPENROUTER || hint == PROVIDER_COUNT) {
        // For OpenRouter, search with full model_id (includes provider prefix like
        // "deepseek/deepseek-r1")
        result = find_model_in_array(model_id, g_openrouter_models, g_openrouter_model_count);
        if (result)
            return result;
        // Also try with actual_id in case user stripped the openrouter/ prefix
        result = find_model_in_array(actual_id, g_openrouter_models, g_openrouter_model_count);
        if (result)
            return result;
    }

    if (hint == PROVIDER_OLLAMA || hint == PROVIDER_COUNT) {
        result = find_model_in_array(actual_id, g_ollama_models, g_ollama_model_count);
        if (result)
            return result;
    }

    if (hint == PROVIDER_MLX || hint == PROVIDER_COUNT) {
        result = find_model_in_array(actual_id, g_mlx_models, g_mlx_model_count);
        if (result)
            return result;
    }

    return NULL;
}

const ModelConfig* model_get_by_provider(ProviderType type, size_t* out_count) {
    switch (type) {
    case PROVIDER_ANTHROPIC:
        if (out_count)
            *out_count = g_anthropic_model_count;
        return g_anthropic_models;
    case PROVIDER_OPENAI:
        if (out_count)
            *out_count = g_openai_model_count;
        return g_openai_models;
    case PROVIDER_GEMINI:
        if (out_count)
            *out_count = g_gemini_model_count;
        return g_gemini_models;
    case PROVIDER_OPENROUTER:
        if (out_count)
            *out_count = g_openrouter_model_count;
        return g_openrouter_models;
    case PROVIDER_OLLAMA:
        if (out_count)
            *out_count = g_ollama_model_count;
        return g_ollama_models;
    case PROVIDER_MLX:
        if (out_count)
            *out_count = g_mlx_model_count;
        return g_mlx_models;
    default:
        if (out_count)
            *out_count = 0;
        return NULL;
    }
}

const ModelConfig* model_get_by_tier(CostTier tier, size_t* out_count) {
    // This would require building a dynamic array - for now return NULL
    // In production, this should be implemented properly
    if (out_count)
        *out_count = 0;
    return NULL;
}

const ModelConfig* model_get_cheapest(ProviderType type) {
    size_t count;
    const ModelConfig* models = model_get_by_provider(type, &count);
    if (!models || count == 0)
        return NULL;

    const ModelConfig* cheapest = &models[0];
    double min_cost = cheapest->input_cost_per_mtok + cheapest->output_cost_per_mtok;

    for (size_t i = 1; i < count; i++) {
        double cost = models[i].input_cost_per_mtok + models[i].output_cost_per_mtok;
        if (cost < min_cost && !models[i].deprecated) {
            cheapest = &models[i];
            min_cost = cost;
        }
    }

    return cheapest;
}

double model_estimate_cost(const char* model_id, size_t input_tokens, size_t output_tokens) {
    const ModelConfig* model = model_get_config(model_id);
    if (!model)
        return 0.0;

    double input_cost = (double)input_tokens / 1000000.0 * model->input_cost_per_mtok;
    double output_cost = (double)output_tokens / 1000000.0 * model->output_cost_per_mtok;

    return input_cost + output_cost;
}

// ============================================================================
// ERROR HANDLING UTILITIES
// ============================================================================

const char* provider_error_message(ProviderError code) {
    if (code < 0 || code > PROVIDER_ERR_UNKNOWN) {
        return g_error_messages[PROVIDER_ERR_UNKNOWN];
    }
    return g_error_messages[code];
}

bool provider_error_is_retryable(ProviderError code) {
    switch (code) {
    case PROVIDER_ERR_RATE_LIMIT:
    case PROVIDER_ERR_OVERLOADED:
    case PROVIDER_ERR_TIMEOUT:
    case PROVIDER_ERR_NETWORK:
        return true;
    default:
        return false;
    }
}

void provider_error_free(ProviderErrorInfo* info) {
    if (!info)
        return;
    free(info->message);
    free(info->provider_code);
    free(info);
}

// ============================================================================
// RETRY CONFIGURATION
// ============================================================================

RetryConfig retry_config_default(void) {
    return (RetryConfig){.max_retries = 3,
                         .base_delay_ms = 1000,
                         .max_delay_ms = 60000,
                         .jitter_factor = 0.2,
                         .retry_on_rate_limit = true,
                         .retry_on_server_error = true};
}

int retry_calculate_delay(const RetryConfig* cfg, int attempt) {
    if (!cfg || attempt < 0)
        return 1000;

    // Exponential backoff: base * 2^attempt
    int delay = cfg->base_delay_ms * (1 << attempt);
    if (delay > cfg->max_delay_ms) {
        delay = cfg->max_delay_ms;
    }

    // Add jitter to prevent thundering herd
    // Random value between -jitter/2 and +jitter/2
    double jitter_range = delay * cfg->jitter_factor;
    int jitter = (int)((((double)rand() / RAND_MAX) * jitter_range) - (jitter_range / 2));
    delay += jitter;

    if (delay < 0)
        delay = cfg->base_delay_ms;

    return delay;
}
