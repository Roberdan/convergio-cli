/**
 * CONVERGIO KERNEL - Enhanced Agentic Capabilities
 *
 * Phase 1: Tool Detection and Installation with User Approval
 *
 * CRITICAL: All tool installations MUST require explicit user approval.
 * NEVER auto-install without consent.
 */

#ifndef NOUS_AGENTIC_H
#define NOUS_AGENTIC_H

#include <stdbool.h>

// ============================================================================
// APPROVAL MODES
// ============================================================================

typedef enum {
    APPROVAL_REQUIRED,      // Always ask (default)
    APPROVAL_REMEMBER,      // Ask once, remember choice
    APPROVAL_AUTO,          // Auto-approve (--auto-approve flag only)
} ApprovalMode;

// ============================================================================
// APPROVAL REQUEST
// ============================================================================

typedef struct {
    const char* action;     // "install gh", "install npm", etc.
    const char* reason;     // Why this tool is needed
    const char* command;    // Command to execute
    bool is_destructive;    // Could cause data loss?
} ApprovalRequest;

// ============================================================================
// TOOL DETECTION
// ============================================================================

// Check if a tool is installed
// Returns: true if tool exists in PATH, false otherwise
bool tool_exists(const char* tool_name);

// Detect and offer to install missing tools
// Returns: 0 on success, -1 on error
int detect_missing_tools(void);

// Get a list of common development tools
// Returns: NULL-terminated array of tool names
const char** get_common_tools(void);

// ============================================================================
// TOOL INSTALLATION
// ============================================================================

// Get the appropriate install command for a tool
// Returns: command string (e.g., "brew install gh") or NULL if unsupported
const char* get_install_command(const char* tool_name);

// Install a tool with user approval
// Returns: 0 on success, -1 on error or denial
int install_tool(const char* tool_name, const char* reason);

// ============================================================================
// USER APPROVAL
// ============================================================================

// Request user approval before executing an action
// Returns: true if approved, false if denied
bool request_user_approval(ApprovalRequest* req);

// Check if an action was previously approved
// Returns: true if approved in ~/.convergio/approvals.json
bool is_action_approved(const char* action);

// Store an approval decision
// Returns: 0 on success, -1 on error
int store_approval(const char* action, bool approved, bool remember);

// Clear all stored approvals
// Returns: 0 on success, -1 on error
int clear_approvals(void);

// ============================================================================
// PLATFORM DETECTION
// ============================================================================

typedef enum {
    PACKAGE_MANAGER_BREW,      // macOS Homebrew
    PACKAGE_MANAGER_APT,       // Debian/Ubuntu apt
    PACKAGE_MANAGER_DNF,       // Fedora/RHEL dnf
    PACKAGE_MANAGER_PACMAN,    // Arch Linux pacman
    PACKAGE_MANAGER_UNKNOWN,   // Unsupported
} PackageManager;

// Detect the system's package manager
// Returns: PackageManager enum
PackageManager detect_package_manager(void);

// Get package manager name
// Returns: human-readable name
const char* package_manager_name(PackageManager pm);

#endif // NOUS_AGENTIC_H
