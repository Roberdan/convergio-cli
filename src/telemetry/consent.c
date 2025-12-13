/**
 * CONVERGIO TELEMETRY - Consent Management
 *
 * Handles user consent for telemetry collection
 * Displays privacy information and status
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/telemetry.h"
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// CONSENT PROMPT
// ============================================================================

void telemetry_show_consent_prompt(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════╗\n");
    printf("║                     CONVERGIO TELEMETRY                               ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Convergio can collect anonymous usage metrics to help improve the product.\n");
    printf("\n");
    printf("CORE PRINCIPLES:\n");
    printf("  • OPT-IN ONLY (never enabled by default)\n");
    printf("  • Privacy-first (no PII, anonymous aggregate metrics only)\n");
    printf("  • User control (view/export/delete at any time)\n");
    printf("\n");
    printf("WHAT WE COLLECT:\n");
    printf("  ✓ Provider/model usage (e.g., \"anthropic/claude-sonnet-4\")\n");
    printf("  ✓ Aggregated token consumption per session\n");
    printf("  ✓ Average API latency\n");
    printf("  ✓ Error/fallback rates (not error content)\n");
    printf("  ✓ Convergio version + OS type\n");
    printf("  ✓ Anonymous hash for deduplication\n");
    printf("\n");
    printf("WHAT WE NEVER COLLECT:\n");
    printf("  ✗ User prompts or AI responses\n");
    printf("  ✗ API keys or credentials\n");
    printf("  ✗ File paths or local data\n");
    printf("  ✗ IP addresses\n");
    printf("  ✗ Personal identifiers\n");
    printf("\n");
    printf("DATA STORAGE:\n");
    printf("  • Data stored locally in ~/.convergio/telemetry.json\n");
    printf("  • No automatic network transmission (backend TBD)\n");
    printf("\n");
    printf("USER CONTROLS:\n");
    printf("  • Enable:  telemetry enable\n");
    printf("  • Disable: telemetry disable\n");
    printf("  • View:    telemetry view\n");
    printf("  • Export:  telemetry export\n");
    printf("  • Delete:  telemetry delete\n");
    printf("\n");
    printf("For more information, visit: https://convergio.ai/privacy\n");
    printf("\n");
}

// ============================================================================
// STATUS DISPLAY
// ============================================================================

void telemetry_status(void) {
    const TelemetryConfig* config = telemetry_get_config();
    if (!config) {
        printf("Telemetry not initialized.\n");
        return;
    }

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════╗\n");
    printf("║                     TELEMETRY STATUS                                  ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Status:          %s\n", config->enabled ? "ENABLED" : "DISABLED");
    printf("Anonymous ID:    %s\n", config->anonymous_id[0] ? config->anonymous_id : "(none)");
    printf("Version:         %s\n", config->convergio_version);
    printf("OS Type:         %s\n", config->os_type);
    printf("Config Path:     %s\n", config->config_path);
    printf("Data Path:       %s\n", config->data_path);
    printf("\n");

    if (config->enabled) {
        // Show aggregated statistics
        char* stats = telemetry_get_stats();
        if (stats) {
            printf("AGGREGATED STATISTICS:\n");
            printf("%s\n", stats);
            free(stats);
        }
    } else {
        printf("Telemetry is currently disabled.\n");
        printf("To enable: telemetry enable\n");
        printf("To learn more: telemetry info\n");
    }

    printf("\n");
}
