/**
 * CONVERGIO DEBUG MUTEX
 *
 * In debug mode, uses ERRORCHECK mutex to detect deadlocks.
 * In release mode, uses standard mutex for performance.
 *
 * Usage:
 *   CONVERGIO_MUTEX_DECLARE(my_mutex);
 *   ...
 *   CONVERGIO_MUTEX_LOCK(&my_mutex);
 *   // critical section
 *   CONVERGIO_MUTEX_UNLOCK(&my_mutex);
 */

#ifndef CONVERGIO_DEBUG_MUTEX_H
#define CONVERGIO_DEBUG_MUTEX_H

#include <pthread.h>
#include <stdio.h>

#ifdef DEBUG

// Debug mutex wrapper with lazy initialization
typedef struct {
    pthread_mutex_t mutex;
    pthread_once_t once;
    volatile int initialized;
} ConvergioMutex;

// Forward declaration for init function
static inline void _convergio_mutex_do_init(ConvergioMutex* m);

// Initializer for static declarations
#define CONVERGIO_MUTEX_DECLARE(name) \
    static ConvergioMutex name = { \
        .mutex = PTHREAD_MUTEX_INITIALIZER, \
        .once = PTHREAD_ONCE_INIT, \
        .initialized = 0 \
    }

// Thread-safe lazy initialization helper
static inline void _convergio_mutex_ensure_init(ConvergioMutex* m) {
    if (!m->initialized) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

        // Destroy the default-initialized mutex first
        pthread_mutex_destroy(&m->mutex);

        // Re-initialize with ERRORCHECK type
        pthread_mutex_init(&m->mutex, &attr);
        pthread_mutexattr_destroy(&attr);

        m->initialized = 1;
    }
}

// Wrapper that checks for errors in debug mode
static inline int convergio_mutex_lock(ConvergioMutex* m) {
    _convergio_mutex_ensure_init(m);

    int ret = pthread_mutex_lock(&m->mutex);
    if (ret != 0) {
        if (ret == 35) { // EDEADLK on macOS
            fprintf(stderr, "[DEBUG] DEADLOCK DETECTED: Thread attempted to re-lock mutex it already holds!\n");
        } else {
            fprintf(stderr, "[DEBUG] Mutex lock failed with error: %d\n", ret);
        }
    }
    return ret;
}

static inline int convergio_mutex_unlock(ConvergioMutex* m) {
    int ret = pthread_mutex_unlock(&m->mutex);
    if (ret != 0) {
        if (ret == 1) { // EPERM on macOS
            fprintf(stderr, "[DEBUG] UNLOCK ERROR: Thread doesn't own this mutex!\n");
        } else {
            fprintf(stderr, "[DEBUG] Mutex unlock failed with error: %d\n", ret);
        }
    }
    return ret;
}

static inline int convergio_mutex_trylock(ConvergioMutex* m) {
    _convergio_mutex_ensure_init(m);
    return pthread_mutex_trylock(&m->mutex);
}

#define CONVERGIO_MUTEX_LOCK(m)    convergio_mutex_lock(m)
#define CONVERGIO_MUTEX_UNLOCK(m)  convergio_mutex_unlock(m)
#define CONVERGIO_MUTEX_TRYLOCK(m) convergio_mutex_trylock(m)

// For accessing raw pthread_mutex_t (e.g., for condition variables)
#define CONVERGIO_MUTEX_RAW(m) (&(m)->mutex)

#else
// Release mode: use standard pthread mutex directly

typedef pthread_mutex_t ConvergioMutex;

#define CONVERGIO_MUTEX_DECLARE(name) \
    static ConvergioMutex name = PTHREAD_MUTEX_INITIALIZER

#define CONVERGIO_MUTEX_LOCK(m)    pthread_mutex_lock(m)
#define CONVERGIO_MUTEX_UNLOCK(m)  pthread_mutex_unlock(m)
#define CONVERGIO_MUTEX_TRYLOCK(m) pthread_mutex_trylock(m)
#define CONVERGIO_MUTEX_RAW(m)     (m)

#endif // DEBUG

#endif // CONVERGIO_DEBUG_MUTEX_H
