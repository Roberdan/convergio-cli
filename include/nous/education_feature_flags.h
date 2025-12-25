/**
 * CONVERGIO EDUCATION - Feature Flags Header
 *
 * Phase 4 Task 4.2: Feature flags for unverified features
 *
 * Copyright (c) 2025 Convergio.io
 */

#ifndef NOUS_EDUCATION_FEATURE_FLAGS_H
#define NOUS_EDUCATION_FEATURE_FLAGS_H

#include <stdbool.h>

/**
 * Check if a feature flag is enabled
 */
bool education_feature_flag_enabled(const char* feature_name);

/**
 * Enable a feature flag
 */
int education_feature_flag_enable(const char* feature_name);

/**
 * Disable a feature flag
 */
int education_feature_flag_disable(const char* feature_name);

/**
 * List all feature flags and their status
 */
void education_feature_flags_list(void);

#endif /* NOUS_EDUCATION_FEATURE_FLAGS_H */
