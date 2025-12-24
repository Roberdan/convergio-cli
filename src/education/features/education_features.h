/**
 * @file education_features.h
 * @brief Education Pack Interactive Features
 *
 * Advanced educational features including homework helper, study sessions,
 * and progress tracking with anti-cheat and parental transparency.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#ifndef EDUCATION_FEATURES_H
#define EDUCATION_FEATURES_H

#include "nous/education.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HOMEWORK HELPER (F01-F06)
// ============================================================================

/**
 * @brief Homework request structure
 */
typedef struct {
    int64_t student_id;
    char subject[128];
    char topic[256];
    char question[2048];
    char** context_files; // Optional context files
    int context_file_count;
    bool anti_cheat_mode; // Guide without giving answers
} HomeworkRequest;

/**
 * @brief Homework response structure
 */
typedef struct {
    char* guidance; // Socratic guidance, not direct answers
    char* hints[5]; // Progressive hints (5 levels)
    int hint_count;
    char* verification_quiz; // Quiz to verify understanding
    char* parent_log;        // Transparent log for parents
} HomeworkResponse;

/**
 * @brief Handle /homework command
 * @param request Homework request
 * @return Response or NULL on error (caller must free)
 */
HomeworkResponse* homework_command_handler(const HomeworkRequest* request);

/**
 * @brief Parse homework request from natural language
 * @param student_id Student ID
 * @param input User input
 * @return Parsed request or NULL (caller must free)
 */
HomeworkRequest* homework_parse_request(int64_t student_id, const char* input);

/**
 * @brief Anti-cheat mode: guide without giving answers
 * @param request Homework request
 * @return Guidance response (caller must free)
 */
char* homework_anti_cheat_mode(const HomeworkRequest* request);

/**
 * @brief Generate progressive hints (escalating help)
 * @param request Homework request
 * @param level Hint level (0-4, where 0 is most subtle)
 * @return Hint string (caller must free)
 */
char* homework_progressive_hints(const HomeworkRequest* request, int level);

/**
 * @brief Verify student understanding with quiz
 * @param request Homework request
 * @return Quiz in JSON format (caller must free)
 */
char* homework_verify_understanding(const HomeworkRequest* request);

/**
 * @brief Log homework interaction for parents
 * @param student_id Student ID
 * @param request Homework request
 * @param response Homework response
 * @return 0 on success, -1 on error
 */
int homework_log_for_parents(int64_t student_id, const HomeworkRequest* request,
                             const HomeworkResponse* response);

/**
 * @brief Free homework request
 */
void homework_request_free(HomeworkRequest* request);

/**
 * @brief Free homework response
 */
void homework_response_free(HomeworkResponse* response);

// ============================================================================
// STUDY SESSION (F07-F12)
// ============================================================================

/**
 * @brief Study session state
 */
typedef enum {
    SESSION_IDLE = 0,
    SESSION_WORKING = 1,
    SESSION_BREAK = 2,
    SESSION_PAUSED = 3
} StudySessionState;

/**
 * @brief Study session structure
 */
typedef struct {
    int64_t id;
    int64_t student_id;
    char subject[128];
    char topic[256];
    time_t started_at;
    time_t current_pomodoro_start;
    int pomodoro_count;
    int breaks_taken;
    int work_duration_minutes;  // Default 25
    int break_duration_minutes; // Default 5
    StudySessionState state;
    char notes[1024];
} StudySession;

/**
 * @brief Study session statistics
 */
typedef struct {
    int total_time_minutes;
    int pomodoros_completed;
    int breaks_taken;
    float focus_score; // 0.0-1.0
    int quiz_score;
    char* summary;
} StudySessionStats;

/**
 * @brief Handle /study command
 * @param student_id Student ID
 * @param subject Subject name
 * @param topic Topic name
 * @return Session ID on success, -1 on error
 */
int64_t study_command_handler(int64_t student_id, const char* subject, const char* topic);

/**
 * @brief Pomodoro timer implementation
 * @param session_id Session ID
 * @param is_break True for break timer, false for work timer
 * @return 0 on success, -1 on error
 */
int pomodoro_timer(int64_t session_id, bool is_break);

/**
 * @brief Send native macOS notification for breaks
 * @param title Notification title
 * @param message Notification message
 * @return 0 on success, -1 on error
 */
int native_notification(const char* title, const char* message);

/**
 * @brief Quick review quiz at end of session
 * @param session_id Session ID
 * @return Quiz in JSON format (caller must free)
 */
char* session_end_quiz(int64_t session_id);

/**
 * @brief Track time spent per subject
 * @param student_id Student ID
 * @param subject Subject name
 * @param minutes Minutes spent
 * @return 0 on success, -1 on error
 */
int session_track_time(int64_t student_id, const char* subject, int minutes);

/**
 * @brief Get active study session
 * @param student_id Student ID
 * @return Active session or NULL (caller must free)
 */
StudySession* study_session_get_active(int64_t student_id);

/**
 * @brief End study session
 * @param session_id Session ID
 * @param stats Session statistics
 * @return 0 on success, -1 on error
 */
int study_session_end(int64_t session_id, const StudySessionStats* stats);

/**
 * @brief Free study session
 */
void study_session_free(StudySession* session);

/**
 * @brief Free study session stats
 */
void study_session_stats_free(StudySessionStats* stats);

// ============================================================================
// PROGRESS TRACKING (F13-F17)
// ============================================================================

/**
 * @brief Progress dashboard data
 */
typedef struct {
    int64_t student_id;
    int total_study_hours;
    int current_streak_days;
    int topics_completed;
    int topics_in_progress;
    float avg_quiz_score;
    char* subject_breakdown; // JSON with per-subject stats
    char* weekly_activity;   // JSON with daily activity
    char* skill_radar;       // JSON for skill radar chart
} ProgressDashboard;

/**
 * @brief Topic status
 */
typedef enum {
    TOPIC_NOT_STARTED = 0,
    TOPIC_IN_PROGRESS = 1,
    TOPIC_COMPLETED = 2,
    TOPIC_MASTERED = 3
} TopicStatus;

/**
 * @brief Topic progress
 */
typedef struct {
    char subject[128];
    char topic[256];
    TopicStatus status;
    float skill_level; // 0.0-1.0
    int time_spent_minutes;
    int quiz_attempts;
    float quiz_avg_score;
    time_t last_activity;
} TopicProgress;

/**
 * @brief Display progress dashboard
 * @param student_id Student ID
 * @return Dashboard data (caller must free)
 */
ProgressDashboard* progress_dashboard(int64_t student_id);

/**
 * @brief Track completed topics
 * @param student_id Student ID
 * @return Array of completed topics (caller must free)
 */
TopicProgress** progress_track_topics(int64_t student_id, int* count);

/**
 * @brief AI recommendation for next topic
 * @param student_id Student ID
 * @return Recommended topic name (caller must free)
 */
char* progress_suggest_next(int64_t student_id);

/**
 * @brief Generate parent report (PDF/email summary)
 * @param student_id Student ID
 * @param format "pdf" or "email"
 * @return File path or email content (caller must free)
 */
char* progress_parent_report(int64_t student_id, const char* format);

/**
 * @brief Free progress dashboard
 */
void progress_dashboard_free(ProgressDashboard* dashboard);

/**
 * @brief Free topic progress array
 */
void progress_topic_list_free(TopicProgress** topics, int count);

#ifdef __cplusplus
}
#endif

#endif /* EDUCATION_FEATURES_H */
