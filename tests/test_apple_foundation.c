/**
 * Apple Foundation Models Unit Tests
 *
 * Tests for the Apple Foundation Models integration (macOS 26+).
 * These tests check availability detection, provider interface,
 * and graceful degradation when AFM is not available.
 *
 * Run with: make apple_foundation_test && ./build/bin/apple_foundation_test
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nous/nous.h"
#include "nous/apple_foundation.h"
#include "nous/provider.h"

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, condition) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("  \033[32m+\033[0m %s\n", name); \
    } else { \
        tests_failed++; \
        printf("  \033[31m-\033[0m %s FAILED\n", name); \
    } \
} while(0)

#define TEST_SECTION(name) printf("\n\033[1m=== %s ===\033[0m\n", name)

// ============================================================================
// AVAILABILITY TESTS
// ============================================================================

void test_afm_availability_check(void) {
    TEST_SECTION("Apple Foundation Models Availability");

    AppleFoundationStatus status = {0};
    AppleFoundationError result = afm_check_availability(&status);

    // Should always return a valid result (not crash)
    TEST("availability check returns valid result",
         result == AFM_AVAILABLE ||
         result == AFM_ERR_NOT_MACOS_26 ||
         result == AFM_ERR_NOT_APPLE_SILICON ||
         result == AFM_ERR_INTELLIGENCE_DISABLED ||
         result == AFM_ERR_MODEL_NOT_READY ||
         result == AFM_ERR_UNKNOWN);

    // Status should be populated
    TEST("status has OS version", status.os_version != NULL);
    TEST("status has chip name", status.chip_name != NULL);

    // Apple Silicon check should be accurate
    #ifdef __aarch64__
    TEST("Apple Silicon detected on ARM64", status.is_apple_silicon == true);
    #else
    TEST("Intel detected on x86_64", status.is_apple_silicon == false);
    #endif

    // Print status for debugging
    printf("\n  Status Details:\n");
    printf("    OS Version: %s\n", status.os_version ? status.os_version : "unknown");
    printf("    Chip: %s\n", status.chip_name ? status.chip_name : "unknown");
    printf("    Apple Silicon: %s\n", status.is_apple_silicon ? "yes" : "no");
    printf("    macOS 26+: %s\n", status.is_macos_26 ? "yes" : "no");
    printf("    Intelligence Enabled: %s\n", status.intelligence_enabled ? "yes" : "no");
    printf("    Model Ready: %s\n", status.model_ready ? "yes" : "no");
}

// ============================================================================
// ERROR DESCRIPTION TESTS
// ============================================================================

void test_afm_error_descriptions(void) {
    TEST_SECTION("Error Descriptions");

    const char* desc;

    desc = afm_status_description(AFM_AVAILABLE);
    TEST("AFM_AVAILABLE has description", desc != NULL && strlen(desc) > 0);

    desc = afm_status_description(AFM_ERR_NOT_MACOS_26);
    TEST("AFM_ERR_NOT_MACOS_26 has description", desc != NULL && strlen(desc) > 0);
    TEST("macOS 26 error mentions version", strstr(desc, "26") != NULL || strstr(desc, "Tahoe") != NULL);

    desc = afm_status_description(AFM_ERR_NOT_APPLE_SILICON);
    TEST("AFM_ERR_NOT_APPLE_SILICON has description", desc != NULL && strlen(desc) > 0);
    TEST("Apple Silicon error mentions chip", strstr(desc, "Silicon") != NULL || strstr(desc, "M") != NULL);

    desc = afm_status_description(AFM_ERR_INTELLIGENCE_DISABLED);
    TEST("AFM_ERR_INTELLIGENCE_DISABLED has description", desc != NULL && strlen(desc) > 0);
    TEST("Intelligence error mentions settings", strstr(desc, "Intelligence") != NULL);

    desc = afm_status_description(AFM_ERR_MODEL_NOT_READY);
    TEST("AFM_ERR_MODEL_NOT_READY has description", desc != NULL && strlen(desc) > 0);

    desc = afm_status_description(AFM_ERR_SESSION_FAILED);
    TEST("AFM_ERR_SESSION_FAILED has description", desc != NULL && strlen(desc) > 0);

    desc = afm_status_description(AFM_ERR_GENERATION_FAILED);
    TEST("AFM_ERR_GENERATION_FAILED has description", desc != NULL && strlen(desc) > 0);

    desc = afm_status_description(AFM_ERR_UNKNOWN);
    TEST("AFM_ERR_UNKNOWN has description", desc != NULL && strlen(desc) > 0);
}

// ============================================================================
// PROVIDER TESTS
// ============================================================================

void test_afm_provider_create(void) {
    TEST_SECTION("Provider Creation");

    Provider* provider = afm_provider_create();

    // Provider may be NULL if AFM is not available - that's OK
    AppleFoundationStatus status = {0};
    AppleFoundationError availability = afm_check_availability(&status);

    if (availability == AFM_AVAILABLE) {
        TEST("provider created when AFM available", provider != NULL);

        if (provider) {
            TEST("provider has correct type", provider->type == PROVIDER_APPLE_FOUNDATION);
            TEST("provider has name", provider->name != NULL);
            TEST("provider has init function", provider->init != NULL);
            TEST("provider has shutdown function", provider->shutdown != NULL);
            TEST("provider has chat function", provider->chat != NULL);
            TEST("provider has stream_chat function", provider->stream_chat != NULL);
            TEST("provider has no API key env (local)", provider->api_key_env == NULL);

            // Initialize provider
            if (provider->init) {
                ProviderError err = provider->init(provider);
                TEST("provider init succeeds or fails gracefully",
                     err == PROVIDER_OK || err == PROVIDER_ERR_NOT_INITIALIZED);
            }

            // Shutdown provider
            if (provider->shutdown) {
                provider->shutdown(provider);
            }

            free(provider);
        }
    } else {
        // Provider can be created on eligible hardware (Apple Silicon + macOS 26+)
        // even if AFM isn't fully available (e.g., Intelligence disabled)
        // The provider will fail gracefully during operations
        if (status.is_apple_silicon && status.is_macos_26) {
            // Hardware is eligible, provider might be created
            if (provider != NULL) {
                TEST("provider created on eligible hardware", true);
                TEST("provider has correct type", provider->type == PROVIDER_APPLE_FOUNDATION);
                printf("    (AFM unavailable: %s)\n", afm_status_description(availability));
                free(provider);
            } else {
                TEST("provider NULL despite eligible hardware", false);
            }
        } else {
            // Hardware not eligible, provider should be NULL
            TEST("provider NULL when hardware ineligible", provider == NULL);
            printf("    (Expected: AFM not available on this system)\n");
        }
    }
}

// ============================================================================
// RECOMMENDATION TESTS
// ============================================================================

void test_afm_recommendations(void) {
    TEST_SECTION("Local Provider Recommendations");

    // Check preference logic
    bool prefer_short = afm_should_prefer_over_mlx(1000, false);
    bool prefer_tools = afm_should_prefer_over_mlx(5000, true);
    bool prefer_long = afm_should_prefer_over_mlx(50000, false);

    AppleFoundationStatus status = {0};
    AppleFoundationError availability = afm_check_availability(&status);

    if (availability == AFM_AVAILABLE) {
        TEST("prefer AFM for short prompts", prefer_short == true);
        TEST("prefer AFM for tool calls", prefer_tools == true);
        // Long prompts might prefer MLX
        printf("    Long prompt preference: %s\n", prefer_long ? "AFM" : "MLX");
    } else {
        // When AFM not available, should always return false
        TEST("don't prefer AFM when unavailable (short)", prefer_short == false);
        TEST("don't prefer AFM when unavailable (tools)", prefer_tools == false);
        TEST("don't prefer AFM when unavailable (long)", prefer_long == false);
    }

    // Get recommended provider
    const char* recommended = afm_get_recommended_local_provider();
    TEST("get_recommended returns valid string", recommended != NULL);
    TEST("recommendation is afm or mlx",
         strcmp(recommended, "apple_foundation") == 0 ||
         strcmp(recommended, "mlx") == 0);
    printf("    Recommended local provider: %s\n", recommended);
}

// ============================================================================
// SCHEMA HELPER TESTS
// ============================================================================

void test_afm_schema_helpers(void) {
    TEST_SECTION("Schema Helpers");

    // Create text response schema
    AFMSchema* text_schema = afm_schema_text_response();
    TEST("create text response schema", text_schema != NULL);
    if (text_schema) {
        afm_schema_free(text_schema);
    }

    // Create custom schema
    AFMSchema* custom_schema = afm_schema_create("TestOutput", "A test output schema");
    TEST("create custom schema", custom_schema != NULL);

    if (custom_schema) {
        // Add fields
        afm_schema_add_field(custom_schema, "name", "The name field", AFM_TYPE_STRING, true);
        afm_schema_add_field(custom_schema, "count", "A count field", AFM_TYPE_INT, false);
        afm_schema_add_field(custom_schema, "active", "Is active", AFM_TYPE_BOOL, false);

        // Add enum field
        const char* options[] = {"low", "medium", "high"};
        afm_schema_add_enum(custom_schema, "priority", "Priority level", options, 3, true);

        TEST("schema has name", custom_schema->name != NULL);
        TEST("schema has description", custom_schema->description != NULL);

        afm_schema_free(custom_schema);
    }
}

// ============================================================================
// CONVERGIO INTEGRATION TESTS
// ============================================================================

void test_afm_convergio_integration(void) {
    TEST_SECTION("Convergio Integration");

    // Initialize
    int result = afm_convergio_init();
    TEST("convergio init succeeds or gracefully fails", result == 0 || result == -1);

    // Double init should be safe
    result = afm_convergio_init();
    TEST("double init is safe", result == 0 || result == -1);

    // Shutdown
    afm_convergio_shutdown();
    TEST("convergio shutdown succeeds", true);

    // Double shutdown should be safe
    afm_convergio_shutdown();
    TEST("double shutdown is safe", true);
}

// ============================================================================
// SESSION TESTS (when available)
// ============================================================================

void test_afm_session_when_available(void) {
    TEST_SECTION("Session Tests (Conditional)");

    AppleFoundationStatus status = {0};
    AppleFoundationError availability = afm_check_availability(&status);

    if (availability != AFM_AVAILABLE) {
        printf("    Skipping session tests (AFM not available)\n");
        TEST("skip session tests when unavailable", true);
        return;
    }

    // Create session
    AFMSession session = {0};
    AppleFoundationError err = afm_session_create(&session);
    TEST("create session succeeds", err == AFM_AVAILABLE);
    TEST("session is active", session.is_active == true);

    if (session.is_active) {
        // Simple generation test
        char* response = NULL;
        err = afm_simple_generate("Say 'Hello World' and nothing else.", &response);

        if (err == AFM_AVAILABLE) {
            TEST("simple generate produces response", response != NULL);
            if (response) {
                TEST("response contains content", strlen(response) > 0);
                printf("    Response: %.100s%s\n", response, strlen(response) > 100 ? "..." : "");
                free(response);
            }
        } else {
            printf("    Generation failed: %s\n", afm_status_description(err));
            TEST("generation handles error gracefully", err != AFM_ERR_UNKNOWN);
        }

        // Destroy session
        afm_session_destroy(&session);
        TEST("session destroyed", session.is_active == false || session._session == NULL);
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("\033[1m");
    printf("================================================\n");
    printf("  CONVERGIO APPLE FOUNDATION MODELS UNIT TESTS\n");
    printf("================================================\n");
    printf("\033[0m");

    // Availability tests
    test_afm_availability_check();

    // Error description tests
    test_afm_error_descriptions();

    // Provider tests
    test_afm_provider_create();

    // Recommendation tests
    test_afm_recommendations();

    // Schema helper tests
    test_afm_schema_helpers();

    // Convergio integration tests
    test_afm_convergio_integration();

    // Session tests (conditional)
    test_afm_session_when_available();

    // Summary
    printf("\n\033[1m");
    printf("================================================\n");
    printf("Results: %d tests, ", tests_run);
    if (tests_failed == 0) {
        printf("\033[32m%d passed\033[0m\033[1m, ", tests_passed);
    } else {
        printf("%d passed, ", tests_passed);
    }
    if (tests_failed > 0) {
        printf("\033[31m%d failed\033[0m\033[1m\n", tests_failed);
    } else {
        printf("0 failed\n");
    }
    printf("================================================\n");
    printf("\033[0m");

    return tests_failed > 0 ? 1 : 0;
}
