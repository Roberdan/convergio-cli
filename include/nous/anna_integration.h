/**
 * @file anna_integration.h
 * @brief Anna Executive Assistant Integration for Education Module
 *
 * Connects Anna (executive assistant) with the Education system to provide:
 * - Homework reminders
 * - Spaced repetition reminders
 * - ADHD-aware break reminders
 * - Achievement celebrations
 *
 * Reminders are stored in the education_db inbox table and checked on app start.
 * Native macOS notifications are delivered via osascript.
 *
 * @see docs/adr/009-anna-executive-assistant.md
 * @see docs/plans/EducationPackPlan.md
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#ifndef NOUS_ANNA_INTEGRATION_H
#define NOUS_ANNA_INTEGRATION_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ENUMS
// ============================================================================

/**
 * @brief Reminder types for education notifications
 */
typedef enum {
    ANNA_REMINDER_HOMEWORK = 0,
    ANNA_REMINDER_SPACED_REPETITION = 1,
    ANNA_REMINDER_BREAK = 2,
    ANNA_REMINDER_CELEBRATION = 3,
    ANNA_REMINDER_SESSION = 4,
    ANNA_REMINDER_GOAL = 5
} AnnaReminderType;

/**
 * @brief Reminder status
 */
typedef enum {
    ANNA_REMINDER_PENDING = 0,
    ANNA_REMINDER_SENT = 1,
    ANNA_REMINDER_ACKNOWLEDGED = 2,
    ANNA_REMINDER_SNOOZED = 3,
    ANNA_REMINDER_CANCELLED = 4
} AnnaReminderStatus;

// ============================================================================
// STRUCTURES
// ============================================================================

/**
 * @brief Reminder record from inbox table
 */
typedef struct {
    int64_t id;
    int64_t student_id;
    AnnaReminderType type;
    char* content;
    time_t scheduled_at;
    time_t created_at;
    AnnaReminderStatus status;
    int retry_count;
} AnnaReminder;

/**
 * @brief Celebration data for achievements
 */
typedef struct {
    const char* achievement_type;  // "quiz_perfect", "streak_7", "level_up", etc.
    const char* title;
    const char* message;
    const char* emoji;  // Optional emoji for notification
} AnnaCelebration;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * @brief Initialize Anna education integration
 *
 * Connects to education database and notification system.
 * Must be called after education_init().
 *
 * @return 0 on success, -1 on error
 */
int anna_education_connect(void);

/**
 * @brief Shutdown Anna education integration
 */
void anna_education_disconnect(void);

/**
 * @brief Check if Anna education integration is active
 * @return true if initialized and connected
 */
bool anna_education_is_connected(void);

// ============================================================================
// REMINDER SCHEDULING
// ============================================================================

/**
 * @brief Schedule a homework reminder
 *
 * Creates a reminder for homework due date. Reminder is sent 24h before
 * and 1h before the due date.
 *
 * @param student_id Student profile ID
 * @param subject Subject name (e.g., "Matematica", "Storia")
 * @param assignment Assignment description
 * @param due_date When homework is due
 * @return Reminder ID on success, -1 on error
 */
int64_t anna_homework_reminder(int64_t student_id, const char* subject,
                                const char* assignment, time_t due_date);

/**
 * @brief Schedule a spaced repetition reminder
 *
 * Creates a reminder to review a topic based on spaced repetition schedule.
 * Timing is determined by SM-2 algorithm from flashcard_reviews table.
 *
 * @param student_id Student profile ID
 * @param topic Topic to review
 * @param next_review When to review (from SM-2 calculation)
 * @return Reminder ID on success, -1 on error
 */
int64_t anna_spaced_repetition_reminder(int64_t student_id, const char* topic,
                                         time_t next_review);

/**
 * @brief Schedule an ADHD-aware break reminder
 *
 * Suggests breaks for students with ADHD. Frequency is adjusted based on
 * ADHD severity and type from accessibility profile.
 *
 * - Mild ADHD: Every 25 minutes (standard Pomodoro)
 * - Moderate ADHD: Every 15 minutes
 * - Severe ADHD: Every 10 minutes
 *
 * @param student_id Student profile ID
 * @param session_id Active learning session ID
 * @return Reminder ID on success, -1 on error
 */
int64_t anna_adhd_break_reminder(int64_t student_id, int64_t session_id);

/**
 * @brief Celebrate a student achievement
 *
 * Sends an immediate celebration notification for achievements like:
 * - Perfect quiz score
 * - 7-day streak
 * - Level up
 * - Goal completion
 *
 * @param student_id Student profile ID
 * @param celebration Celebration data
 * @return 0 on success, -1 on error
 */
int anna_celebration_notify(int64_t student_id, const AnnaCelebration* celebration);

/**
 * @brief Schedule a study session reminder
 *
 * Reminds student to start their planned study session.
 *
 * @param student_id Student profile ID
 * @param subject Subject to study
 * @param scheduled_time When to start studying
 * @return Reminder ID on success, -1 on error
 */
int64_t anna_session_reminder(int64_t student_id, const char* subject,
                               time_t scheduled_time);

/**
 * @brief Schedule a goal deadline reminder
 *
 * Reminds student about upcoming goal deadline.
 * Sent 7 days, 3 days, and 1 day before deadline.
 *
 * @param student_id Student profile ID
 * @param goal_id Goal ID
 * @param goal_description Goal description
 * @param deadline Goal deadline
 * @return Number of reminders scheduled (0-3), -1 on error
 */
int anna_goal_reminder(int64_t student_id, int64_t goal_id,
                        const char* goal_description, time_t deadline);

// ============================================================================
// REMINDER MANAGEMENT
// ============================================================================

/**
 * @brief Check and send due reminders
 *
 * Called on app start to check for any pending reminders that are due.
 * Sends notifications for all due reminders.
 *
 * @return Number of reminders sent, -1 on error
 */
int anna_check_due_reminders(void);

/**
 * @brief Cancel a scheduled reminder
 *
 * @param reminder_id Reminder ID
 * @return 0 on success, -1 on error
 */
int anna_cancel_reminder(int64_t reminder_id);

/**
 * @brief Snooze a reminder
 *
 * @param reminder_id Reminder ID
 * @param snooze_minutes Minutes to snooze
 * @return 0 on success, -1 on error
 */
int anna_snooze_reminder(int64_t reminder_id, int snooze_minutes);

/**
 * @brief List pending reminders for a student
 *
 * @param student_id Student profile ID
 * @param count Output: number of reminders
 * @return Array of reminders (caller must free)
 */
AnnaReminder** anna_list_reminders(int64_t student_id, int* count);

/**
 * @brief Free a reminder
 * @param reminder Reminder to free
 */
void anna_reminder_free(AnnaReminder* reminder);

/**
 * @brief Free a list of reminders
 * @param reminders Array of reminders
 * @param count Number of reminders
 */
void anna_reminder_list_free(AnnaReminder** reminders, int count);

// ============================================================================
// ACCESSIBILITY-AWARE FEATURES
// ============================================================================

/**
 * @brief Get recommended break interval for student
 *
 * Returns break interval in minutes based on accessibility profile.
 * Considers ADHD severity and type.
 *
 * @param student_id Student profile ID
 * @return Break interval in minutes
 */
int anna_get_break_interval(int64_t student_id);

/**
 * @brief Check if student needs break reminder
 *
 * Checks if enough time has passed since last break based on
 * student's accessibility profile.
 *
 * @param student_id Student profile ID
 * @param session_id Active session ID
 * @return true if break is recommended
 */
bool anna_needs_break(int64_t student_id, int64_t session_id);

/**
 * @brief Send notification with accessibility adaptations
 *
 * Sends notification adapted to student's accessibility needs:
 * - Dyslexia: Uses OpenDyslexic font in notification if supported
 * - TTS enabled: Uses system speech synthesis to read notification
 * - Visual impairment: High contrast, larger text
 *
 * @param student_id Student profile ID
 * @param title Notification title
 * @param body Notification body
 * @return 0 on success, -1 on error
 */
int anna_send_accessible_notification(int64_t student_id, const char* title,
                                       const char* body);

// ============================================================================
// STATISTICS
// ============================================================================

/**
 * @brief Reminder statistics
 */
typedef struct {
    int total_pending;
    int total_sent_today;
    int total_sent_week;
    int homework_pending;
    int spaced_rep_pending;
    int break_reminders_today;
    int celebrations_today;
} AnnaReminderStats;

/**
 * @brief Get reminder statistics for a student
 *
 * @param student_id Student profile ID
 * @return Statistics structure
 */
AnnaReminderStats anna_get_stats(int64_t student_id);

#ifdef __cplusplus
}
#endif

#endif /* NOUS_ANNA_INTEGRATION_H */
