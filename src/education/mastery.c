/**
 * CONVERGIO EDUCATION - MASTERY LEARNING
 *
 * Implementation of mastery learning system inspired by Khan Academy.
 * Tracks skill mastery, detects gaps, and provides learning path recommendations.
 *
 * Key Concepts:
 * - Skill mastery: 80%+ correct = mastered
 * - Skill tree: Subject → Topic → Skill hierarchy
 * - Adaptive difficulty: Adjusts based on performance
 * - Prerequisites: Must master foundations before advancing
 *
 * Phase: FASE 11 - Learning Science
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <sqlite3.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MASTERY_THRESHOLD 0.80f       // 80% = mastered
#define PROFICIENT_THRESHOLD 0.60f    // 60% = proficient
#define FAMILIAR_THRESHOLD 0.40f      // 40% = familiar
#define ATTEMPTS_FOR_MASTERY 5        // Minimum attempts needed

// Difficulty adjustment factors
#define DIFFICULTY_INCREASE 1.15f
#define DIFFICULTY_DECREASE 0.85f
#define MIN_DIFFICULTY 0.5f
#define MAX_DIFFICULTY 2.0f

// ============================================================================
// TYPES
// ============================================================================

typedef enum {
    SKILL_NOT_STARTED = 0,
    SKILL_ATTEMPTED = 1,
    SKILL_FAMILIAR = 2,
    SKILL_PROFICIENT = 3,
    SKILL_MASTERED = 4
} SkillStatus;

typedef struct {
    int64_t id;
    int64_t student_id;
    char* skill_id;           // e.g., "math.fractions.addition"
    char* skill_name;         // Display name
    char* parent_skill_id;    // Parent in skill tree

    int attempts;
    int correct;
    float mastery_level;      // 0.0 - 1.0
    float current_difficulty; // Adaptive difficulty
    SkillStatus status;

    time_t last_practice;
    time_t mastered_at;
} MasterySkill;

typedef struct {
    char* skill_id;
    char* skill_name;
    SkillStatus status;
    float mastery_level;
    int children_count;
    struct SkillTreeNode* children;
} SkillTreeNode;

typedef struct {
    MasterySkill* skills;
    int count;
    int capacity;
} MasterySkillList;

// ============================================================================
// EXTERNAL DEPENDENCIES
// ============================================================================

extern sqlite3* g_edu_db;
extern char* llm_generate(const char* prompt, const char* system_prompt);

// ============================================================================
// DATABASE OPERATIONS
// ============================================================================

/**
 * Create mastery tables if they don't exist
 */
int mastery_init_db(void) {
    if (!g_edu_db) return -1;

    const char* sql =
        // Skills table
        "CREATE TABLE IF NOT EXISTS mastery_skills ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  student_id INTEGER NOT NULL,"
        "  skill_id TEXT NOT NULL,"
        "  skill_name TEXT,"
        "  parent_skill_id TEXT,"
        "  attempts INTEGER DEFAULT 0,"
        "  correct INTEGER DEFAULT 0,"
        "  mastery_level REAL DEFAULT 0.0,"
        "  current_difficulty REAL DEFAULT 1.0,"
        "  status INTEGER DEFAULT 0,"
        "  last_practice INTEGER,"
        "  mastered_at INTEGER,"
        "  created_at INTEGER DEFAULT (strftime('%s', 'now')),"
        "  UNIQUE(student_id, skill_id),"
        "  FOREIGN KEY (student_id) REFERENCES student_profiles(id)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_mastery_student ON mastery_skills(student_id);"
        "CREATE INDEX IF NOT EXISTS idx_mastery_parent ON mastery_skills(parent_skill_id);"

        // Skill definitions (curriculum)
        "CREATE TABLE IF NOT EXISTS skill_definitions ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  skill_id TEXT UNIQUE NOT NULL,"
        "  skill_name TEXT NOT NULL,"
        "  parent_id TEXT,"
        "  subject TEXT NOT NULL,"
        "  grade_level INTEGER,"
        "  description TEXT,"
        "  prerequisites TEXT"  // JSON array of prerequisite skill_ids
        ");"

        // Practice history
        "CREATE TABLE IF NOT EXISTS practice_history ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  student_id INTEGER NOT NULL,"
        "  skill_id TEXT NOT NULL,"
        "  was_correct INTEGER NOT NULL,"
        "  difficulty_level REAL,"
        "  response_time_ms INTEGER,"
        "  practiced_at INTEGER DEFAULT (strftime('%s', 'now')),"
        "  FOREIGN KEY (student_id) REFERENCES student_profiles(id)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_practice_student ON practice_history(student_id);";

    char* err = NULL;
    if (sqlite3_exec(g_edu_db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[Mastery] Init failed: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    return 0;
}

// ============================================================================
// MASTERY TRACKING
// ============================================================================

/**
 * Calculate mastery level from attempts and correct answers
 * Uses weighted average favoring recent attempts
 */
static float calculate_mastery(int attempts, int correct, float prev_mastery) {
    if (attempts <= 0) return 0.0f;

    // Simple ratio for new skills
    float simple_ratio = (float)correct / (float)attempts;

    // Weighted average with previous mastery (momentum)
    float weight = fminf((float)attempts / (float)ATTEMPTS_FOR_MASTERY, 1.0f);
    float mastery = weight * simple_ratio + (1.0f - weight) * prev_mastery;

    return fminf(fmaxf(mastery, 0.0f), 1.0f);
}

/**
 * Determine skill status from mastery level
 */
static SkillStatus status_from_mastery(float mastery, int attempts) {
    if (attempts < ATTEMPTS_FOR_MASTERY && mastery < MASTERY_THRESHOLD) {
        return SKILL_ATTEMPTED;
    }
    if (mastery >= MASTERY_THRESHOLD) return SKILL_MASTERED;
    if (mastery >= PROFICIENT_THRESHOLD) return SKILL_PROFICIENT;
    if (mastery >= FAMILIAR_THRESHOLD) return SKILL_FAMILIAR;
    if (attempts > 0) return SKILL_ATTEMPTED;
    return SKILL_NOT_STARTED;
}

/**
 * Record a practice attempt and update mastery
 */
int mastery_record_attempt(int64_t student_id, const char* skill_id,
                           bool was_correct, int response_time_ms) {
    if (!g_edu_db || !skill_id) return -1;

    // Get current state (or create new)
    const char* sql_get =
        "SELECT id, attempts, correct, mastery_level, current_difficulty, status "
        "FROM mastery_skills WHERE student_id = ? AND skill_id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql_get, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, skill_id, -1, SQLITE_STATIC);

    int64_t skill_pk = -1;
    int attempts = 0;
    int correct = 0;
    float mastery_level = 0.0f;
    float difficulty = 1.0f;
    SkillStatus old_status = SKILL_NOT_STARTED;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        skill_pk = sqlite3_column_int64(stmt, 0);
        attempts = sqlite3_column_int(stmt, 1);
        correct = sqlite3_column_int(stmt, 2);
        mastery_level = (float)sqlite3_column_double(stmt, 3);
        difficulty = (float)sqlite3_column_double(stmt, 4);
        old_status = sqlite3_column_int(stmt, 5);
    }
    sqlite3_finalize(stmt);

    // Update counts
    attempts++;
    if (was_correct) correct++;

    // Calculate new mastery
    float new_mastery = calculate_mastery(attempts, correct, mastery_level);
    SkillStatus new_status = status_from_mastery(new_mastery, attempts);

    // Adjust difficulty adaptively
    if (was_correct) {
        difficulty *= DIFFICULTY_INCREASE;
    } else {
        difficulty *= DIFFICULTY_DECREASE;
    }
    difficulty = fminf(fmaxf(difficulty, MIN_DIFFICULTY), MAX_DIFFICULTY);

    time_t now = time(NULL);
    time_t mastered_at = 0;
    if (new_status == SKILL_MASTERED && old_status != SKILL_MASTERED) {
        mastered_at = now;
    }

    // Insert or update skill record
    const char* sql_upsert;
    if (skill_pk >= 0) {
        sql_upsert =
            "UPDATE mastery_skills SET "
            "  attempts = ?, correct = ?, mastery_level = ?, "
            "  current_difficulty = ?, status = ?, last_practice = ?, "
            "  mastered_at = COALESCE(mastered_at, ?) "
            "WHERE id = ?";

        if (sqlite3_prepare_v2(g_edu_db, sql_upsert, -1, &stmt, NULL) != SQLITE_OK) {
            return -1;
        }

        sqlite3_bind_int(stmt, 1, attempts);
        sqlite3_bind_int(stmt, 2, correct);
        sqlite3_bind_double(stmt, 3, new_mastery);
        sqlite3_bind_double(stmt, 4, difficulty);
        sqlite3_bind_int(stmt, 5, new_status);
        sqlite3_bind_int64(stmt, 6, now);
        sqlite3_bind_int64(stmt, 7, mastered_at ? mastered_at : 0);
        sqlite3_bind_int64(stmt, 8, skill_pk);
    } else {
        sql_upsert =
            "INSERT INTO mastery_skills "
            "(student_id, skill_id, attempts, correct, mastery_level, "
            " current_difficulty, status, last_practice, mastered_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

        if (sqlite3_prepare_v2(g_edu_db, sql_upsert, -1, &stmt, NULL) != SQLITE_OK) {
            return -1;
        }

        sqlite3_bind_int64(stmt, 1, student_id);
        sqlite3_bind_text(stmt, 2, skill_id, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, attempts);
        sqlite3_bind_int(stmt, 4, correct);
        sqlite3_bind_double(stmt, 5, new_mastery);
        sqlite3_bind_double(stmt, 6, difficulty);
        sqlite3_bind_int(stmt, 7, new_status);
        sqlite3_bind_int64(stmt, 8, now);
        sqlite3_bind_int64(stmt, 9, mastered_at ? mastered_at : 0);
    }

    int result = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);

    // Record in history
    const char* sql_history =
        "INSERT INTO practice_history "
        "(student_id, skill_id, was_correct, difficulty_level, response_time_ms) "
        "VALUES (?, ?, ?, ?, ?)";

    if (sqlite3_prepare_v2(g_edu_db, sql_history, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        sqlite3_bind_text(stmt, 2, skill_id, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, was_correct ? 1 : 0);
        sqlite3_bind_double(stmt, 4, difficulty);
        sqlite3_bind_int(stmt, 5, response_time_ms);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    return result;
}

/**
 * Get skill mastery level
 */
float mastery_get_level(int64_t student_id, const char* skill_id) {
    if (!g_edu_db || !skill_id) return 0.0f;

    const char* sql =
        "SELECT mastery_level FROM mastery_skills "
        "WHERE student_id = ? AND skill_id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0.0f;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, skill_id, -1, SQLITE_STATIC);

    float level = 0.0f;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        level = (float)sqlite3_column_double(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return level;
}

/**
 * Get adaptive difficulty for a skill
 */
float mastery_get_difficulty(int64_t student_id, const char* skill_id) {
    if (!g_edu_db || !skill_id) return 1.0f;

    const char* sql =
        "SELECT current_difficulty FROM mastery_skills "
        "WHERE student_id = ? AND skill_id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 1.0f;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, skill_id, -1, SQLITE_STATIC);

    float difficulty = 1.0f;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        difficulty = (float)sqlite3_column_double(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return difficulty;
}

// ============================================================================
// SKILL GAP DETECTION
// ============================================================================

/**
 * Find skills with gaps (low mastery with sufficient attempts)
 */
MasterySkillList* mastery_find_gaps(int64_t student_id, const char* subject) {
    if (!g_edu_db) return NULL;

    MasterySkillList* list = calloc(1, sizeof(MasterySkillList));
    if (!list) return NULL;

    list->capacity = 50;
    list->skills = calloc(list->capacity, sizeof(MasterySkill));
    if (!list->skills) {
        free(list);
        return NULL;
    }

    const char* sql =
        "SELECT ms.id, ms.skill_id, sd.skill_name, ms.attempts, ms.correct, "
        "       ms.mastery_level, ms.status "
        "FROM mastery_skills ms "
        "LEFT JOIN skill_definitions sd ON ms.skill_id = sd.skill_id "
        "WHERE ms.student_id = ? "
        "  AND ms.mastery_level < ? "
        "  AND ms.attempts >= ? "
        "  AND (? IS NULL OR sd.subject = ?) "
        "ORDER BY ms.mastery_level ASC "
        "LIMIT 10";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(list->skills);
        free(list);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_double(stmt, 2, PROFICIENT_THRESHOLD);
    sqlite3_bind_int(stmt, 3, 3);  // At least 3 attempts to identify a gap
    if (subject) {
        sqlite3_bind_text(stmt, 4, subject, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, subject, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 4);
        sqlite3_bind_null(stmt, 5);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW && list->count < list->capacity) {
        MasterySkill* skill = &list->skills[list->count];

        skill->id = sqlite3_column_int64(stmt, 0);
        skill->student_id = student_id;

        const char* skill_id = (const char*)sqlite3_column_text(stmt, 1);
        skill->skill_id = skill_id ? strdup(skill_id) : NULL;

        const char* skill_name = (const char*)sqlite3_column_text(stmt, 2);
        skill->skill_name = skill_name ? strdup(skill_name) : NULL;

        skill->attempts = sqlite3_column_int(stmt, 3);
        skill->correct = sqlite3_column_int(stmt, 4);
        skill->mastery_level = (float)sqlite3_column_double(stmt, 5);
        skill->status = sqlite3_column_int(stmt, 6);

        list->count++;
    }

    sqlite3_finalize(stmt);
    return list;
}

/**
 * Free skill list memory
 */
void mastery_free_skills(MasterySkillList* list) {
    if (!list) return;

    for (int i = 0; i < list->count; i++) {
        free(list->skills[i].skill_id);
        free(list->skills[i].skill_name);
        free(list->skills[i].parent_skill_id);
    }
    free(list->skills);
    free(list);
}

// ============================================================================
// LEARNING PATH RECOMMENDATIONS
// ============================================================================

/**
 * Get next recommended skill to practice
 */
char* mastery_recommend_next(int64_t student_id, const char* subject) {
    if (!g_edu_db) return NULL;

    // Priority: 1) Skills with gaps, 2) In-progress skills, 3) New skills
    const char* sql =
        "SELECT ms.skill_id "
        "FROM mastery_skills ms "
        "LEFT JOIN skill_definitions sd ON ms.skill_id = sd.skill_id "
        "WHERE ms.student_id = ? "
        "  AND (? IS NULL OR sd.subject = ?) "
        "  AND ms.status < ? "  // Not mastered
        "ORDER BY "
        "  CASE "
        "    WHEN ms.attempts >= 3 AND ms.mastery_level < 0.5 THEN 0 "  // Gap
        "    WHEN ms.status = 2 OR ms.status = 3 THEN 1 "  // In progress
        "    ELSE 2 "
        "  END, "
        "  ms.last_practice ASC "  // Oldest first for spaced practice
        "LIMIT 1";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    if (subject) {
        sqlite3_bind_text(stmt, 2, subject, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, subject, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 2);
        sqlite3_bind_null(stmt, 3);
    }
    sqlite3_bind_int(stmt, 4, SKILL_MASTERED);

    char* result = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* skill_id = (const char*)sqlite3_column_text(stmt, 0);
        result = skill_id ? strdup(skill_id) : NULL;
    }

    sqlite3_finalize(stmt);
    return result;
}

// ============================================================================
// STATISTICS
// ============================================================================

typedef struct {
    int total_skills;
    int mastered_count;
    int proficient_count;
    int in_progress_count;
    int not_started_count;
    float avg_mastery;
    int total_attempts;
    int total_correct;
    float accuracy;
} MasteryStats;

MasteryStats mastery_get_stats(int64_t student_id, const char* subject) {
    MasteryStats stats = {0};
    if (!g_edu_db) return stats;

    const char* sql =
        "SELECT "
        "  COUNT(*), "
        "  SUM(CASE WHEN status = 4 THEN 1 ELSE 0 END), "
        "  SUM(CASE WHEN status = 3 THEN 1 ELSE 0 END), "
        "  SUM(CASE WHEN status IN (1, 2) THEN 1 ELSE 0 END), "
        "  SUM(CASE WHEN status = 0 THEN 1 ELSE 0 END), "
        "  AVG(mastery_level), "
        "  SUM(attempts), "
        "  SUM(correct) "
        "FROM mastery_skills ms "
        "LEFT JOIN skill_definitions sd ON ms.skill_id = sd.skill_id "
        "WHERE ms.student_id = ? "
        "  AND (? IS NULL OR sd.subject = ?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (subject) {
            sqlite3_bind_text(stmt, 2, subject, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, subject, -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_null(stmt, 2);
            sqlite3_bind_null(stmt, 3);
        }

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_skills = sqlite3_column_int(stmt, 0);
            stats.mastered_count = sqlite3_column_int(stmt, 1);
            stats.proficient_count = sqlite3_column_int(stmt, 2);
            stats.in_progress_count = sqlite3_column_int(stmt, 3);
            stats.not_started_count = sqlite3_column_int(stmt, 4);
            stats.avg_mastery = (float)sqlite3_column_double(stmt, 5);
            stats.total_attempts = sqlite3_column_int(stmt, 6);
            stats.total_correct = sqlite3_column_int(stmt, 7);
        }
        sqlite3_finalize(stmt);
    }

    stats.accuracy = stats.total_attempts > 0 ?
        (float)stats.total_correct / (float)stats.total_attempts : 0.0f;

    return stats;
}

/**
 * Get status label for display
 */
const char* mastery_status_label(SkillStatus status) {
    switch (status) {
        case SKILL_MASTERED:    return "Padroneggiato";
        case SKILL_PROFICIENT:  return "Competente";
        case SKILL_FAMILIAR:    return "Familiare";
        case SKILL_ATTEMPTED:   return "In corso";
        case SKILL_NOT_STARTED: return "Da iniziare";
        default:                return "Sconosciuto";
    }
}

/**
 * Get status emoji for display
 */
const char* mastery_status_emoji(SkillStatus status) {
    switch (status) {
        case SKILL_MASTERED:    return "✅";
        case SKILL_PROFICIENT:  return "🟢";
        case SKILL_FAMILIAR:    return "🟡";
        case SKILL_ATTEMPTED:   return "🟠";
        case SKILL_NOT_STARTED: return "⚪";
        default:                return "❓";
    }
}
