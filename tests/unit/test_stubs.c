/**
 * CONVERGIO TEST STUBS
 *
 * Stub implementations for unit test isolation.
 */

#include <stdio.h>
#include <stdarg.h>

// Stub for nous_log
void nous_log(int level, const char* fmt, ...) {
    (void)level;
    (void)fmt;
    // Silent in tests
}

// Stub for provider_registry_init
int provider_registry_init(void) {
    return 0;  // Always succeed
}

// Stub for provider_registry_shutdown
void provider_registry_shutdown(void) {
    // No-op
}
