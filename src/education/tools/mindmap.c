/**
 * CONVERGIO EDUCATION - MIND MAP GENERATOR
 *
 * Generates Mermaid.js mind maps from topic text using LLM,
 * with export to SVG, PNG, and PDF formats.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/education.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// ============================================================================
// MERMAID TEMPLATES
// ============================================================================

static const char* MINDMAP_TEMPLATE =
    "mindmap\n"
    "  root((%s))\n"
    "%s";

static const char* FLOWCHART_TEMPLATE =
    "flowchart TD\n"
    "%s";

// ============================================================================
// ACCESSIBILITY ADAPTATIONS
// ============================================================================

typedef struct {
    bool high_contrast;
    bool large_font;
    bool simplified;
    const char* color_scheme;
} MindmapAccessibility;

static MindmapAccessibility get_accessibility_settings(const EducationAccessibility* a) {
    MindmapAccessibility ma = {
        .high_contrast = false,
        .large_font = false,
        .simplified = false,
        .color_scheme = "default"
    };

    if (!a) return ma;

    // Dyslexia: larger font, simpler structure
    if (a->dyslexia) {
        ma.large_font = true;
        ma.simplified = a->dyslexia_severity >= SEVERITY_MODERATE;
    }

    // ADHD: fewer branches, clear hierarchy
    if (a->adhd) {
        ma.simplified = true;
    }

    // High contrast for various conditions
    if (a->high_contrast) {
        ma.high_contrast = true;
        ma.color_scheme = "high_contrast";
    }

    return ma;
}

// ============================================================================
// MERMAID GENERATION
// ============================================================================

/**
 * Generate Mermaid mindmap syntax from structured content
 */
char* mindmap_generate_mermaid(const char* topic, const char* content,
                                const MindmapAccessibility* access) {
    if (!topic || !content) return NULL;

    // Build the mindmap body
    // In real implementation, this would parse structured content
    // For now, format content as branches

    size_t buf_size = strlen(content) + strlen(topic) + 1024;
    char* mermaid = malloc(buf_size);
    if (!mermaid) return NULL;

    // Build indented branch structure
    char branches[4096] = {0};
    const char* line = content;
    int branch_count = 0;
    const int max_branches = access && access->simplified ? 5 : 10;

    while (*line && branch_count < max_branches) {
        // Skip whitespace
        while (*line == ' ' || *line == '\t') line++;

        // Find end of line
        const char* end = strchr(line, '\n');
        if (!end) end = line + strlen(line);

        size_t len = end - line;
        if (len > 0 && len < 200) {
            char branch[256];
            snprintf(branch, sizeof(branch), "    %.*s\n", (int)len, line);
            strncat(branches, branch, sizeof(branches) - strlen(branches) - 1);
            branch_count++;
        }

        line = *end ? end + 1 : end;
    }

    // Format final mermaid
    snprintf(mermaid, buf_size, MINDMAP_TEMPLATE, topic, branches);

    return mermaid;
}

/**
 * Generate flowchart for cause-effect or process diagrams
 */
char* mindmap_generate_flowchart(const char* title, const char** steps, int step_count) {
    if (!title || !steps || step_count < 2) return NULL;

    size_t buf_size = 4096;
    char* mermaid = malloc(buf_size);
    if (!mermaid) return NULL;

    char body[3072] = {0};
    char node_id = 'A';

    for (int i = 0; i < step_count && node_id <= 'Z'; i++) {
        char line[256];

        if (i == 0) {
            // First node
            snprintf(line, sizeof(line), "    %c[%s]\n", node_id, steps[i]);
        } else {
            // Arrow from previous to this
            char prev = node_id - 1;
            snprintf(line, sizeof(line), "    %c --> %c[%s]\n", prev, node_id, steps[i]);
        }

        strncat(body, line, sizeof(body) - strlen(body) - 1);
        node_id++;
    }

    snprintf(mermaid, buf_size, FLOWCHART_TEMPLATE, body);
    return mermaid;
}

// ============================================================================
// EXPORT FUNCTIONS
// ============================================================================

/**
 * Export Mermaid to SVG using mmdc (mermaid-cli)
 */
int mindmap_export_svg(const char* mermaid_content, const char* output_path) {
    if (!mermaid_content || !output_path) return -1;

    // Write mermaid content to temp file
    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "/tmp/convergio_mindmap_%d.mmd", getpid());

    FILE* f = fopen(temp_path, "w");
    if (!f) return -1;
    fprintf(f, "%s", mermaid_content);
    fclose(f);

    // Call mmdc to convert
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "mmdc -i %s -o %s -b transparent 2>/dev/null",
             temp_path, output_path);

    int result = system(cmd);

    // Cleanup
    unlink(temp_path);

    return (result == 0) ? 0 : -1;
}

/**
 * Export Mermaid to PNG
 */
int mindmap_export_png(const char* mermaid_content, const char* output_path,
                       int width, int height) {
    if (!mermaid_content || !output_path) return -1;

    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "/tmp/convergio_mindmap_%d.mmd", getpid());

    FILE* f = fopen(temp_path, "w");
    if (!f) return -1;
    fprintf(f, "%s", mermaid_content);
    fclose(f);

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "mmdc -i %s -o %s -w %d -H %d 2>/dev/null",
             temp_path, output_path,
             width > 0 ? width : 1200,
             height > 0 ? height : 800);

    int result = system(cmd);
    unlink(temp_path);

    return (result == 0) ? 0 : -1;
}

/**
 * Export Mermaid to PDF (via SVG conversion)
 */
int mindmap_export_pdf(const char* mermaid_content, const char* output_path) {
    if (!mermaid_content || !output_path) return -1;

    // First export to SVG
    char svg_path[256];
    snprintf(svg_path, sizeof(svg_path), "/tmp/convergio_mindmap_%d.svg", getpid());

    if (mindmap_export_svg(mermaid_content, svg_path) != 0) {
        return -1;
    }

    // Convert SVG to PDF using rsvg-convert or similar
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "rsvg-convert -f pdf -o %s %s 2>/dev/null || "
             "convert %s %s 2>/dev/null",
             output_path, svg_path,
             svg_path, output_path);

    int result = system(cmd);
    unlink(svg_path);

    return (result == 0) ? 0 : -1;
}

// ============================================================================
// LLM INTEGRATION
// ============================================================================

/**
 * Prompt template for LLM to generate mindmap structure
 */
static const char* MINDMAP_PROMPT_TEMPLATE =
    "Generate a mind map structure for the topic: %s\n\n"
    "Context/Content:\n%s\n\n"
    "Requirements:\n"
    "- Create %d main branches maximum\n"
    "- Each branch can have up to %d sub-branches\n"
    "- Use clear, concise labels\n"
    "- Format as Mermaid mindmap syntax\n"
    "%s"
    "\nOutput only the Mermaid code, no explanation.";

/**
 * Generate mindmap using LLM
 * Returns Mermaid syntax string (caller must free)
 */
char* mindmap_generate_from_llm(const char* topic, const char* content,
                                 const EducationAccessibility* access) {
    // This would call the LLM provider
    // For now, return a simple placeholder

    MindmapAccessibility ma = get_accessibility_settings(access);

    int max_branches = ma.simplified ? 4 : 7;
    int max_sub = ma.simplified ? 2 : 4;

    // Build accessibility requirements for prompt
    char access_req[256] = "";
    size_t access_req_remaining = sizeof(access_req) - 1;
    if (ma.simplified) {
        strncat(access_req, "- Keep structure very simple and clear\n", access_req_remaining);
        access_req_remaining = sizeof(access_req) - strlen(access_req) - 1;
    }
    if (ma.large_font) {
        strncat(access_req, "- Use short labels (max 4 words)\n", access_req_remaining);
    }

    // Build full prompt
    size_t prompt_size = strlen(MINDMAP_PROMPT_TEMPLATE) + strlen(topic) +
                         strlen(content) + strlen(access_req) + 100;
    char* prompt = malloc(prompt_size);
    if (!prompt) return NULL;

    snprintf(prompt, prompt_size, MINDMAP_PROMPT_TEMPLATE,
             topic, content, max_branches, max_sub, access_req);

    // TODO: Call LLM provider here
    // For now, generate a simple structure

    char* result = mindmap_generate_mermaid(topic, content, &ma);

    free(prompt);
    return result;
}

// ============================================================================
// CLI COMMAND HANDLER
// ============================================================================

/**
 * Handle /mindmap command
 * Usage: /mindmap <topic> [--format svg|png|pdf] [--output path]
 */
int mindmap_command_handler(int argc, char** argv,
                            const EducationStudentProfile* profile) {
    if (argc < 2) {
        printf("Usage: /mindmap <topic> [--format svg|png|pdf] [--output path]\n");
        return 1;
    }

    const char* topic = argv[1];
    const char* format = "svg";
    const char* output = NULL;

    // Parse options
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--format") == 0 && i + 1 < argc) {
            format = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output = argv[++i];
        }
    }

    // Generate mindmap
    printf("Generating mind map for: %s\n", topic);

    const EducationAccessibility* access = profile ? profile->accessibility : NULL;

    char* mermaid = mindmap_generate_from_llm(topic,
        "Generate appropriate content for the topic", access);

    if (!mermaid) {
        fprintf(stderr, "Failed to generate mind map\n");
        return 1;
    }

    // Determine output path
    char default_output[256];
    if (!output) {
        snprintf(default_output, sizeof(default_output),
                 "mindmap_%s.%s", topic, format);
        // Replace spaces with underscores
        for (char* p = default_output; *p; p++) {
            if (*p == ' ') *p = '_';
        }
        output = default_output;
    }

    // Export
    int result;
    if (strcmp(format, "svg") == 0) {
        result = mindmap_export_svg(mermaid, output);
    } else if (strcmp(format, "png") == 0) {
        result = mindmap_export_png(mermaid, output, 1200, 800);
    } else if (strcmp(format, "pdf") == 0) {
        result = mindmap_export_pdf(mermaid, output);
    } else {
        // Just output the mermaid code
        printf("\n%s\n", mermaid);
        result = 0;
    }

    if (result == 0) {
        printf("Mind map saved to: %s\n", output);
    } else {
        fprintf(stderr, "Failed to export mind map\n");
    }

    free(mermaid);
    return result;
}
