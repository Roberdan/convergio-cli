/**
 * CONVERGIO COST OPTIMIZER
 *
 * Intelligent cost optimization strategies:
 * - Prompt caching detection and utilization
 * - Batch processing for non-urgent tasks
 * - Model tiering based on task complexity
 * - Budget-aware model downgrading
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define CACHE_HASH_SIZE 256
#define CACHE_TTL_SECONDS 300          // 5 minutes default cache TTL
#define BATCH_QUEUE_SIZE 100
#define COST_HISTORY_SIZE 1000

// ============================================================================
// PROMPT CACHE
// ============================================================================

typedef struct CacheEntry {
    char* content_hash;                // SHA-256 of prompt content
    char* cached_id;                   // Provider's cache ID
    ProviderType provider;
    time_t created_at;
    time_t expires_at;
    size_t token_count;
    struct CacheEntry* next;
} CacheEntry;

typedef struct {
    CacheEntry* buckets[CACHE_HASH_SIZE];
    pthread_mutex_t mutex;
    size_t entry_count;
    size_t cache_hits;
    size_t cache_misses;
} PromptCache;

// ============================================================================
// BATCH PROCESSING
// ============================================================================

typedef enum {
    BATCH_PRIORITY_LOW,                // Can wait 24 hours
    BATCH_PRIORITY_MEDIUM,             // Process within hours
    BATCH_PRIORITY_HIGH                // Process soon
} BatchPriority;

typedef struct {
    char* request_id;
    char* model;
    char* system_prompt;
    char* user_message;
    BatchPriority priority;
    void (*callback)(const char* response, void* ctx);
    void* ctx;
    time_t submitted_at;
    time_t deadline;
} BatchRequest;

typedef struct {
    BatchRequest requests[BATCH_QUEUE_SIZE];
    size_t count;
    pthread_mutex_t mutex;
    bool processing;
} BatchQueue;

// ============================================================================
// COST TRACKING
// ============================================================================

typedef struct {
    time_t timestamp;
    const char* model;
    ProviderType provider;
    size_t input_tokens;
    size_t output_tokens;
    double cost;
    bool was_cached;
    bool was_downgraded;
} CostRecord;

typedef struct {
    CostRecord records[COST_HISTORY_SIZE];
    size_t count;
    size_t head;                       // Circular buffer head
    pthread_mutex_t mutex;

    // Aggregates
    double total_cost;
    double cached_savings;
    double downgrade_savings;
    size_t total_requests;
} CostHistory;

// ============================================================================
// OPTIMIZER STATE
// ============================================================================

typedef struct {
    PromptCache prompt_cache;
    BatchQueue batch_queue;
    CostHistory cost_history;

    // Configuration
    bool caching_enabled;
    bool batching_enabled;
    bool auto_downgrade_enabled;
    double daily_budget;
    double monthly_budget;

    // Statistics
    double estimated_monthly_cost;
    double average_request_cost;

    bool initialized;
} CostOptimizer;

static CostOptimizer g_optimizer = {0};

// ============================================================================
// HASH FUNCTIONS
// ============================================================================

// Simple DJB2 hash for prompt content
static unsigned int hash_string(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + (unsigned int)c;
    }
    return hash % CACHE_HASH_SIZE;
}

// Create a content hash for cache key
static char* create_content_hash(const char* system, const char* user) {
    // Combine system and user prompts
    size_t sys_len = system ? strlen(system) : 0;
    size_t usr_len = user ? strlen(user) : 0;

    char* combined = malloc(sys_len + usr_len + 2);
    if (!combined) return NULL;

    (void)(sys_len + usr_len);  // total_len was unused
    size_t offset = 0;
    if (system) {
        memcpy(combined, system, sys_len);
        offset = sys_len;
    }
    combined[offset++] = '|';
    if (user) {
        memcpy(combined + offset, user, usr_len);
        offset += usr_len;
    }
    combined[offset] = '\0';

    // Create hash
    unsigned int hash = hash_string(combined);
    char* hash_str = malloc(32);
    if (hash_str) {
        snprintf(hash_str, 32, "%08x", hash);
    }

    free(combined);
    return hash_str;
}

// ============================================================================
// PROMPT CACHE IMPLEMENTATION
// ============================================================================

static void cache_init(PromptCache* cache) {
    memset(cache->buckets, 0, sizeof(cache->buckets));
    pthread_mutex_init(&cache->mutex, NULL);
    cache->entry_count = 0;
    cache->cache_hits = 0;
    cache->cache_misses = 0;
}

static void cache_cleanup(PromptCache* cache) {
    pthread_mutex_lock(&cache->mutex);

    for (int i = 0; i < CACHE_HASH_SIZE; i++) {
        CacheEntry* entry = cache->buckets[i];
        while (entry) {
            CacheEntry* next = entry->next;
            free(entry->content_hash);
            free(entry->cached_id);
            free(entry);
            entry = next;
        }
        cache->buckets[i] = NULL;
    }

    cache->entry_count = 0;
    pthread_mutex_unlock(&cache->mutex);
    pthread_mutex_destroy(&cache->mutex);
}

static CacheEntry* cache_lookup(PromptCache* cache, const char* hash, ProviderType provider) {
    unsigned int bucket = hash_string(hash);

    pthread_mutex_lock(&cache->mutex);

    CacheEntry* entry = cache->buckets[bucket];
    time_t now = time(NULL);

    while (entry) {
        if (strcmp(entry->content_hash, hash) == 0 &&
            entry->provider == provider &&
            entry->expires_at > now) {
            cache->cache_hits++;
            pthread_mutex_unlock(&cache->mutex);
            return entry;
        }
        entry = entry->next;
    }

    cache->cache_misses++;
    pthread_mutex_unlock(&cache->mutex);
    return NULL;
}

static void cache_insert(PromptCache* cache, const char* hash, const char* cached_id,
                         ProviderType provider, size_t tokens, int ttl_seconds) {
    unsigned int bucket = hash_string(hash);

    pthread_mutex_lock(&cache->mutex);

    // Check if already exists
    CacheEntry* entry = cache->buckets[bucket];
    while (entry) {
        if (strcmp(entry->content_hash, hash) == 0 && entry->provider == provider) {
            // Update existing
            free(entry->cached_id);
            entry->cached_id = strdup(cached_id);
            entry->expires_at = time(NULL) + ttl_seconds;
            entry->token_count = tokens;
            pthread_mutex_unlock(&cache->mutex);
            return;
        }
        entry = entry->next;
    }

    // Create new entry
    entry = calloc(1, sizeof(CacheEntry));
    if (!entry) {
        pthread_mutex_unlock(&cache->mutex);
        return;
    }

    entry->content_hash = strdup(hash);
    entry->cached_id = strdup(cached_id);
    entry->provider = provider;
    entry->created_at = time(NULL);
    entry->expires_at = time(NULL) + ttl_seconds;
    entry->token_count = tokens;

    // Insert at head of bucket
    entry->next = cache->buckets[bucket];
    cache->buckets[bucket] = entry;
    cache->entry_count++;

    pthread_mutex_unlock(&cache->mutex);
}

// ============================================================================
// BATCH QUEUE IMPLEMENTATION
// ============================================================================

static void batch_init(BatchQueue* queue) {
    memset(queue->requests, 0, sizeof(queue->requests));
    pthread_mutex_init(&queue->mutex, NULL);
    queue->count = 0;
    queue->processing = false;
}

static void batch_cleanup(BatchQueue* queue) {
    pthread_mutex_lock(&queue->mutex);

    for (size_t i = 0; i < queue->count; i++) {
        free(queue->requests[i].request_id);
        free(queue->requests[i].model);
        free(queue->requests[i].system_prompt);
        free(queue->requests[i].user_message);
    }
    queue->count = 0;

    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
}

static int batch_add(BatchQueue* queue, const char* model, const char* system,
                     const char* user, BatchPriority priority,
                     void (*callback)(const char*, void*), void* ctx) {
    pthread_mutex_lock(&queue->mutex);

    if (queue->count >= BATCH_QUEUE_SIZE) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;  // Queue full
    }

    BatchRequest* req = &queue->requests[queue->count];

    // Generate request ID
    req->request_id = malloc(32);
    if (req->request_id) {
        snprintf(req->request_id, 32, "batch_%zu_%ld", queue->count, time(NULL));
    }

    req->model = model ? strdup(model) : NULL;
    req->system_prompt = system ? strdup(system) : NULL;
    req->user_message = user ? strdup(user) : NULL;
    req->priority = priority;
    req->callback = callback;
    req->ctx = ctx;
    req->submitted_at = time(NULL);

    // Set deadline based on priority
    switch (priority) {
        case BATCH_PRIORITY_LOW:
            req->deadline = req->submitted_at + 86400;  // 24 hours
            break;
        case BATCH_PRIORITY_MEDIUM:
            req->deadline = req->submitted_at + 3600;   // 1 hour
            break;
        case BATCH_PRIORITY_HIGH:
            req->deadline = req->submitted_at + 300;    // 5 minutes
            break;
    }

    queue->count++;
    pthread_mutex_unlock(&queue->mutex);

    LOG_DEBUG(LOG_CAT_COST, "Batch request queued: %s (priority=%d)",
              req->request_id, priority);
    return 0;
}

// ============================================================================
// COST HISTORY IMPLEMENTATION
// ============================================================================

static void history_init(CostHistory* history) {
    memset(history, 0, sizeof(CostHistory));
    pthread_mutex_init(&history->mutex, NULL);
}

static void history_cleanup(CostHistory* history) {
    pthread_mutex_destroy(&history->mutex);
}

static void history_record(CostHistory* history, const char* model, ProviderType provider,
                          size_t input_tokens, size_t output_tokens, double cost,
                          bool was_cached, bool was_downgraded) {
    pthread_mutex_lock(&history->mutex);

    // Add to circular buffer
    size_t idx = history->head;
    CostRecord* record = &history->records[idx];

    record->timestamp = time(NULL);
    record->model = model;
    record->provider = provider;
    record->input_tokens = input_tokens;
    record->output_tokens = output_tokens;
    record->cost = cost;
    record->was_cached = was_cached;
    record->was_downgraded = was_downgraded;

    history->head = (history->head + 1) % COST_HISTORY_SIZE;
    if (history->count < COST_HISTORY_SIZE) {
        history->count++;
    }

    // Update aggregates
    history->total_cost += cost;
    history->total_requests++;

    if (was_cached) {
        // Estimate savings from caching (90% of input cost)
        const ModelConfig* config = model_get_config(model);
        if (config) {
            double full_cost = (double)input_tokens / 1000000.0 * config->input_cost_per_mtok;
            history->cached_savings += full_cost * 0.9;
        }
    }

    pthread_mutex_unlock(&history->mutex);
}

// ============================================================================
// COST OPTIMIZER PUBLIC API
// ============================================================================

int cost_optimizer_init(void) {
    if (g_optimizer.initialized) return 0;

    cache_init(&g_optimizer.prompt_cache);
    batch_init(&g_optimizer.batch_queue);
    history_init(&g_optimizer.cost_history);

    // Default configuration
    g_optimizer.caching_enabled = true;
    g_optimizer.batching_enabled = true;
    g_optimizer.auto_downgrade_enabled = true;
    g_optimizer.daily_budget = 50.0;
    g_optimizer.monthly_budget = 500.0;

    g_optimizer.initialized = true;

    LOG_INFO(LOG_CAT_COST, "Cost optimizer initialized");
    return 0;
}

void cost_optimizer_shutdown(void) {
    if (!g_optimizer.initialized) return;

    cache_cleanup(&g_optimizer.prompt_cache);
    batch_cleanup(&g_optimizer.batch_queue);
    history_cleanup(&g_optimizer.cost_history);

    g_optimizer.initialized = false;

    LOG_INFO(LOG_CAT_COST, "Cost optimizer shutdown. Total cost: $%.4f, Savings: $%.4f",
             g_optimizer.cost_history.total_cost,
             g_optimizer.cost_history.cached_savings);
}

// ============================================================================
// OPTIMIZATION STRATEGIES
// ============================================================================

/**
 * Check if a prompt can use caching
 */
bool cost_check_cache(const char* system, const char* user, ProviderType provider,
                      char** out_cache_id) {
    if (!g_optimizer.caching_enabled) return false;

    char* hash = create_content_hash(system, user);
    if (!hash) return false;

    CacheEntry* entry = cache_lookup(&g_optimizer.prompt_cache, hash, provider);
    free(hash);

    if (entry && out_cache_id) {
        *out_cache_id = strdup(entry->cached_id);
        return true;
    }

    return false;
}

/**
 * Register a cache hit for future use
 */
void cost_register_cache(const char* system, const char* user, ProviderType provider,
                         const char* cache_id, size_t tokens) {
    if (!g_optimizer.caching_enabled) return;

    char* hash = create_content_hash(system, user);
    if (!hash) return;

    // Different TTLs per provider
    int ttl = CACHE_TTL_SECONDS;
    if (provider == PROVIDER_ANTHROPIC) {
        ttl = 3600;  // Anthropic cache lasts longer
    }

    cache_insert(&g_optimizer.prompt_cache, hash, cache_id, provider, tokens, ttl);
    free(hash);

    LOG_DEBUG(LOG_CAT_COST, "Prompt cached for provider %d, tokens=%zu", provider, tokens);
}

/**
 * Get the optimal model based on task and budget
 */
const char* cost_get_optimal_model(const char* preferred_model,
                                   double remaining_budget,
                                   bool requires_vision,
                                   bool requires_tools,
                                   size_t estimated_tokens) {
    if (!g_optimizer.auto_downgrade_enabled) {
        return preferred_model;
    }

    const ModelConfig* preferred = model_get_config(preferred_model);
    if (!preferred) return preferred_model;

    // Estimate cost for this request
    double estimated_cost = model_estimate_cost(preferred_model, estimated_tokens, estimated_tokens / 2);

    // If we have enough budget, use preferred
    if (estimated_cost < remaining_budget * 0.1) {  // Use at most 10% of remaining budget
        return preferred_model;
    }

    // Find cheaper alternative
    const char* alternatives[] = {
        "anthropic/claude-haiku-3.5",
        "gemini/gemini-1.5-flash",
        "openai/gpt-4o-mini",
        "openai/o1-mini"
    };

    for (int i = 0; i < 4; i++) {
        const ModelConfig* alt = model_get_config(alternatives[i]);
        if (!alt) continue;

        // Check feature requirements
        if (requires_vision && !alt->supports_vision) continue;
        if (requires_tools && !alt->supports_tools) continue;

        // Check if provider is available
        if (!provider_is_available(alt->provider)) continue;

        // Check cost
        double alt_cost = model_estimate_cost(alternatives[i], estimated_tokens, estimated_tokens / 2);
        if (alt_cost < remaining_budget * 0.1) {
            LOG_INFO(LOG_CAT_COST, "Downgrading from %s to %s (budget: $%.2f)",
                     preferred_model, alternatives[i], remaining_budget);
            return alternatives[i];
        }
    }

    return preferred_model;  // Fallback to preferred
}

/**
 * Record a completed request for cost tracking
 */
void cost_record_request(const char* model, ProviderType provider,
                         size_t input_tokens, size_t output_tokens,
                         double cost, bool was_cached) {
    history_record(&g_optimizer.cost_history, model, provider,
                   input_tokens, output_tokens, cost, was_cached, false);
}

/**
 * Queue a request for batch processing
 */
int cost_queue_batch(const char* model, const char* system, const char* user,
                     int priority, void (*callback)(const char*, void*), void* ctx) {
    if (!g_optimizer.batching_enabled) return -1;

    BatchPriority p = (priority < 0) ? BATCH_PRIORITY_LOW :
                      (priority > 1) ? BATCH_PRIORITY_HIGH : BATCH_PRIORITY_MEDIUM;

    return batch_add(&g_optimizer.batch_queue, model, system, user, p, callback, ctx);
}

// ============================================================================
// STATISTICS & REPORTING
// ============================================================================

/**
 * Get cost statistics
 */
void cost_get_stats(double* total_cost, double* cached_savings,
                    size_t* total_requests, size_t* cache_hits, size_t* cache_misses) {
    if (total_cost) *total_cost = g_optimizer.cost_history.total_cost;
    if (cached_savings) *cached_savings = g_optimizer.cost_history.cached_savings;
    if (total_requests) *total_requests = g_optimizer.cost_history.total_requests;
    if (cache_hits) *cache_hits = g_optimizer.prompt_cache.cache_hits;
    if (cache_misses) *cache_misses = g_optimizer.prompt_cache.cache_misses;
}

/**
 * Estimate monthly cost based on current usage
 */
double cost_estimate_monthly(void) {
    CostHistory* h = &g_optimizer.cost_history;

    pthread_mutex_lock(&h->mutex);

    if (h->total_requests == 0) {
        pthread_mutex_unlock(&h->mutex);
        return 0.0;
    }

    // Calculate average daily cost from recent records
    time_t now = time(NULL);
    time_t day_ago = now - 86400;
    double day_cost = 0.0;
    int day_count = 0;

    for (size_t i = 0; i < h->count; i++) {
        if (h->records[i].timestamp >= day_ago) {
            day_cost += h->records[i].cost;
            day_count++;
        }
    }

    pthread_mutex_unlock(&h->mutex);

    if (day_count == 0) {
        return h->total_cost * 30;  // Rough estimate
    }

    return day_cost * 30;  // Extrapolate to monthly
}

/**
 * Print cost report
 */
void cost_print_report(void) {
    double total, savings;
    size_t requests, hits, misses;
    cost_get_stats(&total, &savings, &requests, &hits, &misses);

    double monthly = cost_estimate_monthly();
    double hit_rate = (hits + misses > 0) ? (double)hits / (hits + misses) * 100 : 0;

    printf("\n━━━ Cost Report ━━━\n\n");
    printf("Total Spent:        $%.4f\n", total);
    printf("Cache Savings:      $%.4f\n", savings);
    printf("Net Cost:           $%.4f\n", total - savings);
    printf("\n");
    printf("Total Requests:     %zu\n", requests);
    printf("Cache Hit Rate:     %.1f%%\n", hit_rate);
    printf("\n");
    printf("Est. Monthly:       $%.2f\n", monthly);
    printf("Daily Budget:       $%.2f (%.1f%% used)\n",
           g_optimizer.daily_budget,
           (total / g_optimizer.daily_budget) * 100);
    printf("\n");
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void cost_optimizer_set_budget(double daily, double monthly) {
    g_optimizer.daily_budget = daily;
    g_optimizer.monthly_budget = monthly;
    LOG_INFO(LOG_CAT_COST, "Budget set: daily=$%.2f, monthly=$%.2f", daily, monthly);
}

void cost_enable_caching(bool enabled) {
    g_optimizer.caching_enabled = enabled;
}

void cost_enable_batching(bool enabled) {
    g_optimizer.batching_enabled = enabled;
}

void cost_enable_auto_downgrade(bool enabled) {
    g_optimizer.auto_downgrade_enabled = enabled;
}
