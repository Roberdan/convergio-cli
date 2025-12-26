/**
 * CONVERGIO KERNEL - System Commands
 *
 * Debug, workspace, authentication, style, compare, telemetry, and tools commands
 */

#include "commands_internal.h"

// ============================================================================
// DEBUG COMMANDS
// ============================================================================

int cmd_debug(int argc, char** argv) {
    if (argc < 2) {
        // Show current level and toggle
        LogLevel current = nous_log_get_level();
        if (current == LOG_LEVEL_NONE) {
            nous_log_set_level(LOG_LEVEL_INFO);
            printf("\033[32m✓ Debug mode enabled (level: INFO)\033[0m\n");
            printf("  Use 'debug <level>' to change: off, error, warn, info, debug, trace\n");
        } else {
            nous_log_set_level(LOG_LEVEL_NONE);
            printf("\033[33m✗ Debug mode disabled\033[0m\n");
        }
        return 0;
    }

    // Parse level argument
    LogLevel new_level = LOG_LEVEL_NONE;
    const char* level_arg = argv[1];

    if (strcmp(level_arg, "off") == 0 || strcmp(level_arg, "none") == 0) {
        new_level = LOG_LEVEL_NONE;
    } else if (strcmp(level_arg, "error") == 0) {
        new_level = LOG_LEVEL_ERROR;
    } else if (strcmp(level_arg, "warn") == 0 || strcmp(level_arg, "warning") == 0) {
        new_level = LOG_LEVEL_WARN;
    } else if (strcmp(level_arg, "info") == 0) {
        new_level = LOG_LEVEL_INFO;
    } else if (strcmp(level_arg, "debug") == 0) {
        new_level = LOG_LEVEL_DEBUG;
    } else if (strcmp(level_arg, "trace") == 0 || strcmp(level_arg, "all") == 0) {
        new_level = LOG_LEVEL_TRACE;
    } else {
        printf("Unknown debug level: %s\n", level_arg);
        printf("Valid levels: off, error, warn, info, debug, trace\n");
        return -1;
    }

    nous_log_set_level(new_level);

    if (new_level == LOG_LEVEL_NONE) {
        printf("\033[33m✗ Debug mode disabled\033[0m\n");
    } else {
        printf("\033[32m✓ Debug level set to: %s\033[0m\n", nous_log_level_name(new_level));
    }

    return 0;
}

// ============================================================================
// WORKSPACE/SANDBOX COMMANDS
// ============================================================================

int cmd_allow_dir(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: allow-dir <path>\n");
        printf("Add a directory to the sandbox (allows file operations)\n");
        return -1;
    }

    // Resolve to absolute path
    char resolved[PATH_MAX];
    if (!realpath(argv[1], resolved)) {
        printf("Error: Path not found: %s\n", argv[1]);
        return -1;
    }

    // Check if it's a system path (block for safety)
    const char* blocked_prefixes[] = {"/System", "/usr",         "/bin",         "/sbin",    "/etc",
                                      "/var",    "/private/etc", "/private/var", "/Library", NULL};

    for (int i = 0; blocked_prefixes[i]; i++) {
        if (strncmp(resolved, blocked_prefixes[i], strlen(blocked_prefixes[i])) == 0) {
            printf("Error: Cannot add system paths for security reasons\n");
            return -1;
        }
    }

    // Add to allowed paths
    tools_add_allowed_path(resolved);
    printf("\033[32m✓ Added to sandbox: %s\033[0m\n", resolved);

    return 0;
}

int cmd_allowed_dirs(int argc, char** argv) {
    (void)argc;
    (void)argv;

    size_t count = 0;
    const char** paths = tools_get_allowed_paths(&count);

    printf("\n\033[1mAllowed Directories (Sandbox)\033[0m\n");
    printf("================================\n");

    if (count == 0) {
        printf("  (none - workspace not initialized)\n");
    } else {
        for (size_t i = 0; i < count; i++) {
            if (i == 0) {
                printf("  \033[32m✓\033[0m %s \033[2m(workspace)\033[0m\n", paths[i]);
            } else {
                printf("  \033[32m✓\033[0m %s\n", paths[i]);
            }
        }
    }

    printf("\nUse 'allow-dir <path>' to add more directories.\n\n");

    return 0;
}

// ============================================================================
// AUTHENTICATION COMMANDS
// ============================================================================

int cmd_logout(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if (auth_get_mode() == AUTH_MODE_NONE) {
        printf("Not currently authenticated.\n");
        return 0;
    }

    AuthMode prev_mode = auth_get_mode();
    auth_logout();

    if (prev_mode == AUTH_MODE_OAUTH) {
        printf("\033[32m✓ Logged out from Claude Max.\033[0m\n");
        printf("OAuth tokens have been removed from Keychain.\n");
    }

    // Check if API key is available as fallback
    if (auth_get_mode() == AUTH_MODE_API_KEY) {
        printf("\nNow using API key authentication (ANTHROPIC_API_KEY).\n");
    } else {
        printf("\nNo authentication configured.\n");
        printf("Run 'convergio setup' or set ANTHROPIC_API_KEY environment variable.\n");
    }

    return 0;
}

int cmd_auth(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n\033[1mAuthentication Status\033[0m\n");
    printf("=====================\n\n");

    char* status = auth_get_status_string();
    if (status) {
        AuthMode mode = auth_get_mode();
        const char* mode_name;
        switch (mode) {
        case AUTH_MODE_API_KEY:
            mode_name = "API Key";
            break;
        case AUTH_MODE_OAUTH:
            mode_name = "Claude Max (OAuth)";
            break;
        default:
            mode_name = "None";
            break;
        }

        printf("  Mode:   %s\n", mode_name);
        printf("  Status: %s\n", status);

        if (mode == AUTH_MODE_OAUTH) {
            printf("\n  \033[2mTokens stored in macOS Keychain\033[0m\n");
        } else if (mode == AUTH_MODE_API_KEY) {
            printf("\n  \033[2mUsing ANTHROPIC_API_KEY environment variable\033[0m\n");
        }

        free(status);
    } else {
        printf("  Not authenticated\n");
    }

    printf("\n");
    return 0;
}

// ============================================================================
// SYSTEM COMMANDS
// ============================================================================

int cmd_update(int argc, char** argv) {
    if (argc >= 2 && strcmp(argv[1], "install") == 0) {
        return convergio_cmd_update_install();
    }
    if (argc >= 2 && strcmp(argv[1], "changelog") == 0) {
        return convergio_cmd_update_changelog();
    }
    // Default: check for updates
    return convergio_cmd_update_check();
}

int cmd_hardware(int argc, char** argv) {
    (void)argc;
    (void)argv;
    convergio_print_hardware_info();
    return 0;
}

int cmd_news(int argc, char** argv) {
    UpdateInfo info;
    const char* version = NULL;

    // Optional version argument: news v3.0.3 or news 3.0.3
    if (argc > 1) {
        version = argv[1];
    }

    // Fetch release info (NULL = latest, otherwise specific version)
    if (convergio_fetch_release(version, &info) != 0) {
        if (version) {
            printf("\033[31mError:\033[0m Could not fetch release notes for version %s\n", version);
            printf("  Make sure the version exists (e.g., 3.0.4 or v3.0.4)\n");
        } else {
            printf("\033[31mError:\033[0m Could not fetch latest release notes\n");
        }
        return -1;
    }

    // Print release notes in a nice box
    printf("\n");
    printf("╭─ \033[1;36mConvergio v%s\033[0m ", info.latest_version);

    // Pad the header line
    int header_len = 15 + (int)strlen(info.latest_version);
    for (int i = header_len; i < 54; i++)
        printf("─");
    printf("╮\n");

    if (info.is_prerelease) {
        printf("│  \033[33m⚠ Pre-release\033[0m                                       │\n");
    }

    if (info.published_at[0]) {
        // Format: 2025-12-12T... -> 2025-12-12
        char date[11] = {0};
        strncpy(date, info.published_at, 10);
        printf("│  Released: %s                                  │\n", date);
    }

    printf("├──────────────────────────────────────────────────────┤\n");

    if (strlen(info.release_notes) > 0) {
        // Print release notes with word wrap
        const char* p = info.release_notes;
        while (*p) {
            printf("│  ");
            int col = 0;
            while (*p && *p != '\n' && col < 52) {
                // Handle \r\n
                if (*p == '\r') {
                    p++;
                    continue;
                }
                putchar(*p);
                p++;
                col++;
            }
            while (col < 52) {
                putchar(' ');
                col++;
            }
            printf(" │\n");
            if (*p == '\n')
                p++;
        }
    } else {
        printf("│  No release notes available.                         │\n");
    }

    printf("╰──────────────────────────────────────────────────────╯\n");
    printf("\n");

    return 0;
}

int cmd_stream(int argc, char** argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "on") == 0) {
            g_streaming_enabled = true;
            printf("Streaming mode: \033[32mON\033[0m\n");
            printf("  Live markdown rendering enabled\n");
            printf("  Note: Tool calls are disabled in streaming mode\n");
        } else if (strcmp(argv[1], "off") == 0) {
            g_streaming_enabled = false;
            printf("Streaming mode: \033[2mOFF\033[0m\n");
            printf("  Full tool support enabled, responses wait until complete\n");
        } else {
            printf("Usage: stream [on|off]\n");
        }
    } else {
        // Toggle
        g_streaming_enabled = !g_streaming_enabled;
        if (g_streaming_enabled) {
            printf("Streaming mode: \033[32mON\033[0m\n");
            printf("  Live markdown rendering enabled\n");
            printf("  Note: Tool calls are disabled in streaming mode\n");
        } else {
            printf("Streaming mode: \033[2mOFF\033[0m\n");
            printf("  Full tool support enabled, responses wait until complete\n");
        }
    }
    return 0;
}

int cmd_theme(int argc, char** argv) {
    if (argc > 1) {
        if (theme_set_by_name(argv[1])) {
            const Theme* t = theme_get();
            printf("Theme changed to: %s%s%s\n", t->prompt_name, t->name, theme_reset());
            theme_save();
        } else {
            printf("Unknown theme: %s\n", argv[1]);
            theme_list();
        }
    } else {
        // Interactive theme selector with arrow keys and preview
        ThemeId selected = theme_select_interactive();
        if (selected != theme_get_current_id()) {
            theme_set(selected);
            theme_save();
            const Theme* t = theme_get();
            printf("Theme changed to: %s%s%s\n", t->prompt_name, t->name, theme_reset());
        } else {
            printf("Theme unchanged: %s\n", theme_get_name(selected));
        }
    }
    return 0;
}

// ============================================================================
// RESPONSE STYLE COMMAND
// ============================================================================

// Style definitions
typedef struct {
    const char* name;
    const char* description;
    int max_tokens;
    double temperature;
    bool markdown;
} StyleDef;

static const StyleDef STYLES[] = {
    {"flash", "Ultra fast, direct answers, no formatting", 1024, 0.3, false},
    {"concise", "Brief but formatted, good balance", 2048, 0.5, true},
    {"balanced", "Default, equilibrated detail and speed", 4096, 0.7, true},
    {"detailed", "In-depth analysis, maximum detail", 8192, 0.8, true},
};
#define STYLE_COUNT 4

static const StyleDef* style_get_def(const char* name) {
    for (int i = 0; i < STYLE_COUNT; i++) {
        if (strcmp(STYLES[i].name, name) == 0) {
            return &STYLES[i];
        }
    }
    return NULL;
}

int cmd_style(int argc, char** argv) {
    const char* current = convergio_config_get("style");
    if (!current)
        current = "balanced";

    if (argc > 1) {
        // Set style
        const StyleDef* def = style_get_def(argv[1]);
        if (def) {
            convergio_config_set("style", argv[1]);
            convergio_config_save();
            printf("\n\033[1mStyle changed to: %s\033[0m\n", def->name);
            printf("  %s\n", def->description);
            printf("  Max tokens: %d | Temperature: %.1f | Markdown: %s\n\n", def->max_tokens,
                   def->temperature, def->markdown ? "yes" : "no");
        } else {
            printf("\033[31mUnknown style: %s\033[0m\n\n", argv[1]);
            printf("Available styles:\n");
            for (int i = 0; i < STYLE_COUNT; i++) {
                printf("  \033[1m%-10s\033[0m %s\n", STYLES[i].name, STYLES[i].description);
            }
            printf("\n");
        }
    } else {
        // Show current style and options
        printf("\n\033[1mResponse Style Configuration\033[0m\n\n");
        printf("Current style: \033[1;36m%s\033[0m\n\n", current);
        printf("Available styles:\n");
        for (int i = 0; i < STYLE_COUNT; i++) {
            const char* marker = strcmp(STYLES[i].name, current) == 0 ? " *" : "  ";
            printf("%s\033[1m%-10s\033[0m %s\n", marker, STYLES[i].name, STYLES[i].description);
            printf("              tokens: %d | temp: %.1f | markdown: %s\n", STYLES[i].max_tokens,
                   STYLES[i].temperature, STYLES[i].markdown ? "yes" : "no");
        }
        printf("\nUsage: /style <name>\n\n");
    }
    return 0;
}

// ============================================================================
// MODEL COMPARISON COMMANDS
// ============================================================================

int cmd_compare(int argc, char** argv) {
    // Get defaults from JSON config (or fallback)
    size_t default_count = 0;
    const char** default_models = models_get_compare_defaults(&default_count);

    if (argc < 2) {
        printf("\n\033[1mCommand: compare\033[0m - Compare models side-by-side\n\n");
        printf("\033[1mUsage:\033[0m\n");
        printf("  compare <prompt>                    # Uses default models\n");
        printf("  compare <prompt> <model1> <model2>  # Custom models\n\n");
        printf("\033[1mDefault models:\033[0m (most powerful from each provider)\n");
        for (size_t i = 0; i < default_count; i++) {
            printf("  - %s\n", default_models[i]);
        }
        printf("\n\033[1mExample:\033[0m\n");
        printf("  compare \"Explain quantum computing\"\n");
        printf("  compare \"Write a poem\" claude-opus-4 gpt-5\n\n");
        printf("\033[1mOptions:\033[0m\n");
        printf("  --no-diff      Skip diff generation\n");
        printf("  --json         Output as JSON\n");
        printf("  --sequential   Run sequentially instead of parallel\n\n");
        if (models_loaded_from_json()) {
            printf("\033[2mConfig: %s (v%s)\033[0m\n\n", models_get_loaded_path(),
                   models_get_version());
        }
        return 0;
    }

    // Parse prompt
    const char* prompt = argv[1];

    // Count models and check for options
    CompareOptions opts = compare_options_default();
    size_t model_count = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--no-diff") == 0) {
            opts.show_diff = false;
        } else if (strcmp(argv[i], "--json") == 0) {
            opts.output_format = "json";
        } else if (strcmp(argv[i], "--sequential") == 0) {
            opts.mode = COMPARE_MODE_SEQUENTIAL;
        } else if (argv[i][0] == '-') {
            printf("Warning: Unknown option '%s' ignored.\n", argv[i]);
        } else {
            model_count++;
        }
    }

    // Use default models if none specified
    const char** models_to_use;
    bool using_defaults = false;

    if (model_count == 0) {
        // Use default models from JSON config
        models_to_use = default_models;
        model_count = default_count;
        using_defaults = true;
        printf("\033[36mUsing default models:");
        for (size_t i = 0; i < default_count; i++) {
            printf(" %s%s", default_models[i], i < default_count - 1 ? "," : "");
        }
        printf("\033[0m\n\n");
    } else if (model_count == 1) {
        printf("Error: Need at least 2 models to compare (or none for defaults).\n");
        return -1;
    } else {
        // Filter out options from models list
        models_to_use = malloc(sizeof(char*) * model_count);
        if (!models_to_use) {
            printf("Error: Memory allocation failed.\n");
            return -1;
        }

        size_t idx = 0;
        for (int i = 2; i < argc; i++) {
            if (argv[i][0] != '-') {
                models_to_use[idx++] = argv[i];
            }
        }
    }

    // Run comparison
    CompareResult* results = NULL;
    int ret = compare_models(prompt, NULL, models_to_use, model_count, &opts, &results);

    // Cleanup
    if (results) {
        compare_results_free(results, model_count);
    }
    if (!using_defaults) {
        free((void*)models_to_use);
    }

    return ret;
}

int cmd_benchmark(int argc, char** argv) {
    // Get defaults from JSON config
    const char* default_model = models_get_benchmark_default();
    size_t default_iterations = models_get_benchmark_iterations();

    if (argc < 2) {
        printf("\n\033[1mCommand: benchmark\033[0m - Benchmark a model's performance\n\n");
        printf("\033[1mUsage:\033[0m\n");
        printf("  benchmark <prompt>                    # Uses %s, %zu iterations\n", default_model,
               default_iterations);
        printf("  benchmark <prompt> <model>            # Custom model, %zu iterations\n",
               default_iterations);
        printf("  benchmark <prompt> <model> <N>        # Custom model, N iterations\n\n");
        printf("\033[1mDefaults:\033[0m\n");
        printf("  Model: %s\n", default_model);
        printf("  Iterations: %zu\n\n", default_iterations);
        printf("\033[1mExample:\033[0m\n");
        printf("  benchmark \"Write a haiku\"\n");
        printf("  benchmark \"Explain AI\" gpt-4o-mini 5\n\n");
        return 0;
    }

    const char* prompt = argv[1];
    const char* model = default_model;
    size_t iterations = default_iterations;

    if (argc >= 3) {
        model = argv[2];
    }

    if (argc >= 4) {
        iterations = (size_t)atoi(argv[3]);
        if (iterations == 0 || iterations > 100) {
            printf("Error: Iterations must be between 1 and 100.\n");
            return -1;
        }
    }

    printf("Starting benchmark: %zu iterations of %s\n\n", iterations, model);

    CompareResult result = {0};
    int ret = benchmark_model(prompt, NULL, model, iterations, &result);

    // Cleanup
    if (result.model_id)
        free(result.model_id);
    if (result.response)
        free(result.response);
    if (result.error)
        free(result.error);

    return ret;
}

// ============================================================================
// TELEMETRY COMMAND
// ============================================================================

int cmd_telemetry(int argc, char** argv) {
    if (argc < 2) {
        printf("\n\033[1mTelemetry Management\033[0m\n\n");
        printf("Privacy-first, opt-in telemetry for improving Convergio\n\n");
        printf("\033[1mUsage:\033[0m\n");
        printf("  telemetry status   - Show current telemetry status\n");
        printf("  telemetry info     - Show what data is collected\n");
        printf("  telemetry enable   - Enable telemetry (opt-in)\n");
        printf("  telemetry disable  - Disable telemetry (opt-out)\n");
        printf("  telemetry view     - View collected data\n");
        printf("  telemetry export   - Export data as JSON\n");
        printf("  telemetry delete   - Delete all collected data\n\n");
        printf("\033[1mCore Principles:\033[0m\n");
        printf("  • OPT-IN ONLY (never enabled by default)\n");
        printf("  • Privacy-first (no PII, anonymous metrics only)\n");
        printf("  • User control (view/export/delete at any time)\n\n");
        return 0;
    }

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "status") == 0) {
        telemetry_status();
        return 0;
    }

    if (strcmp(subcommand, "info") == 0) {
        telemetry_show_consent_prompt();
        return 0;
    }

    if (strcmp(subcommand, "enable") == 0) {
        int ret = telemetry_enable();
        if (ret == 0) {
            printf("\nTelemetry has been enabled.\n");
            printf("Thank you for helping improve Convergio!\n\n");
            printf("You can view collected data with: telemetry view\n");
            printf("You can disable at any time with: telemetry disable\n\n");
        } else {
            printf("\nFailed to enable telemetry.\n");
        }
        return ret;
    }

    if (strcmp(subcommand, "disable") == 0) {
        int ret = telemetry_disable();
        if (ret == 0) {
            printf("\nTelemetry has been disabled.\n");
            printf("No further data will be collected.\n\n");
            printf("Existing data is still stored locally.\n");
            printf("To delete it, use: telemetry delete\n\n");
        } else {
            printf("\nFailed to disable telemetry.\n");
        }
        return ret;
    }

    if (strcmp(subcommand, "view") == 0) {
        telemetry_view();
        return 0;
    }

    if (strcmp(subcommand, "export") == 0) {
        char* data = telemetry_export();
        if (data) {
            printf("\n");
            printf("╔═══════════════════════════════════════════════════════════════════════╗\n");
            printf("║                     TELEMETRY DATA EXPORT                             ║\n");
            printf("╚═══════════════════════════════════════════════════════════════════════╝\n");
            printf("\n");
            printf("%s\n", data);
            printf("\n");
            printf("You can save this output with:\n");
            printf("  telemetry export > telemetry_export.json\n\n");
            free(data);
            return 0;
        } else {
            printf("\nNo telemetry data to export.\n");
            return -1;
        }
    }

    if (strcmp(subcommand, "delete") == 0) {
        return telemetry_delete();
    }

    printf("Unknown telemetry subcommand: %s\n", subcommand);
    printf("Run 'telemetry' without arguments for usage information.\n");
    return -1;
}

// ============================================================================
// DEVELOPMENT TOOLS COMMAND
// ============================================================================

int cmd_tools(int argc, char** argv) {
    if (argc < 2) {
        printf("\n\033[1mCommand: tools\033[0m - Manage development tools\n\n");
        printf("Usage:\n");
        printf("  tools check            - Show installed/missing development tools\n");
        printf("  tools install <tool>   - Install a tool (requires approval)\n\n");
        printf("Example:\n");
        printf("  tools check            - List all tools\n");
        printf("  tools install gh       - Install GitHub CLI\n\n");
        return 0;
    }

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "check") == 0) {
        printf("\n\033[1mDevelopment Tools Status\033[0m\n");
        printf("═══════════════════════════════════════════\n\n");

        const char* tools[] = {"gh",   "git",   "node",   "npm", "python3", "pip3", "cargo", "go",
                               "make", "cmake", "docker", "jq",  "curl",    "wget", NULL};

        int installed = 0, missing = 0;

        for (int i = 0; tools[i] != NULL; i++) {
            bool exists = tool_exists(tools[i]);
            if (exists) {
                printf("  \033[32m✓\033[0m %-12s installed\n", tools[i]);
                installed++;
            } else {
                printf("  \033[31m✗\033[0m %-12s not found\n", tools[i]);
                missing++;
            }
        }

        printf("\n%d installed, %d missing\n\n", installed, missing);

        if (missing > 0) {
            printf("To install: \033[33mtools install <tool>\033[0m\n\n");
        }
        return 0;
    }

    if (strcmp(subcommand, "install") == 0) {
        if (argc < 3) {
            printf("Usage: tools install <tool>\n");
            printf("Example: tools install gh\n");
            return -1;
        }

        const char* tool = argv[2];

        // Check if already installed
        if (tool_exists(tool)) {
            printf("\033[32m%s is already installed.\033[0m\n", tool);
            return 0;
        }

        // Get install command
        const char* install_cmd = get_install_command(tool);
        if (!install_cmd) {
            printf("\033[31mError: Don't know how to install '%s'\033[0m\n", tool);
            printf("Please install manually.\n");
            return -1;
        }

        // Request approval
        ApprovalRequest req = {.action = tool,
                               .reason = "Development tool needed",
                               .command = install_cmd,
                               .is_destructive = false};

        if (!request_user_approval(&req)) {
            printf("\nInstallation cancelled.\n");
            return 0;
        }

        // Install
        printf("\nInstalling %s...\n", tool);
        int ret = install_tool(tool, "Development tool needed");

        if (ret == 0) {
            printf("\033[32m✓ %s installed successfully.\033[0m\n", tool);
        } else {
            printf("\033[31m✗ Failed to install %s.\033[0m\n", tool);
        }

        return ret;
    }

    printf("Unknown tools subcommand: %s\n", subcommand);
    printf("Run 'tools' without arguments for usage information.\n");
    return -1;
}

// ============================================================================
