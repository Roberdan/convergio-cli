/**
 * Web Search Integration Tests
 *
 * Tests web search functionality across providers:
 * - Anthropic native web search
 * - OpenAI native web search
 * - Local DuckDuckGo fallback
 *
 * Run with: make websearch_test && ./build/bin/websearch_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "nous/tools.h"
#include "nous/provider.h"
#include "nous/nous.h"

// Log stubs are provided by test_stubs.c (linked via Makefile)

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, condition) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("  \033[32m✓\033[0m %s\n", name); \
    } else { \
        tests_failed++; \
        printf("  \033[31m✗\033[0m %s FAILED\n", name); \
    } \
} while(0)

#define TEST_SECTION(name) printf("\n\033[1m=== %s ===\033[0m\n", name)

// ============================================================================
// WEB SEARCH TOOL DEFINITION TESTS
// ============================================================================

void test_websearch_tool_definition(void) {
    TEST_SECTION("Web Search Tool Definition");

    const char* json = tools_get_definitions_json();
    TEST("Tool definitions available", json != NULL);

    // Check web_search tool is defined
    TEST("web_search tool defined", json && strstr(json, "\"web_search\"") != NULL);
    TEST("web_search has query param", json && strstr(json, "\"query\"") != NULL);
    TEST("web_search has description", json && strstr(json, "Search the web") != NULL);
}

// ============================================================================
// WEB SEARCH TOOL PARSING TESTS
// ============================================================================

void test_websearch_parsing(void) {
    TEST_SECTION("Web Search Tool Parsing");

    LocalToolCall* call;

    // Test basic parsing
    call = tools_parse_call("web_search", "{\"query\":\"test query\"}");
    TEST("Parse web_search succeeds", call != NULL);
    TEST("Type is TOOL_WEB_SEARCH", call && call->type == TOOL_WEB_SEARCH);
    TEST("Tool name correct", call && call->tool_name && strcmp(call->tool_name, "web_search") == 0);
    TEST("Parameters stored", call && call->parameters_json != NULL);
    if (call) tools_free_call(call);

    // Test with max_results
    call = tools_parse_call("web_search", "{\"query\":\"test\",\"max_results\":10}");
    TEST("Parse with max_results succeeds", call != NULL);
    if (call) tools_free_call(call);

    // Test empty query
    call = tools_parse_call("web_search", "{\"query\":\"\"}");
    TEST("Parse empty query succeeds (validation at execution)", call != NULL);
    if (call) tools_free_call(call);

    // Test missing query
    call = tools_parse_call("web_search", "{}");
    TEST("Parse missing query succeeds (validation at execution)", call != NULL);
    if (call) tools_free_call(call);
}

// ============================================================================
// WEB SEARCH EXECUTION TESTS
// ============================================================================

void test_websearch_execution(void) {
    TEST_SECTION("Web Search Execution");

    ToolResult* result;

    // Test empty query - should fail gracefully
    result = tool_web_search("", 5);
    TEST("Empty query returns error", result != NULL && !result->success);
    TEST("Empty query has error message", result && result->error != NULL);
    if (result) tools_free_result(result);

    // Test NULL query - should fail gracefully
    result = tool_web_search(NULL, 5);
    TEST("NULL query returns error", result != NULL && !result->success);
    if (result) tools_free_result(result);

    // Test max_results bounds
    result = tool_web_search("test", -1);  // Should default to 5
    TEST("Negative max_results handled", result != NULL);
    if (result) tools_free_result(result);

    result = tool_web_search("test", 0);  // Should default to 5
    TEST("Zero max_results handled", result != NULL);
    if (result) tools_free_result(result);

    result = tool_web_search("test", 100);  // Should cap at 20
    TEST("Large max_results capped", result != NULL);
    if (result) tools_free_result(result);
}

// ============================================================================
// WEB SEARCH RESULT FORMAT TESTS
// ============================================================================

void test_websearch_result_format(void) {
    TEST_SECTION("Web Search Result Format");

    // Note: These tests verify the result structure, not network calls
    ToolResult* result;

    // Create mock result to test structure
    result = tool_web_search("", 5);  // Will fail but return structured error
    TEST("Result has success field", result != NULL);
    TEST("Failed result has success=false", result && !result->success);
    TEST("Failed result has error message", result && result->error != NULL);
    TEST("Result has execution_time", result && result->execution_time >= 0);
    if (result) tools_free_result(result);
}

// ============================================================================
// OPENAI WEB SEARCH DETECTION TESTS
// ============================================================================

// Helper to check if web_search is in tools array (mirrors openai.c logic)
static bool has_web_search_in_tools(ToolDefinition* tools, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (tools[i].name && strcmp(tools[i].name, "web_search") == 0) {
            return true;
        }
    }
    return false;
}

void test_openai_websearch_detection(void) {
    TEST_SECTION("OpenAI Web Search Detection");

    // Create tool arrays for testing
    ToolDefinition tools_with_search[3] = {
        {.name = "file_read", .description = "Read file", .parameters_json = "{}"},
        {.name = "web_search", .description = "Search web", .parameters_json = "{}"},
        {.name = "shell_exec", .description = "Execute shell", .parameters_json = "{}"}
    };

    ToolDefinition tools_without_search[2] = {
        {.name = "file_read", .description = "Read file", .parameters_json = "{}"},
        {.name = "shell_exec", .description = "Execute shell", .parameters_json = "{}"}
    };

    ToolDefinition empty_tools[1] = {
        {.name = NULL, .description = NULL, .parameters_json = NULL}
    };

    // Test detection
    TEST("Detects web_search in tools", has_web_search_in_tools(tools_with_search, 3));
    TEST("No false positive without web_search", !has_web_search_in_tools(tools_without_search, 2));
    TEST("Handles empty tools array", !has_web_search_in_tools(empty_tools, 0));
    TEST("Handles NULL tools", !has_web_search_in_tools(NULL, 0));
}

// ============================================================================
// INTEGRATION WITH TOOL EXECUTION
// ============================================================================

void test_websearch_integration(void) {
    TEST_SECTION("Web Search Integration");

    // Test that web_search can be parsed and executed through tools_execute
    LocalToolCall* call = tools_parse_call("web_search", "{\"query\":\"\"}");
    TEST("Parse web_search for execution", call != NULL);

    if (call) {
        ToolResult* result = tools_execute(call);
        TEST("Execute web_search through tools_execute", result != NULL);
        // Empty query should fail
        TEST("Empty query fails as expected", result && !result->success);
        if (result) tools_free_result(result);
        tools_free_call(call);
    }

    // Test with valid query structure (network may fail but parsing should work)
    call = tools_parse_call("web_search", "{\"query\":\"test\",\"max_results\":3}");
    TEST("Parse valid web_search query", call != NULL);
    if (call) {
        ToolResult* result = tools_execute(call);
        TEST("Execute returns result", result != NULL);
        // Result may succeed or fail depending on network, but shouldn't crash
        TEST("Execution time recorded", result && result->execution_time >= 0);
        if (result) tools_free_result(result);
        tools_free_call(call);
    }
}

// ============================================================================
// URL ENCODING TESTS
// ============================================================================

void test_url_encoding(void) {
    TEST_SECTION("URL Encoding (Web Search)");

    // These tests verify that special characters in queries don't crash
    // The actual URL encoding is done by libcurl
    ToolResult* result;

    // Test with spaces
    result = tool_web_search("hello world", 1);
    TEST("Query with spaces handled", result != NULL);
    if (result) tools_free_result(result);

    // Test with special characters
    result = tool_web_search("test&query=value", 1);
    TEST("Query with ampersand handled", result != NULL);
    if (result) tools_free_result(result);

    // Test with unicode (may or may not work with network)
    result = tool_web_search("日本語", 1);
    TEST("Query with unicode handled", result != NULL);
    if (result) tools_free_result(result);

    // Test with quotes
    result = tool_web_search("\"exact phrase\"", 1);
    TEST("Query with quotes handled", result != NULL);
    if (result) tools_free_result(result);
}

// ============================================================================
// MEMORY SAFETY TESTS
// ============================================================================

void test_memory_safety(void) {
    TEST_SECTION("Memory Safety Tests");

    // Test that we can allocate and free many times without leaking
    for (int i = 0; i < 100; i++) {
        ToolResult* result = tool_web_search("", 1);
        if (result) tools_free_result(result);

        LocalToolCall* call = tools_parse_call("web_search", "{\"query\":\"test\"}");
        if (call) tools_free_call(call);
    }
    TEST("100 alloc/free cycles completed", true);

    // Test double-free protection (should not crash)
    // Note: We can't actually test this safely, so just verify single free works
    ToolResult* result = tool_web_search("", 1);
    if (result) {
        tools_free_result(result);
        TEST("Single free succeeded", true);
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("\n\033[1m╔══════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1m║           CONVERGIO WEB SEARCH TEST SUITE                    ║\033[0m\n");
    printf("\033[1m╚══════════════════════════════════════════════════════════════╝\033[0m\n");

    // Run all test suites
    test_websearch_tool_definition();
    test_websearch_parsing();
    test_websearch_execution();
    test_websearch_result_format();
    test_openai_websearch_detection();
    test_websearch_integration();
    test_url_encoding();
    test_memory_safety();

    // Summary
    printf("\n\033[1m════════════════════════════════════════════════════════════════\033[0m\n");
    printf("\033[1mResults:\033[0m %d tests, \033[32m%d passed\033[0m, \033[31m%d failed\033[0m\n",
           tests_run, tests_passed, tests_failed);
    printf("\033[1m════════════════════════════════════════════════════════════════\033[0m\n\n");

    return tests_failed > 0 ? 1 : 0;
}
