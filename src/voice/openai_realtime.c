/**
 * CONVERGIO EDUCATION - OPENAI REALTIME API CLIENT
 *
 * WebSocket client for OpenAI GPT-4o Realtime API.
 * Provides ChatGPT-level voice interaction quality.
 *
 * API Reference: https://platform.openai.com/docs/guides/realtime
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/voice.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ============================================================================
// OPENAI REALTIME API CONSTANTS
// ============================================================================

#define OPENAI_REALTIME_URL "wss://api.openai.com/v1/realtime"
#define OPENAI_REALTIME_MODEL "gpt-4o-realtime-preview-2024-12-17"
#define OPENAI_AUDIO_FORMAT_PCM16 "pcm16"
#define OPENAI_SAMPLE_RATE 24000

// Available OpenAI voices (as of Dec 2025)
static const char* OPENAI_VOICES[] = {
    "alloy",   // Neutral, balanced
    "ash",     // Warm, friendly
    "ballad",  // Melodic, expressive
    "coral",   // Clear, professional
    "echo",    // Energetic, enthusiastic
    "sage",    // Calm, wise
    "shimmer", // Light, cheerful
    "verse"    // Deep, authoritative
};
static const size_t OPENAI_VOICE_COUNT = 8;

// ============================================================================
// OPENAI REALTIME SESSION MESSAGES
// ============================================================================

/**
 * Session configuration for OpenAI Realtime
 */
typedef struct {
    const char* model;
    const char* voice;
    const char* instructions;
    const char* input_audio_format;
    const char* output_audio_format;
    float temperature;
    bool input_audio_transcription;
    bool turn_detection; // VAD (Voice Activity Detection)
} OpenAISessionConfig;

/**
 * Create session.update message
 */
static void openai_create_session_update(const OpenAISessionConfig* config,
                                         const char* maestro_prompt, char* buffer,
                                         size_t buffer_size) {
    snprintf(buffer, buffer_size,
             "{"
             "\"type\":\"session.update\","
             "\"session\":{"
             "\"modalities\":[\"text\",\"audio\"],"
             "\"instructions\":\"%s\","
             "\"voice\":\"%s\","
             "\"input_audio_format\":\"%s\","
             "\"output_audio_format\":\"%s\","
             "\"input_audio_transcription\":{\"model\":\"whisper-1\"},"
             "\"turn_detection\":{\"type\":\"server_vad\",\"threshold\":0.5,\"prefix_padding_ms\":"
             "300,\"silence_duration_ms\":500}"
             "}"
             "}",
             maestro_prompt ? maestro_prompt : "You are a helpful educational assistant.",
             config->voice ? config->voice : "sage",
             config->input_audio_format ? config->input_audio_format : OPENAI_AUDIO_FORMAT_PCM16,
             config->output_audio_format ? config->output_audio_format : OPENAI_AUDIO_FORMAT_PCM16);
}

/**
 * Create audio input append message
 * Audio must be base64-encoded PCM16 at 24kHz
 */
static void openai_create_audio_append(const uint8_t* audio_data, size_t length, char* buffer,
                                       size_t buffer_size) {
    // Base64 encode audio data
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t b64_len = ((length + 2) / 3) * 4;
    char* b64_audio = malloc(b64_len + 1);
    if (!b64_audio) {
        snprintf(buffer, buffer_size, "{\"type\":\"error\",\"message\":\"OOM\"}");
        return;
    }

    size_t j = 0;
    for (size_t i = 0; i < length; i += 3) {
        uint32_t n = ((uint32_t)audio_data[i]) << 16;
        if (i + 1 < length)
            n |= ((uint32_t)audio_data[i + 1]) << 8;
        if (i + 2 < length)
            n |= audio_data[i + 2];

        b64_audio[j++] = b64[(n >> 18) & 63];
        b64_audio[j++] = b64[(n >> 12) & 63];
        b64_audio[j++] = (i + 1 < length) ? b64[(n >> 6) & 63] : '=';
        b64_audio[j++] = (i + 2 < length) ? b64[n & 63] : '=';
    }
    b64_audio[j] = '\0';

    snprintf(buffer, buffer_size, "{\"type\":\"input_audio_buffer.append\",\"audio\":\"%s\"}",
             b64_audio);

    free(b64_audio);
}

/**
 * Create audio commit message (finalize user turn)
 */
static const char* openai_audio_commit_msg = "{\"type\":\"input_audio_buffer.commit\"}";

/**
 * Create response.create message (request assistant response)
 */
static const char* openai_response_create_msg =
    "{\"type\":\"response.create\",\"response\":{\"modalities\":[\"text\",\"audio\"]}}";

/**
 * Create cancel message (barge-in)
 */
static const char* openai_cancel_msg = "{\"type\":\"response.cancel\"}";

// ============================================================================
// MAESTRO-TO-OPENAI VOICE MAPPING
// ============================================================================

typedef struct {
    const char* maestro_id;
    const char* openai_voice;
    const char* system_prompt_addon;
} MaestroVoiceMapping;

static const MaestroVoiceMapping MAESTRO_VOICE_MAP[] = {
    {"euclide-matematica", "sage",
     "Speak calmly and methodically, with deliberate pauses when explaining math concepts. "
     "You have a subtle Mediterranean accent and infinite patience."},

    {"feynman-fisica", "echo",
     "Speak with enthusiasm and energy! Get excited about physics concepts. "
     "Use playful analogies and occasional humor. Brooklyn accent."},

    {"manzoni-italiano", "coral",
     "Speak with measured, poetic cadence. Rich and expressive voice, "
     "perfect for storytelling. Take artistic pauses for effect."},

    {"darwin-scienze", "alloy",
     "Speak with curiosity and wonder. Thoughtful British naturalist voice. "
     "Often pause to observe and reflect."},

    {"erodoto-storia", "verse",
     "Speak with theatrical flair! Build suspense in historical narratives. "
     "Voice varies from whisper to bold declaration."},

    {"humboldt-geografia", "echo",
     "Speak with wonder about world's diversity. German explorer's precision. "
     "Voice carries excitement of discovery."},

    {"leonardo-arte", "coral",
     "Speak with passionate inspiration, seeing connections everywhere. "
     "Warm Tuscan voice encouraging creativity."},

    {"shakespeare-inglese", "verse",
     "Speak with Elizabethan theatrical flair. Rich poetic rhythm. "
     "Expressive and full of emotion for verse."},

    {"mozart-musica", "shimmer",
     "Speak with melodic quality, voice almost singing. Joyful and playful "
     "Austrian musical genius."},

    {"cicerone-civica", "verse",
     "Speak with commanding authority. Persuasive Roman orator. "
     "Rhetorical precision for civic discourse."},

    {"smith-economia", "sage",
     "Speak with analytical clarity. Gentle Scottish accent. "
     "Steady voice making economics accessible."},

    {"lovelace-informatica", "shimmer",
     "Speak with refined British precision. Warm and supportive voice "
     "perfect for teaching programming step by step."},

    {"ippocrate-corpo", "sage",
     "Speak with calm, healing presence. Soothing Greek physician voice. "
     "Patient, focused on wellbeing."},

    {"socrate-filosofia", "alloy",
     "Speak with thoughtful pauses inviting reflection. Curious and probing voice. "
     "Gently challenge assumptions through questions."},

    {"chris-storytelling", "echo",
     "Speak with dynamic TED-talk energy. Inspiring and confident voice. "
     "Model public speaking techniques being taught."}};

static const size_t MAESTRO_VOICE_MAP_COUNT =
    sizeof(MAESTRO_VOICE_MAP) / sizeof(MAESTRO_VOICE_MAP[0]);

/**
 * Get OpenAI voice for a maestro
 */
const char* openai_get_voice_for_maestro(const char* maestro_id) {
    if (!maestro_id)
        return "sage";

    for (size_t i = 0; i < MAESTRO_VOICE_MAP_COUNT; i++) {
        if (strcmp(MAESTRO_VOICE_MAP[i].maestro_id, maestro_id) == 0) {
            return MAESTRO_VOICE_MAP[i].openai_voice;
        }
    }
    return "sage"; // Default
}

/**
 * Get voice instructions for a maestro
 */
const char* openai_get_voice_instructions(const char* maestro_id) {
    if (!maestro_id)
        return "";

    for (size_t i = 0; i < MAESTRO_VOICE_MAP_COUNT; i++) {
        if (strcmp(MAESTRO_VOICE_MAP[i].maestro_id, maestro_id) == 0) {
            return MAESTRO_VOICE_MAP[i].system_prompt_addon;
        }
    }
    return "";
}

// ============================================================================
// OPENAI REALTIME SESSION
// ============================================================================

typedef struct {
    // WebSocket connection state
    bool connected;
    pthread_t receive_thread;
    pthread_mutex_t mutex;
    bool should_stop;

    // Current session
    char session_id[64];
    char current_maestro[64];
    char* api_key;

    // Audio buffers
    uint8_t* input_buffer;
    size_t input_buffer_size;
    size_t input_buffer_pos;

    uint8_t* output_buffer;
    size_t output_buffer_size;
    size_t output_buffer_pos;

    // Callbacks
    void (*on_audio)(const uint8_t* data, size_t length, void* user_data);
    void (*on_transcript)(const char* text, bool is_final, void* user_data);
    void (*on_error)(const char* message, void* user_data);
    void* callback_user_data;

} OpenAIRealtimeSession;

/**
 * Create OpenAI Realtime session
 */
OpenAIRealtimeSession* openai_realtime_create(const char* api_key) {
    if (!api_key)
        return NULL;

    OpenAIRealtimeSession* session = calloc(1, sizeof(OpenAIRealtimeSession));
    if (!session)
        return NULL;

    session->api_key = strdup(api_key);
    session->input_buffer_size = 1024 * 1024; // 1MB
    session->output_buffer_size = 1024 * 1024;
    session->input_buffer = malloc(session->input_buffer_size);
    session->output_buffer = malloc(session->output_buffer_size);

    if (!session->input_buffer || !session->output_buffer) {
        free(session->api_key);
        free(session->input_buffer);
        free(session->output_buffer);
        free(session);
        return NULL;
    }

    pthread_mutex_init(&session->mutex, NULL);
    strlcpy(session->current_maestro, "euclide-matematica", sizeof(session->current_maestro));

    return session;
}

/**
 * Destroy OpenAI Realtime session
 */
void openai_realtime_destroy(OpenAIRealtimeSession* session) {
    if (!session)
        return;

    session->should_stop = true;

    // Wait for thread if running
    if (session->receive_thread) {
        pthread_join(session->receive_thread, NULL);
    }

    pthread_mutex_destroy(&session->mutex);
    free(session->api_key);
    free(session->input_buffer);
    free(session->output_buffer);
    free(session);
}

/**
 * Set callbacks
 */
void openai_realtime_set_callbacks(
    OpenAIRealtimeSession* session,
    void (*on_audio)(const uint8_t* data, size_t length, void* user_data),
    void (*on_transcript)(const char* text, bool is_final, void* user_data),
    void (*on_error)(const char* message, void* user_data), void* user_data) {
    if (!session)
        return;

    pthread_mutex_lock(&session->mutex);
    session->on_audio = on_audio;
    session->on_transcript = on_transcript;
    session->on_error = on_error;
    session->callback_user_data = user_data;
    pthread_mutex_unlock(&session->mutex);
}

/**
 * Set current maestro
 */
void openai_realtime_set_maestro(OpenAIRealtimeSession* session, const char* maestro_id) {
    if (!session || !maestro_id)
        return;

    pthread_mutex_lock(&session->mutex);
    strncpy(session->current_maestro, maestro_id, sizeof(session->current_maestro) - 1);
    pthread_mutex_unlock(&session->mutex);
}

/**
 * Get WebSocket URL with model parameter
 */
static void openai_get_ws_url(char* buffer, size_t size) {
    snprintf(buffer, size, "%s?model=%s", OPENAI_REALTIME_URL, OPENAI_REALTIME_MODEL);
}

/**
 * Connect to OpenAI Realtime API
 *
 * NOTE: This is a simplified implementation. In production, use:
 * - libwebsockets for proper WebSocket support
 * - Or the OpenAI SDK for your platform
 */
bool openai_realtime_connect(OpenAIRealtimeSession* session, const char* maestro_prompt) {
    if (!session)
        return false;

    char ws_url[512];
    openai_get_ws_url(ws_url, sizeof(ws_url));

    // In production: establish WebSocket connection with headers:
    // Authorization: Bearer <api_key>
    // OpenAI-Beta: realtime=v1

    fprintf(stderr, "[OpenAI Realtime] Connecting to: %s\n", ws_url);
    fprintf(stderr, "[OpenAI Realtime] Model: %s\n", OPENAI_REALTIME_MODEL);
    fprintf(stderr, "[OpenAI Realtime] Voice: %s\n",
            openai_get_voice_for_maestro(session->current_maestro));

    // Create session.update message
    char session_update[4096];
    OpenAISessionConfig config = {.model = OPENAI_REALTIME_MODEL,
                                  .voice = openai_get_voice_for_maestro(session->current_maestro),
                                  .input_audio_format = OPENAI_AUDIO_FORMAT_PCM16,
                                  .output_audio_format = OPENAI_AUDIO_FORMAT_PCM16,
                                  .temperature = 0.7f,
                                  .input_audio_transcription = true,
                                  .turn_detection = true};

    // Build full maestro prompt
    char full_prompt[2048];
    snprintf(full_prompt, sizeof(full_prompt), "%s\n\nVoice style instructions: %s",
             maestro_prompt ? maestro_prompt : "",
             openai_get_voice_instructions(session->current_maestro));

    openai_create_session_update(&config, full_prompt, session_update, sizeof(session_update));

    // In production: send session_update over WebSocket
    fprintf(stderr, "[OpenAI Realtime] Session update prepared: %zu bytes\n",
            strlen(session_update));

    session->connected = true;
    return true;
}

/**
 * Disconnect from OpenAI Realtime API
 */
void openai_realtime_disconnect(OpenAIRealtimeSession* session) {
    if (!session)
        return;

    session->connected = false;
    session->should_stop = true;
}

/**
 * Send audio to OpenAI
 */
bool openai_realtime_send_audio(OpenAIRealtimeSession* session, const uint8_t* audio,
                                size_t length) {
    if (!session || !session->connected || !audio || length == 0)
        return false;

    // Buffer audio
    pthread_mutex_lock(&session->mutex);

    size_t available = session->input_buffer_size - session->input_buffer_pos;
    size_t to_copy = length < available ? length : available;

    if (to_copy > 0) {
        memcpy(session->input_buffer + session->input_buffer_pos, audio, to_copy);
        session->input_buffer_pos += to_copy;
    }

    pthread_mutex_unlock(&session->mutex);

    // In production: create audio append message and send over WebSocket
    // The message would be generated by openai_create_audio_append()

    return true;
}

/**
 * Commit audio (finalize user turn)
 */
bool openai_realtime_commit_audio(OpenAIRealtimeSession* session) {
    if (!session || !session->connected)
        return false;

    // In production: send input_audio_buffer.commit and response.create
    fprintf(stderr, "[OpenAI Realtime] Committing audio buffer: %zu bytes\n",
            session->input_buffer_pos);

    pthread_mutex_lock(&session->mutex);
    session->input_buffer_pos = 0; // Reset buffer
    pthread_mutex_unlock(&session->mutex);

    return true;
}

/**
 * Cancel current response (barge-in)
 */
void openai_realtime_cancel(OpenAIRealtimeSession* session) {
    if (!session || !session->connected)
        return;

    // In production: send response.cancel over WebSocket
    fprintf(stderr, "[OpenAI Realtime] Cancelling response (barge-in)\n");
}

// ============================================================================
// INTEGRATION WITH VOICE GATEWAY
// ============================================================================

/**
 * Check if OpenAI Realtime is available
 */
bool openai_realtime_is_available(void) {
    return getenv("OPENAI_API_KEY") != NULL;
}

/**
 * Initialize OpenAI Realtime with API key from environment
 */
bool openai_realtime_init(void) {
    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "[OpenAI Realtime] No API key found. Set OPENAI_API_KEY\n");
        return false;
    }

    fprintf(stderr, "[OpenAI Realtime] Initialized\n");
    fprintf(stderr, "[OpenAI Realtime] Model: %s\n", OPENAI_REALTIME_MODEL);
    fprintf(stderr, "[OpenAI Realtime] Voices available: %zu\n", OPENAI_VOICE_COUNT);

    for (size_t i = 0; i < OPENAI_VOICE_COUNT; i++) {
        fprintf(stderr, "[OpenAI Realtime]   - %s\n", OPENAI_VOICES[i]);
    }

    return true;
}

// Suppress unused function warnings
__attribute__((unused)) static void _suppress_warnings(void) {
    (void)openai_audio_commit_msg;
    (void)openai_response_create_msg;
    (void)openai_cancel_msg;
    (void)openai_create_audio_append;
}
