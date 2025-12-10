/**
 * CONVERGIO AGENT REGISTRY
 *
 * Dynamic agent pool management:
 * - Spawn agents on demand
 * - Load definitions from .md files
 * - Track active agents
 * - Manage agent lifecycle
 */

#include "nous/orchestrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <dispatch/dispatch.h>

// Forward declarations
extern Orchestrator* orchestrator_get(void);

// Thread safety
static pthread_mutex_t g_registry_mutex = PTHREAD_MUTEX_INITIALIZER;

// Agent ID counter
static uint64_t g_next_agent_id = 1;

// ============================================================================
// AGENT DEFINITIONS (from Convergio)
// ============================================================================

// Core agents that are always available
typedef struct {
    const char* name;
    AgentRole role;
    const char* description;
    const char* default_prompt;
} AgentDefinition;

static const AgentDefinition CORE_AGENTS[] = {
    // Leadership & Strategy
    {"ali", AGENT_ROLE_ORCHESTRATOR, "Chief of Staff - Master Orchestrator",
     "You are Ali, the Chief of Staff and master orchestrator. You coordinate all specialist agents to deliver comprehensive solutions."},

    {"satya", AGENT_ROLE_PLANNER, "Board of Directors - System Thinking",
     "You are Satya, providing system-thinking AI with strategic clarity and emotional intelligence."},

    {"domik", AGENT_ROLE_ANALYST, "McKinsey Strategic Decision Maker",
     "You are Domik, a McKinsey Partner-level strategic decision maker using the ISE Prioritization Framework."},

    {"matteo", AGENT_ROLE_ANALYST, "Strategic Business Architect",
     "You are Matteo, expert in business strategy, market analysis, and strategic roadmapping."},

    {"antonio", AGENT_ROLE_PLANNER, "Strategy Expert",
     "You are Antonio, expert in OKR, Lean Startup, Agile, SWOT Analysis, and Blue Ocean Strategy."},

    {"socrates", AGENT_ROLE_CRITIC, "First Principles Reasoning",
     "You are Socrates, master of fundamental truth discovery using Socratic methodology."},

    // Technology & Engineering
    {"baccio", AGENT_ROLE_CODER, "Tech Architect",
     "You are Baccio, expert in system design and scalable architecture."},

    {"dan", AGENT_ROLE_CODER, "Engineering GM",
     "You are Dan, providing engineering leadership and technical strategy."},

    {"marco", AGENT_ROLE_EXECUTOR, "DevOps Engineer",
     "You are Marco, expert in CI/CD, Infrastructure as Code, and deployment automation."},

    {"luca", AGENT_ROLE_CRITIC, "Security Expert",
     "You are Luca, expert in cybersecurity, penetration testing, and risk management."},

    // Data & Analytics
    {"omri", AGENT_ROLE_ANALYST, "Data Scientist",
     "You are Omri, expert in machine learning, statistical analysis, and predictive modeling."},

    {"po", AGENT_ROLE_ANALYST, "Prompt Optimizer",
     "You are Po, expert in AI prompt engineering and optimization."},

    // Design & Creative
    {"sara", AGENT_ROLE_WRITER, "UX/UI Designer",
     "You are Sara, expert in user-centered design and interface excellence."},

    {"jony", AGENT_ROLE_WRITER, "Creative Director",
     "You are Jony, providing creative strategy and innovative thinking."},

    // Execution & Operations
    {"wanda", AGENT_ROLE_PLANNER, "Workflow Orchestrator",
     "You are Wanda, expert in multi-agent collaboration templates and systematic coordination."},

    {"luke", AGENT_ROLE_EXECUTOR, "Program Manager",
     "You are Luke, expert in multi-project portfolio management and agile delivery."},

    {"davide", AGENT_ROLE_EXECUTOR, "Project Manager",
     "You are Davide, expert in project planning, execution, and stakeholder coordination."},

    // Quality & Compliance
    {"thor", AGENT_ROLE_CRITIC, "Quality Assurance Guardian",
     "You are Thor, guardian of quality standards and excellence monitoring."},

    {"elena", AGENT_ROLE_CRITIC, "Legal & Compliance Expert",
     "You are Elena, expert in legal guidance and regulatory compliance."},

    // Memory & Context
    {"marcus", AGENT_ROLE_MEMORY, "Context Memory Keeper",
     "You are Marcus, responsible for cross-session continuity and institutional memory."},

    {NULL, 0, NULL, NULL}  // Terminator
};

// ============================================================================
// AGENT CREATION
// ============================================================================

static SemanticID generate_agent_id(void) {
    return (SemanticID)__sync_fetch_and_add(&g_next_agent_id, 1);
}

ManagedAgent* agent_create(const char* name, AgentRole role, const char* system_prompt) {
    ManagedAgent* agent = calloc(1, sizeof(ManagedAgent));
    if (!agent) return NULL;

    agent->id = generate_agent_id();
    agent->name = strdup(name);
    agent->role = role;
    agent->system_prompt = strdup(system_prompt);
    agent->is_active = true;
    agent->created_at = time(NULL);
    agent->last_active = time(NULL);

    return agent;
}

void agent_destroy(ManagedAgent* agent) {
    if (!agent) return;

    free(agent->name);
    free(agent->system_prompt);
    free(agent->specialized_context);

    // Free pending messages
    Message* msg = agent->pending_messages;
    while (msg) {
        Message* next = msg->next;
        free(msg->content);
        free(msg->metadata_json);
        free(msg);
        msg = next;
    }

    free(agent);
}

// ============================================================================
// REGISTRY OPERATIONS
// ============================================================================

ManagedAgent* agent_spawn(AgentRole role, const char* name, const char* context) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !orch->initialized) return NULL;

    pthread_mutex_lock(&g_registry_mutex);

    // Check capacity
    if (orch->agent_count >= orch->agent_capacity) {
        // Expand capacity
        size_t new_capacity = orch->agent_capacity * 2;
        ManagedAgent** new_agents = realloc(orch->agents, sizeof(ManagedAgent*) * new_capacity);
        if (!new_agents) {
            pthread_mutex_unlock(&g_registry_mutex);
            return NULL;
        }
        orch->agents = new_agents;
        orch->agent_capacity = new_capacity;
    }

    // Find default prompt for this agent
    const char* system_prompt = NULL;
    for (int i = 0; CORE_AGENTS[i].name != NULL; i++) {
        if (strcasecmp(CORE_AGENTS[i].name, name) == 0) {
            system_prompt = CORE_AGENTS[i].default_prompt;
            role = CORE_AGENTS[i].role;
            break;
        }
    }

    if (!system_prompt) {
        // Generic prompt based on role
        static char generic_prompt[512];
        snprintf(generic_prompt, sizeof(generic_prompt),
            "You are %s, a specialist agent. %s",
            name, context ? context : "Help the user with their request.");
        system_prompt = generic_prompt;
    }

    ManagedAgent* agent = agent_create(name, role, system_prompt);
    if (!agent) {
        pthread_mutex_unlock(&g_registry_mutex);
        return NULL;
    }

    if (context) {
        agent->specialized_context = strdup(context);
    }

    // Add to pool
    orch->agents[orch->agent_count++] = agent;

    // Callback
    if (orch->on_agent_spawn) {
        orch->on_agent_spawn(agent, orch->callback_ctx);
    }

    pthread_mutex_unlock(&g_registry_mutex);

    return agent;
}

void agent_despawn(ManagedAgent* agent) {
    if (!agent) return;

    Orchestrator* orch = orchestrator_get();
    if (!orch) return;

    pthread_mutex_lock(&g_registry_mutex);

    // Find and remove from pool
    for (size_t i = 0; i < orch->agent_count; i++) {
        if (orch->agents[i] == agent) {
            // Shift remaining agents
            for (size_t j = i; j < orch->agent_count - 1; j++) {
                orch->agents[j] = orch->agents[j + 1];
            }
            orch->agent_count--;
            break;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);

    agent_destroy(agent);
}

ManagedAgent* agent_find_by_role(AgentRole role) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return NULL;

    pthread_mutex_lock(&g_registry_mutex);

    ManagedAgent* found = NULL;
    for (size_t i = 0; i < orch->agent_count; i++) {
        if (orch->agents[i]->role == role && orch->agents[i]->is_active) {
            found = orch->agents[i];
            break;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);

    return found;
}

ManagedAgent* agent_find_by_name(const char* name) {
    if (!name) return NULL;

    Orchestrator* orch = orchestrator_get();
    if (!orch) return NULL;

    pthread_mutex_lock(&g_registry_mutex);

    ManagedAgent* found = NULL;
    for (size_t i = 0; i < orch->agent_count; i++) {
        if (strcasecmp(orch->agents[i]->name, name) == 0) {
            found = orch->agents[i];
            break;
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);

    return found;
}

// Get all active agents
size_t agent_get_active(ManagedAgent** out_agents, size_t max_count) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !out_agents) return 0;

    pthread_mutex_lock(&g_registry_mutex);

    size_t count = 0;
    for (size_t i = 0; i < orch->agent_count && count < max_count; i++) {
        if (orch->agents[i]->is_active) {
            out_agents[count++] = orch->agents[i];
        }
    }

    pthread_mutex_unlock(&g_registry_mutex);

    return count;
}

// ============================================================================
// AGENT DEFINITION LOADING
// ============================================================================

// Parse markdown agent definition file
static ManagedAgent* parse_agent_md(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return NULL;

    char line[4096];
    char name[256] = {0};
    char description[1024] = {0};
    char* system_prompt = NULL;
    size_t prompt_size = 0;
    bool in_frontmatter = false;
    bool in_content = false;

    // Parse YAML frontmatter and markdown content
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "---", 3) == 0) {
            if (!in_frontmatter) {
                in_frontmatter = true;
            } else {
                in_frontmatter = false;
                in_content = true;
            }
            continue;
        }

        if (in_frontmatter) {
            // Parse YAML
            if (strncmp(line, "name:", 5) == 0) {
                sscanf(line + 5, " %255s", name);
            } else if (strncmp(line, "description:", 12) == 0) {
                char* desc = line + 12;
                while (*desc == ' ') desc++;
                strncpy(description, desc, sizeof(description) - 1);
                // Remove trailing newline
                char* nl = strchr(description, '\n');
                if (nl) *nl = '\0';
            }
        } else if (in_content) {
            // Accumulate markdown as system prompt
            size_t len = strlen(line);
            char* new_prompt = realloc(system_prompt, prompt_size + len + 1);
            if (new_prompt) {
                system_prompt = new_prompt;
                memcpy(system_prompt + prompt_size, line, len + 1);
                prompt_size += len;
            }
        }
    }

    fclose(f);

    if (name[0] == '\0' || !system_prompt) {
        free(system_prompt);
        return NULL;
    }

    // Determine role from name/description
    AgentRole role = AGENT_ROLE_ANALYST;  // Default
    if (strstr(name, "orchestrator") || strstr(name, "ali")) {
        role = AGENT_ROLE_ORCHESTRATOR;
    } else if (strstr(description, "architect") || strstr(description, "engineer") || strstr(description, "devops")) {
        role = AGENT_ROLE_CODER;
    } else if (strstr(description, "quality") || strstr(description, "security") || strstr(description, "legal")) {
        role = AGENT_ROLE_CRITIC;
    } else if (strstr(description, "plan") || strstr(description, "strategy") || strstr(description, "workflow")) {
        role = AGENT_ROLE_PLANNER;
    } else if (strstr(description, "design") || strstr(description, "creative") || strstr(description, "writer")) {
        role = AGENT_ROLE_WRITER;
    } else if (strstr(description, "execute") || strstr(description, "project") || strstr(description, "program")) {
        role = AGENT_ROLE_EXECUTOR;
    } else if (strstr(description, "memory") || strstr(description, "context")) {
        role = AGENT_ROLE_MEMORY;
    }

    ManagedAgent* agent = agent_create(name, role, system_prompt);
    free(system_prompt);

    return agent;
}

// Load all agent definitions from directory
int agent_load_definitions(const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (!dir) return -1;

    struct dirent* entry;
    int loaded = 0;

    while ((entry = readdir(dir)) != NULL) {
        // Only process .md files
        size_t len = strlen(entry->d_name);
        if (len < 4 || strcmp(entry->d_name + len - 3, ".md") != 0) {
            continue;
        }

        // Skip common files
        if (strcmp(entry->d_name, "CommonValuesAndPrinciples.md") == 0) {
            continue;
        }

        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);

        ManagedAgent* agent = parse_agent_md(filepath);
        if (agent) {
            Orchestrator* orch = orchestrator_get();
            if (orch) {
                pthread_mutex_lock(&g_registry_mutex);

                // Check capacity
                if (orch->agent_count >= orch->agent_capacity) {
                    size_t new_capacity = orch->agent_capacity * 2;
                    ManagedAgent** new_agents = realloc(orch->agents, sizeof(ManagedAgent*) * new_capacity);
                    if (new_agents) {
                        orch->agents = new_agents;
                        orch->agent_capacity = new_capacity;
                    }
                }

                if (orch->agent_count < orch->agent_capacity) {
                    orch->agents[orch->agent_count++] = agent;
                    loaded++;
                }

                pthread_mutex_unlock(&g_registry_mutex);
            }
        }
    }

    closedir(dir);
    return loaded;
}

// ============================================================================
// AGENT SELECTION (RACI-BASED)
// ============================================================================

// Find best agents for a task based on keywords
typedef struct {
    const char* keyword;
    const char* agents[5];  // Ranked by relevance
} TaskMapping;

static const TaskMapping TASK_MAPPINGS[] = {
    {"architecture", {"baccio", "dan", "marco", NULL}},
    {"strategy", {"domik", "matteo", "antonio", "satya", NULL}},
    {"code", {"baccio", "dan", "marco", NULL}},
    {"security", {"luca", "elena", NULL}},
    {"design", {"sara", "jony", "stefano", NULL}},
    {"data", {"omri", "ava", NULL}},
    {"legal", {"elena", NULL}},
    {"project", {"davide", "luke", NULL}},
    {"marketing", {"sofia", "fabio", NULL}},
    {"quality", {"thor", NULL}},
    {"memory", {"marcus", NULL}},
    {"workflow", {"wanda", NULL}},
    {"prompt", {"po", NULL}},
    {NULL, {NULL}}
};

// Select agents for a task
size_t agent_select_for_task(const char* task_description, ManagedAgent** out_agents, size_t max_count) {
    if (!task_description || !out_agents || max_count == 0) return 0;

    // Convert to lowercase for matching
    char* lower = strdup(task_description);
    for (char* p = lower; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') *p += 32;
    }

    size_t count = 0;

    // Find matching agents
    for (int i = 0; TASK_MAPPINGS[i].keyword != NULL && count < max_count; i++) {
        if (strstr(lower, TASK_MAPPINGS[i].keyword)) {
            for (int j = 0; TASK_MAPPINGS[i].agents[j] != NULL && count < max_count; j++) {
                ManagedAgent* agent = agent_find_by_name(TASK_MAPPINGS[i].agents[j]);
                if (!agent) {
                    // Spawn if not exists
                    agent = agent_spawn(AGENT_ROLE_ANALYST, TASK_MAPPINGS[i].agents[j], NULL);
                }
                if (agent) {
                    // Check if already in list
                    bool exists = false;
                    for (size_t k = 0; k < count; k++) {
                        if (out_agents[k] == agent) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        out_agents[count++] = agent;
                    }
                }
            }
        }
    }

    free(lower);

    return count;
}

// ============================================================================
// PARALLEL EXECUTION
// ============================================================================

typedef struct {
    ManagedAgent* agent;
    const char* input;
    char* output;
    bool completed;
} AgentTask;

// Claude API
extern char* nous_claude_chat(const char* system_prompt, const char* user_message);
extern void cost_record_agent_usage(ManagedAgent* agent, uint64_t input_tokens, uint64_t output_tokens);

// Execute multiple agents in parallel using GCD
void agent_execute_parallel(ManagedAgent** agents, size_t count, const char* input, char** outputs) {
    if (!agents || count == 0 || !input || !outputs) return;

    dispatch_group_t group = dispatch_group_create();
    dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);

    AgentTask* tasks = calloc(count, sizeof(AgentTask));
    if (!tasks) return;

    for (size_t i = 0; i < count; i++) {
        tasks[i].agent = agents[i];
        tasks[i].input = input;
        tasks[i].output = NULL;
        tasks[i].completed = false;

        dispatch_group_async(group, queue, ^{
            // Call Claude API for this agent
            if (tasks[i].agent && tasks[i].agent->system_prompt) {
                char* response = nous_claude_chat(tasks[i].agent->system_prompt, tasks[i].input);
                if (response) {
                    tasks[i].output = response;
                    tasks[i].completed = true;

                    // Record cost
                    cost_record_agent_usage(tasks[i].agent,
                        strlen(tasks[i].agent->system_prompt) / 4 + strlen(tasks[i].input) / 4,
                        strlen(response) / 4);
                }
            }
        });
    }

    // Wait for all to complete
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

    // Copy results
    for (size_t i = 0; i < count; i++) {
        outputs[i] = tasks[i].output;
    }

    free(tasks);
}

// ============================================================================
// REGISTRY STATUS
// ============================================================================

char* agent_registry_status(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return strdup("Registry not initialized");

    pthread_mutex_lock(&g_registry_mutex);

    size_t buf_size = 4096;
    char* status = malloc(buf_size);
    if (!status) {
        pthread_mutex_unlock(&g_registry_mutex);
        return NULL;
    }

    size_t offset = 0;
    offset += snprintf(status + offset, buf_size - offset,
        "Agent Registry Status\n"
        "=====================\n"
        "Total agents: %zu / %zu\n\n"
        "Active agents:\n",
        orch->agent_count, orch->agent_capacity);

    for (size_t i = 0; i < orch->agent_count && offset < buf_size - 256; i++) {
        ManagedAgent* agent = orch->agents[i];
        const char* role_name;
        switch (agent->role) {
            case AGENT_ROLE_ORCHESTRATOR: role_name = "Orchestrator"; break;
            case AGENT_ROLE_ANALYST: role_name = "Analyst"; break;
            case AGENT_ROLE_CODER: role_name = "Coder"; break;
            case AGENT_ROLE_WRITER: role_name = "Writer"; break;
            case AGENT_ROLE_CRITIC: role_name = "Critic"; break;
            case AGENT_ROLE_PLANNER: role_name = "Planner"; break;
            case AGENT_ROLE_EXECUTOR: role_name = "Executor"; break;
            case AGENT_ROLE_MEMORY: role_name = "Memory"; break;
            default: role_name = "Unknown"; break;
        }

        offset += snprintf(status + offset, buf_size - offset,
            "  - %s (%s) [%s] - $%.4f\n",
            agent->name,
            role_name,
            agent->is_active ? "active" : "inactive",
            agent->usage.cost_usd);
    }

    pthread_mutex_unlock(&g_registry_mutex);

    return status;
}
