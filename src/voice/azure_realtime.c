/**
 * CONVERGIO EDUCATION - AZURE OPENAI REALTIME API CLIENT
 *
 * WebSocket client for Azure OpenAI GPT-4o Realtime API.
 * Uses existing Azure infrastructure from VirtualBPM project.
 *
 * Documentation: https://learn.microsoft.com/en-us/azure/ai-foundry/openai/how-to/realtime-audio-websockets
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/voice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ============================================================================
// AZURE OPENAI REALTIME API CONSTANTS
// ============================================================================

// WebSocket URL formats
// GA: wss://{resource}.openai.azure.com/openai/v1/realtime?model={deployment}
// Preview: wss://{resource}.openai.azure.com/openai/realtime?api-version=2025-04-01-preview&deployment={deployment}

#define AZURE_REALTIME_PATH_GA "/openai/v1/realtime"
#define AZURE_REALTIME_PATH_PREVIEW "/openai/realtime"
#define AZURE_REALTIME_API_VERSION "2025-04-01-preview"

// Recommended models for realtime voice
#define AZURE_REALTIME_MODEL_PREVIEW "gpt-4o-realtime-preview"
#define AZURE_REALTIME_MODEL_MINI "gpt-4o-mini-realtime-preview"
#define AZURE_REALTIME_MODEL_GA "gpt-realtime"
#define AZURE_REALTIME_MODEL_GA_MINI "gpt-realtime-mini"

// Available regions for Realtime API
static const char* AZURE_REALTIME_REGIONS[] = {
    "eastus2",
    "swedencentral"
};
static const size_t AZURE_REALTIME_REGIONS_COUNT = 2;

// Audio settings (same as OpenAI)
#define AZURE_AUDIO_FORMAT "pcm16"
#define AZURE_SAMPLE_RATE 24000

// ============================================================================
// AZURE CONFIGURATION
// ============================================================================

typedef struct {
    char endpoint[256];          // e.g., "your-resource-name.openai.azure.com"
    char api_key[256];
    char deployment_name[64];    // e.g., "gpt-4o-realtime-deployment"
    char api_version[32];
    bool use_preview_api;        // true = preview, false = GA
} AzureRealtimeConfig;

/**
 * Load Azure configuration from environment
 * First tries AZURE_OPENAI_REALTIME_* variables, then falls back to standard AZURE_OPENAI_*
 */
bool azure_realtime_load_config(AzureRealtimeConfig* config) {
    if (!config) return false;

    memset(config, 0, sizeof(AzureRealtimeConfig));

    // Realtime-specific endpoint (recommended: create separate resource in eastus2)
    const char* endpoint = getenv("AZURE_OPENAI_REALTIME_ENDPOINT");
    if (!endpoint) endpoint = getenv("AZURE_OPENAI_ENDPOINT");
    if (!endpoint) {
        fprintf(stderr, "[Azure Realtime] No endpoint configured. Set AZURE_OPENAI_REALTIME_ENDPOINT\n");
        return false;
    }

    // Extract hostname from URL
    const char* host_start = strstr(endpoint, "://");
    if (host_start) {
        host_start += 3;  // Skip "://"
    } else {
        host_start = endpoint;
    }

    // Remove trailing slash
    strncpy(config->endpoint, host_start, sizeof(config->endpoint) - 1);
    size_t len = strlen(config->endpoint);
    if (len > 0 && config->endpoint[len - 1] == '/') {
        config->endpoint[len - 1] = '\0';
    }

    // API Key
    const char* api_key = getenv("AZURE_OPENAI_REALTIME_API_KEY");
    if (!api_key) api_key = getenv("AZURE_OPENAI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "[Azure Realtime] No API key configured. Set AZURE_OPENAI_REALTIME_API_KEY\n");
        return false;
    }
    strncpy(config->api_key, api_key, sizeof(config->api_key) - 1);

    // Deployment name (MUST be a realtime model deployment)
    const char* deployment = getenv("AZURE_OPENAI_REALTIME_DEPLOYMENT");
    if (!deployment) {
        // Default deployment name suggestion
        deployment = "gpt-4o-realtime";
        fprintf(stderr, "[Azure Realtime] Using default deployment name: %s\n", deployment);
        fprintf(stderr, "[Azure Realtime] Create this deployment in Azure Portal with model: %s\n",
                AZURE_REALTIME_MODEL_PREVIEW);
    }
    strncpy(config->deployment_name, deployment, sizeof(config->deployment_name) - 1);

    // API Version
    const char* version = getenv("AZURE_OPENAI_REALTIME_API_VERSION");
    if (!version) version = AZURE_REALTIME_API_VERSION;
    strncpy(config->api_version, version, sizeof(config->api_version) - 1);

    // Check if using preview or GA API
    config->use_preview_api = strstr(version, "preview") != NULL;

    return true;
}

/**
 * Build WebSocket URL for Azure OpenAI Realtime
 */
void azure_realtime_build_ws_url(const AzureRealtimeConfig* config, char* buffer, size_t size) {
    if (!config || !buffer || size == 0) return;

    if (config->use_preview_api) {
        // Preview API format
        snprintf(buffer, size,
            "wss://%s%s?api-version=%s&deployment=%s",
            config->endpoint,
            AZURE_REALTIME_PATH_PREVIEW,
            config->api_version,
            config->deployment_name);
    } else {
        // GA API format
        snprintf(buffer, size,
            "wss://%s%s?model=%s",
            config->endpoint,
            AZURE_REALTIME_PATH_GA,
            config->deployment_name);
    }
}

/**
 * Build WebSocket URL with API key in query string (for environments without header support)
 */
void azure_realtime_build_ws_url_with_key(const AzureRealtimeConfig* config, char* buffer, size_t size) {
    if (!config || !buffer || size == 0) return;

    if (config->use_preview_api) {
        snprintf(buffer, size,
            "wss://%s%s?api-version=%s&deployment=%s&api-key=%s",
            config->endpoint,
            AZURE_REALTIME_PATH_PREVIEW,
            config->api_version,
            config->deployment_name,
            config->api_key);
    } else {
        snprintf(buffer, size,
            "wss://%s%s?model=%s&api-key=%s",
            config->endpoint,
            AZURE_REALTIME_PATH_GA,
            config->deployment_name,
            config->api_key);
    }
}

// ============================================================================
// AZURE REALTIME SESSION
// ============================================================================

typedef struct AzureRealtimeSession {
    AzureRealtimeConfig config;
    bool connected;
    char current_maestro[64];

    // Audio buffers
    uint8_t* input_buffer;
    size_t input_buffer_size;
    size_t input_buffer_pos;

    // Callbacks
    void (*on_audio)(const uint8_t* data, size_t length, void* user_data);
    void (*on_transcript)(const char* text, bool is_final, void* user_data);
    void (*on_error)(const char* message, void* user_data);
    void* callback_user_data;

} AzureRealtimeSession;

/**
 * Create Azure Realtime session
 */
AzureRealtimeSession* azure_realtime_create(void) {
    AzureRealtimeSession* session = calloc(1, sizeof(AzureRealtimeSession));
    if (!session) return NULL;

    if (!azure_realtime_load_config(&session->config)) {
        free(session);
        return NULL;
    }

    session->input_buffer_size = 1024 * 1024;
    session->input_buffer = malloc(session->input_buffer_size);
    if (!session->input_buffer) {
        free(session);
        return NULL;
    }

    strlcpy(session->current_maestro, "euclide-matematica", sizeof(session->current_maestro));

    return session;
}

/**
 * Destroy Azure Realtime session
 */
void azure_realtime_destroy(AzureRealtimeSession* session) {
    if (!session) return;
    free(session->input_buffer);
    free(session);
}

/**
 * Connect to Azure OpenAI Realtime API
 */
bool azure_realtime_connect(AzureRealtimeSession* session, const char* maestro_prompt) {
    if (!session) return false;

    char ws_url[1024];
    azure_realtime_build_ws_url(&session->config, ws_url, sizeof(ws_url));

    fprintf(stderr, "[Azure Realtime] Connecting to: %s\n", ws_url);
    fprintf(stderr, "[Azure Realtime] Deployment: %s\n", session->config.deployment_name);
    fprintf(stderr, "[Azure Realtime] API Version: %s\n", session->config.api_version);

    // Connection headers needed:
    // api-key: <your-api-key>
    // OR
    // Authorization: Bearer <token> (for Entra ID)

    if (maestro_prompt) {
        fprintf(stderr, "[Azure Realtime] Maestro prompt length: %zu\n", strlen(maestro_prompt));
    }

    // In production: establish WebSocket connection using libwebsockets or similar
    // The session.update message format is the same as OpenAI Realtime API

    session->connected = true;
    return true;
}

/**
 * Disconnect from Azure Realtime API
 */
void azure_realtime_disconnect(AzureRealtimeSession* session) {
    if (!session) return;
    session->connected = false;
}

/**
 * Set maestro for voice
 */
void azure_realtime_set_maestro(AzureRealtimeSession* session, const char* maestro_id) {
    if (!session || !maestro_id) return;
    strncpy(session->current_maestro, maestro_id, sizeof(session->current_maestro) - 1);
}

/**
 * Send audio to Azure
 */
bool azure_realtime_send_audio(AzureRealtimeSession* session,
                                const uint8_t* audio, size_t length) {
    if (!session || !session->connected || !audio || length == 0) return false;

    size_t available = session->input_buffer_size - session->input_buffer_pos;
    size_t to_copy = length < available ? length : available;

    if (to_copy > 0) {
        memcpy(session->input_buffer + session->input_buffer_pos, audio, to_copy);
        session->input_buffer_pos += to_copy;
    }

    return true;
}

/**
 * Commit audio (finalize user turn)
 */
bool azure_realtime_commit_audio(AzureRealtimeSession* session) {
    if (!session || !session->connected) return false;

    fprintf(stderr, "[Azure Realtime] Committing %zu bytes of audio\n", session->input_buffer_pos);
    session->input_buffer_pos = 0;

    return true;
}

/**
 * Cancel current response
 */
void azure_realtime_cancel(AzureRealtimeSession* session) {
    if (!session || !session->connected) return;
    fprintf(stderr, "[Azure Realtime] Response cancelled (barge-in)\n");
}

// ============================================================================
// DEPLOYMENT HELPER
// ============================================================================

/**
 * Print instructions for creating Azure Realtime deployment
 */
void azure_realtime_print_setup_instructions(void) {
    fprintf(stderr, "\n");
    fprintf(stderr, "╔═══════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║         AZURE OPENAI REALTIME SETUP INSTRUCTIONS                   ║\n");
    fprintf(stderr, "╠═══════════════════════════════════════════════════════════════════╣\n");
    fprintf(stderr, "║                                                                    ║\n");
    fprintf(stderr, "║  1. Go to Azure Portal → Azure OpenAI                             ║\n");
    fprintf(stderr, "║                                                                    ║\n");
    fprintf(stderr, "║  2. Create or use resource in SUPPORTED REGION:                   ║\n");
    fprintf(stderr, "║     - East US 2                                                   ║\n");
    fprintf(stderr, "║     - Sweden Central                                              ║\n");
    fprintf(stderr, "║                                                                    ║\n");
    fprintf(stderr, "║  3. Create Deployment with:                                       ║\n");
    fprintf(stderr, "║     - Model: gpt-4o-realtime-preview (2024-12-17)                ║\n");
    fprintf(stderr, "║     - Deployment name: gpt-4o-realtime                           ║\n");
    fprintf(stderr, "║                                                                    ║\n");
    fprintf(stderr, "║  4. Set environment variables:                                    ║\n");
    fprintf(stderr, "║                                                                    ║\n");
    fprintf(stderr, "║     export AZURE_OPENAI_REALTIME_ENDPOINT=\\                       ║\n");
    fprintf(stderr, "║       \"https://your-resource.openai.azure.com/\"                  ║\n");
    fprintf(stderr, "║                                                                    ║\n");
    fprintf(stderr, "║     export AZURE_OPENAI_REALTIME_API_KEY=\"your-key\"              ║\n");
    fprintf(stderr, "║                                                                    ║\n");
    fprintf(stderr, "║     export AZURE_OPENAI_REALTIME_DEPLOYMENT=\"gpt-4o-realtime\"    ║\n");
    fprintf(stderr, "║                                                                    ║\n");
    fprintf(stderr, "║  5. Optional: Use existing VirtualBPM subscription:              ║\n");
    fprintf(stderr, "║     Subscription ID: 8015083b-adad-42ff-922d-feaed61c5d62        ║\n");
    fprintf(stderr, "║     (Create new resource in eastus2 region)                      ║\n");
    fprintf(stderr, "║                                                                    ║\n");
    fprintf(stderr, "╚═══════════════════════════════════════════════════════════════════╝\n");
    fprintf(stderr, "\n");
}

/**
 * Check if Azure Realtime is properly configured
 */
bool azure_realtime_is_configured(void) {
    const char* endpoint = getenv("AZURE_OPENAI_REALTIME_ENDPOINT");
    const char* api_key = getenv("AZURE_OPENAI_REALTIME_API_KEY");

    // Also check standard Azure OpenAI variables
    if (!endpoint) endpoint = getenv("AZURE_OPENAI_ENDPOINT");
    if (!api_key) api_key = getenv("AZURE_OPENAI_API_KEY");

    return (endpoint != NULL && api_key != NULL);
}

/**
 * Check if the endpoint is in a supported region
 */
bool azure_realtime_check_region(const char* endpoint) {
    if (!endpoint) return false;

    for (size_t i = 0; i < AZURE_REALTIME_REGIONS_COUNT; i++) {
        if (strstr(endpoint, AZURE_REALTIME_REGIONS[i]) != NULL) {
            return true;
        }
    }

    fprintf(stderr, "[Azure Realtime] WARNING: Endpoint may not be in a supported region.\n");
    fprintf(stderr, "[Azure Realtime] Supported regions: eastus2, swedencentral\n");
    return false;
}

// ============================================================================
// INTEGRATION WITH VOICE GATEWAY
// ============================================================================

/**
 * Initialize Azure OpenAI Realtime
 */
bool azure_realtime_init(void) {
    if (!azure_realtime_is_configured()) {
        fprintf(stderr, "[Azure Realtime] Not configured.\n");
        azure_realtime_print_setup_instructions();
        return false;
    }

    AzureRealtimeConfig config;
    if (!azure_realtime_load_config(&config)) {
        return false;
    }

    // Check region
    azure_realtime_check_region(config.endpoint);

    char ws_url[1024];
    azure_realtime_build_ws_url(&config, ws_url, sizeof(ws_url));

    fprintf(stderr, "[Azure Realtime] Initialized\n");
    fprintf(stderr, "[Azure Realtime] Endpoint: %s\n", config.endpoint);
    fprintf(stderr, "[Azure Realtime] Deployment: %s\n", config.deployment_name);
    fprintf(stderr, "[Azure Realtime] WebSocket URL: %s\n", ws_url);

    return true;
}

// ============================================================================
// AZURE CLI HELPER (Optional: Create deployment via CLI)
// ============================================================================

/**
 * Generate Azure CLI command to create realtime deployment
 */
void azure_realtime_generate_cli_command(const char* resource_group,
                                          const char* resource_name,
                                          char* buffer, size_t size) {
    if (!buffer || size == 0) return;

    snprintf(buffer, size,
        "# Create Azure OpenAI Realtime deployment\n"
        "az cognitiveservices account deployment create \\\n"
        "  --resource-group \"%s\" \\\n"
        "  --name \"%s\" \\\n"
        "  --deployment-name \"gpt-4o-realtime\" \\\n"
        "  --model-name \"gpt-4o-realtime-preview\" \\\n"
        "  --model-version \"2024-12-17\" \\\n"
        "  --model-format OpenAI \\\n"
        "  --sku-capacity 1 \\\n"
        "  --sku-name \"GlobalStandard\"\n",
        resource_group ? resource_group : "rg-convergio-education",
        resource_name ? resource_name : "aoai-convergio-eastus2");
}
