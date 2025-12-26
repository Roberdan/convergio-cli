/**
 * CONVERGIO KERNEL - Core Commands
 *
 * Core, status, help display, recall, and cost commands
 */

#include "commands_internal.h"

// Reference to command table (defined in command_dispatch.c)
extern const ReplCommand* commands_get_table(void);
static const ReplCommand* COMMANDS;

// Module initializer to get command table
__attribute__((constructor))
static void init_commands_ref(void) {
    COMMANDS = commands_get_table();
}

// ============================================================================
// CORE COMMANDS
// ============================================================================

// Education Edition help
static void print_help_education(void) {
    printf("\n");
    printf("\033[32m┌──────────────────────────────────────────────────────────────┐\033[0m\n");
    printf("\033[32m│  \033[1;37mCONVERGIO EDUCATION\033[0;32m - Learn from History's Greatest     "
           "  │\033[0m\n");
    printf("\033[32m│  \033[2mAvailable Commands / Comandi Disponibili\033[0;32m                   "
           "│\033[0m\n");
    printf("\033[32m└──────────────────────────────────────────────────────────────┘\033[0m\n\n");

    // 1. YOUR TEACHERS - The 17 Maestri
    printf("\033[1;33m📚 YOUR TEACHERS\033[0m  \033[2m(15 historical maestri ready to "
           "teach)\033[0m\n");
    printf("   \033[36m@ali\033[0m               Principal - guides your learning journey\n");
    printf("   \033[36m@euclide\033[0m           Mathematics - logic, geometry, algebra\n");
    printf("   \033[36m@feynman\033[0m           Physics - makes complex ideas simple\n");
    printf("   \033[36m@manzoni\033[0m           Italian - literature and storytelling\n");
    printf("   \033[36m@darwin\033[0m            Sciences - observation and curiosity\n");
    printf("   \033[36m@erodoto\033[0m           History - bringing the past alive\n");
    printf("   \033[36m@humboldt\033[0m          Geography - nature and culture\n");
    printf("   \033[36m@leonardo\033[0m          Art - creativity and observation\n");
    printf("   \033[36m@shakespeare\033[0m       English - language and expression\n");
    printf("   \033[36m@mozart\033[0m            Music - joy of musical creation\n");
    printf("   \033[36m@cicerone\033[0m          Civics/Latin - rhetoric and citizenship\n");
    printf("   \033[36m@smith\033[0m             Economics - understanding markets\n");
    printf("   \033[36m@lovelace\033[0m          Computer Science - computational thinking\n");
    printf("   \033[36m@ippocrate\033[0m         Health - wellness and body care\n");
    printf("   \033[36m@socrate\033[0m           Philosophy - asking the right questions\n");
    printf("   \033[36m@chris\033[0m             Storytelling - narrative and communication\n");
    printf("   \033[36magents\033[0m             See all teachers and their specialties\n");
    printf("   \033[2m   Tip: @ali or 'back' returns to the Principal\033[0m\n\n");

    // 2. STUDY TOOLS
    printf("\033[1;33m📖 STUDY TOOLS\033[0m  \033[2m(interactive learning features)\033[0m\n");
    printf("   \033[36meducation\033[0m          Enter Education mode (all features)\n");
    printf("   \033[36mstudy <topic>\033[0m      Start a study session on any topic\n");
    printf("   \033[36mhomework <desc>\033[0m    Get help with your homework\n");
    printf("   \033[36mquiz <topic>\033[0m       Test your knowledge with a quiz\n");
    printf("   \033[36mflashcards <topic>\033[0m Create and practice flashcards\n");
    printf("   \033[36mmindmap <topic>\033[0m    Generate a visual mind map\n\n");

    // 3. LANGUAGE TOOLS
    printf("\033[1;33m🗣️  LANGUAGE TOOLS\033[0m  \033[2m(vocabulary and grammar)\033[0m\n");
    printf("   \033[36mdefine <word>\033[0m      Get definition with examples\n");
    printf("   \033[36mconjugate <verb>\033[0m   Show verb conjugations\n");
    printf("   \033[36mpronounce <word>\033[0m   Learn pronunciation\n");
    printf("   \033[36mgrammar <topic>\033[0m    Explain grammar rules\n\n");

    // 4. PROGRESS TRACKING
    printf("\033[1;33m📊 PROGRESS TRACKING\033[0m  \033[2m(your learning journey)\033[0m\n");
    printf("   \033[36mlibretto\033[0m           View your digital report card\n");
    printf("   \033[36mxp\033[0m                 Check your experience points\n");
    printf("   \033[2m   Tip: Complete quizzes and study sessions to earn XP!\033[0m\n\n");

    // 5. SPECIAL FEATURES
    printf("\033[1;33m✨ SPECIAL FEATURES\033[0m\n");
    printf("   \033[36mvoice\033[0m              Enable voice mode (text-to-speech)\n");
    printf("   \033[36mhtml <topic>\033[0m       Generate interactive HTML content\n");
    printf("   \033[36mcalc\033[0m               Scientific calculator\n");
    printf("   \033[36mperiodic\033[0m           Interactive periodic table\n");
    printf("   \033[36mconvert <expr>\033[0m     Unit converter (5km to miles)\n");
    printf("   \033[36mvideo <topic>\033[0m      Search educational videos\n\n");

    // 6. ORGANIZATION
    printf("\033[1;33m📅 ORGANIZATION\033[0m  \033[2m(Anna helps you stay organized)\033[0m\n");
    printf("   \033[36m@anna\033[0m              Ask Anna for help with scheduling\n");
    printf("   \033[36mtodo\033[0m               View your task list\n");
    printf("   \033[36mtodo add <task>\033[0m    Add homework or study tasks\n");
    printf("   \033[36mremind <time> <msg>\033[0m Set study reminders\n\n");

    // 7. SYSTEM
    printf("\033[1;33m⚙️  SYSTEM\033[0m\n");
    printf("   \033[36mstatus\033[0m             System health & active teachers\n");
    printf("   \033[36mtheme\033[0m              Change colors and appearance\n");
    printf("   \033[36msetup\033[0m              Configure your settings\n");
    printf("   \033[36mcost\033[0m               Track API usage\n\n");

    printf("\033[2m───────────────────────────────────────────────────────────────────\033[0m\n");
    printf(
        "\033[2mType \033[0mhelp <command>\033[2m for details  •  Or ask your teacher!\033[0m\n\n");
}

// Master/Full Edition help
static void print_help_master(void) {
    printf("\n");
    printf("\033[36m┌──────────────────────────────────────────────────────────────┐\033[0m\n");
    printf("\033[36m│  \033[1;37mCONVERGIO\033[0;36m - Your AI Team with Human Purpose             "
           "    │\033[0m\n");
    printf("\033[36m└──────────────────────────────────────────────────────────────┘\033[0m\n\n");

    // 1. YOUR AI TEAM - The most important feature
    printf(
        "\033[1;33m🤖 YOUR AI TEAM\033[0m  \033[2m(53 specialized agents ready to help)\033[0m\n");
    printf("   \033[36m@ali\033[0m               Chief of Staff - orchestrates everything\n");
    printf("   \033[36m@baccio\033[0m            Software Architect\n");
    printf("   \033[36m@marco\033[0m             Senior Developer\n");
    printf("   \033[36m@jenny\033[0m             Accessibility Expert\n");
    printf("   \033[36m@<name>\033[0m            Switch to talk with agent (Tab autocomplete)\n");
    printf("   \033[36m@<name> message\033[0m    Send message directly to agent\n");
    printf("   \033[36magents\033[0m             See all 53 agents with their specialties\n");
    printf("   \033[2m   Tip: @ali or 'back' returns to Ali from any agent\033[0m\n\n");

    // ANNA - Executive Assistant
    printf("\033[1;33m👩‍💼 ANNA - Executive Assistant\033[0m  \033[2m(your personal "
           "productivity hub)\033[0m\n");
    printf("   \033[36m@anna\033[0m                  Switch to Anna for task management\n");
    printf("   \033[36m@anna <task>\033[0m           Send task to Anna (IT/EN supported)\n");
    printf(
        "   \033[36mtodo\033[0m / \033[36mtodo list\033[0m      List your tasks with priorities\n");
    printf(
        "   \033[36mtodo add <task>\033[0m        Add a new task (supports @agent delegation)\n");
    printf("   \033[36mtodo done <id>\033[0m         Mark task as completed\n");
    printf("   \033[36mremind <time> <msg>\033[0m    Set reminders (e.g., remind 10m call Bob)\n");
    printf("   \033[36mreminders\033[0m              List pending reminders\n");
    printf("   \033[36mdaemon start\033[0m           Background agent for scheduled tasks\n");
    printf("   \033[2m   Tip: Anna speaks Italian too! \"ricordami tra 5 minuti\"\033[0m\n\n");

    // 2. PROJECTS - Team-based work
    printf("\033[1;33m📁 PROJECTS\033[0m  \033[2m(dedicated agent teams per project)\033[0m\n");
    printf("   \033[36mproject new <name>\033[0m         Create project with dedicated team\n");
    printf("   \033[36mproject team add <agent>\033[0m   Add agent to project team\n");
    printf("   \033[36mproject switch <name>\033[0m      Switch between projects\n");
    printf("   \033[36mproject\033[0m                    Show current project & team\n\n");

    // 3. KNOWLEDGE GRAPH - Persistent semantic memory
    printf(
        "\033[1;33m🧠 KNOWLEDGE GRAPH\033[0m  \033[2m(persistent memory across sessions)\033[0m\n");
    printf("   \033[36mremember <text>\033[0m    Store important facts and preferences\n");
    printf("   \033[36mrecall <query>\033[0m     Search your memories by keyword\n");
    printf("   \033[36mmemories\033[0m           List stored memories and graph stats\n");
    printf("   \033[36mgraph\033[0m              Show knowledge graph statistics\n");
    printf("   \033[36mforget <id>\033[0m        Remove a memory\n");
    printf("   \033[2m   Tip: Memories persist in SQLite and survive restarts\033[0m\n\n");

    // 4. POWER FEATURES
    printf("\033[1;33m⚡ POWER FEATURES\033[0m\n");
    printf("   \033[36mcompare \"prompt\"\033[0m           Compare responses from 2-3 different "
           "models\n");
    printf(
        "   \033[36mbenchmark \"prompt\" <model>\033[0m Test ONE model's speed & cost (N runs)\n");
    printf(
        "   \033[36msetup\033[0m                      Configure providers & models per agent\n\n");

    // 5. CUSTOMIZATION
    printf("\033[1;33m🎨 CUSTOMIZATION\033[0m\n");
    printf("   \033[36mtheme\033[0m              Interactive theme selector with preview\n");
    printf("   \033[36magent edit <name>\033[0m  Customize any agent's personality & model\n");
    printf("   \033[36magent create\033[0m       Create your own custom agent\n\n");

    // 6. SYSTEM
    printf("\033[1;33m⚙️  SYSTEM\033[0m\n");
    printf("   \033[36mcost\033[0m / \033[36mcost report\033[0m   Track spending across all "
           "providers\n");
    printf("   \033[36mstatus\033[0m             System health & active agents\n");
    printf("   \033[36mhardware\033[0m           Show Apple Silicon optimization info\n");
    printf("   \033[36mtools\033[0m              Manage agentic tools (file, web, code)\n");
    printf("   \033[36mrecall\033[0m             View past sessions, \033[36mrecall load "
           "<n>\033[0m to reload\n");
    printf(
        "   \033[36mdebug <level>\033[0m      Set debug level (off/error/warn/info/debug/trace)\n");
    printf("   \033[36mnews\033[0m               What's new in this version\n\n");

    printf("\033[2m───────────────────────────────────────────────────────────────────\033[0m\n");
    printf(
        "\033[2mType \033[0mhelp <command>\033[2m for details  •  Or just talk to Ali!\033[0m\n\n");
}

int cmd_help(int argc, char** argv) {
    // If a specific command is requested, show detailed help
    if (argc >= 2) {
        // Special handling for "help accessibility" in Education edition
        if (strcmp(argv[1], "accessibility") == 0 || strcmp(argv[1], "a11y") == 0) {
            if (edition_current() == EDITION_EDUCATION) {
                printf("\n");
                printf("╔═══════════════════════════════════════════════════════════════╗\n");
                printf("║           ♿ ACCESSIBILITY SUPPORT / ACCESSIBILITÀ            ║\n");
                printf("╠═══════════════════════════════════════════════════════════════╣\n");
                printf("║                                                               ║\n");
                printf("║  🎯 VISUAL / VISIVO                                           ║\n");
                printf("║  • OpenDyslexic font for dyslexia / Font per dislessia        ║\n");
                printf("║  • High contrast mode / Modalità alto contrasto               ║\n");
                printf("║  • Adjustable line spacing / Spaziatura regolabile            ║\n");
                printf("║  • Screen reader compatible / Compatibile con lettori         ║\n");
                printf("║  • VoiceOver support on macOS                                 ║\n");
                printf("║                                                               ║\n");
                printf("║  🖥️ MOTOR / MOTORIO                                            ║\n");
                printf("║  • Full keyboard navigation / Navigazione da tastiera         ║\n");
                printf("║  • Voice commands support / Supporto comandi voce             ║\n");
                printf("║  • No fine motor skills required                              ║\n");
                printf("║                                                               ║\n");
                printf("║  🧠 COGNITIVE / COGNITIVO                                      ║\n");
                printf("║  • ADHD-friendly short responses / Risposte brevi per ADHD    ║\n");
                printf("║  • Simplified language options / Linguaggio semplificato      ║\n");
                printf("║  • Step-by-step breakdowns / Suddivisione passo passo         ║\n");
                printf("║                                                               ║\n");
                printf("║  🔊 AUDIO                                                      ║\n");
                printf("║  • Text-to-speech (TTS) / Sintesi vocale                      ║\n");
                printf("║  • Audio descriptions / Descrizioni audio                     ║\n");
                printf("║                                                               ║\n");
                printf("╠═══════════════════════════════════════════════════════════════╣\n");
                printf("║  Configure with: /settings accessibility                      ║\n");
                printf("║  Contact: Jenny (Accessibility Champion) @jenny               ║\n");
                printf("╚═══════════════════════════════════════════════════════════════╝\n");
                printf("\n");
                return 0;
            }
        }

        // Check if command is available in current edition
        if (!edition_has_command(argv[1])) {
            printf("\n\033[33mCommand '%s' is not available in %s.\033[0m\n\n", argv[1],
                   edition_display_name());
            return -1;
        }

        const CommandHelp* h = find_detailed_help(argv[1]);
        if (h) {
            print_detailed_help(h);
            return 0;
        }

        // Check if it's a known command without detailed help
        for (const ReplCommand* cmd = COMMANDS; cmd->name != NULL; cmd++) {
            if (strcmp(cmd->name, argv[1]) == 0) {
                printf("\n\033[1m%s\033[0m - %s\n", cmd->name, cmd->description);
                printf("\nNo detailed help available for this command.\n\n");
                return 0;
            }
        }

        printf("\nUnknown command: %s\n", argv[1]);
        printf("Type 'help' to see available commands.\n\n");
        return -1;
    }

    // Show edition-specific general help
    switch (edition_current()) {
    case EDITION_EDUCATION:
        print_help_education();
        break;
    case EDITION_BUSINESS:
    case EDITION_DEVELOPER:
        // TODO: Add Business and Developer specific help
        print_help_master();
        break;
    default:
        print_help_master();
        break;
    }

    return 0;
}

// Progress callback for session compaction
static void quit_progress_callback(int percent, const char* msg) {
    // Simple progress bar: [████████░░] 80% Saving...
    int filled = percent / 10;
    int empty = 10 - filled;

    printf("\r\033[K["); // Clear line and start bar
    for (int i = 0; i < filled; i++)
        printf("\033[32m█\033[0m");
    for (int i = 0; i < empty; i++)
        printf("\033[90m░\033[0m");
    printf("] %d%% %s", percent, msg ? msg : "");
    fflush(stdout);

    if (percent >= 100) {
        printf("\n");
    }
}

int cmd_quit(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Compact current session before exit
    printf("\n");
    orchestrator_compact_session(quit_progress_callback);

    g_running = 0;
    return 0;
}

// Static storage for session ID mapping (for recall load/delete by number)
static char* g_recall_session_ids[50] = {0};
static size_t g_recall_session_count = 0;

static void recall_clear_cache(void) {
    for (size_t i = 0; i < g_recall_session_count; i++) {
        free(g_recall_session_ids[i]);
        g_recall_session_ids[i] = NULL;
    }
    g_recall_session_count = 0;
}

static const char* recall_get_session_id(int index) {
    if (index < 1 || (size_t)index > g_recall_session_count)
        return NULL;
    return g_recall_session_ids[index - 1];
}

int cmd_recall(int argc, char** argv) {
    // Handle subcommand: recall clear
    if (argc >= 2 && strcmp(argv[1], "clear") == 0) {
        printf("\n\033[33mAre you sure you want to clear all session summaries?\033[0m\n");
        printf("Type 'yes' to confirm: ");
        fflush(stdout);

        char confirm[10] = {0};
        if (fgets(confirm, sizeof(confirm), stdin) && strncmp(confirm, "yes", 3) == 0) {
            if (persistence_clear_all_summaries() == 0) {
                recall_clear_cache();
                printf("\033[32mAll session summaries cleared.\033[0m\n\n");
            } else {
                printf("\033[31mFailed to clear summaries.\033[0m\n\n");
            }
        } else {
            printf("Cancelled.\n\n");
        }
        return 0;
    }

    // Handle subcommand: recall delete <num>
    if (argc >= 3 && strcmp(argv[1], "delete") == 0) {
        int index = atoi(argv[2]);
        const char* session_id = recall_get_session_id(index);
        if (!session_id) {
            // Maybe they passed the full UUID
            session_id = argv[2];
        }
        if (persistence_delete_session(session_id) == 0) {
            printf("\033[32mSession deleted.\033[0m\n\n");
            recall_clear_cache(); // Invalidate cache
        } else {
            printf("\033[31mFailed to delete session. Run 'recall' first to see valid "
                   "numbers.\033[0m\n\n");
        }
        return 0;
    }

    // Handle subcommand: recall load <num>
    if (argc >= 3 && strcmp(argv[1], "load") == 0) {
        int index = atoi(argv[2]);
        const char* session_id = recall_get_session_id(index);
        if (!session_id) {
            printf("\n\033[31mInvalid session number. Run 'recall' first to see available "
                   "sessions.\033[0m\n\n");
            return -1;
        }

        // Load the checkpoint/summary for this session
        char* checkpoint = persistence_load_latest_checkpoint(session_id);
        if (checkpoint && strlen(checkpoint) > 0) {
            printf("\n\033[1;36m=== Loaded Context from Session %d ===\033[0m\n\n", index);
            printf("%s\n", checkpoint);
            printf("\n\033[32m✓ Context loaded. Ali now has this context for your "
                   "conversation.\033[0m\n\n");

            // Inject into orchestrator context
            Orchestrator* orch = orchestrator_get();
            if (orch) {
                free(orch->user_preferences);
                size_t len = strlen(checkpoint) + 100;
                orch->user_preferences = malloc(len);
                if (orch->user_preferences) {
                    snprintf(orch->user_preferences, len, "Previous session context:\n%s",
                             checkpoint);
                }
            }
            free(checkpoint);
        } else {
            printf("\n\033[33mNo detailed context found for this session.\033[0m\n");
            printf("The session may not have been compacted on exit.\n\n");
            free(checkpoint);
        }
        return 0;
    }

    // Default: show all session summaries
    SessionSummaryList* list = persistence_get_session_summaries();
    if (!list || list->count == 0) {
        printf("\n\033[90mNo past sessions found.\033[0m\n");
        printf("\033[90mSessions are saved when you type 'quit'.\033[0m\n\n");
        if (list)
            persistence_free_session_summaries(list);
        return 0;
    }

    // Cache session IDs for load/delete by number
    recall_clear_cache();
    for (size_t i = 0; i < list->count && i < 50; i++) {
        if (list->items[i].session_id) {
            g_recall_session_ids[i] = strdup(list->items[i].session_id);
            g_recall_session_count++;
        }
    }

    printf("\n\033[1m📚 Past Sessions\033[0m\n");
    printf("\033[90m────────────────────────────────────────────────────────\033[0m\n\n");

    for (size_t i = 0; i < list->count; i++) {
        SessionSummary* s = &list->items[i];

        // Header: [num] date (messages)
        printf("\033[1;36m[%zu]\033[0m \033[33m%s\033[0m", i + 1,
               s->started_at ? s->started_at : "Unknown");
        printf(" \033[90m(%d msgs)\033[0m\n", s->message_count);

        // Summary - the important part!
        if (s->summary && strlen(s->summary) > 0) {
            printf("    \033[37m");
            // Word wrap at ~70 chars with proper indentation
            const char* p = s->summary;
            int col = 0;
            size_t max_len = 300; // Show more of the summary
            size_t printed = 0;
            while (*p && printed < max_len) {
                if (*p == '\n') {
                    printf("\n    ");
                    col = 0;
                } else {
                    putchar(*p);
                    col++;
                    if (col > 65 && *p == ' ') {
                        printf("\n    ");
                        col = 0;
                    }
                }
                p++;
                printed++;
            }
            if (printed >= max_len && *p)
                printf("...");
            printf("\033[0m\n");
        } else {
            printf("    \033[90m(no summary - quit with 'quit' to save)\033[0m\n");
        }
        printf("\n");
    }

    printf("\033[90m────────────────────────────────────────────────────────\033[0m\n");
    printf("\033[36mrecall load <n>\033[0m   Load context into current session\n");
    printf("\033[36mrecall delete <n>\033[0m Delete a session\n");
    printf("\033[36mrecall clear\033[0m      Delete all sessions\n\n");

    persistence_free_session_summaries(list);
    return 0;
}

int cmd_status(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n=== NOUS System Status ===\n\n");

    // Kernel status
    printf("Kernel: %s\n", nous_is_ready() ? "READY" : "NOT READY");

    // Current space
    NousSpace* space = (NousSpace*)g_current_space;
    if (space) {
        printf("\nCurrent Space: %s\n", space->name);
        printf("  Purpose: %s\n", space->purpose);
        printf("  Participants: %zu\n", nous_space_participant_count(space));
        printf("  Urgency: %.2f\n", (double)nous_space_urgency(space));
        printf("  Active: %s\n", nous_space_is_active(space) ? "Yes" : "No");
    } else {
        printf("\nNo active space.\n");
    }

    // Assistant
    NousAgent* assistant = (NousAgent*)g_assistant;
    if (assistant) {
        printf("\nAssistant: %s\n", assistant->name);
        printf("  State: %d\n", assistant->state);
        printf("  Skills: %zu\n", assistant->skill_count);
    }

    printf("\n");

    // GPU stats
    nous_gpu_print_stats();

    // Scheduler metrics
    nous_scheduler_print_metrics();

    printf("\n");
    return 0;
}

// ============================================================================
// COST COMMANDS
// ============================================================================

int cmd_cost(int argc, char** argv) {
    if (argc < 2) {
        // Show brief cost status
        char* status = cost_get_status_line();
        if (status) {
            printf("%s\n", status);
            free(status);
        }
        return 0;
    }

    if (strcmp(argv[1], "report") == 0) {
        char* report = cost_get_report();
        if (report) {
            printf("%s", report);
            free(report);
        }
        return 0;
    }

    if (strcmp(argv[1], "set") == 0) {
        if (argc < 3) {
            printf("Usage: cost set <amount_usd>\n");
            printf("Example: cost set 10.00\n");
            return -1;
        }
        double budget = atof(argv[2]);
        if (budget <= 0) {
            printf("Invalid budget amount.\n");
            return -1;
        }
        cost_set_budget(budget);
        printf("Budget set to $%.2f\n", budget);
        return 0;
    }

    if (strcmp(argv[1], "reset") == 0) {
        cost_reset_session();
        printf("Session spending reset.\n");
        return 0;
    }

    printf("Unknown cost command: %s\n", argv[1]);
    printf("Try: cost, cost report, cost set <amount>, cost reset\n");
    return -1;
}

