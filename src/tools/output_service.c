/**
 * CONVERGIO OUTPUT SERVICE
 *
 * Centralized service for generating structured output documents.
 */

#include "nous/output_service.h"
#include "nous/hyperlink.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <uuid/uuid.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_PATH_LEN 1024
#define MAX_FILENAME_LEN 256
#define DEFAULT_OUTPUT_DIR "outputs"

// Safe snprintf position addition (handles int -> size_t conversion)
#define SNPRINTF_ADD(pos, buf, buf_size, ...) do { \
    int _w = snprintf((buf) + (pos), (buf_size) - (pos), __VA_ARGS__); \
    if (_w > 0) (pos) += (size_t)_w; \
} while(0)

// ============================================================================
// GLOBAL STATE
// ============================================================================

static char g_base_path[MAX_PATH_LEN] = {0};
static bool g_initialized = false;

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static void generate_short_id(char* out, size_t len) {
    uuid_t uuid;
    uuid_generate(uuid);
    char full[37];
    uuid_unparse_lower(uuid, full);
    // Take first 8 chars
    strncpy(out, full, len - 1);
    out[len - 1] = '\0';
}

static void get_date_path(char* out, size_t len) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(out, len, "%Y-%m-%d", tm_info);
}

static void get_timestamp_str(char* out, size_t len) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(out, len, "%Y-%m-%d %H:%M:%S", tm_info);
}

static void ensure_directory(const char* path) {
    char tmp[MAX_PATH_LEN];
    char* p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

static char* sanitize_filename(const char* title) {
    if (!title) return strdup("output");

    size_t len = strlen(title);
    char* result = malloc(len + 1);
    size_t j = 0;

    for (size_t i = 0; i < len && j < 50; i++) {
        char c = title[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_') {
            result[j++] = c;
        } else if (c == ' ') {
            result[j++] = '-';
        }
    }
    result[j] = '\0';

    // Lowercase
    for (size_t i = 0; i < j; i++) {
        if (result[i] >= 'A' && result[i] <= 'Z') {
            result[i] = result[i] + ('a' - 'A');
        }
    }

    if (j == 0) {
        free(result);
        return strdup("output");
    }

    return result;
}

static const char* get_format_extension(OutputFormat format) {
    switch (format) {
        case OUTPUT_FORMAT_MARKDOWN: return ".md";
        case OUTPUT_FORMAT_HTML: return ".html";
        case OUTPUT_FORMAT_JSON: return ".json";
        case OUTPUT_FORMAT_PLAIN: return ".txt";
        default: return ".md";
    }
}

static const char* get_mermaid_type_string(MermaidType type) {
    switch (type) {
        case MERMAID_FLOWCHART: return "flowchart TD";
        case MERMAID_SEQUENCE: return "sequenceDiagram";
        case MERMAID_CLASS: return "classDiagram";
        case MERMAID_STATE: return "stateDiagram-v2";
        case MERMAID_ER: return "erDiagram";
        case MERMAID_GANTT: return "gantt";
        case MERMAID_PIE: return "pie";
        case MERMAID_MINDMAP: return "mindmap";
        case MERMAID_TIMELINE: return "timeline";
        case MERMAID_CUSTOM: return "";
        default: return "flowchart TD";
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

OutputError output_service_init(const char* base_path) {
    if (g_initialized) return OUTPUT_OK;

    if (base_path && base_path[0]) {
        snprintf(g_base_path, sizeof(g_base_path), "%s", base_path);
    } else {
        const char* home = getenv("HOME");
        if (!home) home = "/tmp";
        snprintf(g_base_path, sizeof(g_base_path), "%s/.convergio/%s", home, DEFAULT_OUTPUT_DIR);
    }

    // Create base directory
    ensure_directory(g_base_path);

    g_initialized = true;
    return OUTPUT_OK;
}

void output_service_shutdown(void) {
    g_initialized = false;
}

bool output_service_is_ready(void) {
    return g_initialized;
}

const char* output_service_get_base_path(void) {
    return g_base_path;
}

// ============================================================================
// DOCUMENT CREATION
// ============================================================================

OutputError output_create(const OutputRequest* request, OutputResult* result) {
    if (!g_initialized) {
        output_service_init(NULL);
    }

    if (!request || !request->title || !request->content || !result) {
        return OUTPUT_ERROR_INVALID;
    }

    memset(result, 0, sizeof(OutputResult));

    // Build directory path (base/date/project)
    char date_str[16];
    get_date_path(date_str, sizeof(date_str));

    char dir_path[MAX_PATH_LEN];
    if (request->project_context && request->project_context[0]) {
        char* safe_project = sanitize_filename(request->project_context);
        snprintf(dir_path, sizeof(dir_path), "%s/%s/%s", g_base_path, date_str, safe_project);
        free(safe_project);
    } else {
        snprintf(dir_path, sizeof(dir_path), "%s/%s", g_base_path, date_str);
    }

    ensure_directory(dir_path);

    // Build filename
    char* safe_title = sanitize_filename(request->title);
    char short_id[9];
    generate_short_id(short_id, sizeof(short_id));
    const char* ext = get_format_extension(request->format);

    snprintf(result->filepath, sizeof(result->filepath), "%s/%s-%s%s",
             dir_path, safe_title, short_id, ext);

    // Build relative path
    snprintf(result->relative_path, sizeof(result->relative_path), "%s/%s-%s%s",
             date_str, safe_title, short_id, ext);

    free(safe_title);

    // Create file
    int fd = safe_path_open(result->filepath, safe_path_get_cwd_boundary(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    FILE* f = fd >= 0 ? fdopen(fd, "w") : NULL;
    if (!f) {
        return OUTPUT_ERROR_IO;
    }

    // Write header if markdown
    if (request->format == OUTPUT_FORMAT_MARKDOWN) {
        fprintf(f, "# %s\n\n", request->title);

        if (request->include_timestamp) {
            char timestamp[32];
            get_timestamp_str(timestamp, sizeof(timestamp));
            fprintf(f, "_Generated: %s_", timestamp);
            if (request->agent_name) {
                fprintf(f, " | _Agent: %s_", request->agent_name);
            }
            fprintf(f, "\n\n---\n\n");
        }
    }

    // Write content
    fprintf(f, "%s\n", request->content);
    fclose(f);

    // Set result
    result->success = true;
    result->created_at = time(NULL);

    // Generate terminal link
    char* link = hyperlink_file(result->filepath, request->title);
    if (link) {
        snprintf(result->terminal_link, sizeof(result->terminal_link), "%s", link);
        free(link);
    }

    // Open command
    snprintf(result->open_command, sizeof(result->open_command), "open \"%s\"", result->filepath);

    // Open if requested
    if (request->open_after) {
        system(result->open_command);
    }

    return OUTPUT_OK;
}

OutputError output_append(const char* filepath, const char* content) {
    if (!filepath || !content) return OUTPUT_ERROR_INVALID;

    int fd = safe_path_open(filepath, safe_path_get_cwd_boundary(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    FILE* f = fd >= 0 ? fdopen(fd, "a") : NULL;
    if (!f) return OUTPUT_ERROR_IO;

    fprintf(f, "\n%s", content);
    fclose(f);

    return OUTPUT_OK;
}

OutputError output_from_template(const char* template_name, const char* title,
                                  const char** data, OutputResult* result) {
    if (!template_name || !title || !result) return OUTPUT_ERROR_INVALID;

    // Simple templates
    char content[8192];

    if (strcmp(template_name, "analysis") == 0) {
        snprintf(content, sizeof(content),
            "## Executive Summary\n\n"
            "[Summary here]\n\n"
            "## Key Findings\n\n"
            "1. Finding 1\n"
            "2. Finding 2\n"
            "3. Finding 3\n\n"
            "## Recommendations\n\n"
            "- Recommendation 1\n"
            "- Recommendation 2\n\n"
            "## Next Steps\n\n"
            "- [ ] Action item 1\n"
            "- [ ] Action item 2\n");
    } else if (strcmp(template_name, "architecture") == 0) {
        snprintf(content, sizeof(content),
            "## Overview\n\n"
            "[Architecture description]\n\n"
            "## Components\n\n"
            "### Component 1\n\n"
            "Description here.\n\n"
            "### Component 2\n\n"
            "Description here.\n\n"
            "## Data Flow\n\n"
            "```mermaid\n"
            "flowchart LR\n"
            "    A[Input] --> B[Process]\n"
            "    B --> C[Output]\n"
            "```\n\n"
            "## Dependencies\n\n"
            "| Component | Depends On | Purpose |\n"
            "|-----------|------------|----------|\n"
            "| A | B | Description |\n");
    } else if (strcmp(template_name, "report") == 0) {
        snprintf(content, sizeof(content),
            "## Introduction\n\n"
            "[Introduction]\n\n"
            "## Background\n\n"
            "[Background]\n\n"
            "## Methodology\n\n"
            "[Methodology]\n\n"
            "## Results\n\n"
            "[Results]\n\n"
            "## Conclusion\n\n"
            "[Conclusion]\n\n"
            "## References\n\n"
            "1. Reference 1\n"
            "2. Reference 2\n");
    } else {
        snprintf(content, sizeof(content),
            "## Content\n\n"
            "[Add your content here]\n");
    }

    OutputRequest request = {
        .title = title,
        .content = content,
        .format = OUTPUT_FORMAT_MARKDOWN,
        .include_timestamp = true
    };

    return output_create(&request, result);
}

// ============================================================================
// MERMAID DIAGRAMS
// ============================================================================

char* output_mermaid_block(const MermaidDiagram* diagram) {
    if (!diagram || !diagram->content) return NULL;

    const char* type_str = get_mermaid_type_string(diagram->type);

    size_t len = strlen(type_str) + strlen(diagram->content) + 64;
    if (diagram->title) len += strlen(diagram->title) + 20;

    char* result = malloc(len);
    if (!result) return NULL;

    size_t pos = 0;

    // Title comment if provided
    if (diagram->title) {
        SNPRINTF_ADD(pos, result, len, "<!-- %s -->\n", diagram->title);
    }

    SNPRINTF_ADD(pos, result, len, "```mermaid\n");

    if (diagram->type != MERMAID_CUSTOM) {
        SNPRINTF_ADD(pos, result, len, "%s\n", type_str);
    }

    SNPRINTF_ADD(pos, result, len, "%s\n```", diagram->content);

    return result;
}

char* output_mermaid_flowchart(const char* title, const char* direction,
                                const char** nodes, const char** edges) {
    if (!direction) direction = "TD";

    size_t buf_size = 4096;
    char* buf = malloc(buf_size);
    if (!buf) return NULL;

    size_t pos = 0;

    if (title) {
        SNPRINTF_ADD(pos, buf, buf_size, "---\ntitle: %s\n---\n", title);
    }

    SNPRINTF_ADD(pos, buf, buf_size, "flowchart %s\n", direction);

    // Add nodes
    if (nodes) {
        for (int i = 0; nodes[i]; i++) {
            SNPRINTF_ADD(pos, buf, buf_size, "    %s\n", nodes[i]);
        }
    }

    // Add edges
    if (edges) {
        for (int i = 0; edges[i]; i++) {
            SNPRINTF_ADD(pos, buf, buf_size, "    %s\n", edges[i]);
        }
    }

    return buf;
}

char* output_mermaid_sequence(const char* title, const char** participants,
                               const char** messages) {
    size_t buf_size = 4096;
    char* buf = malloc(buf_size);
    if (!buf) return NULL;

    size_t pos = 0;

    if (title) {
        SNPRINTF_ADD(pos, buf, buf_size, "---\ntitle: %s\n---\n", title);
    }

    SNPRINTF_ADD(pos, buf, buf_size, "sequenceDiagram\n");

    // Add participants
    if (participants) {
        for (int i = 0; participants[i]; i++) {
            SNPRINTF_ADD(pos, buf, buf_size, "    participant %s\n", participants[i]);
        }
    }

    // Add messages
    if (messages) {
        for (int i = 0; messages[i]; i++) {
            SNPRINTF_ADD(pos, buf, buf_size, "    %s\n", messages[i]);
        }
    }

    return buf;
}

char* output_mermaid_gantt(const char* title, const char** sections,
                            const char** tasks) {
    size_t buf_size = 4096;
    char* buf = malloc(buf_size);
    if (!buf) return NULL;

    size_t pos = 0;

    SNPRINTF_ADD(pos, buf, buf_size, "gantt\n");

    if (title) {
        SNPRINTF_ADD(pos, buf, buf_size, "    title %s\n", title);
    }

    SNPRINTF_ADD(pos, buf, buf_size, "    dateFormat YYYY-MM-DD\n");

    // Add sections and tasks
    int section_idx = 0;
    if (sections && tasks) {
        for (int i = 0; tasks[i]; i++) {
            // Check if we need a new section
            if (sections[section_idx] && strncmp(tasks[i], "section:", 8) == 0) {
                SNPRINTF_ADD(pos, buf, buf_size, "    section %s\n", sections[section_idx]);
                section_idx++;
            }
            SNPRINTF_ADD(pos, buf, buf_size, "    %s\n", tasks[i]);
        }
    } else if (tasks) {
        for (int i = 0; tasks[i]; i++) {
            SNPRINTF_ADD(pos, buf, buf_size, "    %s\n", tasks[i]);
        }
    }

    return buf;
}

char* output_mermaid_pie(const char* title, const char** labels,
                          const char** values, int count) {
    if (!labels || !values || count <= 0) return NULL;

    size_t buf_size = 2048;
    char* buf = malloc(buf_size);
    if (!buf) return NULL;

    size_t pos = 0;

    SNPRINTF_ADD(pos, buf, buf_size, "pie showData\n");

    if (title) {
        SNPRINTF_ADD(pos, buf, buf_size, "    title %s\n", title);
    }

    for (int i = 0; i < count && labels[i] && values[i]; i++) {
        SNPRINTF_ADD(pos, buf, buf_size, "    \"%s\" : %s\n", labels[i], values[i]);
    }

    return buf;
}

char* output_mermaid_mindmap(const char* root, const char* branches) {
    if (!root) return NULL;

    size_t len = strlen(root) + (branches ? strlen(branches) : 0) + 64;
    char* buf = malloc(len);
    if (!buf) return NULL;

    if (branches) {
        snprintf(buf, len, "mindmap\n  root((%s))\n%s", root, branches);
    } else {
        snprintf(buf, len, "mindmap\n  root((%s))", root);
    }

    return buf;
}

// ============================================================================
// TABLE GENERATION
// ============================================================================

char* output_table(const TableColumn* columns, int col_count,
                   const char*** rows, int row_count) {
    if (!columns || col_count <= 0) return NULL;

    size_t buf_size = 8192;
    char* buf = malloc(buf_size);
    if (!buf) return NULL;

    size_t pos = 0;

    // Header row
    SNPRINTF_ADD(pos, buf, buf_size, "|");
    for (int i = 0; i < col_count; i++) {
        SNPRINTF_ADD(pos, buf, buf_size, " %s |", columns[i].header ? columns[i].header : "");
    }
    SNPRINTF_ADD(pos, buf, buf_size, "\n|");

    // Separator row with alignment
    for (int i = 0; i < col_count; i++) {
        char align_left = '-';
        char align_right = '-';
        if (columns[i].align == 'c') {
            align_left = ':';
            align_right = ':';
        } else if (columns[i].align == 'r') {
            align_right = ':';
        } else {
            align_left = ':';
        }
        SNPRINTF_ADD(pos, buf, buf_size, "%c---%c|", align_left, align_right);
    }
    SNPRINTF_ADD(pos, buf, buf_size, "\n");

    // Data rows
    if (rows) {
        for (int r = 0; r < row_count && rows[r]; r++) {
            SNPRINTF_ADD(pos, buf, buf_size, "|");
            for (int c = 0; c < col_count; c++) {
                const char* cell = (rows[r] && rows[r][c]) ? rows[r][c] : "";
                SNPRINTF_ADD(pos, buf, buf_size, " %s |", cell);
            }
            SNPRINTF_ADD(pos, buf, buf_size, "\n");
        }
    }

    return buf;
}

char* output_table_simple(const char** headers, int col_count,
                           const char*** rows, int row_count) {
    if (!headers || col_count <= 0) return NULL;

    // Create column definitions with defaults
    TableColumn* columns = malloc(sizeof(TableColumn) * (size_t)col_count);
    for (int i = 0; i < col_count; i++) {
        columns[i].header = headers[i];
        columns[i].width = 0;
        columns[i].align = 'l';
    }

    char* result = output_table(columns, col_count, rows, row_count);
    free(columns);
    return result;
}

// ============================================================================
// TERMINAL INTEGRATION
// ============================================================================

void output_print_link(const char* filepath, const char* label) {
    if (!filepath) return;

    char* link = hyperlink_file(filepath, label);
    if (link) {
        printf("\xF0\x9F\x93\x84 %s\n", link); // 📄 emoji
        free(link);
    } else {
        printf("\xF0\x9F\x93\x84 %s\n", filepath);
    }
}

void output_print_links(const char** filepaths, const char** labels, int count) {
    printf("\n");
    for (int i = 0; i < count && filepaths[i]; i++) {
        const char* label = (labels && labels[i]) ? labels[i] : NULL;
        printf("  ");
        output_print_link(filepaths[i], label);
    }
    printf("\n");
}

char* output_get_link(const char* filepath, const char* label) {
    if (!filepath) return NULL;
    return hyperlink_file(filepath, label);
}

// ============================================================================
// FILE MANAGEMENT
// ============================================================================

OutputError output_get_latest(OutputResult* result) {
    if (!g_initialized || !result) return OUTPUT_ERROR_INVALID;

    memset(result, 0, sizeof(OutputResult));

    // Find most recent file in outputs directory
    DIR* dir = opendir(g_base_path);
    if (!dir) return OUTPUT_ERROR_IO;

    time_t latest_time = 0;
    char latest_path[MAX_PATH_LEN] = {0};

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", g_base_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (st.st_mtime > latest_time) {
                latest_time = st.st_mtime;
                strncpy(latest_path, full_path, sizeof(latest_path) - 1);
            }
        }
    }

    closedir(dir);

    if (latest_path[0] == '\0') {
        return OUTPUT_ERROR_PATH;
    }

    strncpy(result->filepath, latest_path, sizeof(result->filepath) - 1);
    result->success = true;
    result->created_at = latest_time;

    char* link = hyperlink_file(result->filepath, NULL);
    if (link) {
        strncpy(result->terminal_link, link, sizeof(result->terminal_link) - 1);
        free(link);
    }

    return OUTPUT_OK;
}

OutputError output_list_recent(int count, char** out_paths, int* out_count) {
    if (!g_initialized || !out_paths || !out_count || count <= 0) {
        return OUTPUT_ERROR_INVALID;
    }

    *out_count = 0;

    // Simple implementation: just list files from base directory
    DIR* dir = opendir(g_base_path);
    if (!dir) return OUTPUT_ERROR_IO;

    struct dirent* entry;
    int found = 0;

    while ((entry = readdir(dir)) != NULL && found < count) {
        if (entry->d_name[0] == '.') continue;

        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", g_base_path, entry->d_name);

        out_paths[found] = strdup(full_path);
        found++;
    }

    closedir(dir);
    *out_count = found;

    return OUTPUT_OK;
}

OutputError output_delete(const char* filepath) {
    if (!filepath) return OUTPUT_ERROR_INVALID;

    if (unlink(filepath) != 0) {
        return OUTPUT_ERROR_IO;
    }

    return OUTPUT_OK;
}

int output_cleanup(int days_old) {
    if (!g_initialized || days_old < 0) return 0;

    time_t cutoff = time(NULL) - (days_old * 24 * 60 * 60);
    int deleted = 0;

    DIR* dir = opendir(g_base_path);
    if (!dir) return 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", g_base_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0 && st.st_mtime < cutoff) {
            if (unlink(full_path) == 0) {
                deleted++;
            }
        }
    }

    closedir(dir);
    return deleted;
}

size_t output_get_total_size(void) {
    if (!g_initialized) return 0;

    size_t total = 0;

    DIR* dir = opendir(g_base_path);
    if (!dir) return 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", g_base_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0 && st.st_size > 0) {
            total += (size_t)st.st_size;
        }
    }

    closedir(dir);
    return total;
}
