/**
 * CONVERGIO INTENT ROUTER
 *
 * Intelligent routing using fast LLM to understand user intent
 * and route to the appropriate agent.
 *
 * Uses the fastest available model (gemini-flash, gpt-4o-mini, or local)
 */

#include "nous/intent_router.h"
#include "nous/nous.h"
#include "nous/orchestrator.h"
#include "nous/provider.h"
#include <cjson/cJSON.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Router model priority (quality + speed balance)
#define ROUTER_MODEL_ANTHROPIC "claude-3-5-haiku-20241022"
#define ROUTER_MODEL_OPENAI "gpt-4o-mini"
#define ROUTER_MODEL_GEMINI "gemini-1.5-flash"
#define ROUTER_MODEL_LOCAL "llama-3.2-1b"

// System prompt for the router - kept minimal for speed
static const char* ROUTER_SYSTEM_PROMPT =
    "You are an intent router. Analyze the user's message and return JSON with:\n"
    "- \"agent\": the best agent name to handle this (or \"ali\" for general/unclear)\n"
    "- \"confidence\": 0.0-1.0 how confident you are\n"
    "- \"intent\": brief intent description\n\n"
    "Available agents:\n"
    "- ali: general orchestrator, unclear requests, multi-agent coordination\n"
    "- amy-cfo: financial analysis, costs, budgets, ROI, investments\n"
    "- dario-debugger: debugging, bugs, errors, troubleshooting\n"
    "- rex-code-reviewer: code review, code quality, best practices\n"
    "- baccio-tech-architect: architecture, system design, technical decisions\n"
    "- otto-performance-optimizer: performance, optimization, speed\n"
    "- luca-security-expert: security, vulnerabilities, penetration testing\n"
    "- marco-devops-engineer: CI/CD, deployment, infrastructure, docker\n"
    "- sara-ux-ui-designer: UX, UI, design, user experience\n"
    "- omri-data-scientist: data analysis, ML, statistics, predictions\n"
    "- sofia-marketing-strategist: marketing, growth, brand strategy\n"
    "- elena-legal-compliance-expert: legal, compliance, GDPR, contracts\n"
    "- davide-project-manager: project planning, timelines, coordination\n"
    "- anna-executive-assistant: tasks, reminders, scheduling\n\n"
    "If user explicitly names an agent (e.g., 'ask dario', 'talk to amy'), route to that agent.\n"
    "Respond ONLY with valid JSON, no explanation.";

// Cache for recent routes (avoid repeated LLM calls for similar inputs)
#define ROUTE_CACHE_SIZE 32
typedef struct {
    char* input_hash;
    char* agent;
    float confidence;
    time_t timestamp;
} RouteCache;

static RouteCache g_route_cache[ROUTE_CACHE_SIZE] = {0};
static size_t g_cache_index = 0;

// Simple hash for cache lookup
static unsigned long simple_hash(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + (unsigned long)tolower(c);
    }
    return hash;
}

// Check cache for recent route
static const char* cache_lookup(const char* input, float* confidence) {
    unsigned long hash = simple_hash(input);
    char hash_str[32];
    snprintf(hash_str, sizeof(hash_str), "%lu", hash);

    time_t now = time(NULL);
    for (size_t i = 0; i < ROUTE_CACHE_SIZE; i++) {
        if (g_route_cache[i].input_hash && strcmp(g_route_cache[i].input_hash, hash_str) == 0 &&
            (now - g_route_cache[i].timestamp) < 300) { // 5 min cache
            if (confidence)
                *confidence = g_route_cache[i].confidence;
            return g_route_cache[i].agent;
        }
    }
    return NULL;
}

// Add to cache
static void cache_add(const char* input, const char* agent, float confidence) {
    unsigned long hash = simple_hash(input);
    char hash_str[32];
    snprintf(hash_str, sizeof(hash_str), "%lu", hash);

    // Free old entry
    free(g_route_cache[g_cache_index].input_hash);
    free(g_route_cache[g_cache_index].agent);

    g_route_cache[g_cache_index].input_hash = strdup(hash_str);
    g_route_cache[g_cache_index].agent = strdup(agent);
    g_route_cache[g_cache_index].confidence = confidence;
    g_route_cache[g_cache_index].timestamp = time(NULL);

    g_cache_index = (g_cache_index + 1) % ROUTE_CACHE_SIZE;
}

// Check if input is a "switch to agent" request
// Returns agent name if it's a switch request, NULL otherwise
static const char* check_switch_intent(const char* input) {
    char lower[256];
    size_t len = strlen(input);
    if (len >= sizeof(lower))
        len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = (char)tolower((unsigned char)input[i]);
    }
    lower[len] = '\0';

    // Patterns that indicate "switch to agent" intent
    // Italian: "passami X", "fammi parlare con X", "voglio parlare con X", "mettimi con X"
    // English: "let me talk to X", "switch to X", "connect me to X", "pass me to X"
    bool is_switch =
        (strstr(lower, "passami") || strstr(lower, "passa mi") || strstr(lower, "fammi parlare") ||
         strstr(lower, "voglio parlare") || strstr(lower, "mettimi con") ||
         strstr(lower, "metti mi con") || strstr(lower, "let me talk") ||
         strstr(lower, "switch to") || strstr(lower, "connect me") || strstr(lower, "pass me to") ||
         strstr(lower, "talk to") || strstr(lower, "parla con") || strstr(lower, "chiamami") ||
         strstr(lower, "chiama mi"));

    if (!is_switch)
        return NULL;

    // Find which agent they want to switch to
    if (strstr(lower, "ali"))
        return "ali";
    if (strstr(lower, "amy"))
        return "amy-cfo";
    if (strstr(lower, "dario"))
        return "dario-debugger";
    if (strstr(lower, "rex"))
        return "rex-code-reviewer";
    if (strstr(lower, "baccio"))
        return "baccio-tech-architect";
    if (strstr(lower, "otto"))
        return "otto-performance-optimizer";
    if (strstr(lower, "luca"))
        return "luca-security-expert";
    if (strstr(lower, "marco"))
        return "marco-devops-engineer";
    if (strstr(lower, "sara"))
        return "sara-ux-ui-designer";
    if (strstr(lower, "omri"))
        return "omri-data-scientist";
    if (strstr(lower, "sofia"))
        return "sofia-marketing-strategist";
    if (strstr(lower, "elena"))
        return "elena-legal-compliance-expert";
    if (strstr(lower, "davide"))
        return "davide-project-manager";
    if (strstr(lower, "anna"))
        return "anna-executive-assistant";
    if (strstr(lower, "jenny"))
        return "jenny-inclusive-accessibility-champion";
    if (strstr(lower, "thor"))
        return "thor-quality-assurance-guardian";

    return NULL; // Switch intent but couldn't identify agent
}

// Quick pattern check before calling LLM (optimization)
static const char* quick_pattern_route(const char* input, bool* is_delegation) {
    // Convert to lowercase for matching
    char lower[256];
    size_t len = strlen(input);
    if (len >= sizeof(lower))
        len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = (char)tolower((unsigned char)input[i]);
    }
    lower[len] = '\0';

    // FIX: Delegation requests must go to Ali, not directly to named agents
    // When user says "delega a rex e baccio", it should NOT route to rex
    // but let Ali handle the orchestration and delegation
    // IMPORTANT: Return "ali" explicitly, NOT NULL (NULL would fall through to LLM router)
    if (strstr(lower, "delega") || strstr(lower, "delegate") ||
        strstr(lower, "coordina") || strstr(lower, "orchestra") ||
        strstr(lower, "chiedi a") || strstr(lower, "ask ") ||
        strstr(lower, "fai analizzare") || strstr(lower, "fai fare")) {
        if (is_delegation) *is_delegation = true;
        return "ali"; // Ali handles delegation - must be explicit, not NULL
    }

    // Explicit agent mentions
    if (strstr(lower, "dario") || strstr(lower, "debug"))
        return "dario-debugger";
    if (strstr(lower, "amy") || strstr(lower, "finanz") || strstr(lower, "cost") ||
        strstr(lower, "budget"))
        return "amy-cfo";
    if (strstr(lower, "rex") || strstr(lower, "review") || strstr(lower, "rivedi"))
        return "rex-code-reviewer";
    if (strstr(lower, "baccio") || strstr(lower, "architettura") || strstr(lower, "architect"))
        return "baccio-tech-architect";
    if (strstr(lower, "otto") || strstr(lower, "performance") || strstr(lower, "ottimizz"))
        return "otto-performance-optimizer";
    if (strstr(lower, "luca") || strstr(lower, "security") || strstr(lower, "sicurezz"))
        return "luca-security-expert";
    if (strstr(lower, "marco") || strstr(lower, "deploy") || strstr(lower, "docker") ||
        strstr(lower, "ci/cd"))
        return "marco-devops-engineer";
    if (strstr(lower, "sara") || strstr(lower, "design") || strstr(lower, "ux") ||
        strstr(lower, "ui"))
        return "sara-ux-ui-designer";
    if (strstr(lower, "omri") || strstr(lower, "data") || strstr(lower, "ml") ||
        strstr(lower, "statistic"))
        return "omri-data-scientist";
    if (strstr(lower, "sofia") || strstr(lower, "marketing") || strstr(lower, "brand"))
        return "sofia-marketing-strategist";
    if (strstr(lower, "elena") || strstr(lower, "legal") || strstr(lower, "gdpr") ||
        strstr(lower, "compliance"))
        return "elena-legal-compliance-expert";
    if (strstr(lower, "davide") || strstr(lower, "project") || strstr(lower, "timeline"))
        return "davide-project-manager";
    if (strstr(lower, "anna") || strstr(lower, "remind") || strstr(lower, "task") ||
        strstr(lower, "schedul"))
        return "anna-executive-assistant";

    // Bug-related keywords -> Dario
    if (strstr(lower, "bug") || strstr(lower, "error") || strstr(lower, "crash") ||
        strstr(lower, "fix") || strstr(lower, "broken"))
        return "dario-debugger";

    return NULL; // Need LLM
}

// Parse JSON response from router LLM
static bool parse_route_response(const char* response, char* agent_out, size_t agent_size,
                                 float* confidence_out, char* intent_out, size_t intent_size) {
    if (!response)
        return false;

    // Find JSON in response (might have extra text)
    const char* json_start = strchr(response, '{');
    const char* json_end = strrchr(response, '}');
    if (!json_start || !json_end || json_end <= json_start)
        return false;

    size_t json_len = (size_t)(json_end - json_start + 1);
    char* json_str = malloc(json_len + 1);
    if (!json_str)
        return false;
    memcpy(json_str, json_start, json_len);
    json_str[json_len] = '\0';

    cJSON* root = cJSON_Parse(json_str);
    free(json_str);
    if (!root)
        return false;

    bool success = false;

    cJSON* agent_obj = cJSON_GetObjectItem(root, "agent");
    if (cJSON_IsString(agent_obj) && agent_obj->valuestring) {
        strncpy(agent_out, agent_obj->valuestring, agent_size - 1);
        agent_out[agent_size - 1] = '\0';
        success = true;
    }

    cJSON* conf_obj = cJSON_GetObjectItem(root, "confidence");
    if (cJSON_IsNumber(conf_obj)) {
        *confidence_out = (float)conf_obj->valuedouble;
    }

    cJSON* intent_obj = cJSON_GetObjectItem(root, "intent");
    if (cJSON_IsString(intent_obj) && intent_obj->valuestring && intent_out) {
        strncpy(intent_out, intent_obj->valuestring, intent_size - 1);
        intent_out[intent_size - 1] = '\0';
    }

    cJSON_Delete(root);
    return success;
}

// Main routing function
RouterResult intent_router_route(const char* user_input) {
    RouterResult result = {.agent = "ali",
                           .confidence = 0.5f,
                           .intent = "general",
                           .used_llm = false,
                           .type = INTENT_MESSAGE};

    if (!user_input || !user_input[0]) {
        return result;
    }

    // 0. Check for "switch to agent" intent FIRST (highest priority)
    const char* switch_agent = check_switch_intent(user_input);
    if (switch_agent) {
        LOG_INFO(LOG_CAT_AGENT, "Router: switch intent detected -> %s", switch_agent);
        strncpy(result.agent, switch_agent, sizeof(result.agent) - 1);
        result.confidence = 0.95f;
        strncpy(result.intent, "switch_to_agent", sizeof(result.intent) - 1);
        result.type = INTENT_SWITCH;
        return result;
    }

    // 1. Check cache first
    float cached_conf;
    const char* cached = cache_lookup(user_input, &cached_conf);
    if (cached) {
        LOG_DEBUG(LOG_CAT_AGENT, "Router cache hit: %s (%.2f)", cached, (double)cached_conf);
        strncpy(result.agent, cached, sizeof(result.agent) - 1);
        result.confidence = cached_conf;
        return result;
    }

    // 2. Try quick pattern matching (no LLM needed)
    bool is_delegation = false;
    const char* quick = quick_pattern_route(user_input, &is_delegation);
    if (quick) {
        LOG_DEBUG(LOG_CAT_AGENT, "Router pattern match: %s (delegation=%d)", quick, is_delegation);
        strncpy(result.agent, quick, sizeof(result.agent) - 1);
        result.confidence = 0.85f;
        if (is_delegation) {
            result.type = INTENT_DELEGATE;
            strncpy(result.intent, "delegation_request", sizeof(result.intent) - 1);
            LOG_INFO(LOG_CAT_AGENT, "Router: INTENT_DELEGATE set for delegation request");
        }
        cache_add(user_input, quick, 0.85f);
        return result;
    }

    // 3. Need LLM for complex routing
    LOG_DEBUG(LOG_CAT_AGENT, "Router using LLM for: %.50s...", user_input);

    // Ensure provider registry is initialized
    provider_registry_init();

    // Try providers in order: Anthropic (Haiku - best quality) -> OpenAI -> Gemini -> Local
    Provider* provider = NULL;
    const char* model = NULL;

    provider = provider_get(PROVIDER_ANTHROPIC);
    if (provider && provider->chat) {
        model = ROUTER_MODEL_ANTHROPIC;
    } else {
        provider = provider_get(PROVIDER_OPENAI);
        if (provider && provider->chat) {
            model = ROUTER_MODEL_OPENAI;
        } else {
            provider = provider_get(PROVIDER_GEMINI);
            if (provider && provider->chat) {
                model = ROUTER_MODEL_GEMINI;
            } else {
                provider = provider_get(PROVIDER_OLLAMA);
                if (provider && provider->chat) {
                    model = ROUTER_MODEL_LOCAL;
                }
            }
        }
    }

    if (!provider || !provider->chat) {
        LOG_WARN(LOG_CAT_AGENT, "No router provider available, defaulting to Ali");
        return result;
    }

    // Call LLM
    TokenUsage usage = {0};
    char* response = provider->chat(provider, model, ROUTER_SYSTEM_PROMPT, user_input, &usage);

    if (response) {
        result.used_llm = true;
        char agent[64] = {0};
        char intent[128] = {0};
        float confidence = 0.5f;

        if (parse_route_response(response, agent, sizeof(agent), &confidence, intent,
                                 sizeof(intent))) {
            strncpy(result.agent, agent, sizeof(result.agent) - 1);
            result.confidence = confidence;
            strncpy(result.intent, intent, sizeof(result.intent) - 1);

            LOG_INFO(LOG_CAT_AGENT, "Router LLM: %s (%.2f) - %s", agent, (double)confidence,
                     intent);
            cache_add(user_input, agent, confidence);
        }

        free(response);
    }

    return result;
}

// Cleanup
void intent_router_cleanup(void) {
    for (size_t i = 0; i < ROUTE_CACHE_SIZE; i++) {
        free(g_route_cache[i].input_hash);
        free(g_route_cache[i].agent);
        g_route_cache[i].input_hash = NULL;
        g_route_cache[i].agent = NULL;
    }
}
