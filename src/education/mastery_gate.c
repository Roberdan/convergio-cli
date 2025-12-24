/**
 * CONVERGIO EDUCATION - Mastery Gate (80% Threshold Enforcement)
 *
 * Phase 2 Task 2.2: Wire Mastery into progress
 * Blocks student advancement until 80% mastery is achieved
 *
 * Copyright (c) 2025 Convergio.io
 */

#include "nous/education.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration
extern bool education_mastery_is_mastered(int64_t student_id, const char* skill_id);
extern float education_mastery_get_level(int64_t student_id, const char* skill_id);

/**
 * Check if student can advance to next topic/skill
 * Returns true if prerequisite skills are mastered (80%+)
 */
bool mastery_can_advance(int64_t student_id, const char* target_skill_id,
                         const char** prerequisite_skills, int prereq_count) {
    if (!target_skill_id || !prerequisite_skills) {
        return false;
    }

    // Check all prerequisites are mastered
    for (int i = 0; i < prereq_count; i++) {
        if (!prerequisite_skills[i])
            continue;

        if (!education_mastery_is_mastered(student_id, prerequisite_skills[i])) {
            return false; // Block advancement - prerequisite not mastered
        }
    }

    return true; // All prerequisites mastered, can advance
}

/**
 * Get mastery level for a skill (0.0 - 1.0)
 */
float mastery_get_level(int64_t student_id, const char* skill_id) {
    if (!skill_id)
        return 0.0f;
    return education_mastery_get_level(student_id, skill_id);
}

/**
 * Check if skill is mastered (80%+)
 */
bool mastery_is_mastered(int64_t student_id, const char* skill_id) {
    if (!skill_id)
        return false;
    return education_mastery_is_mastered(student_id, skill_id);
}

/**
 * Get blocking message if student cannot advance
 * Returns message string (caller must free) or NULL if can advance
 */
char* mastery_get_blocking_message(int64_t student_id, const char* target_skill_id,
                                   const char** prerequisite_skills, int prereq_count) {
    if (mastery_can_advance(student_id, target_skill_id, prerequisite_skills, prereq_count)) {
        return NULL; // Can advance, no blocking
    }

    // Find which prerequisites are not mastered
    char* message = malloc(1024);
    if (!message)
        return NULL;

    snprintf(message, 1024,
             "You need to master the prerequisites first (80%% required).\n"
             "Missing mastery in:\n");

    for (int i = 0; i < prereq_count; i++) {
        if (!prerequisite_skills[i])
            continue;

        if (!education_mastery_is_mastered(student_id, prerequisite_skills[i])) {
            float level = education_mastery_get_level(student_id, prerequisite_skills[i]);
            char line[256];
            snprintf(line, sizeof(line), "  • %s: %.0f%% (need 80%%)\n", prerequisite_skills[i],
                     level * 100.0f);
            strncat(message, line, 1024 - strlen(message) - 1);
        }
    }

    return message;
}
