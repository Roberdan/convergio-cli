/**
 * CONVERGIO UPDATER
 *
 * Auto-update system via GitHub Releases
 * Checks for updates, downloads, and applies them
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_UPDATER_H
#define CONVERGIO_UPDATER_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// UPDATE INFO STRUCTURE
// ============================================================================

typedef struct {
    char current_version[32];
    char latest_version[32];
    char download_url[512];
    char release_notes[8192];
    char published_at[32];
    size_t download_size;
    bool update_available;
    bool is_prerelease;
} UpdateInfo;

// ============================================================================
// VERSION COMPARISON
// ============================================================================

/**
 * Compare two semantic versions
 * Returns: -1 if v1 < v2, 0 if v1 == v2, 1 if v1 > v2
 */
int convergio_version_compare(const char* v1, const char* v2);

/**
 * Get current version string
 */
const char* convergio_get_version(void);

// ============================================================================
// UPDATE CHECKING
// ============================================================================

/**
 * Check GitHub for updates
 * Populates info struct with latest release details
 * Returns 0 on success, -1 on failure
 */
int convergio_check_update(UpdateInfo* info);

/**
 * Print update info to stdout
 */
void convergio_print_update_info(const UpdateInfo* info);

// ============================================================================
// UPDATE DOWNLOAD & INSTALLATION
// ============================================================================

/**
 * Download update to specified path
 * Shows progress during download
 * Returns 0 on success, -1 on failure
 */
int convergio_download_update(const UpdateInfo* info, const char* dest_path);

/**
 * Apply downloaded update
 * Backs up current binary, replaces with new one
 * Returns 0 on success, -1 on failure
 */
int convergio_apply_update(const char* new_binary_path);

/**
 * Rollback to previous version (if backup exists)
 * Returns 0 on success, -1 on failure
 */
int convergio_rollback_update(void);

// ============================================================================
// CLI COMMANDS
// ============================================================================

/**
 * Handle 'convergio update check' command
 */
int convergio_cmd_update_check(void);

/**
 * Handle 'convergio update install' command
 */
int convergio_cmd_update_install(void);

/**
 * Handle 'convergio update changelog' command
 */
int convergio_cmd_update_changelog(void);

// ============================================================================
// GITHUB API
// ============================================================================

#define CONVERGIO_GITHUB_REPO "Roberdan/convergio-cli"
#define CONVERGIO_GITHUB_API "https://api.github.com/repos/Roberdan/convergio-cli/releases/latest"

#endif // CONVERGIO_UPDATER_H
