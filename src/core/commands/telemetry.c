/**
 * CONVERGIO TELEMETRY COMMANDS
 *
 * CLI commands for telemetry management
 */

#include "nous/telemetry.h"
#include "nous/commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// TELEMETRY COMMANDS
// ============================================================================

int cmd_telemetry(int argc, char** argv) {
    if (argc < 2) {
        printf("Telemetry commands:\n");
        printf("  telemetry status     Show telemetry status and statistics\n");
        printf("  telemetry enable     Enable telemetry (opt-in)\n");
        printf("  telemetry disable    Disable telemetry\n");
        printf("  telemetry view       View collected telemetry data\n");
        printf("  telemetry export     Export telemetry data as JSON\n");
        printf("  telemetry delete     Delete all collected telemetry data\n");
        printf("  telemetry consent    Show telemetry consent prompt\n");
        printf("\n");
        printf("Privacy: All telemetry data is stored locally and never transmitted.\n");
        printf("Telemetry is disabled by default (opt-in only).\n");
        return 0;
    }

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "status") == 0) {
        telemetry_status();
        return 0;
    } else if (strcmp(subcommand, "enable") == 0) {
        return telemetry_enable();
    } else if (strcmp(subcommand, "disable") == 0) {
        return telemetry_disable();
    } else if (strcmp(subcommand, "view") == 0) {
        telemetry_view();
        return 0;
    } else if (strcmp(subcommand, "export") == 0) {
        char* json = telemetry_export();
        if (json) {
            printf("%s\n", json);
            free(json);
            return 0;
        } else {
            fprintf(stderr, "Error: Failed to export telemetry data\n");
            return 1;
        }
    } else if (strcmp(subcommand, "delete") == 0) {
        return telemetry_delete();
    } else if (strcmp(subcommand, "consent") == 0) {
        telemetry_show_consent_prompt();
        return 0;
    } else {
        fprintf(stderr, "Error: Unknown telemetry command: %s\n", subcommand);
        fprintf(stderr, "Run 'telemetry' for available commands.\n");
        return 1;
    }
}
