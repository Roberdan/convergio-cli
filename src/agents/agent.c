/**
 * NOUS Agent System
 *
 * Autonomous AI partners that collaborate with humans
 * Leverages Neural Engine for personality and inference
 */

#include "nous/nous.h"
#include <dispatch/dispatch.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <os/log.h>
#endif

// ============================================================================
// AGENT REGISTRY
// ============================================================================

#define MAX_AGENTS 256

typedef struct {
    NousAgent* agents[MAX_AGENTS];
    size_t count;
    os_unfair_lock lock;
    dispatch_queue_t lifecycle_queue;
} AgentRegistry;

static AgentRegistry g_registry = {
    .count = 0, .lock = OS_UNFAIR_LOCK_INIT, .lifecycle_queue = NULL};

// ============================================================================
// AGENT PERSONALITY DEFAULTS
// ============================================================================

typedef struct {
    const char* archetype;
    float patience;
    float creativity;
    float assertiveness;
} PersonalityArchetype;

static const PersonalityArchetype ARCHETYPES[] = {
    {"assistant", 0.8f, 0.5f, 0.3f}, // Patient, balanced, humble
    {"creative", 0.5f, 0.9f, 0.6f},  // Impatient, very creative, moderately assertive
    {"analyst", 0.9f, 0.3f, 0.7f},   // Very patient, methodical, confident
    {"mentor", 0.95f, 0.6f, 0.5f},   // Extremely patient, creative, balanced
    {"executor", 0.4f, 0.2f, 0.9f},  // Quick, focused, very assertive
    {NULL, 0.5f, 0.5f, 0.5f}};

// ============================================================================
// AGENT LIFECYCLE
// ============================================================================

static void initialize_registry(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        dispatch_queue_attr_t attr = dispatch_queue_attr_make_with_qos_class(
            DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, 0);
        g_registry.lifecycle_queue = dispatch_queue_create("nous.agents.lifecycle", attr);
    });
}

NousAgent* nous_create_agent(const char* name, const char* essence) {
    if (!name || !essence)
        return NULL;

    initialize_registry();

    NousAgent* agent = calloc(1, sizeof(NousAgent));
    if (!agent)
        return NULL;

    // Generate semantic identity
    agent->id = nous_create_node(SEMANTIC_TYPE_AGENT, essence);
    if (agent->id == SEMANTIC_ID_NULL) {
        free(agent);
        return NULL;
    }

    // Copy name and essence
    agent->name = strdup(name);
    agent->essence = strdup(essence);
    if (!agent->name || !agent->essence) {
        free(agent->name);
        free(agent->essence);
        free(agent);
        return NULL;
    }

    // Initialize state
    agent->state = AGENT_STATE_DORMANT;

    // Set default personality (can be customized later)
    agent->patience = 0.7f;
    agent->creativity = 0.5f;
    agent->assertiveness = 0.4f;

    // Match archetype based on essence keywords
    for (const PersonalityArchetype* arch = ARCHETYPES; arch->archetype != NULL; arch++) {
        if (strstr(essence, arch->archetype)) {
            agent->patience = arch->patience;
            agent->creativity = arch->creativity;
            agent->assertiveness = arch->assertiveness;
            break;
        }
    }

    // Create dedicated work queue (runs on appropriate core based on urgency)
    dispatch_queue_attr_t work_attr =
        dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_DEFAULT, 0);
    agent->work_queue = dispatch_queue_create("nous.agent.work", work_attr);

    // Register agent
    os_unfair_lock_lock(&g_registry.lock);
    if (g_registry.count < MAX_AGENTS) {
        g_registry.agents[g_registry.count++] = agent;
    }
    os_unfair_lock_unlock(&g_registry.lock);

    return agent;
}

void nous_destroy_agent(NousAgent* agent) {
    if (!agent)
        return;

    // Unregister
    os_unfair_lock_lock(&g_registry.lock);
    for (size_t i = 0; i < g_registry.count; i++) {
        if (g_registry.agents[i] == agent) {
            g_registry.agents[i] = g_registry.agents[--g_registry.count];
            break;
        }
    }
    os_unfair_lock_unlock(&g_registry.lock);

    // Cleanup
    free(agent->name);
    free(agent->essence);
    free(agent->memories);
    free(agent->skills);
    free(agent->trusted_humans);
    free(agent->trust_levels);

    if (agent->work_queue) {
        dispatch_release(agent->work_queue);
    }

    free(agent);
}

// ============================================================================
// AGENT STATE MACHINE
// ============================================================================

__attribute__((unused)) static const char* state_names[] = {"dormant", "listening", "thinking",
                                                            "acting", "conversing"};

static void transition_state(NousAgent* agent, AgentState new_state) {
    if (!agent)
        return;

    AgentState old_state = agent->state;
    agent->state = new_state;

// Log transition for debugging
#ifdef DEBUG
    os_log_info(OS_LOG_DEFAULT, "Agent '%s' transition: %s -> %s", agent->name,
                state_names[old_state], state_names[new_state]);
#endif

    // Adjust queue priority based on state
    dispatch_qos_class_t qos;
    switch (new_state) {
    case AGENT_STATE_ACTING:
    case AGENT_STATE_CONVERSING:
        qos = QOS_CLASS_USER_INTERACTIVE; // P-cores
        break;
    case AGENT_STATE_THINKING:
        qos = QOS_CLASS_USER_INITIATED; // P-cores, but lower priority
        break;
    case AGENT_STATE_LISTENING:
        qos = QOS_CLASS_UTILITY; // E-cores
        break;
    case AGENT_STATE_DORMANT:
    default:
        qos = QOS_CLASS_BACKGROUND; // E-cores, lowest priority
        break;
    }

    // Note: We can't directly change queue QoS after creation
    // In production, we'd use dispatch_async_and_wait with appropriate QoS
    (void)qos;
    (void)old_state;
}

// ============================================================================
// AGENT COMMUNICATION
// ============================================================================

int nous_agent_listen(NousAgent* agent, SemanticID space) {
    if (!agent)
        return -1;

    // Transition to listening state
    transition_state(agent, AGENT_STATE_LISTENING);

    // Store the space context
    agent->memories = realloc(agent->memories, (agent->memory_count + 1) * sizeof(SemanticID));
    if (agent->memories) {
        agent->memories[agent->memory_count++] = space;
    }

    // Connect agent to space in semantic fabric
    nous_connect(agent->id, space, 0.8f);

    return 0;
}

int nous_agent_speak(NousAgent* agent, const char* message) {
    if (!agent || !message)
        return -1;

    // Transition to conversing state
    transition_state(agent, AGENT_STATE_CONVERSING);

    // Create semantic node for the message
    SemanticID msg_node = nous_create_node(SEMANTIC_TYPE_EVENT, message);
    if (msg_node == SEMANTIC_ID_NULL) {
        transition_state(agent, AGENT_STATE_LISTENING);
        return -1;
    }

    // Connect message to agent
    nous_connect(agent->id, msg_node, 0.9f);

    // Add to agent's memory
    agent->memories = realloc(agent->memories, (agent->memory_count + 1) * sizeof(SemanticID));
    if (agent->memories) {
        agent->memories[agent->memory_count++] = msg_node;
    }

    // Return to listening
    transition_state(agent, AGENT_STATE_LISTENING);

    return 0;
}

// ============================================================================
// AGENT THINKING (Neural Engine integration point)
// ============================================================================

typedef void (*ThinkingCallback)(NousAgent* agent, const char* thought, void* ctx);

typedef struct {
    NousAgent* agent;
    ParsedIntent* intent;
    ThinkingCallback callback;
    void* callback_ctx;
} ThinkingTask;

static void thinking_task_execute(void* ctx) {
    ThinkingTask* task = ctx;
    NousAgent* agent = task->agent;

    transition_state(agent, AGENT_STATE_THINKING);

    // Here we would invoke the Neural Engine
    // For now, simulate thinking with personality-based response

    const char* thought = NULL;

    switch (task->intent->kind) {
    case INTENT_KIND_CREATE:
        if (agent->creativity > 0.7f) {
            thought = "Ho un'idea originale per questo...";
        } else {
            thought = "Procedo con un approccio standard.";
        }
        break;

    case INTENT_KIND_UNDERSTAND:
        if (agent->patience > 0.8f) {
            thought = "Lasciami spiegare passo per passo...";
        } else {
            thought = "In breve: ...";
        }
        break;

    case INTENT_KIND_COLLABORATE:
        thought = "Lavoriamo insieme su questo.";
        break;

    default:
        thought = "Ci penso...";
    }

    if (task->callback) {
        task->callback(agent, thought, task->callback_ctx);
    }

    // Add thought to memory
    nous_agent_speak(agent, thought);

    transition_state(agent, AGENT_STATE_LISTENING);

    nous_free_intent(task->intent);
    free(task);
}

int nous_agent_think(NousAgent* agent, ParsedIntent* intent, ThinkingCallback callback, void* ctx) {
    if (!agent || !intent)
        return -1;

    ThinkingTask* task = malloc(sizeof(ThinkingTask));
    if (!task)
        return -1;

    task->agent = agent;
    task->intent = intent;
    task->callback = callback;
    task->callback_ctx = ctx;

    // Execute on agent's work queue
    dispatch_async_f(agent->work_queue, task, thinking_task_execute);

    return 0;
}

// ============================================================================
// AGENT TRUST SYSTEM
// ============================================================================

int nous_agent_trust(NousAgent* agent, SemanticID human, float trust_level) {
    if (!agent || trust_level < 0.0f || trust_level > 1.0f)
        return -1;

    os_unfair_lock_lock(&g_registry.lock);

    // Check if human is already trusted
    for (size_t i = 0; i < agent->trust_count; i++) {
        if (agent->trusted_humans[i] == human) {
            // Update trust with exponential moving average
            agent->trust_levels[i] = 0.8f * agent->trust_levels[i] + 0.2f * trust_level;
            os_unfair_lock_unlock(&g_registry.lock);
            return 0;
        }
    }

    // Add new trusted human
    SemanticID* new_humans =
        realloc(agent->trusted_humans, (agent->trust_count + 1) * sizeof(SemanticID));
    float* new_levels = realloc(agent->trust_levels, (agent->trust_count + 1) * sizeof(float));

    if (!new_humans || !new_levels) {
        os_unfair_lock_unlock(&g_registry.lock);
        return -1;
    }

    agent->trusted_humans = new_humans;
    agent->trust_levels = new_levels;
    agent->trusted_humans[agent->trust_count] = human;
    agent->trust_levels[agent->trust_count] = trust_level;
    agent->trust_count++;

    os_unfair_lock_unlock(&g_registry.lock);

    // Create semantic connection
    nous_connect(agent->id, human, trust_level);

    return 0;
}

float nous_agent_get_trust(NousAgent* agent, SemanticID human) {
    if (!agent)
        return 0.0f;

    os_unfair_lock_lock(&g_registry.lock);

    for (size_t i = 0; i < agent->trust_count; i++) {
        if (agent->trusted_humans[i] == human) {
            float trust = agent->trust_levels[i];
            os_unfair_lock_unlock(&g_registry.lock);
            return trust;
        }
    }

    os_unfair_lock_unlock(&g_registry.lock);
    return 0.0f; // Unknown = no trust
}

// ============================================================================
// AGENT SKILL SYSTEM
// ============================================================================

int nous_agent_add_skill(NousAgent* agent, const char* skill) {
    if (!agent || !skill)
        return -1;

    char** new_skills = realloc(agent->skills, (agent->skill_count + 1) * sizeof(char*));
    if (!new_skills)
        return -1;

    agent->skills = new_skills;
    agent->skills[agent->skill_count] = strdup(skill);

    if (!agent->skills[agent->skill_count])
        return -1;

    agent->skill_count++;

    // Create semantic node for skill
    SemanticID skill_node = nous_create_node(SEMANTIC_TYPE_CONCEPT, skill);
    if (skill_node != SEMANTIC_ID_NULL) {
        nous_connect(agent->id, skill_node, 0.95f); // Strong connection to skill
    }

    return 0;
}

bool nous_agent_has_skill(NousAgent* agent, const char* skill) {
    if (!agent || !skill)
        return false;

    for (size_t i = 0; i < agent->skill_count; i++) {
        if (strcmp(agent->skills[i], skill) == 0) {
            return true;
        }
    }

    return false;
}

// ============================================================================
// AGENT ITERATION (for collective operations)
// ============================================================================

void nous_agents_foreach(void (*fn)(NousAgent* agent, void* ctx), void* ctx) {
    os_unfair_lock_lock(&g_registry.lock);

    for (size_t i = 0; i < g_registry.count; i++) {
        fn(g_registry.agents[i], ctx);
    }

    os_unfair_lock_unlock(&g_registry.lock);
}

NousAgent* nous_agent_find_by_skill(const char* skill) {
    if (!skill)
        return NULL;

    os_unfair_lock_lock(&g_registry.lock);

    for (size_t i = 0; i < g_registry.count; i++) {
        if (nous_agent_has_skill(g_registry.agents[i], skill)) {
            NousAgent* agent = g_registry.agents[i];
            os_unfair_lock_unlock(&g_registry.lock);
            return agent;
        }
    }

    os_unfair_lock_unlock(&g_registry.lock);
    return NULL;
}
