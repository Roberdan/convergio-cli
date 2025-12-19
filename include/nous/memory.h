/**
 * Memory System - Ali's Historical Memory
 *
 * Provides cross-session memory for Ali to maintain continuity
 * and context across conversations with all agents.
 *
 * Storage: ~/.convergio/memory/
 * - summaries/  - Conversation summaries per agent
 * - index.json  - Searchable index of all memories
 */

#ifndef NOUS_MEMORY_H
#define NOUS_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Memory entry representing a conversation summary
typedef struct {
    char id[64];              // Unique memory ID
    char agent_name[64];      // Agent involved
    char summary[2048];       // Conversation summary
    char topics[512];         // Key topics (comma-separated)
    char decisions[1024];     // Key decisions made
    char action_items[1024];  // Action items identified
    time_t timestamp;         // When memory was created
    int importance;           // 1-5 importance score
} MemoryEntry;

// Memory search result
typedef struct {
    MemoryEntry* entries;
    size_t count;
    size_t capacity;
} MemorySearchResult;

/**
 * Initialize memory system
 * Creates directories if needed
 */
int memory_init(void);

/**
 * Shutdown memory system
 */
void memory_shutdown(void);

/**
 * Save a memory entry (conversation summary)
 * Called when a significant conversation ends
 */
int memory_save(const MemoryEntry* entry);

/**
 * Generate a summary from conversation messages
 * Uses LLM to create intelligent summary
 *
 * @param agent_name Name of the agent
 * @param messages Array of message strings (alternating user/assistant)
 * @param message_count Number of messages
 * @param entry Output entry (caller allocates)
 * @return 0 on success, -1 on error
 */
int memory_generate_summary(const char* agent_name,
                           const char** messages,
                           const char** roles,
                           int message_count,
                           MemoryEntry* entry);

/**
 * Load recent memories for Ali context injection
 *
 * @param max_entries Maximum entries to load
 * @param result Output search result (caller frees with memory_free_result)
 * @return 0 on success, -1 on error
 */
int memory_load_recent(int max_entries, MemorySearchResult* result);

/**
 * Search memories by query
 *
 * @param query Search query (matches topics, summaries)
 * @param max_results Maximum results
 * @param result Output search result
 * @return 0 on success, -1 on error
 */
int memory_search(const char* query, int max_results, MemorySearchResult* result);

/**
 * Load memories for specific agent
 */
int memory_load_by_agent(const char* agent_name, int max_entries, MemorySearchResult* result);

/**
 * Build context string for Ali from memories
 * Formats memories into a prompt-friendly string
 *
 * @param result Memory search result
 * @param max_length Maximum output length
 * @return Allocated string (caller frees) or NULL on error
 */
char* memory_build_context(const MemorySearchResult* result, size_t max_length);

/**
 * Free memory search result
 */
void memory_free_result(MemorySearchResult* result);

/**
 * Get memory statistics
 */
typedef struct {
    int total_memories;
    int memories_this_week;
    int agents_with_memories;
    time_t oldest_memory;
    time_t newest_memory;
} MemoryStats;

int memory_get_stats(MemoryStats* stats);

#endif // NOUS_MEMORY_H
