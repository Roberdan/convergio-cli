/**
 * CONVERGIO - Conversational Config Module Implementation
 *
 * Reusable LLM-based configuration gathering through natural conversation.
 *
 * Copyright (c) 2025 Convergio.io
 */

#include "nous/conversational_config.h"
#include "nous/nous.h"
#include "nous/orchestrator.h"
#include <cjson/cJSON.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_INPUT_LENGTH 1024
#define MAX_CONVERSATION_LENGTH 32768
#define MAX_RESPONSE_LENGTH 4096
#define DEFAULT_MAX_TURNS 15
#define DEFAULT_MIN_TURNS 3

// ANSI defaults
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_DIM "\033[2m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_DEFAULT_PERSONA "\033[1;36m"

// ============================================================================
// INTERNAL STATE
// ============================================================================

typedef struct {
    char history[MAX_CONVERSATION_LENGTH];
    size_t history_len;
    int turn_count;
    bool fields_gathered[CC_MAX_REQUIRED_FIELDS];
    int fields_gathered_count;
    bool ready_to_extract;
} ConversationState;

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static void append_to_history(ConversationState* state, const char* speaker, const char* message) {
    size_t remaining = MAX_CONVERSATION_LENGTH - state->history_len - 1;
    if (remaining < 100)
        return; // Safety margin

    int written =
        snprintf(state->history + state->history_len, remaining, "%s: %s\n", speaker, message);

    if (written > 0 && (size_t)written < remaining) {
        state->history_len += (size_t)written;
    }
}

static void print_persona(FILE* out, const char* name, const char* color, const char* message) {
    const char* c = color ? color : ANSI_DEFAULT_PERSONA;
    fprintf(out, "\n  %s%s:%s %s\n", c, name, ANSI_RESET, message);
}

static void print_prompt(FILE* out, const char* name) {
    fprintf(out, "\n  %s> %s", ANSI_DIM, ANSI_RESET);
    fflush(out);
}

static char* read_input(FILE* in, char* buffer, size_t size) {
    if (fgets(buffer, (int)size, in) == NULL) {
        return NULL;
    }
    buffer[strcspn(buffer, "\n")] = 0;

    // Trim whitespace
    char* start = buffer;
    while (*start && isspace((unsigned char)*start))
        start++;
    if (start != buffer) {
        memmove(buffer, start, strlen(start) + 1);
    }

    return buffer;
}

static bool is_exit_command(const char* input) {
    return (strcasecmp(input, "esci") == 0 || strcasecmp(input, "exit") == 0 ||
            strcasecmp(input, "quit") == 0 || strcasecmp(input, "q") == 0);
}

static void check_fields_in_history(ConversationState* state, const ConversationalConfig* config) {
    // Simple heuristic: count gathered fields based on keywords in history
    // This is a basic implementation - could be enhanced with LLM analysis

    const char* history = state->history;

    for (size_t i = 0; i < config->required_count && i < CC_MAX_REQUIRED_FIELDS; i++) {
        if (state->fields_gathered[i])
            continue;

        const char* field = config->required_fields[i];
        if (!field)
            continue;

        // Check if field name appears in conversation (basic heuristic)
        if (strcasestr(history, field) != NULL) {
            state->fields_gathered[i] = true;
            state->fields_gathered_count++;

            if (config->on_field_gathered) {
                config->on_field_gathered(field, config->callback_context);
            }
        }
    }

    // Mark ready if we have enough fields and turns
    if (state->fields_gathered_count >= (int)config->required_count / 2 + 1) {
        if (state->turn_count >= (config->min_turns > 0 ? config->min_turns : DEFAULT_MIN_TURNS)) {
            state->ready_to_extract = true;
        }
    }
}

static char* extract_json_from_conversation(const ConversationalConfig* config,
                                            const char* history) {
    if (!llm_is_available()) {
        return NULL;
    }

    // Build extraction prompt
    char prompt[MAX_CONVERSATION_LENGTH + 2048];
    snprintf(prompt, sizeof(prompt),
             "Extract data from this conversation:\n\n%s\n\n"
             "Return ONLY valid JSON matching this schema:\n%s",
             history, config->extraction_schema ? config->extraction_schema : "{}");

    // Call LLM for extraction
    TokenUsage usage = {0};
    char* response = llm_chat(
        config->extraction_prompt
            ? config->extraction_prompt
            : "You are a data extraction assistant. Return only valid JSON, no extra text.",
        prompt, &usage);

    if (!response)
        return NULL;

    // Find JSON in response
    char* json_start = strchr(response, '{');
    char* json_end = strrchr(response, '}');

    if (!json_start || !json_end || json_end <= json_start) {
        free(response);
        return NULL;
    }

    // Extract just the JSON
    size_t json_len = (size_t)(json_end - json_start + 1);
    char* json = malloc(json_len + 1);
    if (!json) {
        free(response);
        return NULL;
    }

    memcpy(json, json_start, json_len);
    json[json_len] = '\0';

    free(response);
    return json;
}

// ============================================================================
// FALLBACK FORM MODE
// ============================================================================

static ConversationalResult run_fallback_form(const ConversationalConfig* config, FILE* input,
                                              FILE* output) {
    ConversationalResult result = {0};
    result.used_fallback = true;

    cJSON* root = cJSON_CreateObject();
    if (!root) {
        result.error = strdup("Failed to create JSON object");
        return result;
    }

    fprintf(output, "\n  %s[Modalità form - LLM non disponibile]%s\n\n", ANSI_YELLOW, ANSI_RESET);

    char buffer[MAX_INPUT_LENGTH];

    for (size_t i = 0; i < config->required_count && i < CC_MAX_REQUIRED_FIELDS; i++) {
        const char* field = config->required_fields[i];
        const char* prompt = (config->fallback_prompts[i]) ? config->fallback_prompts[i] : field;

        fprintf(output, "  %s: ", prompt);
        fflush(output);

        if (!read_input(input, buffer, sizeof(buffer))) {
            break;
        }

        if (is_exit_command(buffer)) {
            cJSON_Delete(root);
            result.error = strdup("User cancelled");
            return result;
        }

        cJSON_AddStringToObject(root, field, buffer);
        result.turns_taken++;
    }

    result.json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return result;
}

// ============================================================================
// MAIN CONVERSATION LOOP
// ============================================================================

static ConversationalResult run_conversation(const ConversationalConfig* config, FILE* input,
                                             FILE* output) {
    ConversationalResult result = {0};
    ConversationState state = {0};
    char user_input[MAX_INPUT_LENGTH];
    char response[MAX_RESPONSE_LENGTH];

    int max_turns = config->max_turns > 0 ? config->max_turns : DEFAULT_MAX_TURNS;
    const char* persona = config->persona_name ? config->persona_name : "Assistant";
    const char* color = config->persona_color;

    // Print greeting
    if (config->greeting) {
        print_persona(output, persona, color, config->greeting);
        append_to_history(&state, persona, config->greeting);
    }

    // Main loop
    while (state.turn_count < max_turns) {
        print_prompt(output, persona);

        if (!read_input(input, user_input, sizeof(user_input))) {
            break;
        }

        if (strlen(user_input) == 0)
            continue;

        if (is_exit_command(user_input)) {
            print_persona(output, persona, color, "Va bene, ci vediamo! Torna quando vuoi.");
            result.error = strdup("User cancelled");
            return result;
        }

        // Add to history
        append_to_history(&state, "User", user_input);
        state.turn_count++;

        // Check gathered fields
        check_fields_in_history(&state, config);

        // Build conversation prompt
        char prompt[MAX_CONVERSATION_LENGTH + 2048];
        snprintf(prompt, sizeof(prompt),
                 "CONVERSATION:\n%s\n\n"
                 "TURN: %d/%d\n"
                 "FIELDS GATHERED: %d/%zu\n\n"
                 "Respond naturally. %s",
                 state.history, state.turn_count, max_turns, state.fields_gathered_count,
                 config->required_count,
                 (state.ready_to_extract && config->completion_hint)
                     ? config->completion_hint
                     : "Continue the conversation.");

        // Get LLM response
        TokenUsage usage = {0};
        char* llm_response =
            llm_chat(config->persona_prompt ? config->persona_prompt
                                            : "You are a friendly assistant gathering information "
                                              "through natural conversation.",
                     prompt, &usage);

        if (!llm_response || strlen(llm_response) == 0) {
            strncpy(response, "Sorry, I didn't catch that. Could you repeat?",
                    sizeof(response) - 1);
            if (llm_response)
                free(llm_response);
        } else {
            strncpy(response, llm_response, sizeof(response) - 1);
            response[sizeof(response) - 1] = '\0';
            free(llm_response);
        }

        // Print and record response
        print_persona(output, persona, color, response);
        append_to_history(&state, persona, response);

        // Callback
        if (config->on_turn) {
            config->on_turn(state.turn_count, user_input, response, config->callback_context);
        }

        // Check if ready to extract
        if (state.ready_to_extract && state.turn_count >= DEFAULT_MIN_TURNS) {
            // Check for completion phrases
            if (strstr(response, "profilo") || strstr(response, "profile") ||
                strstr(response, "pronto") || strstr(response, "ready") ||
                strstr(response, "confermi") || strstr(response, "confirm")) {
                break;
            }
        }
    }

    result.turns_taken = state.turn_count;

    // Extraction phase
    fprintf(output, "\n  %sProcessing...%s\n", ANSI_DIM, ANSI_RESET);

    char* json = extract_json_from_conversation(config, state.history);
    if (!json) {
        result.error = strdup("Failed to extract data from conversation");
        return result;
    }

    result.json = json;
    return result;
}

// ============================================================================
// PUBLIC API
// ============================================================================

ConversationalResult conversational_config_run(const ConversationalConfig* config) {
    return conversational_config_run_with_io(config, stdin, stdout);
}

ConversationalResult conversational_config_run_with_io(const ConversationalConfig* config,
                                                       FILE* input, FILE* output) {
    ConversationalResult result = {0};

    if (!config) {
        result.error = strdup("Config is NULL");
        return result;
    }

    // Check LLM availability
    if (!llm_is_available()) {
        if (config->enable_fallback) {
            return run_fallback_form(config, input, output);
        } else {
            result.error = strdup("LLM not available and fallback disabled");
            return result;
        }
    }

    return run_conversation(config, input, output);
}

bool conversational_config_validate(const char* json, const char** required_fields, size_t count) {
    if (!json || !required_fields)
        return false;

    cJSON* root = cJSON_Parse(json);
    if (!root)
        return false;

    bool valid = true;
    for (size_t i = 0; i < count && valid; i++) {
        if (!required_fields[i])
            continue;

        cJSON* item = cJSON_GetObjectItemCaseSensitive(root, required_fields[i]);
        if (!item || cJSON_IsNull(item)) {
            valid = false;
        }
        // Check for empty strings
        if (cJSON_IsString(item) && strlen(item->valuestring) == 0) {
            valid = false;
        }
    }

    cJSON_Delete(root);
    return valid;
}

void conversational_result_free(ConversationalResult* result) {
    if (!result)
        return;
    if (result->json) {
        free(result->json);
        result->json = NULL;
    }
    if (result->error) {
        free(result->error);
        result->error = NULL;
    }
}

ConversationalConfig conversational_config_default(void) {
    ConversationalConfig config = {0};
    config.max_turns = DEFAULT_MAX_TURNS;
    config.min_turns = DEFAULT_MIN_TURNS;
    config.enable_fallback = true;
    return config;
}

char* conversational_config_build_extraction_prompt(const char* schema, char* buffer,
                                                    size_t buffer_size) {
    if (!schema || !buffer || buffer_size < 256)
        return NULL;

    snprintf(buffer, buffer_size,
             "You are a data extraction assistant.\n"
             "Extract information from conversations and return ONLY valid JSON.\n"
             "Use this schema as reference:\n%s\n"
             "Return null for missing fields. No explanations, just JSON.",
             schema);

    return buffer;
}

// ============================================================================
// PRESET CONFIGURATIONS
// ============================================================================

ConversationalConfig conversational_config_preset_onboarding(void) {
    ConversationalConfig config = conversational_config_default();

    config.persona_name = "Convergio";
    config.persona_color = "\033[1;36m";
    config.greeting = "Welcome! I'd like to get to know you better. What's your name?";
    config.completion_hint = "If you have enough info, offer to save the profile.";

    config.persona_prompt =
        "You are Convergio, a friendly AI assistant.\n"
        "You're gathering basic info about a new user through natural conversation.\n"
        "Be warm, curious, and never judgmental.\n"
        "Ask ONE question at a time. Don't make lists.";

    config.extraction_schema = "{\n"
                               "  \"name\": \"user's name (string)\",\n"
                               "  \"preferred_language\": \"en|it|es|fr|de\",\n"
                               "  \"role\": \"developer|designer|manager|other\",\n"
                               "  \"experience_level\": \"beginner|intermediate|expert\",\n"
                               "  \"interests\": [\"list\", \"of\", \"interests\"]\n"
                               "}";

    config.required_fields[0] = "name";
    config.required_fields[1] = "role";
    config.required_count = 2;

    config.fallback_prompts[0] = "Your name";
    config.fallback_prompts[1] = "Your role (developer/designer/manager/other)";

    return config;
}

ConversationalConfig conversational_config_preset_project(void) {
    ConversationalConfig config = conversational_config_default();

    config.persona_name = "Convergio";
    config.persona_color = "\033[1;33m";
    config.greeting = "Let's set up your new project! What would you like to build?";
    config.completion_hint = "If project is clear, offer to create the configuration.";

    config.persona_prompt = "You are Convergio, helping set up a new software project.\n"
                            "Understand what the user wants to build.\n"
                            "Ask about technology preferences, team size, timeline.\n"
                            "Be helpful and suggest best practices when appropriate.";

    config.extraction_schema = "{\n"
                               "  \"project_name\": \"name of the project\",\n"
                               "  \"description\": \"brief description\",\n"
                               "  \"type\": \"web|mobile|cli|api|library|other\",\n"
                               "  \"languages\": [\"typescript\", \"python\", etc],\n"
                               "  \"frameworks\": [\"react\", \"fastapi\", etc],\n"
                               "  \"team_size\": number or null\n"
                               "}";

    config.required_fields[0] = "project_name";
    config.required_fields[1] = "type";
    config.required_count = 2;

    config.fallback_prompts[0] = "Project name";
    config.fallback_prompts[1] = "Project type (web/mobile/cli/api/library/other)";

    return config;
}

ConversationalConfig conversational_config_preset_preferences(void) {
    ConversationalConfig config = conversational_config_default();

    config.persona_name = "Convergio";
    config.persona_color = "\033[1;35m";
    config.greeting = "Let's customize your experience! How do you prefer to work?";
    config.max_turns = 10;

    config.persona_prompt = "You are Convergio, helping customize the user experience.\n"
                            "Ask about UI preferences, workflow style, notification preferences.\n"
                            "Be concise - this shouldn't take long.";

    config.extraction_schema = "{\n"
                               "  \"theme\": \"dark|light|auto\",\n"
                               "  \"verbosity\": \"minimal|normal|detailed\",\n"
                               "  \"auto_suggestions\": true|false,\n"
                               "  \"notification_level\": \"all|important|none\"\n"
                               "}";

    config.required_fields[0] = "theme";
    config.required_count = 1;

    config.fallback_prompts[0] = "Theme preference (dark/light/auto)";

    return config;
}
