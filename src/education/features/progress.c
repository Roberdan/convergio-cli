/**
 * CONVERGIO EDUCATION - PROGRESS TRACKING
 *
 * Comprehensive progress tracking with dashboard, topic completion,
 * AI-powered recommendations, and parent reporting.
 *
 * Features:
 * - F13: Interactive progress dashboard
 * - F14: Topic completion tracking
 * - F15: AI-powered next topic recommendations
 * - F16: Parent reports (PDF/email)
 * - F17: Skill radar charts and visualizations
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#include "education_features.h"
#include "nous/education.h"
#include <math.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// EXTERNAL DEPENDENCIES
// ============================================================================

extern sqlite3* g_edu_db;
extern char* llm_generate(const char* prompt, const char* system_prompt);

// ============================================================================
// CONSTANTS
// ============================================================================

#define SKILL_MASTERY_THRESHOLD 0.80f // Aligned with mastery.c MASTERY_THRESHOLD
#define SKILL_COMPLETED_THRESHOLD 0.70f
#define SKILL_IN_PROGRESS_THRESHOLD 0.30f

#define SECONDS_PER_HOUR 3600
#define DAYS_IN_WEEK 7

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

__attribute__((unused)) static float calculate_skill_level(int quiz_score, int time_spent_minutes,
                                                           int interactions) {
    // Simple heuristic: combine quiz performance with engagement
    // NOTE: Reserved for future use in adaptive learning (S18)
    float quiz_component = (quiz_score / 100.0f) * 0.6f;
    float time_component = fminf((time_spent_minutes / 120.0f), 1.0f) * 0.2f;
    float interaction_component = fminf((interactions / 10.0f), 1.0f) * 0.2f;

    return fminf(quiz_component + time_component + interaction_component, 1.0f);
}

static TopicStatus determine_status(float skill_level) {
    if (skill_level >= SKILL_MASTERY_THRESHOLD) {
        return TOPIC_MASTERED;
    } else if (skill_level >= SKILL_COMPLETED_THRESHOLD) {
        return TOPIC_COMPLETED;
    } else if (skill_level >= SKILL_IN_PROGRESS_THRESHOLD) {
        return TOPIC_IN_PROGRESS;
    } else {
        return TOPIC_NOT_STARTED;
    }
}

// ============================================================================
// PROGRESS DASHBOARD
// ============================================================================

ProgressDashboard* progress_dashboard(int64_t student_id) {
    if (!g_edu_db) {
        return NULL;
    }

    ProgressDashboard* dashboard = calloc(1, sizeof(ProgressDashboard));
    if (!dashboard) {
        return NULL;
    }

    dashboard->student_id = student_id;

    // Calculate total study hours
    const char* sql_hours =
        "SELECT COALESCE(SUM(duration_seconds), 0) / 3600.0 FROM learning_sessions "
        "WHERE student_id = ? AND completed = 1";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql_hours, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            dashboard->total_study_hours = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Get current streak
    const char* sql_streak = "SELECT current_streak FROM gamification WHERE student_id = ?";

    if (sqlite3_prepare_v2(g_edu_db, sql_streak, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            dashboard->current_streak_days = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Count topics by status
    const char* sql_topics =
        "SELECT "
        "SUM(CASE WHEN skill_level >= 0.70 THEN 1 ELSE 0 END) as completed, "
        "SUM(CASE WHEN skill_level < 0.70 AND skill_level > 0 THEN 1 ELSE 0 END) as in_progress "
        "FROM learning_progress WHERE student_id = ?";

    if (sqlite3_prepare_v2(g_edu_db, sql_topics, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            dashboard->topics_completed = sqlite3_column_int(stmt, 0);
            dashboard->topics_in_progress = sqlite3_column_int(stmt, 1);
        }
        sqlite3_finalize(stmt);
    }

    // Average quiz score
    const char* sql_quiz = "SELECT AVG(score_percent) FROM quiz_history WHERE student_id = ?";

    if (sqlite3_prepare_v2(g_edu_db, sql_quiz, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            dashboard->avg_quiz_score = (float)sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Build subject breakdown JSON
    const char* sql_subjects = "SELECT subject, "
                               "COUNT(*) as topic_count, "
                               "AVG(skill_level) as avg_skill, "
                               "SUM(total_time_spent) as total_minutes "
                               "FROM learning_progress "
                               "WHERE student_id = ? "
                               "GROUP BY subject";

    char subjects_json[4096] = "{\n  \"subjects\": [\n";

    if (sqlite3_prepare_v2(g_edu_db, sql_subjects, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);

        bool first = true;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* subject = (const char*)sqlite3_column_text(stmt, 0);
            int topic_count = sqlite3_column_int(stmt, 1);
            float avg_skill = (float)sqlite3_column_double(stmt, 2);
            int total_minutes = sqlite3_column_int(stmt, 3);

            char entry[512];
            snprintf(entry, sizeof(entry),
                     "%s    {\"subject\": \"%s\", \"topics\": %d, \"skill\": %.2f, \"time_hours\": "
                     "%.1f}",
                     first ? "" : ",\n", subject, topic_count, avg_skill, total_minutes / 60.0f);
            strncat(subjects_json, entry, sizeof(subjects_json) - strlen(subjects_json) - 1);

            first = false;
        }
        sqlite3_finalize(stmt);
    }

    strncat(subjects_json, "\n  ]\n}", sizeof(subjects_json) - strlen(subjects_json) - 1);
    dashboard->subject_breakdown = strdup(subjects_json);

    // Build weekly activity JSON
    const char* sql_weekly = "SELECT date(started_at, 'unixepoch') as day, "
                             "COUNT(*) as sessions, "
                             "SUM(duration_seconds)/3600.0 as hours "
                             "FROM learning_sessions "
                             "WHERE student_id = ? "
                             "AND started_at > strftime('%s', 'now', '-7 days') "
                             "GROUP BY day ORDER BY day";

    char weekly_json[2048] = "{\n  \"days\": [\n";

    if (sqlite3_prepare_v2(g_edu_db, sql_weekly, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);

        bool first = true;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* day = (const char*)sqlite3_column_text(stmt, 0);
            int sessions = sqlite3_column_int(stmt, 1);
            float hours = (float)sqlite3_column_double(stmt, 2);

            char entry[256];
            snprintf(entry, sizeof(entry),
                     "%s    {\"date\": \"%s\", \"sessions\": %d, \"hours\": %.1f}",
                     first ? "" : ",\n", day, sessions, hours);
            strncat(weekly_json, entry, sizeof(weekly_json) - strlen(weekly_json) - 1);

            first = false;
        }
        sqlite3_finalize(stmt);
    }

    strncat(weekly_json, "\n  ]\n}", sizeof(weekly_json) - strlen(weekly_json) - 1);
    dashboard->weekly_activity = strdup(weekly_json);

    // Build skill radar JSON
    const char* sql_radar = "SELECT subject, AVG(skill_level) as avg_skill "
                            "FROM learning_progress "
                            "WHERE student_id = ? "
                            "GROUP BY subject "
                            "LIMIT 8";

    char radar_json[2048] = "{\n  \"skills\": [\n";

    if (sqlite3_prepare_v2(g_edu_db, sql_radar, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);

        bool first = true;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* subject = (const char*)sqlite3_column_text(stmt, 0);
            float avg_skill = (float)sqlite3_column_double(stmt, 1);

            char entry[256];
            snprintf(entry, sizeof(entry), "%s    {\"subject\": \"%s\", \"level\": %.2f}",
                     first ? "" : ",\n", subject, avg_skill * 100.0f);
            strncat(radar_json, entry, sizeof(radar_json) - strlen(radar_json) - 1);

            first = false;
        }
        sqlite3_finalize(stmt);
    }

    strncat(radar_json, "\n  ]\n}", sizeof(radar_json) - strlen(radar_json) - 1);
    dashboard->skill_radar = strdup(radar_json);

    return dashboard;
}

// ============================================================================
// TOPIC TRACKING
// ============================================================================

TopicProgress** progress_track_topics(int64_t student_id, int* count) {
    if (!g_edu_db || !count) {
        return NULL;
    }

    *count = 0;

    // Get all topics for student
    const char* sql = "SELECT subject, topic, skill_level, total_time_spent, "
                      "interaction_count, quiz_score_avg, last_interaction "
                      "FROM learning_progress "
                      "WHERE student_id = ? "
                      "ORDER BY skill_level DESC, last_interaction DESC";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);

    // Count results
    int capacity = 100;
    TopicProgress** topics = malloc(sizeof(TopicProgress*) * capacity);
    if (!topics) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (*count >= capacity) {
            capacity *= 2;
            TopicProgress** new_topics = realloc(topics, sizeof(TopicProgress*) * capacity);
            if (!new_topics) {
                break;
            }
            topics = new_topics;
        }

        TopicProgress* tp = calloc(1, sizeof(TopicProgress));
        if (!tp)
            continue;

        const char* subject = (const char*)sqlite3_column_text(stmt, 0);
        const char* topic = (const char*)sqlite3_column_text(stmt, 1);

        if (subject)
            strncpy(tp->subject, subject, sizeof(tp->subject) - 1);
        if (topic)
            strncpy(tp->topic, topic, sizeof(tp->topic) - 1);

        tp->skill_level = (float)sqlite3_column_double(stmt, 2);
        tp->time_spent_minutes = sqlite3_column_int(stmt, 3);
        tp->quiz_attempts = sqlite3_column_int(stmt, 4);
        tp->quiz_avg_score = (float)sqlite3_column_double(stmt, 5);
        tp->last_activity = sqlite3_column_int64(stmt, 6);

        tp->status = determine_status(tp->skill_level);

        topics[*count] = tp;
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return topics;
}

// ============================================================================
// AI RECOMMENDATIONS
// ============================================================================

char* progress_suggest_next(int64_t student_id) {
    if (!g_edu_db) {
        return NULL;
    }

    // Get current progress summary
    int topic_count;
    TopicProgress** topics = progress_track_topics(student_id, &topic_count);

    if (!topics || topic_count == 0) {
        return strdup("Start with any topic that interests you!");
    }

    // Build context for LLM
    char context[4096] = "Student Progress:\n\n";

    for (int i = 0; i < topic_count && i < 20; i++) {
        char line[256];
        const char* status_str;
        switch (topics[i]->status) {
        case TOPIC_MASTERED:
            status_str = "Mastered";
            break;
        case TOPIC_COMPLETED:
            status_str = "Completed";
            break;
        case TOPIC_IN_PROGRESS:
            status_str = "In Progress";
            break;
        default:
            status_str = "Not Started";
            break;
        }

        snprintf(line, sizeof(line), "- %s: %s (skill: %.0f%%, time: %dmin)\n", topics[i]->topic,
                 status_str, topics[i]->skill_level * 100.0f, topics[i]->time_spent_minutes);
        strncat(context, line, sizeof(context) - strlen(context) - 1);
    }

    strncat(context, "\nWhat should the student focus on next?",
            sizeof(context) - strlen(context) - 1);

    const char* system_prompt =
        "You are an educational AI advisor. Based on the student's progress, "
        "recommend the next topic to study. Consider: 1) Building on completed "
        "topics, 2) Addressing gaps, 3) Maintaining momentum. "
        "Give a single, specific recommendation with brief reasoning.";

    char* recommendation = llm_generate(context, system_prompt);

    // Free topics
    for (int i = 0; i < topic_count; i++) {
        free(topics[i]);
    }
    free(topics);

    if (!recommendation) {
        recommendation = strdup("Continue practicing topics you've started to build mastery!");
    }

    return recommendation;
}

// ============================================================================
// PARENT REPORTS
// ============================================================================

char* progress_parent_report(int64_t student_id, const char* format) {
    if (!format) {
        return NULL;
    }

    ProgressDashboard* dashboard = progress_dashboard(student_id);
    if (!dashboard) {
        return NULL;
    }

    // Get student name
    char student_name[128] = "Student";
    const char* sql_name = "SELECT name FROM student_profiles WHERE id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(g_edu_db, sql_name, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* name = (const char*)sqlite3_column_text(stmt, 0);
            if (name)
                strncpy(student_name, name, sizeof(student_name) - 1);
        }
        sqlite3_finalize(stmt);
    }

    // Generate report
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%B %d, %Y", tm_info);

    char* report = malloc(8192);
    if (!report) {
        progress_dashboard_free(dashboard);
        return NULL;
    }

    if (strcmp(format, "email") == 0) {
        snprintf(report, 8192,
                 "Subject: Weekly Progress Report for %s\n\n"
                 "Dear Parent/Guardian,\n\n"
                 "Here is the weekly progress report for %s as of %s:\n\n"
                 "📊 OVERVIEW\n"
                 "───────────────────────────────────\n"
                 "Total Study Time: %d hours\n"
                 "Current Streak: %d days\n"
                 "Topics Completed: %d\n"
                 "Topics In Progress: %d\n"
                 "Average Quiz Score: %.1f%%\n\n"
                 "📚 SUBJECT BREAKDOWN\n"
                 "───────────────────────────────────\n"
                 "%s\n\n"
                 "📈 WEEKLY ACTIVITY\n"
                 "───────────────────────────────────\n"
                 "%s\n\n"
                 "🎯 RECOMMENDATION\n"
                 "───────────────────────────────────\n"
                 "Your child is making good progress! Encourage continued daily study "
                 "sessions to maintain the %d-day streak.\n\n"
                 "Best regards,\n"
                 "Convergio Education Team\n",
                 student_name, student_name, date_str, dashboard->total_study_hours,
                 dashboard->current_streak_days, dashboard->topics_completed,
                 dashboard->topics_in_progress, dashboard->avg_quiz_score,
                 dashboard->subject_breakdown ? dashboard->subject_breakdown : "No data",
                 dashboard->weekly_activity ? dashboard->weekly_activity : "No data",
                 dashboard->current_streak_days);
    } else {
        // PDF format (simplified - would use proper PDF library)
        snprintf(report, 8192,
                 "PROGRESS REPORT\n"
                 "═══════════════════════════════════════\n\n"
                 "Student: %s\n"
                 "Date: %s\n\n"
                 "SUMMARY\n"
                 "───────────────────────────────────────\n"
                 "Study Hours: %d\n"
                 "Streak: %d days\n"
                 "Completed: %d topics\n"
                 "In Progress: %d topics\n"
                 "Quiz Average: %.1f%%\n\n"
                 "SUBJECTS\n"
                 "───────────────────────────────────────\n"
                 "%s\n\n"
                 "WEEKLY ACTIVITY\n"
                 "───────────────────────────────────────\n"
                 "%s\n",
                 student_name, date_str, dashboard->total_study_hours,
                 dashboard->current_streak_days, dashboard->topics_completed,
                 dashboard->topics_in_progress, dashboard->avg_quiz_score,
                 dashboard->subject_breakdown ? dashboard->subject_breakdown : "No data",
                 dashboard->weekly_activity ? dashboard->weekly_activity : "No data");
    }

    progress_dashboard_free(dashboard);
    return report;
}

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

void progress_dashboard_free(ProgressDashboard* dashboard) {
    if (!dashboard)
        return;

    free(dashboard->subject_breakdown);
    free(dashboard->weekly_activity);
    free(dashboard->skill_radar);
    free(dashboard);
}

void progress_topic_list_free(TopicProgress** topics, int count) {
    if (!topics)
        return;

    for (int i = 0; i < count; i++) {
        free(topics[i]);
    }
    free(topics);
}
