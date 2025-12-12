/**
 * CONVERGIO UPDATER
 *
 * Auto-update system via GitHub Releases
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/updater.h"
#include "nous/nous.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <spawn.h>

extern char **environ;

// Version is defined at compile time
#ifndef CONVERGIO_VERSION
#define CONVERGIO_VERSION "0.0.0"
#endif

// ============================================================================
// SECURITY HELPERS
// ============================================================================

/**
 * Validate version string to prevent command injection
 * Only allows: alphanumeric, dots, dashes
 */
static int is_safe_version_string(const char* str) {
    if (!str || !*str) return 0;
    for (const char* p = str; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '.' && *p != '-' && *p != '_') {
            return 0;
        }
    }
    return 1;
}

/**
 * Safe command execution using posix_spawn (no shell)
 * Returns 0 on success, -1 on failure
 */
static int safe_exec(char* const argv[]) {
    pid_t pid;
    int status;

    if (posix_spawn(&pid, argv[0], NULL, NULL, argv, environ) != 0) {
        return -1;
    }

    if (waitpid(pid, &status, 0) == -1) {
        return -1;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
}

// ============================================================================
// CURL HELPERS
// ============================================================================

typedef struct {
    char* data;
    size_t size;
} ResponseBuffer;

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    ResponseBuffer* buf = (ResponseBuffer*)userp;

    char* ptr = realloc(buf->data, buf->size + realsize + 1);
    if (!ptr) {
        return 0;
    }

    buf->data = ptr;
    memcpy(&(buf->data[buf->size]), contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = '\0';

    return realsize;
}

static size_t write_file_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    return fwrite(contents, size, nmemb, (FILE*)userp);
}

// ============================================================================
// JSON PARSING (minimal, just for GitHub API response)
// ============================================================================

static const char* find_json_string(const char* json, const char* key, char* value, size_t value_size) {
    char search_key[128];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    const char* pos = strstr(json, search_key);
    if (!pos) return NULL;

    pos = strchr(pos + strlen(search_key), ':');
    if (!pos) return NULL;

    // Skip whitespace
    while (*pos && (*pos == ':' || *pos == ' ' || *pos == '\t')) pos++;

    if (*pos != '"') return NULL;
    pos++;  // Skip opening quote

    // Find closing quote
    const char* end = strchr(pos, '"');
    if (!end) return NULL;

    size_t len = end - pos;
    if (len >= value_size) len = value_size - 1;

    strncpy(value, pos, len);
    value[len] = '\0';

    return end;
}

// ============================================================================
// VERSION COMPARISON
// ============================================================================

int convergio_version_compare(const char* v1, const char* v2) {
    int major1, minor1, patch1;
    int major2, minor2, patch2;

    if (sscanf(v1, "%d.%d.%d", &major1, &minor1, &patch1) != 3) {
        return 0;
    }
    if (sscanf(v2, "%d.%d.%d", &major2, &minor2, &patch2) != 3) {
        return 0;
    }

    if (major1 != major2) return (major1 > major2) ? 1 : -1;
    if (minor1 != minor2) return (minor1 > minor2) ? 1 : -1;
    if (patch1 != patch2) return (patch1 > patch2) ? 1 : -1;

    return 0;
}

const char* convergio_get_version(void) {
    return CONVERGIO_VERSION;
}

// ============================================================================
// UPDATE CHECKING
// ============================================================================

int convergio_check_update(UpdateInfo* info) {
    if (!info) return -1;

    memset(info, 0, sizeof(UpdateInfo));
    strncpy(info->current_version, CONVERGIO_VERSION, sizeof(info->current_version) - 1);

    CURL* curl = curl_easy_init();
    if (!curl) {
        return -1;
    }

    ResponseBuffer response = {0};
    response.data = malloc(1);
    response.data[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, CONVERGIO_GITHUB_API);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Convergio-CLI/" CONVERGIO_VERSION);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(response.data);
        return -1;
    }

    // Parse response
    char tag_name[32] = {0};
    find_json_string(response.data, "tag_name", tag_name, sizeof(tag_name));

    // Remove 'v' prefix if present
    const char* version = tag_name;
    if (tag_name[0] == 'v') {
        version = tag_name + 1;
    }
    strncpy(info->latest_version, version, sizeof(info->latest_version) - 1);

    // Get other fields
    find_json_string(response.data, "published_at", info->published_at, sizeof(info->published_at));

    // Look for download URL (arm64 darwin tarball)
    // Search for the full JSON key including quotes so find_json_string works correctly
    char* browser_url_start = strstr(response.data, "\"browser_download_url\"");
    while (browser_url_start) {
        char url[512] = {0};
        find_json_string(browser_url_start, "browser_download_url", url, sizeof(url));
        if (strstr(url, "arm64") && strstr(url, "darwin") && strstr(url, ".tar.gz")) {
            strncpy(info->download_url, url, sizeof(info->download_url) - 1);
            break;
        }
        browser_url_start = strstr(browser_url_start + 1, "\"browser_download_url\"");
    }

    // Get release notes (body)
    find_json_string(response.data, "body", info->release_notes, sizeof(info->release_notes));

    // Check if prerelease
    if (strstr(response.data, "\"prerelease\":true") || strstr(response.data, "\"prerelease\": true")) {
        info->is_prerelease = true;
    }

    free(response.data);

    // Compare versions
    info->update_available = (convergio_version_compare(info->latest_version, info->current_version) > 0);

    return 0;
}

void convergio_print_update_info(const UpdateInfo* info) {
    if (!info) return;

    printf("\n");
    printf("Current version: %s\n", info->current_version);
    printf("Latest version:  %s", info->latest_version);
    if (info->is_prerelease) {
        printf(" (prerelease)");
    }
    printf("\n");

    if (info->update_available) {
        printf("\n\033[33m⚡ Update available!\033[0m\n");
        printf("Run 'convergio update install' to update.\n");
    } else {
        printf("\n\033[32m✓ You're up to date.\033[0m\n");
    }
    printf("\n");
}

// ============================================================================
// UPDATE DOWNLOAD & INSTALLATION
// ============================================================================

int convergio_download_update(const UpdateInfo* info, const char* dest_path) {
    if (!info || !dest_path) {
        fprintf(stderr, "\033[31mError: Invalid update info\033[0m\n");
        return -1;
    }
    if (strlen(info->download_url) == 0) {
        fprintf(stderr, "\033[31mError: No download URL found for your platform (arm64-darwin)\033[0m\n");
        fprintf(stderr, "\033[33mPlease update manually:\033[0m brew upgrade convergio\n\n");
        return -1;
    }

    printf("Downloading update from:\n%s\n\n", info->download_url);

    FILE* fp = fopen(dest_path, "wb");
    if (!fp) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Cannot write to %s", dest_path);
        return -1;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        fclose(fp);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, info->download_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Convergio-CLI/" CONVERGIO_VERSION);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);  // Show progress

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        unlink(dest_path);
        LOG_ERROR(LOG_CAT_SYSTEM, "Download failed: %s", curl_easy_strerror(res));
        return -1;
    }

    printf("\nDownload complete: %s\n", dest_path);
    return 0;
}

int convergio_apply_update(const char* new_binary_path) {
    if (!new_binary_path) return -1;

    // Get current binary path
    char current_path[1024];
    uint32_t size = sizeof(current_path);

    #ifdef __APPLE__
    if (_NSGetExecutablePath(current_path, &size) != 0) {
        fprintf(stderr, "\033[31mError: Cannot determine current executable path\033[0m\n");
        return -1;
    }
    #else
    ssize_t len = readlink("/proc/self/exe", current_path, sizeof(current_path) - 1);
    if (len == -1) {
        fprintf(stderr, "\033[31mError: Cannot determine current executable path\033[0m\n");
        return -1;
    }
    current_path[len] = '\0';
    #endif

    // Check if installed via Homebrew
    if (strstr(current_path, "/homebrew/") || strstr(current_path, "/Cellar/")) {
        fprintf(stderr, "\033[33mHomebrew installation detected.\033[0m\n");
        fprintf(stderr, "Please update using: \033[1mbrew upgrade convergio\033[0m\n\n");
        return -1;
    }

    // Create backup
    char backup_path[1100];
    snprintf(backup_path, sizeof(backup_path), "%s.backup", current_path);

    printf("Creating backup: %s\n", backup_path);
    if (rename(current_path, backup_path) != 0) {
        fprintf(stderr, "\033[31mError: Cannot create backup (permission denied?)\033[0m\n");
        fprintf(stderr, "Try: sudo convergio update install\n");
        return -1;
    }

    // Move new binary to current path
    printf("Installing new version...\n");
    if (rename(new_binary_path, current_path) != 0) {
        // Restore backup
        rename(backup_path, current_path);
        fprintf(stderr, "\033[31mError: Cannot install new version (permission denied?)\033[0m\n");
        fprintf(stderr, "Try: sudo convergio update install\n");
        return -1;
    }

    // Make executable
    chmod(current_path, 0755);

    printf("\n\033[32m✓ Update installed successfully!\033[0m\n");
    printf("Restart Convergio to use the new version.\n");

    return 0;
}

int convergio_rollback_update(void) {
    char current_path[1024];
    uint32_t size = sizeof(current_path);

    #ifdef __APPLE__
    if (_NSGetExecutablePath(current_path, &size) != 0) {
        return -1;
    }
    #else
    return -1;
    #endif

    char backup_path[1100];
    snprintf(backup_path, sizeof(backup_path), "%s.backup", current_path);

    struct stat st;
    if (stat(backup_path, &st) != 0) {
        LOG_ERROR(LOG_CAT_SYSTEM, "No backup found at %s", backup_path);
        return -1;
    }

    // Remove current and restore backup
    unlink(current_path);
    if (rename(backup_path, current_path) != 0) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Cannot restore backup");
        return -1;
    }

    printf("Rolled back to previous version.\n");
    return 0;
}

// ============================================================================
// CLI COMMANDS
// ============================================================================

// Internal: perform update given info (no confirmation)
static int convergio_do_update_install(const UpdateInfo* info) {
    printf("\nInstalling update: %s -> %s\n\n", info->current_version, info->latest_version);

    // SECURITY: Validate version string to prevent command injection
    if (!is_safe_version_string(info->latest_version)) {
        fprintf(stderr, "\033[31mError: Invalid version string format\033[0m\n");
        return -1;
    }

    // Download to temp location
    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "/tmp/convergio-update-%s.tar.gz", info->latest_version);

    if (convergio_download_update(info, temp_path) != 0) {
        return -1;
    }

    // Extract tarball using posix_spawn (no shell injection possible)
    char extract_dir[256];
    snprintf(extract_dir, sizeof(extract_dir), "/tmp/convergio-update-%s", info->latest_version);

    // Create directory using mkdir (no shell)
    printf("Extracting update...\n");
    char* mkdir_args[] = {"/bin/mkdir", "-p", extract_dir, NULL};
    if (safe_exec(mkdir_args) != 0) {
        fprintf(stderr, "\033[31mError: Failed to create extraction directory\033[0m\n");
        unlink(temp_path);
        return -1;
    }

    // Extract using tar (no shell)
    char* tar_args[] = {"/usr/bin/tar", "-xzf", temp_path, "-C", extract_dir, NULL};
    if (safe_exec(tar_args) != 0) {
        fprintf(stderr, "\033[31mError: Failed to extract update\033[0m\n");
        unlink(temp_path);
        return -1;
    }

    // Apply update
    char new_binary[512];
    snprintf(new_binary, sizeof(new_binary), "%s/convergio", extract_dir);

    if (convergio_apply_update(new_binary) != 0) {
        unlink(temp_path);
        return -1;
    }

    // Cleanup using rm (no shell)
    unlink(temp_path);
    char* rm_args[] = {"/bin/rm", "-rf", extract_dir, NULL};
    (void)safe_exec(rm_args);  // Ignore cleanup errors

    return 0;
}

int convergio_cmd_update_check(void) {
    printf("Checking for updates...\n");

    UpdateInfo info;
    if (convergio_check_update(&info) != 0) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Could not check for updates");
        LOG_INFO(LOG_CAT_SYSTEM, "Please check your internet connection");
        return -1;
    }

    printf("\n");
    printf("Current version: %s\n", info.current_version);
    printf("Latest version:  %s", info.latest_version);
    if (info.is_prerelease) {
        printf(" (prerelease)");
    }
    printf("\n");

    if (info.update_available) {
        printf("\n\033[33m⚡ Update available!\033[0m\n\n");
        printf("Do you want to install it? (y/N): ");
        fflush(stdout);

        char response[16];
        if (fgets(response, sizeof(response), stdin)) {
            if (response[0] == 'y' || response[0] == 'Y') {
                return convergio_do_update_install(&info);
            }
        }
        printf("Update skipped. Run 'convergio update' again to install.\n");
    } else {
        printf("\n\033[32m✓ You're up to date.\033[0m\n");
    }
    printf("\n");
    return 0;
}

int convergio_cmd_update_install(void) {
    printf("Checking for updates...\n");

    UpdateInfo info;
    if (convergio_check_update(&info) != 0) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Could not check for updates");
        return -1;
    }

    if (!info.update_available) {
        printf("\n\033[32m✓ You're already running the latest version (%s).\033[0m\n\n",
               info.current_version);
        return 0;
    }

    printf("\nUpdate available: %s -> %s\n", info.current_version, info.latest_version);
    printf("Do you want to install it? (y/N): ");
    fflush(stdout);

    char response[16];
    if (!fgets(response, sizeof(response), stdin)) {
        return -1;
    }

    if (response[0] != 'y' && response[0] != 'Y') {
        printf("Update cancelled.\n");
        return 0;
    }

    return convergio_do_update_install(&info);
}

int convergio_cmd_update_changelog(void) {
    UpdateInfo info;
    if (convergio_check_update(&info) != 0) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Could not fetch changelog");
        return -1;
    }

    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  Changelog for v%-37s ║\n", info.latest_version);
    printf("╠══════════════════════════════════════════════════════╣\n");

    if (strlen(info.release_notes) > 0) {
        // Print release notes with word wrap
        const char* p = info.release_notes;
        while (*p) {
            printf("║  ");
            int col = 0;
            while (*p && *p != '\n' && col < 52) {
                putchar(*p);
                p++;
                col++;
            }
            while (col < 52) {
                putchar(' ');
                col++;
            }
            printf(" ║\n");
            if (*p == '\n') p++;
        }
    } else {
        printf("║  No release notes available.                         ║\n");
    }

    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}

// ============================================================================
// FETCH RELEASE BY VERSION
// ============================================================================

int convergio_fetch_release(const char* version, UpdateInfo* info) {
    if (!info) return -1;

    memset(info, 0, sizeof(UpdateInfo));
    strncpy(info->current_version, CONVERGIO_VERSION, sizeof(info->current_version) - 1);

    CURL* curl = curl_easy_init();
    if (!curl) {
        return -1;
    }

    ResponseBuffer response = {0};
    response.data = malloc(1);
    response.data[0] = '\0';

    // Build URL - either latest or specific version
    char url[256];
    if (version && *version) {
        // Ensure version has 'v' prefix for GitHub tags
        if (version[0] == 'v') {
            snprintf(url, sizeof(url),
                "https://api.github.com/repos/%s/releases/tags/%s",
                CONVERGIO_GITHUB_REPO, version);
        } else {
            snprintf(url, sizeof(url),
                "https://api.github.com/repos/%s/releases/tags/v%s",
                CONVERGIO_GITHUB_REPO, version);
        }
    } else {
        strncpy(url, CONVERGIO_GITHUB_API, sizeof(url) - 1);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Convergio-CLI/" CONVERGIO_VERSION);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code != 200) {
        free(response.data);
        return -1;
    }

    // Parse response
    char tag_name[32] = {0};
    find_json_string(response.data, "tag_name", tag_name, sizeof(tag_name));

    // Remove 'v' prefix if present
    const char* ver = tag_name;
    if (tag_name[0] == 'v') {
        ver = tag_name + 1;
    }
    strncpy(info->latest_version, ver, sizeof(info->latest_version) - 1);

    // Get other fields
    find_json_string(response.data, "published_at", info->published_at, sizeof(info->published_at));
    find_json_string(response.data, "body", info->release_notes, sizeof(info->release_notes));

    // Check if prerelease
    if (strstr(response.data, "\"prerelease\":true") || strstr(response.data, "\"prerelease\": true")) {
        info->is_prerelease = true;
    }

    free(response.data);

    return 0;
}
