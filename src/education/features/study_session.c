/**
 * CONVERGIO EDUCATION - STUDY SESSIONS
 *
 * Pomodoro-based study sessions with macOS native notifications,
 * automatic breaks, end-of-session quizzes, and time tracking.
 *
 * Features:
 * - F07: Pomodoro timer (25min work, 5min break)
 * - F08: Native macOS notifications via osascript
 * - F09: Session end quick review quiz
 * - F10: Time tracking per subject
 * - F11: Session statistics and focus scoring
 * - F12: Gamification with XP rewards
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#include "education_features.h"
#include "nous/education.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sqlite3.h>

// ============================================================================
// EXTERNAL DEPENDENCIES
// ============================================================================

extern sqlite3* g_edu_db;
extern char* llm_generate(const char* prompt, const char* system_prompt);
extern int64_t education_session_start(int64_t student_id, const char* session_type,
                                       const char* subject, const char* topic);
extern int education_session_end(int64_t session_id, int xp_earned);
extern int education_xp_add(int64_t student_id, int xp_amount, const char* reason);

// Libretto integration for automatic activity logging
extern int64_t libretto_add_log_entry(int64_t student_id, const char* maestro_id,
                                      const char* activity_type, const char* subject,
                                      const char* topic, int duration_minutes,
                                      const char* notes);

// ============================================================================
// CONSTANTS
// ============================================================================

#define DEFAULT_WORK_DURATION 25      // minutes
#define DEFAULT_BREAK_DURATION 5      // minutes
#define LONG_BREAK_DURATION 15        // minutes after 4 pomodoros
#define POMODOROS_BEFORE_LONG_BREAK 4

#define XP_PER_POMODORO 20
#define XP_BONUS_QUIZ_PERFECT 50
#define XP_BONUS_QUIZ_GOOD 25

// ============================================================================
// GLOBALS (would be better in a session manager struct)
// ============================================================================

static StudySession* g_active_session = NULL;
static pthread_mutex_t g_session_mutex = PTHREAD_MUTEX_INITIALIZER;

// ============================================================================
// NATIVE MACOS NOTIFICATIONS
// ============================================================================

int native_notification(const char* title, const char* message) {
    if (!title || !message) {
        return -1;
    }

    // Use osascript to trigger macOS notification
    char command[1024];
    snprintf(command, sizeof(command),
        "osascript -e 'display notification \"%s\" with title \"%s\" sound name \"Glass\"'",
        message, title);

    int result = system(command);
    return (result == 0) ? 0 : -1;
}

// ============================================================================
// POMODORO TIMER
// ============================================================================

typedef struct {
    int64_t session_id;
    int duration_seconds;
    bool is_break;
} TimerContext;

static void* timer_thread(void* arg) {
    TimerContext* ctx = (TimerContext*)arg;

    // Sleep for the duration
    sleep(ctx->duration_seconds);

    // Send notification
    if (ctx->is_break) {
        native_notification("Break Over!", "Time to get back to studying! 📚");
    } else {
        native_notification("Pomodoro Complete!", "Great work! Take a 5-minute break. 🎉");
    }

    free(ctx);
    return NULL;
}

int pomodoro_timer(int64_t session_id, bool is_break) {
    pthread_mutex_lock(&g_session_mutex);

    if (!g_active_session || g_active_session->id != session_id) {
        pthread_mutex_unlock(&g_session_mutex);
        return -1;
    }

    int duration_minutes = is_break ?
        g_active_session->break_duration_minutes :
        g_active_session->work_duration_minutes;

    // Check if it's time for a long break
    if (is_break && g_active_session->pomodoro_count > 0 &&
        g_active_session->pomodoro_count % POMODOROS_BEFORE_LONG_BREAK == 0) {
        duration_minutes = LONG_BREAK_DURATION;
    }

    pthread_mutex_unlock(&g_session_mutex);

    // Create timer context
    TimerContext* ctx = malloc(sizeof(TimerContext));
    if (!ctx) {
        return -1;
    }

    ctx->session_id = session_id;
    ctx->duration_seconds = duration_minutes * 60;
    ctx->is_break = is_break;

    // Start timer thread
    pthread_t thread;
    if (pthread_create(&thread, NULL, timer_thread, ctx) != 0) {
        free(ctx);
        return -1;
    }

    pthread_detach(thread);

    // Send start notification
    char message[256];
    if (is_break) {
        snprintf(message, sizeof(message),
            "Break time! Relax for %d minutes.", duration_minutes);
        native_notification("Break Started", message);
    } else {
        snprintf(message, sizeof(message),
            "Focus time! Work for %d minutes.", duration_minutes);
        native_notification("Pomodoro Started", message);
    }

    return 0;
}

// ============================================================================
// SESSION END QUIZ
// ============================================================================

char* session_end_quiz(int64_t session_id) {
    pthread_mutex_lock(&g_session_mutex);

    if (!g_active_session || g_active_session->id != session_id) {
        pthread_mutex_unlock(&g_session_mutex);
        return NULL;
    }

    char subject[128];
    char topic[256];
    strncpy(subject, g_active_session->subject, sizeof(subject) - 1);
    strncpy(topic, g_active_session->topic, sizeof(topic) - 1);

    pthread_mutex_unlock(&g_session_mutex);

    // Generate quick review quiz
    char prompt[1024];
    snprintf(prompt, sizeof(prompt),
        "Create a quick 3-question review quiz for:\n"
        "Subject: %s\n"
        "Topic: %s\n\n"
        "Questions should be concise and test key concepts. "
        "Return as JSON array with format:\n"
        "[{\"question\": \"...\", \"type\": \"multiple_choice\", "
        "\"options\": [...], \"correct\": 0}]",
        subject, topic);

    const char* system_prompt =
        "You are a quiz generator. Create engaging, educational questions "
        "that test understanding, not just memorization. Keep questions brief.";

    char* quiz = llm_generate(prompt, system_prompt);

    if (!quiz) {
        // Fallback quiz
        quiz = malloc(2048);
        if (quiz) {
            snprintf(quiz, 2048,
                "[\n"
                "  {\n"
                "    \"question\": \"What was the main concept we studied?\",\n"
                "    \"type\": \"open\"\n"
                "  },\n"
                "  {\n"
                "    \"question\": \"Can you explain one key takeaway?\",\n"
                "    \"type\": \"open\"\n"
                "  },\n"
                "  {\n"
                "    \"question\": \"How would you apply this concept?\",\n"
                "    \"type\": \"open\"\n"
                "  }\n"
                "]");
        }
    }

    return quiz;
}

// ============================================================================
// TIME TRACKING
// ============================================================================

int session_track_time(int64_t student_id, const char* subject, int minutes) {
    if (!g_edu_db || !subject || minutes <= 0) {
        return -1;
    }

    // Upsert time tracking
    const char* sql =
        "INSERT INTO subject_time_tracking (student_id, subject, total_minutes, last_studied) "
        "VALUES (?, ?, ?, strftime('%s','now')) "
        "ON CONFLICT(student_id, subject) DO UPDATE SET "
        "total_minutes = total_minutes + ?, last_studied = strftime('%s','now')";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, subject, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, minutes);
    sqlite3_bind_int(stmt, 4, minutes);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ============================================================================
// SESSION MANAGEMENT
// ============================================================================

int64_t study_command_handler(int64_t student_id, const char* subject, const char* topic) {
    if (!subject || !topic) {
        return -1;
    }

    pthread_mutex_lock(&g_session_mutex);

    // Check if there's already an active session
    if (g_active_session) {
        fprintf(stderr, "Error: Session already active. End current session first.\n");
        pthread_mutex_unlock(&g_session_mutex);
        return -1;
    }

    // Create session in database
    int64_t session_id = education_session_start(student_id, "study", subject, topic);
    if (session_id < 0) {
        pthread_mutex_unlock(&g_session_mutex);
        return -1;
    }

    // Create active session
    g_active_session = calloc(1, sizeof(StudySession));
    if (!g_active_session) {
        pthread_mutex_unlock(&g_session_mutex);
        return -1;
    }

    g_active_session->id = session_id;
    g_active_session->student_id = student_id;
    strncpy(g_active_session->subject, subject, sizeof(g_active_session->subject) - 1);
    strncpy(g_active_session->topic, topic, sizeof(g_active_session->topic) - 1);
    g_active_session->started_at = time(NULL);
    g_active_session->current_pomodoro_start = time(NULL);
    g_active_session->pomodoro_count = 0;
    g_active_session->breaks_taken = 0;
    g_active_session->work_duration_minutes = DEFAULT_WORK_DURATION;
    g_active_session->break_duration_minutes = DEFAULT_BREAK_DURATION;
    g_active_session->state = SESSION_WORKING;

    pthread_mutex_unlock(&g_session_mutex);

    // Start first pomodoro
    pomodoro_timer(session_id, false);

    printf("\n✓ Study session started!\n");
    printf("Subject: %s\n", subject);
    printf("Topic: %s\n", topic);
    printf("Pomodoro: %d minutes work, %d minutes break\n",
           DEFAULT_WORK_DURATION, DEFAULT_BREAK_DURATION);
    printf("\nFocus on your studies. You'll get a notification when time's up!\n\n");

    return session_id;
}

StudySession* study_session_get_active(int64_t student_id) {
    pthread_mutex_lock(&g_session_mutex);

    if (!g_active_session || g_active_session->student_id != student_id) {
        pthread_mutex_unlock(&g_session_mutex);
        return NULL;
    }

    // Create a copy
    StudySession* copy = malloc(sizeof(StudySession));
    if (copy) {
        memcpy(copy, g_active_session, sizeof(StudySession));
    }

    pthread_mutex_unlock(&g_session_mutex);
    return copy;
}

int study_session_end(int64_t session_id, const StudySessionStats* stats) {
    pthread_mutex_lock(&g_session_mutex);

    if (!g_active_session || g_active_session->id != session_id) {
        pthread_mutex_unlock(&g_session_mutex);
        return -1;
    }

    int64_t student_id = g_active_session->student_id;
    char subject[128];
    strncpy(subject, g_active_session->subject, sizeof(subject) - 1);

    time_t now = time(NULL);
    int duration_minutes = (now - g_active_session->started_at) / 60;

    // Track time
    session_track_time(student_id, subject, duration_minutes);

    // Log to Libretto (LB12 - automatic study session logging)
    char notes[256];
    snprintf(notes, sizeof(notes), "%d pomodori completati, %d min focus",
             g_active_session->pomodoro_count, duration_minutes);
    libretto_add_log_entry(student_id, NULL, "study", subject,
                           g_active_session->topic, duration_minutes, notes);

    // Calculate XP
    int xp_earned = g_active_session->pomodoro_count * XP_PER_POMODORO;
    if (stats && stats->quiz_score >= 90) {
        xp_earned += XP_BONUS_QUIZ_PERFECT;
    } else if (stats && stats->quiz_score >= 70) {
        xp_earned += XP_BONUS_QUIZ_GOOD;
    }

    // End session in database
    education_session_end(session_id, xp_earned);

    // Award XP
    education_xp_add(student_id, xp_earned, "study_session");

    // Free active session
    free(g_active_session);
    g_active_session = NULL;

    pthread_mutex_unlock(&g_session_mutex);

    printf("\n✓ Study session ended!\n");
    printf("Duration: %d minutes\n", duration_minutes);
    printf("Pomodoros: %d\n", stats ? stats->pomodoros_completed : 0);
    printf("XP Earned: %d\n", xp_earned);
    printf("\nGreat work! Keep up the momentum! 🚀\n\n");

    return 0;
}

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

void study_session_free(StudySession* session) {
    free(session);
}

void study_session_stats_free(StudySessionStats* stats) {
    if (!stats) return;
    free(stats->summary);
    free(stats);
}
