/**
 * CONVERGIO TOOL CALLING
 *
 * Function/tool calling support for LLM providers:
 * - Tool definition schema
 * - Multi-provider tool format conversion
 * - Tool result handling
 * - Tool execution framework
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ============================================================================
// TOOL DEFINITION
// ============================================================================

typedef enum {
    TOOL_PARAM_STRING,
    TOOL_PARAM_NUMBER,
    TOOL_PARAM_INTEGER,
    TOOL_PARAM_BOOLEAN,
    TOOL_PARAM_ARRAY,
    TOOL_PARAM_OBJECT,
} ToolParamType;

typedef struct ToolParameter {
    char* name;
    char* description;
    ToolParamType type;
    bool required;
    char* enum_values;        // JSON array for enum
    char* default_value;      // Default value as string
    struct ToolParameter* next;
} ToolParameter;

typedef struct Tool {
    char* name;
    char* description;
    ToolParameter* parameters;
    char* (*handler)(const char* args_json, void* ctx);
    void* handler_ctx;
    struct Tool* next;
} Tool;

// ============================================================================
// TOOL REGISTRY
// ============================================================================

static Tool* g_tools = NULL;
static size_t g_tool_count = 0;

// ============================================================================
// TOOL PARAMETER HELPERS
// ============================================================================

static const char* param_type_string(ToolParamType type) {
    switch (type) {
        case TOOL_PARAM_STRING:  return "string";
        case TOOL_PARAM_NUMBER:  return "number";
        case TOOL_PARAM_INTEGER: return "integer";
        case TOOL_PARAM_BOOLEAN: return "boolean";
        case TOOL_PARAM_ARRAY:   return "array";
        case TOOL_PARAM_OBJECT:  return "object";
        default:                 return "string";
    }
}

static ToolParameter* param_create(const char* name, const char* description,
                                    ToolParamType type, bool required) {
    ToolParameter* param = calloc(1, sizeof(ToolParameter));
    if (!param) return NULL;

    param->name = name ? strdup(name) : NULL;
    param->description = description ? strdup(description) : NULL;
    param->type = type;
    param->required = required;

    return param;
}

static void param_destroy(ToolParameter* param) {
    if (!param) return;
    free(param->name);
    free(param->description);
    free(param->enum_values);
    free(param->default_value);
    free(param);
}

// ============================================================================
// TOOL MANAGEMENT
// ============================================================================

Tool* tool_create(const char* name, const char* description) {
    Tool* tool = calloc(1, sizeof(Tool));
    if (!tool) return NULL;

    tool->name = name ? strdup(name) : NULL;
    tool->description = description ? strdup(description) : NULL;

    return tool;
}

void tool_destroy(Tool* tool) {
    if (!tool) return;

    // Free parameters
    ToolParameter* param = tool->parameters;
    while (param) {
        ToolParameter* next = param->next;
        param_destroy(param);
        param = next;
    }

    free(tool->name);
    free(tool->description);
    free(tool);
}

void tool_add_parameter(Tool* tool, const char* name, const char* description,
                        ToolParamType type, bool required) {
    if (!tool) return;

    ToolParameter* param = param_create(name, description, type, required);
    if (!param) return;

    // Add to end of list
    if (!tool->parameters) {
        tool->parameters = param;
    } else {
        ToolParameter* last = tool->parameters;
        while (last->next) last = last->next;
        last->next = param;
    }
}

void tool_set_handler(Tool* tool, char* (*handler)(const char*, void*), void* ctx) {
    if (!tool) return;
    tool->handler = handler;
    tool->handler_ctx = ctx;
}

// ============================================================================
// TOOL REGISTRY
// ============================================================================

void tools_register(Tool* tool) {
    if (!tool) return;

    tool->next = g_tools;
    g_tools = tool;
    g_tool_count++;
}

void tools_unregister(const char* name) {
    if (!name) return;

    Tool** current = &g_tools;
    while (*current) {
        if (strcmp((*current)->name, name) == 0) {
            Tool* to_remove = *current;
            *current = to_remove->next;
            tool_destroy(to_remove);
            g_tool_count--;
            return;
        }
        current = &(*current)->next;
    }
}

Tool* tools_find(const char* name) {
    if (!name) return NULL;

    Tool* tool = g_tools;
    while (tool) {
        if (strcmp(tool->name, name) == 0) {
            return tool;
        }
        tool = tool->next;
    }
    return NULL;
}

void tools_clear(void) {
    while (g_tools) {
        Tool* next = g_tools->next;
        tool_destroy(g_tools);
        g_tools = next;
    }
    g_tool_count = 0;
}

// ============================================================================
// TOOL EXECUTION
// ============================================================================

char* tool_execute(const char* name, const char* args_json) {
    Tool* tool = tools_find(name);
    if (!tool) {
        return strdup("{\"error\":\"Tool not found\"}");
    }

    if (!tool->handler) {
        return strdup("{\"error\":\"Tool has no handler\"}");
    }

    return tool->handler(args_json, tool->handler_ctx);
}

// ============================================================================
// JSON SCHEMA GENERATION
// ============================================================================

/**
 * Generate JSON schema for a tool (Anthropic format)
 */
char* tool_to_anthropic_json(Tool* tool) {
    if (!tool) return NULL;

    // Calculate required buffer size
    size_t size = 1024;
    ToolParameter* param = tool->parameters;
    while (param) {
        size += 256;  // Per parameter
        param = param->next;
    }

    char* json = malloc(size);
    if (!json) return NULL;

    int offset = snprintf(json, size,
        "{\"name\":\"%s\",\"description\":\"%s\",\"input_schema\":{\"type\":\"object\",\"properties\":{",
        tool->name, tool->description);

    // Add parameters
    param = tool->parameters;
    bool first = true;
    while (param) {
        if (!first) {
            offset += snprintf(json + offset, size - offset, ",");
        }
        offset += snprintf(json + offset, size - offset,
            "\"%s\":{\"type\":\"%s\",\"description\":\"%s\"}",
            param->name, param_type_string(param->type), param->description);
        first = false;
        param = param->next;
    }

    offset += snprintf(json + offset, size - offset, "},\"required\":[");

    // Add required parameters
    param = tool->parameters;
    first = true;
    while (param) {
        if (param->required) {
            if (!first) {
                offset += snprintf(json + offset, size - offset, ",");
            }
            offset += snprintf(json + offset, size - offset, "\"%s\"", param->name);
            first = false;
        }
        param = param->next;
    }

    snprintf(json + offset, size - offset, "]}}");

    return json;
}

/**
 * Generate JSON schema for a tool (OpenAI format)
 */
char* tool_to_openai_json(Tool* tool) {
    if (!tool) return NULL;

    size_t size = 1024;
    ToolParameter* param = tool->parameters;
    while (param) {
        size += 256;
        param = param->next;
    }

    char* json = malloc(size);
    if (!json) return NULL;

    int offset = snprintf(json, size,
        "{\"type\":\"function\",\"function\":{\"name\":\"%s\",\"description\":\"%s\","
        "\"parameters\":{\"type\":\"object\",\"properties\":{",
        tool->name, tool->description);

    param = tool->parameters;
    bool first = true;
    while (param) {
        if (!first) {
            offset += snprintf(json + offset, size - offset, ",");
        }
        offset += snprintf(json + offset, size - offset,
            "\"%s\":{\"type\":\"%s\",\"description\":\"%s\"}",
            param->name, param_type_string(param->type), param->description);
        first = false;
        param = param->next;
    }

    offset += snprintf(json + offset, size - offset, "},\"required\":[");

    param = tool->parameters;
    first = true;
    while (param) {
        if (param->required) {
            if (!first) {
                offset += snprintf(json + offset, size - offset, ",");
            }
            offset += snprintf(json + offset, size - offset, "\"%s\"", param->name);
            first = false;
        }
        param = param->next;
    }

    snprintf(json + offset, size - offset, "]}}}");

    return json;
}

/**
 * Generate JSON schema for a tool (Gemini format)
 */
char* tool_to_gemini_json(Tool* tool) {
    if (!tool) return NULL;

    size_t size = 1024;
    ToolParameter* param = tool->parameters;
    while (param) {
        size += 256;
        param = param->next;
    }

    char* json = malloc(size);
    if (!json) return NULL;

    // Gemini uses a slightly different format
    int offset = snprintf(json, size,
        "{\"name\":\"%s\",\"description\":\"%s\",\"parameters\":{\"type\":\"OBJECT\",\"properties\":{",
        tool->name, tool->description);

    param = tool->parameters;
    bool first = true;
    while (param) {
        if (!first) {
            offset += snprintf(json + offset, size - offset, ",");
        }

        // Gemini uses uppercase type names
        const char* type_str;
        switch (param->type) {
            case TOOL_PARAM_STRING:  type_str = "STRING"; break;
            case TOOL_PARAM_NUMBER:  type_str = "NUMBER"; break;
            case TOOL_PARAM_INTEGER: type_str = "INTEGER"; break;
            case TOOL_PARAM_BOOLEAN: type_str = "BOOLEAN"; break;
            case TOOL_PARAM_ARRAY:   type_str = "ARRAY"; break;
            case TOOL_PARAM_OBJECT:  type_str = "OBJECT"; break;
            default:                 type_str = "STRING";
        }

        offset += snprintf(json + offset, size - offset,
            "\"%s\":{\"type\":\"%s\",\"description\":\"%s\"}",
            param->name, type_str, param->description);
        first = false;
        param = param->next;
    }

    offset += snprintf(json + offset, size - offset, "},\"required\":[");

    param = tool->parameters;
    first = true;
    while (param) {
        if (param->required) {
            if (!first) {
                offset += snprintf(json + offset, size - offset, ",");
            }
            offset += snprintf(json + offset, size - offset, "\"%s\"", param->name);
            first = false;
        }
        param = param->next;
    }

    snprintf(json + offset, size - offset, "]}}");

    return json;
}

// ============================================================================
// TOOL LIST GENERATION
// ============================================================================

/**
 * Generate tools array for a specific provider
 */
char* tools_to_json(ProviderType provider) {
    if (g_tool_count == 0) return strdup("[]");

    size_t size = 256 + g_tool_count * 1024;
    char* json = malloc(size);
    if (!json) return NULL;

    int offset = snprintf(json, size, "[");

    Tool* tool = g_tools;
    bool first = true;
    while (tool) {
        char* tool_json;
        switch (provider) {
            case PROVIDER_ANTHROPIC:
                tool_json = tool_to_anthropic_json(tool);
                break;
            case PROVIDER_OPENAI:
                tool_json = tool_to_openai_json(tool);
                break;
            case PROVIDER_GEMINI:
                tool_json = tool_to_gemini_json(tool);
                break;
            default:
                tool_json = tool_to_anthropic_json(tool);
        }

        if (tool_json) {
            if (!first) {
                offset += snprintf(json + offset, size - offset, ",");
            }
            offset += snprintf(json + offset, size - offset, "%s", tool_json);
            free(tool_json);
            first = false;
        }

        tool = tool->next;
    }

    snprintf(json + offset, size - offset, "]");

    return json;
}

// ============================================================================
// TOOL CALL PARSING
// ============================================================================

// Using ToolCall from provider.h:
// - tool_name
// - tool_id
// - arguments_json

/**
 * Parse tool calls from Anthropic response
 */
ToolCall* parse_anthropic_tool_calls(const char* response, size_t* count) {
    // Look for "type":"tool_use" blocks
    // Simplified - real implementation would use proper JSON parsing

    *count = 0;
    if (!response) return NULL;

    // Count tool_use occurrences
    const char* ptr = response;
    while ((ptr = strstr(ptr, "\"type\":\"tool_use\"")) != NULL) {
        (*count)++;
        ptr++;
    }

    if (*count == 0) return NULL;

    ToolCall* calls = calloc(*count, sizeof(ToolCall));
    if (!calls) {
        *count = 0;
        return NULL;
    }

    // Parse each tool_use block
    ptr = response;
    for (size_t i = 0; i < *count; i++) {
        ptr = strstr(ptr, "\"type\":\"tool_use\"");
        if (!ptr) break;

        // Find name
        const char* name_start = strstr(ptr, "\"name\":\"");
        if (name_start) {
            name_start += 8;
            const char* name_end = strchr(name_start, '"');
            if (name_end) {
                calls[i].tool_name = strndup(name_start, name_end - name_start);
            }
        }

        // Find id
        const char* id_start = strstr(ptr, "\"id\":\"");
        if (id_start) {
            id_start += 6;
            const char* id_end = strchr(id_start, '"');
            if (id_end) {
                calls[i].tool_id = strndup(id_start, id_end - id_start);
            }
        }

        // Find input
        const char* input_start = strstr(ptr, "\"input\":");
        if (input_start) {
            input_start += 8;
            // Find matching brace
            int depth = 0;
            const char* input_end = input_start;
            while (*input_end) {
                if (*input_end == '{') depth++;
                if (*input_end == '}') {
                    depth--;
                    if (depth == 0) {
                        input_end++;
                        break;
                    }
                }
                input_end++;
            }
            calls[i].arguments_json = strndup(input_start, input_end - input_start);
        }

        ptr++;
    }

    return calls;
}

void free_tool_calls(ToolCall* calls, size_t count) {
    if (!calls) return;

    for (size_t i = 0; i < count; i++) {
        free(calls[i].tool_name);
        free(calls[i].tool_id);
        free(calls[i].arguments_json);
    }
    free(calls);
}

// ============================================================================
// BUILT-IN TOOLS
// ============================================================================

static char* tool_read_file(const char* args_json, void* ctx) {
    (void)ctx;

    // Parse path from args
    const char* path_start = strstr(args_json, "\"path\":\"");
    if (!path_start) return strdup("{\"error\":\"Missing path parameter\"}");

    path_start += 8;
    const char* path_end = strchr(path_start, '"');
    if (!path_end) return strdup("{\"error\":\"Invalid path parameter\"}");

    char* path = strndup(path_start, path_end - path_start);

    FILE* f = fopen(path, "r");
    if (!f) {
        char* error = malloc(256);
        snprintf(error, 256, "{\"error\":\"Cannot open file: %s\"}", path);
        free(path);
        return error;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size > 100000) {  // 100KB limit
        fclose(f);
        free(path);
        return strdup("{\"error\":\"File too large\"}");
    }

    char* content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);
    free(path);

    // Escape for JSON
    size_t json_size = size * 2 + 64;
    char* result = malloc(json_size);
    snprintf(result, json_size, "{\"content\":\"%s\"}", content);  // Should escape properly
    free(content);

    return result;
}

void tools_register_builtins(void) {
    Tool* read_file = tool_create("read_file", "Read contents of a file");
    tool_add_parameter(read_file, "path", "Path to the file to read",
                       TOOL_PARAM_STRING, true);
    tool_set_handler(read_file, tool_read_file, NULL);
    tools_register(read_file);
}
