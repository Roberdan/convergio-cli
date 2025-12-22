/**
 * CONVERGIO - Conversational Config Module
 *
 * Reusable LLM-based configuration gathering through natural conversation.
 * Instead of rigid forms/wizards, uses AI to understand user input naturally
 * and extract structured JSON data at the end.
 *
 * Usage:
 *   ConversationalConfig cfg = {
 *       .persona_name = "Ali",
 *       .persona_prompt = "You are Ali, a friendly assistant...",
 *       .extraction_schema = "{ \"name\": \"string\", \"age\": \"number\" }",
 *       .greeting = "Hello! What's your name?",
 *       .max_turns = 10,
 *       .required_fields = {"name", "age"},
 *       .required_count = 2
 *   };
 *
 *   char* json = conversational_config_run(&cfg);
 *   if (json) {
 *       // Parse and use the extracted data
 *       free(json);
 *   }
 *
 * Copyright (c) 2025 Convergio.io
 */

#ifndef CONVERSATIONAL_CONFIG_H
#define CONVERSATIONAL_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONFIGURATION STRUCTURE
// ============================================================================

#define CC_MAX_REQUIRED_FIELDS 20
#define CC_MAX_FIELD_NAME 64
#define CC_MAX_PROMPT_LENGTH 8192
#define CC_MAX_GREETING_LENGTH 1024

/**
 * Configuration for a conversational config session.
 */
typedef struct {
    // Persona settings
    const char* persona_name;           // Name shown to user (e.g., "Ali")
    const char* persona_prompt;         // System prompt for LLM conversation
    const char* persona_color;          // ANSI color code (e.g., "\033[1;35m")

    // Extraction settings
    const char* extraction_prompt;      // System prompt for JSON extraction
    const char* extraction_schema;      // JSON schema example for extraction

    // Conversation settings
    const char* greeting;               // Initial greeting message
    const char* completion_hint;        // Hint when enough info gathered
    int max_turns;                      // Maximum conversation turns (default: 15)
    int min_turns;                      // Minimum turns before extraction (default: 3)

    // Required fields tracking
    const char* required_fields[CC_MAX_REQUIRED_FIELDS];
    size_t required_count;

    // Callbacks (optional)
    void (*on_turn)(int turn, const char* user_input, const char* response, void* ctx);
    void (*on_field_gathered)(const char* field_name, void* ctx);
    void* callback_context;

    // Fallback (when LLM unavailable)
    bool enable_fallback;               // Enable form-based fallback
    const char* fallback_prompts[CC_MAX_REQUIRED_FIELDS];  // Form prompts per field

} ConversationalConfig;

// ============================================================================
// RESULT STRUCTURE
// ============================================================================

typedef struct {
    char* json;                         // Extracted JSON (caller must free)
    int turns_taken;                    // Number of conversation turns
    bool used_fallback;                 // True if fell back to form mode
    char* error;                        // Error message if failed (caller must free)
} ConversationalResult;

// ============================================================================
// API FUNCTIONS
// ============================================================================

/**
 * Run a conversational config session.
 *
 * @param config Configuration for the session
 * @return Result with extracted JSON or error. Caller must free json and error.
 */
ConversationalResult conversational_config_run(const ConversationalConfig* config);

/**
 * Run conversational config with custom I/O streams.
 * Useful for testing or non-terminal environments.
 *
 * @param config Configuration for the session
 * @param input Input stream (e.g., stdin or test data)
 * @param output Output stream (e.g., stdout or buffer)
 * @return Result with extracted JSON or error
 */
ConversationalResult conversational_config_run_with_io(
    const ConversationalConfig* config,
    FILE* input,
    FILE* output
);

/**
 * Validate extracted JSON against required fields.
 *
 * @param json JSON string to validate
 * @param required_fields Array of required field names
 * @param count Number of required fields
 * @return True if all required fields are present and non-null
 */
bool conversational_config_validate(
    const char* json,
    const char** required_fields,
    size_t count
);

/**
 * Free a ConversationalResult.
 */
void conversational_result_free(ConversationalResult* result);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Create a default config with common settings.
 */
ConversationalConfig conversational_config_default(void);

/**
 * Build extraction prompt from schema.
 *
 * @param schema JSON schema example
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Pointer to buffer on success, NULL on failure
 */
char* conversational_config_build_extraction_prompt(
    const char* schema,
    char* buffer,
    size_t buffer_size
);

// ============================================================================
// PRESET CONFIGS
// ============================================================================

/**
 * Get preset config for user onboarding.
 */
ConversationalConfig conversational_config_preset_onboarding(void);

/**
 * Get preset config for project setup.
 */
ConversationalConfig conversational_config_preset_project(void);

/**
 * Get preset config for preferences gathering.
 */
ConversationalConfig conversational_config_preset_preferences(void);

#ifdef __cplusplus
}
#endif

#endif // CONVERSATIONAL_CONFIG_H
