/**
 * Memory System Implementation - Ali's Historical Memory
 *
 * Provides cross-session memory persistence and retrieval.
 */

#include "nous/memory.h"
#include "nous/orchestrator.h"
#include "nous/safe_path.h"
#include <cjson/cJSON.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define MEMORY_BASE_DIR "~/.convergio/memory"
#define MEMORY_SUMMARIES_DIR "~/.convergio/memory/summaries"
#define MEMORY_INDEX_FILE "~/.convergio/memory/index.json"
#define MAX_MEMORIES 1000

// Expand ~ to home directory
static char* expand_path(const char* path) {
    if (path[0] != '~') {
        return strdup(path);
    }
    const char* home = getenv("HOME");
    if (!home)
        home = "/tmp";
    size_t len = strlen(home) + strlen(path);
    char* expanded = malloc(len);
    snprintf(expanded, len, "%s%s", home, path + 1);
    return expanded;
}

// Ensure directories exist
static void ensure_dirs(void) {
    char* base = expand_path(MEMORY_BASE_DIR);
    char* summaries = expand_path(MEMORY_SUMMARIES_DIR);
    mkdir(base, 0755);
    mkdir(summaries, 0755);
    free(base);
    free(summaries);
}

// Generate unique memory ID
static void generate_memory_id(char* id, size_t size) {
    snprintf(id, size, "mem_%ld_%d", time(NULL), rand() % 10000);
}

int memory_init(void) {
    ensure_dirs();
    srand((unsigned int)time(NULL));
    return 0;
}

void memory_shutdown(void) {
    // Nothing to cleanup
}

int memory_save(const MemoryEntry* entry) {
    if (!entry || !entry->id[0])
        return -1;

    ensure_dirs();

    // Save to summaries directory
    char* dir = expand_path(MEMORY_SUMMARIES_DIR);
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s.json", dir, entry->id);
    free(dir);

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "id", entry->id);
    cJSON_AddStringToObject(json, "agent_name", entry->agent_name);
    cJSON_AddStringToObject(json, "summary", entry->summary);
    cJSON_AddStringToObject(json, "topics", entry->topics);
    cJSON_AddStringToObject(json, "decisions", entry->decisions);
    cJSON_AddStringToObject(json, "action_items", entry->action_items);
    cJSON_AddNumberToObject(json, "timestamp", (double)entry->timestamp);
    cJSON_AddNumberToObject(json, "importance", entry->importance);

    char* json_str = cJSON_Print(json);
    cJSON_Delete(json);

    int fd =
        safe_path_open(filepath, safe_path_get_user_boundary(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    FILE* f = fd >= 0 ? fdopen(fd, "w") : NULL;
    if (!f) {
        free(json_str);
        return -1;
    }
    fputs(json_str, f);
    fclose(f);
    free(json_str);

    return 0;
}

int memory_generate_summary(const char* agent_name, const char** messages, const char** roles,
                            int message_count, MemoryEntry* entry) {
    if (!agent_name || !messages || !roles || message_count < 2 || !entry) {
        return -1;
    }

    memset(entry, 0, sizeof(MemoryEntry));
    generate_memory_id(entry->id, sizeof(entry->id));
    strncpy(entry->agent_name, agent_name, sizeof(entry->agent_name) - 1);
    entry->timestamp = time(NULL);
    entry->importance = 3; // Default medium importance

    // Build conversation text for summarization
    size_t conv_size = 8192;
    char* conversation = malloc(conv_size);
    size_t conv_len = 0;

    for (int i = 0; i < message_count && conv_len < conv_size - 512; i++) {
        const char* role = roles[i];
        const char* msg = messages[i];
        if (!msg)
            continue;

        // Truncate long messages
        int written = snprintf(conversation + conv_len, conv_size - conv_len, "%s: %.500s%s\n",
                               strcmp(role, "user") == 0 ? "User" : "Agent", msg,
                               strlen(msg) > 500 ? "..." : "");
        if (written > 0)
            conv_len += (size_t)written;
    }

    // Build summarization prompt
    char prompt[16384];
    snprintf(prompt, sizeof(prompt),
             "Analyze this conversation with %s and provide a structured summary.\n\n"
             "CONVERSATION:\n%s\n\n"
             "Respond with ONLY a JSON object (no markdown, no explanation):\n"
             "{\n"
             "  \"summary\": \"2-3 sentence summary of what was discussed\",\n"
             "  \"topics\": \"comma-separated key topics\",\n"
             "  \"decisions\": \"key decisions made (or 'none')\",\n"
             "  \"action_items\": \"action items identified (or 'none')\",\n"
             "  \"importance\": 1-5 (1=trivial, 5=critical)\n"
             "}",
             agent_name, conversation);

    free(conversation);

    // Call LLM for summarization (using orchestrator's process)
    char* response = orchestrator_process(prompt);
    if (!response) {
        // Fallback: create basic summary without LLM
        snprintf(entry->summary, sizeof(entry->summary), "Conversation with %s (%d messages)",
                 agent_name, message_count);
        strncpy(entry->topics, agent_name, sizeof(entry->topics) - 1);
        strncpy(entry->decisions, "none", sizeof(entry->decisions) - 1);
        strncpy(entry->action_items, "none", sizeof(entry->action_items) - 1);
        return 0;
    }

    // Parse JSON response
    cJSON* json = cJSON_Parse(response);
    free(response);

    if (json) {
        cJSON* item;

        item = cJSON_GetObjectItem(json, "summary");
        if (item && cJSON_IsString(item)) {
            strncpy(entry->summary, item->valuestring, sizeof(entry->summary) - 1);
        }

        item = cJSON_GetObjectItem(json, "topics");
        if (item && cJSON_IsString(item)) {
            strncpy(entry->topics, item->valuestring, sizeof(entry->topics) - 1);
        }

        item = cJSON_GetObjectItem(json, "decisions");
        if (item && cJSON_IsString(item)) {
            strncpy(entry->decisions, item->valuestring, sizeof(entry->decisions) - 1);
        }

        item = cJSON_GetObjectItem(json, "action_items");
        if (item && cJSON_IsString(item)) {
            strncpy(entry->action_items, item->valuestring, sizeof(entry->action_items) - 1);
        }

        item = cJSON_GetObjectItem(json, "importance");
        if (item && cJSON_IsNumber(item)) {
            entry->importance = (int)item->valuedouble;
            if (entry->importance < 1)
                entry->importance = 1;
            if (entry->importance > 5)
                entry->importance = 5;
        }

        cJSON_Delete(json);
    } else {
        // Response wasn't valid JSON, use as plain summary
        strncpy(entry->summary, response, sizeof(entry->summary) - 1);
    }

    return 0;
}

// Load a single memory entry from file
static int load_memory_entry(const char* filepath, MemoryEntry* entry) {
    int fd = safe_path_open(filepath, safe_path_get_user_boundary(), O_RDONLY, 0);
    FILE* f = fd >= 0 ? fdopen(fd, "r") : NULL;
    if (!f)
        return -1;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0 || fsize > 1024 * 1024) {
        fclose(f);
        return -1;
    }

    char* content = malloc((size_t)fsize + 1);
    fread(content, 1, (size_t)fsize, f);
    content[(size_t)fsize] = '\0';
    fclose(f);

    cJSON* json = cJSON_Parse(content);
    free(content);
    if (!json)
        return -1;

    memset(entry, 0, sizeof(MemoryEntry));

    cJSON* item;
    item = cJSON_GetObjectItem(json, "id");
    if (item && cJSON_IsString(item)) {
        strncpy(entry->id, item->valuestring, sizeof(entry->id) - 1);
    }

    item = cJSON_GetObjectItem(json, "agent_name");
    if (item && cJSON_IsString(item)) {
        strncpy(entry->agent_name, item->valuestring, sizeof(entry->agent_name) - 1);
    }

    item = cJSON_GetObjectItem(json, "summary");
    if (item && cJSON_IsString(item)) {
        strncpy(entry->summary, item->valuestring, sizeof(entry->summary) - 1);
    }

    item = cJSON_GetObjectItem(json, "topics");
    if (item && cJSON_IsString(item)) {
        strncpy(entry->topics, item->valuestring, sizeof(entry->topics) - 1);
    }

    item = cJSON_GetObjectItem(json, "decisions");
    if (item && cJSON_IsString(item)) {
        strncpy(entry->decisions, item->valuestring, sizeof(entry->decisions) - 1);
    }

    item = cJSON_GetObjectItem(json, "action_items");
    if (item && cJSON_IsString(item)) {
        strncpy(entry->action_items, item->valuestring, sizeof(entry->action_items) - 1);
    }

    item = cJSON_GetObjectItem(json, "timestamp");
    if (item && cJSON_IsNumber(item)) {
        entry->timestamp = (time_t)item->valuedouble;
    }

    item = cJSON_GetObjectItem(json, "importance");
    if (item && cJSON_IsNumber(item)) {
        entry->importance = (int)item->valuedouble;
    }

    cJSON_Delete(json);
    return 0;
}

// Compare memories by timestamp (newest first)
static int compare_memories_by_time(const void* a, const void* b) {
    const MemoryEntry* ma = (const MemoryEntry*)a;
    const MemoryEntry* mb = (const MemoryEntry*)b;
    if (mb->timestamp > ma->timestamp)
        return 1;
    if (mb->timestamp < ma->timestamp)
        return -1;
    return 0;
}

int memory_load_recent(int max_entries, MemorySearchResult* result) {
    if (!result || max_entries <= 0)
        return -1;

    result->entries = NULL;
    result->count = 0;
    result->capacity = 0;

    char* dir = expand_path(MEMORY_SUMMARIES_DIR);
    DIR* d = opendir(dir);
    if (!d) {
        free(dir);
        return 0; // No memories yet, not an error
    }

    // Allocate space for entries
    result->capacity = (size_t)max_entries;
    result->entries = calloc(result->capacity, sizeof(MemoryEntry));

    struct dirent* entry;
    MemoryEntry* temp_entries = calloc(MAX_MEMORIES, sizeof(MemoryEntry));
    int temp_count = 0;

    while ((entry = readdir(d)) != NULL && temp_count < MAX_MEMORIES) {
        if (strstr(entry->d_name, ".json") == NULL)
            continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);

        if (load_memory_entry(filepath, &temp_entries[temp_count]) == 0) {
            temp_count++;
        }
    }

    closedir(d);
    free(dir);

    // Sort by timestamp (newest first)
    qsort(temp_entries, (size_t)temp_count, sizeof(MemoryEntry), compare_memories_by_time);

    // Copy top N entries
    int copy_count = temp_count < max_entries ? temp_count : max_entries;
    for (int i = 0; i < copy_count; i++) {
        result->entries[i] = temp_entries[i];
        result->count++;
    }

    free(temp_entries);
    return 0;
}

int memory_search(const char* query, int max_results, MemorySearchResult* result) {
    if (!query || !result || max_results <= 0)
        return -1;

    result->entries = NULL;
    result->count = 0;
    result->capacity = 0;

    char* dir = expand_path(MEMORY_SUMMARIES_DIR);
    DIR* d = opendir(dir);
    if (!d) {
        free(dir);
        return 0;
    }

    result->capacity = (size_t)max_results;
    result->entries = calloc(result->capacity, sizeof(MemoryEntry));

    // Convert query to lowercase for case-insensitive search
    char query_lower[256];
    strncpy(query_lower, query, sizeof(query_lower) - 1);
    for (char* p = query_lower; *p; p++) {
        if (*p >= 'A' && *p <= 'Z')
            *p += 32;
    }

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL && result->count < (size_t)max_results) {
        if (strstr(entry->d_name, ".json") == NULL)
            continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);

        MemoryEntry mem;
        if (load_memory_entry(filepath, &mem) != 0)
            continue;

        // Search in summary, topics, agent_name
        char search_text[4096];
        snprintf(search_text, sizeof(search_text), "%s %s %s %s", mem.summary, mem.topics,
                 mem.agent_name, mem.decisions);

        // Convert to lowercase
        for (char* p = search_text; *p; p++) {
            if (*p >= 'A' && *p <= 'Z')
                *p += 32;
        }

        if (strstr(search_text, query_lower) != NULL) {
            result->entries[result->count++] = mem;
        }
    }

    closedir(d);
    free(dir);

    return 0;
}

int memory_load_by_agent(const char* agent_name, int max_entries, MemorySearchResult* result) {
    if (!agent_name || !result || max_entries <= 0)
        return -1;

    result->entries = NULL;
    result->count = 0;
    result->capacity = 0;

    char* dir = expand_path(MEMORY_SUMMARIES_DIR);
    DIR* d = opendir(dir);
    if (!d) {
        free(dir);
        return 0;
    }

    result->capacity = (size_t)max_entries;
    result->entries = calloc(result->capacity, sizeof(MemoryEntry));

    struct dirent* entry;
    MemoryEntry* temp_entries = calloc(MAX_MEMORIES, sizeof(MemoryEntry));
    int temp_count = 0;

    while ((entry = readdir(d)) != NULL && temp_count < MAX_MEMORIES) {
        if (strstr(entry->d_name, ".json") == NULL)
            continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);

        MemoryEntry mem;
        if (load_memory_entry(filepath, &mem) != 0)
            continue;

        if (strstr(mem.agent_name, agent_name) != NULL) {
            temp_entries[temp_count++] = mem;
        }
    }

    closedir(d);
    free(dir);

    // Sort by timestamp (newest first)
    qsort(temp_entries, (size_t)temp_count, sizeof(MemoryEntry), compare_memories_by_time);

    // Copy top N entries
    int copy_count = temp_count < max_entries ? temp_count : max_entries;
    for (int i = 0; i < copy_count; i++) {
        result->entries[i] = temp_entries[i];
        result->count++;
    }

    free(temp_entries);
    return 0;
}

char* memory_build_context(const MemorySearchResult* result, size_t max_length) {
    if (!result || result->count == 0)
        return NULL;

    char* context = malloc(max_length);
    size_t len = 0;
    int written;

    written = snprintf(context + len, max_length - len,
                       "\n## Historical Memory (Cross-Session Context)\n\n"
                       "You have access to summaries of past conversations. Use this context to "
                       "maintain continuity.\n\n");
    if (written > 0)
        len += (size_t)written;

    for (size_t i = 0; i < result->count && len < max_length - 512; i++) {
        const MemoryEntry* mem = &result->entries[i];

        // Format timestamp
        char time_str[64];
        struct tm* tm_info = localtime(&mem->timestamp);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);

        written = snprintf(context + len, max_length - len,
                           "### Memory: %s (%s)\n"
                           "**Summary**: %s\n"
                           "**Topics**: %s\n",
                           mem->agent_name, time_str, mem->summary, mem->topics);
        if (written > 0)
            len += (size_t)written;

        if (strcmp(mem->decisions, "none") != 0 && mem->decisions[0]) {
            written =
                snprintf(context + len, max_length - len, "**Decisions**: %s\n", mem->decisions);
            if (written > 0)
                len += (size_t)written;
        }

        if (strcmp(mem->action_items, "none") != 0 && mem->action_items[0]) {
            written = snprintf(context + len, max_length - len, "**Action Items**: %s\n",
                               mem->action_items);
            if (written > 0)
                len += (size_t)written;
        }

        written = snprintf(context + len, max_length - len, "\n");
        if (written > 0)
            len += (size_t)written;
    }

    return context;
}

void memory_free_result(MemorySearchResult* result) {
    if (result && result->entries) {
        free(result->entries);
        result->entries = NULL;
        result->count = 0;
        result->capacity = 0;
    }
}

int memory_get_stats(MemoryStats* stats) {
    if (!stats)
        return -1;

    memset(stats, 0, sizeof(MemoryStats));
    stats->oldest_memory = time(NULL);
    stats->newest_memory = 0;

    char* dir = expand_path(MEMORY_SUMMARIES_DIR);
    DIR* d = opendir(dir);
    if (!d) {
        free(dir);
        return 0;
    }

    time_t week_ago = time(NULL) - (7 * 24 * 60 * 60);
    char agents_seen[1024][64];
    int agent_count = 0;

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (strstr(entry->d_name, ".json") == NULL)
            continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);

        MemoryEntry mem;
        if (load_memory_entry(filepath, &mem) != 0)
            continue;

        stats->total_memories++;

        if (mem.timestamp > week_ago) {
            stats->memories_this_week++;
        }

        if (mem.timestamp < stats->oldest_memory) {
            stats->oldest_memory = mem.timestamp;
        }
        if (mem.timestamp > stats->newest_memory) {
            stats->newest_memory = mem.timestamp;
        }

        // Track unique agents
        int found = 0;
        for (int i = 0; i < agent_count; i++) {
            if (strcmp(agents_seen[i], mem.agent_name) == 0) {
                found = 1;
                break;
            }
        }
        if (!found && agent_count < 1024) {
            strncpy(agents_seen[agent_count++], mem.agent_name, 63);
        }
    }

    stats->agents_with_memories = agent_count;

    closedir(d);
    free(dir);

    return 0;
}
