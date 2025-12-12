/**
 * CONVERGIO KERNEL - Tool Detection
 *
 * Detects installed development tools on the system
 */

#include "nous/agentic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ============================================================================
// COMMON DEVELOPMENT TOOLS
// ============================================================================

static const char* COMMON_TOOLS[] = {
    "gh",           // GitHub CLI
    "git",          // Git version control
    "node",         // Node.js runtime
    "npm",          // Node package manager
    "python3",      // Python 3
    "pip3",         // Python package manager
    "cargo",        // Rust package manager
    "go",           // Go language
    "make",         // GNU Make
    "cmake",        // CMake build system
    "docker",       // Docker containers
    "jq",           // JSON processor
    "curl",         // HTTP client
    "wget",         // File downloader
    NULL
};

const char** get_common_tools(void) {
    return COMMON_TOOLS;
}

// ============================================================================
// TOOL DETECTION
// ============================================================================

bool tool_exists(const char* tool_name) {
    if (!tool_name || tool_name[0] == '\0') {
        return false;
    }

    // Use 'command -v' which is POSIX-compliant and works on all shells
    char cmd[512];
    int written = snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1", tool_name);

    if (written < 0 || (size_t)written >= sizeof(cmd)) {
        return false;  // Tool name too long
    }

    // system() returns 0 if command succeeds (tool found)
    int result = system(cmd);
    return (result == 0);
}

// ============================================================================
// TOOL DETECTION SCANNING
// ============================================================================

int detect_missing_tools(void) {
    printf("\n\033[1mDevelopment Tools Status\033[0m\n");
    printf("═══════════════════════════════════════════════\n\n");

    size_t installed_count = 0;
    size_t missing_count = 0;
    const char* missing_tools[64] = {NULL};

    for (const char** tool = COMMON_TOOLS; *tool != NULL; tool++) {
        bool exists = tool_exists(*tool);

        if (exists) {
            printf("  \033[32m✓\033[0m %-12s (installed)\n", *tool);
            installed_count++;
        } else {
            printf("  \033[31m✗\033[0m %-12s (missing)\n", *tool);
            if (missing_count < 64) {
                missing_tools[missing_count++] = *tool;
            }
        }
    }

    printf("\n");
    printf("Summary: %zu installed, %zu missing\n\n", installed_count, missing_count);

    // Offer to install missing tools
    if (missing_count > 0) {
        printf("\033[36mMissing tools can be installed with:\033[0m\n");
        printf("  /tools install <tool_name>\n\n");

        printf("Example:\n");
        if (missing_tools[0]) {
            printf("  /tools install %s\n\n", missing_tools[0]);
        }
    } else {
        printf("\033[32mAll common development tools are installed!\033[0m\n\n");
    }

    return 0;
}

// ============================================================================
// PLATFORM DETECTION
// ============================================================================

PackageManager detect_package_manager(void) {
    // Check for Homebrew (macOS)
    if (tool_exists("brew")) {
        return PACKAGE_MANAGER_BREW;
    }

    // Check for apt (Debian/Ubuntu)
    if (tool_exists("apt-get")) {
        return PACKAGE_MANAGER_APT;
    }

    // Check for dnf (Fedora/RHEL)
    if (tool_exists("dnf")) {
        return PACKAGE_MANAGER_DNF;
    }

    // Check for pacman (Arch Linux)
    if (tool_exists("pacman")) {
        return PACKAGE_MANAGER_PACMAN;
    }

    return PACKAGE_MANAGER_UNKNOWN;
}

const char* package_manager_name(PackageManager pm) {
    switch (pm) {
        case PACKAGE_MANAGER_BREW:   return "Homebrew";
        case PACKAGE_MANAGER_APT:    return "apt";
        case PACKAGE_MANAGER_DNF:    return "dnf";
        case PACKAGE_MANAGER_PACMAN: return "pacman";
        default:                     return "unknown";
    }
}
