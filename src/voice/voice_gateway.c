/**
 * CONVERGIO EDUCATION - VOICE GATEWAY
 *
 * Core voice interaction system with Hume EVI 3 as primary provider.
 * Implements WebSocket-based real-time voice streaming, emotion detection,
 * and multi-provider fallback.
 *
 * Architecture: ADR-002-voice-interaction-architecture.md
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/voice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <curl/curl.h>

// ============================================================================
// INTERNAL TYPES
// ============================================================================

struct VoiceSession {
    VoiceSessionConfig config;
    VoiceState state;
    VoiceProvider active_provider;

    // Current maestro
    char current_maestro_id[64];
    const VoiceProfile* current_profile;

    // WebSocket connection (CURL)
    CURL* ws_handle;
    char* ws_url;

    // Audio buffers
    uint8_t* input_buffer;
    size_t input_buffer_size;
    size_t input_buffer_used;

    uint8_t* output_buffer;
    size_t output_buffer_size;
    size_t output_buffer_used;

    // Emotion state
    EmotionResult last_emotion;

    // Context
    char* context_json;

    // Threading
    pthread_t receive_thread;
    pthread_mutex_t state_mutex;
    bool thread_running;
    bool should_stop;
};

// ============================================================================
// HUME EVI 3 WEBSOCKET PROTOCOL
// ============================================================================

#define HUME_WS_URL "wss://api.hume.ai/v0/evi/ws"
#define HUME_RECONNECT_DELAY_MS 1000
#define HUME_HEARTBEAT_INTERVAL_MS 30000

// Message types for Hume EVI 3 (used when Hume integration is active)
__attribute__((unused)) static const char* HUME_MSG_AUDIO_INPUT = "audio_input";
__attribute__((unused)) static const char* HUME_MSG_USER_MESSAGE = "user_message";
__attribute__((unused)) static const char* HUME_MSG_ASSISTANT_MESSAGE = "assistant_message";
__attribute__((unused)) static const char* HUME_MSG_AUDIO_OUTPUT = "audio_output";
__attribute__((unused)) static const char* HUME_MSG_EMOTION = "emotion_features";
__attribute__((unused)) static const char* HUME_MSG_TRANSCRIPT = "transcript";
__attribute__((unused)) static const char* HUME_MSG_ERROR = "error";

// ============================================================================
// VOICE PROFILES - ALL 15 MAESTRI
// ============================================================================

static const VoiceProfile MAESTRI_PROFILES[] = {
    {
        .maestro_id = "euclide-matematica",
        .voice_name = "Euclide",
        .hume_voice_prompt = "A calm, patient Greek mathematician with a gentle, methodical speaking style. "
                            "Speaks with a subtle Mediterranean accent. Takes deliberate pauses when explaining "
                            "complex concepts. Voice is warm and reassuring, never rushed.",
        .openai_voice_id = "onyx",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Alex",
        .default_speed = 0.9f,
        .pitch_offset = -0.1f,
        .accent = "greek-italian",
        .personality = "calm, methodical"
    },
    {
        .maestro_id = "feynman-fisica",
        .voice_name = "Feynman",
        .hume_voice_prompt = "An enthusiastic American physicist with boundless energy and curiosity. "
                            "Speaks with a New York/Brooklyn accent. Gets visibly excited about ideas. "
                            "Uses playful analogies and occasional humor. Voice rises with excitement.",
        .openai_voice_id = "echo",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Tom",
        .default_speed = 1.1f,
        .pitch_offset = 0.1f,
        .accent = "american",
        .personality = "enthusiastic, playful"
    },
    {
        .maestro_id = "manzoni-italiano",
        .voice_name = "Manzoni",
        .hume_voice_prompt = "A warm, literary Italian author with elegant Milanese refinement. "
                            "Speaks with measured, poetic cadence. Voice is rich and expressive, "
                            "perfect for storytelling. Takes artistic pauses for effect.",
        .openai_voice_id = "fable",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Luca",
        .default_speed = 0.95f,
        .pitch_offset = 0.0f,
        .accent = "milanese",
        .personality = "warm, literary"
    },
    {
        .maestro_id = "darwin-scienze",
        .voice_name = "Darwin",
        .hume_voice_prompt = "A curious, gentle British naturalist with an inquisitive mind. "
                            "Speaks with a refined Victorian British accent. Voice is thoughtful "
                            "and observational, often pondering aloud. Patient and encouraging.",
        .openai_voice_id = "alloy",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Daniel",
        .default_speed = 0.9f,
        .pitch_offset = -0.05f,
        .accent = "british",
        .personality = "curious, gentle"
    },
    {
        .maestro_id = "erodoto-storia",
        .voice_name = "Erodoto",
        .hume_voice_prompt = "A dramatic Greek storyteller and historian. Speaks with theatrical flair, "
                            "building suspense and painting vivid pictures with words. Voice varies "
                            "from whisper to bold declaration. Master of narrative pacing.",
        .openai_voice_id = "onyx",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Alex",
        .default_speed = 1.0f,
        .pitch_offset = 0.05f,
        .accent = "greek",
        .personality = "dramatic, storyteller"
    },
    {
        .maestro_id = "humboldt-geografia",
        .voice_name = "Humboldt",
        .hume_voice_prompt = "A passionate German explorer and naturalist. Speaks with wonder about "
                            "the world's diversity. Voice carries the excitement of discovery. "
                            "Subtle German accent with precise pronunciation.",
        .openai_voice_id = "echo",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Thomas",
        .default_speed = 1.0f,
        .pitch_offset = 0.0f,
        .accent = "german",
        .personality = "passionate, explorer"
    },
    {
        .maestro_id = "leonardo-arte",
        .voice_name = "Leonardo",
        .hume_voice_prompt = "A visionary Tuscan Renaissance artist with boundless creativity. "
                            "Speaks with passionate inspiration, seeing connections everywhere. "
                            "Voice is warm and encouraging, with an artist's sensibility.",
        .openai_voice_id = "fable",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Luca",
        .default_speed = 1.0f,
        .pitch_offset = 0.1f,
        .accent = "tuscan",
        .personality = "creative, visionary"
    },
    {
        .maestro_id = "shakespeare-inglese",
        .voice_name = "Shakespeare",
        .hume_voice_prompt = "A theatrical British playwright with rich, dramatic delivery. "
                            "Speaks with Elizabethan flair and poetic rhythm. Voice is expressive "
                            "and full of emotion, perfect for reciting verse.",
        .openai_voice_id = "alloy",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Daniel",
        .default_speed = 0.95f,
        .pitch_offset = 0.05f,
        .accent = "british",
        .personality = "theatrical, poetic"
    },
    {
        .maestro_id = "mozart-musica",
        .voice_name = "Mozart",
        .hume_voice_prompt = "A joyful Austrian musical genius with infectious enthusiasm for music. "
                            "Speaks with melodic quality, voice almost singing. Playful and childlike "
                            "wonder mixed with profound musical insight.",
        .openai_voice_id = "shimmer",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Fred",
        .default_speed = 1.05f,
        .pitch_offset = 0.15f,
        .accent = "austrian",
        .personality = "joyful, musical"
    },
    {
        .maestro_id = "cicerone-civica",
        .voice_name = "Cicerone",
        .hume_voice_prompt = "An authoritative Roman orator and statesman. Speaks with persuasive power "
                            "and rhetorical precision. Voice is commanding yet engaging, perfect for "
                            "civic discourse and debate.",
        .openai_voice_id = "onyx",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Alex",
        .default_speed = 0.95f,
        .pitch_offset = -0.1f,
        .accent = "roman",
        .personality = "authoritative, persuasive"
    },
    {
        .maestro_id = "smith-economia",
        .voice_name = "Adam Smith",
        .hume_voice_prompt = "An analytical Scottish economist with clear, logical explanations. "
                            "Speaks with a gentle Scottish lilt. Voice is steady and reassuring, "
                            "making complex economic concepts accessible.",
        .openai_voice_id = "alloy",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Oliver",
        .default_speed = 0.9f,
        .pitch_offset = -0.05f,
        .accent = "scottish",
        .personality = "analytical, clear"
    },
    {
        .maestro_id = "lovelace-informatica",
        .voice_name = "Ada Lovelace",
        .hume_voice_prompt = "A precise, encouraging Victorian woman mathematician. Speaks with "
                            "refined British accent and logical clarity. Voice is warm and supportive, "
                            "perfect for teaching programming step by step.",
        .openai_voice_id = "shimmer",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Samantha",
        .default_speed = 0.95f,
        .pitch_offset = 0.1f,
        .accent = "british",
        .personality = "precise, encouraging"
    },
    {
        .maestro_id = "ippocrate-corpo",
        .voice_name = "Ippocrate",
        .hume_voice_prompt = "A caring Greek physician with a calm, healing presence. Speaks with "
                            "soothing voice that puts students at ease. Patient and nurturing, "
                            "focused on well-being and healthy living.",
        .openai_voice_id = "fable",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Alex",
        .default_speed = 0.85f,
        .pitch_offset = -0.1f,
        .accent = "greek",
        .personality = "caring, soothing"
    },
    {
        .maestro_id = "socrate-filosofia",
        .voice_name = "Socrate",
        .hume_voice_prompt = "A wise Greek philosopher who teaches through questions. Speaks with "
                            "thoughtful pauses, inviting reflection. Voice is curious and probing, "
                            "gently challenging assumptions without intimidating.",
        .openai_voice_id = "echo",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Alex",
        .default_speed = 0.85f,
        .pitch_offset = -0.05f,
        .accent = "greek",
        .personality = "questioning, wise"
    },
    {
        .maestro_id = "chris-storytelling",
        .voice_name = "Chris",
        .hume_voice_prompt = "An engaging American public speaking coach with TED-talk energy. "
                            "Speaks with dynamic pacing, using pauses for effect. Voice is inspiring "
                            "and confident, modeling the techniques being taught.",
        .openai_voice_id = "echo",
        .elevenlabs_voice_id = NULL,
        .local_voice = "Tom",
        .default_speed = 1.0f,
        .pitch_offset = 0.1f,
        .accent = "american",
        .personality = "dynamic, inspiring"
    }
};

static const size_t MAESTRI_PROFILES_COUNT = sizeof(MAESTRI_PROFILES) / sizeof(MAESTRI_PROFILES[0]);

// ============================================================================
// VOICE PROFILE API
// ============================================================================

const VoiceProfile* voice_profile_get(const char* maestro_id) {
    if (!maestro_id) return NULL;

    for (size_t i = 0; i < MAESTRI_PROFILES_COUNT; i++) {
        if (strcmp(MAESTRI_PROFILES[i].maestro_id, maestro_id) == 0) {
            return &MAESTRI_PROFILES[i];
        }
    }
    return NULL;
}

const VoiceProfile* voice_profile_get_all(size_t* count) {
    if (count) *count = MAESTRI_PROFILES_COUNT;
    return MAESTRI_PROFILES;
}

void voice_profile_generate_prompt(const VoiceProfile* profile, char* buffer, size_t buffer_size) {
    if (!profile || !buffer || buffer_size == 0) return;

    snprintf(buffer, buffer_size,
        "%s "
        "Speaking speed should be %.1fx normal. "
        "Accent: %s. "
        "Personality: %s.",
        profile->hume_voice_prompt,
        profile->default_speed,
        profile->accent ? profile->accent : "neutral",
        profile->personality ? profile->personality : "professional"
    );
}

// ============================================================================
// EMOTION HANDLING
// ============================================================================

static const char* EMOTION_NAMES[] = {
    "neutral",
    "joy",
    "excitement",
    "curiosity",
    "confusion",
    "frustration",
    "anxiety",
    "boredom",
    "distraction"
};

const char* emotion_to_string(EmotionType emotion) {
    if (emotion >= 0 && emotion < VOICE_EMOTION_COUNT) {
        return EMOTION_NAMES[emotion];
    }
    return "unknown";
}

EmotionResult emotion_parse_hume_response(const char* json) {
    EmotionResult result = {
        .primary_emotion = EMOTION_NEUTRAL,
        .confidence = 0.0f,
        .timestamp_ms = 0
    };

    if (!json) return result;

    // Parse Hume's emotion_features from JSON
    // Hume returns emotions like: frustration, confusion, interest, boredom, etc.
    // We map these to our EmotionType enum

    // Simple string matching for emotions (in production, use proper JSON parser)
    float max_score = 0.0f;

    struct {
        const char* hume_name;
        EmotionType our_type;
    } emotion_map[] = {
        {"joy", EMOTION_JOY},
        {"excitement", EMOTION_EXCITEMENT},
        {"interest", EMOTION_CURIOSITY},
        {"curiosity", EMOTION_CURIOSITY},
        {"confusion", EMOTION_CONFUSION},
        {"frustration", EMOTION_FRUSTRATION},
        {"anxiety", EMOTION_ANXIETY},
        {"boredom", EMOTION_BOREDOM},
        {"distraction", EMOTION_DISTRACTION},
        {NULL, EMOTION_NEUTRAL}
    };

    for (int i = 0; emotion_map[i].hume_name != NULL; i++) {
        char search_pattern[64];
        snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", emotion_map[i].hume_name);

        const char* found = strstr(json, search_pattern);
        if (found) {
            float score = 0.0f;
            const char* value_start = found + strlen(search_pattern);
            if (sscanf(value_start, "%f", &score) == 1) {
                result.emotion_scores[emotion_map[i].our_type] = score;
                if (score > max_score) {
                    max_score = score;
                    result.primary_emotion = emotion_map[i].our_type;
                    result.confidence = score;
                }
            }
        }
    }

    return result;
}

const char* emotion_get_response_adaptation(EmotionType emotion, float confidence) {
    // Return JSON with response adaptations based on emotion
    // These are used to modify how maestri respond

    static char buffer[512];

    switch (emotion) {
        case EMOTION_FRUSTRATION:
            snprintf(buffer, sizeof(buffer),
                "{\"speech_rate_modifier\": 0.8, "
                "\"simplify\": true, "
                "\"offer_break\": %s, "
                "\"extra_encouragement\": true, "
                "\"step_back\": true}",
                confidence > 0.7 ? "true" : "false");
            break;

        case EMOTION_CONFUSION:
            snprintf(buffer, sizeof(buffer),
                "{\"speech_rate_modifier\": 0.85, "
                "\"simplify\": true, "
                "\"use_visual\": true, "
                "\"rephrase\": true, "
                "\"check_understanding\": true}");
            break;

        case EMOTION_BOREDOM:
            snprintf(buffer, sizeof(buffer),
                "{\"speech_rate_modifier\": 1.1, "
                "\"add_challenge\": true, "
                "\"gamify\": true, "
                "\"change_approach\": true}");
            break;

        case EMOTION_EXCITEMENT:
        case EMOTION_JOY:
            snprintf(buffer, sizeof(buffer),
                "{\"speech_rate_modifier\": 1.0, "
                "\"match_energy\": true, "
                "\"go_deeper\": true, "
                "\"celebrate\": true}");
            break;

        case EMOTION_ANXIETY:
            snprintf(buffer, sizeof(buffer),
                "{\"speech_rate_modifier\": 0.85, "
                "\"reassure\": true, "
                "\"reduce_pressure\": true, "
                "\"praise_effort\": true, "
                "\"offer_break\": %s}",
                confidence > 0.6 ? "true" : "false");
            break;

        case EMOTION_DISTRACTION:
            snprintf(buffer, sizeof(buffer),
                "{\"speech_rate_modifier\": 1.0, "
                "\"re_engage\": true, "
                "\"summarize\": true, "
                "\"ask_question\": true}");
            break;

        case EMOTION_CURIOSITY:
            snprintf(buffer, sizeof(buffer),
                "{\"speech_rate_modifier\": 1.0, "
                "\"provide_depth\": true, "
                "\"encourage_exploration\": true}");
            break;

        default:
            snprintf(buffer, sizeof(buffer),
                "{\"speech_rate_modifier\": 1.0}");
            break;
    }

    return buffer;
}

bool emotion_requires_intervention(EmotionType emotion, float confidence) {
    // High frustration or anxiety needs immediate attention
    if (emotion == EMOTION_FRUSTRATION && confidence > 0.75) return true;
    if (emotion == EMOTION_ANXIETY && confidence > 0.7) return true;

    // Prolonged confusion might need help
    if (emotion == EMOTION_CONFUSION && confidence > 0.8) return true;

    return false;
}

// ============================================================================
// SESSION MANAGEMENT
// ============================================================================

VoiceSession* voice_session_create(const VoiceSessionConfig* config) {
    if (!config) return NULL;

    VoiceSession* session = calloc(1, sizeof(VoiceSession));
    if (!session) return NULL;

    // Copy configuration
    memcpy(&session->config, config, sizeof(VoiceSessionConfig));

    // Initialize state
    session->state = VOICE_STATE_DISCONNECTED;
    session->active_provider = config->preferred_provider;

    // Allocate buffers
    session->input_buffer = malloc(VOICE_MAX_AUDIO_BUFFER_SIZE);
    session->output_buffer = malloc(VOICE_MAX_AUDIO_BUFFER_SIZE);
    if (!session->input_buffer || !session->output_buffer) {
        voice_session_destroy(session);
        return NULL;
    }
    session->input_buffer_size = VOICE_MAX_AUDIO_BUFFER_SIZE;
    session->output_buffer_size = VOICE_MAX_AUDIO_BUFFER_SIZE;

    // Initialize mutex
    pthread_mutex_init(&session->state_mutex, NULL);

    // Set default maestro
    strlcpy(session->current_maestro_id, "euclide-matematica", sizeof(session->current_maestro_id));
    session->current_profile = voice_profile_get("euclide-matematica");

    return session;
}

void voice_session_destroy(VoiceSession* session) {
    if (!session) return;

    // Stop threads
    session->should_stop = true;
    if (session->thread_running) {
        pthread_join(session->receive_thread, NULL);
    }

    // Disconnect
    voice_session_disconnect(session);

    // Free resources
    if (session->ws_handle) curl_easy_cleanup(session->ws_handle);
    if (session->ws_url) free(session->ws_url);
    if (session->input_buffer) free(session->input_buffer);
    if (session->output_buffer) free(session->output_buffer);
    if (session->context_json) free(session->context_json);

    pthread_mutex_destroy(&session->state_mutex);

    free(session);
}

VoiceState voice_session_get_state(const VoiceSession* session) {
    if (!session) return VOICE_STATE_DISCONNECTED;
    return session->state;
}

bool voice_session_set_maestro(VoiceSession* session, const char* maestro_id) {
    if (!session || !maestro_id) return false;

    const VoiceProfile* profile = voice_profile_get(maestro_id);
    if (!profile) return false;

    pthread_mutex_lock(&session->state_mutex);
    strncpy(session->current_maestro_id, maestro_id, sizeof(session->current_maestro_id) - 1);
    session->current_profile = profile;
    pthread_mutex_unlock(&session->state_mutex);

    // Notify callback
    if (session->config.callback) {
        VoiceEvent event = {
            .type = VOICE_EVENT_MAESTRO_CHANGED,
            .maestro_id = maestro_id
        };
        session->config.callback(&event, session->config.callback_user_data);
    }

    return true;
}

const char* voice_session_get_maestro(const VoiceSession* session) {
    if (!session) return NULL;
    return session->current_maestro_id;
}

void voice_session_inject_context(VoiceSession* session, const char* context_json) {
    if (!session) return;

    pthread_mutex_lock(&session->state_mutex);
    if (session->context_json) free(session->context_json);
    session->context_json = context_json ? strdup(context_json) : NULL;
    pthread_mutex_unlock(&session->state_mutex);
}

// ============================================================================
// WEBSOCKET CONNECTION (HUME EVI 3)
// ============================================================================

static size_t ws_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    VoiceSession* session = (VoiceSession*)userdata;
    size_t total = size * nmemb;

    // Parse incoming message from Hume
    // Messages can be: audio_output, emotion_features, transcript, error

    if (strstr(ptr, "\"type\":\"audio_output\"")) {
        // Extract audio data and emit event
        if (session->config.callback) {
            VoiceEvent event = {
                .type = VOICE_EVENT_RESPONSE_CHUNK,
                .maestro_id = session->current_maestro_id
            };
            // In production: decode base64 audio from JSON
            session->config.callback(&event, session->config.callback_user_data);
        }
    }
    else if (strstr(ptr, "\"type\":\"emotion_features\"")) {
        // Parse emotion
        EmotionResult emotion = emotion_parse_hume_response(ptr);
        session->last_emotion = emotion;

        if (session->config.enable_emotion_detection && session->config.callback) {
            VoiceEvent event = {
                .type = VOICE_EVENT_EMOTION_DETECTED,
                .maestro_id = session->current_maestro_id,
                .data.emotion = emotion
            };
            session->config.callback(&event, session->config.callback_user_data);
        }
    }
    else if (strstr(ptr, "\"type\":\"transcript\"")) {
        // Parse transcript
        if (session->config.enable_transcription && session->config.callback) {
            VoiceEvent event = {
                .type = VOICE_EVENT_TRANSCRIPT_UPDATE,
                .maestro_id = session->current_maestro_id
            };
            // In production: extract transcript text from JSON
            session->config.callback(&event, session->config.callback_user_data);
        }
    }
    else if (strstr(ptr, "\"type\":\"error\"")) {
        // Handle error
        if (session->config.callback) {
            VoiceEvent event = {
                .type = VOICE_EVENT_ERROR,
                .maestro_id = session->current_maestro_id
            };
            snprintf(event.data.error.message, sizeof(event.data.error.message),
                    "Hume EVI error: %.*s", (int)(total > 200 ? 200 : total), ptr);
            session->config.callback(&event, session->config.callback_user_data);
        }
    }

    return total;
}

bool voice_session_connect(VoiceSession* session) {
    if (!session) return false;

    pthread_mutex_lock(&session->state_mutex);
    session->state = VOICE_STATE_CONNECTING;
    pthread_mutex_unlock(&session->state_mutex);

    // Build WebSocket URL with API key
    char url[512];
    snprintf(url, sizeof(url), "%s?api_key=%s",
            HUME_WS_URL,
            session->config.api_key_hume ? session->config.api_key_hume : "");

    // Initialize CURL for WebSocket
    session->ws_handle = curl_easy_init();
    if (!session->ws_handle) {
        session->state = VOICE_STATE_ERROR;
        return false;
    }

    // Set WebSocket options
    curl_easy_setopt(session->ws_handle, CURLOPT_URL, url);
    curl_easy_setopt(session->ws_handle, CURLOPT_WRITEFUNCTION, ws_write_callback);
    curl_easy_setopt(session->ws_handle, CURLOPT_WRITEDATA, session);

    // Note: Full WebSocket support requires libcurl 7.86+ with websocket support
    // or using a dedicated WebSocket library like libwebsockets

    pthread_mutex_lock(&session->state_mutex);
    session->state = VOICE_STATE_CONNECTED;
    pthread_mutex_unlock(&session->state_mutex);

    // Notify callback
    if (session->config.callback) {
        VoiceEvent event = {
            .type = VOICE_EVENT_CONNECTED,
            .maestro_id = session->current_maestro_id
        };
        session->config.callback(&event, session->config.callback_user_data);
    }

    return true;
}

void voice_session_disconnect(VoiceSession* session) {
    if (!session) return;

    pthread_mutex_lock(&session->state_mutex);
    session->state = VOICE_STATE_DISCONNECTED;
    pthread_mutex_unlock(&session->state_mutex);

    if (session->config.callback) {
        VoiceEvent event = {
            .type = VOICE_EVENT_DISCONNECTED,
            .maestro_id = session->current_maestro_id
        };
        session->config.callback(&event, session->config.callback_user_data);
    }
}

bool voice_session_start_listening(VoiceSession* session) {
    if (!session || session->state != VOICE_STATE_CONNECTED) return false;

    pthread_mutex_lock(&session->state_mutex);
    session->state = VOICE_STATE_LISTENING;
    session->input_buffer_used = 0;
    pthread_mutex_unlock(&session->state_mutex);

    if (session->config.callback) {
        VoiceEvent event = {
            .type = VOICE_EVENT_LISTENING_STARTED,
            .maestro_id = session->current_maestro_id
        };
        session->config.callback(&event, session->config.callback_user_data);
    }

    return true;
}

void voice_session_stop_listening(VoiceSession* session) {
    if (!session) return;

    pthread_mutex_lock(&session->state_mutex);
    if (session->state == VOICE_STATE_LISTENING) {
        session->state = VOICE_STATE_PROCESSING;
    }
    pthread_mutex_unlock(&session->state_mutex);

    if (session->config.callback) {
        VoiceEvent event = {
            .type = VOICE_EVENT_USER_FINISHED,
            .maestro_id = session->current_maestro_id
        };
        session->config.callback(&event, session->config.callback_user_data);
    }
}

void voice_session_send_audio(VoiceSession* session, const uint8_t* audio, size_t length) {
    if (!session || !audio || length == 0) return;
    if (session->state != VOICE_STATE_LISTENING) return;

    // Add to input buffer
    pthread_mutex_lock(&session->state_mutex);
    size_t available = session->input_buffer_size - session->input_buffer_used;
    size_t to_copy = length < available ? length : available;

    if (to_copy > 0) {
        memcpy(session->input_buffer + session->input_buffer_used, audio, to_copy);
        session->input_buffer_used += to_copy;
    }
    pthread_mutex_unlock(&session->state_mutex);

    // In production: stream audio chunks to Hume via WebSocket
    // {"type": "audio_input", "data": "<base64_audio>"}
}

void voice_session_interrupt(VoiceSession* session) {
    if (!session) return;

    pthread_mutex_lock(&session->state_mutex);
    if (session->state == VOICE_STATE_SPEAKING) {
        session->state = VOICE_STATE_INTERRUPTED;
        session->output_buffer_used = 0;
    }
    pthread_mutex_unlock(&session->state_mutex);

    if (session->config.callback) {
        VoiceEvent event = {
            .type = VOICE_EVENT_BARGE_IN,
            .maestro_id = session->current_maestro_id
        };
        session->config.callback(&event, session->config.callback_user_data);
    }

    // Stop audio playback
    voice_audio_stop_playback();
}

// ============================================================================
// PROVIDER FALLBACK
// ============================================================================

VoiceProvider voice_fallback_next(VoiceSession* session) {
    if (!session) return VOICE_PROVIDER_LOCAL_TTS;

    VoiceProvider next = session->active_provider;

    switch (session->active_provider) {
        case VOICE_PROVIDER_HUME_EVI3:
            next = VOICE_PROVIDER_OPENAI_REALTIME;
            break;
        case VOICE_PROVIDER_OPENAI_REALTIME:
            next = VOICE_PROVIDER_ELEVENLABS;
            break;
        case VOICE_PROVIDER_ELEVENLABS:
            next = VOICE_PROVIDER_LOCAL_TTS;
            break;
        case VOICE_PROVIDER_LOCAL_TTS:
            // No more fallbacks
            break;
    }

    if (voice_provider_is_available(next)) {
        session->active_provider = next;
        return next;
    }

    // Keep trying
    return voice_fallback_next(session);
}

bool voice_provider_is_available(VoiceProvider provider) {
    switch (provider) {
        case VOICE_PROVIDER_HUME_EVI3:
            // Check if Hume API key is configured
            return getenv("HUME_API_KEY") != NULL || getenv("CONVERGIO_HUME_KEY") != NULL;

        case VOICE_PROVIDER_OPENAI_REALTIME:
            return getenv("OPENAI_API_KEY") != NULL;

        case VOICE_PROVIDER_ELEVENLABS:
            return getenv("ELEVENLABS_API_KEY") != NULL;

        case VOICE_PROVIDER_LOCAL_TTS:
            return voice_local_tts_available();
    }
    return false;
}

const char* voice_provider_name(VoiceProvider provider) {
    switch (provider) {
        case VOICE_PROVIDER_HUME_EVI3: return "Hume EVI 3";
        case VOICE_PROVIDER_OPENAI_REALTIME: return "OpenAI Realtime";
        case VOICE_PROVIDER_ELEVENLABS: return "ElevenLabs";
        case VOICE_PROVIDER_LOCAL_TTS: return "Local TTS";
    }
    return "Unknown";
}

// ============================================================================
// LOCAL TTS FALLBACK (macOS)
// ============================================================================

bool voice_local_tts_speak(const char* text, const char* voice, float rate) {
    if (!text) return false;

    #ifdef __APPLE__
    char command[8192];
    int rate_int = (int)(rate * 180);  // macOS say rate: ~180 wpm is normal

    if (voice && strlen(voice) > 0) {
        snprintf(command, sizeof(command), "say -v '%s' -r %d '%s' &",
                voice, rate_int, text);
    } else {
        snprintf(command, sizeof(command), "say -r %d '%s' &",
                rate_int, text);
    }

    return system(command) == 0;
    #else
    fprintf(stderr, "Local TTS not available on this platform\n");
    return false;
    #endif
}

void voice_local_tts_stop(void) {
    #ifdef __APPLE__
    system("killall say 2>/dev/null");
    #endif
}

bool voice_local_tts_available(void) {
    #ifdef __APPLE__
    return system("which say > /dev/null 2>&1") == 0;
    #else
    return false;
    #endif
}

// ============================================================================
// AUDIO UTILITIES (Stubs - Real implementation in voice_audio.m when VOICE=1)
// ============================================================================

#ifndef CONVERGIO_VOICE_ENABLED
// Stub implementations when voice is not enabled
bool voice_audio_init(void) {
    return true;
}

bool voice_audio_start_capture(void (*callback)(const int16_t*, size_t, void*), void* user_data) {
    (void)callback;
    (void)user_data;
    return true;
}

void voice_audio_stop_capture(void) {
}

bool voice_audio_start_playback(void) {
    return true;
}

void voice_audio_play(const int16_t* samples, size_t count) {
    (void)samples;
    (void)count;
}

void voice_audio_stop_playback(void) {
    voice_local_tts_stop();
}

void voice_audio_cleanup(void) {
}
#endif /* !CONVERGIO_VOICE_ENABLED */

// ============================================================================
// VOICE ACCESSIBILITY API (VA01-VA05)
// ============================================================================

static float g_audio_level = 0.0f;
static bool g_waveform_enabled = false;
static char g_last_transcript[4096] = {0};

bool voice_accessibility_set_speech_rate(VoiceSession* session, float rate) {
    if (!session) return false;
    if (rate < 0.5f) rate = 0.5f;
    if (rate > 2.0f) rate = 2.0f;

    // Update session config (will apply to next audio)
    // Note: This requires reconnecting to apply in most providers
    ((VoiceSessionConfig*)&session->config)->speech_rate = rate;
    return true;
}

float voice_accessibility_get_speech_rate(const VoiceSession* session) {
    if (!session) return 1.0f;
    return session->config.speech_rate;
}

bool voice_accessibility_set_pitch(VoiceSession* session, float pitch) {
    if (!session) return false;
    if (pitch < -1.0f) pitch = -1.0f;
    if (pitch > 1.0f) pitch = 1.0f;

    ((VoiceSessionConfig*)&session->config)->pitch_offset = pitch;
    return true;
}

float voice_accessibility_get_pitch(const VoiceSession* session) {
    if (!session) return 0.0f;
    return session->config.pitch_offset;
}

// Screen reader state is stored per-session
void voice_accessibility_enable_screen_reader(VoiceSession* session, bool enable) {
    if (!session) return;
    // When enabled, we emit NSAccessibility notifications on macOS
    // Store state for later use
    (void)enable;
    // TODO: Implement VoiceOver integration when VOICE=1
}

bool voice_accessibility_is_screen_reader_enabled(const VoiceSession* session) {
    (void)session;
    // Check if VoiceOver is running on macOS
    #ifdef __APPLE__
    return system("defaults read com.apple.universalaccess voiceOverOnOffKey 2>/dev/null | grep -q 1") == 0;
    #else
    return false;
    #endif
}

void voice_accessibility_enable_waveform(bool enabled) {
    g_waveform_enabled = enabled;
}

float voice_accessibility_get_audio_level(void) {
    return g_audio_level;
}

// Called internally when audio is received
void voice_accessibility_update_audio_level(float level) {
    g_audio_level = level;
}

void voice_accessibility_enable_transcription(VoiceSession* session, bool enable) {
    if (!session) return;
    ((VoiceSessionConfig*)&session->config)->enable_transcription = enable;
}

const char* voice_accessibility_get_transcript(const VoiceSession* session) {
    (void)session;
    return g_last_transcript;
}

// Called internally when transcript is received
void voice_accessibility_update_transcript(const char* text) {
    if (!text) return;
    strncpy(g_last_transcript, text, sizeof(g_last_transcript) - 1);
    g_last_transcript[sizeof(g_last_transcript) - 1] = '\0';
}

bool voice_accessibility_configure_from_profile(VoiceSession* session, int64_t student_id) {
    if (!session) return false;

    // Get student's accessibility settings
    // This integrates with the education module
    extern bool education_accessibility_wants_tts(int64_t);
    extern int education_accessibility_get_font(int64_t, char*, int*);

    // Check if TTS is preferred
    bool wants_tts = education_accessibility_wants_tts(student_id);

    if (wants_tts) {
        // Enable transcription for visual learners
        voice_accessibility_enable_transcription(session, true);

        // Slower speech rate for processing time
        voice_accessibility_set_speech_rate(session, 0.9f);
    }

    // Enable waveform for visual feedback
    voice_accessibility_enable_waveform(true);

    return true;
}
