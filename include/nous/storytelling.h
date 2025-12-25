/**
 * @file storytelling.h
 * @brief CONVERGIO EDUCATION - STORYTELLING ENGINE API
 *
 * Chris Maestro's storytelling support system.
 * Provides analysis tools for public speaking practice:
 * - Filler word detection (ST06)
 * - Pacing analysis (ST07)
 * - 18-minute TED timer (ST05)
 * - Story hook suggestions (XS01-XS05)
 *
 * Phase: FASE 12 - Storytelling Integration
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#ifndef CONVERGIO_STORYTELLING_H
#define CONVERGIO_STORYTELLING_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONSTANTS
// ============================================================================

#define TED_TOTAL_TIME_SECONDS     1080  // 18 minutes
#define TED_SECTION_HOOK_SECONDS   180   // 3 minutes
#define TED_SECTION_MAIN_SECONDS   180   // 3 minutes
#define TED_SECTION_PILLARS_SECONDS 540  // 9 minutes
#define TED_SECTION_ACTION_SECONDS 180   // 3 minutes

// ============================================================================
// FILLER WORD ANALYSIS (ST06)
// ============================================================================

/**
 * Individual filler word count
 */
typedef struct {
    char* word;
    int count;
} FillerCount;

/**
 * Filler word analysis result
 */
typedef struct {
    FillerCount* fillers;         // Array of filler words found
    int filler_count;             // Number of unique filler words
    int total_filler_words;       // Total filler word occurrences
    int total_words;              // Total words in text
    float filler_percentage;      // Percentage of filler words
} FillerAnalysis;

/**
 * Analyze filler words in speech transcript
 * @param text Speech transcript text
 * @param language "it" for Italian, "en" for English
 * @return Analysis result (caller must free with storytelling_free_filler_analysis)
 */
FillerAnalysis* storytelling_analyze_fillers(const char* text, const char* language);

/**
 * Free filler analysis result
 */
void storytelling_free_filler_analysis(FillerAnalysis* analysis);

/**
 * Get feedback message based on filler percentage
 * @param percentage Filler word percentage (0-100)
 * @return Feedback message (do not free)
 */
const char* storytelling_filler_feedback(float percentage);

// ============================================================================
// PACING ANALYSIS (ST07)
// ============================================================================

/**
 * Pacing analysis result
 */
typedef struct {
    int words_per_minute;
    const char* rating;           // "too_slow", "slow", "ideal", "fast", "too_fast"
    const char* suggestion;       // Improvement suggestion
    int total_words;
    int duration_seconds;
} PacingAnalysis;

/**
 * Analyze speaking pace
 * @param text Speech transcript
 * @param duration_seconds Duration of speech in seconds
 * @return Analysis result (caller must free with storytelling_free_pacing_analysis)
 */
PacingAnalysis* storytelling_analyze_pacing(const char* text, int duration_seconds);

/**
 * Free pacing analysis result
 */
void storytelling_free_pacing_analysis(PacingAnalysis* analysis);

// ============================================================================
// TED TIMER (ST05)
// ============================================================================

/**
 * TED timer state
 */
typedef struct {
    int elapsed_seconds;
    int remaining_seconds;
    const char* current_section;  // "hook", "main_idea", "three_pillars", "call_to_action", "overtime"
    int section_remaining;
    float progress_percent;
    bool overtime;
} TEDTimerState;

/**
 * Start TED talk timer
 */
void storytelling_ted_timer_start(void);

/**
 * Get current TED timer state
 */
TEDTimerState storytelling_ted_timer_state(void);

/**
 * Stop TED timer
 */
void storytelling_ted_timer_stop(void);

/**
 * Get Italian label for section
 * @param section Section identifier
 * @return Human-readable label in Italian
 */
const char* storytelling_section_label(const char* section);

// ============================================================================
// STORY HOOKS (XS01-XS05)
// ============================================================================

/**
 * Get a random story hook for a maestro
 * @param maestro_id Maestro identifier (e.g., "socrate-filosofia")
 * @return Story hook text (do not free), or NULL if not found
 */
const char* storytelling_get_hook(const char* maestro_id);

/**
 * Get all story hooks for a maestro
 * @param maestro_id Maestro identifier
 * @return NULL-terminated array of hooks (do not free), or NULL if not found
 */
const char** storytelling_get_all_hooks(const char* maestro_id);

// ============================================================================
// H.A.I.L. FRAMEWORK
// ============================================================================

/**
 * H.A.I.L. qualities for public speaking
 */
typedef enum {
    HAIL_HONESTY = 0,       // Be authentic and transparent
    HAIL_AUTHENTICITY = 1,  // Be yourself, don't imitate
    HAIL_INTEGRITY = 2,     // Be consistent with values
    HAIL_LOVE = 3           // Genuine interest in audience
} HAILQuality;

/**
 * Get description of a H.A.I.L. quality
 */
const char* storytelling_hail_description(HAILQuality quality);

/**
 * Evaluate speech for H.A.I.L. qualities
 * @param text Speech transcript
 * @param scores Output: scores for each quality (0.0-1.0)
 * @return Overall H.A.I.L. score (0.0-1.0)
 */
float storytelling_evaluate_hail(const char* text, float scores[4]);

// ============================================================================
// C.N.E.P.R. STRUCTURE
// ============================================================================

/**
 * C.N.E.P.R. presentation sections
 */
typedef enum {
    CNEPR_CONNECTION = 0,   // 2 min - Capture attention
    CNEPR_NARRATION = 1,    // 5 min - Personal story
    CNEPR_EXPLANATION = 2,  // 6 min - Core concept
    CNEPR_PERSUASION = 3,   // 3 min - Convince with data
    CNEPR_REVELATION = 4    // 2 min - Call to action
} CNEPRSection;

/**
 * Get recommended duration for a C.N.E.P.R. section
 * @param section Section type
 * @return Duration in seconds
 */
int storytelling_cnepr_duration(CNEPRSection section);

/**
 * Get section name in Italian
 */
const char* storytelling_cnepr_name(CNEPRSection section);

/**
 * Get tips for a C.N.E.P.R. section
 */
const char* storytelling_cnepr_tips(CNEPRSection section);

#ifdef __cplusplus
}
#endif

#endif // CONVERGIO_STORYTELLING_H
