/**
 * CONVERGIO FILE LOCK MANAGER
 *
 * Implementation of file-level synchronization for multi-agent workspace access.
 * Uses flock() for advisory locking with timeout support.
 */

#include "nous/file_lock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>

// ============================================================================
// GLOBALS
// ============================================================================

static FileLockManager g_manager = {0};
static pthread_mutex_t g_lock_mutex = PTHREAD_MUTEX_INITIALIZER;

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static char* safe_strdup(const char* s) {
    return s ? strdup(s) : NULL;
}

static FileLock* lock_create(const char* filepath, FileLockType type, uint64_t owner_id) {
    FileLock* lock = calloc(1, sizeof(FileLock));
    if (!lock) return NULL;

    lock->filepath = safe_strdup(filepath);
    lock->fd = -1;
    lock->type = type;
    lock->owner_id = owner_id;
    lock->acquired_at = 0;
    lock->expires_at = 0;
    lock->is_valid = false;

    return lock;
}

static void lock_destroy(FileLock* lock) {
    if (!lock) return;

    if (lock->fd >= 0) {
        flock(lock->fd, LOCK_UN);
        close(lock->fd);
    }

    free(lock->filepath);
    free(lock);
}

static FileLock* find_lock_by_path(const char* filepath) {
    for (size_t i = 0; i < g_manager.lock_count; i++) {
        if (g_manager.locks[i] && g_manager.locks[i]->filepath &&
            strcmp(g_manager.locks[i]->filepath, filepath) == 0) {
            return g_manager.locks[i];
        }
    }
    return NULL;
}

static bool add_lock_to_manager(FileLock* lock) {
    if (!lock) return false;

    // Grow array if needed
    if (g_manager.lock_count >= g_manager.lock_capacity) {
        size_t new_capacity = g_manager.lock_capacity == 0 ? 16 : g_manager.lock_capacity * 2;
        FileLock** new_locks = realloc(g_manager.locks, sizeof(FileLock*) * new_capacity);
        if (!new_locks) return false;
        g_manager.locks = new_locks;
        g_manager.lock_capacity = new_capacity;
    }

    g_manager.locks[g_manager.lock_count++] = lock;
    return true;
}

static void remove_lock_from_manager(FileLock* lock) {
    if (!lock) return;

    for (size_t i = 0; i < g_manager.lock_count; i++) {
        if (g_manager.locks[i] == lock) {
            // Shift remaining locks
            for (size_t j = i; j < g_manager.lock_count - 1; j++) {
                g_manager.locks[j] = g_manager.locks[j + 1];
            }
            g_manager.lock_count--;
            break;
        }
    }
}

static int flock_type_for(FileLockType type) {
    switch (type) {
        case LOCK_READ:      return LOCK_SH;
        case LOCK_WRITE:
        case LOCK_EXCLUSIVE: return LOCK_EX;
        default:             return LOCK_EX;
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

int filelock_init(void) {
    pthread_mutex_lock(&g_lock_mutex);

    if (g_manager.initialized) {
        pthread_mutex_unlock(&g_lock_mutex);
        return 0;
    }

    g_manager.locks = NULL;
    g_manager.lock_count = 0;
    g_manager.lock_capacity = 0;
    g_manager.total_acquires = 0;
    g_manager.total_releases = 0;
    g_manager.total_timeouts = 0;
    g_manager.total_conflicts = 0;
    g_manager.initialized = true;

    pthread_mutex_unlock(&g_lock_mutex);
    return 0;
}

void filelock_shutdown(void) {
    pthread_mutex_lock(&g_lock_mutex);

    // Release all locks
    for (size_t i = 0; i < g_manager.lock_count; i++) {
        if (g_manager.locks[i]) {
            lock_destroy(g_manager.locks[i]);
        }
    }

    free(g_manager.locks);
    g_manager.locks = NULL;
    g_manager.lock_count = 0;
    g_manager.lock_capacity = 0;
    g_manager.initialized = false;

    pthread_mutex_unlock(&g_lock_mutex);
}

FileLockManager* filelock_get_manager(void) {
    return &g_manager;
}

// ============================================================================
// LOCK OPERATIONS
// ============================================================================

FileLock* filelock_acquire(const char* filepath, FileLockType type,
                           uint64_t owner_id, int timeout_ms) {
    return filelock_acquire_timed(filepath, type, owner_id, timeout_ms, 0);
}

FileLock* filelock_acquire_timed(const char* filepath, FileLockType type,
                                  uint64_t owner_id, int timeout_ms,
                                  int expire_seconds) {
    if (!filepath || !g_manager.initialized) return NULL;

    pthread_mutex_lock(&g_lock_mutex);

    // Check for existing lock
    FileLock* existing = find_lock_by_path(filepath);
    if (existing && existing->is_valid) {
        // Same owner can upgrade
        if (existing->owner_id == owner_id) {
            if (type == LOCK_WRITE && existing->type == LOCK_READ) {
                pthread_mutex_unlock(&g_lock_mutex);
                if (filelock_upgrade(existing, timeout_ms) == LOCK_SUCCESS) {
                    return existing;
                }
                return NULL;
            }
            pthread_mutex_unlock(&g_lock_mutex);
            return existing;  // Already have the lock
        }

        // Check for read-read compatibility
        if (type == LOCK_READ && existing->type == LOCK_READ) {
            // Allow multiple readers
        } else {
            // Conflict
            g_manager.total_conflicts++;
            pthread_mutex_unlock(&g_lock_mutex);
            return NULL;
        }
    }

    pthread_mutex_unlock(&g_lock_mutex);

    // Create new lock
    FileLock* lock = lock_create(filepath, type, owner_id);
    if (!lock) return NULL;

    // Open file
    int flags = (type == LOCK_READ) ? O_RDONLY : O_RDWR;
    lock->fd = open(filepath, flags);
    if (lock->fd < 0) {
        // Try creating for write locks
        if (type != LOCK_READ) {
            lock->fd = open(filepath, O_RDWR | O_CREAT, 0644);
        }
        if (lock->fd < 0) {
            lock_destroy(lock);
            return NULL;
        }
    }

    // Try to acquire flock
    int flock_flags = flock_type_for(type);
    int result;

    if (timeout_ms == 0) {
        // Non-blocking
        flock_flags |= LOCK_NB;
        result = flock(lock->fd, flock_flags);
    } else if (timeout_ms < 0) {
        // Blocking (infinite wait)
        result = flock(lock->fd, flock_flags);
    } else {
        // Timeout - poll with backoff
        flock_flags |= LOCK_NB;
        int elapsed = 0;
        int sleep_ms = 10;  // Start with 10ms

        while (elapsed < timeout_ms) {
            result = flock(lock->fd, flock_flags);
            if (result == 0) break;

            if (errno != EWOULDBLOCK) break;

            usleep((useconds_t)(sleep_ms * 1000));
            elapsed += sleep_ms;
            sleep_ms = (sleep_ms < 100) ? sleep_ms * 2 : 100;  // Max 100ms
        }

        if (result != 0 && elapsed >= timeout_ms) {
            pthread_mutex_lock(&g_lock_mutex);
            g_manager.total_timeouts++;
            pthread_mutex_unlock(&g_lock_mutex);
        }
    }

    if (result != 0) {
        lock_destroy(lock);
        return NULL;
    }

    // Lock acquired
    lock->acquired_at = time(NULL);
    lock->is_valid = true;

    if (expire_seconds > 0) {
        lock->expires_at = lock->acquired_at + expire_seconds;
    }

    // Add to manager
    pthread_mutex_lock(&g_lock_mutex);
    if (!add_lock_to_manager(lock)) {
        pthread_mutex_unlock(&g_lock_mutex);
        lock_destroy(lock);
        return NULL;
    }
    g_manager.total_acquires++;
    pthread_mutex_unlock(&g_lock_mutex);

    return lock;
}

FileLockError filelock_release(FileLock* lock) {
    if (!lock) return LOCK_ERROR_INVALID;
    if (!lock->is_valid) return LOCK_SUCCESS;

    pthread_mutex_lock(&g_lock_mutex);

    // Release flock
    if (lock->fd >= 0) {
        flock(lock->fd, LOCK_UN);
        close(lock->fd);
        lock->fd = -1;
    }

    lock->is_valid = false;
    remove_lock_from_manager(lock);
    g_manager.total_releases++;

    pthread_mutex_unlock(&g_lock_mutex);

    lock_destroy(lock);
    return LOCK_SUCCESS;
}

FileLockError filelock_upgrade(FileLock* lock, int timeout_ms) {
    if (!lock || !lock->is_valid) return LOCK_ERROR_INVALID;
    if (lock->type != LOCK_READ) return LOCK_ERROR_INVALID;

    // Try to get exclusive lock
    int flags = LOCK_EX;
    int result;

    if (timeout_ms == 0) {
        flags |= LOCK_NB;
        result = flock(lock->fd, flags);
    } else if (timeout_ms < 0) {
        result = flock(lock->fd, flags);
    } else {
        flags |= LOCK_NB;
        int elapsed = 0;
        int sleep_ms = 10;

        while (elapsed < timeout_ms) {
            result = flock(lock->fd, flags);
            if (result == 0) break;
            if (errno != EWOULDBLOCK) break;

            usleep((useconds_t)(sleep_ms * 1000));
            elapsed += sleep_ms;
            sleep_ms = (sleep_ms < 100) ? sleep_ms * 2 : 100;
        }
    }

    if (result != 0) {
        return (errno == EWOULDBLOCK) ? LOCK_ERROR_BUSY : LOCK_ERROR_IO;
    }

    lock->type = LOCK_WRITE;
    return LOCK_SUCCESS;
}

FileLockError filelock_downgrade(FileLock* lock) {
    if (!lock || !lock->is_valid) return LOCK_ERROR_INVALID;
    if (lock->type == LOCK_READ) return LOCK_SUCCESS;

    int result = flock(lock->fd, LOCK_SH);
    if (result != 0) {
        return LOCK_ERROR_IO;
    }

    lock->type = LOCK_READ;
    return LOCK_SUCCESS;
}

// ============================================================================
// LOCK QUERIES
// ============================================================================

bool filelock_is_locked(const char* filepath, FileLockType type) {
    if (!filepath) return false;

    pthread_mutex_lock(&g_lock_mutex);

    FileLock* lock = find_lock_by_path(filepath);
    bool result = false;

    if (lock && lock->is_valid) {
        if ((int)type < 0) {
            result = true;  // Any lock type
        } else {
            result = (lock->type == type);
        }
    }

    pthread_mutex_unlock(&g_lock_mutex);
    return result;
}

uint64_t filelock_get_owner(const char* filepath) {
    if (!filepath) return 0;

    pthread_mutex_lock(&g_lock_mutex);

    FileLock* lock = find_lock_by_path(filepath);
    uint64_t owner = (lock && lock->is_valid) ? lock->owner_id : 0;

    pthread_mutex_unlock(&g_lock_mutex);
    return owner;
}

size_t filelock_get_by_owner(uint64_t owner_id, FileLock** out_locks, size_t max_count) {
    if (!out_locks || max_count == 0) return 0;

    pthread_mutex_lock(&g_lock_mutex);

    size_t count = 0;
    for (size_t i = 0; i < g_manager.lock_count && count < max_count; i++) {
        if (g_manager.locks[i] && g_manager.locks[i]->owner_id == owner_id) {
            out_locks[count++] = g_manager.locks[i];
        }
    }

    pthread_mutex_unlock(&g_lock_mutex);
    return count;
}

// ============================================================================
// MULTI-FILE OPERATIONS
// ============================================================================

FileLockError filelock_acquire_batch(const char** filepaths, size_t count,
                                      FileLockType type, uint64_t owner_id,
                                      int timeout_ms, FileLock** out_locks) {
    if (!filepaths || count == 0 || !out_locks) return LOCK_ERROR_INVALID;

    // Sort paths to prevent deadlock (canonical ordering)
    const char** sorted = malloc(sizeof(char*) * count);
    if (!sorted) return LOCK_ERROR_INTERNAL;
    memcpy(sorted, filepaths, sizeof(char*) * count);

    // Simple bubble sort for small arrays
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = 0; j < count - i - 1; j++) {
            if (strcmp(sorted[j], sorted[j + 1]) > 0) {
                const char* tmp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = tmp;
            }
        }
    }

    // Acquire locks in order
    size_t acquired = 0;
    for (size_t i = 0; i < count; i++) {
        out_locks[i] = filelock_acquire(sorted[i], type, owner_id, timeout_ms);
        if (!out_locks[i]) {
            // Rollback
            for (size_t j = 0; j < acquired; j++) {
                filelock_release(out_locks[j]);
                out_locks[j] = NULL;
            }
            free(sorted);
            return LOCK_ERROR_BUSY;
        }
        acquired++;
    }

    free(sorted);
    return LOCK_SUCCESS;
}

size_t filelock_release_all(uint64_t owner_id) {
    pthread_mutex_lock(&g_lock_mutex);

    // Collect locks to release (can't modify while iterating)
    FileLock** to_release = malloc(sizeof(FileLock*) * g_manager.lock_count);
    size_t release_count = 0;

    for (size_t i = 0; i < g_manager.lock_count; i++) {
        if (g_manager.locks[i] && g_manager.locks[i]->owner_id == owner_id) {
            to_release[release_count++] = g_manager.locks[i];
        }
    }

    pthread_mutex_unlock(&g_lock_mutex);

    // Release collected locks
    for (size_t i = 0; i < release_count; i++) {
        filelock_release(to_release[i]);
    }

    free(to_release);
    return release_count;
}

// ============================================================================
// DEADLOCK DETECTION
// ============================================================================

// Simple wait-for graph based detection
typedef struct {
    uint64_t agent_id;
    uint64_t waiting_for;  // Agent ID we're waiting for
} WaitEdge;

static WaitEdge g_wait_graph[64];
static size_t g_wait_count = 0;
static pthread_mutex_t g_wait_mutex = PTHREAD_MUTEX_INITIALIZER;

bool filelock_would_deadlock(uint64_t requester_id, const char* filepath) {
    if (!filepath) return false;

    pthread_mutex_lock(&g_lock_mutex);
    FileLock* holder = find_lock_by_path(filepath);
    if (!holder || !holder->is_valid || holder->owner_id == requester_id) {
        pthread_mutex_unlock(&g_lock_mutex);
        return false;
    }

    uint64_t holder_id = holder->owner_id;
    pthread_mutex_unlock(&g_lock_mutex);

    // Check if holder is waiting for requester (direct cycle)
    pthread_mutex_lock(&g_wait_mutex);

    bool deadlock = false;
    uint64_t current = holder_id;
    size_t depth = 0;

    while (depth < g_wait_count && depth < 64) {
        bool found = false;
        for (size_t i = 0; i < g_wait_count; i++) {
            if (g_wait_graph[i].agent_id == current) {
                if (g_wait_graph[i].waiting_for == requester_id) {
                    deadlock = true;
                    break;
                }
                current = g_wait_graph[i].waiting_for;
                found = true;
                break;
            }
        }
        if (!found || deadlock) break;
        depth++;
    }

    pthread_mutex_unlock(&g_wait_mutex);
    return deadlock;
}

size_t filelock_get_deadlock_cycle(uint64_t* out_agents, size_t max_count) {
    // Simplified: just return current wait graph participants
    pthread_mutex_lock(&g_wait_mutex);

    size_t count = 0;
    for (size_t i = 0; i < g_wait_count && count < max_count; i++) {
        out_agents[count++] = g_wait_graph[i].agent_id;
    }

    pthread_mutex_unlock(&g_wait_mutex);
    return count;
}

// ============================================================================
// MAINTENANCE
// ============================================================================

size_t filelock_cleanup_expired(void) {
    time_t now = time(NULL);
    size_t cleaned = 0;

    pthread_mutex_lock(&g_lock_mutex);

    // Collect expired locks
    FileLock** expired = malloc(sizeof(FileLock*) * g_manager.lock_count);
    size_t expired_count = 0;

    for (size_t i = 0; i < g_manager.lock_count; i++) {
        FileLock* lock = g_manager.locks[i];
        if (lock && lock->is_valid && lock->expires_at > 0 && lock->expires_at <= now) {
            expired[expired_count++] = lock;
        }
    }

    pthread_mutex_unlock(&g_lock_mutex);

    // Release expired locks
    for (size_t i = 0; i < expired_count; i++) {
        filelock_release(expired[i]);
        cleaned++;
    }

    free(expired);
    return cleaned;
}

FileLockError filelock_force_release(const char* filepath) {
    if (!filepath) return LOCK_ERROR_INVALID;

    pthread_mutex_lock(&g_lock_mutex);

    FileLock* lock = find_lock_by_path(filepath);
    if (!lock) {
        pthread_mutex_unlock(&g_lock_mutex);
        return LOCK_SUCCESS;
    }

    pthread_mutex_unlock(&g_lock_mutex);
    return filelock_release(lock);
}

// ============================================================================
// STATISTICS
// ============================================================================

char* filelock_stats_json(void) {
    pthread_mutex_lock(&g_lock_mutex);

    char* json = malloc(1024);
    if (!json) {
        pthread_mutex_unlock(&g_lock_mutex);
        return NULL;
    }

    snprintf(json, 1024,
        "{"
        "\"active_locks\":%zu,"
        "\"total_acquires\":%llu,"
        "\"total_releases\":%llu,"
        "\"total_timeouts\":%llu,"
        "\"total_conflicts\":%llu"
        "}",
        g_manager.lock_count,
        (unsigned long long)g_manager.total_acquires,
        (unsigned long long)g_manager.total_releases,
        (unsigned long long)g_manager.total_timeouts,
        (unsigned long long)g_manager.total_conflicts);

    pthread_mutex_unlock(&g_lock_mutex);
    return json;
}

char* filelock_status(void) {
    pthread_mutex_lock(&g_lock_mutex);

    size_t buf_size = 2048;
    char* status = malloc(buf_size);
    if (!status) {
        pthread_mutex_unlock(&g_lock_mutex);
        return NULL;
    }

    size_t offset = (size_t)snprintf(status, buf_size,
        "File Lock Manager Status\n"
        "========================\n"
        "Active locks: %zu\n"
        "Total acquires: %llu\n"
        "Total releases: %llu\n"
        "Timeouts: %llu\n"
        "Conflicts: %llu\n\n"
        "Active Locks:\n",
        g_manager.lock_count,
        (unsigned long long)g_manager.total_acquires,
        (unsigned long long)g_manager.total_releases,
        (unsigned long long)g_manager.total_timeouts,
        (unsigned long long)g_manager.total_conflicts);

    const char* type_names[] = {"READ", "WRITE", "EXCLUSIVE"};

    for (size_t i = 0; i < g_manager.lock_count && offset < buf_size - 200; i++) {
        FileLock* lock = g_manager.locks[i];
        if (lock && lock->is_valid) {
            offset += (size_t)snprintf(status + offset, buf_size - offset,
                "  [%s] %s (owner: %llu)\n",
                type_names[lock->type],
                lock->filepath,
                (unsigned long long)lock->owner_id);
        }
    }

    pthread_mutex_unlock(&g_lock_mutex);
    return status;
}
