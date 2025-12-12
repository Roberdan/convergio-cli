/**
 * CONVERGIO KERNEL - User Approval System
 *
 * Handles user approval for tool installations and system changes
 *
 * CRITICAL: This is a security-critical module. All installations
 * MUST go through explicit user approval. NEVER bypass this.
 */

#include "nous/agentic.h"
#include "nous/safe_path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

// JSON parsing helper (simple)
#include <cjson/cJSON.h>

#define APPROVALS_FILE "approvals.json"
#define MAX_INPUT 128

// ============================================================================
// APPROVAL FILE MANAGEMENT
// ============================================================================

static char* get_approvals_path(void) {
    static char path[1024];

    // Use ~/.convergio/approvals.json
    const char* home = getenv("HOME");
    if (!home) {
        return NULL;
    }

    snprintf(path, sizeof(path), "%s/.convergio/%s", home, APPROVALS_FILE);
    return path;
}

static cJSON* load_approvals(void) {
    const char* path = get_approvals_path();
    if (!path) {
        return NULL;
    }

    FILE* f = fopen(path, "r");
    if (!f) {
        // File doesn't exist yet - return empty object
        return cJSON_CreateObject();
    }

    // Read file
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        return cJSON_CreateObject();
    }

    char* content = malloc((size_t)size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    size_t read_size = fread(content, 1, (size_t)size, f);
    content[read_size] = '\0';
    fclose(f);

    cJSON* json = cJSON_Parse(content);
    free(content);

    if (!json) {
        // Parse error - return empty object
        return cJSON_CreateObject();
    }

    return json;
}

static int save_approvals(cJSON* approvals) {
    if (!approvals) {
        return -1;
    }

    const char* path = get_approvals_path();
    if (!path) {
        return -1;
    }

    // Ensure ~/.convergio directory exists
    const char* home = getenv("HOME");
    if (home) {
        char dir[1024];
        snprintf(dir, sizeof(dir), "%s/.convergio", home);
        mkdir(dir, 0755);
    }

    // Serialize JSON
    char* json_str = cJSON_Print(approvals);
    if (!json_str) {
        return -1;
    }

    // Write file
    FILE* f = fopen(path, "w");
    if (!f) {
        free(json_str);
        return -1;
    }

    fputs(json_str, f);
    fclose(f);
    free(json_str);

    return 0;
}

// ============================================================================
// APPROVAL CHECKING
// ============================================================================

bool is_action_approved(const char* action) {
    if (!action || action[0] == '\0') {
        return false;
    }

    cJSON* approvals = load_approvals();
    if (!approvals) {
        return false;
    }

    cJSON* item = cJSON_GetObjectItem(approvals, action);
    bool approved = false;

    if (item && cJSON_IsBool(item)) {
        approved = cJSON_IsTrue(item);
    }

    cJSON_Delete(approvals);
    return approved;
}

int store_approval(const char* action, bool approved, bool remember) {
    if (!action || action[0] == '\0') {
        return -1;
    }

    // Only store if "remember" is true
    if (!remember) {
        return 0;
    }

    cJSON* approvals = load_approvals();
    if (!approvals) {
        return -1;
    }

    // Add or update the approval
    cJSON_AddBoolToObject(approvals, action, approved);

    int result = save_approvals(approvals);
    cJSON_Delete(approvals);

    return result;
}

int clear_approvals(void) {
    const char* path = get_approvals_path();
    if (!path) {
        return -1;
    }

    if (unlink(path) == 0 || errno == ENOENT) {
        return 0;
    }

    return -1;
}

// ============================================================================
// USER APPROVAL PROMPT
// ============================================================================

static void trim_whitespace(char* str) {
    if (!str) return;

    // Trim leading whitespace
    char* start = str;
    while (isspace((unsigned char)*start)) start++;

    // Trim trailing whitespace
    char* end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';

    // Move trimmed string to beginning
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

bool request_user_approval(ApprovalRequest* req) {
    if (!req || !req->action || !req->command) {
        return false;
    }

    // Check if already approved
    if (is_action_approved(req->action)) {
        printf("\033[2m(Previously approved: %s)\033[0m\n", req->action);
        return true;
    }

    // Display approval prompt
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║              \033[1;33mUSER APPROVAL REQUIRED\033[0m                   ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  \033[1mAction:\033[0m   %s\n", req->action);
    printf("  \033[1mReason:\033[0m   %s\n", req->reason);
    printf("  \033[1mCommand:\033[0m  %s\n", req->command);
    printf("\n");

    if (req->is_destructive) {
        printf("  \033[31m⚠ WARNING: This action could modify system files\033[0m\n");
        printf("\n");
    }

    printf("Do you approve this action?\n");
    printf("  [y]es      - Approve once\n");
    printf("  [N]o       - Deny (default)\n");
    printf("  [a]lways   - Approve and remember\n");
    printf("  [n]ever    - Deny and remember\n");
    printf("\n");
    printf("Your choice: ");
    fflush(stdout);

    // Read user input
    char input[MAX_INPUT];
    if (!fgets(input, sizeof(input), stdin)) {
        return false;  // EOF or error - deny
    }

    trim_whitespace(input);

    // Convert to lowercase for comparison
    for (char* p = input; *p; p++) {
        *p = (char)tolower((unsigned char)*p);
    }

    // Parse response
    bool approved = false;
    bool remember = false;

    if (strcmp(input, "y") == 0 || strcmp(input, "yes") == 0) {
        approved = true;
        remember = false;
    } else if (strcmp(input, "a") == 0 || strcmp(input, "always") == 0) {
        approved = true;
        remember = true;
    } else if (strcmp(input, "n") == 0 || strcmp(input, "no") == 0 || input[0] == '\0') {
        approved = false;
        remember = false;
    } else if (strcmp(input, "never") == 0) {
        approved = false;
        remember = true;
    } else {
        // Unknown response - default to deny
        printf("\nInvalid response. Defaulting to: No\n");
        approved = false;
        remember = false;
    }

    // Store decision if remember is requested
    if (remember) {
        store_approval(req->action, approved, true);
        if (approved) {
            printf("\n\033[32m✓ Approved and remembered\033[0m\n");
        } else {
            printf("\n\033[31m✗ Denied and remembered\033[0m\n");
        }
    } else {
        if (approved) {
            printf("\n\033[32m✓ Approved (once)\033[0m\n");
        } else {
            printf("\n\033[31m✗ Denied\033[0m\n");
        }
    }

    return approved;
}
