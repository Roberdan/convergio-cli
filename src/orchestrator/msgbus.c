/**
 * CONVERGIO MESSAGE BUS
 *
 * Asynchronous inter-agent communication:
 * - Message routing
 * - Broadcast capability
 * - Message history
 * - Threading support
 */

#include "nous/orchestrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dispatch/dispatch.h>
#include "nous/debug_mutex.h"

// Forward declarations
extern Orchestrator* orchestrator_get(void);

// Thread safety
CONVERGIO_MUTEX_DECLARE(g_msgbus_mutex);

// Message ID counter
static uint64_t g_next_message_id = 1;

// Message queue for async delivery
static dispatch_queue_t g_message_queue = NULL;

// ============================================================================
// INITIALIZATION
// ============================================================================

int msgbus_init(void) {
    if (g_message_queue != NULL) return 0;

    // Create serial queue for message ordering
    g_message_queue = dispatch_queue_create(
        "io.convergio.msgbus",
        DISPATCH_QUEUE_SERIAL
    );

    return g_message_queue ? 0 : -1;
}

void msgbus_shutdown(void) {
    if (g_message_queue) {
        g_message_queue = NULL;
    }
}

// ============================================================================
// MESSAGE CREATION
// ============================================================================

static uint64_t generate_message_id(void) {
    return __sync_fetch_and_add(&g_next_message_id, 1);
}

Message* message_create(MessageType type, SemanticID sender,
                        SemanticID recipient, const char* content) {
    Message* msg = calloc(1, sizeof(Message));
    if (!msg) return NULL;

    msg->id = generate_message_id();
    msg->type = type;
    msg->sender = sender;
    msg->recipient = recipient;
    msg->content = content ? strdup(content) : NULL;
    msg->timestamp = time(NULL);

    return msg;
}

Message* message_create_with_metadata(MessageType type, SemanticID sender,
                                       SemanticID recipient, const char* content,
                                       const char* metadata_json) {
    Message* msg = message_create(type, sender, recipient, content);
    if (msg && metadata_json) {
        msg->metadata_json = strdup(metadata_json);
    }
    return msg;
}

void message_destroy(Message* msg) {
    if (!msg) return;
    free(msg->content);
    free(msg->metadata_json);
    free(msg);
}

// ============================================================================
// MESSAGE DELIVERY
// ============================================================================

// Deliver message to specific agent
static void deliver_to_agent(Message* msg, ManagedAgent* agent) {
    if (!msg || !agent) return;

    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    // Add to agent's pending messages
    msg->next = agent->pending_messages;
    agent->pending_messages = msg;
    agent->last_active = time(NULL);

    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
}

// Send message to specific recipient
void message_send(Message* msg) {
    if (!msg) return;

    Orchestrator* orch = orchestrator_get();
    if (!orch) {
        message_destroy(msg);
        return;
    }

    // Add to history
    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);
    msg->next = orch->message_history;
    orch->message_history = msg;
    orch->message_count++;
    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);

    // Trigger callback
    if (orch->on_message) {
        orch->on_message(msg, orch->callback_ctx);
    }

    // Deliver to recipient if specified
    if (msg->recipient != 0) {
        ManagedAgent* recipient = NULL;
        for (size_t i = 0; i < orch->agent_count; i++) {
            if (orch->agents[i]->id == msg->recipient) {
                recipient = orch->agents[i];
                break;
            }
        }
        if (recipient) {
            // Create copy for agent
            Message* copy = message_create(msg->type, msg->sender,
                                           msg->recipient, msg->content);
            if (copy) {
                copy->metadata_json = msg->metadata_json ? strdup(msg->metadata_json) : NULL;
                copy->parent_id = msg->parent_id;
                copy->tokens_used = msg->tokens_used;
                deliver_to_agent(copy, recipient);
            }
        }
    }
}

// Broadcast message to all active agents
void message_broadcast(Message* msg) {
    if (!msg) return;

    Orchestrator* orch = orchestrator_get();
    if (!orch) {
        message_destroy(msg);
        return;
    }

    // Add to history
    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);
    msg->next = orch->message_history;
    orch->message_history = msg;
    orch->message_count++;
    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);

    // Trigger callback
    if (orch->on_message) {
        orch->on_message(msg, orch->callback_ctx);
    }

    // Deliver copy to each active agent (except sender)
    for (size_t i = 0; i < orch->agent_count; i++) {
        ManagedAgent* agent = orch->agents[i];
        if (agent->is_active && agent->id != msg->sender) {
            Message* copy = message_create(msg->type, msg->sender, agent->id, msg->content);
            if (copy) {
                copy->metadata_json = msg->metadata_json ? strdup(msg->metadata_json) : NULL;
                copy->parent_id = msg->id;
                deliver_to_agent(copy, agent);
            }
        }
    }
}

// ============================================================================
// MESSAGE RETRIEVAL
// ============================================================================

// Get pending messages for an agent
Message* message_get_pending(ManagedAgent* agent) {
    if (!agent) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);
    Message* pending = agent->pending_messages;
    agent->pending_messages = NULL;
    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);

    return pending;
}

// Get recent messages from history
Message** message_get_history(size_t limit, size_t* out_count) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !out_count) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    size_t count = orch->message_count < limit ? orch->message_count : limit;
    Message** messages = malloc(sizeof(Message*) * count);
    if (!messages) {
        CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
        return NULL;
    }

    Message* current = orch->message_history;
    size_t i = 0;
    while (current && i < count) {
        messages[i++] = current;
        current = current->next;
    }

    *out_count = i;
    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);

    return messages;
}

// Get messages of specific type
Message** message_get_by_type(MessageType type, size_t limit, size_t* out_count) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !out_count) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    Message** messages = malloc(sizeof(Message*) * limit);
    if (!messages) {
        CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
        return NULL;
    }

    Message* current = orch->message_history;
    size_t count = 0;
    while (current && count < limit) {
        if (current->type == type) {
            messages[count++] = current;
        }
        current = current->next;
    }

    *out_count = count;
    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);

    return messages;
}

// ============================================================================
// ASYNC MESSAGE PROCESSING
// ============================================================================

typedef struct {
    Message* msg;
    void (*handler)(Message*, void*);
    void* ctx;
} AsyncMessageTask;

// Process message asynchronously
void message_send_async(Message* msg, void (*on_delivered)(Message*, void*), void* ctx) {
    if (!msg || !g_message_queue) {
        if (msg) message_destroy(msg);
        return;
    }

    AsyncMessageTask* task = malloc(sizeof(AsyncMessageTask));
    if (!task) {
        message_destroy(msg);
        return;
    }

    task->msg = msg;
    task->handler = on_delivered;
    task->ctx = ctx;

    dispatch_async(g_message_queue, ^{
        message_send(task->msg);
        if (task->handler) {
            task->handler(task->msg, task->ctx);
        }
        free(task);
    });
}

// ============================================================================
// MESSAGE THREADING
// ============================================================================

// Create a reply to a message
Message* message_reply(Message* original, MessageType type, const char* content) {
    if (!original) return NULL;

    Message* reply = message_create(type, original->recipient, original->sender, content);
    if (reply) {
        reply->parent_id = original->id;
    }
    return reply;
}

// Get thread (chain of related messages)
Message** message_get_thread(uint64_t message_id, size_t* out_count) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !out_count) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    // Find all messages in thread (simplistic: parent_id chain)
    size_t capacity = 32;
    Message** thread = malloc(sizeof(Message*) * capacity);
    if (!thread) {
        CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
        return NULL;
    }

    size_t count = 0;
    Message* current = orch->message_history;

    // First, find the root message
    uint64_t root_id = message_id;
    while (current) {
        if (current->id == root_id && current->parent_id != 0) {
            root_id = current->parent_id;
            current = orch->message_history;  // Restart search
            continue;
        }
        current = current->next;
    }

    // Now collect all messages with this root
    current = orch->message_history;
    while (current) {
        if (current->id == root_id || current->parent_id == root_id) {
            if (count >= capacity) {
                capacity *= 2;
                Message** new_thread = realloc(thread, sizeof(Message*) * capacity);
                if (!new_thread) break;
                thread = new_thread;
            }
            thread[count++] = current;
        }
        current = current->next;
    }

    *out_count = count;
    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);

    return thread;
}

// ============================================================================
// CONVERGENCE MESSAGES
// ============================================================================

// Create a convergence message (final synthesized response)
Message* message_create_convergence(SemanticID sender, const char* content,
                                     Message** source_messages, size_t source_count) {
    Message* msg = message_create(MSG_TYPE_CONVERGENCE, sender, 0, content);
    if (!msg) return NULL;

    // Create metadata with source message IDs
    if (source_messages && source_count > 0) {
        char* metadata = malloc(1024);
        if (metadata) {
            size_t offset = (size_t)snprintf(metadata, 1024, "{\"sources\":[");
            for (size_t i = 0; i < source_count && offset < 900; i++) {
                offset += (size_t)snprintf(metadata + offset, 1024 - offset,
                    "%s%llu", i > 0 ? "," : "", (unsigned long long)source_messages[i]->id);
            }
            snprintf(metadata + offset, 1024 - offset, "]}");
            msg->metadata_json = metadata;
        }
    }

    return msg;
}

// ============================================================================
// MESSAGE STATISTICS
// ============================================================================

typedef struct {
    size_t total_messages;
    size_t user_messages;
    size_t agent_responses;
    size_t delegations;
    size_t convergences;
    uint64_t total_tokens;
    double total_cost;
} MessageStats;

MessageStats message_get_stats(void) {
    MessageStats stats = {0};

    Orchestrator* orch = orchestrator_get();
    if (!orch) return stats;

    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    Message* current = orch->message_history;
    while (current) {
        stats.total_messages++;
        stats.total_tokens += current->tokens_used.input_tokens + current->tokens_used.output_tokens;
        stats.total_cost += current->tokens_used.estimated_cost;

        switch (current->type) {
            case MSG_TYPE_USER_INPUT:
                stats.user_messages++;
                break;
            case MSG_TYPE_AGENT_RESPONSE:
                stats.agent_responses++;
                break;
            case MSG_TYPE_TASK_DELEGATE:
                stats.delegations++;
                break;
            case MSG_TYPE_CONVERGENCE:
                stats.convergences++;
                break;
            default:
                break;
        }

        current = current->next;
    }

    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);

    return stats;
}

// ============================================================================
// DEBUG
// ============================================================================

void message_print(Message* msg) {
    if (!msg) return;

    const char* type_str;
    switch (msg->type) {
        case MSG_TYPE_USER_INPUT: type_str = "USER"; break;
        case MSG_TYPE_AGENT_THOUGHT: type_str = "THOUGHT"; break;
        case MSG_TYPE_AGENT_ACTION: type_str = "ACTION"; break;
        case MSG_TYPE_AGENT_RESPONSE: type_str = "RESPONSE"; break;
        case MSG_TYPE_TASK_DELEGATE: type_str = "DELEGATE"; break;
        case MSG_TYPE_TASK_REPORT: type_str = "REPORT"; break;
        case MSG_TYPE_CONVERGENCE: type_str = "CONVERGE"; break;
        case MSG_TYPE_ERROR: type_str = "ERROR"; break;
        default: type_str = "UNKNOWN"; break;
    }

    printf("[%s] %llu -> %llu: %.50s%s\n",
        type_str,
        (unsigned long long)msg->sender,
        (unsigned long long)msg->recipient,
        msg->content ? msg->content : "(null)",
        msg->content && strlen(msg->content) > 50 ? "..." : "");
}

// ============================================================================
// PROVIDER-AWARE MESSAGING
// ============================================================================

typedef struct {
    uint8_t provider_type;   // ProviderType enum value
    char* model_id;          // Model used for this message
    uint64_t latency_ms;     // Response latency
    bool cache_hit;          // Whether response was from cache
} MessageProviderInfo;

// Extended message with provider tracking
typedef struct ExtendedMessage {
    Message base;
    MessageProviderInfo provider_info;
    uint8_t priority;        // 0 = low, 255 = high
    bool requires_ack;       // Needs acknowledgment
    bool acknowledged;
} ExtendedMessage;

// Provider statistics per message type
typedef struct {
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t total_latency_ms;
    uint64_t cache_hits;
    uint64_t errors;
} ProviderMessageStats;

static ProviderMessageStats g_provider_stats[4] = {0};  // One per provider

// Record provider statistics for a message
void msgbus_record_provider_stat(uint8_t provider_type, uint64_t latency_ms,
                                  bool is_cache_hit, bool is_error) {
    if (provider_type >= 4) return;

    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    ProviderMessageStats* stats = &g_provider_stats[provider_type];
    stats->messages_received++;
    stats->total_latency_ms += latency_ms;
    if (is_cache_hit) stats->cache_hits++;
    if (is_error) stats->errors++;

    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
}

// Get provider statistics
char* msgbus_provider_stats_json(void) {
    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    char* json = malloc(2048);
    if (!json) {
        CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
        return NULL;
    }

    const char* provider_names[] = {"anthropic", "openai", "gemini", "ollama"};
    size_t offset = (size_t)snprintf(json, 2048, "{\"providers\":{");

    for (int i = 0; i < 4; i++) {
        ProviderMessageStats* stats = &g_provider_stats[i];
        double avg_latency = stats->messages_received > 0
            ? (double)stats->total_latency_ms / stats->messages_received
            : 0.0;

        offset += (size_t)snprintf(json + offset, 2048 - offset,
            "%s\"%s\":{\"sent\":%llu,\"received\":%llu,\"avg_latency_ms\":%.2f,"
            "\"cache_hits\":%llu,\"errors\":%llu}",
            i > 0 ? "," : "",
            provider_names[i],
            (unsigned long long)stats->messages_sent,
            (unsigned long long)stats->messages_received,
            avg_latency,
            (unsigned long long)stats->cache_hits,
            (unsigned long long)stats->errors);
    }

    snprintf(json + offset, 2048 - offset, "}}");

    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
    return json;
}

// ============================================================================
// PRIORITY MESSAGE QUEUE
// ============================================================================

typedef struct PriorityQueueNode {
    Message* msg;
    uint8_t priority;
    struct PriorityQueueNode* next;
} PriorityQueueNode;

static PriorityQueueNode* g_priority_queue = NULL;

// Enqueue message with priority
void msgbus_enqueue_priority(Message* msg, uint8_t priority) {
    if (!msg) return;

    PriorityQueueNode* node = malloc(sizeof(PriorityQueueNode));
    if (!node) {
        message_destroy(msg);
        return;
    }

    node->msg = msg;
    node->priority = priority;
    node->next = NULL;

    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    // Insert in priority order (higher priority first)
    if (!g_priority_queue || priority > g_priority_queue->priority) {
        node->next = g_priority_queue;
        g_priority_queue = node;
    } else {
        PriorityQueueNode* current = g_priority_queue;
        while (current->next && current->next->priority >= priority) {
            current = current->next;
        }
        node->next = current->next;
        current->next = node;
    }

    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
}

// Dequeue highest priority message
Message* msgbus_dequeue_priority(void) {
    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    if (!g_priority_queue) {
        CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
        return NULL;
    }

    PriorityQueueNode* node = g_priority_queue;
    g_priority_queue = node->next;

    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);

    Message* msg = node->msg;
    free(node);
    return msg;
}

// Get queue depth
size_t msgbus_queue_depth(void) {
    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    size_t count = 0;
    PriorityQueueNode* current = g_priority_queue;
    while (current) {
        count++;
        current = current->next;
    }

    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
    return count;
}

// ============================================================================
// TOPIC-BASED SUBSCRIPTION
// ============================================================================

typedef struct Subscription {
    char* topic;
    SemanticID subscriber_id;
    void (*callback)(Message*, void*);
    void* ctx;
    struct Subscription* next;
} Subscription;

static Subscription* g_subscriptions = NULL;

// Subscribe to a topic
void msgbus_subscribe(const char* topic, SemanticID subscriber_id,
                      void (*callback)(Message*, void*), void* ctx) {
    if (!topic || !callback) return;

    Subscription* sub = malloc(sizeof(Subscription));
    if (!sub) return;

    sub->topic = strdup(topic);
    sub->subscriber_id = subscriber_id;
    sub->callback = callback;
    sub->ctx = ctx;

    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);
    sub->next = g_subscriptions;
    g_subscriptions = sub;
    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
}

// Unsubscribe from a topic
void msgbus_unsubscribe(const char* topic, SemanticID subscriber_id) {
    if (!topic) return;

    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    Subscription** current = &g_subscriptions;
    while (*current) {
        if ((*current)->subscriber_id == subscriber_id &&
            strcmp((*current)->topic, topic) == 0) {
            Subscription* to_remove = *current;
            *current = to_remove->next;
            free(to_remove->topic);
            free(to_remove);
        } else {
            current = &(*current)->next;
        }
    }

    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
}

// Publish message to topic subscribers
void msgbus_publish(const char* topic, Message* msg) {
    if (!topic || !msg) return;

    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    Subscription* current = g_subscriptions;
    while (current) {
        if (strcmp(current->topic, topic) == 0) {
            // Copy message for each subscriber
            Message* copy = message_create(msg->type, msg->sender,
                                           current->subscriber_id, msg->content);
            if (copy) {
                copy->metadata_json = msg->metadata_json ? strdup(msg->metadata_json) : NULL;
                copy->parent_id = msg->id;

                // Invoke callback outside lock to prevent deadlock
                void (*cb)(Message*, void*) = current->callback;
                void* ctx = current->ctx;
                CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);

                cb(copy, ctx);

                CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);
            }
        }
        current = current->next;
    }

    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
}

// ============================================================================
// MESSAGE FILTERING
// ============================================================================

typedef struct {
    MessageType* allowed_types;
    size_t type_count;
    SemanticID* allowed_senders;
    size_t sender_count;
    uint8_t min_priority;
} MessageFilter;

// Apply filter to message list
Message** msgbus_filter(Message** messages, size_t count, MessageFilter* filter, size_t* out_count) {
    if (!messages || !filter || !out_count) return NULL;

    Message** filtered = malloc(sizeof(Message*) * count);
    if (!filtered) return NULL;

    size_t n = 0;
    for (size_t i = 0; i < count; i++) {
        Message* msg = messages[i];
        if (!msg) continue;

        // Check type filter
        bool type_ok = (filter->type_count == 0);
        for (size_t j = 0; j < filter->type_count; j++) {
            if (msg->type == filter->allowed_types[j]) {
                type_ok = true;
                break;
            }
        }
        if (!type_ok) continue;

        // Check sender filter
        bool sender_ok = (filter->sender_count == 0);
        for (size_t j = 0; j < filter->sender_count; j++) {
            if (msg->sender == filter->allowed_senders[j]) {
                sender_ok = true;
                break;
            }
        }
        if (!sender_ok) continue;

        filtered[n++] = msg;
    }

    *out_count = n;
    return filtered;
}

// ============================================================================
// AGENT MODEL ROUTING
// ============================================================================

// Route message to appropriate provider based on agent's configured model
typedef struct {
    SemanticID agent_id;
    uint8_t preferred_provider;
    char* preferred_model;
    uint8_t fallback_provider;
    char* fallback_model;
} AgentProviderRoute;

static AgentProviderRoute* g_agent_routes = NULL;
static size_t g_route_count = 0;
static size_t g_route_capacity = 0;

// Register agent's provider preference
void msgbus_register_agent_route(SemanticID agent_id, uint8_t provider,
                                  const char* model, uint8_t fallback_provider,
                                  const char* fallback_model) {
    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    // Check for existing route
    for (size_t i = 0; i < g_route_count; i++) {
        if (g_agent_routes[i].agent_id == agent_id) {
            // Update existing
            free(g_agent_routes[i].preferred_model);
            free(g_agent_routes[i].fallback_model);
            g_agent_routes[i].preferred_provider = provider;
            g_agent_routes[i].preferred_model = model ? strdup(model) : NULL;
            g_agent_routes[i].fallback_provider = fallback_provider;
            g_agent_routes[i].fallback_model = fallback_model ? strdup(fallback_model) : NULL;
            CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
            return;
        }
    }

    // Add new route
    if (g_route_count >= g_route_capacity) {
        size_t new_cap = g_route_capacity == 0 ? 8 : g_route_capacity * 2;
        AgentProviderRoute* new_routes = realloc(g_agent_routes, sizeof(AgentProviderRoute) * new_cap);
        if (!new_routes) {
            CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
            return;
        }
        g_agent_routes = new_routes;
        g_route_capacity = new_cap;
    }

    g_agent_routes[g_route_count].agent_id = agent_id;
    g_agent_routes[g_route_count].preferred_provider = provider;
    g_agent_routes[g_route_count].preferred_model = model ? strdup(model) : NULL;
    g_agent_routes[g_route_count].fallback_provider = fallback_provider;
    g_agent_routes[g_route_count].fallback_model = fallback_model ? strdup(fallback_model) : NULL;
    g_route_count++;

    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
}

// Get provider for agent
bool msgbus_get_agent_provider(SemanticID agent_id, uint8_t* out_provider, char** out_model) {
    CONVERGIO_MUTEX_LOCK(&g_msgbus_mutex);

    for (size_t i = 0; i < g_route_count; i++) {
        if (g_agent_routes[i].agent_id == agent_id) {
            if (out_provider) *out_provider = g_agent_routes[i].preferred_provider;
            if (out_model) *out_model = g_agent_routes[i].preferred_model;
            CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
            return true;
        }
    }

    CONVERGIO_MUTEX_UNLOCK(&g_msgbus_mutex);
    return false;
}
