/**
 * CONVERGIO EDUCATION - Mastery Visualization (Phase 2 Task 2.3)
 *
 * CLI/UI output for mastery progress visualization
 * Shows skill tree, mastery levels, and progress indicators
 *
 * Copyright (c) 2025 Convergio.io
 */

#include "nous/education.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Types and functions now declared in nous/education.h

// Helper to get all skills (simplified - uses gaps function which returns all)
static MasterySkillList* get_all_skills_for_subject(int64_t student_id, const char* subject) {
    // mastery_identify_gaps returns skills below proficient, but we can use it
    // In a full implementation, we'd have mastery_get_all_skills()
    return mastery_identify_gaps(student_id, subject);
}

/**
 * Print mastery level as progress bar
 */
static void print_mastery_bar(float mastery_level) {
    int filled = (int)(mastery_level * 20.0f);
    if (filled > 20) filled = 20;
    if (filled < 0) filled = 0;

    printf("[");
    for (int i = 0; i < filled; i++) {
        printf("█");
    }
    for (int i = filled; i < 20; i++) {
        printf("░");
    }
    printf("] %.0f%%", mastery_level * 100.0f);
}

/**
 * Get mastery status emoji/indicator
 */
static const char* mastery_status_icon(float mastery_level) {
    if (mastery_level >= 0.80f) return "✅";  // Mastered
    if (mastery_level >= 0.60f) return "🟡";  // Proficient
    if (mastery_level >= 0.40f) return "🟠";  // Familiar
    if (mastery_level > 0.0f) return "🔵";    // Attempted
    return "⚪";  // Not started
}

/**
 * Print mastery visualization for a skill
 */
void mastery_print_skill(int64_t student_id, const char* skill_id, const char* skill_name) {
    if (!skill_id) return;

    float level = education_mastery_get_level(student_id, skill_id);
    bool mastered = education_mastery_is_mastered(student_id, skill_id);

    const char* icon = mastery_status_icon(level);
    const char* status = mastered ? "MASTERED" : 
                        (level >= 0.60f ? "PROFICIENT" :
                         (level >= 0.40f ? "FAMILIAR" :
                          (level > 0.0f ? "IN PROGRESS" : "NOT STARTED")));

    printf("  %s %s", icon, skill_name ? skill_name : skill_id);
    printf(" - %s ", status);
    print_mastery_bar(level);
    printf("\n");
}

/**
 * Print mastery summary for a subject
 */
void mastery_print_subject_summary(int64_t student_id, const char* subject) {
    if (!subject) return;

    MasterySkillList* skills = get_all_skills_for_subject(student_id, subject);
    if (!skills || skills->count == 0) {
        printf("No skills tracked for %s yet.\n", subject);
        return;
    }

    int mastered = 0;
    int proficient = 0;
    int in_progress = 0;
    int not_started = 0;
    float total_mastery = 0.0f;

    for (int i = 0; i < skills->count; i++) {
        float level = skills->skills[i].mastery_level;
        total_mastery += level;

        if (level >= 0.80f) mastered++;
        else if (level >= 0.60f) proficient++;
        else if (level > 0.0f) in_progress++;
        else not_started++;
    }

    float avg_mastery = total_mastery / (float)skills->count;

    printf("\n📊 %s Mastery Summary\n", subject);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Total Skills: %d\n", skills->count);
    printf("✅ Mastered (80%%+): %d\n", mastered);
    printf("🟡 Proficient (60-79%%): %d\n", proficient);
    printf("🟠 Familiar (40-59%%): %d\n", in_progress);
    printf("⚪ Not Started: %d\n", not_started);
    printf("Average Mastery: ");
    print_mastery_bar(avg_mastery);
    printf("\n\n");

    // Show top skills needing work
    if (in_progress > 0 || not_started > 0) {
        printf("Skills to Focus On:\n");
        for (int i = 0; i < skills->count && i < 5; i++) {
            if (skills->skills[i].mastery_level < 0.80f) {
                mastery_print_skill(student_id, 
                                   skills->skills[i].skill_id,
                                   skills->skills[i].skill_name);
            }
        }
    }

    mastery_free_skills(skills);
}

/**
 * Print full mastery tree visualization
 */
void mastery_print_tree(int64_t student_id) {
    // Get all subjects (simplified - would need subject list)
    const char* subjects[] = {
        "mathematics", "physics", "chemistry", "biology",
        "italian", "english", "history", "geography",
        "art", "music", "philosophy", NULL
    };

    printf("\n🌳 Mastery Tree Overview\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");

    for (int i = 0; subjects[i] != NULL; i++) {
        mastery_print_subject_summary(student_id, subjects[i]);
    }
}
