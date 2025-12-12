/**
 * CONVERGIO SAFE PATH UTILITIES
 *
 * Implementation of secure path operations.
 */

#include "nous/safe_path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

// Static buffer for user boundary
static char g_user_boundary[PATH_MAX] = {0};
static char g_cwd_boundary[PATH_MAX] = {0};

const char* safe_path_strerror(SafePathResult result) {
    switch (result) {
        case SAFE_PATH_OK:
            return "Success";
        case SAFE_PATH_NULL_INPUT:
            return "NULL input parameter";
        case SAFE_PATH_TOO_LONG:
            return "Path exceeds maximum length";
        case SAFE_PATH_RESOLVE_FAILED:
            return "Failed to resolve path";
        case SAFE_PATH_OUTSIDE_BOUNDARY:
            return "Path escapes allowed directory boundary";
        case SAFE_PATH_SYMLINK_ATTACK:
            return "Potential symlink attack detected";
        case SAFE_PATH_OPEN_FAILED:
            return "Failed to open file";
        case SAFE_PATH_STAT_FAILED:
            return "Failed to stat path";
        default:
            return "Unknown error";
    }
}

SafePathResult safe_path_resolve(const char* path,
                                  const char* boundary,
                                  char* resolved_out,
                                  size_t resolved_size) {
    if (!path || !resolved_out || resolved_size == 0) {
        return SAFE_PATH_NULL_INPUT;
    }

    if (resolved_size < PATH_MAX) {
        return SAFE_PATH_TOO_LONG;
    }

    // Resolve the input path to canonical form
    char* resolved = realpath(path, resolved_out);
    if (!resolved) {
        // If the file doesn't exist, we need to resolve the parent directory
        // and append the filename
        char path_copy[PATH_MAX];
        strncpy(path_copy, path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';

        // Find the last component
        char* last_slash = strrchr(path_copy, '/');
        char filename[PATH_MAX] = {0};

        if (last_slash) {
            strncpy(filename, last_slash + 1, sizeof(filename) - 1);
            *last_slash = '\0';  // Truncate to get directory

            // Resolve the directory
            char dir_resolved[PATH_MAX];
            if (!realpath(path_copy, dir_resolved)) {
                return SAFE_PATH_RESOLVE_FAILED;
            }

            // Join back with filename
            int written = snprintf(resolved_out, resolved_size, "%s/%s",
                                   dir_resolved, filename);
            if (written < 0 || (size_t)written >= resolved_size) {
                return SAFE_PATH_TOO_LONG;
            }
        } else {
            // No slash - resolve current directory and append
            char cwd[PATH_MAX];
            if (!getcwd(cwd, sizeof(cwd))) {
                return SAFE_PATH_RESOLVE_FAILED;
            }

            int written = snprintf(resolved_out, resolved_size, "%s/%s",
                                   cwd, path);
            if (written < 0 || (size_t)written >= resolved_size) {
                return SAFE_PATH_TOO_LONG;
            }
        }
    }

    // If boundary is specified, verify the resolved path is within it
    if (boundary) {
        char boundary_resolved[PATH_MAX];
        if (!realpath(boundary, boundary_resolved)) {
            return SAFE_PATH_RESOLVE_FAILED;
        }

        size_t boundary_len = strlen(boundary_resolved);

        // The resolved path must start with the boundary path
        if (strncmp(resolved_out, boundary_resolved, boundary_len) != 0) {
            return SAFE_PATH_OUTSIDE_BOUNDARY;
        }

        // After the boundary prefix, must be end of string or '/'
        char next_char = resolved_out[boundary_len];
        if (next_char != '\0' && next_char != '/') {
            return SAFE_PATH_OUTSIDE_BOUNDARY;
        }
    }

    return SAFE_PATH_OK;
}

bool safe_path_within_boundary_weak(const char* path, const char* boundary) {
    if (!path || !boundary) {
        return false;
    }

    // Check for obvious traversal attempts
    if (strstr(path, "..")) {
        return false;
    }

    // If path is absolute, check it starts with boundary
    if (path[0] == '/') {
        size_t boundary_len = strlen(boundary);
        if (strncmp(path, boundary, boundary_len) != 0) {
            return false;
        }
        // Next char must be '/' or end of string
        char next = path[boundary_len];
        if (next != '\0' && next != '/') {
            return false;
        }
    }

    return true;
}

int safe_path_open(const char* path,
                   const char* boundary,
                   int flags,
                   int mode) {
    if (!path) {
        errno = EINVAL;
        return -1;
    }

    char resolved[PATH_MAX];
    SafePathResult result;

    // For existing files, resolve with boundary check
    if (!(flags & O_CREAT)) {
        result = safe_path_resolve(path, boundary, resolved, sizeof(resolved));
        if (result != SAFE_PATH_OK) {
            errno = (result == SAFE_PATH_OUTSIDE_BOUNDARY) ? EACCES : ENOENT;
            return -1;
        }

        // Open with O_NOFOLLOW to prevent symlink attacks on last component
        // Note: realpath already followed symlinks, so this is extra protection
        // against race conditions where symlink is created after realpath
        return open(resolved, flags | O_NOFOLLOW, mode);
    }

    // For new files, resolve parent directory and check boundary
    char path_copy[PATH_MAX];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char* last_slash = strrchr(path_copy, '/');
    const char* filename;
    char parent_dir[PATH_MAX];

    if (last_slash) {
        filename = last_slash + 1;
        *last_slash = '\0';
        strncpy(parent_dir, path_copy, sizeof(parent_dir) - 1);
        parent_dir[sizeof(parent_dir) - 1] = '\0';
    } else {
        filename = path_copy;
        if (!getcwd(parent_dir, sizeof(parent_dir))) {
            return -1;
        }
    }

    // Resolve parent directory with boundary check
    char parent_resolved[PATH_MAX];
    result = safe_path_resolve(parent_dir, boundary, parent_resolved, sizeof(parent_resolved));
    if (result != SAFE_PATH_OK) {
        errno = (result == SAFE_PATH_OUTSIDE_BOUNDARY) ? EACCES : ENOENT;
        return -1;
    }

    // Build final path
    int written = snprintf(resolved, sizeof(resolved), "%s/%s",
                           parent_resolved, filename);
    if (written < 0 || (size_t)written >= sizeof(resolved)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    // For creation, add O_EXCL to prevent race conditions
    // The caller can handle EEXIST and retry without O_CREAT if desired
    return open(resolved, flags | O_NOFOLLOW | O_EXCL, mode);
}

SafePathResult safe_path_join(const char* base,
                               const char* component,
                               char* result,
                               size_t result_size) {
    if (!base || !component || !result || result_size == 0) {
        return SAFE_PATH_NULL_INPUT;
    }

    // Reject component that tries to escape
    if (component[0] == '/' || strstr(component, "..")) {
        return SAFE_PATH_OUTSIDE_BOUNDARY;
    }

    // Calculate required size
    size_t base_len = strlen(base);
    size_t comp_len = strlen(component);
    size_t need_slash = (base_len > 0 && base[base_len - 1] != '/') ? 1 : 0;
    size_t total = base_len + need_slash + comp_len + 1;

    if (total > result_size) {
        return SAFE_PATH_TOO_LONG;
    }

    // Build the path
    if (need_slash) {
        snprintf(result, result_size, "%s/%s", base, component);
    } else {
        snprintf(result, result_size, "%s%s", base, component);
    }

    return SAFE_PATH_OK;
}

const char* safe_path_get_user_boundary(void) {
    if (g_user_boundary[0] == '\0') {
        const char* home = getenv("HOME");
        if (home) {
            snprintf(g_user_boundary, sizeof(g_user_boundary),
                     "%s/.convergio", home);

            // Create if doesn't exist
            mkdir(g_user_boundary, 0700);
        }
    }
    return g_user_boundary;
}

const char* safe_path_get_cwd_boundary(void) {
    if (g_cwd_boundary[0] == '\0') {
        getcwd(g_cwd_boundary, sizeof(g_cwd_boundary));
    }
    return g_cwd_boundary;
}
