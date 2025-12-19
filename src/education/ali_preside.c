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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define PRESIDE_MAX_REPORT_LEN 4096
#define PRESIDE_CONCERN_THRESHOLD_GRADE 5.0f
#define PRESIDE_CONCERN_THRESHOLD_TREND -1.0f
#define PRESIDE_WEEKLY_REPORT_DAYS 7

// Maestri IDs for reference
static const char* MAESTRI_IDS[] = {
    "ED01", "ED02", "ED03", "ED04", "ED05", "ED06", "ED07",
    "ED08", "ED09", "ED10", "ED11", "ED12", "ED13", "ED14"
};
static const char* MAESTRI_NAMES[] = {
    "Socrate", "Euclide", "Feynman", "Erodoto", "Humboldt", "Manzoni", "Darwin",
    "Leonardo", "Mozart", "Shakespeare", "Cicerone", "Adam Smith", "Lovelace", "Ippocrate"
};
static const char* MAESTRI_SUBJECTS[] = {
    "Filosofia", "Matematica", "Fisica", "Storia", "Geografia", "Italiano", "Scienze",
    "Arte", "Musica", "Inglese", "Ed. Civica", "Economia", "Informatica", "Sport"
};
static const int NUM_MAESTRI = 14;

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
    if (!profile) return NULL;

    StudentDashboard* dashboard = calloc(1, sizeof(StudentDashboard));
    if (!dashboard) {
        education_profile_free(profile);
        return NULL;
    }

    dashboard->student_id = student_id;
    strncpy(dashboard->student_name, profile->name,
            sizeof(dashboard->student_name) - 1);

    // Get progress report for last 30 days
    time_t now = time(NULL);
    time_t from_date = now - (30 * 24 * 60 * 60);
    EducationProgressReport* report = libretto_get_progress_report(
        student_id, from_date, now);

    if (report) {
        dashboard->overall_average = report->overall_average;
        dashboard->total_study_hours = report->total_study_hours;
        dashboard->total_sessions = report->total_sessions;
        dashboard->goals_achieved = report->goals_achieved;
        dashboard->current_streak = report->current_streak;

        // Copy subject stats
        if (report->subject_count > 0 && report->subjects) {
            dashboard->maestro_count = report->subject_count;
            dashboard->maestro_stats = calloc(report->subject_count,
                                               sizeof(MaestroStats));
            if (dashboard->maestro_stats) {
                for (int i = 0; i < report->subject_count; i++) {
                    // Find maestro for this subject
                    for (int j = 0; j < NUM_MAESTRI; j++) {
                        if (strcmp(report->subjects[i].subject,
                                   MAESTRI_SUBJECTS[j]) == 0) {
                            strncpy(dashboard->maestro_stats[i].maestro_id,
                                    MAESTRI_IDS[j], 7);
                            strncpy(dashboard->maestro_stats[i].maestro_name,
                                    MAESTRI_NAMES[j], 31);
                            break;
                        }
                    }
                    strncpy(dashboard->maestro_stats[i].subject,
                            report->subjects[i].subject, 31);
                    dashboard->maestro_stats[i].average_grade =
                        report->subjects[i].average_grade;
                    dashboard->maestro_stats[i].grade_count =
                        report->subjects[i].grade_count;
                    dashboard->maestro_stats[i].trend =
                        report->subjects[i].trend;
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

    if (dashboard->concerns) dashboard->concerns[0] = '\0';
    if (dashboard->strengths) dashboard->strengths[0] = '\0';

    for (int i = 0; i < dashboard->maestro_count; i++) {
        MaestroStats* ms = &dashboard->maestro_stats[i];

        // Check for concerns
        if (ms->average_grade < PRESIDE_CONCERN_THRESHOLD_GRADE) {
            char buf[128];
            snprintf(buf, sizeof(buf), "- %s: media insufficiente (%.1f)\n",
                     ms->subject, ms->average_grade);
            if (dashboard->concerns) {
                strncat(dashboard->concerns, buf,
                       concerns_size - strlen(dashboard->concerns) - 1);
            }
        }
        if (ms->trend < PRESIDE_CONCERN_THRESHOLD_TREND) {
            char buf[128];
            snprintf(buf, sizeof(buf), "- %s: trend in calo (%.1f)\n",
                     ms->subject, ms->trend);
            if (dashboard->concerns) {
                strncat(dashboard->concerns, buf,
                       concerns_size - strlen(dashboard->concerns) - 1);
            }
        }

        // Check for strengths
        if (ms->average_grade >= 8.0f) {
            char buf[128];
            snprintf(buf, sizeof(buf), "- %s: eccellente (%.1f)\n",
                     ms->subject, ms->average_grade);
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
    if (!dashboard) return;
    free(dashboard->maestro_stats);
    free(dashboard->concerns);
    free(dashboard->strengths);
    free(dashboard);
}

/**
 * Print dashboard to console (ASCII format)
 */
void preside_print_dashboard(const StudentDashboard* dashboard) {
    if (!dashboard) return;

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║             DASHBOARD STUDENTE - %s                          \n", dashboard->student_name);
    printf("╠════════════════════════════════════════════════════════════════════╣\n");

    printf("║ Media generale: %.1f                                               \n",
           dashboard->overall_average);
    printf("║ Ore di studio: %d    Sessioni: %d    Streak: %d giorni             \n",
           dashboard->total_study_hours, dashboard->total_sessions,
           dashboard->current_streak);
    printf("║ Obiettivi raggiunti: %d                                            \n",
           dashboard->goals_achieved);

    printf("╠════════════════════════════════════════════════════════════════════╣\n");
    printf("║ RENDIMENTO PER MATERIA                                             \n");
    printf("╠════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Materia          Maestro        Media   Trend   Ore               \n");
    printf("╟────────────────────────────────────────────────────────────────────╢\n");

    for (int i = 0; i < dashboard->maestro_count; i++) {
        MaestroStats* ms = &dashboard->maestro_stats[i];
        const char* trend_indicator = ms->trend > 0 ? "↑" : (ms->trend < 0 ? "↓" : "→");
        printf("║ %-16s %-14s %5.1f   %s%.1f   %3d                \n",
               ms->subject, ms->maestro_name, ms->average_grade,
               trend_indicator, ms->trend, ms->study_minutes / 60);
    }

    if (dashboard->concerns && dashboard->concerns[0]) {
        printf("╠════════════════════════════════════════════════════════════════════╣\n");
        printf("║ ⚠️  PUNTI DI ATTENZIONE                                             \n");
        printf("╟────────────────────────────────────────────────────────────────────╢\n");
        printf("%s", dashboard->concerns);
    }

    if (dashboard->strengths && dashboard->strengths[0]) {
        printf("╠════════════════════════════════════════════════════════════════════╣\n");
        printf("║ ⭐ PUNTI DI FORZA                                                   \n");
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
    if (!dashboard) return NULL;

    ClassCouncil* council = calloc(1, sizeof(ClassCouncil));
    if (!council) {
        preside_dashboard_free(dashboard);
        return NULL;
    }

    council->student_id = student_id;
    strncpy(council->student_name, dashboard->student_name,
            sizeof(council->student_name) - 1);
    council->scheduled_at = time(NULL);

    // Build agenda
    council->agenda = malloc(PRESIDE_MAX_REPORT_LEN);
    if (council->agenda) {
        snprintf(council->agenda, PRESIDE_MAX_REPORT_LEN,
            "CONSIGLIO DI CLASSE VIRTUALE - %s\n"
            "Data: %s\n\n"
            "ORDINE DEL GIORNO:\n"
            "1. Analisi rendimento generale\n"
            "2. Discussione aree critiche\n"
            "3. Valorizzazione punti di forza\n"
            "4. Proposte intervento\n"
            "5. Comunicazioni famiglia\n",
            council->student_name,
            ctime(&council->scheduled_at));
    }

    // Build discussion points from dashboard
    council->discussion_points = malloc(PRESIDE_MAX_REPORT_LEN);
    if (council->discussion_points) {
        snprintf(council->discussion_points, PRESIDE_MAX_REPORT_LEN,
            "PUNTI DI DISCUSSIONE:\n\n"
            "Media generale: %.1f\n"
            "Ore di studio settimanali: %d\n\n"
            "AREE CRITICHE:\n%s\n"
            "ECCELLENZE:\n%s\n",
            dashboard->overall_average,
            dashboard->total_study_hours,
            dashboard->concerns ? dashboard->concerns : "Nessuna",
            dashboard->strengths ? dashboard->strengths : "Nessuna");
    }

    // Generate recommendations based on analysis
    council->recommendations = malloc(PRESIDE_MAX_REPORT_LEN);
    if (council->recommendations) {
        council->recommendations[0] = '\0';

        if (dashboard->overall_average < 6.0f) {
            strncat(council->recommendations,
                "- Attivare supporto didattico personalizzato\n",
                PRESIDE_MAX_REPORT_LEN - 1);
        }
        if (dashboard->current_streak < 3) {
            strncat(council->recommendations,
                "- Potenziare motivazione e costanza nello studio\n",
                PRESIDE_MAX_REPORT_LEN - strlen(council->recommendations) - 1);
        }
        if (dashboard->concerns && strstr(dashboard->concerns, "trend in calo")) {
            strncat(council->recommendations,
                "- Monitoraggio ravvicinato materie in calo\n",
                PRESIDE_MAX_REPORT_LEN - strlen(council->recommendations) - 1);
        }
        if (council->recommendations[0] == '\0') {
            strncat(council->recommendations,
                "- Mantenere il buon livello raggiunto\n"
                "- Incoraggiare approfondimento nelle eccellenze\n",
                PRESIDE_MAX_REPORT_LEN - 1);
        }
    }

    preside_dashboard_free(dashboard);
    return council;
}

void preside_class_council_free(ClassCouncil* council) {
    if (!council) return;
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
    if (!dashboard) return NULL;

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
        "       REPORT SETTIMANALE EDUCATIVO\n"
        "═══════════════════════════════════════════════\n"
        "Studente: %s\n"
        "Settimana del: %s\n"
        "───────────────────────────────────────────────\n\n"
        "RIEPILOGO GENERALE\n"
        "• Media generale: %.1f\n"
        "• Ore di studio: %d\n"
        "• Sessioni completate: %d\n"
        "• Obiettivi raggiunti: %d\n"
        "• Streak attuale: %d giorni\n\n"
        "RENDIMENTO PER MATERIA\n",
        dashboard->student_name,
        date_str,
        dashboard->overall_average,
        dashboard->total_study_hours,
        dashboard->total_sessions,
        dashboard->goals_achieved,
        dashboard->current_streak);

    // Add per-subject details
    for (int i = 0; i < dashboard->maestro_count; i++) {
        MaestroStats* ms = &dashboard->maestro_stats[i];
        char subject_line[128];
        const char* status = ms->average_grade >= 6.0f ? "✓" : "⚠";
        snprintf(subject_line, sizeof(subject_line),
            "%s %-15s: %.1f (%s)\n",
            status, ms->subject, ms->average_grade,
            ms->trend > 0 ? "in crescita" :
            ms->trend < 0 ? "in calo" : "stabile");
        strncat(report, subject_line, PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
    }

    // Add concerns and strengths
    if (dashboard->concerns && dashboard->concerns[0]) {
        strncat(report, "\nATTENZIONE RICHIESTA\n",
               PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
        strncat(report, dashboard->concerns,
               PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
    }

    if (dashboard->strengths && dashboard->strengths[0]) {
        strncat(report, "\nPUNTI DI FORZA\n",
               PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
        strncat(report, dashboard->strengths,
               PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
    }

    strncat(report, "\n───────────────────────────────────────────────\n",
           PRESIDE_MAX_REPORT_LEN - strlen(report) - 1);
    strncat(report, "Report generato da Ali (Preside Virtuale)\n",
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
    if (!dashboard) return NULL;

    // Analyze for concerning patterns
    int concern_capacity = 10;
    StudentConcern* concerns = calloc(concern_capacity, sizeof(StudentConcern));
    int concern_count = 0;

    // Check overall average
    if (dashboard->overall_average < 5.0f) {
        concerns[concern_count].type = CONCERN_LOW_GRADE;
        strncpy(concerns[concern_count].subject, "Generale", 31);
        snprintf(concerns[concern_count].description, 255,
            "Media generale gravemente insufficiente: %.1f",
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
            snprintf(concerns[concern_count].description, 255,
                "Voto gravemente insufficiente in %s: %.1f",
                ms->subject, ms->average_grade);
            concerns[concern_count].severity = 4;
            concerns[concern_count].detected_at = time(NULL);
            concern_count++;
        }

        if (ms->trend < -2.0f) {
            concerns[concern_count].type = CONCERN_DECLINING_TREND;
            strncpy(concerns[concern_count].subject, ms->subject, 31);
            snprintf(concerns[concern_count].description, 255,
                "Trend in forte calo in %s: %.1f",
                ms->subject, ms->trend);
            concerns[concern_count].severity = 3;
            concerns[concern_count].detected_at = time(NULL);
            concern_count++;
        }
    }

    // Check engagement
    if (dashboard->total_study_hours < 5) {
        concerns[concern_count].type = CONCERN_LOW_ENGAGEMENT;
        strncpy(concerns[concern_count].subject, "Generale", 31);
        snprintf(concerns[concern_count].description, 255,
            "Tempo di studio molto basso: %d ore",
            dashboard->total_study_hours);
        concerns[concern_count].severity = 3;
        concerns[concern_count].detected_at = time(NULL);
        concern_count++;
    }

    // Check streak
    if (dashboard->current_streak == 0 && dashboard->total_sessions > 10) {
        concerns[concern_count].type = CONCERN_BREAK_STREAK;
        strncpy(concerns[concern_count].subject, "Generale", 31);
        snprintf(concerns[concern_count].description, 255,
            "Interruzione della continuita nello studio");
        concerns[concern_count].severity = 2;
        concerns[concern_count].detected_at = time(NULL);
        concern_count++;
    }

    if (concern_count == 0) {
        free(concerns);
        preside_dashboard_free(dashboard);
        return NULL;  // Not a difficult case
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
    if (!dc) return;
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
    if (!dashboard) return NULL;

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
        "Gentile %s,\n\n"
        "Le scrivo per aggiornarla sull'andamento scolastico di %s.\n\n"
        "SITUAZIONE ATTUALE (al %s)\n"
        "• Media generale: %.1f\n"
        "• Ore di studio settimanali: %d\n"
        "• Continuita: %d giorni consecutivi\n\n",
        profile->parent_name ? profile->parent_name : "Genitore",
        dashboard->student_name,
        date_str,
        dashboard->overall_average,
        dashboard->total_study_hours,
        dashboard->current_streak);

    if (dashboard->strengths && dashboard->strengths[0]) {
        strncat(message, "PUNTI DI FORZA\n",
               PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
        strncat(message, dashboard->strengths,
               PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
        strncat(message, "\n", PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
    }

    if (include_concerns && dashboard->concerns && dashboard->concerns[0]) {
        strncat(message, "AREE DI MIGLIORAMENTO\n",
               PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
        strncat(message, dashboard->concerns,
               PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
        strncat(message, "\n", PRESIDE_MAX_REPORT_LEN - strlen(message) - 1);
    }

    strncat(message,
        "Resto a disposizione per qualsiasi chiarimento.\n\n"
        "Cordiali saluti,\n"
        "Ali - Preside Virtuale\n"
        "Sistema Educativo Convergio\n",
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
    if (!dashboard) return NULL;

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
            strncat(a11y_buf, "dislessia, ", 500);
        if (profile->accessibility->dyscalculia)
            strncat(a11y_buf, "discalculia, ", 500 - strlen(a11y_buf));
        if (profile->accessibility->adhd)
            strncat(a11y_buf, "ADHD, ", 500 - strlen(a11y_buf));
        if (profile->accessibility->autism)
            strncat(a11y_buf, "autismo, ", 500 - strlen(a11y_buf));
        if (profile->accessibility->cerebral_palsy)
            strncat(a11y_buf, "paralisi cerebrale, ", 500 - strlen(a11y_buf));
        if (strlen(a11y_buf) > 0)
            a11y_buf[strlen(a11y_buf) - 2] = '\0';  // Remove trailing ", "
        accessibility_notes = a11y_buf;
    }

    snprintf(context, PRESIDE_MAX_REPORT_LEN,
        "CONTESTO STUDENTE (uso riservato ai Maestri)\n"
        "═══════════════════════════════════════════\n"
        "Nome: %s\n"
        "Eta: %d anni\n"
        "Curriculum: %s (Anno %d)\n"
        "Accessibilita: %s\n\n"
        "STATO ATTUALE\n"
        "Media generale: %.1f\n"
        "Streak studio: %d giorni\n\n"
        "AREE CRITICHE\n%s\n"
        "ECCELLENZE\n%s\n"
        "═══════════════════════════════════════════\n"
        "NOTA: Adattare sempre le risposte al profilo\n"
        "accessibilita dello studente.\n",
        profile->name,
        profile->age,
        profile->curriculum_id ? profile->curriculum_id : "N/D",
        profile->grade_level,
        strlen(accessibility_notes) > 0 ? accessibility_notes : "Nessuna nota",
        dashboard->overall_average,
        dashboard->current_streak,
        dashboard->concerns ? dashboard->concerns : "Nessuna",
        dashboard->strengths ? dashboard->strengths : "Nessuna");

    preside_dashboard_free(dashboard);
    education_profile_free(profile);
    return context;
}

/**
 * Detect cross-subject connections for interdisciplinary work
 */
char* preside_suggest_interdisciplinary(int64_t student_id, const char* topic) {
    if (!topic) return NULL;

    // Simple mapping of topics to related subjects
    char* suggestion = malloc(1024);
    if (!suggestion) return NULL;

    // This would be more sophisticated with actual topic analysis
    snprintf(suggestion, 1024,
        "SUGGERIMENTI INTERDISCIPLINARI per: %s\n"
        "─────────────────────────────────────────\n"
        "Materie collegate:\n"
        "• Storia: contesto storico dell'argomento\n"
        "• Filosofia: implicazioni etiche e filosofiche\n"
        "• Italiano: opere letterarie correlate\n"
        "• Arte: rappresentazioni artistiche\n\n"
        "Proposta progetto interdisciplinare:\n"
        "Coinvolgere 2-3 maestri per un approfondimento\n"
        "che colleghi diverse prospettive.\n",
        topic);

    return suggestion;
}
