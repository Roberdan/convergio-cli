/**
 * CONVERGIO EDUCATION - FSRS (Free Spaced Repetition Scheduler)
 *
 * Implementation of the FSRS algorithm (2024 version) for optimal
 * spaced repetition scheduling based on Duolingo's research.
 *
 * Algorithm: Stability = S * (11^D - 1) * e^(k*(1-R)) * e^(0.2*t) * e^(-0.1*lapse)
 *
 * Phase: FASE 11 - Learning Science
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#include <math.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define FSRS_INITIAL_STABILITY 1.0f
#define FSRS_INITIAL_DIFFICULTY 0.3f
#define FSRS_DESIRED_RETENTION 0.9f
#define FSRS_K_FACTOR 19.0f // Controls stability growth rate

// Quality ratings (1-5)
#define QUALITY_FORGOT 1
#define QUALITY_HARD 2
#define QUALITY_OKAY 3
#define QUALITY_GOOD 4
#define QUALITY_PERFECT 5

// ============================================================================
// TYPES
// ============================================================================

typedef struct {
    int64_t card_id;
    int64_t student_id;
    char* topic_id;
    char* front; // Question
    char* back;  // Answer

    float stability;      // How long memory lasts (in days)
    float difficulty;     // 0.0 (easy) to 1.0 (hard)
    float retrievability; // Current probability of recall

    int reps;   // Total review count
    int lapses; // Times forgotten
    time_t last_review;
    time_t next_review;

    time_t created_at;
} FSRSCard;

typedef struct {
    FSRSCard* cards;
    int count;
    int capacity;
} FSRSCardList;

// ============================================================================
// EXTERNAL DEPENDENCIES
// ============================================================================

extern sqlite3* g_edu_db;

// ============================================================================
// FSRS ALGORITHM CORE
// ============================================================================

/**
 * Calculate retrievability (probability of recall) based on elapsed time
 * R(t) = (1 + t/S)^(-1/w) where w is the decay factor
 */
static float fsrs_retrievability(float stability, float days_elapsed) {
    if (stability <= 0 || days_elapsed < 0)
        return 1.0f;

    // Power law decay with stability as the time constant
    const float w = 0.95f; // Decay sharpness
    float r = powf(1.0f + days_elapsed / (9.0f * stability), -1.0f / w);

    return fminf(fmaxf(r, 0.0f), 1.0f);
}

/**
 * Calculate new stability based on review quality
 * Using FSRS-5 algorithm parameters
 */
static float fsrs_new_stability(float S, float D, float R, int quality, int lapses) {
    if (quality == QUALITY_FORGOT) {
        // Stability after forgetting (significantly reduced)
        return S * 0.3f * powf(11.0f, D - 1.0f);
    }

    // FSRS formula for successful recall
    float k = FSRS_K_FACTOR;
    float base = 11.0f;

    float stability = S * (powf(base, D) - 1.0f) * expf(k * (1.0f - R)) * expf(0.2f * S) *
                      expf(-0.1f * (float)lapses);

    // Quality modifiers
    switch (quality) {
    case QUALITY_HARD:
        stability *= 0.6f;
        break;
    case QUALITY_OKAY:
        stability *= 0.85f;
        break;
    case QUALITY_GOOD:
        // No modifier
        break;
    case QUALITY_PERFECT:
        stability *= 1.3f;
        break;
    }

    // Clamp to reasonable bounds (1 hour to 3 years)
    return fminf(fmaxf(stability, 0.04f), 1095.0f);
}

/**
 * Calculate new difficulty based on review quality
 */
static float fsrs_new_difficulty(float D, int quality) {
    float delta = 0.0f;

    switch (quality) {
    case QUALITY_FORGOT:
        delta = 0.1f; // Harder
        break;
    case QUALITY_HARD:
        delta = 0.05f;
        break;
    case QUALITY_OKAY:
        delta = 0.0f;
        break;
    case QUALITY_GOOD:
        delta = -0.03f;
        break;
    case QUALITY_PERFECT:
        delta = -0.07f; // Easier
        break;
    }

    // Mean reversion toward 0.3
    float new_D = D + delta + 0.05f * (0.3f - D);

    return fminf(fmaxf(new_D, 0.0f), 1.0f);
}

/**
 * Calculate optimal interval until next review
 * Based on desired retention rate (default 90%)
 */
static int fsrs_next_interval(float stability, float desired_retention) {
    // Solve R(t) = desired_retention for t
    // t = S * ((1/R)^w - 1) * 9
    const float w = 0.95f;

    float days = stability * (powf(1.0f / desired_retention, w) - 1.0f) * 9.0f;

    // Minimum 1 hour, maximum 365 days
    int hours = (int)(days * 24.0f);
    if (hours < 1)
        hours = 1;
    if (hours > 365 * 24)
        hours = 365 * 24;

    return hours;
}

// ============================================================================
// DATABASE OPERATIONS
// ============================================================================

/**
 * Create FSRS tables if they don't exist
 */
int fsrs_init_db(void) {
    if (!g_edu_db)
        return -1;

    const char* sql = "CREATE TABLE IF NOT EXISTS fsrs_cards ("
                      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "  student_id INTEGER NOT NULL,"
                      "  topic_id TEXT NOT NULL,"
                      "  front TEXT NOT NULL,"
                      "  back TEXT NOT NULL,"
                      "  stability REAL DEFAULT 1.0,"
                      "  difficulty REAL DEFAULT 0.3,"
                      "  reps INTEGER DEFAULT 0,"
                      "  lapses INTEGER DEFAULT 0,"
                      "  last_review INTEGER,"
                      "  next_review INTEGER,"
                      "  created_at INTEGER DEFAULT (strftime('%s', 'now')),"
                      "  FOREIGN KEY (student_id) REFERENCES student_profiles(id)"
                      ");"
                      "CREATE INDEX IF NOT EXISTS idx_fsrs_student ON fsrs_cards(student_id);"
                      "CREATE INDEX IF NOT EXISTS idx_fsrs_next ON fsrs_cards(next_review);";

    char* err = NULL;
    if (sqlite3_exec(g_edu_db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[FSRS] Init failed: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    return 0;
}

/**
 * Add a new flashcard for spaced repetition
 */
int64_t fsrs_add_card(int64_t student_id, const char* topic_id, const char* front,
                      const char* back) {
    if (!g_edu_db || !topic_id || !front || !back)
        return -1;

    const char* sql = "INSERT INTO fsrs_cards (student_id, topic_id, front, back, next_review) "
                      "VALUES (?, ?, ?, ?, strftime('%s', 'now'))";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, topic_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, front, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, back, -1, SQLITE_STATIC);

    int64_t card_id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        card_id = sqlite3_last_insert_rowid(g_edu_db);
    }

    sqlite3_finalize(stmt);
    return card_id;
}

/**
 * Get cards due for review
 */
FSRSCardList* fsrs_get_due_cards(int64_t student_id, int limit) {
    if (!g_edu_db)
        return NULL;

    FSRSCardList* list = calloc(1, sizeof(FSRSCardList));
    if (!list)
        return NULL;

    list->capacity = limit > 0 ? limit : 20;
    list->cards = calloc(list->capacity, sizeof(FSRSCard));
    if (!list->cards) {
        free(list);
        return NULL;
    }

    const char* sql = "SELECT id, student_id, topic_id, front, back, stability, difficulty, "
                      "       reps, lapses, last_review, next_review, created_at "
                      "FROM fsrs_cards "
                      "WHERE student_id = ? AND next_review <= strftime('%s', 'now') "
                      "ORDER BY next_review ASC "
                      "LIMIT ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(list->cards);
        free(list);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_int(stmt, 2, list->capacity);

    while (sqlite3_step(stmt) == SQLITE_ROW && list->count < list->capacity) {
        FSRSCard* card = &list->cards[list->count];

        card->card_id = sqlite3_column_int64(stmt, 0);
        card->student_id = sqlite3_column_int64(stmt, 1);

        const char* topic = (const char*)sqlite3_column_text(stmt, 2);
        card->topic_id = topic ? strdup(topic) : NULL;

        const char* front = (const char*)sqlite3_column_text(stmt, 3);
        card->front = front ? strdup(front) : NULL;

        const char* back = (const char*)sqlite3_column_text(stmt, 4);
        card->back = back ? strdup(back) : NULL;

        card->stability = (float)sqlite3_column_double(stmt, 5);
        card->difficulty = (float)sqlite3_column_double(stmt, 6);
        card->reps = sqlite3_column_int(stmt, 7);
        card->lapses = sqlite3_column_int(stmt, 8);
        card->last_review = sqlite3_column_int64(stmt, 9);
        card->next_review = sqlite3_column_int64(stmt, 10);
        card->created_at = sqlite3_column_int64(stmt, 11);

        list->count++;
    }

    sqlite3_finalize(stmt);
    return list;
}

/**
 * Record a review and update card scheduling
 */
int fsrs_record_review(int64_t card_id, int quality) {
    if (!g_edu_db || quality < 1 || quality > 5)
        return -1;

    // Get current card state
    const char* sql_get = "SELECT stability, difficulty, reps, lapses, last_review "
                          "FROM fsrs_cards WHERE id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql_get, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, card_id);

    float S = FSRS_INITIAL_STABILITY;
    float D = FSRS_INITIAL_DIFFICULTY;
    int reps = 0;
    int lapses = 0;
    time_t last_review = 0;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        S = (float)sqlite3_column_double(stmt, 0);
        D = (float)sqlite3_column_double(stmt, 1);
        reps = sqlite3_column_int(stmt, 2);
        lapses = sqlite3_column_int(stmt, 3);
        last_review = sqlite3_column_int64(stmt, 4);
    }
    sqlite3_finalize(stmt);

    // Calculate days elapsed since last review
    time_t now = time(NULL);
    float days_elapsed = last_review > 0 ? (float)(now - last_review) / (24.0f * 3600.0f) : 0.0f;

    // Calculate current retrievability
    float R = fsrs_retrievability(S, days_elapsed);

    // Update counts
    reps++;
    if (quality == QUALITY_FORGOT) {
        lapses++;
    }

    // Calculate new FSRS parameters
    float new_S = fsrs_new_stability(S, D, R, quality, lapses);
    float new_D = fsrs_new_difficulty(D, quality);

    // Calculate next review interval
    int hours_until_next = fsrs_next_interval(new_S, FSRS_DESIRED_RETENTION);
    time_t next_review = now + hours_until_next * 3600;

    // Update database
    const char* sql_update = "UPDATE fsrs_cards SET "
                             "  stability = ?, difficulty = ?, reps = ?, lapses = ?, "
                             "  last_review = ?, next_review = ? "
                             "WHERE id = ?";

    if (sqlite3_prepare_v2(g_edu_db, sql_update, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_double(stmt, 1, new_S);
    sqlite3_bind_double(stmt, 2, new_D);
    sqlite3_bind_int(stmt, 3, reps);
    sqlite3_bind_int(stmt, 4, lapses);
    sqlite3_bind_int64(stmt, 5, now);
    sqlite3_bind_int64(stmt, 6, next_review);
    sqlite3_bind_int64(stmt, 7, card_id);

    int result = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);

    return result;
}

/**
 * Free card list memory
 */
void fsrs_free_cards(FSRSCardList* list) {
    if (!list)
        return;

    for (int i = 0; i < list->count; i++) {
        free(list->cards[i].topic_id);
        free(list->cards[i].front);
        free(list->cards[i].back);
    }
    free(list->cards);
    free(list);
}

// ============================================================================
// STATS AND REPORTING
// ============================================================================

/**
 * Get student's FSRS statistics
 */
typedef struct {
    int total_cards;
    int cards_due;
    int cards_mastered; // stability > 30 days
    float avg_stability;
    float avg_difficulty;
    int streak_days;
    time_t last_study;
} FSRSStats;

FSRSStats fsrs_get_stats(int64_t student_id) {
    FSRSStats stats = {0};
    if (!g_edu_db)
        return stats;

    // Get card counts and averages
    const char* sql = "SELECT "
                      "  COUNT(*), "
                      "  SUM(CASE WHEN next_review <= strftime('%s', 'now') THEN 1 ELSE 0 END), "
                      "  SUM(CASE WHEN stability > 30 THEN 1 ELSE 0 END), "
                      "  AVG(stability), "
                      "  AVG(difficulty), "
                      "  MAX(last_review) "
                      "FROM fsrs_cards WHERE student_id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_cards = sqlite3_column_int(stmt, 0);
            stats.cards_due = sqlite3_column_int(stmt, 1);
            stats.cards_mastered = sqlite3_column_int(stmt, 2);
            stats.avg_stability = (float)sqlite3_column_double(stmt, 3);
            stats.avg_difficulty = (float)sqlite3_column_double(stmt, 4);
            stats.last_study = sqlite3_column_int64(stmt, 5);
        }
        sqlite3_finalize(stmt);
    }

    // Calculate streak (days with at least one review)
    const char* sql_streak =
        "SELECT COUNT(DISTINCT DATE(last_review, 'unixepoch')) "
        "FROM fsrs_cards "
        "WHERE student_id = ? AND last_review >= strftime('%s', 'now', '-7 days')";

    if (sqlite3_prepare_v2(g_edu_db, sql_streak, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.streak_days = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return stats;
}

/**
 * Get predicted retention for all cards
 */
float fsrs_predicted_retention(int64_t student_id) {
    if (!g_edu_db)
        return 0.0f;

    const char* sql = "SELECT stability, last_review FROM fsrs_cards WHERE student_id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0.0f;
    }

    sqlite3_bind_int64(stmt, 1, student_id);

    float total_R = 0.0f;
    int count = 0;
    time_t now = time(NULL);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        float stability = (float)sqlite3_column_double(stmt, 0);
        time_t last_review = sqlite3_column_int64(stmt, 1);

        float days = last_review > 0 ? (float)(now - last_review) / (24.0f * 3600.0f) : 0.0f;

        total_R += fsrs_retrievability(stability, days);
        count++;
    }

    sqlite3_finalize(stmt);

    return count > 0 ? total_R / (float)count : 0.0f;
}
