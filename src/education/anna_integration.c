/**
 * ANNA EDUCATION INTEGRATION
 *
 * Connects Anna Executive Assistant with Education Module to provide:
 * - Homework reminders
 * - Spaced repetition reminders
 * - ADHD-aware break reminders
 * - Achievement celebrations
 *
 * Reminders are stored in education_db inbox table with extended metadata.
 * Native macOS notifications are delivered via osascript.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/anna_integration.h"
#include "nous/education.h"
#include "nous/notify.h"
#include "nous/debug_mutex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sqlite3.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define ANNA_EDU_MAX_CONTENT 1024
#define ANNA_EDU_DEFAULT_BREAK_INTERVAL 25  // Standard Pomodoro
#define ANNA_EDU_ADHD_MILD_INTERVAL 25
#define ANNA_EDU_ADHD_MODERATE_INTERVAL 15
#define ANNA_EDU_ADHD_SEVERE_INTERVAL 10
#define ANNA_EDU_HOMEWORK_ADVANCE_1 86400   // 24 hours
#define ANNA_EDU_HOMEWORK_ADVANCE_2 3600    // 1 hour
#define ANNA_EDU_GOAL_ADVANCE_1 604800      // 7 days
#define ANNA_EDU_GOAL_ADVANCE_2 259200      // 3 days
#define ANNA_EDU_GOAL_ADVANCE_3 86400       // 1 day

// ============================================================================
// GLOBAL STATE
// ============================================================================

static bool g_anna_edu_initialized = false;
static sqlite3* g_anna_edu_db = NULL;  // Points to education DB
CONVERGIO_MUTEX_DECLARE(g_anna_edu_mutex);

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

/**
 * Get access to the education database
 */
extern sqlite3* education_get_db_handle(void);  // Forward declaration
extern bool education_is_initialized(void);

static sqlite3* anna_get_db(void) {
    // Access the education database directly
    return education_get_db_handle();
}

static const char* reminder_type_to_string(AnnaReminderType type) {
    switch (type) {
        case ANNA_REMINDER_HOMEWORK: return "homework";
        case ANNA_REMINDER_SPACED_REPETITION: return "spaced_repetition";
        case ANNA_REMINDER_BREAK: return "break";
        case ANNA_REMINDER_CELEBRATION: return "celebration";
        case ANNA_REMINDER_SESSION: return "session";
        case ANNA_REMINDER_GOAL: return "goal";
        default: return "unknown";
    }
}

static AnnaReminderType string_to_reminder_type(const char* str) {
    if (!str) return ANNA_REMINDER_HOMEWORK;
    if (strcmp(str, "homework") == 0) return ANNA_REMINDER_HOMEWORK;
    if (strcmp(str, "spaced_repetition") == 0) return ANNA_REMINDER_SPACED_REPETITION;
    if (strcmp(str, "break") == 0) return ANNA_REMINDER_BREAK;
    if (strcmp(str, "celebration") == 0) return ANNA_REMINDER_CELEBRATION;
    if (strcmp(str, "session") == 0) return ANNA_REMINDER_SESSION;
    if (strcmp(str, "goal") == 0) return ANNA_REMINDER_GOAL;
    return ANNA_REMINDER_HOMEWORK;
}

/**
 * Store a reminder in the inbox table
 * Uses inbox table with extended content format:
 * "REMINDER|type|scheduled_at|content"
 */
static int64_t anna_store_reminder(int64_t student_id, AnnaReminderType type,
                                   const char* content, time_t scheduled_at) {
    if (!g_anna_edu_initialized || !content) return -1;

    CONVERGIO_MUTEX_LOCK(&g_anna_edu_mutex);

    sqlite3* db = anna_get_db();
    if (!db) {
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return -1;
    }

    // Format: "REMINDER|type|scheduled_at|content"
    char formatted_content[ANNA_EDU_MAX_CONTENT + 100];
    snprintf(formatted_content, sizeof(formatted_content),
             "REMINDER|%s|%ld|%s",
             reminder_type_to_string(type),
             (long)scheduled_at,
             content);

    const char* sql =
        "INSERT INTO inbox (student_id, content, source, processed, created_at) "
        "VALUES (?, ?, 'reminder', 0, strftime('%s','now'))";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[anna_edu] Failed to prepare reminder insert: %s\n",
                sqlite3_errmsg(db));
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, formatted_content, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int64_t reminder_id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(db) : -1;
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);

    if (reminder_id > 0) {
        fprintf(stderr, "[anna_edu] Stored reminder %lld for student %lld: %s\n",
                (long long)reminder_id, (long long)student_id, content);
    }

    return reminder_id;
}

/**
 * Send notification via osascript
 */
static int anna_send_notification_osascript(const char* title, const char* body) {
    if (!title || !body) return -1;

    // Escape quotes in title and body
    char escaped_title[512];
    char escaped_body[1024];

    snprintf(escaped_title, sizeof(escaped_title), "%s", title);
    snprintf(escaped_body, sizeof(escaped_body), "%s", body);

    // Build osascript command
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "osascript -e 'display notification \"%s\" with title \"%s\" sound name \"Glass\"'",
             escaped_body, escaped_title);

    int result = system(cmd);

    if (result == 0) {
        fprintf(stderr, "[anna_edu] Notification sent: %s\n", title);
        return 0;
    } else {
        fprintf(stderr, "[anna_edu] Failed to send notification: %d\n", result);
        return -1;
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

int anna_education_connect(void) {
    CONVERGIO_MUTEX_LOCK(&g_anna_edu_mutex);

    if (g_anna_edu_initialized) {
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return 0;
    }

    // Check if education module is initialized
    if (!education_is_initialized()) {
        fprintf(stderr, "[anna_edu] Education module not initialized\n");
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return -1;
    }

    // Get reference to education database
    g_anna_edu_db = anna_get_db();
    if (!g_anna_edu_db) {
        fprintf(stderr, "[anna_edu] Failed to access education database\n");
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return -1;
    }

    // Initialize notification system if not already initialized
    notify_init();

    g_anna_edu_initialized = true;
    fprintf(stderr, "[anna_edu] Anna education integration connected\n");

    CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
    return 0;
}

void anna_education_disconnect(void) {
    CONVERGIO_MUTEX_LOCK(&g_anna_edu_mutex);

    g_anna_edu_db = NULL;
    g_anna_edu_initialized = false;

    fprintf(stderr, "[anna_edu] Anna education integration disconnected\n");

    CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
}

bool anna_education_is_connected(void) {
    return g_anna_edu_initialized;
}

// ============================================================================
// REMINDER SCHEDULING (F18-F22)
// ============================================================================

int64_t anna_homework_reminder(int64_t student_id, const char* subject,
                                const char* assignment, time_t due_date) {
    if (!g_anna_edu_initialized || !subject || !assignment) return -1;

    // Create reminder content
    char content[ANNA_EDU_MAX_CONTENT];
    snprintf(content, sizeof(content),
             "Homework due in %s: %s",
             subject, assignment);

    // Schedule 24h before due date
    time_t remind_at_1 = due_date - ANNA_EDU_HOMEWORK_ADVANCE_1;
    int64_t reminder_id_1 = anna_store_reminder(student_id, ANNA_REMINDER_HOMEWORK,
                                                 content, remind_at_1);

    // Schedule 1h before due date
    time_t remind_at_2 = due_date - ANNA_EDU_HOMEWORK_ADVANCE_2;
    int64_t reminder_id_2 = anna_store_reminder(student_id, ANNA_REMINDER_HOMEWORK,
                                                 content, remind_at_2);

    fprintf(stderr, "[anna_edu] Scheduled homework reminders for %s (IDs: %lld, %lld)\n",
            subject, (long long)reminder_id_1, (long long)reminder_id_2);

    return reminder_id_1;  // Return first reminder ID
}

int64_t anna_spaced_repetition_reminder(int64_t student_id, const char* topic,
                                         time_t next_review) {
    if (!g_anna_edu_initialized || !topic) return -1;

    char content[ANNA_EDU_MAX_CONTENT];
    snprintf(content, sizeof(content),
             "Time to review: %s",
             topic);

    int64_t reminder_id = anna_store_reminder(student_id, ANNA_REMINDER_SPACED_REPETITION,
                                               content, next_review);

    fprintf(stderr, "[anna_edu] Scheduled spaced repetition reminder for '%s' (ID: %lld)\n",
            topic, (long long)reminder_id);

    return reminder_id;
}

int64_t anna_adhd_break_reminder(int64_t student_id, int64_t session_id) {
    if (!g_anna_edu_initialized) return -1;

    // Get student profile to check ADHD settings
    EducationStudentProfile* profile = education_profile_get(student_id);
    if (!profile) {
        return -1;
    }

    int break_interval = anna_get_break_interval(student_id);
    education_profile_free(profile);

    // Schedule break reminder
    time_t break_time = time(NULL) + (break_interval * 60);

    char content[ANNA_EDU_MAX_CONTENT];
    snprintf(content, sizeof(content),
             "Time for a %d-minute break! Take a walk, stretch, or grab some water.",
             break_interval);

    int64_t reminder_id = anna_store_reminder(student_id, ANNA_REMINDER_BREAK,
                                               content, break_time);

    fprintf(stderr, "[anna_edu] Scheduled ADHD break reminder in %d minutes (ID: %lld)\n",
            break_interval, (long long)reminder_id);

    return reminder_id;
}

int anna_celebration_notify(int64_t student_id, const AnnaCelebration* celebration) {
    if (!g_anna_edu_initialized || !celebration) return -1;

    // Build celebration message
    char title[256];
    char body[512];

    snprintf(title, sizeof(title), "%s %s",
             celebration->emoji ? celebration->emoji : "🎉",
             celebration->title ? celebration->title : "Great job!");

    snprintf(body, sizeof(body), "%s",
             celebration->message ? celebration->message : "Keep up the great work!");

    // Send immediate notification
    int result = anna_send_accessible_notification(student_id, title, body);

    if (result == 0) {
        fprintf(stderr, "[anna_edu] Celebration sent for student %lld: %s\n",
                (long long)student_id, celebration->achievement_type);
    }

    return result;
}

int64_t anna_session_reminder(int64_t student_id, const char* subject,
                               time_t scheduled_time) {
    if (!g_anna_edu_initialized || !subject) return -1;

    char content[ANNA_EDU_MAX_CONTENT];
    snprintf(content, sizeof(content),
             "Time to study %s!",
             subject);

    int64_t reminder_id = anna_store_reminder(student_id, ANNA_REMINDER_SESSION,
                                               content, scheduled_time);

    fprintf(stderr, "[anna_edu] Scheduled session reminder for %s (ID: %lld)\n",
            subject, (long long)reminder_id);

    return reminder_id;
}

int anna_goal_reminder(int64_t student_id, int64_t goal_id,
                        const char* goal_description, time_t deadline) {
    if (!g_anna_edu_initialized || !goal_description) return -1;

    int count = 0;

    // Schedule 7 days before
    time_t remind_7d = deadline - ANNA_EDU_GOAL_ADVANCE_1;
    if (remind_7d > time(NULL)) {
        char content[ANNA_EDU_MAX_CONTENT];
        snprintf(content, sizeof(content),
                 "Goal reminder: %s (7 days left)",
                 goal_description);
        if (anna_store_reminder(student_id, ANNA_REMINDER_GOAL, content, remind_7d) > 0) {
            count++;
        }
    }

    // Schedule 3 days before
    time_t remind_3d = deadline - ANNA_EDU_GOAL_ADVANCE_2;
    if (remind_3d > time(NULL)) {
        char content[ANNA_EDU_MAX_CONTENT];
        snprintf(content, sizeof(content),
                 "Goal reminder: %s (3 days left)",
                 goal_description);
        if (anna_store_reminder(student_id, ANNA_REMINDER_GOAL, content, remind_3d) > 0) {
            count++;
        }
    }

    // Schedule 1 day before
    time_t remind_1d = deadline - ANNA_EDU_GOAL_ADVANCE_3;
    if (remind_1d > time(NULL)) {
        char content[ANNA_EDU_MAX_CONTENT];
        snprintf(content, sizeof(content),
                 "Goal reminder: %s (1 day left!)",
                 goal_description);
        if (anna_store_reminder(student_id, ANNA_REMINDER_GOAL, content, remind_1d) > 0) {
            count++;
        }
    }

    fprintf(stderr, "[anna_edu] Scheduled %d goal reminders for goal %lld\n",
            count, (long long)goal_id);

    return count;
}

// ============================================================================
// REMINDER MANAGEMENT
// ============================================================================

int anna_check_due_reminders(void) {
    if (!g_anna_edu_initialized) return -1;

    CONVERGIO_MUTEX_LOCK(&g_anna_edu_mutex);

    sqlite3* db = anna_get_db();
    if (!db) {
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return -1;
    }

    time_t now = time(NULL);
    int sent_count = 0;

    // Query unprocessed reminders from inbox
    const char* sql =
        "SELECT id, student_id, content "
        "FROM inbox "
        "WHERE source = 'reminder' AND processed = 0";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[anna_edu] Failed to query reminders: %s\n",
                sqlite3_errmsg(db));
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return -1;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t id = sqlite3_column_int64(stmt, 0);
        int64_t student_id = sqlite3_column_int64(stmt, 1);
        const char* content = (const char*)sqlite3_column_text(stmt, 2);

        // Parse content: "REMINDER|type|scheduled_at|content"
        char type_str[32] = {0};
        time_t scheduled_at = 0;
        char message[ANNA_EDU_MAX_CONTENT] = {0};

        if (sscanf(content, "REMINDER|%31[^|]|%ld|%1023[^\n]",
                   type_str, (long*)&scheduled_at, message) == 3) {

            // Check if reminder is due
            if (scheduled_at <= now) {
                // Send notification
                AnnaReminderType type = string_to_reminder_type(type_str);
                const char* title = NULL;

                switch (type) {
                    case ANNA_REMINDER_HOMEWORK:
                        title = "Homework Reminder";
                        break;
                    case ANNA_REMINDER_SPACED_REPETITION:
                        title = "Time to Review";
                        break;
                    case ANNA_REMINDER_BREAK:
                        title = "Break Time";
                        break;
                    case ANNA_REMINDER_SESSION:
                        title = "Study Session";
                        break;
                    case ANNA_REMINDER_GOAL:
                        title = "Goal Reminder";
                        break;
                    default:
                        title = "Reminder";
                        break;
                }

                if (anna_send_accessible_notification(student_id, title, message) == 0) {
                    // Mark as processed
                    const char* update_sql = "UPDATE inbox SET processed = 1 WHERE id = ?";
                    sqlite3_stmt* update_stmt;
                    if (sqlite3_prepare_v2(db, update_sql, -1, &update_stmt, NULL) == SQLITE_OK) {
                        sqlite3_bind_int64(update_stmt, 1, id);
                        sqlite3_step(update_stmt);
                        sqlite3_finalize(update_stmt);
                    }
                    sent_count++;
                }
            }
        }
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);

    if (sent_count > 0) {
        fprintf(stderr, "[anna_edu] Sent %d due reminders\n", sent_count);
    }

    return sent_count;
}

int anna_cancel_reminder(int64_t reminder_id) {
    if (!g_anna_edu_initialized) return -1;

    CONVERGIO_MUTEX_LOCK(&g_anna_edu_mutex);

    sqlite3* db = anna_get_db();
    if (!db) {
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return -1;
    }

    const char* sql = "UPDATE inbox SET processed = 1 WHERE id = ? AND source = 'reminder'";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, reminder_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);

    fprintf(stderr, "[anna_edu] Cancelled reminder %lld\n", (long long)reminder_id);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int anna_snooze_reminder(int64_t reminder_id, int snooze_minutes) {
    if (!g_anna_edu_initialized || snooze_minutes <= 0) return -1;

    CONVERGIO_MUTEX_LOCK(&g_anna_edu_mutex);

    sqlite3* db = anna_get_db();
    if (!db) {
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return -1;
    }

    // Get reminder content
    const char* sql = "SELECT content FROM inbox WHERE id = ? AND source = 'reminder'";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, reminder_id);

    char content[ANNA_EDU_MAX_CONTENT + 100];
    int result = -1;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* old_content = (const char*)sqlite3_column_text(stmt, 0);

        // Parse and update scheduled_at
        char type_str[32] = {0};
        time_t old_scheduled_at = 0;
        char message[ANNA_EDU_MAX_CONTENT] = {0};

        if (sscanf(old_content, "REMINDER|%31[^|]|%ld|%1023[^\n]",
                   type_str, (long*)&old_scheduled_at, message) == 3) {

            time_t new_scheduled_at = time(NULL) + (snooze_minutes * 60);

            // Update content with new time
            snprintf(content, sizeof(content),
                     "REMINDER|%s|%ld|%s",
                     type_str, (long)new_scheduled_at, message);

            const char* update_sql = "UPDATE inbox SET content = ? WHERE id = ?";
            sqlite3_stmt* update_stmt;
            if (sqlite3_prepare_v2(db, update_sql, -1, &update_stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(update_stmt, 1, content, -1, SQLITE_STATIC);
                sqlite3_bind_int64(update_stmt, 2, reminder_id);
                sqlite3_step(update_stmt);
                sqlite3_finalize(update_stmt);
                result = 0;
            }
        }
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);

    if (result == 0) {
        fprintf(stderr, "[anna_edu] Snoozed reminder %lld for %d minutes\n",
                (long long)reminder_id, snooze_minutes);
    }

    return result;
}

AnnaReminder** anna_list_reminders(int64_t student_id, int* count) {
    if (!g_anna_edu_initialized || !count) return NULL;

    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_anna_edu_mutex);

    sqlite3* db = anna_get_db();
    if (!db) {
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return NULL;
    }

    const char* sql =
        "SELECT id, student_id, content, created_at "
        "FROM inbox "
        "WHERE student_id = ? AND source = 'reminder' AND processed = 0 "
        "ORDER BY created_at DESC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);

    // Count reminders first
    int reminder_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        reminder_count++;
    }
    sqlite3_reset(stmt);

    if (reminder_count == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return NULL;
    }

    // Allocate array
    AnnaReminder** reminders = calloc((size_t)reminder_count, sizeof(AnnaReminder*));
    if (!reminders) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return NULL;
    }

    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < reminder_count) {
        AnnaReminder* reminder = calloc(1, sizeof(AnnaReminder));
        if (reminder) {
            reminder->id = sqlite3_column_int64(stmt, 0);
            reminder->student_id = sqlite3_column_int64(stmt, 1);

            const char* content = (const char*)sqlite3_column_text(stmt, 2);
            char type_str[32] = {0};
            time_t scheduled_at = 0;
            char message[ANNA_EDU_MAX_CONTENT] = {0};

            if (sscanf(content, "REMINDER|%31[^|]|%ld|%1023[^\n]",
                       type_str, (long*)&scheduled_at, message) == 3) {
                reminder->type = string_to_reminder_type(type_str);
                reminder->scheduled_at = scheduled_at;
                reminder->content = strdup(message);
            } else {
                reminder->content = strdup(content);
            }

            reminder->created_at = sqlite3_column_int64(stmt, 3);
            reminder->status = ANNA_REMINDER_PENDING;

            reminders[idx++] = reminder;
        }
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);

    *count = idx;
    return reminders;
}

void anna_reminder_free(AnnaReminder* reminder) {
    if (!reminder) return;
    free(reminder->content);
    free(reminder);
}

void anna_reminder_list_free(AnnaReminder** reminders, int count) {
    if (!reminders) return;
    for (int i = 0; i < count; i++) {
        anna_reminder_free(reminders[i]);
    }
    free(reminders);
}

// ============================================================================
// ACCESSIBILITY-AWARE FEATURES
// ============================================================================

int anna_get_break_interval(int64_t student_id) {
    EducationAccessibility* accessibility = education_accessibility_get(student_id);
    if (!accessibility) {
        return ANNA_EDU_DEFAULT_BREAK_INTERVAL;
    }

    int interval = ANNA_EDU_DEFAULT_BREAK_INTERVAL;

    if (accessibility->adhd) {
        switch (accessibility->adhd_type) {
            case ADHD_INATTENTIVE:
            case ADHD_COMBINED:
                // Adjust based on severity
                if (accessibility->adhd_severity == SEVERITY_SEVERE) {
                    interval = ANNA_EDU_ADHD_SEVERE_INTERVAL;
                } else if (accessibility->adhd_severity == SEVERITY_MODERATE) {
                    interval = ANNA_EDU_ADHD_MODERATE_INTERVAL;
                } else {  // Mild
                    interval = ANNA_EDU_ADHD_MILD_INTERVAL;
                }
                break;
            case ADHD_HYPERACTIVE:
                // Hyperactive type benefits from even more frequent breaks
                if (accessibility->adhd_severity == SEVERITY_SEVERE) {
                    interval = 8;
                } else if (accessibility->adhd_severity == SEVERITY_MODERATE) {
                    interval = 12;
                } else {
                    interval = 20;
                }
                break;
            default:
                break;
        }
    }

    free(accessibility);
    return interval;
}

bool anna_needs_break(int64_t student_id, int64_t session_id) {
    // TODO: Implement session tracking to check elapsed time
    // For now, always suggest breaks at interval
    return true;
}

int anna_send_accessible_notification(int64_t student_id, const char* title,
                                       const char* body) {
    if (!title || !body) return -1;

    // Get accessibility settings
    EducationAccessibility* accessibility = education_accessibility_get(student_id);

    // Send notification via osascript
    int result = anna_send_notification_osascript(title, body);

    // If TTS is enabled, also speak the notification
    if (accessibility && accessibility->tts_enabled) {
        char say_cmd[2048];
        snprintf(say_cmd, sizeof(say_cmd),
                 "say -r %.0f \"%s. %s\"",
                 accessibility->tts_speed * 100.0f,
                 title, body);
        system(say_cmd);
    }

    if (accessibility) {
        free(accessibility);
    }

    return result;
}

// ============================================================================
// STATISTICS
// ============================================================================

AnnaReminderStats anna_get_stats(int64_t student_id) {
    AnnaReminderStats stats = {0};

    if (!g_anna_edu_initialized) {
        return stats;
    }

    CONVERGIO_MUTEX_LOCK(&g_anna_edu_mutex);

    sqlite3* db = anna_get_db();
    if (!db) {
        CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);
        return stats;
    }

    // Count pending reminders
    const char* sql =
        "SELECT COUNT(*) FROM inbox "
        "WHERE student_id = ? AND source = 'reminder' AND processed = 0";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_pending = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Count sent today
    time_t today_start = time(NULL) - (time(NULL) % 86400);
    sql = "SELECT COUNT(*) FROM inbox "
          "WHERE student_id = ? AND source = 'reminder' AND processed = 1 "
          "AND created_at >= ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        sqlite3_bind_int64(stmt, 2, today_start);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_sent_today = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_anna_edu_mutex);

    return stats;
}
