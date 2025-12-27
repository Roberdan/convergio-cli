/**
 * CONVERGIO AGENT REGISTRY
 *
 * Dynamic agent pool management:
 * - Spawn agents on demand
 * - Load definitions from embedded agents (compiled into binary)
 * - Track active agents
 * - Manage agent lifecycle
 */

#include "nous/debug_mutex.h"
#include "nous/orchestrator.h"
#include "nous/safe_path.h"
#include <dirent.h>
#include <dispatch/dispatch.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Embedded agents
#include "nous/edition.h"
#include "nous/embedded_agents.h"
#include "nous/projects.h"

// ============================================================================
// ANTI-HALLUCINATION CONSTITUTION (MANDATORY FOR ALL AGENTS)
// ============================================================================
// This constitution is prepended to EVERY agent's system prompt to ensure
// brutal honesty, prevent hallucinations, and enforce uncertainty disclosure.

static const char* AGENT_CONSTITUTION =
    "## MANDATORY CONSTITUTION (NON-NEGOTIABLE)\n\n"
    "**You are bound by this constitution. Violating it is unacceptable.**\n\n"
    "### Rule 1: ABSOLUTE HONESTY\n"
    "- NEVER fabricate, invent, or guess information\n"
    "- NEVER pretend to have done something you haven't done\n"
    "- NEVER claim capabilities you don't have\n"
    "- If you don't know something, say \"I don't know\"\n"
    "- If you're not 100% certain, explicitly state your uncertainty level\n\n"
    "### Rule 2: UNCERTAINTY DISCLOSURE\n"
    "- When uncertain, preface with: \"I'm not certain, but...\"\n"
    "- When making assumptions, clearly state: \"I'm assuming...\"\n"
    "- When guessing, say: \"This is my best guess...\"\n"
    "- Distinguish clearly between facts you know and inferences you make\n\n"
    "### Rule 3: SOURCE ATTRIBUTION\n"
    "- If you read it from a file, say so\n"
    "- If you searched for it, say so\n"
    "- If you're inferring it, say so\n"
    "- If it's from your training knowledge, acknowledge the cutoff date\n\n"
    "### Rule 4: ERROR ACKNOWLEDGMENT\n"
    "- If you make a mistake, immediately acknowledge it\n"
    "- Never double down on errors\n"
    "- Correct yourself promptly when wrong\n\n"
    "### Rule 5: LIMITATION TRANSPARENCY\n"
    "- State clearly what you cannot do\n"
    "- Don't overpromise capabilities\n"
    "- Recommend external resources when you've reached your limits\n\n"
    "**END OF CONSTITUTION - Your specific role follows below:**\n\n";

// Forward declarations
extern Orchestrator* orchestrator_get(void);

// Thread safety
CONVERGIO_MUTEX_DECLARE(g_registry_mutex);

// Agent ID counter
static uint64_t g_next_agent_id = 1;

// ============================================================================
// HASH TABLE IMPLEMENTATION (FNV-1a)
// ============================================================================

// FNV-1a hash for strings (case-insensitive for consistent lookup)
static uint32_t fnv1a_hash(const char* str) {
    uint32_t hash = 2166136261u; // FNV offset basis
    while (*str) {
        // Convert to lowercase for case-insensitive hashing
        uint8_t c = (uint8_t)(*str++);
        if (c >= 'A' && c <= 'Z')
            c += 32;
        hash ^= c;
        hash *= 16777619u; // FNV prime
    }
    return hash;
}

// Hash for SemanticID
static uint32_t id_hash(SemanticID id) {
    // Mix the bits for better distribution
    uint64_t x = id;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return (uint32_t)(x ^ (x >> 31));
}

AgentHashTable* agent_hash_create(void) {
    AgentHashTable* ht = calloc(1, sizeof(AgentHashTable));
    return ht;
}

void agent_hash_destroy(AgentHashTable* ht) {
    if (!ht)
        return;
    for (size_t i = 0; i < AGENT_HASH_SIZE; i++) {
        AgentHashEntry* entry = ht->buckets[i];
        while (entry) {
            AgentHashEntry* next = entry->next;
            free(entry);
            entry = next;
        }
    }
    free(ht);
}

void agent_hash_insert_by_id(AgentHashTable* ht, ManagedAgent* agent) {
    if (!ht || !agent)
        return;
    uint32_t idx = id_hash(agent->id) % AGENT_HASH_SIZE;

    AgentHashEntry* entry = malloc(sizeof(AgentHashEntry));
    entry->id = agent->id;
    entry->agent = agent;
    entry->next = ht->buckets[idx];
    ht->buckets[idx] = entry;
    ht->count++;
}

void agent_hash_insert_by_name(AgentHashTable* ht, ManagedAgent* agent) {
    if (!ht || !agent || !agent->name)
        return;
    uint32_t idx = fnv1a_hash(agent->name) % AGENT_HASH_SIZE;

    AgentHashEntry* entry = malloc(sizeof(AgentHashEntry));
    entry->id = agent->id;
    entry->agent = agent;
    entry->next = ht->buckets[idx];
    ht->buckets[idx] = entry;
    // Don't increment count - already counted in by_id
}

ManagedAgent* agent_hash_find_by_id(AgentHashTable* ht, SemanticID id) {
    if (!ht || id == 0)
        return NULL;
    uint32_t idx = id_hash(id) % AGENT_HASH_SIZE;

    AgentHashEntry* entry = ht->buckets[idx];
    while (entry) {
        if (entry->id == id)
            return entry->agent;
        entry = entry->next;
    }
    return NULL;
}

ManagedAgent* agent_hash_find_by_name(AgentHashTable* ht, const char* name) {
    if (!ht || !name)
        return NULL;
    uint32_t idx = fnv1a_hash(name) % AGENT_HASH_SIZE;

    AgentHashEntry* entry = ht->buckets[idx];
    while (entry) {
        if (entry->agent && entry->agent->name && strcasecmp(entry->agent->name, name) == 0) {
            return entry->agent;
        }
        entry = entry->next;
    }
    return NULL;
}

void agent_hash_remove(AgentHashTable* ht, SemanticID id) {
    if (!ht || id == 0)
        return;
    uint32_t idx = id_hash(id) % AGENT_HASH_SIZE;

    AgentHashEntry* prev = NULL;
    AgentHashEntry* entry = ht->buckets[idx];
    while (entry) {
        if (entry->id == id) {
            if (prev) {
                prev->next = entry->next;
            } else {
                ht->buckets[idx] = entry->next;
            }
            free(entry);
            ht->count--;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

// ============================================================================
// MESSAGE POOL IMPLEMENTATION
// ============================================================================

MessagePool* message_pool_create(void) {
    MessagePool* pool = calloc(1, sizeof(MessagePool));
    return pool;
}

void message_pool_destroy(MessagePool* pool) {
    if (!pool)
        return;
    // Free any strings in used messages
    for (size_t i = 0; i < MESSAGE_POOL_SIZE; i++) {
        if (pool->in_use[i]) {
            free(pool->messages[i].content);
            free(pool->messages[i].metadata_json);
        }
    }
    free(pool);
}

Message* message_pool_alloc(MessagePool* pool) {
    if (!pool)
        return NULL;

    // Find free slot starting from next_free
    for (size_t i = 0; i < MESSAGE_POOL_SIZE; i++) {
        size_t idx = (pool->next_free + i) % MESSAGE_POOL_SIZE;
        if (!pool->in_use[idx]) {
            pool->in_use[idx] = 1;
            pool->next_free = (idx + 1) % MESSAGE_POOL_SIZE;
            pool->active_count++;
            memset(&pool->messages[idx], 0, sizeof(Message));
            return &pool->messages[idx];
        }
    }

    // Pool exhausted, fallback to malloc
    return calloc(1, sizeof(Message));
}

void message_pool_free(MessagePool* pool, Message* msg) {
    if (!msg)
        return;

    // Free strings
    free(msg->content);
    free(msg->metadata_json);
    msg->content = NULL;
    msg->metadata_json = NULL;

    if (pool) {
        // Check if msg is from the pool
        ptrdiff_t offset = msg - pool->messages;
        if (offset >= 0 && offset < MESSAGE_POOL_SIZE) {
            pool->in_use[offset] = 0;
            pool->active_count--;
            return;
        }
    }

    // Not from pool, free normally
    free(msg);
}

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
     "You are Ali, the Chief of Staff and master orchestrator. You coordinate all specialist "
     "agents to deliver comprehensive solutions."},

    {"satya", AGENT_ROLE_PLANNER, "Board of Directors - System Thinking",
     "You are Satya, providing system-thinking AI with strategic clarity and emotional "
     "intelligence."},

    {"domik", AGENT_ROLE_ANALYST, "McKinsey Strategic Decision Maker",
     "You are Domik, a McKinsey Partner-level strategic decision maker using the ISE "
     "Prioritization Framework."},

    {"matteo", AGENT_ROLE_ANALYST, "Strategic Business Architect",
     "You are Matteo, expert in business strategy, market analysis, and strategic roadmapping."},

    {"antonio", AGENT_ROLE_PLANNER, "Strategy Expert",
     "You are Antonio, expert in OKR, Lean Startup, Agile, SWOT Analysis, and Blue Ocean "
     "Strategy."},

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

    {NULL, 0, NULL, NULL} // Terminator
};

// ============================================================================
// AGENT CREATION
// ============================================================================

static SemanticID generate_agent_id(void) {
    return (SemanticID)__sync_fetch_and_add(&g_next_agent_id, 1);
}

/**
 * Generate current context header with date/time
 * Format: ## Current Context\n- Date: YYYY-MM-DD\n- Time: HH:MM:SS (timezone)\n\n
 */
static char* generate_context_header(void) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);

    char* header = malloc(256);
    if (!header)
        return NULL;

    char date_str[32];
    char time_str[32];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
    strftime(time_str, sizeof(time_str), "%H:%M:%S %Z", tm_info);

    snprintf(header, 256,
             "## Current Context\n"
             "- **Date**: %s\n"
             "- **Time**: %s\n"
             "- **Important**: When searching the web, use the current year (%d) for up-to-date "
             "results.\n\n",
             date_str, time_str, tm_info->tm_year + 1900);

    return header;
}

ManagedAgent* agent_create(const char* name, AgentRole role, const char* system_prompt) {
    ManagedAgent* agent = calloc(1, sizeof(ManagedAgent));
    if (!agent)
        return NULL;

    agent->id = generate_agent_id();
    agent->name = strdup(name);
    agent->role = role;

    // Generate current context header with date/time
    char* context_header = generate_context_header();
    size_t context_len = context_header ? strlen(context_header) : 0;

    // CRITICAL: Prepend the anti-hallucination constitution to ALL agent prompts
    // This ensures every agent is bound by brutal honesty requirements
    size_t constitution_len = strlen(AGENT_CONSTITUTION);
    size_t prompt_len = system_prompt ? strlen(system_prompt) : 0;
    size_t total_len = context_len + constitution_len + prompt_len + 1;

    char* full_prompt = malloc(total_len);
    if (full_prompt) {
        size_t offset = 0;

        // 1. Current context (date/time)
        if (context_header) {
            memcpy(full_prompt, context_header, context_len);
            offset = context_len;
        }

        // 2. Constitution
        memcpy(full_prompt + offset, AGENT_CONSTITUTION, constitution_len);
        offset += constitution_len;

        // 3. Agent-specific prompt
        if (system_prompt) {
            memcpy(full_prompt + offset, system_prompt, prompt_len);
        }
        full_prompt[total_len - 1] = '\0';
        agent->system_prompt = full_prompt;
    } else {
        // Fallback: just use original prompt if allocation fails
        agent->system_prompt = system_prompt ? strdup(system_prompt) : NULL;
    }

    free(context_header);

    agent->is_active = true;
    agent->created_at = time(NULL);
    agent->last_active = time(NULL);

    return agent;
}

void agent_destroy(ManagedAgent* agent) {
    if (!agent)
        return;

    free(agent->name);
    free(agent->description);
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
    if (!orch || !orch->initialized)
        return NULL;

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    // Check capacity
    if (orch->agent_count >= orch->agent_capacity) {
        // Expand capacity
        size_t new_capacity = orch->agent_capacity * 2;
        ManagedAgent** new_agents = realloc(orch->agents, sizeof(ManagedAgent*) * new_capacity);
        if (!new_agents) {
            CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);
            return NULL;
        }
        orch->agents = new_agents;
        orch->agent_capacity = new_capacity;
    }

    // Find default prompt for this agent
    const char* system_prompt = NULL;
    for (size_t i = 0; CORE_AGENTS[i].name != NULL; i++) {
        if (strcasecmp(CORE_AGENTS[i].name, name) == 0) {
            system_prompt = CORE_AGENTS[i].default_prompt;
            role = CORE_AGENTS[i].role;
            break;
        }
    }

    if (!system_prompt) {
        // Generic prompt based on role
        static char generic_prompt[512];
        snprintf(generic_prompt, sizeof(generic_prompt), "You are %s, a specialist agent. %s", name,
                 context ? context : "Help the user with their request.");
        system_prompt = generic_prompt;
    }

    ManagedAgent* agent = agent_create(name, role, system_prompt);
    if (!agent) {
        CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);
        return NULL;
    }

    if (context) {
        agent->specialized_context = strdup(context);
    }

    // Add to pool
    orch->agents[orch->agent_count++] = agent;

    // Add to hash tables for O(1) lookup
    if (orch->agent_by_id) {
        agent_hash_insert_by_id(orch->agent_by_id, agent);
    }
    if (orch->agent_by_name) {
        agent_hash_insert_by_name(orch->agent_by_name, agent);
    }

    // Callback
    if (orch->on_agent_spawn) {
        orch->on_agent_spawn(agent, orch->callback_ctx);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);

    return agent;
}

void agent_despawn(ManagedAgent* agent) {
    if (!agent)
        return;

    Orchestrator* orch = orchestrator_get();
    if (!orch)
        return;

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    // Remove from hash tables
    if (orch->agent_by_id) {
        agent_hash_remove(orch->agent_by_id, agent->id);
    }
    // Note: We don't have agent_hash_remove_by_name, but since we destroy
    // the agent, the entry will become stale. This is acceptable for now.

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

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);

    agent_destroy(agent);
}

ManagedAgent* agent_find_by_role(AgentRole role) {
    Orchestrator* orch = orchestrator_get();
    if (!orch)
        return NULL;

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    ManagedAgent* found = NULL;
    for (size_t i = 0; i < orch->agent_count; i++) {
        if (orch->agents[i]->role == role && orch->agents[i]->is_active) {
            found = orch->agents[i];
            break;
        }
    }

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);

    return found;
}

ManagedAgent* agent_find_by_name(const char* name) {
    if (!name)
        return NULL;

    Orchestrator* orch = orchestrator_get();
    if (!orch)
        return NULL;

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    ManagedAgent* found = NULL;

    // FIX-14: Use O(1) hash table lookup first (exact match)
    if (orch->agent_by_name) {
        found = agent_hash_find_by_name(orch->agent_by_name, name);
    }

    // If not found in hash, try prefix match (e.g., "baccio" matches "Baccio-tech-architect")
    // This is still O(n) but only used as fallback for short names
    if (!found) {
        LOG_INFO(LOG_CAT_AGENT, "[AGENT LOOKUP] Hash miss for '%s', trying prefix match across %zu agents",
                  name, orch->agent_count);
        size_t name_len = strlen(name);
        for (size_t i = 0; i < orch->agent_count; i++) {
            const char* agent_name = orch->agents[i]->name;
            // Check if name is a prefix (case-insensitive)
            if (strncasecmp(agent_name, name, name_len) == 0) {
                // Make sure it's at a word boundary (next char is '-', '_', or end)
                char next_char = agent_name[name_len];
                if (next_char == '\0' || next_char == '-' || next_char == '_') {
                    LOG_INFO(LOG_CAT_AGENT, "[AGENT LOOKUP] Prefix match: '%s' -> '%s'",
                              name, agent_name);
                    found = orch->agents[i];
                    break;
                }
            }
        }
        if (!found) {
            LOG_WARN(LOG_CAT_AGENT, "[AGENT LOOKUP] No prefix match found for '%s'", name);
        }
    }

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);

    return found;
}

// Check if a name is a known/embedded agent (without spawning)
bool agent_is_known_name(const char* name) {
    if (!name)
        return false;

    // Check CORE_AGENTS list
    for (size_t i = 0; CORE_AGENTS[i].name != NULL; i++) {
        // Match by full name or short name (first word before hyphen)
        const char* agent_name = CORE_AGENTS[i].name;
        if (strcasecmp(agent_name, name) == 0) {
            return true;
        }
        // Also match short name (e.g., "amy" matches "amy-cfo")
        const char* hyphen = strchr(agent_name, '-');
        if (hyphen) {
            size_t short_len = (size_t)(hyphen - agent_name);
            if (strlen(name) == short_len && strncasecmp(agent_name, name, short_len) == 0) {
                return true;
            }
        }
    }
    return false;
}

// Get all active agents
size_t agent_get_active(ManagedAgent** out_agents, size_t max_count) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !out_agents)
        return 0;

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    size_t count = 0;
    for (size_t i = 0; i < orch->agent_count && count < max_count; i++) {
        if (orch->agents[i]->is_active) {
            out_agents[count++] = orch->agents[i];
        }
    }

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);

    return count;
}

// Get ALL registered agents (for autocomplete, regardless of active state)
size_t agent_get_all(ManagedAgent** out_agents, size_t max_count) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !out_agents)
        return 0;

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    size_t count = 0;
    for (size_t i = 0; i < orch->agent_count && count < max_count; i++) {
        out_agents[count++] = orch->agents[i];
    }

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);

    return count;
}

// ============================================================================
// AGENT DEFINITION LOADING
// ============================================================================

// Parse markdown agent definition file (reserved for future dynamic agent loading)
__attribute__((unused)) static ManagedAgent* parse_agent_md(const char* filepath) {
    int fd = safe_path_open(filepath, safe_path_get_cwd_boundary(), O_RDONLY, 0);
    FILE* f = fd >= 0 ? fdopen(fd, "r") : NULL;
    if (!f)
        return NULL;

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
                while (*desc == ' ')
                    desc++;
                strncpy(description, desc, sizeof(description) - 1);
                // Remove trailing newline
                char* nl = strchr(description, '\n');
                if (nl)
                    *nl = '\0';
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
    AgentRole role = AGENT_ROLE_ANALYST; // Default
    if (strstr(name, "orchestrator") || strstr(name, "ali")) {
        role = AGENT_ROLE_ORCHESTRATOR;
    } else if (strstr(description, "architect") || strstr(description, "engineer") ||
               strstr(description, "devops")) {
        role = AGENT_ROLE_CODER;
    } else if (strstr(description, "quality") || strstr(description, "security") ||
               strstr(description, "legal")) {
        role = AGENT_ROLE_CRITIC;
    } else if (strstr(description, "plan") || strstr(description, "strategy") ||
               strstr(description, "workflow")) {
        role = AGENT_ROLE_PLANNER;
    } else if (strstr(description, "design") || strstr(description, "creative") ||
               strstr(description, "writer")) {
        role = AGENT_ROLE_WRITER;
    } else if (strstr(description, "execute") || strstr(description, "project") ||
               strstr(description, "program")) {
        role = AGENT_ROLE_EXECUTOR;
    } else if (strstr(description, "memory") || strstr(description, "context")) {
        role = AGENT_ROLE_MEMORY;
    }

    ManagedAgent* agent = agent_create(name, role, system_prompt);
    free(system_prompt);

    return agent;
}

// Parse embedded agent content (not from file)
static ManagedAgent* parse_embedded_agent(const char* filename, const char* content) {
    if (!content)
        return NULL;

    char name[256] = {0};
    char description[1024] = {0};
    char* system_prompt = NULL;
    size_t prompt_size = 0;
    bool in_frontmatter = false;
    bool in_content = false;

    const char* ptr = content;
    char line[4096];

    while (*ptr) {
        // Read one line
        const char* eol = strchr(ptr, '\n');
        size_t line_len = eol ? (size_t)(eol - ptr) : strlen(ptr);
        if (line_len >= sizeof(line))
            line_len = sizeof(line) - 1;
        strncpy(line, ptr, line_len);
        line[line_len] = '\0';

        if (strncmp(line, "---", 3) == 0) {
            if (!in_frontmatter) {
                in_frontmatter = true;
            } else {
                in_frontmatter = false;
                in_content = true;
            }
            ptr = eol ? eol + 1 : ptr + line_len;
            continue;
        }

        if (in_frontmatter) {
            if (strncmp(line, "name:", 5) == 0) {
                sscanf(line + 5, " %255s", name);
            } else if (strncmp(line, "description:", 12) == 0) {
                char* desc = line + 12;
                while (*desc == ' ')
                    desc++;
                strncpy(description, desc, sizeof(description) - 1);
            }
        } else if (in_content) {
            size_t len = line_len + 1; // +1 for newline
            char* new_prompt = realloc(system_prompt, prompt_size + len + 1);
            if (new_prompt) {
                system_prompt = new_prompt;
                memcpy(system_prompt + prompt_size, line, line_len);
                system_prompt[prompt_size + line_len] = '\n';
                system_prompt[prompt_size + len] = '\0';
                prompt_size += len;
            }
        }

        ptr = eol ? eol + 1 : ptr + line_len;
        if (!eol)
            break;
    }

    if (name[0] == '\0' || !system_prompt) {
        free(system_prompt);
        return NULL;
    }

    // Determine role from name/description
    AgentRole role = AGENT_ROLE_ANALYST;
    if (strstr(name, "orchestrator") || strstr(name, "ali")) {
        role = AGENT_ROLE_ORCHESTRATOR;
    } else if (strstr(description, "architect") || strstr(description, "engineer") ||
               strstr(description, "devops")) {
        role = AGENT_ROLE_CODER;
    } else if (strstr(description, "quality") || strstr(description, "security") ||
               strstr(description, "legal")) {
        role = AGENT_ROLE_CRITIC;
    } else if (strstr(description, "plan") || strstr(description, "strategy") ||
               strstr(description, "workflow")) {
        role = AGENT_ROLE_PLANNER;
    } else if (strstr(description, "design") || strstr(description, "creative") ||
               strstr(description, "writer")) {
        role = AGENT_ROLE_WRITER;
    } else if (strstr(description, "execute") || strstr(description, "project") ||
               strstr(description, "program")) {
        role = AGENT_ROLE_EXECUTOR;
    } else if (strstr(description, "memory") || strstr(description, "context")) {
        role = AGENT_ROLE_MEMORY;
    }

    ManagedAgent* agent = agent_create(name, role, system_prompt);
    if (agent && description[0]) {
        agent->description = strdup(description);
    }
    free(system_prompt);
    return agent;
}

// Load all agent definitions from embedded data
int agent_load_definitions(const char* dir_path) {
    (void)dir_path; // Ignored - using embedded agents

    size_t agent_count = 0;
    const EmbeddedAgent* agents = get_all_embedded_agents(&agent_count);

    if (!agents || agent_count == 0) {
        return -1;
    }

    int loaded = 0;
    ConvergioEdition current_edition = edition_current();
    bool is_education = (current_edition == EDITION_EDUCATION);

    // CRITICAL: Verify edition is set before loading agents
    if (current_edition == EDITION_MASTER) {
        // Master edition loads all agents - no filtering needed
    } else {
        // Limited edition - must filter agents
        LOG_INFO(LOG_CAT_SYSTEM, "[Agent Load] Filtering agents for edition: %s",
                 edition_get_name(current_edition));
    }

    for (size_t i = 0; i < agent_count; i++) {
        const EmbeddedAgent* ea = &agents[i];

        // Skip common files and safety guidelines
        if (strcmp(ea->filename, "CommonValuesAndPrinciples.md") == 0) {
            continue;
        }
        if (strstr(ea->filename, "SAFETY_AND_INCLUSIVITY") != NULL) {
            continue;
        }

        // Skip Ali - created manually in orchestrator_init with correct edition prompt
        if (is_education) {
            if (strstr(ea->filename, "ali-principal") != NULL) {
                continue;
            }
        } else {
            if (strstr(ea->filename, "ali-chief-of-staff") != NULL) {
                continue;
            }
        }

        // Parse agent to get name for edition filtering
        ManagedAgent* agent = parse_embedded_agent(ea->filename, ea->content);
        if (!agent)
            continue;

        // CRITICAL: Filter by edition - only load agents available for this edition
        // Double-check edition is still correct (defensive programming)
        if (current_edition != EDITION_MASTER) {
            if (!edition_has_agent(agent->name)) {
                agent_destroy(agent); // Clean up agent we won't use
                continue;
            }
        }

        // Agent passed edition filter - add to orchestrator
        Orchestrator* orch = orchestrator_get();
        if (orch) {
            CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

            // Check capacity
            if (orch->agent_count >= orch->agent_capacity) {
                size_t new_capacity = orch->agent_capacity * 2;
                ManagedAgent** new_agents =
                    realloc(orch->agents, sizeof(ManagedAgent*) * new_capacity);
                if (new_agents) {
                    orch->agents = new_agents;
                    orch->agent_capacity = new_capacity;
                }
            }

            if (orch->agent_count < orch->agent_capacity) {
                orch->agents[orch->agent_count++] = agent;

                // Add to hash tables for O(1) lookup
                if (orch->agent_by_id) {
                    agent_hash_insert_by_id(orch->agent_by_id, agent);
                }
                if (orch->agent_by_name) {
                    agent_hash_insert_by_name(orch->agent_by_name, agent);
                }

                loaded++;
            }

            CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);
        }
    }

    return loaded;
}

// ============================================================================
// AGENT SELECTION (RACI-BASED)
// ============================================================================

// Find best agents for a task based on keywords
typedef struct {
    const char* keyword;
    const char* agents[5]; // Ranked by relevance
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
    {NULL, {NULL}}};

// Select agents for a task
size_t agent_select_for_task(const char* task_description, ManagedAgent** out_agents,
                             size_t max_count) {
    if (!task_description || !out_agents || max_count == 0)
        return 0;

    // Convert to lowercase for matching
    char* lower = strdup(task_description);
    for (char* p = lower; *p; p++) {
        if (*p >= 'A' && *p <= 'Z')
            *p += 32;
    }

    size_t count = 0;

    // Find matching agents
    for (size_t i = 0; TASK_MAPPINGS[i].keyword != NULL && count < max_count; i++) {
        if (strstr(lower, TASK_MAPPINGS[i].keyword)) {
            for (size_t j = 0; TASK_MAPPINGS[i].agents[j] != NULL && count < max_count; j++) {
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

// Provider interface for agent execution
#include "nous/provider.h"
#define REGISTRY_MODEL "claude-sonnet-4-20250514"

extern void cost_record_agent_usage(ManagedAgent* agent, uint64_t input_tokens,
                                    uint64_t output_tokens);

// Execute multiple agents in parallel using GCD
void agent_execute_parallel(ManagedAgent** agents, size_t count, const char* input,
                            char** outputs) {
    if (!agents || count == 0 || !input || !outputs)
        return;

    dispatch_group_t group = dispatch_group_create();
    dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);

    AgentTask* tasks = calloc(count, sizeof(AgentTask));
    if (!tasks)
        return;

    for (size_t i = 0; i < count; i++) {
        tasks[i].agent = agents[i];
        tasks[i].input = input;
        tasks[i].output = NULL;
        tasks[i].completed = false;

        dispatch_group_async(group, queue, ^{
            // Use Provider interface for agent execution
            if (tasks[i].agent && tasks[i].agent->system_prompt) {
                Provider* provider = provider_get(PROVIDER_ANTHROPIC);
                char* response = NULL;
                if (provider && provider->chat) {
                    TokenUsage usage = {0};
                    response =
                        provider->chat(provider, REGISTRY_MODEL, tasks[i].agent->system_prompt,
                                       tasks[i].input, &usage);
                }
                if (response) {
                    tasks[i].output = response;
                    tasks[i].completed = true;

                    // Record cost
                    cost_record_agent_usage(
                        tasks[i].agent,
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
// AGENT STATE MANAGEMENT
// ============================================================================

void agent_set_working(ManagedAgent* agent, AgentWorkState state, const char* task) {
    if (!agent)
        return;

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    agent->work_state = state;
    agent->work_started_at = time(NULL);

    free(agent->current_task);
    agent->current_task = task ? strdup(task) : NULL;

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);
}

void agent_set_idle(ManagedAgent* agent) {
    if (!agent)
        return;

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    agent->work_state = WORK_STATE_IDLE;
    free(agent->current_task);
    agent->current_task = NULL;
    agent->collaborating_with = 0;
    agent->work_started_at = 0;

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);
}

void agent_set_collaborating(ManagedAgent* agent, SemanticID partner_id) {
    if (!agent)
        return;

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    agent->work_state = WORK_STATE_COMMUNICATING;
    agent->collaborating_with = partner_id;

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);
}

size_t agent_get_working(ManagedAgent** out_agents, size_t max_count) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !out_agents)
        return 0;

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    size_t count = 0;
    for (size_t i = 0; i < orch->agent_count && count < max_count; i++) {
        if (orch->agents[i]->work_state != WORK_STATE_IDLE) {
            out_agents[count++] = orch->agents[i];
        }
    }

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);

    return count;
}

char* agent_get_working_status(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch)
        return strdup("No orchestrator");

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    size_t buf_size = 4096;
    char* status = malloc(buf_size);
    if (!status) {
        CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);
        return NULL;
    }

    size_t offset = 0;
    size_t working_count = 0;

    // Count working agents first
    for (size_t i = 0; i < orch->agent_count; i++) {
        if (orch->agents[i]->work_state != WORK_STATE_IDLE) {
            working_count++;
        }
    }

    if (working_count == 0) {
        offset = (size_t)snprintf(status, buf_size, "\033[2mNo agents currently working\033[0m\n");
    } else {
        offset = (size_t)snprintf(status, buf_size,
                                  "\033[1m🔄 Active Agents (%zu working)\033[0m\n", working_count);

        for (size_t i = 0; i < orch->agent_count && offset < buf_size - 256; i++) {
            ManagedAgent* agent = orch->agents[i];
            if (agent->work_state == WORK_STATE_IDLE)
                continue;

            const char* state_icon;
            const char* state_color;
            switch (agent->work_state) {
            case WORK_STATE_THINKING:
                state_icon = "💭";
                state_color = "\033[33m"; // Yellow
                break;
            case WORK_STATE_EXECUTING:
                state_icon = "⚡";
                state_color = "\033[32m"; // Green
                break;
            case WORK_STATE_WAITING:
                state_icon = "⏳";
                state_color = "\033[34m"; // Blue
                break;
            case WORK_STATE_COMMUNICATING:
                state_icon = "💬";
                state_color = "\033[35m"; // Magenta
                break;
            default:
                state_icon = "•";
                state_color = "\033[0m";
            }

            // Calculate duration
            time_t now = time(NULL);
            int duration = agent->work_started_at > 0 ? (int)(now - agent->work_started_at) : 0;

            offset += (size_t)snprintf(status + offset, buf_size - offset, "  %s %s%s\033[0m",
                                       state_icon, state_color, agent->name);

            if (agent->current_task) {
                offset += (size_t)snprintf(status + offset, buf_size - offset, " - %.40s%s",
                                           agent->current_task,
                                           strlen(agent->current_task) > 40 ? "..." : "");
            }

            if (duration > 0) {
                offset += (size_t)snprintf(status + offset, buf_size - offset,
                                           " \033[2m(%ds)\033[0m", duration);
            }

            // Show collaboration
            if (agent->collaborating_with != 0) {
                // Find partner name
                for (size_t j = 0; j < orch->agent_count; j++) {
                    if (orch->agents[j]->id == agent->collaborating_with) {
                        offset += (size_t)snprintf(status + offset, buf_size - offset, " ↔ %s",
                                                   orch->agents[j]->name);
                        break;
                    }
                }
            }

            offset += (size_t)snprintf(status + offset, buf_size - offset, "\n");
        }
    }

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);

    return status;
}

// ============================================================================
// INTER-AGENT COMMUNICATION
// ============================================================================

int agent_send_message(ManagedAgent* from, ManagedAgent* to, const char* content) {
    if (!from || !to || !content)
        return -1;

    Message* msg = message_create(MSG_TYPE_AGENT_RESPONSE, from->id, to->id, content);
    if (!msg)
        return -1;

    message_send(msg);
    return 0;
}

int agent_broadcast_status(ManagedAgent* agent, const char* status_msg) {
    if (!agent || !status_msg)
        return -1;

    Message* msg = message_create(MSG_TYPE_AGENT_THOUGHT, agent->id, 0, status_msg);
    if (!msg)
        return -1;

    message_broadcast(msg);
    return 0;
}

Message* agent_receive_message(ManagedAgent* agent) {
    if (!agent)
        return NULL;
    return message_get_pending(agent);
}

// ============================================================================
// REGISTRY STATUS
// ============================================================================

// Helper: categorize agent by name/description into expertise area (Business/Developer editions)
static int get_agent_category(const ManagedAgent* agent) {
    if (!agent || !agent->name)
        return 9; // Other

    const char* n = agent->name;
    const char* d = agent->description ? agent->description : "";

    // Leadership & Strategy
    if (strcasestr(n, "ali") || strcasestr(n, "domik") || strcasestr(n, "satya") ||
        strcasestr(n, "sam-") || strcasestr(n, "antonio") || strcasestr(n, "mckinsey") ||
        strcasestr(d, "strategic") || strcasestr(d, "CEO") || strcasestr(d, "Chief"))
        return 0;

    // Technology & Engineering
    if (strcasestr(n, "baccio") || strcasestr(n, "dan-") || strcasestr(n, "marco") ||
        strcasestr(n, "luca") || strcasestr(n, "devops") || strcasestr(n, "guardian") ||
        strcasestr(d, "architect") || strcasestr(d, "engineer") || strcasestr(d, "security"))
        return 1;

    // Data & Analytics
    if (strcasestr(n, "angela") || strcasestr(n, "ethan") || strcasestr(n, "ava") ||
        strcasestr(n, "omri") || strcasestr(n, "data") || strcasestr(d, "analytics") ||
        strcasestr(d, "data"))
        return 2;

    // Product & Design
    if (strcasestr(n, "sara") || strcasestr(n, "stefano") || strcasestr(n, "oliver") ||
        strcasestr(n, "marcus") || strcasestr(n, "ux") || strcasestr(n, "design") ||
        strcasestr(d, "design") || strcasestr(d, "UX") || strcasestr(d, "product"))
        return 3;

    // Business Operations
    if (strcasestr(n, "amy") || strcasestr(n, "fabio") || strcasestr(n, "sofia") ||
        strcasestr(n, "enrico") || strcasestr(n, "cfo") || strcasestr(n, "sales") ||
        strcasestr(d, "financial") || strcasestr(d, "sales") || strcasestr(d, "marketing"))
        return 4;

    // Project Management
    if (strcasestr(n, "wanda") || strcasestr(n, "thor") || strcasestr(n, "dave") ||
        strcasestr(n, "coach") || strcasestr(n, "workflow") || strcasestr(d, "project") ||
        strcasestr(d, "workflow") || strcasestr(d, "quality"))
        return 5;

    // Customer & HR
    if (strcasestr(n, "andrea") || strcasestr(n, "giulia") || strcasestr(n, "behice") ||
        strcasestr(n, "customer") || strcasestr(n, "hr") || strcasestr(d, "customer") ||
        strcasestr(d, "HR") || strcasestr(d, "talent"))
        return 6;

    // Healthcare & Compliance
    if (strcasestr(n, "enzo") || strcasestr(n, "healthcare") || strcasestr(n, "compliance") ||
        strcasestr(d, "healthcare") || strcasestr(d, "compliance"))
        return 7;

    // Creative & Content
    if (strcasestr(n, "po-") || strcasestr(n, "prompt") || strcasestr(n, "content") ||
        strcasestr(d, "content") || strcasestr(d, "prompt") || strcasestr(d, "creative"))
        return 8;

    return 9; // Other
}

// Helper: categorize education agents by subject area
static int get_education_category(const ManagedAgent* agent) {
    if (!agent || !agent->name)
        return 5; // Other

    const char* n = agent->name;

    // 0: School Leadership (Direzione Scolastica)
    if (strcasestr(n, "ali-principal") || strcasestr(n, "preside"))
        return 0;

    // 1: Sciences (Scienze) - math, physics, chemistry, astronomy, biology, computer science,
    // geography, health
    if (strcasestr(n, "euclide") || strcasestr(n, "matematica") || strcasestr(n, "feynman") ||
        strcasestr(n, "fisica") || strcasestr(n, "darwin") || strcasestr(n, "scienze") ||
        strcasestr(n, "curie") || strcasestr(n, "chimica") || strcasestr(n, "galileo") ||
        strcasestr(n, "astronomia") || strcasestr(n, "lovelace") || strcasestr(n, "informatica") ||
        strcasestr(n, "humboldt") || strcasestr(n, "geografia") || strcasestr(n, "ippocrate") ||
        strcasestr(n, "corpo"))
        return 1;

    // 2: Humanities (Lettere e Umanistiche) - history, civics, philosophy, economics
    if (strcasestr(n, "erodoto") || strcasestr(n, "storia") || strcasestr(n, "cicerone") ||
        strcasestr(n, "civica") || strcasestr(n, "socrate") || strcasestr(n, "filosofia") ||
        strcasestr(n, "smith") || strcasestr(n, "economia"))
        return 2;

    // 3: Languages (Lingue) - Italian, English
    if (strcasestr(n, "manzoni") || strcasestr(n, "italiano") || strcasestr(n, "shakespeare") ||
        strcasestr(n, "inglese"))
        return 3;

    // 4: Arts (Arti) - visual arts, music, storytelling
    if (strcasestr(n, "leonardo") || strcasestr(n, "arte") || strcasestr(n, "mozart") ||
        strcasestr(n, "musica") || strcasestr(n, "chris") || strcasestr(n, "storytelling"))
        return 4;

    return 5; // Other Teachers
}

char* agent_registry_status(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch)
        return strdup("Registry not initialized");

    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    size_t buf_size = 32768;
    char* status = malloc(buf_size);
    if (!status) {
        CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);
        return NULL;
    }

    size_t offset = 0;

    // Check for active project filtering
    ConvergioProject* proj = project_current();

    // Count available agents for this edition
    // CRITICAL: Only count agents that are actually available for this edition
    // This ensures we don't count agents loaded before edition was set, or agents
    // that shouldn't be in this edition
    size_t edition_agent_count = 0;
    // Note: edition_has_agent() internally uses edition_current()
    for (size_t i = 0; i < orch->agent_count; i++) {
        if (!orch->agents[i] || !orch->agents[i]->name) {
            continue; // Skip invalid agents
        }
        // Double-check: agent must be in edition whitelist AND edition must be correct
        if (edition_has_agent(orch->agents[i]->name)) {
            edition_agent_count++;
        }
    }

    // Header (different if project active)
    if (proj) {
        offset += (size_t)snprintf(status + offset, buf_size - offset,
                                   "\033[1mProject Agents: %s\033[0m\n\n"
                                   "Team of \033[1;36m%zu agents\033[0m for this project:\n\n",
                                   proj->name, proj->team_count);
    } else {
        const EditionInfo* edition = edition_get_current_info();
        if (edition->id != EDITION_FULL) {
            offset +=
                (size_t)snprintf(status + offset, buf_size - offset,
                                 "\033[1m%s - Available Agents\033[0m\n\n"
                                 "\033[1;36m%zu specialist agents\033[0m organized by area:\n\n",
                                 edition->name, edition_agent_count);
        } else {
            offset +=
                (size_t)snprintf(status + offset, buf_size - offset,
                                 "\033[1mMy Available Agents\033[0m\n\n"
                                 "\033[1;36m%zu specialist agents\033[0m organized by area:\n\n",
                                 edition_agent_count);
        }
    }

    // Categories with emojis - different for education vs business editions
    const char* business_cat_names[] = {"🎯 Leadership & Strategy", "⚡ Technology & Engineering",
                                        "📊 Data & Analytics",      "🎨 Product & Design",
                                        "💼 Business Operations",   "📋 Project Management",
                                        "👥 Customer & HR",         "🏥 Healthcare & Compliance",
                                        "✨ Creative & Content",    "🔧 Other Specialists"};

    const char* education_cat_names[] = {
        "🏫 Direzione Scolastica", "🔬 Scienze", "📖 Lettere e Umanistiche", "🗣️ Lingue", "🎨 Arti",
        "📚 Altri Maestri"};

    // Use education categories for education edition
    bool is_education = (edition_current() == EDITION_EDUCATION);
    size_t num_cats = is_education ? 6 : 10;
    const char** cat_names = is_education ? education_cat_names : business_cat_names;

    for (size_t cat = 0; cat < num_cats; cat++) {
        // Count agents in this category (with project and edition filtering)
        size_t count = 0;
        for (size_t i = 0; i < orch->agent_count; i++) {
            ManagedAgent* a = orch->agents[i];
            int agent_cat = is_education ? get_education_category(a) : get_agent_category(a);
            if (agent_cat == (int)cat) {
                // Apply edition filter first
                if (!edition_has_agent(a->name))
                    continue;
                // Apply project filter if active
                if (!proj || project_has_agent(a->name))
                    count++;
            }
        }
        if (count == 0)
            continue;

        // Category header
        offset += (size_t)snprintf(status + offset, buf_size - offset, "\033[1m%s\033[0m\n",
                                   cat_names[cat]);

        // Agents in this category (with project and edition filtering)
        for (size_t i = 0; i < orch->agent_count && offset < buf_size - 512; i++) {
            ManagedAgent* agent = orch->agents[i];
            int agent_cat =
                is_education ? get_education_category(agent) : get_agent_category(agent);
            if (agent_cat != (int)cat)
                continue;

            // Apply edition filter first
            if (!edition_has_agent(agent->name))
                continue;

            // Apply project filter if active
            if (proj && !project_has_agent(agent->name))
                continue;

            // Extract short name (capitalize first letter)
            char short_name[32];
            strncpy(short_name, agent->name, sizeof(short_name) - 1);
            short_name[sizeof(short_name) - 1] = '\0';
            // Remove suffix like -cto, -pm, etc for cleaner display
            char* dash = strchr(short_name, '-');
            if (dash && strlen(dash) < 6)
                *dash = '\0';
            if (short_name[0] >= 'a' && short_name[0] <= 'z') {
                short_name[0] -= 32; // Capitalize
            }

            // Short description
            char desc[60] = "";
            if (agent->description) {
                // Find first meaningful part
                const char* d = agent->description;
                // Skip "Elite" prefix
                if (strncasecmp(d, "Elite ", 6) == 0)
                    d += 6;
                // Skip "Senior" prefix
                if (strncasecmp(d, "Senior ", 7) == 0)
                    d += 7;
                // Skip "IC6" etc
                if (strncasecmp(d, "IC6 ", 4) == 0)
                    d += 4;

                size_t len = strlen(d);
                if (len > 55) {
                    memcpy(desc, d, 52);
                    memcpy(desc + 52, "...", 4);
                } else {
                    memcpy(desc, d, len + 1);
                }
            }

            offset += (size_t)snprintf(status + offset, buf_size - offset,
                                       "  • \033[36m%-12s\033[0m \033[2m- %s\033[0m\n", short_name,
                                       desc[0] ? desc : "-");
        }
        offset += (size_t)snprintf(status + offset, buf_size - offset, "\n");
    }

    // Footer with edition-appropriate example
    if (is_education) {
        offset +=
            (size_t)snprintf(status + offset, buf_size - offset,
                             "\033[2mParla con un maestro:\033[0m \033[36m@nome domanda\033[0m  "
                             "\033[2m(es. @euclide spiegami le frazioni)\033[0m\n");
    } else {
        offset += (size_t)snprintf(status + offset, buf_size - offset,
                                   "\033[2mUsa:\033[0m \033[36m@nome messaggio\033[0m  \033[2m(es. "
                                   "@baccio rivedi questo codice)\033[0m\n");
    }

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);

    return status;
}
