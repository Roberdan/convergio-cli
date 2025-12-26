/**
 * CONVERGIO KERNEL - Command Dispatch
 *
 * Command table and dispatcher
 */

#include "commands_internal.h"

// ============================================================================
// COMMAND TABLE
// ============================================================================

static const ReplCommand COMMANDS[] = {
    {"help", "Show available commands", cmd_help},
    {"agent", "Manage agents", cmd_agent},
    {"agents", "List all available agents", cmd_agents},
    {"project", "Manage projects with dedicated teams", cmd_project},
    {"setup", "Configure providers and agent models", cmd_setup},
    {"space", "Manage collaborative spaces", cmd_space},
    {"status", "Show system status", cmd_status},
    {"cost", "Show/set cost and budget", cmd_cost},
    {"debug", "Toggle debug mode (off/error/warn/info/debug/trace)", cmd_debug},
    {"allow-dir", "Add directory to sandbox", cmd_allow_dir},
    {"allowed-dirs", "Show allowed directories", cmd_allowed_dirs},
    {"logout", "Logout and clear credentials", cmd_logout},
    {"auth", "Show authentication status", cmd_auth},
    {"update", "Check for and install updates", cmd_update},
    {"hardware", "Show hardware information", cmd_hardware},
    {"stream", "Toggle streaming mode (on/off)", cmd_stream},
    {"theme", "Interactive theme selector (or /theme <name>)", cmd_theme},
    {"style", "Set response style (flash/concise/balanced/detailed)", cmd_style},
    {"compare", "Compare responses from 2-3 models", cmd_compare},
    {"benchmark", "Test ONE model's speed (N runs)", cmd_benchmark},
    {"telemetry", "Manage telemetry settings", cmd_telemetry},
    {"tools", "Manage development tools", cmd_tools},
    {"news", "Show release notes", cmd_news},
    // Session recall
    {"recall", "View/load past sessions", cmd_recall},
    // Semantic memory commands
    {"reset", "Reset all Convergio memory (graph, notes, cache, etc.)", cmd_reset},
    {"remember", "Store a memory", cmd_remember},
    {"search", "Search memories semantically", cmd_search},
    {"memories", "List recent/important memories", cmd_memories},
    {"forget", "Delete a memory by ID", cmd_forget},
    {"graph", "Show knowledge graph stats", cmd_graph},
    // Todo manager (Anna Executive Assistant)
    {"todo", "Manage tasks and reminders", cmd_todo},
    {"remind", "Quick reminder: /remind <msg> <when>", cmd_remind},
    {"reminders", "Show upcoming reminders", cmd_reminders},
    // Git/Test workflow commands
    {"test", "Run project tests (auto-detect framework)", cmd_test},
    {"git", "Git workflow helper (status/commit/push)", cmd_git},
    {"pr", "Create pull request via gh CLI", cmd_pr},
    // Anna Executive Assistant - Daemon & MCP
    {"daemon", "Manage notification daemon", cmd_daemon},
    {"mcp", "Manage MCP servers and tools", cmd_mcp},
    // Execution Plan management
    {"plan", "Manage execution plans (list/status/export)", cmd_plan},
    // Output Service
    {"output", "Manage generated outputs (list/open/delete)", cmd_output},
    // Workflow orchestration
    {"workflow", "Manage workflows (list/show/execute/resume)", cmd_workflow},
    // Education Pack commands
    {"education", "Education setup and management", cmd_education},
    {"study", "Start a study session with a maestro", cmd_study},
    {"homework", "Get help with homework (anti-cheating)", cmd_homework},
    {"quiz", "Generate adaptive quizzes", cmd_quiz},
    {"flashcards", "Create and review flashcards", cmd_flashcards},
    {"mindmap", "Generate visual mind maps", cmd_mindmap},
    {"libretto", "Student gradebook and activity log", cmd_libretto},
    {"voice", "Conversational voice mode with maestri", cmd_voice},
    {"upload", "Upload a document for study help", cmd_upload},
    {"doc", "Manage uploaded documents", cmd_doc},
    {"onboarding", "Start Ali's conversational setup", cmd_onboarding},
    {"settings", "Display/manage settings and accessibility", cmd_settings},
    {"profile", "Show student profile", cmd_profile},
    {"quit", "Exit Convergio", cmd_quit},
    {"exit", "Exit Convergio", cmd_quit},
    {NULL, NULL, NULL}};

const ReplCommand* commands_get_table(void) {
    return COMMANDS;
}
