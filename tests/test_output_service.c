/**
 * CONVERGIO OUTPUT SERVICE TESTS
 *
 * Unit tests for centralized output document generation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "nous/output_service.h"

// ============================================================================
// TEST MACROS
// ============================================================================

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  Running %s...", #name); \
    fflush(stdout); \
    test_##name(); \
    printf(" OK\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "\n    ASSERT FAILED: %s (line %d)\n", #cond, __LINE__); \
        exit(1); \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_STR_CONTAINS(haystack, needle) ASSERT(strstr((haystack), (needle)) != NULL)

// ============================================================================
// TEST FIXTURES
// ============================================================================

static const char* TEST_OUTPUT_DIR = "build/test_outputs";  // Must be within cwd for safe_path

static void setup(void) {
    // Clean up old test directory
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", TEST_OUTPUT_DIR);
    system(cmd);

    // Initialize service
    OutputError err = output_service_init(TEST_OUTPUT_DIR);
    ASSERT_EQ(err, OUTPUT_OK);
    ASSERT(output_service_is_ready());
}

static void teardown(void) {
    output_service_shutdown();

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", TEST_OUTPUT_DIR);
    system(cmd);
}

static char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    return content;
}

// ============================================================================
// INITIALIZATION TESTS
// ============================================================================

TEST(init_service) {
    ASSERT(output_service_is_ready());

    const char* base = output_service_get_base_path();
    ASSERT(base != NULL);
    ASSERT(strlen(base) > 0);

    // Directory should exist
    struct stat st;
    ASSERT_EQ(stat(base, &st), 0);
    ASSERT(S_ISDIR(st.st_mode));
}

// ============================================================================
// DOCUMENT CREATION TESTS
// ============================================================================

TEST(create_simple_document) {
    OutputRequest req = {
        .title = "Test Document",
        .content = "This is a test document.\n\nWith multiple paragraphs.",
        .format = OUTPUT_FORMAT_MARKDOWN,
        .include_timestamp = true
    };

    OutputResult result;
    OutputError err = output_create(&req, &result);

    ASSERT_EQ(err, OUTPUT_OK);
    ASSERT(result.success);
    ASSERT(strlen(result.filepath) > 0);
    ASSERT(strlen(result.terminal_link) > 0);

    // File should exist
    struct stat st;
    ASSERT_EQ(stat(result.filepath, &st), 0);

    // Content should include title and content
    char* content = read_file(result.filepath);
    ASSERT(content != NULL);
    ASSERT_STR_CONTAINS(content, "# Test Document");
    ASSERT_STR_CONTAINS(content, "This is a test document");
    ASSERT_STR_CONTAINS(content, "Generated:");
    free(content);
}

TEST(create_document_with_agent) {
    OutputRequest req = {
        .title = "Agent Report",
        .content = "Analysis complete.",
        .agent_name = "baccio-architect",
        .format = OUTPUT_FORMAT_MARKDOWN,
        .include_timestamp = true
    };

    OutputResult result;
    OutputError err = output_create(&req, &result);

    ASSERT_EQ(err, OUTPUT_OK);
    ASSERT(result.success);

    char* content = read_file(result.filepath);
    ASSERT(content != NULL);
    ASSERT_STR_CONTAINS(content, "baccio-architect");
    free(content);
}

TEST(create_document_with_project_context) {
    OutputRequest req = {
        .title = "Project Analysis",
        .content = "Project content here.",
        .project_context = "MyProject",
        .format = OUTPUT_FORMAT_MARKDOWN
    };

    OutputResult result;
    OutputError err = output_create(&req, &result);

    ASSERT_EQ(err, OUTPUT_OK);
    ASSERT(result.success);

    // Path should contain project name
    ASSERT_STR_CONTAINS(result.filepath, "myproject");
}

TEST(create_plain_text) {
    OutputRequest req = {
        .title = "Plain Output",
        .content = "Just plain text.",
        .format = OUTPUT_FORMAT_PLAIN
    };

    OutputResult result;
    OutputError err = output_create(&req, &result);

    ASSERT_EQ(err, OUTPUT_OK);
    ASSERT_STR_CONTAINS(result.filepath, ".txt");
}

TEST(create_json_output) {
    OutputRequest req = {
        .title = "JSON Output",
        .content = "{\"key\": \"value\"}",
        .format = OUTPUT_FORMAT_JSON
    };

    OutputResult result;
    OutputError err = output_create(&req, &result);

    ASSERT_EQ(err, OUTPUT_OK);
    ASSERT_STR_CONTAINS(result.filepath, ".json");
}

TEST(append_to_document) {
    OutputRequest req = {
        .title = "Append Test",
        .content = "Initial content.",
        .format = OUTPUT_FORMAT_MARKDOWN
    };

    OutputResult result;
    output_create(&req, &result);

    // Append
    OutputError err = output_append(result.filepath, "\n\n## New Section\n\nAppended content.");
    ASSERT_EQ(err, OUTPUT_OK);

    char* content = read_file(result.filepath);
    ASSERT(content != NULL);
    ASSERT_STR_CONTAINS(content, "Initial content");
    ASSERT_STR_CONTAINS(content, "New Section");
    ASSERT_STR_CONTAINS(content, "Appended content");
    free(content);
}

// ============================================================================
// TEMPLATE TESTS
// ============================================================================

TEST(create_from_template_analysis) {
    OutputResult result;
    OutputError err = output_from_template("analysis", "Security Analysis", NULL, &result);

    ASSERT_EQ(err, OUTPUT_OK);
    ASSERT(result.success);

    char* content = read_file(result.filepath);
    ASSERT(content != NULL);
    ASSERT_STR_CONTAINS(content, "Executive Summary");
    ASSERT_STR_CONTAINS(content, "Key Findings");
    ASSERT_STR_CONTAINS(content, "Recommendations");
    free(content);
}

TEST(create_from_template_architecture) {
    OutputResult result;
    OutputError err = output_from_template("architecture", "System Architecture", NULL, &result);

    ASSERT_EQ(err, OUTPUT_OK);

    char* content = read_file(result.filepath);
    ASSERT(content != NULL);
    ASSERT_STR_CONTAINS(content, "Overview");
    ASSERT_STR_CONTAINS(content, "Components");
    ASSERT_STR_CONTAINS(content, "mermaid");
    ASSERT_STR_CONTAINS(content, "flowchart");
    free(content);
}

// ============================================================================
// MERMAID TESTS
// ============================================================================

TEST(mermaid_block) {
    MermaidDiagram diagram = {
        .type = MERMAID_FLOWCHART,
        .title = "Test Flow",
        .content = "A --> B\nB --> C"
    };

    char* block = output_mermaid_block(&diagram);
    ASSERT(block != NULL);
    ASSERT_STR_CONTAINS(block, "```mermaid");
    ASSERT_STR_CONTAINS(block, "flowchart TD");
    ASSERT_STR_CONTAINS(block, "A --> B");
    ASSERT_STR_CONTAINS(block, "```");
    free(block);
}

TEST(mermaid_flowchart) {
    const char* nodes[] = {"A[Start]", "B{Decision}", "C[End]", NULL};
    const char* edges[] = {"A --> B", "B -->|Yes| C", "B -->|No| A", NULL};

    char* diagram = output_mermaid_flowchart("My Flowchart", "TD", nodes, edges);
    ASSERT(diagram != NULL);
    ASSERT_STR_CONTAINS(diagram, "flowchart TD");
    ASSERT_STR_CONTAINS(diagram, "A[Start]");
    ASSERT_STR_CONTAINS(diagram, "B{Decision}");
    ASSERT_STR_CONTAINS(diagram, "A --> B");
    free(diagram);
}

TEST(mermaid_sequence) {
    const char* participants[] = {"Client", "Server", "Database", NULL};
    const char* messages[] = {
        "Client->>Server: Request",
        "Server->>Database: Query",
        "Database-->>Server: Results",
        "Server-->>Client: Response",
        NULL
    };

    char* diagram = output_mermaid_sequence("API Flow", participants, messages);
    ASSERT(diagram != NULL);
    ASSERT_STR_CONTAINS(diagram, "sequenceDiagram");
    ASSERT_STR_CONTAINS(diagram, "participant Client");
    ASSERT_STR_CONTAINS(diagram, "Client->>Server");
    free(diagram);
}

TEST(mermaid_gantt) {
    const char* tasks[] = {
        "Design :done, design, 2024-01-01, 7d",
        "Development :active, dev, after design, 14d",
        "Testing :test, after dev, 7d",
        NULL
    };

    char* diagram = output_mermaid_gantt("Project Timeline", NULL, tasks);
    ASSERT(diagram != NULL);
    ASSERT_STR_CONTAINS(diagram, "gantt");
    ASSERT_STR_CONTAINS(diagram, "Project Timeline");
    ASSERT_STR_CONTAINS(diagram, "Design");
    free(diagram);
}

TEST(mermaid_pie) {
    const char* labels[] = {"Category A", "Category B", "Category C"};
    const char* values[] = {"30", "45", "25"};

    char* diagram = output_mermaid_pie("Distribution", labels, values, 3);
    ASSERT(diagram != NULL);
    ASSERT_STR_CONTAINS(diagram, "pie");
    ASSERT_STR_CONTAINS(diagram, "Category A");
    ASSERT_STR_CONTAINS(diagram, "30");
    free(diagram);
}

TEST(mermaid_mindmap) {
    char* diagram = output_mermaid_mindmap("Central Topic",
        "    Branch1\n"
        "      Leaf1\n"
        "      Leaf2\n"
        "    Branch2\n"
        "      Leaf3\n");

    ASSERT(diagram != NULL);
    ASSERT_STR_CONTAINS(diagram, "mindmap");
    ASSERT_STR_CONTAINS(diagram, "Central Topic");
    ASSERT_STR_CONTAINS(diagram, "Branch1");
    free(diagram);
}

// ============================================================================
// TABLE TESTS
// ============================================================================

TEST(generate_simple_table) {
    const char* headers[] = {"Name", "Age", "City"};
    const char* row1[] = {"Alice", "30", "NYC"};
    const char* row2[] = {"Bob", "25", "LA"};
    const char** rows[] = {row1, row2, NULL};

    char* table = output_table_simple(headers, 3, (const char***)rows, 2);
    ASSERT(table != NULL);
    ASSERT_STR_CONTAINS(table, "| Name |");
    ASSERT_STR_CONTAINS(table, "| Alice |");
    ASSERT_STR_CONTAINS(table, "| Bob |");
    ASSERT_STR_CONTAINS(table, "---"); // Separator
    free(table);
}

TEST(generate_aligned_table) {
    TableColumn columns[] = {
        {.header = "Left", .align = 'l'},
        {.header = "Center", .align = 'c'},
        {.header = "Right", .align = 'r'}
    };

    const char* row1[] = {"A", "B", "C"};
    const char** rows[] = {row1, NULL};

    char* table = output_table(columns, 3, (const char***)rows, 1);
    ASSERT(table != NULL);
    ASSERT_STR_CONTAINS(table, ":---"); // Left align
    ASSERT_STR_CONTAINS(table, ":---:"); // Center align
    ASSERT_STR_CONTAINS(table, "---:"); // Right align
    free(table);
}

// ============================================================================
// TERMINAL LINK TESTS
// ============================================================================

TEST(get_link) {
    char* link = output_get_link("/tmp/test.md", "Test File");
    ASSERT(link != NULL);
    // Link should contain the path or the label
    // (format depends on terminal support)
    ASSERT(strlen(link) > 0);
    free(link);
}

// ============================================================================
// FILE MANAGEMENT TESTS
// ============================================================================

TEST(get_latest) {
    // Create a document
    OutputRequest req = {
        .title = "Latest Test",
        .content = "Content",
        .format = OUTPUT_FORMAT_MARKDOWN
    };
    OutputResult created;
    output_create(&req, &created);

    // Get latest
    OutputResult latest;
    OutputError err = output_get_latest(&latest);
    ASSERT_EQ(err, OUTPUT_OK);
    ASSERT(latest.success);
    ASSERT(strlen(latest.filepath) > 0);
}

TEST(list_recent) {
    // Create multiple documents
    for (int i = 0; i < 3; i++) {
        char title[32];
        snprintf(title, sizeof(title), "Doc %d", i);
        OutputRequest req = {
            .title = title,
            .content = "Content",
            .format = OUTPUT_FORMAT_MARKDOWN
        };
        OutputResult result;
        output_create(&req, &result);
    }

    // List recent
    char* paths[10] = {0};
    int count = 0;
    OutputError err = output_list_recent(10, paths, &count);
    ASSERT_EQ(err, OUTPUT_OK);
    ASSERT(count >= 1); // At least the date directory

    for (int i = 0; i < count; i++) {
        free(paths[i]);
    }
}

TEST(delete_output) {
    OutputRequest req = {
        .title = "Delete Me",
        .content = "To be deleted",
        .format = OUTPUT_FORMAT_MARKDOWN
    };
    OutputResult result;
    output_create(&req, &result);

    // Verify exists
    struct stat st;
    ASSERT_EQ(stat(result.filepath, &st), 0);

    // Delete
    OutputError err = output_delete(result.filepath);
    ASSERT_EQ(err, OUTPUT_OK);

    // Verify gone
    ASSERT_NE(stat(result.filepath, &st), 0);
}

TEST(get_total_size) {
    OutputRequest req = {
        .title = "Size Test",
        .content = "Some content for size calculation.",
        .format = OUTPUT_FORMAT_MARKDOWN
    };
    OutputResult result;
    output_create(&req, &result);

    size_t size = output_get_total_size();
    ASSERT(size > 0);
}

// ============================================================================
// INTEGRATION TEST
// ============================================================================

TEST(full_document_with_mermaid_and_table) {
    // Create a document with everything
    const char* nodes[] = {"A[Input]", "B[Process]", "C[Output]", NULL};
    const char* edges[] = {"A --> B", "B --> C", NULL};
    char* flowchart = output_mermaid_flowchart("Data Flow", "LR", nodes, edges);

    const char* headers[] = {"Step", "Description", "Status"};
    const char* row1[] = {"1", "Initialize", "Done"};
    const char* row2[] = {"2", "Process", "Active"};
    const char* row3[] = {"3", "Finalize", "Pending"};
    const char** rows[] = {row1, row2, row3, NULL};
    char* table = output_table_simple(headers, 3, (const char***)rows, 3);

    // Build full content
    size_t content_len = strlen(flowchart) + strlen(table) + 256;
    char* content = malloc(content_len);
    snprintf(content, content_len,
        "## Overview\n\n"
        "This document demonstrates all features.\n\n"
        "## Diagram\n\n"
        "```mermaid\n%s```\n\n"
        "## Data\n\n"
        "%s\n"
        "## Conclusion\n\n"
        "Everything works!\n",
        flowchart, table);

    OutputRequest req = {
        .title = "Complete Integration Test",
        .content = content,
        .agent_name = "test-agent",
        .project_context = "IntegrationTests",
        .format = OUTPUT_FORMAT_MARKDOWN,
        .include_timestamp = true
    };

    OutputResult result;
    OutputError err = output_create(&req, &result);

    ASSERT_EQ(err, OUTPUT_OK);
    ASSERT(result.success);

    // Read and verify
    char* file_content = read_file(result.filepath);
    ASSERT(file_content != NULL);
    ASSERT_STR_CONTAINS(file_content, "Complete Integration Test");
    ASSERT_STR_CONTAINS(file_content, "mermaid");
    ASSERT_STR_CONTAINS(file_content, "flowchart");
    ASSERT_STR_CONTAINS(file_content, "| Step |");
    ASSERT_STR_CONTAINS(file_content, "test-agent");

    free(file_content);
    free(content);
    free(flowchart);
    free(table);

    printf("\n    Created: %s", result.filepath);
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    printf("\n=== Convergio Output Service Tests ===\n\n");

    printf("[INITIALIZATION]\n");
    setup();
    RUN_TEST(init_service);
    teardown();

    printf("\n[DOCUMENT CREATION]\n");
    setup();
    RUN_TEST(create_simple_document);
    RUN_TEST(create_document_with_agent);
    RUN_TEST(create_document_with_project_context);
    RUN_TEST(create_plain_text);
    RUN_TEST(create_json_output);
    RUN_TEST(append_to_document);
    teardown();

    printf("\n[TEMPLATES]\n");
    setup();
    RUN_TEST(create_from_template_analysis);
    RUN_TEST(create_from_template_architecture);
    teardown();

    printf("\n[MERMAID DIAGRAMS]\n");
    setup();
    RUN_TEST(mermaid_block);
    RUN_TEST(mermaid_flowchart);
    RUN_TEST(mermaid_sequence);
    RUN_TEST(mermaid_gantt);
    RUN_TEST(mermaid_pie);
    RUN_TEST(mermaid_mindmap);
    teardown();

    printf("\n[TABLES]\n");
    setup();
    RUN_TEST(generate_simple_table);
    RUN_TEST(generate_aligned_table);
    teardown();

    printf("\n[TERMINAL LINKS]\n");
    setup();
    RUN_TEST(get_link);
    teardown();

    printf("\n[FILE MANAGEMENT]\n");
    setup();
    RUN_TEST(get_latest);
    RUN_TEST(list_recent);
    RUN_TEST(delete_output);
    RUN_TEST(get_total_size);
    teardown();

    printf("\n[INTEGRATION]\n");
    setup();
    RUN_TEST(full_document_with_mermaid_and_table);
    teardown();

    printf("\n\n=== All Output Service Tests Passed! ===\n\n");
    return 0;
}
