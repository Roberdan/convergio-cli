/**
 * CONVERGIO KERNEL - Agent Commands
 *
 * Agent management and space commands
 */

#include "commands_internal.h"

// ============================================================================
// AGENT COMMANDS
// ============================================================================

int cmd_agents(int argc, char** argv) {
    (void)argc;

    // Check for subcommands
    if (argc >= 2) {
        if (strcmp(argv[1], "working") == 0 || strcmp(argv[1], "active") == 0) {
            // Show only working agents
            char* working = agent_get_working_status();
            if (working) {
                printf("\n%s\n", working);
                free(working);
            }
            return 0;
        }
    }

    // Show working status first
    char* working = agent_get_working_status();
    if (working) {
        printf("\n%s", working);
        free(working);
    }

    // Then show full registry
    printf("\n");
    char* status = agent_registry_status();
    if (status) {
        printf("%s", status);
        free(status);
    }
    return 0;
}

int cmd_agent(int argc, char** argv) {
    if (argc < 2) {
        printf("\n\033[1mCommand: agent\033[0m - Agent management\n\n");
        printf("\033[1mSubcommands:\033[0m\n");
        printf("  \033[36mlist\033[0m                    List all available agents\n");
        printf("  \033[36minfo <name>\033[0m             Show agent details (model, role, etc.)\n");
        printf("  \033[36medit <name>\033[0m             Open agent in editor to modify\n");
        printf("  \033[36mreload\033[0m                  Reload all agents after changes\n");
        printf("  \033[36mcreate <name> <desc>\033[0m    Create a new dynamic agent\n");
        printf("  \033[36mskill <skill_name>\033[0m      Add skill to assistant\n");
        printf("\n\033[1mExamples:\033[0m\n");
        printf("  agent list              # Show all agents\n");
        printf("  agent info baccio       # Details about Baccio\n");
        printf("  agent edit amy          # Edit Amy in your editor\n");
        printf("  agent reload            # Reload after changes\n");
        printf("  agent create helper \"A generic assistant\"\n");
        printf("\n");
        return 0;
    }

    // agent list
    if (strcmp(argv[1], "list") == 0) {
        char* status = agent_registry_status();
        if (status) {
            printf("\n%s", status);
            free(status);
        }
        return 0;
    }

    // agent info <name>
    if (strcmp(argv[1], "info") == 0) {
        if (argc < 3) {
            printf("Usage: agent info <agent_name>\n");
            printf("Example: agent info baccio\n");
            return -1;
        }

        ManagedAgent* agent = agent_find_by_name(argv[2]);
        if (!agent) {
            printf("Agent '%s' not found.\n", argv[2]);
            printf("Use 'agent list' to see available agents.\n");
            return -1;
        }

        printf("\n\033[1m📋 Agent Info: %s\033[0m\n\n", agent->name);
        printf("  \033[36mName:\033[0m        %s\n", agent->name);
        printf("  \033[36mDescription:\033[0m %s\n", agent->description ? agent->description : "-");

        const char* role_names[] = {"Orchestrator", "Analyst", "Coder",    "Writer",
                                    "Critic",       "Planner", "Executor", "Memory"};
        printf("  \033[36mRole:\033[0m        %s\n", role_names[agent->role]);

        // Model is determined by role for now
        const char* model = "claude-sonnet-4-20250514"; // Default
        if (agent->role == AGENT_ROLE_ORCHESTRATOR) {
            model = "claude-opus-4-20250514";
        }
        printf("  \033[36mModel:\033[0m       %s\n", model);

        printf("  \033[36mActive:\033[0m      %s\n", agent->is_active ? "Yes" : "No");

        const char* state_names[] = {"Idle", "Thinking", "Executing", "Reviewing", "Waiting"};
        printf("  \033[36mState:\033[0m       %s\n", state_names[agent->work_state]);

        if (agent->current_task) {
            printf("  \033[36mTask:\033[0m        %s\n", agent->current_task);
        }

        printf("\n  \033[2mUse @%s <message> to communicate with this agent\033[0m\n\n",
               agent->name);
        return 0;
    }

    // agent edit <name>
    if (strcmp(argv[1], "edit") == 0) {
        if (argc < 3) {
            printf("Usage: agent edit <agent_name>\n");
            printf("Example: agent edit amy\n");
            return -1;
        }

        const char* agent_name = argv[2];

        // Build path to agent definition file
        char path[PATH_MAX];
        bool found = false;

        // First try exact match
        snprintf(path, sizeof(path), "src/agents/definitions/%s.md", agent_name);
        if (access(path, F_OK) == 0) {
            found = true;
        }

        // Try to find by prefix (e.g., "amy" -> "amy-cfo.md")
        if (!found) {
            DIR* dir = opendir("src/agents/definitions");
            if (dir) {
                struct dirent* entry;
                size_t name_len = strlen(agent_name);
                while ((entry = readdir(dir)) != NULL) {
                    if (strncasecmp(entry->d_name, agent_name, name_len) == 0 &&
                        (entry->d_name[name_len] == '-' || entry->d_name[name_len] == '.')) {
                        snprintf(path, sizeof(path), "src/agents/definitions/%s", entry->d_name);
                        found = true;
                        break;
                    }
                }
                closedir(dir);
            }
        }

        if (!found) {
            printf("\033[31mAgent '%s' not found.\033[0m\n", agent_name);
            printf("Use 'agent list' to see available agents.\n");
            return -1;
        }

        // Get editor from environment
        const char* editor = getenv("EDITOR");
        if (!editor)
            editor = getenv("VISUAL");

        char cmd[PATH_MAX + 64];

        if (editor) {
            // Use $EDITOR
            snprintf(cmd, sizeof(cmd), "%s \"%s\"", editor, path);
        } else {
            // macOS: use 'open' command
            snprintf(cmd, sizeof(cmd), "open \"%s\"", path);
        }

        printf("\033[36mOpening %s...\033[0m\n", path);
        int result = system(cmd);

        if (result != 0) {
            printf("\033[31mFailed to open editor.\033[0m\n");
            return -1;
        }

        printf("\n\033[33mAfter editing, run 'agent reload' to apply changes.\033[0m\n");
        return 0;
    }

    // agent reload
    if (strcmp(argv[1], "reload") == 0) {
        printf("\033[36mReloading agent definitions...\033[0m\n");

        // Re-run the embed script if available
        if (access("scripts/embed_agents.sh", X_OK) == 0) {
            printf("  Running embed_agents.sh...\n");
            int result = system("./scripts/embed_agents.sh");
            if (result != 0) {
                printf("\033[31mFailed to regenerate embedded agents.\033[0m\n");
                printf("You may need to rebuild: make clean && make\n");
                return -1;
            }
            printf("\033[32m✓ Agents regenerated.\033[0m\n");
            printf("\n\033[33mNote: Rebuild required to apply changes: make\033[0m\n");
        } else {
            printf("\033[33mNo embed script found. Manual rebuild required.\033[0m\n");
            printf("Run: make clean && make\n");
        }
        return 0;
    }

    // agent create <name> <essence>
    if (strcmp(argv[1], "create") == 0) {
        if (argc < 4) {
            printf("Usage: agent create <name> <description>\n");
            printf("Example: agent create helper \"A generic task assistant\"\n");
            return -1;
        }

        char essence[512] = {0};
        size_t ess_len = 0;
        for (int i = 3; i < argc && ess_len < sizeof(essence) - 2; i++) {
            if (i > 3 && ess_len < sizeof(essence) - 1) {
                essence[ess_len++] = ' ';
            }
            size_t arg_len = strlen(argv[i]);
            size_t copy_len = (ess_len + arg_len < sizeof(essence) - 1)
                                  ? arg_len
                                  : (sizeof(essence) - 1 - ess_len);
            memcpy(essence + ess_len, argv[i], copy_len);
            ess_len += copy_len;
        }
        essence[ess_len] = '\0';

        NousAgent* agent = nous_create_agent(argv[2], essence);
        if (!agent) {
            printf("Error: unable to create agent.\n");
            return -1;
        }

        printf("Created agent \"%s\"\n", agent->name);
        printf("  Patience: %.2f\n", (double)agent->patience);
        printf("  Creativity: %.2f\n", (double)agent->creativity);
        printf("  Assertiveness: %.2f\n", (double)agent->assertiveness);

        NousAgent* assistant = (NousAgent*)g_assistant;
        if (!assistant) {
            g_assistant = agent;
            printf("Set as primary assistant.\n");
        }

        return 0;
    }

    // agent skill <skill_name>
    if (strcmp(argv[1], "skill") == 0) {
        if (argc < 3) {
            printf("Usage: agent skill <skill_name>\n");
            return -1;
        }
        NousAgent* assistant = (NousAgent*)g_assistant;
        if (!assistant) {
            printf("Error: no active assistant.\n");
            printf("Create an agent first with: agent create <name> <description>\n");
            return -1;
        }

        if (nous_agent_add_skill(assistant, argv[2]) == 0) {
            printf("Added skill \"%s\" to %s\n", argv[2], assistant->name);
        }
        return 0;
    }

    printf("Unknown subcommand: %s\n", argv[1]);
    printf("Use 'agent' without arguments to see help.\n");
    return -1;
}

static void on_thought(NousAgent* agent, const char* thought, void* ctx) {
    (void)ctx;
    printf("\n%s: %s\n\n", agent->name, thought);
}

int cmd_think(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: think <intent>\n");
        return -1;
    }

    NousAgent* assistant = (NousAgent*)g_assistant;
    if (!assistant) {
        printf("No assistant available. Create one with: agent create <name> <essence>\n");
        return -1;
    }

    // Join all arguments as intent (with bounds checking)
    char input[1024] = {0};
    size_t input_len = 0;
    for (int i = 1; i < argc && input_len < sizeof(input) - 2; i++) {
        if (i > 1 && input_len < sizeof(input) - 1) {
            input[input_len++] = ' ';
        }
        size_t arg_len = strlen(argv[i]);
        size_t copy_len =
            (input_len + arg_len < sizeof(input) - 1) ? arg_len : (sizeof(input) - 1 - input_len);
        memcpy(input + input_len, argv[i], copy_len);
        input_len += copy_len;
    }
    input[input_len] = '\0';

    // Parse intent
    ParsedIntent* intent = nous_parse_intent(input, strlen(input));
    if (!intent) {
        printf("Failed to parse intent.\n");
        return -1;
    }

    printf("Intent parsed:\n");
    printf("  Kind: %d\n", intent->kind);
    printf("  Confidence: %.2f\n", (double)intent->confidence);
    printf("  Urgency: %.2f\n", (double)intent->urgency);

    if (intent->question_count > 0) {
        printf("\nClarification needed:\n");
        for (size_t i = 0; i < intent->question_count; i++) {
            printf("  - %s\n", intent->questions[i]);
        }
    }

    // Have assistant think about it
    extern int nous_agent_think(NousAgent*, ParsedIntent*, void (*)(NousAgent*, const char*, void*),
                                void*);
    nous_agent_think(assistant, intent, on_thought, NULL);

    return 0;
}

// ============================================================================
// SPACE COMMANDS
// ============================================================================

int cmd_create(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: create <essence>\n");
        printf("Example: create \"un concetto di bellezza\"\n");
        return -1;
    }

    // Join all arguments as the essence (with bounds checking)
    char essence[1024] = {0};
    size_t ess_len = 0;
    for (int i = 1; i < argc && ess_len < sizeof(essence) - 2; i++) {
        if (i > 1 && ess_len < sizeof(essence) - 1) {
            essence[ess_len++] = ' ';
        }
        size_t arg_len = strlen(argv[i]);
        size_t copy_len =
            (ess_len + arg_len < sizeof(essence) - 1) ? arg_len : (sizeof(essence) - 1 - ess_len);
        memcpy(essence + ess_len, argv[i], copy_len);
        ess_len += copy_len;
    }
    essence[ess_len] = '\0';

    SemanticID id = nous_create_node(SEMANTIC_TYPE_CONCEPT, essence);
    if (id == SEMANTIC_ID_NULL) {
        printf("Failed to create node.\n");
        return -1;
    }

    printf("Created semantic node: 0x%016llx\n", (unsigned long long)id);
    printf("Essence: \"%s\"\n", essence);

    return 0;
}

int cmd_space(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: space <create|join|leave|list> [args]\n");
        return -1;
    }

    if (strcmp(argv[1], "create") == 0) {
        if (argc < 4) {
            printf("Usage: space create <name> <purpose>\n");
            return -1;
        }

        char purpose[512] = {0};
        size_t purp_len = 0;
        for (int i = 3; i < argc && purp_len < sizeof(purpose) - 2; i++) {
            if (i > 3 && purp_len < sizeof(purpose) - 1) {
                purpose[purp_len++] = ' ';
            }
            size_t arg_len = strlen(argv[i]);
            size_t copy_len = (purp_len + arg_len < sizeof(purpose) - 1)
                                  ? arg_len
                                  : (sizeof(purpose) - 1 - purp_len);
            memcpy(purpose + purp_len, argv[i], copy_len);
            purp_len += copy_len;
        }
        purpose[purp_len] = '\0';

        NousSpace* space = nous_create_space(argv[2], purpose);
        if (!space) {
            printf("Failed to create space.\n");
            return -1;
        }

        printf("Created space \"%s\"\n", space->name);
        printf("  Purpose: %s\n", space->purpose);

        g_current_space = space;
        printf("Entered space.\n");

        // Auto-join assistant if exists
        NousAgent* assistant = (NousAgent*)g_assistant;
        if (assistant) {
            nous_join_space(assistant->id, space->id);
            printf("Assistant joined space.\n");
        }

        return 0;
    }

    NousSpace* space = (NousSpace*)g_current_space;
    if (strcmp(argv[1], "urgency") == 0 && space) {
        printf("Current urgency: %.2f\n", (double)nous_space_urgency(space));
        return 0;
    }

    printf("Unknown space command: %s\n", argv[1]);
    return -1;
}

// ============================================================================
