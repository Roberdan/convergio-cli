/**
 * CONVERGIO EDUCATION - REAL-TIME VOICE INTERACTION SYSTEM
 *
 * Provides fluid, natural voice interaction with all maestri using
 * Hume AI EVI 3 for emotional intelligence and voice synthesis.
 *
 * Architecture: ADR-002-voice-interaction-architecture.md
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef CONVERGIO_VOICE_H
#define CONVERGIO_VOICE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONSTANTS
// ============================================================================

#define VOICE_MAX_AUDIO_BUFFER_SIZE (1024 * 1024)  // 1MB audio buffer
#define VOICE_SAMPLE_RATE 24000                     // Hume EVI 3 sample rate
#define VOICE_CHANNELS 1                            // Mono audio
#define VOICE_BITS_PER_SAMPLE 16
#define VOICE_MAX_MAESTRI 16
#define VOICE_EMOTION_COUNT 9
#define VOICE_MAX_TRANSCRIPT_LENGTH 4096

// ============================================================================
// ENUMS
// ============================================================================

/**
 * Voice provider backends
 */
typedef enum {
    VOICE_PROVIDER_HUME_EVI3,      // Primary: Best emotion detection
    VOICE_PROVIDER_OPENAI_REALTIME, // Fallback 1: Good instruction following
    VOICE_PROVIDER_ELEVENLABS,      // Fallback 2: Voice cloning
    VOICE_PROVIDER_LOCAL_TTS        // Fallback 3: macOS say command
} VoiceProvider;

/**
 * Voice session states
 */
typedef enum {
    VOICE_STATE_DISCONNECTED,
    VOICE_STATE_CONNECTING,
    VOICE_STATE_CONNECTED,
    VOICE_STATE_LISTENING,        // Waiting for user input
    VOICE_STATE_PROCESSING,       // Processing with LLM
    VOICE_STATE_SPEAKING,         // Playing audio response
    VOICE_STATE_INTERRUPTED,      // User interrupted (barge-in)
    VOICE_STATE_ERROR
} VoiceState;

/**
 * Detected emotions (Hume EVI 3)
 */
typedef enum {
    EMOTION_NEUTRAL = 0,
    EMOTION_JOY,
    EMOTION_EXCITEMENT,
    EMOTION_CURIOSITY,
    EMOTION_CONFUSION,
    EMOTION_FRUSTRATION,
    EMOTION_ANXIETY,
    EMOTION_BOREDOM,
    EMOTION_DISTRACTION
} EmotionType;

/**
 * Voice interaction events
 */
typedef enum {
    VOICE_EVENT_CONNECTED,
    VOICE_EVENT_DISCONNECTED,
    VOICE_EVENT_LISTENING_STARTED,
    VOICE_EVENT_USER_SPEAKING,
    VOICE_EVENT_USER_FINISHED,
    VOICE_EVENT_EMOTION_DETECTED,
    VOICE_EVENT_RESPONSE_STARTED,
    VOICE_EVENT_RESPONSE_CHUNK,
    VOICE_EVENT_RESPONSE_FINISHED,
    VOICE_EVENT_BARGE_IN,
    VOICE_EVENT_ERROR,
    VOICE_EVENT_TRANSCRIPT_UPDATE,
    VOICE_EVENT_MAESTRO_CHANGED
} VoiceEventType;

// ============================================================================
// TYPES
// ============================================================================

/**
 * Voice profile for a maestro
 */
typedef struct {
    const char* maestro_id;        // e.g., "euclide-matematica"
    const char* voice_name;        // Display name
    const char* hume_voice_prompt; // Prompt for Hume voice generation
    const char* openai_voice_id;   // OpenAI preset voice (fallback)
    const char* elevenlabs_voice_id; // ElevenLabs voice (fallback)
    const char* local_voice;       // macOS voice name (fallback)
    float default_speed;           // 0.5 - 2.0
    float pitch_offset;            // -1.0 to 1.0
    const char* accent;            // e.g., "greek-italian", "british"
    const char* personality;       // e.g., "calm", "enthusiastic"
} VoiceProfile;

/**
 * Emotion detection result
 */
typedef struct {
    EmotionType primary_emotion;
    float confidence;              // 0.0 - 1.0
    float emotion_scores[VOICE_EMOTION_COUNT];
    int64_t timestamp_ms;
} EmotionResult;

/**
 * Audio chunk for streaming
 */
typedef struct {
    uint8_t* data;
    size_t length;
    int64_t timestamp_ms;
    bool is_final;
} AudioChunk;

/**
 * Transcript update
 */
typedef struct {
    char text[VOICE_MAX_TRANSCRIPT_LENGTH];
    bool is_user;                  // true = user, false = maestro
    bool is_final;                 // false = interim, true = final
    int64_t timestamp_ms;
} TranscriptUpdate;

/**
 * Voice event data
 */
typedef struct {
    VoiceEventType type;
    const char* maestro_id;
    union {
        EmotionResult emotion;
        AudioChunk audio;
        TranscriptUpdate transcript;
        struct {
            int code;
            char message[256];
        } error;
    } data;
} VoiceEvent;

/**
 * Voice event callback
 */
typedef void (*VoiceEventCallback)(const VoiceEvent* event, void* user_data);

/**
 * Voice session configuration
 */
typedef struct {
    const char* api_key_hume;      // Hume EVI 3 API key
    const char* api_key_openai;    // OpenAI API key (fallback)
    const char* api_key_elevenlabs; // ElevenLabs API key (fallback)
    VoiceProvider preferred_provider;
    bool enable_emotion_detection;
    bool enable_barge_in;
    bool enable_transcription;
    float speech_rate;             // User preference: 0.5 - 2.0
    float pitch_offset;            // User preference: -1.0 to 1.0
    const char* language;          // "it", "en", "es", "fr", "de"
    VoiceEventCallback callback;
    void* callback_user_data;
} VoiceSessionConfig;

/**
 * Voice session handle (opaque)
 */
typedef struct VoiceSession VoiceSession;

// ============================================================================
// VOICE SESSION API
// ============================================================================

/**
 * Create a new voice session
 */
VoiceSession* voice_session_create(const VoiceSessionConfig* config);

/**
 * Destroy a voice session
 */
void voice_session_destroy(VoiceSession* session);

/**
 * Connect to voice provider
 * @return true on success, false on failure
 */
bool voice_session_connect(VoiceSession* session);

/**
 * Disconnect from voice provider
 */
void voice_session_disconnect(VoiceSession* session);

/**
 * Get current session state
 */
VoiceState voice_session_get_state(const VoiceSession* session);

/**
 * Start listening for user voice input
 * @return true if listening started
 */
bool voice_session_start_listening(VoiceSession* session);

/**
 * Stop listening and process input
 */
void voice_session_stop_listening(VoiceSession* session);

/**
 * Send audio chunk to voice provider
 * @param audio Raw PCM audio data (24kHz, mono, 16-bit)
 * @param length Length in bytes
 */
void voice_session_send_audio(VoiceSession* session, const uint8_t* audio, size_t length);

/**
 * Interrupt current response (barge-in)
 */
void voice_session_interrupt(VoiceSession* session);

/**
 * Set the active maestro for voice profile
 */
bool voice_session_set_maestro(VoiceSession* session, const char* maestro_id);

/**
 * Get current active maestro
 */
const char* voice_session_get_maestro(const VoiceSession* session);

/**
 * Inject context for the current conversation
 * (Student profile, accessibility settings, current topic)
 */
void voice_session_inject_context(VoiceSession* session, const char* context_json);

// ============================================================================
// VOICE PROFILES API
// ============================================================================

/**
 * Get voice profile for a maestro
 */
const VoiceProfile* voice_profile_get(const char* maestro_id);

/**
 * Get all available voice profiles
 * @param count Output: number of profiles
 * @return Array of profiles (do not free)
 */
const VoiceProfile* voice_profile_get_all(size_t* count);

/**
 * Generate Hume voice prompt from profile
 * @param profile Voice profile
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 */
void voice_profile_generate_prompt(const VoiceProfile* profile, char* buffer, size_t buffer_size);

// ============================================================================
// EMOTION HANDLING API
// ============================================================================

/**
 * Get emotion name as string
 */
const char* emotion_to_string(EmotionType emotion);

/**
 * Parse emotion from Hume response
 */
EmotionResult emotion_parse_hume_response(const char* json);

/**
 * Get response adaptation based on emotion
 * Returns JSON with: speech_rate_modifier, simplify, offer_break, etc.
 */
const char* emotion_get_response_adaptation(EmotionType emotion, float confidence);

/**
 * Check if emotion requires immediate intervention
 * (e.g., high frustration, anxiety)
 */
bool emotion_requires_intervention(EmotionType emotion, float confidence);

// ============================================================================
// AUDIO UTILITIES
// ============================================================================

/**
 * Initialize audio input (microphone)
 */
bool voice_audio_init_input(void);

/**
 * Initialize audio output (speaker)
 */
bool voice_audio_init_output(void);

/**
 * Start capturing microphone audio
 * @param callback Called with each audio chunk (16-bit PCM samples)
 */
bool voice_audio_start_capture(void (*callback)(const int16_t* samples, size_t count, void* user_data), void* user_data);

/**
 * Stop capturing microphone audio
 */
void voice_audio_stop_capture(void);

/**
 * Play audio chunk through speaker
 * @param samples 16-bit PCM audio samples
 * @param count Number of samples
 */
void voice_audio_play(const int16_t* samples, size_t count);

/**
 * Stop audio playback
 */
void voice_audio_stop_playback(void);

/**
 * Cleanup audio system
 */
void voice_audio_cleanup(void);

// ============================================================================
// FALLBACK PROVIDERS
// ============================================================================

/**
 * Try fallback to next provider
 * @return New provider that was activated, or current if no fallback available
 */
VoiceProvider voice_fallback_next(VoiceSession* session);

/**
 * Check if provider is available
 */
bool voice_provider_is_available(VoiceProvider provider);

/**
 * Get provider name
 */
const char* voice_provider_name(VoiceProvider provider);

// ============================================================================
// LOCAL TTS FALLBACK (macOS)
// ============================================================================

/**
 * Speak text using local TTS (macOS say command)
 * Used when all cloud providers are unavailable
 */
bool voice_local_tts_speak(const char* text, const char* voice, float rate);

/**
 * Stop local TTS
 */
void voice_local_tts_stop(void);

/**
 * Check if local TTS is available
 */
bool voice_local_tts_available(void);

// ============================================================================
// VOICE ACCESSIBILITY API (VA01-VA05)
// ============================================================================

/**
 * Set speech rate adjustment (VA01)
 * @param session Voice session
 * @param rate Rate multiplier (0.5 = half speed, 2.0 = double speed)
 * @return true on success
 */
bool voice_accessibility_set_speech_rate(VoiceSession* session, float rate);

/**
 * Get current speech rate
 */
float voice_accessibility_get_speech_rate(const VoiceSession* session);

/**
 * Set pitch adjustment (VA02)
 * @param session Voice session
 * @param pitch Pitch offset (-1.0 to 1.0, where 0 is normal)
 * @return true on success
 */
bool voice_accessibility_set_pitch(VoiceSession* session, float pitch);

/**
 * Get current pitch offset
 */
float voice_accessibility_get_pitch(const VoiceSession* session);

/**
 * Enable screen reader compatibility mode (VA03)
 * Sends events compatible with VoiceOver/JAWS
 */
void voice_accessibility_enable_screen_reader(VoiceSession* session, bool enable);

/**
 * Check if screen reader mode is enabled
 */
bool voice_accessibility_is_screen_reader_enabled(const VoiceSession* session);

/**
 * Enable visual feedback waveform (VA04)
 * @param enabled true to show waveform indicator
 */
void voice_accessibility_enable_waveform(bool enabled);

/**
 * Get current audio level for waveform display
 * @return Audio level 0.0 - 1.0
 */
float voice_accessibility_get_audio_level(void);

/**
 * Enable live transcription display (VA05)
 */
void voice_accessibility_enable_transcription(VoiceSession* session, bool enable);

/**
 * Get last transcript text
 */
const char* voice_accessibility_get_transcript(const VoiceSession* session);

/**
 * Configure voice accessibility from education profile
 * @param student_id Student profile ID
 * @return true on success
 */
bool voice_accessibility_configure_from_profile(VoiceSession* session, int64_t student_id);

#ifdef __cplusplus
}
#endif

#endif // CONVERGIO_VOICE_H
