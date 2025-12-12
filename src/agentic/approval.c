/**
 * CONVERGIO KERNEL - User Approval System
 *
 * Handles user approval for tool installations and system changes
 *
 * CRITICAL: This is a security-critical module. All installations
 * MUST go through explicit user approval. NEVER bypass this.
 */

#include "nous/agentic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#define APPROVALS_FILE "approvals.txt"
#define MAX_INPUT 128
#define MAX_LINE 512

// ============================================================================
// APPROVAL FILE MANAGEMENT (Simple text format: "action=0" or "action=1")
// ============================================================================

static char* get_approvals_path(void) {
    static char path[1024];

    // Use ~/.convergio/approvals.txt
    const char* home = getenv("HOME");
    if (!home) {
        return NULL;
    }

    snprintf(path, sizeof(path), "%s/.convergio/%s", home, APPROVALS_FILE);
    return path;
}

static void ensure_config_dir(void) {
    const char* home = getenv("HOME");
    if (home) {
        char dir[1024];
        snprintf(dir, sizeof(dir), "%s/.convergio", home);
        mkdir(dir, 0755);
    }
}

// ============================================================================
// APPROVAL CHECKING
// ============================================================================

bool is_action_approved(const char* action) {
    if (!action || action[0] == '\0') {
        return false;
    }

    const char* path = get_approvals_path();
    if (!path) return false;

    FILE* f = fopen(path, "r");
    if (!f) return false;

    char line[MAX_LINE];
    bool approved = false;
    size_t action_len = strlen(action);

    while (fgets(line, sizeof(line), f)) {
        // Format: action=1 or action=0
        if (strncmp(line, action, action_len) == 0 && line[action_len] == '=') {
            approved = (line[action_len + 1] == '1');
            break;
        }
    }

    fclose(f);
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

    ensure_config_dir();

    const char* path = get_approvals_path();
    if (!path) return -1;

    // Read existing approvals
    FILE* f = fopen(path, "r");
    char lines[100][MAX_LINE];
    int count = 0;
    size_t action_len = strlen(action);
    bool found = false;

    if (f) {
        while (count < 100 && fgets(lines[count], MAX_LINE, f)) {
            // Update if action already exists
            if (strncmp(lines[count], action, action_len) == 0 &&
                lines[count][action_len] == '=') {
                snprintf(lines[count], MAX_LINE, "%s=%d\n", action, approved ? 1 : 0);
                found = true;
            }
            count++;
        }
        fclose(f);
    }

    // Add new entry if not found
    if (!found && count < 100) {
        snprintf(lines[count], MAX_LINE, "%s=%d\n", action, approved ? 1 : 0);
        count++;
    }

    // Write back
    f = fopen(path, "w");
    if (!f) return -1;

    for (int i = 0; i < count; i++) {
        fputs(lines[i], f);
    }
    fclose(f);

    return 0;
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

bool request_user_approval(ApprovalRequest* req) {
    if (!req || !req->action || !req->command) {
        return false;
    }

    // Check if already approved
    if (is_action_approved(req->action)) {
        return true;
    }

    // Display approval prompt
    printf("\n");
    printf("┌─────────────────────────────────────────────────────────────┐\n");
    printf("│  \033[1;33mAPPROVAL REQUIRED\033[0m                                          │\n");
    printf("├─────────────────────────────────────────────────────────────┤\n");
    printf("│  Action: \033[1m%-48s\033[0m │\n", req->action);
    if (req->reason) {
        printf("│  Reason: %-48s │\n", req->reason);
    }
    printf("│  Command: \033[33m%-47s\033[0m │\n", req->command);

    if (req->is_destructive) {
        printf("│  \033[31m⚠ WARNING: This action may be destructive!\033[0m                 │\n");
    }

    printf("├─────────────────────────────────────────────────────────────┤\n");
    printf("│  [y] Yes    [n] No (default)    [a] Always    [N] Never    │\n");
    printf("└─────────────────────────────────────────────────────────────┘\n");
    printf("\nApprove? ");
    fflush(stdout);

    // Read user input
    char input[MAX_INPUT];
    if (!fgets(input, sizeof(input), stdin)) {
        return false;
    }

    // Parse input
    char c = tolower(input[0]);

    switch (c) {
        case 'y':
            // Yes - approve once
            return true;

        case 'a':
            // Always - approve and remember
            store_approval(req->action, true, true);
            printf("\033[32m✓ Approved and remembered.\033[0m\n");
            return true;

        case 'n':
        case '\n':
        case '\0':
            // No - deny once (default)
            return false;

        case 'N':
            // Never - deny and remember
            store_approval(req->action, false, true);
            printf("\033[31m✗ Denied and remembered.\033[0m\n");
            return false;

        default:
            // Unknown - default to no
            printf("Unknown response. Defaulting to No.\n");
            return false;
    }
}
