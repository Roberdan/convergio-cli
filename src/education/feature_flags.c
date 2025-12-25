/**
 * CONVERGIO EDUCATION - Feature Flags System
 *
 * Phase 4 Task 4.2: Feature flags for unverified features
 * Allows enabling/disabling features that are implemented but not fully tested
 *
 * Copyright (c) 2025 Convergio.io
 */

#include "nous/education.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Feature flag definitions
typedef enum {
    FEATURE_FLAG_VOICE_INTERACTION = 0,
    FEATURE_FLAG_FSRS_INTEGRATION,
    FEATURE_FLAG_MASTERY_TRACKING,
    FEATURE_FLAG_ACCESSIBILITY_RUNTIME,
    FEATURE_FLAG_VIDEO_SEARCH,
    FEATURE_FLAG_PERIODIC_TABLE,
    FEATURE_FLAG_PDF_EXPORT,
    FEATURE_FLAG_CERTIFICATES,
    FEATURE_FLAG_ACTIVE_BREAKS,
    FEATURE_FLAG_COUNT
} EducationFeatureFlag;

static const char* FEATURE_FLAG_NAMES[] = {"voice_interaction", "fsrs_integration",
                                           "mastery_tracking",  "accessibility_runtime",
                                           "video_search",      "periodic_table",
                                           "pdf_export",        "certificates",
                                           "active_breaks",     NULL};

// Default states (false = disabled by default for unverified features)
static bool g_feature_flags[FEATURE_FLAG_COUNT] = {
    false, // voice_interaction - not fully tested
    true,  // fsrs_integration - verified working
    false, // mastery_tracking - not fully integrated
    false, // accessibility_runtime - not fully tested
    true,  // video_search - implemented
    true,  // periodic_table - implemented
    true,  // pdf_export - implemented
    true,  // certificates - implemented
    true,  // active_breaks - implemented
};

/**
 * Check if a feature flag is enabled
 */
bool education_feature_flag_enabled(const char* feature_name) {
    if (!feature_name)
        return false;

    for (int i = 0; i < FEATURE_FLAG_COUNT; i++) {
        if (strcmp(feature_name, FEATURE_FLAG_NAMES[i]) == 0) {
            return g_feature_flags[i];
        }
    }

    return false;
}

/**
 * Enable a feature flag
 */
int education_feature_flag_enable(const char* feature_name) {
    if (!feature_name)
        return -1;

    for (int i = 0; i < FEATURE_FLAG_COUNT; i++) {
        if (strcmp(feature_name, FEATURE_FLAG_NAMES[i]) == 0) {
            g_feature_flags[i] = true;
            return 0;
        }
    }

    return -1;
}

/**
 * Disable a feature flag
 */
int education_feature_flag_disable(const char* feature_name) {
    if (!feature_name)
        return -1;

    for (int i = 0; i < FEATURE_FLAG_COUNT; i++) {
        if (strcmp(feature_name, FEATURE_FLAG_NAMES[i]) == 0) {
            g_feature_flags[i] = false;
            return 0;
        }
    }

    return -1;
}

/**
 * List all feature flags and their status
 */
void education_feature_flags_list(void) {
    printf("\n🔧 Feature Flags Status\n");
    printf("─────────────────────────────────────────────────────────────────\n");

    for (int i = 0; i < FEATURE_FLAG_COUNT; i++) {
        const char* status = g_feature_flags[i] ? "✅ ENABLED" : "❌ DISABLED";
        printf("  %-30s %s\n", FEATURE_FLAG_NAMES[i], status);
    }

    printf("─────────────────────────────────────────────────────────────────\n\n");
}
