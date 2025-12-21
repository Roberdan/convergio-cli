/**
 * CONVERGIO UNIFIED REGISTRY PATTERN
 *
 * Provides a common interface abstraction for all registry types:
 * - Agent Registry (agents/registry.c) - hash table with O(1) lookup
 * - Provider Registry (providers/provider.c) - enum-indexed static array
 * - Tool Registry (tools/tools.c) - config-based dynamic loading
 * - Orchestrator Registry (orchestrator/orchestrator.c) - hash table
 *
 * Each registry implements these common operations through type-specific
 * functions following the naming convention: <type>_registry_<operation>
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef NOUS_REGISTRY_H
#define NOUS_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// REGISTRY RESULT CODES
// ============================================================================

typedef enum {
    REGISTRY_OK = 0,
    REGISTRY_ERR_NOT_FOUND = -1,
    REGISTRY_ERR_ALREADY_EXISTS = -2,
    REGISTRY_ERR_FULL = -3,
    REGISTRY_ERR_INVALID = -4,
    REGISTRY_ERR_NOT_INITIALIZED = -5
} RegistryResult;

// ============================================================================
// REGISTRY ITERATOR
// ============================================================================

/**
 * Generic registry iterator for traversing entries
 */
typedef struct {
    void* registry;          // Opaque registry pointer
    size_t index;            // Current position
    void* current;           // Current entry
    bool (*next)(void*);     // Advance function
} RegistryIterator;

// ============================================================================
// REGISTRY STATISTICS
// ============================================================================

typedef struct {
    size_t count;            // Current number of entries
    size_t capacity;         // Maximum capacity (0 = unlimited)
    size_t lookups;          // Total lookup operations
    size_t hits;             // Successful lookups
    size_t misses;           // Failed lookups
    size_t inserts;          // Total insert operations
    size_t deletes;          // Total delete operations
} RegistryStats;

// ============================================================================
// REGISTRY INTERFACE MACROS
// ============================================================================

/**
 * These macros document the expected interface for registry implementations.
 * Each registry should implement functions following these patterns.
 *
 * Example for Agent Registry:
 *   agent_registry_init()      -> int
 *   agent_registry_shutdown()  -> void
 *   agent_registry_add()       -> RegistryResult
 *   agent_registry_remove()    -> RegistryResult
 *   agent_registry_find()      -> void*
 *   agent_registry_count()     -> size_t
 *   agent_registry_stats()     -> RegistryStats
 */

#define REGISTRY_INTERFACE(type) \
    int type##_registry_init(void); \
    void type##_registry_shutdown(void); \
    size_t type##_registry_count(void); \
    RegistryStats type##_registry_stats(void)

// ============================================================================
// AGENT REGISTRY (from orchestrator/registry.c)
// ============================================================================

// Forward declaration
struct ManagedAgent;

/**
 * Agent registry uses FNV-1a hash table for O(1) name lookup.
 * Capacity: Dynamic (starts at 64, grows as needed)
 * Thread-safe: Yes (mutex protected)
 */

int agent_registry_init(void);
void agent_registry_shutdown(void);

RegistryResult agent_registry_add(struct ManagedAgent* agent);
RegistryResult agent_registry_remove(const char* name);
struct ManagedAgent* agent_registry_find_by_name(const char* name);
struct ManagedAgent* agent_registry_find_by_id(int agent_id);
size_t agent_registry_count(void);
RegistryStats agent_registry_stats(void);

// Iterator
bool agent_registry_iterate(RegistryIterator* iter);
struct ManagedAgent* agent_registry_iter_current(RegistryIterator* iter);

// ============================================================================
// PROVIDER REGISTRY (from providers/provider.c)
// ============================================================================

// Forward declaration
struct Provider;
typedef enum ProviderType ProviderType;

/**
 * Provider registry uses enum-indexed static array for O(1) lookup.
 * Capacity: Fixed (PROVIDER_COUNT from provider.h)
 * Thread-safe: Yes (immutable after init)
 */

int provider_registry_init(void);
void provider_registry_shutdown(void);

struct Provider* provider_registry_get(ProviderType type);
struct Provider* provider_registry_get_by_name(const char* name);
size_t provider_registry_count(void);
RegistryStats provider_registry_stats(void);

// Check availability
bool provider_registry_is_available(ProviderType type);
ProviderType provider_registry_get_default(void);

// ============================================================================
// TOOL REGISTRY (from tools/tools.c)
// ============================================================================

// Forward declaration
struct ToolDefinition;

/**
 * Tool registry uses config-based dynamic loading.
 * Tools are loaded from embedded definitions and can be extended.
 * Capacity: Dynamic
 * Thread-safe: Yes (mutex protected)
 */

int tool_registry_init(void);
void tool_registry_shutdown(void);

RegistryResult tool_registry_add(struct ToolDefinition* tool);
RegistryResult tool_registry_remove(const char* name);
struct ToolDefinition* tool_registry_find(const char* name);
size_t tool_registry_count(void);
RegistryStats tool_registry_stats(void);

// Tool execution
int tool_registry_execute(const char* name, const char* args, char** result);

// ============================================================================
// ORCHESTRATOR REGISTRY (from orchestrator/orchestrator.c)
// ============================================================================

/**
 * Orchestrator registry manages the global orchestrator state.
 * Uses hash tables for agent lookups (by ID and by name).
 * Capacity: Single instance
 * Thread-safe: Yes (mutex protected)
 */

// Orchestrator is a singleton, so registry functions are wrappers
int orchestrator_registry_init(double budget);
void orchestrator_registry_shutdown(void);
RegistryStats orchestrator_registry_stats(void);

#endif // NOUS_REGISTRY_H
