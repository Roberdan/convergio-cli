/**
 * CONVERGIO OUTPUT SERVICE
 *
 * Centralized service for generating structured output documents.
 * Provides unified API for all agents to create rich Markdown files
 * with Mermaid diagrams, tables, and terminal-friendly links.
 *
 * Features:
 * - Markdown document generation
 * - Mermaid diagram embedding (flowchart, sequence, gantt, etc.)
 * - Table generation from structured data
 * - OSC8 terminal hyperlinks for clickable file paths
 * - Auto-organization by date/project
 * - Cleanup policies for old outputs
 */

#ifndef CONVERGIO_OUTPUT_SERVICE_H
#define CONVERGIO_OUTPUT_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// ============================================================================
// TYPES
// ============================================================================

typedef enum {
    OUTPUT_FORMAT_MARKDOWN,
    OUTPUT_FORMAT_HTML,
    OUTPUT_FORMAT_JSON,
    OUTPUT_FORMAT_PLAIN
} OutputFormat;

typedef enum {
    MERMAID_FLOWCHART,
    MERMAID_SEQUENCE,
    MERMAID_CLASS,
    MERMAID_STATE,
    MERMAID_ER,
    MERMAID_GANTT,
    MERMAID_PIE,
    MERMAID_MINDMAP,
    MERMAID_TIMELINE,
    MERMAID_CUSTOM
} MermaidType;

typedef enum {
    OUTPUT_OK = 0,
    OUTPUT_ERROR_INIT,
    OUTPUT_ERROR_IO,
    OUTPUT_ERROR_INVALID,
    OUTPUT_ERROR_MEMORY,
    OUTPUT_ERROR_PATH
} OutputError;

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Output document request
 */
typedef struct {
    const char* title;           // Document title (required)
    const char* content;         // Main content in Markdown (required)
    const char* agent_name;      // Name of agent creating the output
    const char* project_context; // Optional project name for organization
    OutputFormat format;         // Output format
    bool open_after;             // Open file after creation
    bool include_timestamp;      // Add timestamp header
    bool include_toc;            // Generate table of contents
} OutputRequest;

/**
 * Output result with file path and terminal link
 */
typedef struct {
    bool success;
    char filepath[1024];        // Absolute path to created file
    char terminal_link[2048];   // OSC8 hyperlink for terminal display
    char open_command[256];     // Command to open file manually
    char relative_path[512];    // Path relative to outputs directory
    time_t created_at;
} OutputResult;

/**
 * Table column definition
 */
typedef struct {
    const char* header;         // Column header text
    int width;                  // Min width (0 for auto)
    char align;                 // 'l'=left, 'r'=right, 'c'=center
} TableColumn;

/**
 * Mermaid diagram configuration
 */
typedef struct {
    MermaidType type;
    const char* title;          // Optional diagram title
    const char* content;        // Mermaid syntax content
    const char* theme;          // Optional theme (default, dark, forest, neutral)
} MermaidDiagram;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize the output service
 * Creates output directories if needed
 *
 * @param base_path  Base path for outputs (NULL for default ~/.convergio/outputs)
 * @return           OUTPUT_OK on success
 */
OutputError output_service_init(const char* base_path);

/**
 * Shutdown output service and cleanup resources
 */
void output_service_shutdown(void);

/**
 * Check if service is initialized
 */
bool output_service_is_ready(void);

/**
 * Get the base output directory
 */
const char* output_service_get_base_path(void);

// ============================================================================
// DOCUMENT CREATION
// ============================================================================

/**
 * Create a new output document
 *
 * @param request  Document configuration
 * @param result   Output result (filepath, link, etc.)
 * @return         OUTPUT_OK on success
 */
OutputError output_create(const OutputRequest* request, OutputResult* result);

/**
 * Append content to an existing output document
 *
 * @param filepath  Path to existing document
 * @param content   Content to append
 * @return          OUTPUT_OK on success
 */
OutputError output_append(const char* filepath, const char* content);

/**
 * Create a document with predefined template
 *
 * @param template_name  Template name (e.g., "analysis", "report", "architecture")
 * @param title          Document title
 * @param data           Key-value pairs for template substitution (NULL-terminated)
 * @param result         Output result
 * @return               OUTPUT_OK on success
 */
OutputError output_from_template(const char* template_name, const char* title,
                                  const char** data, OutputResult* result);

// ============================================================================
// MERMAID DIAGRAMS
// ============================================================================

/**
 * Generate Mermaid diagram code block
 *
 * @param diagram  Diagram configuration
 * @return         Allocated Markdown code block, caller must free
 */
char* output_mermaid_block(const MermaidDiagram* diagram);

/**
 * Create a flowchart diagram
 *
 * @param title      Diagram title
 * @param direction  "TB" (top-bottom), "BT", "LR" (left-right), "RL"
 * @param nodes      Node definitions (A[Label], B{Decision}, etc.)
 * @param edges      Edge definitions (A --> B, B -->|Yes| C, etc.)
 * @return           Allocated Mermaid code, caller must free
 */
char* output_mermaid_flowchart(const char* title, const char* direction,
                                const char** nodes, const char** edges);

/**
 * Create a sequence diagram
 *
 * @param title        Diagram title
 * @param participants Array of participant names
 * @param messages     Array of messages ("A->>B: message")
 * @return             Allocated Mermaid code, caller must free
 */
char* output_mermaid_sequence(const char* title, const char** participants,
                               const char** messages);

/**
 * Create a Gantt chart
 *
 * @param title    Chart title
 * @param sections Array of section names
 * @param tasks    Array of task definitions ("Task :done, t1, 2024-01-01, 3d")
 * @return         Allocated Mermaid code, caller must free
 */
char* output_mermaid_gantt(const char* title, const char** sections,
                            const char** tasks);

/**
 * Create a pie chart
 *
 * @param title  Chart title
 * @param labels Array of labels
 * @param values Array of values (as strings, e.g., "30")
 * @param count  Number of items
 * @return       Allocated Mermaid code, caller must free
 */
char* output_mermaid_pie(const char* title, const char** labels,
                          const char** values, int count);

/**
 * Create a mindmap
 *
 * @param root     Root node text
 * @param branches Nested branch definitions (indentation matters)
 * @return         Allocated Mermaid code, caller must free
 */
char* output_mermaid_mindmap(const char* root, const char* branches);

// ============================================================================
// TABLE GENERATION
// ============================================================================

/**
 * Generate a Markdown table
 *
 * @param columns    Column definitions
 * @param col_count  Number of columns
 * @param rows       2D array of cell values (row-major)
 * @param row_count  Number of rows
 * @return           Allocated Markdown table, caller must free
 */
char* output_table(const TableColumn* columns, int col_count,
                   const char*** rows, int row_count);

/**
 * Generate a simple table from string arrays
 *
 * @param headers    Array of header strings
 * @param col_count  Number of columns
 * @param rows       2D array of cell values
 * @param row_count  Number of rows
 * @return           Allocated Markdown table, caller must free
 */
char* output_table_simple(const char** headers, int col_count,
                           const char*** rows, int row_count);

// ============================================================================
// TERMINAL INTEGRATION
// ============================================================================

/**
 * Print a file link to terminal (with OSC8 if supported)
 *
 * @param filepath  Path to file
 * @param label     Display label (NULL uses filename)
 */
void output_print_link(const char* filepath, const char* label);

/**
 * Print multiple links as a list
 *
 * @param filepaths  Array of file paths
 * @param labels     Array of labels (can be NULL)
 * @param count      Number of items
 */
void output_print_links(const char** filepaths, const char** labels, int count);

/**
 * Get the terminal link string for a file
 *
 * @param filepath  Path to file
 * @param label     Display label
 * @return          Allocated link string, caller must free
 */
char* output_get_link(const char* filepath, const char* label);

// ============================================================================
// FILE MANAGEMENT
// ============================================================================

/**
 * Get the latest output file
 *
 * @param result  Output result structure
 * @return        OUTPUT_OK if found, OUTPUT_ERROR_NOT_FOUND otherwise
 */
OutputError output_get_latest(OutputResult* result);

/**
 * List recent outputs
 *
 * @param count       Maximum number of outputs to return
 * @param out_paths   Output array for paths (caller allocates)
 * @param out_count   Actual count returned
 * @return            OUTPUT_OK on success
 */
OutputError output_list_recent(int count, char** out_paths, int* out_count);

/**
 * Delete an output file
 *
 * @param filepath  Path to output file
 * @return          OUTPUT_OK on success
 */
OutputError output_delete(const char* filepath);

/**
 * Cleanup old outputs
 *
 * @param days_old  Delete outputs older than this many days
 * @return          Number of files deleted
 */
int output_cleanup(int days_old);

/**
 * Get total size of output directory
 *
 * @return Size in bytes
 */
size_t output_get_total_size(void);

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

/**
 * Quick output creation with defaults
 */
#define OUTPUT_QUICK(title, content) \
    do { \
        OutputRequest req = { .title = (title), .content = (content), \
                              .format = OUTPUT_FORMAT_MARKDOWN, \
                              .include_timestamp = true }; \
        OutputResult res; \
        output_create(&req, &res); \
        if (res.success) output_print_link(res.filepath, NULL); \
    } while(0)

/**
 * Output with Mermaid diagram
 */
#define OUTPUT_WITH_DIAGRAM(title, content, mermaid_content) \
    do { \
        char* _diagram = output_mermaid_block(&(MermaidDiagram){ \
            .type = MERMAID_FLOWCHART, .content = (mermaid_content) \
        }); \
        size_t _len = strlen(content) + strlen(_diagram) + 4; \
        char* _full = malloc(_len); \
        snprintf(_full, _len, "%s\n\n%s", (content), _diagram); \
        OutputRequest req = { .title = (title), .content = _full, \
                              .format = OUTPUT_FORMAT_MARKDOWN }; \
        OutputResult res; \
        output_create(&req, &res); \
        free(_full); free(_diagram); \
        if (res.success) output_print_link(res.filepath, NULL); \
    } while(0)

#endif // CONVERGIO_OUTPUT_SERVICE_H
