/**
 * CONVERGIO KERNEL - Command Handlers
 *
 * REPL command implementations
 */

#ifndef NOUS_COMMANDS_H
#define NOUS_COMMANDS_H

#include <signal.h>
#include <stdbool.h>

// ============================================================================
// COMMAND TABLE
// ============================================================================

typedef struct {
    const char* name;
    const char* description;
    int (*handler)(int argc, char** argv);
} ReplCommand;

// Get the command table
const ReplCommand* commands_get_table(void);

// ============================================================================
// COMMAND HANDLERS
// ============================================================================

// Core commands
int cmd_help(int argc, char** argv);
int cmd_quit(int argc, char** argv);
int cmd_status(int argc, char** argv);

// Agent management
int cmd_agent(int argc, char** argv);
int cmd_agents(int argc, char** argv);
int cmd_think(int argc, char** argv);

// Space management
int cmd_space(int argc, char** argv);
int cmd_create(int argc, char** argv);

// Cost and budget
int cmd_cost(int argc, char** argv);

// System configuration
int cmd_debug(int argc, char** argv);
int cmd_allow_dir(int argc, char** argv);
int cmd_allowed_dirs(int argc, char** argv);
int cmd_stream(int argc, char** argv);
int cmd_theme(int argc, char** argv);

// Authentication
int cmd_logout(int argc, char** argv);
int cmd_auth(int argc, char** argv);

// System updates
int cmd_update(int argc, char** argv);
int cmd_hardware(int argc, char** argv);

// ============================================================================
// GLOBAL STATE ACCESS
// ============================================================================

// These need to be accessed by commands
extern volatile sig_atomic_t g_running;
extern void* g_current_space;  // NousSpace*
extern void* g_assistant;      // NousAgent*
extern bool g_streaming_enabled;

#endif // NOUS_COMMANDS_H
