/**
 * CONVERGIO KERNEL - Tool Installation
 *
 * Installs development tools with user approval
 */

#include "nous/agentic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// TOOL INSTALLATION COMMANDS
// ============================================================================

// Map of tool names to their package names (if different)
typedef struct {
    const char* tool_name;
    const char* brew_package;
    const char* apt_package;
    const char* dnf_package;
    const char* pacman_package;
} ToolPackageMap;

static const ToolPackageMap TOOL_PACKAGES[] = {
    {"gh",      "gh",           "gh",           "gh",           "github-cli"},
    {"git",     "git",          "git",          "git",          "git"},
    {"node",    "node",         "nodejs",       "nodejs",       "nodejs"},
    {"npm",     "node",         "npm",          "npm",          "npm"},
    {"python3", "python@3",     "python3",      "python3",      "python"},
    {"pip3",    "python@3",     "python3-pip",  "python3-pip",  "python-pip"},
    {"cargo",   "rust",         "cargo",        "cargo",        "rust"},
    {"go",      "go",           "golang",       "golang",       "go"},
    {"make",    "make",         "build-essential", "make",      "base-devel"},
    {"cmake",   "cmake",        "cmake",        "cmake",        "cmake"},
    {"docker",  "docker",       "docker.io",    "docker",       "docker"},
    {"jq",      "jq",           "jq",           "jq",           "jq"},
    {"curl",    "curl",         "curl",         "curl",         "curl"},
    {"wget",    "wget",         "wget",         "wget",         "wget"},
    {NULL, NULL, NULL, NULL, NULL}
};

static const ToolPackageMap* find_tool_package(const char* tool_name) {
    for (const ToolPackageMap* map = TOOL_PACKAGES; map->tool_name != NULL; map++) {
        if (strcmp(map->tool_name, tool_name) == 0) {
            return map;
        }
    }
    return NULL;
}

const char* get_install_command(const char* tool_name) {
    if (!tool_name || tool_name[0] == '\0') {
        return NULL;
    }

    // Find the tool's package mapping
    const ToolPackageMap* map = find_tool_package(tool_name);
    if (!map) {
        return NULL;  // Unknown tool
    }

    // Detect package manager
    PackageManager pm = detect_package_manager();

    // Build installation command
    static char cmd_buffer[512];
    const char* package = NULL;

    switch (pm) {
        case PACKAGE_MANAGER_BREW:
            package = map->brew_package;
            if (package) {
                snprintf(cmd_buffer, sizeof(cmd_buffer), "brew install %s", package);
                return cmd_buffer;
            }
            break;

        case PACKAGE_MANAGER_APT:
            package = map->apt_package;
            if (package) {
                snprintf(cmd_buffer, sizeof(cmd_buffer), "sudo apt-get install -y %s", package);
                return cmd_buffer;
            }
            break;

        case PACKAGE_MANAGER_DNF:
            package = map->dnf_package;
            if (package) {
                snprintf(cmd_buffer, sizeof(cmd_buffer), "sudo dnf install -y %s", package);
                return cmd_buffer;
            }
            break;

        case PACKAGE_MANAGER_PACMAN:
            package = map->pacman_package;
            if (package) {
                snprintf(cmd_buffer, sizeof(cmd_buffer), "sudo pacman -S --noconfirm %s", package);
                return cmd_buffer;
            }
            break;

        default:
            break;
    }

    return NULL;
}

// ============================================================================
// TOOL INSTALLATION WITH APPROVAL
// ============================================================================

int install_tool(const char* tool_name, const char* reason) {
    if (!tool_name || tool_name[0] == '\0') {
        fprintf(stderr, "Error: Tool name is required\n");
        return -1;
    }

    // Check if already installed
    if (tool_exists(tool_name)) {
        printf("\033[32m✓\033[0m %s is already installed\n", tool_name);
        return 0;
    }

    // Get installation command
    const char* install_cmd = get_install_command(tool_name);
    if (!install_cmd) {
        fprintf(stderr, "Error: Don't know how to install '%s'\n", tool_name);
        fprintf(stderr, "Package manager not supported or tool not in registry\n");
        return -1;
    }

    // Build approval request
    ApprovalRequest req = {
        .action = tool_name,
        .reason = reason ? reason : "Development tool required",
        .command = install_cmd,
        .is_destructive = false
    };

    // Request approval (CRITICAL: never skip this)
    if (!request_user_approval(&req)) {
        printf("Installation cancelled by user\n");
        return -1;
    }

    // Execute installation
    printf("\nInstalling %s...\n", tool_name);
    printf("Command: %s\n\n", install_cmd);

    int result = system(install_cmd);

    if (result == 0) {
        // Verify installation succeeded
        if (tool_exists(tool_name)) {
            printf("\n\033[32m✓ Successfully installed %s\033[0m\n", tool_name);
            return 0;
        } else {
            fprintf(stderr, "\n\033[31m✗ Installation completed but %s not found in PATH\033[0m\n", tool_name);
            fprintf(stderr, "You may need to restart your shell or add it to PATH manually\n");
            return -1;
        }
    } else {
        fprintf(stderr, "\n\033[31m✗ Installation failed (exit code: %d)\033[0m\n", result);
        return -1;
    }
}
