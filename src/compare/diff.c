/**
 * CONVERGIO MODEL COMPARISON - Diff Generation
 *
 * Line-by-line diff between model responses
 */

#include "nous/compare.h"
#include "nous/nous.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// LINE SPLITTING
// ============================================================================

typedef struct {
    char** lines;
    size_t count;
} LineArray;

static LineArray split_lines(const char* text) {
    LineArray arr = {NULL, 0};
    if (!text) return arr;

    // Count lines
    size_t count = 1;
    for (const char* p = text; *p; p++) {
        if (*p == '\n') count++;
    }

    // Allocate lines array
    arr.lines = calloc(count, sizeof(char*));
    if (!arr.lines) return arr;

    // Split lines
    const char* start = text;
    size_t idx = 0;

    for (const char* p = text; *p; p++) {
        if (*p == '\n') {
            size_t len = p - start;
            arr.lines[idx] = malloc(len + 1);
            if (arr.lines[idx]) {
                memcpy(arr.lines[idx], start, len);
                arr.lines[idx][len] = '\0';
                idx++;
            }
            start = p + 1;
        }
    }

    // Last line (if no trailing newline)
    if (*start) {
        arr.lines[idx] = strdup(start);
        if (arr.lines[idx]) {
            idx++;
        }
    }

    arr.count = idx;
    return arr;
}

static void free_line_array(LineArray* arr) {
    if (!arr || !arr->lines) return;
    for (size_t i = 0; i < arr->count; i++) {
        free(arr->lines[i]);
    }
    free(arr->lines);
    arr->lines = NULL;
    arr->count = 0;
}

// ============================================================================
// SIMPLE DIFF ALGORITHM (LCS-based approximation)
// ============================================================================

typedef enum {
    DIFF_SAME,
    DIFF_ADD,
    DIFF_DELETE,
    DIFF_CHANGE
} DiffType;

typedef struct {
    DiffType type;
    size_t line1;
    size_t line2;
    char* text;
} DiffLine;

static DiffLine* generate_diff_lines(const LineArray* arr1, const LineArray* arr2, size_t* out_count) {
    // Simple line-by-line comparison (not optimal, but functional)
    size_t max_lines = arr1->count + arr2->count;
    DiffLine* diffs = calloc(max_lines, sizeof(DiffLine));
    if (!diffs) return NULL;

    size_t diff_count = 0;
    size_t i1 = 0, i2 = 0;

    while (i1 < arr1->count || i2 < arr2->count) {
        if (i1 >= arr1->count) {
            // Rest are additions
            diffs[diff_count].type = DIFF_ADD;
            diffs[diff_count].line2 = i2;
            diffs[diff_count].text = strdup(arr2->lines[i2]);
            diff_count++;
            i2++;
        } else if (i2 >= arr2->count) {
            // Rest are deletions
            diffs[diff_count].type = DIFF_DELETE;
            diffs[diff_count].line1 = i1;
            diffs[diff_count].text = strdup(arr1->lines[i1]);
            diff_count++;
            i1++;
        } else if (strcmp(arr1->lines[i1], arr2->lines[i2]) == 0) {
            // Lines are the same
            diffs[diff_count].type = DIFF_SAME;
            diffs[diff_count].line1 = i1;
            diffs[diff_count].line2 = i2;
            diffs[diff_count].text = strdup(arr1->lines[i1]);
            diff_count++;
            i1++;
            i2++;
        } else {
            // Lines differ - check if it's a change or add/delete
            // Simple heuristic: if next lines match, it's a change
            if (i1 + 1 < arr1->count && i2 + 1 < arr2->count &&
                strcmp(arr1->lines[i1 + 1], arr2->lines[i2 + 1]) == 0) {
                // Change
                diffs[diff_count].type = DIFF_DELETE;
                diffs[diff_count].line1 = i1;
                diffs[diff_count].text = strdup(arr1->lines[i1]);
                diff_count++;

                diffs[diff_count].type = DIFF_ADD;
                diffs[diff_count].line2 = i2;
                diffs[diff_count].text = strdup(arr2->lines[i2]);
                diff_count++;

                i1++;
                i2++;
            } else {
                // Assume deletion for now
                diffs[diff_count].type = DIFF_DELETE;
                diffs[diff_count].line1 = i1;
                diffs[diff_count].text = strdup(arr1->lines[i1]);
                diff_count++;
                i1++;
            }
        }
    }

    *out_count = diff_count;
    return diffs;
}

static void free_diff_lines(DiffLine* diffs, size_t count) {
    if (!diffs) return;
    for (size_t i = 0; i < count; i++) {
        free(diffs[i].text);
    }
    free(diffs);
}

// ============================================================================
// DIFF RENDERING
// ============================================================================

char* generate_response_diff(const char* response1, const char* response2,
                              const char* label1, const char* label2) {
    if (!response1 || !response2) return NULL;

    // Split into lines
    LineArray arr1 = split_lines(response1);
    LineArray arr2 = split_lines(response2);

    if (!arr1.lines || !arr2.lines) {
        free_line_array(&arr1);
        free_line_array(&arr2);
        return NULL;
    }

    // Generate diff
    size_t diff_count = 0;
    DiffLine* diffs = generate_diff_lines(&arr1, &arr2, &diff_count);

    // Build output string
    size_t output_size = 8192;
    char* output = malloc(output_size);
    if (!output) {
        free_diff_lines(diffs, diff_count);
        free_line_array(&arr1);
        free_line_array(&arr2);
        return NULL;
    }

    size_t pos = 0;
    pos += snprintf(output + pos, output_size - pos, "--- %s\n", label1 ? label1 : "Response 1");
    pos += snprintf(output + pos, output_size - pos, "+++ %s\n", label2 ? label2 : "Response 2");

    for (size_t i = 0; i < diff_count && pos < output_size - 100; i++) {
        switch (diffs[i].type) {
            case DIFF_SAME:
                pos += snprintf(output + pos, output_size - pos, "  %s\n",
                               diffs[i].text ? diffs[i].text : "");
                break;
            case DIFF_DELETE:
                pos += snprintf(output + pos, output_size - pos, "- %s\n",
                               diffs[i].text ? diffs[i].text : "");
                break;
            case DIFF_ADD:
                pos += snprintf(output + pos, output_size - pos, "+ %s\n",
                               diffs[i].text ? diffs[i].text : "");
                break;
            case DIFF_CHANGE:
                // Not used in current implementation
                break;
        }
    }

    free_diff_lines(diffs, diff_count);
    free_line_array(&arr1);
    free_line_array(&arr2);

    return output;
}

// ============================================================================
// DISPLAY ALL DIFFS
// ============================================================================

void display_all_diffs(const CompareResult* results, size_t count) {
    if (!results || count < 2) return;

    printf("\n");
    printf("═══ RESPONSE DIFFS ═══\n");
    printf("\n");

    // Compare first successful result with all others
    size_t base_idx = (size_t)-1;
    for (size_t i = 0; i < count; i++) {
        if (results[i].success && results[i].response) {
            base_idx = i;
            break;
        }
    }

    if (base_idx == (size_t)-1) {
        printf("No successful responses to compare.\n");
        return;
    }

    for (size_t i = 0; i < count; i++) {
        if (i == base_idx) continue;
        if (!results[i].success || !results[i].response) continue;

        printf("\033[1mDiff: %s vs %s\033[0m\n", results[base_idx].model_id, results[i].model_id);
        printf("────────────────────────────────────────────────────────────────\n");

        char* diff = generate_response_diff(results[base_idx].response, results[i].response,
                                           results[base_idx].model_id, results[i].model_id);
        if (diff) {
            // Colorize diff output
            char* line = diff;
            while (*line) {
                if (*line == '-' && *(line + 1) != '-') {
                    printf("\033[31m");  // Red for deletions
                } else if (*line == '+' && *(line + 1) != '+') {
                    printf("\033[32m");  // Green for additions
                }

                // Print until newline
                char* end = strchr(line, '\n');
                if (end) {
                    printf("%.*s\033[0m\n", (int)(end - line), line);
                    line = end + 1;
                } else {
                    printf("%s\033[0m\n", line);
                    break;
                }
            }

            free(diff);
        }

        printf("\n");
    }
}
