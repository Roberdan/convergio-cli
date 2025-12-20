/**
 * CONVERGIO EDUCATION - VOICE WEBSOCKET CLIENT
 *
 * WebSocket client for Azure/OpenAI Realtime API using libwebsockets.
 * Handles bidirectional audio streaming for conversational voice.
 *
 * ADR: ADR-003-voice-cli-conversational-ux.md
 *
 * Build with: make VOICE=1
 * Dependency: brew install libwebsockets
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifdef CONVERGIO_VOICE_ENABLED

#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define VOICE_WS_RX_BUFFER_SIZE (1024 * 64)   // 64KB receive buffer
#define VOICE_WS_TX_BUFFER_SIZE (1024 * 32)   // 32KB transmit buffer
#define VOICE_AUDIO_CHUNK_SIZE  4800          // 100ms of audio at 24kHz mono 16-bit
#define VOICE_SAMPLE_RATE       24000
#define VOICE_RECONNECT_DELAY   1000          // 1 second

// ============================================================================
// TYPES
// ============================================================================

typedef enum {
    VOICE_WS_DISCONNECTED,
    VOICE_WS_CONNECTING,
    VOICE_WS_CONNECTED,
    VOICE_WS_AUTHENTICATING,
    VOICE_WS_READY,
    VOICE_WS_ERROR
} VoiceWebSocketState;

typedef struct {
    // Connection state
    VoiceWebSocketState state;
    struct lws_context *context;
    struct lws *wsi;

    // Configuration
    char api_key[256];
    char endpoint[512];
    char deployment[64];
    bool use_azure;

    // Audio buffers
    uint8_t *audio_send_buffer;
    size_t audio_send_size;
    size_t audio_send_pos;

    uint8_t *audio_recv_buffer;
    size_t audio_recv_size;
    size_t audio_recv_pos;

    // Transcript
    char *current_transcript;
    bool transcript_is_final;

    // Callbacks
    void (*on_audio_received)(const uint8_t *data, size_t length, void *user_data);
    void (*on_transcript)(const char *text, bool is_user, bool is_final, void *user_data);
    void (*on_state_change)(VoiceWebSocketState state, void *user_data);
    void (*on_error)(const char *message, void *user_data);
    void *callback_user_data;

    // Threading
    pthread_t service_thread;
    pthread_mutex_t mutex;
    bool running;
    bool should_stop;

    // Session
    char session_id[64];
    char current_maestro[64];
    char maestro_instructions[2048];

} VoiceWebSocket;

// Global instance for callback access
static VoiceWebSocket *g_voice_ws = NULL;

// Forward declarations
void voice_ws_disconnect(VoiceWebSocket *ws);

// ============================================================================
// BASE64 ENCODING/DECODING
// ============================================================================

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static size_t base64_encode(const uint8_t *data, size_t length, char *output, size_t output_size) {
    size_t out_len = ((length + 2) / 3) * 4;
    if (output_size < out_len + 1) return 0;

    size_t j = 0;
    for (size_t i = 0; i < length; i += 3) {
        uint32_t n = ((uint32_t)data[i]) << 16;
        if (i + 1 < length) n |= ((uint32_t)data[i + 1]) << 8;
        if (i + 2 < length) n |= data[i + 2];

        output[j++] = b64_table[(n >> 18) & 63];
        output[j++] = b64_table[(n >> 12) & 63];
        output[j++] = (i + 1 < length) ? b64_table[(n >> 6) & 63] : '=';
        output[j++] = (i + 2 < length) ? b64_table[n & 63] : '=';
    }
    output[j] = '\0';
    return j;
}

static int b64_decode_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static size_t base64_decode(const char *input, size_t length, uint8_t *output, size_t output_size) {
    if (length % 4 != 0) return 0;

    size_t out_len = (length / 4) * 3;
    if (input[length - 1] == '=') out_len--;
    if (input[length - 2] == '=') out_len--;
    if (output_size < out_len) return 0;

    size_t j = 0;
    for (size_t i = 0; i < length; i += 4) {
        int n0 = b64_decode_char(input[i]);
        int n1 = b64_decode_char(input[i + 1]);
        int n2 = (input[i + 2] != '=') ? b64_decode_char(input[i + 2]) : 0;
        int n3 = (input[i + 3] != '=') ? b64_decode_char(input[i + 3]) : 0;

        if (n0 < 0 || n1 < 0) return 0;

        uint32_t n = (n0 << 18) | (n1 << 12) | (n2 << 6) | n3;

        output[j++] = (n >> 16) & 0xFF;
        if (input[i + 2] != '=') output[j++] = (n >> 8) & 0xFF;
        if (input[i + 3] != '=') output[j++] = n & 0xFF;
    }

    return j;
}

// ============================================================================
// MESSAGE BUILDING
// ============================================================================

static void build_session_update(VoiceWebSocket *ws, char *buffer, size_t size) {
    snprintf(buffer, size,
        "{"
        "\"type\":\"session.update\","
        "\"session\":{"
            "\"modalities\":[\"text\",\"audio\"],"
            "\"instructions\":\"%s\","
            "\"voice\":\"sage\","
            "\"input_audio_format\":\"pcm16\","
            "\"output_audio_format\":\"pcm16\","
            "\"input_audio_transcription\":{\"model\":\"whisper-1\"},"
            "\"turn_detection\":{"
                "\"type\":\"server_vad\","
                "\"threshold\":0.5,"
                "\"prefix_padding_ms\":300,"
                "\"silence_duration_ms\":500"
            "}"
        "}"
        "}",
        ws->maestro_instructions[0] ? ws->maestro_instructions :
            "You are a helpful educational assistant. Speak naturally in Italian."
    );
}

static void build_audio_append(const uint8_t *audio, size_t length, char *buffer, size_t size) {
    char *b64 = malloc(((length + 2) / 3) * 4 + 1);
    if (!b64) return;

    base64_encode(audio, length, b64, ((length + 2) / 3) * 4 + 1);

    snprintf(buffer, size,
        "{\"type\":\"input_audio_buffer.append\",\"audio\":\"%s\"}",
        b64
    );

    free(b64);
}

static const char *MSG_RESPONSE_CREATE =
    "{\"type\":\"response.create\",\"response\":{\"modalities\":[\"text\",\"audio\"]}}";

static const char *MSG_AUDIO_COMMIT =
    "{\"type\":\"input_audio_buffer.commit\"}";

static const char *MSG_RESPONSE_CANCEL =
    "{\"type\":\"response.cancel\"}";

// ============================================================================
// MESSAGE PARSING
// ============================================================================

static void parse_audio_delta(VoiceWebSocket *ws, const char *json) {
    // Find "delta":"<base64>" in JSON
    const char *delta_key = "\"delta\":\"";
    const char *delta_start = strstr(json, delta_key);
    if (!delta_start) return;

    delta_start += strlen(delta_key);
    const char *delta_end = strchr(delta_start, '"');
    if (!delta_end) return;

    size_t b64_len = delta_end - delta_start;
    if (b64_len == 0) return;

    // Decode base64
    size_t max_decoded = (b64_len / 4) * 3;
    uint8_t *audio = malloc(max_decoded);
    if (!audio) return;

    size_t decoded_len = base64_decode(delta_start, b64_len, audio, max_decoded);

    if (decoded_len > 0 && ws->on_audio_received) {
        ws->on_audio_received(audio, decoded_len, ws->callback_user_data);
    }

    free(audio);
}

static void parse_transcript(VoiceWebSocket *ws, const char *json, bool is_user) {
    // Find "transcript":"<text>" or "text":"<text>"
    const char *text_key = is_user ? "\"transcript\":\"" : "\"text\":\"";
    const char *text_start = strstr(json, text_key);
    if (!text_start) {
        text_key = "\"text\":\"";
        text_start = strstr(json, text_key);
    }
    if (!text_start) return;

    text_start += strlen(text_key);
    const char *text_end = strchr(text_start, '"');
    if (!text_end) return;

    size_t len = text_end - text_start;
    char *transcript = malloc(len + 1);
    if (!transcript) return;

    memcpy(transcript, text_start, len);
    transcript[len] = '\0';

    bool is_final = strstr(json, "\"is_final\":true") != NULL ||
                    strstr(json, "\"type\":\"conversation.item.input_audio_transcription.completed\"") != NULL;

    if (ws->on_transcript) {
        ws->on_transcript(transcript, is_user, is_final, ws->callback_user_data);
    }

    free(transcript);
}

static void handle_message(VoiceWebSocket *ws, const char *json, size_t len) {
    // Parse message type
    if (strstr(json, "\"type\":\"response.audio.delta\"")) {
        parse_audio_delta(ws, json);
    }
    else if (strstr(json, "\"type\":\"response.audio_transcript.delta\"")) {
        parse_transcript(ws, json, false);
    }
    else if (strstr(json, "\"type\":\"conversation.item.input_audio_transcription\"")) {
        parse_transcript(ws, json, true);
    }
    else if (strstr(json, "\"type\":\"session.created\"")) {
        ws->state = VOICE_WS_READY;
        if (ws->on_state_change) {
            ws->on_state_change(VOICE_WS_READY, ws->callback_user_data);
        }

        // Send session update with maestro instructions
        char session_update[4096];
        build_session_update(ws, session_update, sizeof(session_update));

        unsigned char buf[LWS_PRE + 4096];
        size_t msg_len = strlen(session_update);
        memcpy(&buf[LWS_PRE], session_update, msg_len);
        lws_write(ws->wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
    }
    else if (strstr(json, "\"type\":\"error\"")) {
        if (ws->on_error) {
            ws->on_error(json, ws->callback_user_data);
        }
    }
    else if (strstr(json, "\"type\":\"response.done\"")) {
        // Response complete, ready for more input
    }
}

// ============================================================================
// WEBSOCKET CALLBACKS
// ============================================================================

static int voice_ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                              void *user, void *in, size_t len) {
    VoiceWebSocket *ws = g_voice_ws;
    if (!ws) return 0;

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            fprintf(stderr, "[Voice WS] Connected\n");
            ws->state = VOICE_WS_CONNECTED;
            if (ws->on_state_change) {
                ws->on_state_change(VOICE_WS_CONNECTED, ws->callback_user_data);
            }
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            handle_message(ws, (const char *)in, len);
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            // Send queued audio if any
            pthread_mutex_lock(&ws->mutex);
            if (ws->audio_send_pos > 0) {
                char msg[VOICE_WS_TX_BUFFER_SIZE + 1024];
                build_audio_append(ws->audio_send_buffer, ws->audio_send_pos, msg, sizeof(msg));

                unsigned char buf[LWS_PRE + VOICE_WS_TX_BUFFER_SIZE + 1024];
                size_t msg_len = strlen(msg);
                memcpy(&buf[LWS_PRE], msg, msg_len);
                lws_write(wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);

                ws->audio_send_pos = 0;
            }
            pthread_mutex_unlock(&ws->mutex);
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            fprintf(stderr, "[Voice WS] Connection error: %s\n", in ? (char *)in : "unknown");
            ws->state = VOICE_WS_ERROR;
            if (ws->on_error) {
                ws->on_error(in ? (char *)in : "Connection failed", ws->callback_user_data);
            }
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            fprintf(stderr, "[Voice WS] Disconnected\n");
            ws->state = VOICE_WS_DISCONNECTED;
            ws->wsi = NULL;
            if (ws->on_state_change) {
                ws->on_state_change(VOICE_WS_DISCONNECTED, ws->callback_user_data);
            }
            break;

        default:
            break;
    }

    return 0;
}

static const struct lws_protocols protocols[] = {
    {
        .name = "realtime",
        .callback = voice_ws_callback,
        .per_session_data_size = 0,
        .rx_buffer_size = VOICE_WS_RX_BUFFER_SIZE,
    },
    { NULL, NULL, 0, 0 }
};

// ============================================================================
// SERVICE THREAD
// ============================================================================

static void *voice_ws_service_thread(void *arg) {
    VoiceWebSocket *ws = (VoiceWebSocket *)arg;

    while (!ws->should_stop) {
        if (ws->context) {
            lws_service(ws->context, 50);  // 50ms timeout
        } else {
            usleep(10000);  // 10ms
        }
    }

    return NULL;
}

// ============================================================================
// PUBLIC API
// ============================================================================

VoiceWebSocket *voice_ws_create(void) {
    VoiceWebSocket *ws = calloc(1, sizeof(VoiceWebSocket));
    if (!ws) return NULL;

    ws->state = VOICE_WS_DISCONNECTED;

    ws->audio_send_buffer = malloc(VOICE_WS_TX_BUFFER_SIZE);
    ws->audio_recv_buffer = malloc(VOICE_WS_RX_BUFFER_SIZE);
    ws->audio_send_size = VOICE_WS_TX_BUFFER_SIZE;
    ws->audio_recv_size = VOICE_WS_RX_BUFFER_SIZE;

    if (!ws->audio_send_buffer || !ws->audio_recv_buffer) {
        free(ws->audio_send_buffer);
        free(ws->audio_recv_buffer);
        free(ws);
        return NULL;
    }

    pthread_mutex_init(&ws->mutex, NULL);

    g_voice_ws = ws;

    return ws;
}

void voice_ws_destroy(VoiceWebSocket *ws) {
    if (!ws) return;

    voice_ws_disconnect(ws);

    pthread_mutex_destroy(&ws->mutex);
    free(ws->audio_send_buffer);
    free(ws->audio_recv_buffer);
    free(ws->current_transcript);
    free(ws);

    if (g_voice_ws == ws) g_voice_ws = NULL;
}

bool voice_ws_connect(VoiceWebSocket *ws) {
    if (!ws) return false;

    ws->state = VOICE_WS_CONNECTING;

    // Load API configuration from environment
    const char *azure_endpoint = getenv("AZURE_OPENAI_REALTIME_ENDPOINT");
    const char *azure_key = getenv("AZURE_OPENAI_REALTIME_API_KEY");
    const char *azure_deployment = getenv("AZURE_OPENAI_REALTIME_DEPLOYMENT");
    const char *openai_key = getenv("OPENAI_API_KEY");

    // Prefer Azure, fallback to OpenAI
    if (azure_endpoint && azure_key && azure_deployment) {
        ws->use_azure = true;
        strncpy(ws->endpoint, azure_endpoint, sizeof(ws->endpoint) - 1);
        strncpy(ws->api_key, azure_key, sizeof(ws->api_key) - 1);
        strncpy(ws->deployment, azure_deployment, sizeof(ws->deployment) - 1);

        // Remove https:// prefix if present
        char *host = ws->endpoint;
        if (strncmp(host, "https://", 8) == 0) host += 8;
        if (strncmp(host, "http://", 7) == 0) host += 7;

        fprintf(stderr, "[Voice WS] Using Azure OpenAI: %s\n", host);
    } else if (openai_key) {
        ws->use_azure = false;
        strcpy(ws->endpoint, "api.openai.com");
        strncpy(ws->api_key, openai_key, sizeof(ws->api_key) - 1);
        strcpy(ws->deployment, "gpt-4o-realtime-preview");

        fprintf(stderr, "[Voice WS] Using OpenAI direct\n");
    } else {
        fprintf(stderr, "[Voice WS] No API key found!\n");
        ws->state = VOICE_WS_ERROR;
        return false;
    }

    // Create context
    struct lws_context_creation_info info = {0};
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    ws->context = lws_create_context(&info);
    if (!ws->context) {
        fprintf(stderr, "[Voice WS] Failed to create context\n");
        ws->state = VOICE_WS_ERROR;
        return false;
    }

    // Build connection path
    char path[1024];
    if (ws->use_azure) {
        snprintf(path, sizeof(path),
            "/openai/realtime?api-version=2025-04-01-preview&deployment=%s",
            ws->deployment);
    } else {
        snprintf(path, sizeof(path),
            "/v1/realtime?model=%s",
            ws->deployment);
    }

    // Connect
    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = ws->context;
    ccinfo.address = ws->endpoint;
    ccinfo.port = 443;
    ccinfo.path = path;
    ccinfo.host = ws->endpoint;
    ccinfo.origin = ws->endpoint;
    ccinfo.ssl_connection = LCCSCF_USE_SSL;
    ccinfo.protocol = "realtime";

    ws->wsi = lws_client_connect_via_info(&ccinfo);
    if (!ws->wsi) {
        fprintf(stderr, "[Voice WS] Failed to connect\n");
        lws_context_destroy(ws->context);
        ws->context = NULL;
        ws->state = VOICE_WS_ERROR;
        return false;
    }

    // Start service thread
    ws->should_stop = false;
    ws->running = true;
    pthread_create(&ws->service_thread, NULL, voice_ws_service_thread, ws);

    return true;
}

void voice_ws_disconnect(VoiceWebSocket *ws) {
    if (!ws) return;

    ws->should_stop = true;

    if (ws->running) {
        pthread_join(ws->service_thread, NULL);
        ws->running = false;
    }

    if (ws->context) {
        lws_context_destroy(ws->context);
        ws->context = NULL;
    }

    ws->wsi = NULL;
    ws->state = VOICE_WS_DISCONNECTED;
}

void voice_ws_send_audio(VoiceWebSocket *ws, const uint8_t *audio, size_t length) {
    if (!ws || ws->state != VOICE_WS_READY) return;

    pthread_mutex_lock(&ws->mutex);

    size_t available = ws->audio_send_size - ws->audio_send_pos;
    size_t to_copy = length < available ? length : available;

    if (to_copy > 0) {
        memcpy(ws->audio_send_buffer + ws->audio_send_pos, audio, to_copy);
        ws->audio_send_pos += to_copy;
    }

    pthread_mutex_unlock(&ws->mutex);

    // Request write callback
    if (ws->wsi) {
        lws_callback_on_writable(ws->wsi);
    }
}

void voice_ws_commit_audio(VoiceWebSocket *ws) {
    if (!ws || !ws->wsi || ws->state != VOICE_WS_READY) return;

    unsigned char buf[LWS_PRE + 256];
    size_t len = strlen(MSG_AUDIO_COMMIT);
    memcpy(&buf[LWS_PRE], MSG_AUDIO_COMMIT, len);
    lws_write(ws->wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
}

void voice_ws_request_response(VoiceWebSocket *ws) {
    if (!ws || !ws->wsi || ws->state != VOICE_WS_READY) return;

    unsigned char buf[LWS_PRE + 256];
    size_t len = strlen(MSG_RESPONSE_CREATE);
    memcpy(&buf[LWS_PRE], MSG_RESPONSE_CREATE, len);
    lws_write(ws->wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
}

void voice_ws_cancel_response(VoiceWebSocket *ws) {
    if (!ws || !ws->wsi || ws->state != VOICE_WS_READY) return;

    unsigned char buf[LWS_PRE + 256];
    size_t len = strlen(MSG_RESPONSE_CANCEL);
    memcpy(&buf[LWS_PRE], MSG_RESPONSE_CANCEL, len);
    lws_write(ws->wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
}

void voice_ws_set_maestro(VoiceWebSocket *ws, const char *maestro_id, const char *instructions) {
    if (!ws) return;

    strncpy(ws->current_maestro, maestro_id, sizeof(ws->current_maestro) - 1);
    if (instructions) {
        strncpy(ws->maestro_instructions, instructions, sizeof(ws->maestro_instructions) - 1);
    }
}

void voice_ws_set_callbacks(VoiceWebSocket *ws,
                            void (*on_audio)(const uint8_t*, size_t, void*),
                            void (*on_transcript)(const char*, bool, bool, void*),
                            void (*on_state)(VoiceWebSocketState, void*),
                            void (*on_error)(const char*, void*),
                            void *user_data) {
    if (!ws) return;

    ws->on_audio_received = on_audio;
    ws->on_transcript = on_transcript;
    ws->on_state_change = on_state;
    ws->on_error = on_error;
    ws->callback_user_data = user_data;
}

VoiceWebSocketState voice_ws_get_state(VoiceWebSocket *ws) {
    return ws ? ws->state : VOICE_WS_DISCONNECTED;
}

#endif /* CONVERGIO_VOICE_ENABLED */
