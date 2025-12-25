/**
 * Voice History Implementation
 *
 * SQLite-backed persistence for voice transcription history
 * with emotion analysis and session management.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/voice_history.h"
#include "nous/nous.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uuid/uuid.h>
#include <sys/stat.h>
#include <errno.h>

// ============================================================================
// PRIVATE STATE
// ============================================================================

static sqlite3* g_voice_db = NULL;
static bool g_initialized = false;
static char g_db_path[512] = {0};

// Emotion name lookup table
static const char* EMOTION_NAMES[VOICE_EMOTION_COUNT] = {
    "neutral",
    "confusion",
    "frustration",
    "anxiety",
    "boredom",
    "excitement",
    "curiosity",
    "joy",
    "sadness",
    "anger"
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static void generate_uuid(char* out) {
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, out);
}

static int ensure_directory(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0 && errno != EEXIST) {
            return -1;
        }
    }
    return 0;
}

static int init_database(void) {
    const char* schema =
        // Voice sessions table
        "CREATE TABLE IF NOT EXISTS voice_sessions ("
        "  session_id TEXT PRIMARY KEY,"
        "  agent_name TEXT NOT NULL,"
        "  start_time INTEGER NOT NULL,"
        "  end_time INTEGER,"
        "  total_exchanges INTEGER DEFAULT 0,"
        "  total_duration_ms INTEGER DEFAULT 0,"
        "  dominant_emotion INTEGER DEFAULT 0,"
        "  avg_speech_clarity REAL DEFAULT 0.0,"
        "  avg_background_noise REAL DEFAULT 0.0,"
        "  summary TEXT"
        ");"

        // Voice transcripts table
        "CREATE TABLE IF NOT EXISTS voice_transcripts ("
        "  id TEXT PRIMARY KEY,"
        "  session_id TEXT NOT NULL,"
        "  agent_name TEXT NOT NULL,"
        "  user_transcript TEXT,"
        "  assistant_response TEXT,"
        "  audio_response_id TEXT,"
        "  timestamp INTEGER NOT NULL,"
        "  duration_ms INTEGER DEFAULT 0,"
        "  response_latency_ms INTEGER DEFAULT 0,"
        "  dominant_emotion INTEGER DEFAULT 0,"
        "  emotion_confidence REAL DEFAULT 0.0,"
        "  speech_clarity REAL DEFAULT 0.0,"
        "  background_noise REAL DEFAULT 0.0,"
        "  language TEXT,"
        "  topic TEXT,"
        "  intent TEXT,"
        "  is_command INTEGER DEFAULT 0,"
        "  emotion_data TEXT," // JSON blob for full emotion analysis
        "  FOREIGN KEY (session_id) REFERENCES voice_sessions(session_id)"
        ");"

        // Session topics junction table
        "CREATE TABLE IF NOT EXISTS session_topics ("
        "  session_id TEXT NOT NULL,"
        "  topic TEXT NOT NULL,"
        "  count INTEGER DEFAULT 1,"
        "  PRIMARY KEY (session_id, topic),"
        "  FOREIGN KEY (session_id) REFERENCES voice_sessions(session_id)"
        ");"

        // Emotion counts per session
        "CREATE TABLE IF NOT EXISTS session_emotions ("
        "  session_id TEXT NOT NULL,"
        "  emotion INTEGER NOT NULL,"
        "  count INTEGER DEFAULT 0,"
        "  PRIMARY KEY (session_id, emotion),"
        "  FOREIGN KEY (session_id) REFERENCES voice_sessions(session_id)"
        ");"

        // Indexes for common queries
        "CREATE INDEX IF NOT EXISTS idx_transcripts_session ON voice_transcripts(session_id);"
        "CREATE INDEX IF NOT EXISTS idx_transcripts_timestamp ON voice_transcripts(timestamp);"
        "CREATE INDEX IF NOT EXISTS idx_transcripts_agent ON voice_transcripts(agent_name);"
        "CREATE INDEX IF NOT EXISTS idx_sessions_agent ON voice_sessions(agent_name);"
        "CREATE INDEX IF NOT EXISTS idx_sessions_time ON voice_sessions(start_time);";

    char* err_msg = NULL;
    int rc = sqlite3_exec(g_voice_db, schema, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create voice history schema: %s", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    return 0;
}

// ============================================================================
// PUBLIC API
// ============================================================================

int voice_history_init(void) {
    if (g_initialized) {
        return 0;
    }

    // Get home directory and create voice history path
    const char* home = getenv("HOME");
    if (!home) {
        LOG_ERROR("HOME environment variable not set");
        return -1;
    }

    char base_path[256];
    snprintf(base_path, sizeof(base_path), "%s/.convergio", home);
    if (ensure_directory(base_path) != 0) {
        LOG_ERROR("Failed to create .convergio directory");
        return -1;
    }

    char voice_path[256];
    snprintf(voice_path, sizeof(voice_path), "%s/.convergio/voice_history", home);
    if (ensure_directory(voice_path) != 0) {
        LOG_ERROR("Failed to create voice_history directory");
        return -1;
    }

    // Open database
    snprintf(g_db_path, sizeof(g_db_path), "%s/voice_history.db", voice_path);

    int rc = sqlite3_open(g_db_path, &g_voice_db);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to open voice history database: %s", sqlite3_errmsg(g_voice_db));
        return -1;
    }

    // Enable WAL mode for better concurrency
    sqlite3_exec(g_voice_db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(g_voice_db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);

    // Initialize schema
    if (init_database() != 0) {
        sqlite3_close(g_voice_db);
        g_voice_db = NULL;
        return -1;
    }

    g_initialized = true;
    LOG_INFO("Voice history system initialized: %s", g_db_path);
    return 0;
}

void voice_history_shutdown(void) {
    if (!g_initialized) {
        return;
    }

    if (g_voice_db) {
        sqlite3_close(g_voice_db);
        g_voice_db = NULL;
    }

    g_initialized = false;
    LOG_INFO("Voice history system shutdown");
}

int voice_session_start(const char* agent_name, char* out_session_id) {
    if (!g_initialized || !agent_name || !out_session_id) {
        return -1;
    }

    generate_uuid(out_session_id);

    const char* sql =
        "INSERT INTO voice_sessions (session_id, agent_name, start_time) "
        "VALUES (?, ?, ?);";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_voice_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to prepare session start: %s", sqlite3_errmsg(g_voice_db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, out_session_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, agent_name, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        LOG_ERROR("Failed to insert session: %s", sqlite3_errmsg(g_voice_db));
        return -1;
    }

    // Initialize emotion counts for this session
    for (int i = 0; i < VOICE_EMOTION_COUNT; i++) {
        const char* emotion_sql =
            "INSERT INTO session_emotions (session_id, emotion, count) VALUES (?, ?, 0);";
        sqlite3_stmt* emotion_stmt;
        if (sqlite3_prepare_v2(g_voice_db, emotion_sql, -1, &emotion_stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(emotion_stmt, 1, out_session_id, -1, SQLITE_STATIC);
            sqlite3_bind_int(emotion_stmt, 2, i);
            sqlite3_step(emotion_stmt);
            sqlite3_finalize(emotion_stmt);
        }
    }

    LOG_INFO("Voice session started: %s with agent %s", out_session_id, agent_name);
    return 0;
}

int voice_session_end(const char* session_id) {
    if (!g_initialized || !session_id) {
        return -1;
    }

    const char* sql =
        "UPDATE voice_sessions SET end_time = ? WHERE session_id = ?;";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_voice_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)time(NULL));
    sqlite3_bind_text(stmt, 2, session_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return -1;
    }

    LOG_INFO("Voice session ended: %s", session_id);
    return 0;
}

int voice_history_save(const VoiceTranscriptEntry* entry) {
    if (!g_initialized || !entry) {
        return -1;
    }

    const char* sql =
        "INSERT INTO voice_transcripts ("
        "  id, session_id, agent_name, user_transcript, assistant_response, "
        "  audio_response_id, timestamp, duration_ms, response_latency_ms, "
        "  dominant_emotion, emotion_confidence, speech_clarity, background_noise, "
        "  language, topic, intent, is_command"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_voice_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to prepare transcript insert: %s", sqlite3_errmsg(g_voice_db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, entry->id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entry->session_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, entry->agent_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, entry->user_transcript, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, entry->assistant_response, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, entry->audio_response_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 7, (sqlite3_int64)entry->timestamp);
    sqlite3_bind_int(stmt, 8, entry->duration_ms);
    sqlite3_bind_int(stmt, 9, entry->response_latency_ms);
    sqlite3_bind_int(stmt, 10, (int)entry->user_emotion.dominant_emotion);
    sqlite3_bind_double(stmt, 11, entry->user_emotion.dominant_confidence);
    sqlite3_bind_double(stmt, 12, entry->speech_clarity);
    sqlite3_bind_double(stmt, 13, entry->background_noise);
    sqlite3_bind_text(stmt, 14, entry->language, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 15, entry->topic, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 16, entry->intent, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 17, entry->is_command ? 1 : 0);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        LOG_ERROR("Failed to insert transcript: %s", sqlite3_errmsg(g_voice_db));
        return -1;
    }

    // Update session statistics
    const char* update_sql =
        "UPDATE voice_sessions SET "
        "  total_exchanges = total_exchanges + 1, "
        "  total_duration_ms = total_duration_ms + ? "
        "WHERE session_id = ?;";

    sqlite3_stmt* update_stmt;
    if (sqlite3_prepare_v2(g_voice_db, update_sql, -1, &update_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(update_stmt, 1, entry->duration_ms);
        sqlite3_bind_text(update_stmt, 2, entry->session_id, -1, SQLITE_STATIC);
        sqlite3_step(update_stmt);
        sqlite3_finalize(update_stmt);
    }

    // Update emotion counts
    const char* emotion_sql =
        "UPDATE session_emotions SET count = count + 1 "
        "WHERE session_id = ? AND emotion = ?;";

    sqlite3_stmt* emotion_stmt;
    if (sqlite3_prepare_v2(g_voice_db, emotion_sql, -1, &emotion_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(emotion_stmt, 1, entry->session_id, -1, SQLITE_STATIC);
        sqlite3_bind_int(emotion_stmt, 2, (int)entry->user_emotion.dominant_emotion);
        sqlite3_step(emotion_stmt);
        sqlite3_finalize(emotion_stmt);
    }

    return 0;
}

int voice_history_save_simple(
    const char* session_id,
    const char* agent_name,
    const char* user_text,
    const char* assistant_text,
    int duration_ms,
    const char* language
) {
    if (!g_initialized || !session_id || !agent_name) {
        return -1;
    }

    VoiceTranscriptEntry entry = {0};
    generate_uuid((char*)entry.id);
    strncpy((char*)entry.session_id, session_id, sizeof(entry.session_id) - 1);
    strncpy((char*)entry.agent_name, agent_name, sizeof(entry.agent_name) - 1);

    entry.user_transcript = (char*)user_text;
    entry.assistant_response = (char*)assistant_text;
    entry.timestamp = time(NULL);
    entry.duration_ms = duration_ms;
    entry.language = (char*)language;

    // Default emotion analysis (neutral)
    entry.user_emotion.dominant_emotion = VOICE_EMOTION_NEUTRAL;
    entry.user_emotion.dominant_confidence = 1.0f;

    return voice_history_save(&entry);
}

int voice_history_export_to_chat(
    const char* session_id,
    char*** out_messages,
    char*** out_roles,
    size_t* out_count
) {
    if (!g_initialized || !session_id || !out_messages || !out_roles || !out_count) {
        return -1;
    }

    const char* sql =
        "SELECT user_transcript, assistant_response FROM voice_transcripts "
        "WHERE session_id = ? ORDER BY timestamp ASC;";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_voice_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);

    // First pass: count entries
    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_text(stmt, 0)) count++;
        if (sqlite3_column_text(stmt, 1)) count++;
    }

    if (count == 0) {
        sqlite3_finalize(stmt);
        *out_messages = NULL;
        *out_roles = NULL;
        *out_count = 0;
        return 0;
    }

    // Allocate arrays
    *out_messages = malloc(count * sizeof(char*));
    *out_roles = malloc(count * sizeof(char*));
    if (!*out_messages || !*out_roles) {
        sqlite3_finalize(stmt);
        return -1;
    }

    // Reset and second pass: populate
    sqlite3_reset(stmt);
    size_t idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* user = (const char*)sqlite3_column_text(stmt, 0);
        const char* assistant = (const char*)sqlite3_column_text(stmt, 1);

        if (user) {
            (*out_messages)[idx] = strdup(user);
            (*out_roles)[idx] = strdup("user");
            idx++;
        }
        if (assistant) {
            (*out_messages)[idx] = strdup(assistant);
            (*out_roles)[idx] = strdup("assistant");
            idx++;
        }
    }

    sqlite3_finalize(stmt);
    *out_count = idx;
    return 0;
}

const char* voice_emotion_name(VoiceEmotion emotion) {
    if (emotion >= 0 && emotion < VOICE_EMOTION_COUNT) {
        return EMOTION_NAMES[emotion];
    }
    return "unknown";
}

int voice_history_get_stats(VoiceHistoryStats* stats) {
    if (!g_initialized || !stats) {
        return -1;
    }

    memset(stats, 0, sizeof(VoiceHistoryStats));

    // Get session count
    const char* session_sql = "SELECT COUNT(*) FROM voice_sessions;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_voice_db, session_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats->total_sessions = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Get transcript count and duration
    const char* transcript_sql =
        "SELECT COUNT(*), SUM(duration_ms) FROM voice_transcripts;";
    if (sqlite3_prepare_v2(g_voice_db, transcript_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats->total_transcripts = sqlite3_column_int(stmt, 0);
            stats->total_duration_seconds = sqlite3_column_int(stmt, 1) / 1000;
        }
        sqlite3_finalize(stmt);
    }

    // Get time range
    const char* time_sql =
        "SELECT MIN(start_time), MAX(start_time) FROM voice_sessions;";
    if (sqlite3_prepare_v2(g_voice_db, time_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats->first_session = (time_t)sqlite3_column_int64(stmt, 0);
            stats->last_session = (time_t)sqlite3_column_int64(stmt, 1);
        }
        sqlite3_finalize(stmt);
    }

    // Get most used agent
    const char* agent_sql =
        "SELECT agent_name, COUNT(*) as cnt FROM voice_sessions "
        "GROUP BY agent_name ORDER BY cnt DESC LIMIT 1;";
    if (sqlite3_prepare_v2(g_voice_db, agent_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* agent = (const char*)sqlite3_column_text(stmt, 0);
            if (agent) {
                strncpy(stats->most_used_agent, agent, sizeof(stats->most_used_agent) - 1);
            }
        }
        sqlite3_finalize(stmt);
    }

    // Get most common emotion
    const char* emotion_sql =
        "SELECT dominant_emotion, COUNT(*) as cnt FROM voice_transcripts "
        "GROUP BY dominant_emotion ORDER BY cnt DESC LIMIT 1;";
    if (sqlite3_prepare_v2(g_voice_db, emotion_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats->most_common_emotion = (VoiceEmotion)sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return 0;
}

void voice_transcript_free(VoiceTranscriptEntry* entry) {
    if (!entry) return;

    free(entry->user_transcript);
    free(entry->assistant_response);
    free(entry->audio_response_id);
    free(entry->language);
    free(entry->topic);
    free(entry->intent);

    memset(entry, 0, sizeof(VoiceTranscriptEntry));
}

void voice_session_metadata_free(VoiceSessionMetadata* metadata) {
    if (!metadata) return;

    free(metadata->summary);
    for (int i = 0; i < metadata->topic_count; i++) {
        free(metadata->key_topics[i]);
    }

    memset(metadata, 0, sizeof(VoiceSessionMetadata));
}
