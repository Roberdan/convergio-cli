/**
 * CONVERGIO FILE LOCK MANAGER
 *
 * Provides file-level synchronization for multi-agent access:
 * - Advisory locks (flock-based)
 * - Read-write lock semantics
 * - Timeout support
 * - Deadlock detection
 *
 * Used for workspace file safety when multiple agents
 * operate on the same codebase concurrently.
 */

#ifndef CONVERGIO_FILE_LOCK_H
#define CONVERGIO_FILE_LOCK_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// ============================================================================
// LOCK TYPES
// ============================================================================

typedef enum {
    LOCK_READ,       // Shared read lock (multiple readers OK)
    LOCK_WRITE,      // Exclusive write lock (single writer)
    LOCK_EXCLUSIVE,  // Full exclusive (no readers or writers)
} FileLockType;

typedef enum {
    LOCK_SUCCESS = 0,
    LOCK_ERROR_BUSY,       // Lock held by another process
    LOCK_ERROR_TIMEOUT,    // Timeout waiting for lock
    LOCK_ERROR_DEADLOCK,   // Potential deadlock detected
    LOCK_ERROR_INVALID,    // Invalid path or parameters
    LOCK_ERROR_IO,         // I/O error
    LOCK_ERROR_INTERNAL,   // Internal error
} FileLockError;

// ============================================================================
// LOCK HANDLE
// ============================================================================

typedef struct {
    char* filepath;        // Absolute path to locked file
    int fd;                // File descriptor
    FileLockType type;     // Type of lock held
    uint64_t owner_id;     // Agent ID that owns the lock
    time_t acquired_at;    // When lock was acquired
    time_t expires_at;     // Auto-release time (0 = no expiry)
    bool is_valid;         // Lock is currently held
} FileLock;

// ============================================================================
// LOCK MANAGER STATE
// ============================================================================

typedef struct {
    FileLock** locks;      // Active locks
    size_t lock_count;
    size_t lock_capacity;
    bool initialized;

    // Statistics
    uint64_t total_acquires;
    uint64_t total_releases;
    uint64_t total_timeouts;
    uint64_t total_conflicts;
} FileLockManager;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize the file lock manager
 * Returns 0 on success, -1 on failure
 */
int filelock_init(void);

/**
 * Shutdown and release all locks
 */
void filelock_shutdown(void);

/**
 * Get manager instance (for statistics)
 */
FileLockManager* filelock_get_manager(void);

// ============================================================================
// LOCK OPERATIONS
// ============================================================================

/**
 * Acquire a lock on a file
 *
 * @param filepath   Absolute path to file
 * @param type       Type of lock to acquire
 * @param owner_id   ID of the agent/process acquiring
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking, -1 = infinite)
 * @return           Lock handle or NULL on failure
 */
FileLock* filelock_acquire(const char* filepath, FileLockType type,
                           uint64_t owner_id, int timeout_ms);

/**
 * Acquire with automatic expiry
 */
FileLock* filelock_acquire_timed(const char* filepath, FileLockType type,
                                  uint64_t owner_id, int timeout_ms,
                                  int expire_seconds);

/**
 * Release a lock
 *
 * @param lock  Lock handle to release
 * @return      LOCK_SUCCESS or error code
 */
FileLockError filelock_release(FileLock* lock);

/**
 * Try to upgrade a read lock to write lock
 *
 * @param lock       Existing read lock
 * @param timeout_ms Timeout for upgrade
 * @return           LOCK_SUCCESS or error code
 */
FileLockError filelock_upgrade(FileLock* lock, int timeout_ms);

/**
 * Downgrade a write lock to read lock
 *
 * @param lock  Existing write lock
 * @return      LOCK_SUCCESS or error code
 */
FileLockError filelock_downgrade(FileLock* lock);

// ============================================================================
// LOCK QUERIES
// ============================================================================

/**
 * Check if a file is locked
 *
 * @param filepath  Path to check
 * @param type      Type of lock to check for (or -1 for any)
 * @return          true if locked
 */
bool filelock_is_locked(const char* filepath, FileLockType type);

/**
 * Get owner of current lock on file
 *
 * @param filepath  Path to check
 * @return          Owner ID or 0 if not locked
 */
uint64_t filelock_get_owner(const char* filepath);

/**
 * Get all locks held by an owner
 *
 * @param owner_id   ID to search for
 * @param out_locks  Output array (caller allocates)
 * @param max_count  Maximum locks to return
 * @return           Number of locks found
 */
size_t filelock_get_by_owner(uint64_t owner_id, FileLock** out_locks, size_t max_count);

// ============================================================================
// MULTI-FILE OPERATIONS
// ============================================================================

/**
 * Acquire locks on multiple files atomically
 * All-or-nothing: either all locks succeed or none are acquired
 *
 * @param filepaths  Array of file paths
 * @param count      Number of files
 * @param type       Type of lock for all files
 * @param owner_id   Agent ID
 * @param timeout_ms Timeout for entire operation
 * @param out_locks  Output array for lock handles
 * @return           LOCK_SUCCESS or error code
 */
FileLockError filelock_acquire_batch(const char** filepaths, size_t count,
                                      FileLockType type, uint64_t owner_id,
                                      int timeout_ms, FileLock** out_locks);

/**
 * Release all locks held by an owner
 *
 * @param owner_id  Agent ID
 * @return          Number of locks released
 */
size_t filelock_release_all(uint64_t owner_id);

// ============================================================================
// DEADLOCK DETECTION
// ============================================================================

/**
 * Check for potential deadlock between agents
 *
 * @param requester_id  Agent requesting a lock
 * @param filepath      File being requested
 * @return              true if deadlock would occur
 */
bool filelock_would_deadlock(uint64_t requester_id, const char* filepath);

/**
 * Get deadlock cycle (for debugging)
 *
 * @param out_agents  Output array of agent IDs in cycle
 * @param max_count   Maximum agents to return
 * @return            Number of agents in cycle (0 = no deadlock)
 */
size_t filelock_get_deadlock_cycle(uint64_t* out_agents, size_t max_count);

// ============================================================================
// MAINTENANCE
// ============================================================================

/**
 * Release expired locks
 *
 * @return  Number of locks released
 */
size_t filelock_cleanup_expired(void);

/**
 * Force release a lock (admin operation)
 *
 * @param filepath  Path to unlock
 * @return          LOCK_SUCCESS or error code
 */
FileLockError filelock_force_release(const char* filepath);

// ============================================================================
// STATISTICS
// ============================================================================

/**
 * Get lock statistics as JSON
 */
char* filelock_stats_json(void);

/**
 * Get human-readable status
 */
char* filelock_status(void);

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

/**
 * RAII-style lock guard (GCC/Clang)
 * Usage:
 *   FILELOCK_GUARD(lock, "path", LOCK_WRITE, agent_id) {
 *       // do work with file
 *   }  // auto-release
 */
#define FILELOCK_GUARD(name, path, type, owner) \
    for (FileLock* name = filelock_acquire(path, type, owner, 5000), \
         *_guard_##name = (void*)1; \
         _guard_##name && name; \
         _guard_##name = NULL, filelock_release(name))

/**
 * Try-lock pattern
 */
#define FILELOCK_TRY(name, path, type, owner) \
    FileLock* name = filelock_acquire(path, type, owner, 0); \
    if (name)

#endif // CONVERGIO_FILE_LOCK_H
