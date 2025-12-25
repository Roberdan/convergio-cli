/**
 * CONVERGIO EDUCATION - ALI PRESIDE (SCHOOL PRINCIPAL)
 *
 * Extension of Ali as school principal for the Education Pack.
 * Implements FASE 7 coordination requirements:
 * - AL01: Extension of Ali role as preside
 * - AL02: Student progress dashboard
 * - AL03: Virtual class council
 * - AL04: Automatic weekly report
 * - AL05: Difficult cases management
 * - AL06: Parent communication
 *
 * Also implements:
 * - CM01: Shared student context
 * - CM02: Cross-subject signaling
 * - CM03: Interdisciplinary projects
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#include "nous/education.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define PRESIDE_MAX_REPORT_LEN 4096
#define PRESIDE_CONCERN_THRESHOLD_GRADE 5.0f
#define PRESIDE_CONCERN_THRESHOLD_TREND -1.0f
#define PRESIDE_WEEKLY_REPORT_DAYS 7

// Maestri IDs for reference (canonical slug IDs)
static const char* MAESTRI_IDS[] = {
    "socrate-filosofia",    "euclide-matematica",  "feynman-fisica",    "erodoto-storia",
    "humboldt-geografia",   "manzoni-italiano",    "darwin-scienze",    "leonardo-arte",
    "mozart-musica",        "shakespeare-inglese", "cicerone-civica",   "smith-economia",
    "lovelace-informatica", "ippocrate-corpo",     "chris-storytelling"};
static const char* MAESTRI_NAMES[] = {"Socrates", "Euclid",      "Feynman", "Herodotus",
                                      "Humboldt", "Manzoni",     "Darwin",  "Leonardo",
                                      "Mozart",   "Shakespeare", "Cicero",  "Adam Smith",
                                      "Lovelace", "Hippocrates", "Chris"};
static const char* MAESTRI_SUBJECTS[] = {"Philosophy", "Mathematics", "Physics",     "History",
                                         "Geography",  "Italian",     "Science",     "Art",
                                         "Music",      "English",     "Civics",      "Economics",
                                         "Computing",  "Health",      "Storytelling"};
static const int NUM_MAESTRI = 15;

// ============================================================================
// TYPES - Use types from education.h
// ============================================================================

// Type aliases for internal use (types defined in education.h)
typedef PresideMaestroStats MaestroStats;
typedef PresideStudentDashboard StudentDashboard;
typedef PresideConcernType ConcernType;
typedef PresideStudentConcern StudentConcern;
typedef PresideDifficultCase DifficultCase;
typedef PresideClassCouncil ClassCouncil;

// Re-define enum values for internal use
#define CONCERN_LOW_GRADE PRESIDE_CONCERN_LOW_GRADE
#define CONCERN_DECLINING_TREND PRESIDE_CONCERN_DECLINING_TREND
#define CONCERN_LOW_ENGAGEMENT PRESIDE_CONCERN_LOW_ENGAGEMENT
#define CONCERN_MISSED_GOALS PRESIDE_CONCERN_MISSED_GOALS
#define CONCERN_BREAK_STREAK PRESIDE_CONCERN_BREAK_STREAK

// External functions declared in education.h

// ============================================================================
// AL02: STUDENT DASHBOARD
// ============================================================================

/**
 * Generate a comprehensive student dashboard for the preside
 */
StudentDashboard* preside_get_dashboard(int64_t student_id) {
    EducationStudentProfile* profile = education_profile_get(student_id);
    if (!profile)
        return NULL;

    StudentDashboard* dashboard = calloc(1, sizeof(StudentDashboard));
    if (!dashboard) {
        education_profile_free(profile);
        return NULL;
    }

    dashboard->student_id = student_id;
    strncpy(dashboard->student_name, profile->name, sizeof(dashboard->student_name) - 1);

    // Get progress report for last 30 days
    time_t now = time(NULL);
    time_t from_date = now - (30 * 24 * 60 * 60);
    EducationProgressReport* report = libretto_get_progress_report(student_id, from_date, now);

    if (report) {
        dashboard->overall_average = report->overall_average;
        dashboard->total_study_hours = report->total_study_hours;
        dashboard->total_sessions = report->total_sessions;
        dashboard->goals_achieved = report->goals_achieved;
        dashboard->current_streak = report->current_streak;

        // Copy subject stats
        if (report->subject_count > 0 && report->subjects) {
            dashboard->maestro_count = report->subject_count;
            dashboard->maestro_stats = calloc(report->subject_count, sizeof(MaestroStats));
            if (dashboard->maestro_stats) {
                for (int i = 0; i < report->subject_count; i++) {
                    // Find maestro for this subject
                    for (int j = 0; j < NUM_MAESTRI; j++) {
                        if (strcmp(report->subjects[i].subject, MAESTRI_SUBJECTS[j]) == 0) {
                            strncpy(dashboard->maestro_stats[i].maestro_id, MAESTRI_IDS[j], 7);
                            strncpy(dashboard->maestro_stats[i].maestro_name, MAESTRI_NAMES[j], 31);
                            break;
                        }
                    }
                    strncpy(dashboard->maestro_stats[i].subject, report->subjects[i].subject, 31);
                    dashboard->maestro_stats[i].average_grade = report->subjects[i].average_grade;
                    dashboard->maestro_stats[i].grade_count = report->subjects[i].grade_count;
                    dashboard->maestro_stats[i].trend = report->subjects[i].trend;
                    dashboard->maestro_stats[i].study_minutes =
                        report->subjects[i].total_study_minutes;
                }
            }
        }

        libretto_report_free(report);
    }

    // Analyze concerns and strengths
    size_t concerns_size = 1024;
    size_t strengths_size = 1024;
    dashboard->concerns = malloc(concerns_size);
    dashboard->strengths = malloc(strengths_size);

    if (dashboard->concerns)
        dashboard->concerns[0] = '\0';
    if (dashboard->strengths)
        dashboard->strengths[0] = '\0';

    for (int i = 0; i < dashboard->maestro_count; i++) {
        MaestroStats* ms = &dashboard->maestro_stats[i];

        // Check for concerns
        if (ms->average_grade < PRESIDE_CONCERN_THRESHOLD_GRADE) {
            char buf[128];
            snprintf(buf, sizeof(buf), "- %s: failing average (%.1f)\n", ms->subject,
                     ms->average_grade);
            if (dashboard->concerns) {
                strncat(dashboard->concerns, buf, concerns_size - strlen(dashboard->concerns) - 1);
            }
        }
        if (ms->trend < PRESIDE_CONCERN_THRESHOLD_TREND) {
            char buf[128];
            snprintf(buf, sizeof(buf), "- %s: declining trend (%.1f)\n", ms->subject, ms->trend);
            if (dashboard->concerns) {
                strncat(dashboard->concerns, buf, concerns_size - strlen(dashboard->concerns) - 1);
            }
        }

        // Check for strengths
        if (ms->average_grade >= 8.0f) {
            char buf[128];
            snprintf(buf, sizeof(buf), "- %s: excellent (%.1f)\n", ms->subject, ms->average_grade);
            if (dashboard->strengths) {
                strncat(dashboard->strengths, buf,
                        strengths_size - strlen(dashboard->strengths) - 1);
            }
        }
    }

    education_profile_free(profile);
    return dashboard;
}

void preside_dashboard_free(StudentDashboard* dashboard) {
    if (!dashboard)
        return;
    free(dashboard->maestro_stats);
    free(dashboard->concerns);
    free(dashboard->strengths);
    free(dashboard);
}

/**
 * Print dashboard to console (ASCII format)
 */
void preside_print_dashboard(const StudentDashboard* dashboard) {
    if (!dashboard)
        return;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║             STUDENT DASHBOARD - %s                          \n",
           dashboard->student_name);
    printf("╠════════════════════════════════════════════════════════════════════╣\n");

    printf("║ Overall average: %.1f                                               \n",
           dashboard->overall_average);
    printf("║ Study hours: %d    Sessions: %d    Streak: %d days             \n",
           dashboard->total_study_hours, dashboard->total_sessions, dashboard->current_streak);
    printf("║ Goals achieved: %d                                            \n",
           dashboard->goals_achieved);

    printf("╠════════════════════════════════════════════════════════════════════╣\n");
    printf("║ PERFORMANCE BY SUBJECT                                             \n");
    printf("╠════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Subject          Teacher        Avg     Trend   Hours               \n");
    printf("╟────────────────────────────────────────────────────────────────────╢\n");

    for (int i = 0; i < dashboard->maestro_count; i++) {
        MaestroStats* ms = &dashboard->maestro_stats[i];
        const char* trend_indicator = ms->trend > 0 ? "↑" : (ms->trend < 0 ? "↓" : "→");
        printf("║ %-16s %-14s %5.1f   %s%.1f   %3d                \n", ms->subject,
               ms->maestro_name, ms->average_grade, trend_indicator, ms->trend,
               ms->study_minutes / 60);
    }

    if (dashboard->concerns && dashboard->concerns[0]) {
        printf("╠════════════════════════════════════════════════════════════════════╣\n");
        printf("║ ATTENTION NEEDED                                                    \n");
        printf("╟────────────────────────────────────────────────────────────────────╢\n");
        printf("%s", dashboard->concerns);
    }

    if (dashboard->strengths && dashboard->strengths[0]) {
        printf("╠════════════════════════════════════════════════════════════════════╣\n");
        printf("║ STRENGTHS                                                           \n");
        printf("╟────────────────────────────────────────────────────────────────────╢\n");
        printf("%s", dashboard->strengths);
    }

    printf("╚════════════════════════════════════════════════════════════════════╝\n\n");
}

// ============================================================================
// AL03: VIRTUAL CLASS COUNCIL
// ============================================================================

/**
 * Prepare a virtual class council for a student
 */
ClassCouncil* preside_prepare_class_council(int64_t student_id) {
    StudentDashboard* dashboard = preside_get_dashboard(student_id);
    if (!dashboard)
        return NULL;

    ClassCouncil* council = calloc(1, sizeof(ClassCouncil));
    if (!council) {
        preside_dashboard_free(dashboard);
        return NULL;
    }

    council->student_id = student_id;
    strncpy(council->student_name, dashboard->student_name, sizeof(council->student_name) - 1);
    council->scheduled_at = time(NULL);

    // Build agenda
    council->agenda = malloc(PRESIDE_MAX_REPORT_LEN);
    if (council->agenda) {
        snprintf(council->agenda, PRESIDE_MAX_REPORT_LEN,
                 "VIRTUAL CLASS COUNCIL - %s\n"
                 "Date: %s\n\n"
                 "AGENDA:\n"
                 "1. Overall performance analysis\n"
                 "2. Discussion of critical areas\n"
                 "3. Recognition of strengths\n"
                 "4. Intervention proposals\n"
                 "5. Parent communication\n",
                 council->student_name, ctime(&council->scheduled_at));
    }

    // Build discussion points from dashboard
    council->discussion_points = malloc(PRESIDE_MAX_REPORT_LEN);
    if (council->discussion_points) {
        snprintf(council->discussion_points, PRESIDE_MAX_REPORT_LEN,
                 "DISCUSSION POINTS:\n\n"
                 "Overall average: %.1f\n"
                 "Weekly study hours: %d\n\n"
                 "CRITICAL AREAS:\n%s\n"
                 "STRENGTHS:\n%s\n",
                 dashboard->overall_average, dashboard->total_study_hours,
                 dashboard->concerns ? dashboard->concerns : "None",
                 dashboard->strengths ? dashboard->strengths : "None");
    }

    // Generate recommendations based on analysis
    council->recommendations = malloc(PRESIDE_MAX_REPORT_LEN);
    if (council->recommendations) {
        council->recommendations[0] = '\0';

        if (dashboard->overall_average < 6.0f) {
            strncat(council->recommendations, "- Activate personalized learning support\n",
                    PRESIDE_MAX_REPORT_LEN - 1);
        }
        if (dashboard->current_streak < 3) {
            strncat(council->recommendations, "- Strengthen motivation and study consistency\n",
                    PRESIDE_MAX_REPORT_LEN - strlen(council->recommendations) - 1);
        }
        if (dashboard->concerns && strstr(dashboard->concerns, "declining trend")) {
            strncat(council->recommendations, "- Close monitoring of declining subjects\n",
                    PRESIDE_MAX_REPORT_LEN - strlen(council->recommendations) - 1);
        }
        if (council->recommendations[0] == '\0') {
            strncat(council->recommendations,
                    "- Maintain current good performance\n"
                    "- Encourage deeper exploration of strengths\n",
                    PRESIDE_MAX_REPORT_LEN - 1);
        }
    }

    preside_dashboard_free(dashboard);
    return council;
}

void preside_class_council_free(ClassCouncil* council) {
    if (!council)
        return;
    free(council->agenda);
    free(council->discussion_points);
    free(council->recommendations);
    free(council);
}

// ============================================================================
// AL04: WEEKLY REPORT
// ============================================================================

/**
 * Generate automatic weekly report
 */
char* preside_generate_weekly_report(int64_t student_id) {
    StudentDashboard* dashboard = preside_get_dashboard(student_id);
    if (!dashboard)
        return NULL;

    char* report = malloc(PRESIDE_MAX_REPORT_LEN);
    if (!report) {
        preside_dashboard_free(dashboard);
        return NULL;
    }

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%d/%m/%Y", tm_info);

    snprintf(report, PRESIDE_MAX_REPORT_LEN,
             "═══════════════════════════════════════════════\n"
             "       WEEKLY EDUCATIONAL REPORT\n"
             "═══════════════════════════════════════════════\n"
             "Student: %s\n"
             "Week of: %s\n"
             "───────────────────────────────────────────────\n\n"
             "GENERAL SUMMARY\n"
             "• Overall average: %.1f\n"
             "• Study hours: %d\n"
             "• Completed sessions: %d\n"
             "• Goals achieved: %d\n"
             "• Current streak: %d days\n\n"
             "PERFORMANCE BY SUBJECT\n",
             dashboard->student_name, date_str, dashboard->overall_average,
             dashboard->total_study_hours, dashboard->total_sessions, dashboard->goals_achieved,
             dashboard->current_streak);

    // Add per-subject details
    for (int i = 0; i < dashboard->maestro_count; i++) {
        MaestroStats* ms = &dashboard->maestro_stats[i];
        char subject_line[128];
        const char* status = ms->average_grade >= 6.0f ? "✓" : "⚠";
        snprintf(subject_line, sizeof(subject_line), "%s %-15s: %.1f (%s)\n", status, ms->subject,
                 ms->average_grade,
                 ms->trend > 0   ? "improving"
                 : ms->trend < 0 ? "declining"
                                 : "stable");
        strncat(report, subject_line, PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
    }

    // Add concerns and strengths
    if (dashboard->concerns && dashboard->concerns[0]) {
        strncat(report, "\nATTENTION REQUIRED\n", PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
        strncat(report, dashboard->concerns, PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
    }

    if (dashboard->strengths && dashboard->strengths[0]) {
        strncat(report, "\nSTRENGTHS\n", PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
        strncat(report, dashboard->strengths, PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
    }

    strncat(report, "\n───────────────────────────────────────────────\n",
            PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
    strncat(report, "Report generated by Ali (Virtual Principal)\n",
            PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);

    preside_dashboard_free(dashboard);
    return report;
}

// ============================================================================
// AL05: DIFFICULT CASES MANAGEMENT
// ============================================================================

/**
 * Detect difficult cases requiring escalation
 */
DifficultCase* preside_detect_difficult_case(int64_t student_id) {
    StudentDashboard* dashboard = preside_get_dashboard(student_id);
    if (!dashboard)
        return NULL;

    // Analyze for concerning patterns
    int concern_capacity = 10;
    StudentConcern* concerns = calloc(concern_capacity, sizeof(StudentConcern));
    int concern_count = 0;

    // Check overall average
    if (dashboard->overall_average < 5.0f) {
        concerns[concern_count].type = CONCERN_LOW_GRADE;
        strncpy(concerns[concern_count].subject, "Overall", 31);
        snprintf(concerns[concern_count].description, 255, "Severely failing overall average: %.1f",
                 dashboard->overall_average);
        concerns[concern_count].severity = 5;
        concerns[concern_count].detected_at = time(NULL);
        concern_count++;
    }

    // Check individual subjects
    for (int i = 0; i < dashboard->maestro_count && concern_count < concern_capacity; i++) {
        MaestroStats* ms = &dashboard->maestro_stats[i];

        if (ms->average_grade < 4.0f) {
            concerns[concern_count].type = CONCERN_LOW_GRADE;
            strncpy(concerns[concern_count].subject, ms->subject, 31);
            snprintf(concerns[concern_count].description, 255, "Severely failing grade in %s: %.1f",
                     ms->subject, ms->average_grade);
            concerns[concern_count].severity = 4;
            concerns[concern_count].detected_at = time(NULL);
            concern_count++;
        }

        if (ms->trend < -2.0f) {
            concerns[concern_count].type = CONCERN_DECLINING_TREND;
            strncpy(concerns[concern_count].subject, ms->subject, 31);
            snprintf(concerns[concern_count].description, 255,
                     "Strongly declining trend in %s: %.1f", ms->subject, ms->trend);
            concerns[concern_count].severity = 3;
            concerns[concern_count].detected_at = time(NULL);
            concern_count++;
        }
    }

    // Check engagement
    if (dashboard->total_study_hours < 5) {
        concerns[concern_count].type = CONCERN_LOW_ENGAGEMENT;
        strncpy(concerns[concern_count].subject, "Overall", 31);
        snprintf(concerns[concern_count].description, 255, "Very low study time: %d hours",
                 dashboard->total_study_hours);
        concerns[concern_count].severity = 3;
        concerns[concern_count].detected_at = time(NULL);
        concern_count++;
    }

    // Check streak
    if (dashboard->current_streak == 0 && dashboard->total_sessions > 10) {
        concerns[concern_count].type = CONCERN_BREAK_STREAK;
        strncpy(concerns[concern_count].subject, "Overall", 31);
        snprintf(concerns[concern_count].description, 255, "Study continuity interrupted");
        concerns[concern_count].severity = 2;
        concerns[concern_count].detected_at = time(NULL);
        concern_count++;
    }

    if (concern_count == 0) {
        free(concerns);
        preside_dashboard_free(dashboard);
        return NULL; // Not a difficult case
    }

    DifficultCase* dc = calloc(1, sizeof(DifficultCase));
    if (!dc) {
        free(concerns);
        preside_dashboard_free(dashboard);
        return NULL;
    }

    dc->student_id = student_id;
    strncpy(dc->student_name, dashboard->student_name, 63);
    dc->concerns = concerns;
    dc->concern_count = concern_count;

    preside_dashboard_free(dashboard);
    return dc;
}

void preside_difficult_case_free(DifficultCase* dc) {
    if (!dc)
        return;
    free(dc->concerns);
    free(dc);
}

// ============================================================================
// AL06: PARENT COMMUNICATION
// ============================================================================

/**
 * Generate parent communication message
 */
char* preside_generate_parent_message(int64_t student_id, bool include_concerns) {
    StudentDashboard* dashboard = preside_get_dashboard(student_id);
    if (!dashboard)
        return NULL;

    EducationStudentProfile* profile = education_profile_get(student_id);
    if (!profile) {
        preside_dashboard_free(dashboard);
        return NULL;
    }

    char* message = malloc(PRESIDE_MAX_REPORT_LEN);
    if (!message) {
        preside_dashboard_free(dashboard);
        education_profile_free(profile);
        return NULL;
    }

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%d/%m/%Y", tm_info);

    snprintf(message, PRESIDE_MAX_REPORT_LEN,
             "Dear %s,\n\n"
             "I am writing to update you on %s's academic progress.\n\n"
             "CURRENT STATUS (as of %s)\n"
             "• Overall average: %.1f\n"
             "• Weekly study hours: %d\n"
             "• Continuity: %d consecutive days\n\n",
             profile->parent_name ? profile->parent_name : "Parent", dashboard->student_name,
             date_str, dashboard->overall_average, dashboard->total_study_hours,
             dashboard->current_streak);

    if (dashboard->strengths && dashboard->strengths[0]) {
        strncat(message, "STRENGTHS\n", PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
        strncat(message, dashboard->strengths, PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
        strncat(message, "\n", PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
    }

    if (include_concerns && dashboard->concerns && dashboard->concerns[0]) {
        strncat(message, "AREAS FOR IMPROVEMENT\n", PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
        strncat(message, dashboard->concerns, PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
        strncat(message, "\n", PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
    }

    strncat(message,
            "Please feel free to reach out with any questions.\n\n"
            "Best regards,\n"
            "Ali - Virtual Principal\n"
            "Convergio Education System\n",
            PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);

    preside_dashboard_free(dashboard);
    education_profile_free(profile);
    return message;
}

// ============================================================================
// CM01-03: MAESTRI COMMUNICATION
// ============================================================================

/**
 * Get shared context for maestri about a student
 */
char* preside_get_shared_context(int64_t student_id) {
    StudentDashboard* dashboard = preside_get_dashboard(student_id);
    if (!dashboard)
        return NULL;

    EducationStudentProfile* profile = education_profile_get(student_id);
    if (!profile) {
        preside_dashboard_free(dashboard);
        return NULL;
    }

    char* context = malloc(PRESIDE_MAX_REPORT_LEN);
    if (!context) {
        preside_dashboard_free(dashboard);
        education_profile_free(profile);
        return NULL;
    }

    // Build accessibility context string
    const char* accessibility_notes = "";
    if (profile->accessibility) {
        static char a11y_buf[512];
        a11y_buf[0] = '\0';
        if (profile->accessibility->dyslexia)
            strncat(a11y_buf, "dyslexia, ", 500);
        if (profile->accessibility->dyscalculia)
            strncat(a11y_buf, "dyscalculia, ", 500 - strlen(a11y_buf));
        if (profile->accessibility->adhd)
            strncat(a11y_buf, "ADHD, ", 500 - strlen(a11y_buf));
        if (profile->accessibility->autism)
            strncat(a11y_buf, "autism, ", 500 - strlen(a11y_buf));
        if (profile->accessibility->cerebral_palsy)
            strncat(a11y_buf, "cerebral palsy, ", 500 - strlen(a11y_buf));
        if (strlen(a11y_buf) > 0)
            a11y_buf[strlen(a11y_buf) - 2] = '\0'; // Remove trailing ", "
        accessibility_notes = a11y_buf;
    }

    snprintf(context, PRESIDE_MAX_REPORT_LEN,
             "STUDENT CONTEXT (for Teachers only)\n"
             "═══════════════════════════════════════════\n"
             "Name: %s\n"
             "Age: %d years\n"
             "Curriculum: %s (Year %d)\n"
             "Accessibility: %s\n\n"
             "CURRENT STATUS\n"
             "Overall average: %.1f\n"
             "Study streak: %d days\n\n"
             "CRITICAL AREAS\n%s\n"
             "STRENGTHS\n%s\n"
             "═══════════════════════════════════════════\n"
             "NOTE: Always adapt responses to the student's\n"
             "accessibility profile.\n",
             profile->name, profile->age, profile->curriculum_id ? profile->curriculum_id : "N/A",
             profile->grade_level, strlen(accessibility_notes) > 0 ? accessibility_notes : "None",
             dashboard->overall_average, dashboard->current_streak,
             dashboard->concerns ? dashboard->concerns : "None",
             dashboard->strengths ? dashboard->strengths : "None");

    preside_dashboard_free(dashboard);
    education_profile_free(profile);
    return context;
}

/**
 * Detect cross-subject connections for interdisciplinary work
 */
char* preside_suggest_interdisciplinary(int64_t student_id, const char* topic) {
    if (!topic)
        return NULL;

    // Simple mapping of topics to related subjects
    char* suggestion = malloc(1024);
    if (!suggestion)
        return NULL;

    // This would be more sophisticated with actual topic analysis
    snprintf(suggestion, 1024,
             "INTERDISCIPLINARY SUGGESTIONS for: %s\n"
             "─────────────────────────────────────────\n"
             "Connected subjects:\n"
             "• History: historical context of the topic\n"
             "• Philosophy: ethical and philosophical implications\n"
             "• Literature: related literary works\n"
             "• Art: artistic representations\n\n"
             "Interdisciplinary project proposal:\n"
             "Involve 2-3 teachers for an in-depth exploration\n"
             "connecting different perspectives.\n",
             topic);

    return suggestion;
}

// ============================================================================
// EDUCATION WELCOME SYSTEM
// ============================================================================

/**
 * Show Ali's welcome message at startup.
 * Detects first-time users and shows personalized greeting.
 * Returns 0 on success, -1 on error.
 */
int education_show_welcome(void) {
    // Initialize education system
    if (education_init() != 0) {
        fprintf(stderr, "\033[31mError: Failed to initialize education system\033[0m\n");
        return -1;
    }

    // Check for existing profile
    EducationStudentProfile* profile = education_profile_get_active();

    printf("\n");
    printf(
        "  "
        "\033[1;38;5;135m┌─────────────────────────────────────────────────────────────┐\033[0m\n");
    printf("  \033[1;38;5;135m│\033[0m  \033[1;38;5;214m🎓 Ali, il Preside\033[0m                  "
           "                        \033[1;38;5;135m│\033[0m\n");
    printf(
        "  "
        "\033[1;38;5;135m├─────────────────────────────────────────────────────────────┤\033[0m\n");

    if (!profile) {
        // First-time user
        printf("  \033[1;38;5;135m│\033[0m                                                         "
               "    \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m  \033[1mBenvenuto a Convergio Education!\033[0m         "
               "                  \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m                                                         "
               "    \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m  Sono Ali, il tuo Preside virtuale. Sono qui per        "
               "   \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m  guidarti nel tuo percorso di apprendimento.            "
               "   \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m                                                         "
               "    \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m  Abbiamo 17 maestri straordinari pronti ad aiutarti:    "
               "   \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m  Socrate, Euclide, Feynman, Erodoto, Darwin, e altri!   "
               "   \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m                                                         "
               "    \033[1;38;5;135m│\033[0m\n");

        // Check if LLM is available for conversational onboarding
        extern bool llm_is_available(void);
        if (llm_is_available()) {
            printf("  \033[1;38;5;135m│\033[0m  \033[33mPrima di iniziare, vorrei conoscerti "
                   "meglio...\033[0m             \033[1;38;5;135m│\033[0m\n");
            printf("  \033[1;38;5;135m│\033[0m                                                     "
                   "        \033[1;38;5;135m│\033[0m\n");
            printf("  "
                   "\033[1;38;5;"
                   "135m└─────────────────────────────────────────────────────────────┘\033[0m\n");
            printf("\n");

            // Automatically start conversational onboarding with Ali
            extern bool ali_conversational_onboarding(void);
            ali_conversational_onboarding();
        } else {
            // LLM not available - show setup instructions
            printf("  \033[1;38;5;135m│\033[0m  \033[33mUsa /setup per creare il tuo profilo "
                   "studente.\033[0m            \033[1;38;5;135m│\033[0m\n");
            printf("  \033[1;38;5;135m│\033[0m  \033[2m(Configura ANTHROPIC_API_KEY per "
                   "l'onboarding AI)\033[0m         \033[1;38;5;135m│\033[0m\n");
            printf("  \033[1;38;5;135m│\033[0m                                                     "
                   "        \033[1;38;5;135m│\033[0m\n");
            printf("  "
                   "\033[1;38;5;"
                   "135m└─────────────────────────────────────────────────────────────┘\033[0m\n");
        }
    } else {
        // Returning user - personalized greeting
        const char* name = profile->name ? profile->name : "studente";

        // Get time-appropriate greeting
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        const char* greeting;
        if (tm_info->tm_hour < 12) {
            greeting = "Buongiorno";
        } else if (tm_info->tm_hour < 18) {
            greeting = "Buon pomeriggio";
        } else {
            greeting = "Buonasera";
        }

        printf("  \033[1;38;5;135m│\033[0m                                                         "
               "    \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m  \033[1m%s, %s!\033[0m", greeting, name);
        // Pad to fit the box
        int padding = 47 - (int)strlen(greeting) - (int)strlen(name);
        for (int i = 0; i < padding; i++)
            printf(" ");
        printf("\033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m                                                         "
               "    \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m  Bentornato/a nella nostra Scuola Virtuale!             "
               "   \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m  I nostri 17 maestri sono a tua disposizione.           "
               "   \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m                                                         "
               "    \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m  \033[36mCosa vorresti imparare oggi?\033[0m            "
               "                  \033[1;38;5;135m│\033[0m\n");
        printf("  \033[1;38;5;135m│\033[0m                                                         "
               "    \033[1;38;5;135m│\033[0m\n");
        printf("  "
               "\033[1;38;5;"
               "135m└─────────────────────────────────────────────────────────────┘\033[0m\n");
        printf("\n");

        // Show last session info if available
        if (profile->last_session_at > 0) {
            time_t diff = now - profile->last_session_at;
            if (diff > 86400) { // More than a day
                int days = (int)(diff / 86400);
                printf("  \033[2mÈ passato%s %d giorn%s dall'ultima sessione.\033[0m\n\n",
                       days == 1 ? "" : "no", days, days == 1 ? "o" : "i");
            }
        }
    }

    return 0;
}
