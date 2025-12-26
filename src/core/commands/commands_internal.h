/**
 * CONVERGIO KERNEL - Commands Internal Header
 *
 * Shared declarations for command implementations
 */

#ifndef COMMANDS_INTERNAL_H
#define COMMANDS_INTERNAL_H

#include "nous/commands.h"
#include "../../auth/oauth.h"
#include "nous/agentic.h"
#include "nous/compare.h"
#include "nous/config.h"
#include "nous/edition.h"
#include "nous/hardware.h"
#include "nous/mcp_client.h"
#include "nous/model_loader.h"
#include "nous/notify.h"
#include "nous/nous.h"
#include "nous/orchestrator.h"
#include "nous/output_service.h"
#include "nous/plan_db.h"
#include "nous/projects.h"
#include "nous/semantic_persistence.h"
#include "nous/telemetry.h"
#include "nous/theme.h"
#include "nous/todo.h"
#include "nous/tools.h"
#include "nous/updater.h"
#include "nous/workflow.h"
#include <cjson/cJSON.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ============================================================================
// EXTERNAL DECLARATIONS
// ============================================================================

extern int nous_gpu_init(void);
extern void nous_gpu_shutdown(void);
extern void nous_gpu_print_stats(void);
extern int nous_scheduler_init(void);
extern void nous_scheduler_shutdown(void);
extern void nous_scheduler_print_metrics(void);

// ============================================================================
// HELP SYSTEM (from commands_help.c)
// ============================================================================

typedef struct {
    const char* name;
    const char* usage;
    const char* description;
    const char* details;
    const char* examples;
} CommandHelp;

// Get detailed help data array (defined in commands_help.c)
const CommandHelp* commands_get_detailed_help(void);

// Find detailed help for a command
const CommandHelp* find_detailed_help(const char* cmd_name);

// Print detailed help for a command
void print_detailed_help(const CommandHelp* h);

// ============================================================================
// FORWARD DECLARATIONS FOR ALL COMMANDS
// ============================================================================

// Core commands (commands_core.c)
int cmd_help(int argc, char** argv);
int cmd_quit(int argc, char** argv);
int cmd_status(int argc, char** argv);
int cmd_hardware(int argc, char** argv);
int cmd_update(int argc, char** argv);
int cmd_news(int argc, char** argv);
int cmd_recall(int argc, char** argv);
int cmd_stream(int argc, char** argv);
int cmd_theme(int argc, char** argv);

// Cost commands (commands_core.c)
int cmd_cost(int argc, char** argv);

// Agent commands (commands_agent.c)
int cmd_agent(int argc, char** argv);
int cmd_agents(int argc, char** argv);

// Space commands (commands_agent.c)
int cmd_space(int argc, char** argv);

// System commands (commands_system.c)
int cmd_debug(int argc, char** argv);
int cmd_allow_dir(int argc, char** argv);
int cmd_allowed_dirs(int argc, char** argv);
int cmd_logout(int argc, char** argv);
int cmd_auth(int argc, char** argv);
int cmd_style(int argc, char** argv);
int cmd_compare(int argc, char** argv);
int cmd_benchmark(int argc, char** argv);
int cmd_telemetry(int argc, char** argv);
int cmd_tools(int argc, char** argv);

// Project commands (commands_project.c)
int cmd_project(int argc, char** argv);
int cmd_setup(int argc, char** argv);

// Memory commands (commands_memory.c)
int cmd_remember(int argc, char** argv);
int cmd_search(int argc, char** argv);
int cmd_memories(int argc, char** argv);
int cmd_forget(int argc, char** argv);
int cmd_graph(int argc, char** argv);
int cmd_test(int argc, char** argv);
int cmd_git(int argc, char** argv);
int cmd_pr(int argc, char** argv);

// Todo commands (commands_todo.c)
int cmd_todo(int argc, char** argv);
int cmd_remind(int argc, char** argv);
int cmd_reminders(int argc, char** argv);
int cmd_daemon(int argc, char** argv);
int cmd_mcp(int argc, char** argv);

// Plan commands (commands_plan.c)
int cmd_plan(int argc, char** argv);
int cmd_output(int argc, char** argv);

// Workflow command (external, in workflow.c)
extern int cmd_workflow(int argc, char** argv);

// Education commands (education_commands.c)
int cmd_education(int argc, char** argv);
int cmd_study(int argc, char** argv);
int cmd_homework(int argc, char** argv);
int cmd_quiz(int argc, char** argv);
int cmd_flashcards(int argc, char** argv);
int cmd_mindmap(int argc, char** argv);
int cmd_libretto(int argc, char** argv);
int cmd_voice(int argc, char** argv);
int cmd_upload(int argc, char** argv);
int cmd_doc(int argc, char** argv);
int cmd_onboarding(int argc, char** argv);
int cmd_settings(int argc, char** argv);
int cmd_profile(int argc, char** argv);

#endif // COMMANDS_INTERNAL_H
