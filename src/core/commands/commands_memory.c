/**
 * CONVERGIO KERNEL - Memory Commands
 *
 * Semantic memory and git/test workflow commands
 */

#include "commands_internal.h"

// SEMANTIC MEMORY COMMANDS
// ============================================================================

/**
 * /remember <text> - Store a memory with high importance
 */
int cmd_remember(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: remember <text to remember>\n");
        printf("Example: remember Roberto prefers clean, readable code\n");
        return -1;
    }

    // Join all arguments into one string
    char content[2048] = {0};
    size_t len = 0;
    for (int i = 1; i < argc && len < sizeof(content) - 2; i++) {
        if (i > 1)
            content[len++] = ' ';
        size_t arg_len = strlen(argv[i]);
        if (len + arg_len < sizeof(content) - 1) {
            memcpy(content + len, argv[i], arg_len);
            len += arg_len;
        }
    }

    // Create semantic node
    SemanticID id = nous_create_node(SEMANTIC_TYPE_MEMORY, content);
    if (id == SEMANTIC_ID_NULL) {
        printf("\033[31mError: Failed to store memory.\033[0m\n");
        return -1;
    }

    // Set high importance for explicitly remembered items (both DB and in-memory)
    sem_persist_update_importance(id, 0.9f);
    NousSemanticNode* node = nous_get_node(id);
    if (node) {
        node->importance = 0.9f;
        nous_release_node(node);
    }

    printf("\033[32m✓ Remembered:\033[0m \"%s\"\n", content);
    printf("\033[90mMemory ID: 0x%llx\033[0m\n", (unsigned long long)id);

    return 0;
}

/**
 * /search <query> - Search memories semantically
 */
int cmd_search(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: search <search query>\n");
        printf("Example: search what does Roberto prefer\n");
        return -1;
    }

    // Join all arguments into search query
    char query[1024] = {0};
    size_t len = 0;
    for (int i = 1; i < argc && len < sizeof(query) - 2; i++) {
        if (i > 1)
            query[len++] = ' ';
        size_t arg_len = strlen(argv[i]);
        if (len + arg_len < sizeof(query) - 1) {
            memcpy(query + len, argv[i], arg_len);
            len += arg_len;
        }
    }

    // Search by essence (keyword search for now)
    size_t count = 0;
    SemanticID* results = sem_persist_search_essence(query, 10, &count);

    if (!results || count == 0) {
        printf("\033[33mNo memories found for:\033[0m \"%s\"\n", query);
        return 0;
    }

    printf("\033[1mFound %zu matching memories:\033[0m\n\n", count);

    for (size_t i = 0; i < count; i++) {
        NousSemanticNode* node = nous_get_node(results[i]);
        if (node && node->essence) {
            printf("  \033[36m[%zu]\033[0m %s\n", i + 1, node->essence);
            printf("      \033[90mID: 0x%llx | Importance: %.2f\033[0m\n",
                   (unsigned long long)node->id, (double)node->importance);
            nous_release_node(node);
        }
    }

    free(results);
    return 0;
}

/**
 * /memories - List recent and important memories
 */
int cmd_memories(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Get graph statistics
    GraphStats stats = sem_persist_get_stats();

    printf("\033[1m📚 Knowledge Graph\033[0m\n");
    printf("   Total nodes: %zu\n", stats.total_nodes);
    printf("   Total relations: %zu\n", stats.total_relations);
    printf("   Nodes in memory: %zu\n", stats.nodes_in_memory);
    printf("\n");

    // Load most important memories
    size_t count = 0;
    SemanticID* important = sem_persist_load_important(10, 0.5f, &count);

    if (important && count > 0) {
        printf("\033[1m⭐ Most Important Memories:\033[0m\n\n");
        for (size_t i = 0; i < count; i++) {
            NousSemanticNode* node = nous_get_node(important[i]);
            if (node && node->essence) {
                // Truncate long essences
                char display[80];
                if (strlen(node->essence) > 75) {
                    snprintf(display, sizeof(display), "%.72s...", node->essence);
                } else {
                    snprintf(display, sizeof(display), "%s", node->essence);
                }
                printf("  \033[36m[%zu]\033[0m %s\n", i + 1, display);
                printf("      \033[90mImportance: %.2f | Accessed: %llu times\033[0m\n",
                       (double)node->importance, (unsigned long long)node->access_count);
                nous_release_node(node);
            }
        }
        free(important);
    } else {
        printf("\033[33mNo memories stored yet.\033[0m\n");
        printf("Use \033[1mremember <text>\033[0m to store your first memory!\n");
    }

    return 0;
}

/**
 * /forget <id> - Delete a memory by ID
 */
int cmd_forget(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: forget <memory_id>\n");
        printf("Example: forget 0x1234567890abcdef\n");
        printf("\nUse 'memories' or 'recall' to find memory IDs.\n");
        return -1;
    }

    // Parse hex ID with proper error checking
    char* endptr = NULL;
    const char* num_str = argv[1];
    if (strncmp(num_str, "0x", 2) == 0 || strncmp(num_str, "0X", 2) == 0) {
        num_str += 2;
    }

    errno = 0;
    SemanticID id = strtoull(num_str, &endptr, 16);

    // Check for parsing errors: no digits parsed, or trailing garbage
    if (endptr == num_str || *endptr != '\0' || errno == ERANGE) {
        printf("\033[31mError: Invalid memory ID format. Use hex format like 0x1234.\033[0m\n");
        return -1;
    }

    // Note: id == 0 is technically valid but very unlikely to be a real ID
    if (id == 0) {
        printf("\033[33mWarning: ID 0 is unusual. Proceeding anyway.\033[0m\n");
    }

    // Check if it exists
    if (!sem_persist_node_exists(id)) {
        printf("\033[31mError: Memory 0x%llx not found.\033[0m\n", (unsigned long long)id);
        return -1;
    }

    // Delete from persistence
    int rc = sem_persist_delete_node(id);
    if (rc != 0) {
        printf("\033[31mError: Failed to delete memory.\033[0m\n");
        return -1;
    }

    // Also delete from in-memory fabric (if loaded)
    nous_delete_node(id);

    printf("\033[32m✓ Forgotten memory 0x%llx\033[0m\n", (unsigned long long)id);
    return 0;
}

/**
 * /graph - Show knowledge graph statistics
 */
int cmd_graph(int argc, char** argv) {
    (void)argc;
    (void)argv;

    GraphStats stats = sem_persist_get_stats();

    printf("\033[1m🧠 Semantic Knowledge Graph\033[0m\n\n");

    printf("  \033[36mNodes\033[0m\n");
    printf("    Total in database:    %zu\n", stats.total_nodes);
    printf("    Loaded in memory:     %zu\n", stats.nodes_in_memory);
    printf("\n");

    printf("  \033[36mRelations\033[0m\n");
    printf("    Total connections:    %zu\n", stats.total_relations);
    printf("\n");

    printf("  \033[36mNodes by Type\033[0m\n");
    const char* type_names[] = {"Void",  "Concept", "Entity",  "Relation", "Intent", "Agent",
                                "Space", "Event",   "Feeling", "Memory",   "Pattern"};
    for (int i = 0; i < 11 && i < 16; i++) {
        if (stats.nodes_by_type[i] > 0) {
            printf("    %-12s: %zu\n", type_names[i], stats.nodes_by_type[i]);
        }
    }

    return 0;
}

// ============================================================================
// GIT/TEST WORKFLOW COMMANDS (Issue #15)
// ============================================================================

/**
 * /test - Run project tests with auto-detection of test framework
 */
int cmd_test(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Check what test frameworks/files exist
    bool has_makefile = (access("Makefile", F_OK) == 0);
    bool has_package_json = (access("package.json", F_OK) == 0);
    bool has_cargo_toml = (access("Cargo.toml", F_OK) == 0);
    bool has_go_mod = (access("go.mod", F_OK) == 0);
    bool has_pytest = (access("pytest.ini", F_OK) == 0 || access("pyproject.toml", F_OK) == 0);
    bool has_tests_dir = (access("tests", F_OK) == 0);

    const char* cmd = NULL;
    const char* framework = NULL;

    // Priority: explicit test directory structure first
    if (has_makefile) {
        // Check if make test target exists
        FILE* fp = popen("make -n test 2>/dev/null", "r");
        if (fp) {
            char buf[256];
            if (fgets(buf, sizeof(buf), fp) != NULL) {
                cmd = "make test";
                framework = "make";
            }
            pclose(fp);
        }
    }

    if (!cmd && has_cargo_toml) {
        cmd = "cargo test";
        framework = "cargo";
    }

    if (!cmd && has_go_mod) {
        cmd = "go test ./...";
        framework = "go";
    }

    if (!cmd && has_package_json) {
        cmd = "npm test";
        framework = "npm";
    }

    if (!cmd && (has_pytest || has_tests_dir)) {
        cmd = "python3 -m pytest -v";
        framework = "pytest";
    }

    if (!cmd) {
        printf("\033[33m⚠ No test framework detected.\033[0m\n\n");
        printf("Supported frameworks:\n");
        printf("  • make test     (Makefile with 'test' target)\n");
        printf("  • cargo test    (Rust - Cargo.toml)\n");
        printf("  • go test       (Go - go.mod)\n");
        printf("  • npm test      (Node.js - package.json)\n");
        printf("  • pytest        (Python - pytest.ini/pyproject.toml/tests/)\n");
        return -1;
    }

    printf("\033[1;36m🧪 Running tests with %s\033[0m\n", framework);
    printf("  Command: %s\n\n", cmd);

    int result = system(cmd);

    printf("\n");
    if (result == 0) {
        printf("\033[32m✓ Tests passed!\033[0m\n");
    } else {
        printf("\033[31m✗ Tests failed (exit code: %d)\033[0m\n", WEXITSTATUS(result));
    }

    return result == 0 ? 0 : -1;
}

/**
 * /git - Git workflow helper
 */
int cmd_git(int argc, char** argv) {
    // Check if we're in a git repo
    if (access(".git", F_OK) != 0) {
        printf("\033[31mError: Not in a git repository.\033[0m\n");
        return -1;
    }

    bool has_gh = (system("which gh >/dev/null 2>&1") == 0);
    const char* subcommand = (argc > 1) ? argv[1] : "status";

    if (strcmp(subcommand, "status") == 0 || strcmp(subcommand, "s") == 0) {
        printf("\033[1;36m📊 Git Status\033[0m\n\n");
        system("git status --short --branch");
        printf("\n\033[36mRecent commits:\033[0m\n");
        system("git log --oneline -5");
        return 0;
    }

    if (strcmp(subcommand, "commit") == 0 || strcmp(subcommand, "c") == 0) {
        if (argc < 3) {
            printf("Usage: git commit <message>\n");
            return -1;
        }

        char msg[1024] = {0};
        for (int i = 2; i < argc; i++) {
            if (i > 2)
                strncat(msg, " ", sizeof(msg) - strlen(msg) - 1);
            strncat(msg, argv[i], sizeof(msg) - strlen(msg) - 1);
        }

        printf("\033[36mStaging changes...\033[0m\n");
        system("git add -A");

        int status = system("git diff --cached --quiet");
        if (status == 0) {
            printf("\033[33mNo changes to commit.\033[0m\n");
            return 0;
        }

        char cmd_buf[2048];
        snprintf(cmd_buf, sizeof(cmd_buf),
                 "git commit -m \"%s\" -m \"\" -m \"🤖 Generated with [Claude "
                 "Code](https://claude.com/claude-code)\" -m \"Co-Authored-By: Claude Opus 4.5 "
                 "<noreply@anthropic.com>\"",
                 msg);

        printf("\033[36mCommitting...\033[0m\n");
        int result = system(cmd_buf);
        if (result == 0) {
            printf("\033[32m✓ Committed!\033[0m\n");
        }
        return result == 0 ? 0 : -1;
    }

    if (strcmp(subcommand, "push") == 0 || strcmp(subcommand, "p") == 0) {
        printf("\033[36mPushing...\033[0m\n");
        int result = system("git push");
        if (result == 0)
            printf("\033[32m✓ Pushed!\033[0m\n");
        return result == 0 ? 0 : -1;
    }

    if (strcmp(subcommand, "sync") == 0) {
        printf("\033[36mSyncing...\033[0m\n");
        int result = system("git pull --rebase && git push");
        if (result == 0)
            printf("\033[32m✓ Synced!\033[0m\n");
        return result == 0 ? 0 : -1;
    }

    printf("\033[1;36m📦 Git Workflow\033[0m\n\n");
    printf("Subcommands:\n");
    printf("  status, s       Show status and recent commits\n");
    printf("  commit, c <msg> Stage all and commit\n");
    printf("  push, p         Push to remote\n");
    printf("  sync            Pull --rebase and push\n");
    if (has_gh)
        printf("\nFor PRs: /pr <title>\n");
    return 0;
}

/**
 * /pr - Create pull request via gh CLI
 */
int cmd_pr(int argc, char** argv) {
    if (system("which gh >/dev/null 2>&1") != 0) {
        printf("\033[31mError: 'gh' CLI not installed.\033[0m\n");
        printf("Install: brew install gh && gh auth login\n");
        return -1;
    }

    if (access(".git", F_OK) != 0) {
        printf("\033[31mError: Not in a git repository.\033[0m\n");
        return -1;
    }

    FILE* fp = popen("git branch --show-current", "r");
    if (!fp)
        return -1;
    char branch[256] = {0};
    if (fgets(branch, sizeof(branch), fp)) {
        branch[strcspn(branch, "\n")] = 0;
    }
    pclose(fp);

    if (strcmp(branch, "main") == 0 || strcmp(branch, "master") == 0) {
        printf("\033[31mError: Cannot create PR from %s.\033[0m\n", branch);
        printf("Create a feature branch first.\n");
        return -1;
    }

    char title[512] = {0};
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (i > 1)
                strncat(title, " ", sizeof(title) - strlen(title) - 1);
            strncat(title, argv[i], sizeof(title) - strlen(title) - 1);
        }
    } else {
        strncpy(title, branch, sizeof(title) - 1);
        for (char* p = title; *p; p++) {
            if (*p == '-' || *p == '_')
                *p = ' ';
        }
    }

    printf("\033[1;36m🔀 Creating PR\033[0m\n");
    printf("  Branch: %s\n  Title: %s\n\n", branch, title);

    char push_cmd[512];
    snprintf(push_cmd, sizeof(push_cmd), "git push -u origin %s 2>&1", branch);
    system(push_cmd);

    char pr_cmd[2048];
    snprintf(pr_cmd, sizeof(pr_cmd),
             "gh pr create --title \"%s\" --body \"## Summary\\n\\n## Test Plan\\n- [ ] Tests "
             "pass\\n\\n🤖 Generated with [Claude Code](https://claude.com/claude-code)\"",
             title);

    int result = system(pr_cmd);
    if (result == 0)
        printf("\n\033[32m✓ PR created!\033[0m\n");
    return result == 0 ? 0 : -1;
}

// ============================================================================
