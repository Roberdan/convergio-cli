/**
 * Voice History System - Voice Transcription to Chat History
 *
 * Persists voice interactions with full metadata including:
 * - Transcripts (interim and final)
 * - Emotion detection results
 * - Session metadata
 * - Audio quality metrics
 *
 * Storage: ~/.convergio/voice_history/
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef NOUS_VOICE_HISTORY_H
#define NOUS_VOICE_HISTORY_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// ============================================================================
// VOICE EMOTION TYPES
// ============================================================================

typedef enum {
    VOICE_EMOTION_NEUTRAL = 0,
    VOICE_EMOTION_CONFUSION,
    VOICE_EMOTION_FRUSTRATION,
    VOICE_EMOTION_ANXIETY,
    VOICE_EMOTION_BOREDOM,
    VOICE_EMOTION_EXCITEMENT,
    VOICE_EMOTION_CURIOSITY,
    VOICE_EMOTION_JOY,
    VOICE_EMOTION_SADNESS,
    VOICE_EMOTION_ANGER,
    VOICE_EMOTION_COUNT
} VoiceEmotion;

typedef struct {
    VoiceEmotion emotion;
    float confidence;          // 0.0 - 1.0
} EmotionScore;

typedef struct {
    EmotionScore scores[VOICE_EMOTION_COUNT];
    VoiceEmotion dominant_emotion;
    float dominant_confidence;
} EmotionAnalysis;

// ============================================================================
// VOICE TRANSCRIPT ENTRY
// ============================================================================

typedef struct {
    char id[64];               // Unique transcript ID (UUID)
    char session_id[64];       // Voice session ID
    char agent_name[64];       // Agent involved (e.g., "ali", "maestro-da-vinci")

    // Transcript content
    char* user_transcript;     // User's spoken words
    char* assistant_response;  // AI response text
    char* audio_response_id;   // Reference to audio response if any

    // Timing
    time_t timestamp;          // When this exchange occurred
    int duration_ms;           // Duration of user speech in milliseconds
    int response_latency_ms;   // Time to first response token

    // Emotion analysis
    EmotionAnalysis user_emotion;
    bool emotion_triggered_adaptation;  // Did emotion cause behavior change?

    // Quality metrics
    float speech_clarity;      // 0.0 - 1.0
    float background_noise;    // 0.0 - 1.0 (higher = noisier)
    char* language;            // Detected language (e.g., "it-IT", "en-US")

    // Context
    char* topic;               // Conversation topic if detected
    char* intent;              // User intent classification
    bool is_command;           // Was this a command vs conversation?
} VoiceTranscriptEntry;

// ============================================================================
// VOICE SESSION METADATA
// ============================================================================

typedef struct {
    char session_id[64];
    char agent_name[64];
    time_t start_time;
    time_t end_time;
    int total_exchanges;
    int total_duration_ms;

    // Aggregated emotion distribution
    int emotion_counts[VOICE_EMOTION_COUNT];
    VoiceEmotion session_dominant_emotion;

    // Session quality
    float avg_speech_clarity;
    float avg_background_noise;

    // Session summary (generated)
    char* summary;
    char* key_topics[16];
    int topic_count;
} VoiceSessionMetadata;

// ============================================================================
// VOICE HISTORY API
// ============================================================================

/**
 * Initialize voice history system
 * Creates directories and database tables if needed
 * @return 0 on success, -1 on error
 */
int voice_history_init(void);

/**
 * Shutdown voice history system
 */
void voice_history_shutdown(void);

/**
 * Start a new voice session
 * @param agent_name Agent for this session
 * @param out_session_id Buffer to receive session ID (min 64 bytes)
 * @return 0 on success, -1 on error
 */
int voice_session_start(const char* agent_name, char* out_session_id);

/**
 * End a voice session and generate summary
 * @param session_id Session to end
 * @return 0 on success, -1 on error
 */
int voice_session_end(const char* session_id);

/**
 * Save a voice transcript entry
 * @param entry Transcript entry to save
 * @return 0 on success, -1 on error
 */
int voice_history_save(const VoiceTranscriptEntry* entry);

/**
 * Save transcript with automatic emotion analysis
 * @param session_id Current session ID
 * @param agent_name Agent name
 * @param user_text User's spoken text
 * @param assistant_text Assistant's response
 * @param duration_ms Speech duration
 * @param language Detected language
 * @return 0 on success, -1 on error
 */
int voice_history_save_simple(
    const char* session_id,
    const char* agent_name,
    const char* user_text,
    const char* assistant_text,
    int duration_ms,
    const char* language
);

/**
 * Load voice history for a session
 * @param session_id Session ID
 * @param out_entries Array to receive entries (caller allocates)
 * @param max_entries Maximum entries to load
 * @param out_count Actual count loaded
 * @return 0 on success, -1 on error
 */
int voice_history_load_session(
    const char* session_id,
    VoiceTranscriptEntry* out_entries,
    size_t max_entries,
    size_t* out_count
);

/**
 * Search voice history by text
 * @param query Search query
 * @param max_results Maximum results
 * @param out_entries Array to receive entries
 * @param out_count Actual count found
 * @return 0 on success, -1 on error
 */
int voice_history_search(
    const char* query,
    size_t max_results,
    VoiceTranscriptEntry* out_entries,
    size_t* out_count
);

/**
 * Load recent voice sessions
 * @param max_sessions Maximum sessions to load
 * @param out_sessions Array to receive session metadata
 * @param out_count Actual count loaded
 * @return 0 on success, -1 on error
 */
int voice_history_load_recent_sessions(
    size_t max_sessions,
    VoiceSessionMetadata* out_sessions,
    size_t* out_count
);

/**
 * Get session metadata
 * @param session_id Session ID
 * @param out_metadata Output metadata
 * @return 0 on success, -1 on error
 */
int voice_session_get_metadata(
    const char* session_id,
    VoiceSessionMetadata* out_metadata
);

/**
 * Export voice history to chat format
 * Converts voice transcripts to standard chat history format
 * for integration with memory system
 * @param session_id Session to export
 * @param out_messages Array of message strings (caller frees each)
 * @param out_roles Array of role strings ("user" or "assistant")
 * @param out_count Number of messages
 * @return 0 on success, -1 on error
 */
int voice_history_export_to_chat(
    const char* session_id,
    char*** out_messages,
    char*** out_roles,
    size_t* out_count
);

/**
 * Generate summary for voice session using LLM
 * @param session_id Session to summarize
 * @param out_summary Output summary (caller frees)
 * @return 0 on success, -1 on error
 */
int voice_session_generate_summary(
    const char* session_id,
    char** out_summary
);

/**
 * Get emotion distribution for session
 * @param session_id Session ID
 * @param out_distribution Array of VOICE_EMOTION_COUNT floats (percentages)
 * @return 0 on success, -1 on error
 */
int voice_session_emotion_distribution(
    const char* session_id,
    float* out_distribution
);

/**
 * Free a transcript entry's dynamic memory
 */
void voice_transcript_free(VoiceTranscriptEntry* entry);

/**
 * Free session metadata dynamic memory
 */
void voice_session_metadata_free(VoiceSessionMetadata* metadata);

/**
 * Get emotion name string
 */
const char* voice_emotion_name(VoiceEmotion emotion);

/**
 * Get statistics
 */
typedef struct {
    int total_sessions;
    int total_transcripts;
    int total_duration_seconds;
    time_t first_session;
    time_t last_session;
    VoiceEmotion most_common_emotion;
    char most_used_agent[64];
} VoiceHistoryStats;

int voice_history_get_stats(VoiceHistoryStats* stats);

#endif // NOUS_VOICE_HISTORY_H
