/**
 * CONVERGIO SAFE PATH UTILITIES
 *
 * Provides secure path operations:
 * - Path canonicalization with realpath
 * - Boundary checking (ensure paths stay within allowed directories)
 * - Symlink attack protection
 * - Race condition (TOCTOU) mitigation
 */

#ifndef CONVERGIO_SAFE_PATH_H
#define CONVERGIO_SAFE_PATH_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Result codes for safe path operations
 */
typedef enum {
    SAFE_PATH_OK = 0,           // Success
    SAFE_PATH_NULL_INPUT,       // NULL input parameter
    SAFE_PATH_TOO_LONG,         // Path exceeds PATH_MAX
    SAFE_PATH_RESOLVE_FAILED,   // realpath() failed
    SAFE_PATH_OUTSIDE_BOUNDARY, // Path escapes allowed boundary
    SAFE_PATH_SYMLINK_ATTACK,   // Potential symlink attack detected
    SAFE_PATH_OPEN_FAILED,      // Failed to open file
    SAFE_PATH_STAT_FAILED,      // Failed to stat path
} SafePathResult;

/**
 * Get human-readable error message for result code
 */
const char* safe_path_strerror(SafePathResult result);

/**
 * Resolve a path to its canonical form and verify it stays within a boundary.
 *
 * This function:
 * 1. Resolves the path to absolute form using realpath()
 * 2. Verifies the resolved path starts with the boundary prefix
 * 3. Returns error if path escapes the boundary
 *
 * @param path           The path to resolve (can be relative or absolute)
 * @param boundary       The directory boundary that path must stay within
 * @param resolved_out   Output buffer for the resolved canonical path
 * @param resolved_size  Size of the output buffer (should be PATH_MAX)
 * @return               SAFE_PATH_OK on success, error code otherwise
 *
 * Example:
 *   char resolved[PATH_MAX];
 *   if (safe_path_resolve("/home/user/../user/file.txt", "/home/user",
 *                         resolved, sizeof(resolved)) == SAFE_PATH_OK) {
 *       // resolved = "/home/user/file.txt"
 *   }
 *
 * Security notes:
 * - Uses realpath() which follows ALL symlinks
 * - Boundary check uses resolved paths, not the input
 * - Does NOT create missing directories
 */
SafePathResult safe_path_resolve(const char* path,
                                  const char* boundary,
                                  char* resolved_out,
                                  size_t resolved_size);

/**
 * Check if a path is within a boundary WITHOUT resolving symlinks first.
 * This is useful for checking user input before attempting file operations.
 *
 * WARNING: This is a weaker check than safe_path_resolve() because it
 * doesn't follow symlinks. Use safe_path_resolve() for actual file access.
 *
 * @param path      The path to check
 * @param boundary  The directory boundary
 * @return          true if path appears to be within boundary
 */
bool safe_path_within_boundary_weak(const char* path, const char* boundary);

/**
 * Open a file safely with TOCTOU protection.
 *
 * This function:
 * 1. Resolves the path with boundary checking
 * 2. Opens with O_NOFOLLOW to prevent last-component symlink attacks
 * 3. Verifies the opened file matches what we expected
 *
 * @param path      Path to open
 * @param boundary  Directory boundary (NULL to skip boundary check)
 * @param flags     Open flags (O_RDONLY, O_WRONLY, O_RDWR, etc.)
 * @param mode      File mode for creation (only used with O_CREAT)
 * @return          File descriptor on success, -1 on error (check errno)
 *
 * Note: For O_CREAT, also adds O_EXCL to prevent race conditions
 */
int safe_path_open(const char* path,
                   const char* boundary,
                   int flags,
                   int mode);

/**
 * Join two path components safely.
 *
 * @param base       Base directory path
 * @param component  Path component to append (should not start with /)
 * @param result     Output buffer for joined path
 * @param result_size Size of output buffer
 * @return           SAFE_PATH_OK on success
 */
SafePathResult safe_path_join(const char* base,
                               const char* component,
                               char* result,
                               size_t result_size);

/**
 * Get the boundary for user data files (~/.convergio).
 * Returns a static buffer - do not free.
 */
const char* safe_path_get_user_boundary(void);

/**
 * Get the current working directory boundary.
 * Returns a static buffer - do not free.
 */
const char* safe_path_get_cwd_boundary(void);

#endif // CONVERGIO_SAFE_PATH_H
