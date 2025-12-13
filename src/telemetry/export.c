/**
 * CONVERGIO TELEMETRY - Export and Deletion
 *
 * Provides user control over telemetry data
 * Export, view, and delete functionality
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/telemetry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

// ============================================================================
// EXPORT
// ============================================================================

char* telemetry_export(void) {
    const TelemetryConfig* config = telemetry_get_config();
    if (!config) {
        return NULL;
    }

    // Read data file
    FILE* f = fopen(config->data_path, "r");
    if (!f) {
        fprintf(stderr, "No telemetry data to export.\n");
        return NULL;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        fprintf(stderr, "Telemetry data file is empty.\n");
        return NULL;
    }

    // Allocate buffer
    char* data = malloc((size_t)size + 1);
    if (!data) {
        fclose(f);
        fprintf(stderr, "Failed to allocate memory for export.\n");
        return NULL;
    }

    // Read file
    size_t read_size = fread(data, 1, (size_t)size, f);
    fclose(f);

    if (read_size != (size_t)size) {
        free(data);
        fprintf(stderr, "Failed to read telemetry data.\n");
        return NULL;
    }

    data[size] = '\0';
    return data;
}

// ============================================================================
// DELETE
// ============================================================================

int telemetry_delete(void) {
    const TelemetryConfig* config = telemetry_get_config();
    if (!config) {
        return -1;
    }

    // Confirm deletion
    printf("\n");
    printf("WARNING: This will permanently delete all collected telemetry data.\n");
    printf("This action cannot be undone.\n");
    printf("\n");
    printf("Are you sure you want to delete all telemetry data? (yes/no): ");

    char response[16];
    if (fgets(response, sizeof(response), stdin)) {
        // Remove newline
        size_t len = strlen(response);
        if (len > 0 && response[len - 1] == '\n') {
            response[len - 1] = '\0';
        }

        if (strcmp(response, "yes") != 0) {
            printf("Deletion cancelled.\n");
            return -1;
        }
    } else {
        printf("Deletion cancelled.\n");
        return -1;
    }

    // Delete data file
    if (unlink(config->data_path) != 0) {
        // File might not exist, which is fine
        if (access(config->data_path, F_OK) == 0) {
            fprintf(stderr, "Failed to delete telemetry data file.\n");
            return -1;
        }
    }

    printf("All telemetry data has been deleted.\n");
    return 0;
}

// ============================================================================
// VIEW
// ============================================================================

void telemetry_view(void) {
    const TelemetryConfig* config = telemetry_get_config();
    if (!config) {
        printf("Telemetry not initialized.\n");
        return;
    }

    // Read data file
    FILE* f = fopen(config->data_path, "r");
    if (!f) {
        printf("No telemetry data collected yet.\n");
        return;
    }

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════╗\n");
    printf("║                     COLLECTED TELEMETRY DATA                          ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    // Simple parsing to display human-readable format
    char line[2048];
    bool in_events = false;
    int event_count = 0;

    while (fgets(line, sizeof(line), f)) {
        // Check for events array
        if (strstr(line, "\"events\"")) {
            in_events = true;
            printf("EVENTS:\n");
            printf("-------\n");
            continue;
        }

        if (in_events) {
            // Display event type
            if (strstr(line, "\"type\"")) {
                event_count++;
                printf("\nEvent #%d:\n", event_count);
            }

            // Display key fields
            if (strstr(line, "\"type\"") ||
                strstr(line, "\"timestamp\"") ||
                strstr(line, "\"provider\"") ||
                strstr(line, "\"model\"") ||
                strstr(line, "\"tokens_input\"") ||
                strstr(line, "\"tokens_output\"") ||
                strstr(line, "\"latency_ms\"") ||
                strstr(line, "\"error_type\"") ||
                strstr(line, "\"from_provider\"") ||
                strstr(line, "\"to_provider\"")) {

                // Extract and format
                char key[64], value[256];
                if (sscanf(line, " \"%63[^\"]\" : \"%255[^\"]\"", key, value) == 2) {
                    printf("  %s: %s\n", key, value);
                } else if (sscanf(line, " \"%63[^\"]\" : %255[^,}]", key, value) == 2) {
                    // Handle timestamp conversion
                    if (strcmp(key, "timestamp") == 0) {
                        time_t t = atol(value);
                        struct tm* tm_info = localtime(&t);
                        char time_buf[64];
                        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
                        printf("  %s: %s (%s)\n", key, value, time_buf);
                    } else {
                        printf("  %s: %s\n", key, value);
                    }
                }
            }
        }
    }

    fclose(f);

    printf("\n");
    printf("Total events: %d\n", event_count);
    printf("\n");
    printf("To export this data as JSON: telemetry export\n");
    printf("To delete all data: telemetry delete\n");
    printf("\n");
}
