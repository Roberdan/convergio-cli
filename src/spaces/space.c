/**
 * NOUS Collaborative Spaces
 *
 * Shared environments where humans and AI agents work together
 * Each space has its own semantic context and rhythm
 */

#include "nous/nous.h"
#include <dispatch/dispatch.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// SPACE REGISTRY
// ============================================================================

#define MAX_SPACES 128
#define MAX_PARTICIPANTS_PER_SPACE 64

typedef struct {
    NousSpace* spaces[MAX_SPACES];
    size_t count;
    os_unfair_lock lock;
    dispatch_queue_t sync_queue;
} SpaceRegistry;

static SpaceRegistry g_spaces = {.count = 0, .lock = OS_UNFAIR_LOCK_INIT, .sync_queue = NULL};

// ============================================================================
// RHYTHM SYSTEM
// ============================================================================

typedef struct {
    float base_urgency;
    uint64_t last_interaction_ns;
    uint64_t deadline_ns;  // 0 = no deadline
    float attention_level; // How focused participants are [0,1]
} SpaceRhythm;

static void update_rhythm(NousSpace* space) {
    if (!space)
        return;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;

    // Calculate time since last interaction
    uint64_t idle_ns = now_ns - space->last_activity;
    float idle_seconds = (float)idle_ns / 1e9f;

    // Urgency decays with idle time, but deadline proximity increases it
    float base = space->urgency_level;

    // Decay: urgency reduces over time without activity
    float decay = 1.0f / (1.0f + idle_seconds / 60.0f); // Half-life ~1 minute

    space->urgency_level = base * decay;

    // Clamp
    if (space->urgency_level < 0.1f)
        space->urgency_level = 0.1f;
    if (space->urgency_level > 1.0f)
        space->urgency_level = 1.0f;
}

// ============================================================================
// SPACE LIFECYCLE
// ============================================================================

static void initialize_space_registry(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        dispatch_queue_attr_t attr = dispatch_queue_attr_make_with_qos_class(
            DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, 0);
        g_spaces.sync_queue = dispatch_queue_create("nous.spaces.sync", attr);
    });
}

NousSpace* nous_create_space(const char* name, const char* purpose) {
    if (!name || !purpose)
        return NULL;

    initialize_space_registry();

    NousSpace* space = calloc(1, sizeof(NousSpace));
    if (!space)
        return NULL;

    // Generate semantic identity
    space->id = nous_create_node(SEMANTIC_TYPE_SPACE, purpose);
    if (space->id == SEMANTIC_ID_NULL) {
        free(space);
        return NULL;
    }

    // Copy name and purpose
    space->name = strdup(name);
    space->purpose = strdup(purpose);
    if (!space->name || !space->purpose) {
        free(space->name);
        free(space->purpose);
        free(space);
        return NULL;
    }

    // Initialize participant arrays
    space->agents = calloc(MAX_PARTICIPANTS_PER_SPACE, sizeof(SemanticID));
    space->humans = calloc(MAX_PARTICIPANTS_PER_SPACE, sizeof(SemanticID));
    if (!space->agents || !space->humans) {
        free(space->name);
        free(space->purpose);
        free(space->agents);
        free(space->humans);
        free(space);
        return NULL;
    }

    // Initialize rhythm
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    space->last_activity = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    space->urgency_level = 0.5f; // Default: medium urgency

    // Defaults
    space->allow_external_agents = false;
    space->persistent = true;

    // Register space
    os_unfair_lock_lock(&g_spaces.lock);
    if (g_spaces.count < MAX_SPACES) {
        g_spaces.spaces[g_spaces.count++] = space;
    }
    os_unfair_lock_unlock(&g_spaces.lock);

    return space;
}

void nous_destroy_space(NousSpace* space) {
    if (!space)
        return;

    // Unregister
    os_unfair_lock_lock(&g_spaces.lock);
    for (size_t i = 0; i < g_spaces.count; i++) {
        if (g_spaces.spaces[i] == space) {
            g_spaces.spaces[i] = g_spaces.spaces[--g_spaces.count];
            break;
        }
    }
    os_unfair_lock_unlock(&g_spaces.lock);

    // Cleanup
    free(space->name);
    free(space->purpose);
    free(space->agents);
    free(space->humans);
    free(space);
}

// ============================================================================
// PARTICIPATION
// ============================================================================

int nous_join_space(SemanticID entity, SemanticID space_id) {
    if (entity == SEMANTIC_ID_NULL || space_id == SEMANTIC_ID_NULL)
        return -1;

    // Find space
    NousSpace* space = NULL;
    os_unfair_lock_lock(&g_spaces.lock);
    for (size_t i = 0; i < g_spaces.count; i++) {
        if (g_spaces.spaces[i]->id == space_id) {
            space = g_spaces.spaces[i];
            break;
        }
    }
    os_unfair_lock_unlock(&g_spaces.lock);

    if (!space)
        return -1;

    // Determine entity type from SemanticID
    SemanticType type = (SemanticType)((entity & SEMANTIC_TYPE_MASK) >> SEMANTIC_TYPE_SHIFT);

    if (type == SEMANTIC_TYPE_AGENT) {
        // Check permission for external agents
        if (!space->allow_external_agents && space->agent_count > 0) {
            // Check if this agent was explicitly invited
            // For now, allow all agents
        }

        // Add to agents list
        if (space->agent_count < MAX_PARTICIPANTS_PER_SPACE) {
            // Check not already present
            for (size_t i = 0; i < space->agent_count; i++) {
                if (space->agents[i] == entity)
                    return 0; // Already joined
            }
            space->agents[space->agent_count++] = entity;
        }
    } else {
        // Treat as human
        if (space->human_count < MAX_PARTICIPANTS_PER_SPACE) {
            for (size_t i = 0; i < space->human_count; i++) {
                if (space->humans[i] == entity)
                    return 0;
            }
            space->humans[space->human_count++] = entity;
        }
    }

    // Create semantic connection
    nous_connect(entity, space_id, 0.9f);
    nous_connect(space_id, entity, 0.9f); // Bidirectional

    // Update activity
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    space->last_activity = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;

    return 0;
}

int nous_leave_space(SemanticID entity, SemanticID space_id) {
    if (entity == SEMANTIC_ID_NULL || space_id == SEMANTIC_ID_NULL)
        return -1;

    NousSpace* space = NULL;
    os_unfair_lock_lock(&g_spaces.lock);
    for (size_t i = 0; i < g_spaces.count; i++) {
        if (g_spaces.spaces[i]->id == space_id) {
            space = g_spaces.spaces[i];
            break;
        }
    }
    os_unfair_lock_unlock(&g_spaces.lock);

    if (!space)
        return -1;

    // Remove from appropriate list
    SemanticType type = (SemanticType)((entity & SEMANTIC_TYPE_MASK) >> SEMANTIC_TYPE_SHIFT);

    if (type == SEMANTIC_TYPE_AGENT) {
        for (size_t i = 0; i < space->agent_count; i++) {
            if (space->agents[i] == entity) {
                space->agents[i] = space->agents[--space->agent_count];
                break;
            }
        }
    } else {
        for (size_t i = 0; i < space->human_count; i++) {
            if (space->humans[i] == entity) {
                space->humans[i] = space->humans[--space->human_count];
                break;
            }
        }
    }

    // Note: We don't remove semantic connections (history is preserved)

    return 0;
}

// ============================================================================
// SPACE COMMUNICATION
// ============================================================================

typedef void (*SpaceMessageCallback)(SemanticID sender, const char* message, void* ctx);

typedef struct {
    SemanticID space_id;
    SpaceMessageCallback callbacks[MAX_PARTICIPANTS_PER_SPACE];
    void* callback_contexts[MAX_PARTICIPANTS_PER_SPACE];
    size_t callback_count;
} SpaceMessageBus;

// Forward declaration - implementation pending for message bus feature
__attribute__((unused)) static SpaceMessageBus* get_or_create_bus(SemanticID space_id);

int nous_space_broadcast(SemanticID space_id, SemanticID sender, const char* message) {
    if (space_id == SEMANTIC_ID_NULL || !message)
        return -1;

    // Find space
    NousSpace* space = NULL;
    os_unfair_lock_lock(&g_spaces.lock);
    for (size_t i = 0; i < g_spaces.count; i++) {
        if (g_spaces.spaces[i]->id == space_id) {
            space = g_spaces.spaces[i];
            break;
        }
    }
    os_unfair_lock_unlock(&g_spaces.lock);

    if (!space)
        return -1;

    // Create semantic node for message
    SemanticID msg_node = nous_create_node(SEMANTIC_TYPE_EVENT, message);
    if (msg_node != SEMANTIC_ID_NULL) {
        nous_connect(sender, msg_node, 1.0f);   // Sender -> message
        nous_connect(msg_node, space_id, 0.8f); // Message -> space
    }

    // Update rhythm (activity = higher urgency)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    space->last_activity = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    space->urgency_level = fminf(space->urgency_level + 0.1f, 1.0f);

    return 0;
}

// ============================================================================
// CONSENSUS BUILDING
// ============================================================================

typedef struct {
    SemanticID proposal;
    SemanticID* supporters;
    SemanticID* opposers;
    size_t support_count;
    size_t oppose_count;
    float consensus_threshold; // Required agreement level
} ConsensusProcess;

ConsensusProcess* nous_space_propose(NousSpace* space, const char* proposal_text) {
    if (!space || !proposal_text)
        return NULL;

    ConsensusProcess* process = calloc(1, sizeof(ConsensusProcess));
    if (!process)
        return NULL;

    // Create proposal as semantic node
    process->proposal = nous_create_node(SEMANTIC_TYPE_INTENT, proposal_text);
    if (process->proposal == SEMANTIC_ID_NULL) {
        free(process);
        return NULL;
    }

    // Connect to space
    nous_connect(process->proposal, space->id, 0.9f);

    // Allocate supporter/opposer arrays
    size_t max_voters = space->agent_count + space->human_count;
    process->supporters = calloc(max_voters, sizeof(SemanticID));
    process->opposers = calloc(max_voters, sizeof(SemanticID));

    if (!process->supporters || !process->opposers) {
        free(process->supporters);
        free(process->opposers);
        free(process);
        return NULL;
    }

    process->consensus_threshold = 0.7f; // 70% agreement by default

    return process;
}

int nous_consensus_vote(ConsensusProcess* process, SemanticID voter, bool support) {
    if (!process)
        return -1;

    if (support) {
        // Remove from opposers if present
        for (size_t i = 0; i < process->oppose_count; i++) {
            if (process->opposers[i] == voter) {
                process->opposers[i] = process->opposers[--process->oppose_count];
                break;
            }
        }
        // Add to supporters
        process->supporters[process->support_count++] = voter;

        // Create semantic connection
        nous_connect(voter, process->proposal, 0.9f);
    } else {
        // Remove from supporters if present
        for (size_t i = 0; i < process->support_count; i++) {
            if (process->supporters[i] == voter) {
                process->supporters[i] = process->supporters[--process->support_count];
                break;
            }
        }
        // Add to opposers
        process->opposers[process->oppose_count++] = voter;

        // Create weak/negative connection
        nous_connect(voter, process->proposal, 0.1f);
    }

    return 0;
}

bool nous_consensus_reached(ConsensusProcess* process) {
    if (!process)
        return false;

    size_t total = process->support_count + process->oppose_count;
    if (total == 0)
        return false;

    float support_ratio = (float)process->support_count / (float)total;
    return support_ratio >= process->consensus_threshold;
}

void nous_consensus_free(ConsensusProcess* process) {
    if (!process)
        return;
    free(process->supporters);
    free(process->opposers);
    free(process);
}

// ============================================================================
// SPACE QUERIES
// ============================================================================

NousSpace* nous_space_find_by_name(const char* name) {
    if (!name)
        return NULL;

    os_unfair_lock_lock(&g_spaces.lock);

    for (size_t i = 0; i < g_spaces.count; i++) {
        if (strcmp(g_spaces.spaces[i]->name, name) == 0) {
            NousSpace* space = g_spaces.spaces[i];
            os_unfair_lock_unlock(&g_spaces.lock);
            return space;
        }
    }

    os_unfair_lock_unlock(&g_spaces.lock);
    return NULL;
}

void nous_spaces_foreach(void (*fn)(NousSpace* space, void* ctx), void* ctx) {
    os_unfair_lock_lock(&g_spaces.lock);

    for (size_t i = 0; i < g_spaces.count; i++) {
        fn(g_spaces.spaces[i], ctx);
    }

    os_unfair_lock_unlock(&g_spaces.lock);
}

// ============================================================================
// SPACE RHYTHM QUERIES
// ============================================================================

float nous_space_urgency(NousSpace* space) {
    if (!space)
        return 0.0f;
    update_rhythm(space);
    return space->urgency_level;
}

void nous_space_set_urgency(NousSpace* space, float urgency) {
    if (!space)
        return;
    if (urgency < 0.0f)
        urgency = 0.0f;
    if (urgency > 1.0f)
        urgency = 1.0f;
    space->urgency_level = urgency;
}

size_t nous_space_participant_count(NousSpace* space) {
    if (!space)
        return 0;
    return space->agent_count + space->human_count;
}

bool nous_space_is_active(NousSpace* space) {
    if (!space)
        return false;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;

    // Active if last activity was within 5 minutes
    uint64_t idle_ns = now_ns - space->last_activity;
    return idle_ns < (5ULL * 60ULL * 1000000000ULL);
}
