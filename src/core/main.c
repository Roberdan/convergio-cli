// Paste part1 (headers, logging, global state)
/**
 * CONVERGIO KERNEL
 *
 * Main entry point and CLI interface
 * Human purpose. AI momentum.
 * With Ali as Chief of Staff orchestrating all agents
 */

#include "nous/nous.h"
#include "nous/orchestrator.h"
#include "nous/tools.h"
#include "nous/config.h"
#include "nous/hardware.h"
#include "nous/updater.h"
#include "nous/theme.h"
#include "nous/statusbar.h"
#include "nous/commands.h"
#include "nous/repl.h"
#include "nous/signals.h"
#include "nous/safe_path.h"
#include <fcntl.h>
#include "nous/projects.h"
#include "nous/mlx.h"
#include "nous/notify.h"
#include "nous/plan_db.h"
#include "nous/output_service.h"
#include "nous/telemetry.h"
#include "../auth/oauth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <curl/curl.h>  // For curl_global_init/cleanup - must be called once before any threads


// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

extern int nous_gpu_init(void);
extern void nous_gpu_shutdown(void);
extern int nous_scheduler_init(void);
extern void nous_scheduler_shutdown(void);
extern void nous_destroy_agent(NousAgent* agent);

// Default budget in USD
#define DEFAULT_BUDGET_USD 5.00

// MLX local mode flag
static bool g_use_local_mlx = false;
static char g_mlx_model[64] = {0};  // Selected MLX model

// External router function for local mode
extern void router_set_local_mode(bool enabled, const char* model_id);

// ============================================================================
// DEBUG LOGGING IMPLEMENTATION
// ============================================================================

LogLevel g_log_level = LOG_LEVEL_NONE;

static const char* LOG_LEVEL_NAMES[] = {
    "NONE", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
};

static const char* LOG_CAT_NAMES[] = {
    "SYSTEM", "AGENT", "TOOL", "API", "MEMORY", "MSGBUS", "COST", "WORKFLOW"
};

static const char* LOG_CAT_COLORS[] = {
    "\033[36m",   // Cyan - SYSTEM
    "\033[33m",   // Yellow - AGENT
    "\033[32m",   // Green - TOOL
    "\033[35m",   // Magenta - API
    "\033[34m",   // Blue - MEMORY
    "\033[37m",   // White - MSGBUS
    "\033[31m",   // Red - COST
    "\033[93m"    // Bright Yellow - WORKFLOW
};

void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) {
    if (level > g_log_level || g_log_level == LOG_LEVEL_NONE) return;

    // Timestamp
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[16];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

    // Level indicator
    const char* level_color;
    switch (level) {
        case LOG_LEVEL_ERROR: level_color = "\033[31m"; break;  // Red
        case LOG_LEVEL_WARN:  level_color = "\033[33m"; break;  // Yellow
        case LOG_LEVEL_INFO:  level_color = "\033[32m"; break;  // Green
        case LOG_LEVEL_DEBUG: level_color = "\033[36m"; break;  // Cyan
        case LOG_LEVEL_TRACE: level_color = "\033[2m"; break;   // Dim
        default: level_color = "\033[0m";
    }

    // Print header
    fprintf(stderr, "\033[2m[%s]\033[0m %s[%-5s]\033[0m %s[%s]\033[0m ",
            time_str,
            level_color, LOG_LEVEL_NAMES[level],
            LOG_CAT_COLORS[cat], LOG_CAT_NAMES[cat]);

    // Print message
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\033[0m\n");
}

void nous_log_set_level(LogLevel level) {
    g_log_level = level;
}

LogLevel nous_log_get_level(void) {
    return g_log_level;
}

const char* nous_log_level_name(LogLevel level) {
    if (level <= LOG_LEVEL_TRACE) return LOG_LEVEL_NAMES[level];
    return "UNKNOWN";
}

// ============================================================================
// GLOBAL STATE
// ============================================================================

volatile sig_atomic_t g_running = 1;
void* g_current_space = NULL;
void* g_assistant = NULL;
bool g_streaming_enabled = false;  // Default OFF to enable tool support

// ============================================================================
// BANNER DISPLAY
// ============================================================================

// MAIN
// ============================================================================

// Print a single UTF-8 character with color
static void print_colored_char(const char* ch, int len, int col, int total_cols) {
    // Gradient: violet/purple -> magenta -> orange
    // Creates a warm purple-to-orange transition
    float t = (float)col / (float)total_cols;

    int color;
    if (t < 0.12f) {
        color = 99;  // Light purple/violet
    } else if (t < 0.24f) {
        color = 135; // Medium purple
    } else if (t < 0.36f) {
        color = 171; // Light magenta
    } else if (t < 0.48f) {
        color = 207; // Pink-magenta
    } else if (t < 0.60f) {
        color = 213; // Hot pink
    } else if (t < 0.72f) {
        color = 209; // Salmon/coral
    } else if (t < 0.84f) {
        color = 208; // Bright orange
    } else {
        color = 214; // Gold/orange (end)
    }

    printf("\033[1m\033[38;5;%dm%.*s\033[0m", color, len, ch);
}

// Print a line with horizontal gradient
static void print_gradient_line(const char* line) {
    int col = 0;
    int total_cols = 0;

    // First pass: count visible columns (skip spaces for color calculation)
    const char* p = line;
    while (*p) {
        if ((unsigned char)*p >= 0x80) {
            // UTF-8 multibyte
            if ((*p & 0xE0) == 0xC0) p += 2;
            else if ((*p & 0xF0) == 0xE0) p += 3;
            else if ((*p & 0xF8) == 0xF0) p += 4;
            else p++;
        } else {
            p++;
        }
        total_cols++;
    }

    // Second pass: print with colors
    p = line;
    col = 0;
    while (*p) {
        int char_len = 1;
        if ((unsigned char)*p >= 0x80) {
            if ((*p & 0xE0) == 0xC0) char_len = 2;
            else if ((*p & 0xF0) == 0xE0) char_len = 3;
            else if ((*p & 0xF8) == 0xF0) char_len = 4;
        }

        if (*p == ' ') {
            printf(" ");
        } else {
            print_colored_char(p, char_len, col, total_cols);
        }

        p += char_len;
        col++;
    }
    printf("\n");
}

static void print_banner(void) {
    const char* rst = "\033[0m";
    const char* dim = "\033[2m";
    const char* c3 = "\033[38;5;75m";

    printf("\n");
    // Block-style > arrow with CONVERGIO text
    print_gradient_line(" ███           ██████╗ ██████╗ ███╗   ██╗██╗   ██╗███████╗██████╗  ██████╗ ██╗ ██████╗ ");
    print_gradient_line("  ░░███       ██╔════╝██╔═══██╗████╗  ██║██║   ██║██╔════╝██╔══██╗██╔════╝ ██║██╔═══██╗");
    print_gradient_line("    ░░███     ██║     ██║   ██║██╔██╗ ██║██║   ██║█████╗  ██████╔╝██║  ███╗██║██║   ██║");
    print_gradient_line("     ███░     ██║     ██║   ██║██║╚██╗██║╚██╗ ██╔╝██╔══╝  ██╔══██╗██║   ██║██║██║   ██║");
    print_gradient_line("   ███░      ╚██████╗╚██████╔╝██║ ╚████║ ╚████╔╝ ███████╗██║  ██║╚██████╔╝██║╚██████╔╝");
    print_gradient_line(" ███░         ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═╝ ╚═════╝ ");
    printf("\n");
    print_gradient_line("          Your team, with human purpose and AI momentum.");
    printf("\n");
    printf("  %sv%s%s  •  %s/help%s for commands\n", dim, convergio_get_version(), rst, c3, rst);
    printf("\n");
}

int main(int argc, char** argv) {
    // Workspace path (default: current directory)
    char workspace[PATH_MAX];
    getcwd(workspace, sizeof(workspace));

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
            nous_log_set_level(LOG_LEVEL_DEBUG);
        } else if (strcmp(argv[i], "--trace") == 0 || strcmp(argv[i], "-t") == 0) {
            nous_log_set_level(LOG_LEVEL_TRACE);
        } else if (strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "-q") == 0) {
            nous_log_set_level(LOG_LEVEL_ERROR);
        } else if ((strcmp(argv[i], "--workspace") == 0 || strcmp(argv[i], "-w") == 0) && i + 1 < argc) {
            // Custom workspace path
            char resolved[PATH_MAX];
            if (realpath(argv[++i], resolved)) {
                strncpy(workspace, resolved, sizeof(workspace) - 1);
                workspace[sizeof(workspace) - 1] = '\0';
            } else {
                fprintf(stderr, "Error: Cannot access workspace path: %s\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "--local") == 0 || strcmp(argv[i], "-l") == 0) {
            g_use_local_mlx = true;
        } else if ((strcmp(argv[i], "--model") == 0 || strcmp(argv[i], "-m") == 0) && i + 1 < argc) {
            strncpy(g_mlx_model, argv[++i], sizeof(g_mlx_model) - 1);
            g_mlx_model[sizeof(g_mlx_model) - 1] = '\0';
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Convergio — Human purpose. AI momentum.\n\n");
            printf("Usage: convergio [OPTIONS] [COMMAND]\n\n");
            printf("Commands:\n");
            printf("  setup                   Configure API key, models, and settings\n");
            printf("  update [check|install]  Check for or install updates\n\n");
            printf("Options:\n");
            printf("  -w, --workspace <path>  Set workspace directory (default: current dir)\n");
            printf("  -l, --local             Use MLX local models (Apple Silicon only)\n");
            printf("  -m, --model <model>     Specify model (e.g., llama-3.2-3b, deepseek-r1-7b)\n");
            printf("  -d, --debug             Enable debug logging\n");
            printf("  -t, --trace             Enable trace logging (verbose)\n");
            printf("  -q, --quiet             Suppress non-error output\n");
            printf("  -v, --version           Show version\n");
            printf("  -h, --help              Show this help message\n\n");
            printf("Local Models (MLX):\n");
            printf("  Convergio supports 100%% offline operation using MLX on Apple Silicon.\n");
            printf("  Use /setup -> Local Models to download models, or:\n");
            printf("    convergio --local --model deepseek-r1-7b\n\n");
            printf("  Available models: llama-3.2-1b, llama-3.2-3b, deepseek-r1-1.5b,\n");
            printf("                    deepseek-r1-7b, deepseek-r1-14b, qwen2.5-coder-7b,\n");
            printf("                    phi-3-mini, mistral-7b-q4, llama-3.1-8b-q4\n");
            return 0;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("Convergio %s\n", convergio_get_version());
            return 0;
        } else if (strcmp(argv[i], "setup") == 0) {
            convergio_config_init();
            return convergio_setup_wizard();
        } else if (strcmp(argv[i], "update") == 0) {
            if (i + 1 < argc && strcmp(argv[i + 1], "install") == 0) {
                return convergio_cmd_update_install();
            }
            return convergio_cmd_update_check();
        }
    }

    // Setup signal handling
    signals_init();

    // Initialize libcurl globally - MUST be called once before any threads
    // See: https://curl.se/libcurl/c/curl_global_init.html
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Save the terminal app for notifications to return to the correct terminal
    const char* term_program = getenv("TERM_PROGRAM");
    const char* home_dir = getenv("HOME");
    if (term_program && home_dir) {
        char term_file[PATH_MAX];
        snprintf(term_file, sizeof(term_file), "%s/.convergio/terminal", home_dir);
        int fd = safe_path_open(term_file, safe_path_get_user_boundary(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        FILE* f = fd >= 0 ? fdopen(fd, "w") : NULL;
        if (f) {
            fprintf(f, "%s", term_program);
            fclose(f);
        }
    }

    // Only print banner if not in quiet mode (-q sets LOG_LEVEL_ERROR)
    bool quiet_mode = (g_log_level == LOG_LEVEL_ERROR);
    if (!quiet_mode) {
        print_banner();
    }

    // Show debug mode if enabled (but not in quiet mode)
    if (!quiet_mode && g_log_level != LOG_LEVEL_NONE) {
        printf("  \033[33m⚡ Debug mode: %s\033[0m\n\n", nous_log_level_name(g_log_level));
    }

    // Initialize subsystems silently (only show errors)
    bool init_errors = false;

    // Initialize configuration first
    if (convergio_config_init() != 0) {
        fprintf(stderr, "  \033[31m✗ Config initialization failed\033[0m\n");
        init_errors = true;
    }

    // Initialize theme system
    theme_init();

    // Detect hardware
    if (convergio_detect_hardware() != 0) {
        fprintf(stderr, "  \033[31m✗ Hardware detection failed\033[0m\n");
        init_errors = true;
    }

    // Initialize authentication
    if (auth_init() != 0) {
        printf("  \033[33m⚠ No API key found\033[0m\n");
        printf("\n");
        printf("  ┌─────────────────────────────────────────────────────────────┐\n");
        printf("  │  \033[1mWelcome to Convergio!\033[0m                                      │\n");
        printf("  │                                                             │\n");
        printf("  │  To get started, you need an Anthropic API key.            │\n");
        printf("  │                                                             │\n");
        printf("  │  \033[1mHow to get your API key:\033[0m                                   │\n");
        printf("  │  1. Go to \033[36mhttps://console.anthropic.com/settings/keys\033[0m      │\n");
        printf("  │  2. Sign up or log in to your Anthropic account            │\n");
        printf("  │  3. Click \"Create Key\" and copy it                         │\n");
        printf("  └─────────────────────────────────────────────────────────────┘\n");
        printf("\n");

        // Ask if user wants to open browser
        printf("  Would you like to open the Anthropic console in your browser? [Y/n]: ");
        fflush(stdout);

        char response[16] = {0};
        if (fgets(response, sizeof(response), stdin)) {
            if (response[0] != 'n' && response[0] != 'N') {
                // Open browser on macOS
                system("open 'https://console.anthropic.com/settings/keys' 2>/dev/null");
                printf("\n  \033[32m✓ Browser opened!\033[0m\n\n");
            }
        }

        // Now ask for the key
        printf("  Enter your API key (starts with 'sk-ant-'): ");
        fflush(stdout);

        char api_key[256] = {0};
        if (fgets(api_key, sizeof(api_key), stdin)) {
            // Trim newline
            size_t len = strlen(api_key);
            if (len > 0 && api_key[len-1] == '\n') {
                api_key[len-1] = '\0';
                len--;
            }

            // Validate and store if provided
            if (len > 10 && strncmp(api_key, "sk-", 3) == 0) {
                if (convergio_store_api_key(api_key) == 0) {
                    printf("\n  \033[32m✓ API key saved to macOS Keychain!\033[0m\n");
                    printf("    Your key is stored securely and you won't need to enter it again.\n\n");
                    // Re-initialize auth with the new key
                    auth_init();
                } else {
                    // Fallback: set environment variable for this session
                    setenv("ANTHROPIC_API_KEY", api_key, 1);
                    printf("\n  \033[32m✓ API key configured for this session.\033[0m\n");
                    printf("    Run 'convergio setup' later to save it permanently.\n\n");
                    auth_init();
                }
            } else if (len > 0) {
                printf("\n  \033[33m⚠ Invalid key format.\033[0m Keys should start with 'sk-ant-'.\n");
                printf("    You can run 'convergio setup' later to configure it.\n\n");
            } else {
                printf("\n  \033[2mSkipped.\033[0m You can run 'convergio setup' later.\n");
                printf("    \033[33mNote: Convergio won't work until you configure an API key.\033[0m\n\n");
            }
        }
    } else {
        // Auth already configured, no message needed
        (void)0;
    }

    // Show multi-provider status (all providers are required for full functionality)
    {
        const char* anthropic_key = getenv("ANTHROPIC_API_KEY");
        const char* openai_key = getenv("OPENAI_API_KEY");
        const char* gemini_key = getenv("GEMINI_API_KEY");

        bool has_anthropic = (auth_get_mode() != AUTH_MODE_NONE) || (anthropic_key && strlen(anthropic_key) > 0);
        bool has_openai = (openai_key && strlen(openai_key) > 0);
        bool has_gemini = (gemini_key && strlen(gemini_key) > 0);

        int missing_count = (!has_anthropic ? 1 : 0) + (!has_openai ? 1 : 0) + (!has_gemini ? 1 : 0);

        // Always show provider status box if any key is missing
        if (missing_count > 0) {
            printf("\n");
            printf("  ╭─ Provider Status ─────────────────────────────────────────╮\n");
            printf("  │  %s Anthropic   %-42s │\n",
                   has_anthropic ? "\033[32m✓\033[0m" : "\033[31m✗\033[0m",
                   has_anthropic ? "(configured)" : "(ANTHROPIC_API_KEY missing)");
            printf("  │  %s OpenAI      %-42s │\n",
                   has_openai ? "\033[32m✓\033[0m" : "\033[31m✗\033[0m",
                   has_openai ? "(configured)" : "(OPENAI_API_KEY missing)");
            printf("  │  %s Gemini      %-42s │\n",
                   has_gemini ? "\033[32m✓\033[0m" : "\033[31m✗\033[0m",
                   has_gemini ? "(configured)" : "(GEMINI_API_KEY missing)");
            printf("  ╰──────────────────────────────────────────────────────────╯\n");

            printf("\n");
            printf("  \033[33m⚠ %d provider key(s) missing!\033[0m\n", missing_count);
            printf("  All keys are required - agents use different providers.\n\n");
            printf("  \033[1mHow to configure:\033[0m\n");
            printf("  Add these lines to your ~/.zshrc (or ~/.bashrc):\n\n");

            if (!has_anthropic) {
                printf("    export ANTHROPIC_API_KEY=\"sk-ant-...\"  \033[2m# https://console.anthropic.com/settings/keys\033[0m\n");
            }
            if (!has_openai) {
                printf("    export OPENAI_API_KEY=\"sk-...\"         \033[2m# https://platform.openai.com/api-keys\033[0m\n");
            }
            if (!has_gemini) {
                printf("    export GEMINI_API_KEY=\"...\"            \033[2m# https://aistudio.google.com/apikey\033[0m\n");
            }

            printf("\n  Then run: source ~/.zshrc\n\n");
        }
    }

    if (nous_init() != 0) {
        fprintf(stderr, "  \033[31m✗ Fabric initialization failed\033[0m\n");
        return 1;
    }

    if (nous_scheduler_init() != 0) {
        fprintf(stderr, "  \033[31m✗ Scheduler initialization failed\033[0m\n");
        nous_shutdown();
        return 1;
    }

    // GPU optional, no error message needed
    nous_gpu_init();

    // Initialize agent configurations from config files
    extern int agent_config_init(void);
    extern int agent_config_load_directory(const char* dir_path);

    if (agent_config_init() == 0) {
        // Try to load custom configs from config directory
        char config_path[PATH_MAX];
        snprintf(config_path, sizeof(config_path), "%s/config", workspace);
        agent_config_load_directory(config_path);
    }

    // Initialize Orchestrator with budget from config (or default)
    double budget = g_config.budget_limit > 0 ? g_config.budget_limit : DEFAULT_BUDGET_USD;
    if (orchestrator_init(budget) != 0) {
        fprintf(stderr, "  \033[31m✗ Orchestrator initialization failed\033[0m\n");
        init_errors = true;
    }

    // Initialize Plan Database for persistent execution plans
    if (plan_db_init(NULL) != PLAN_DB_OK) {
        fprintf(stderr, "  \033[33m⚠ Plan database initialization failed (non-critical)\033[0m\n");
        // Non-critical: plans will work in-memory only
    }

    // Initialize Output Service for structured document generation
    if (output_service_init(NULL) != OUTPUT_OK) {
        fprintf(stderr, "  \033[33m⚠ Output service initialization failed (non-critical)\033[0m\n");
        // Non-critical: agents will output to terminal only
    }

    // Set local MLX mode if requested
    if (g_use_local_mlx) {
        const char* model_id = g_mlx_model[0] ? g_mlx_model : "deepseek-r1-1.5b";
        router_set_local_mode(true, model_id);

        // Pre-check and download model BEFORE entering REPL
        // This ensures the progress bar is visible (not hidden by spinner)
        if (mlx_is_available()) {
            size_t model_count = 0;
            const MLXModelInfo* models = mlx_get_available_models(&model_count);

            // Find the requested model
            const MLXModelInfo* selected = NULL;
            for (size_t i = 0; i < model_count; i++) {
                if (strcmp(models[i].id, model_id) == 0) {
                    selected = &models[i];
                    break;
                }
            }

            if (selected) {
                printf("\n  \033[36m🖥️  MLX Local Mode\033[0m\n");
                printf("  Model: %s (%zu MB)\n", selected->display_name, selected->size_mb);

                // Pre-download if not cached
                extern bool mlx_bridge_model_exists(const char*);
                if (!mlx_bridge_model_exists(selected->huggingface_id)) {
                    printf("  \033[33m⚠ Model not cached locally. Starting download...\033[0m\n");
                    printf("  \033[90mThis is a one-time download from HuggingFace.\033[0m\n\n");

                    MLXError err = mlx_download_model_with_progress(selected->huggingface_id);
                    if (err != MLX_OK) {
                        printf("  \033[31m✗ Failed to download model: %s\033[0m\n", mlx_error_message(err));
                        printf("  \033[90mTrying to continue anyway...\033[0m\n");
                    }
                } else {
                    printf("  \033[32m✓ Model ready (cached)\033[0m\n\n");
                }
            } else {
                printf("  \033[31m✗ Unknown model: %s\033[0m\n", model_id);
                printf("  Available: llama-3.2-1b, llama-3.2-3b, deepseek-r1-1.5b, deepseek-r1-7b, etc.\n\n");
            }
        } else {
            printf("  \033[31m✗ MLX not available on this system\033[0m\n");
            printf("  MLX requires Apple Silicon (M1/M2/M3/M4) and macOS 14+\n\n");
        }
    }

    // Initialize workspace sandbox
    tools_init_workspace(workspace);

    // Initialize projects
    projects_init();

    // Initialize notification system (for daemon, reminders, etc.)
    notify_init();

    // Initialize telemetry system (privacy-first, opt-in)
    if (telemetry_init() != 0) {
        fprintf(stderr, "  \033[33m⚠ Telemetry initialization failed (non-critical)\033[0m\n");
        // Non-critical: telemetry is optional
    } else {
        // Record session start in telemetry
        telemetry_record_session_start();
    }

    // Only show status if there were errors during initialization
    (void)init_errors;  // Suppress unused warning - errors already printed

    // Create fallback assistant (only used if orchestrator fails)
    g_assistant = nous_create_agent("Aria", "creative and collaborative assistant");
    if (g_assistant) {
        nous_agent_add_skill(g_assistant, "programming");
        nous_agent_add_skill(g_assistant, "analysis");
        nous_agent_add_skill(g_assistant, "creativity");
    }

    // Status bar disabled - was causing terminal issues
    // If needed in future, call statusbar_init() and statusbar_set_visible(true)
    (void)statusbar_init;  // Suppress unused warning

    // REPL with cost in prompt
    char prompt[256];
    char* line;

    using_history();

    // Set up readline completion for @agent names
    rl_attempted_completion_function = repl_agent_completion;

    // Bind Ctrl+V to clipboard paste (including images)
    // Ctrl+V is ASCII 22 (0x16)
    rl_bind_key(22, repl_paste_clipboard_image);

    while (g_running) {
        // Get terminal width for separator line
        struct winsize ws;
        int term_width = 80;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
            term_width = ws.ws_col;
        }

        // Print top separator line (dim horizontal line)
        printf("\033[2m");
        for (int i = 0; i < term_width; i++) printf("─");
        printf("\033[0m\n");

        // Convergio prompt with separator style
        const Theme* t = theme_get();
        // Set blinking block cursor
        printf("\033[1 q");

        // Check for current agent, active agents, and project
        ManagedAgent* current_agent = repl_get_current_agent();
        ConvergioProject* current_proj = project_current();

        // Get working/active agents
        ManagedAgent* working_agents[8] = {0};
        size_t working_count = agent_get_working(working_agents, 8);

        // Build agents string: (Ali) or (Ali, Jenny) or (Ali, Jenny, Baccio, ...)
        char agents_str[128] = "";
        char* agents_ptr = agents_str;
        size_t agents_remaining = sizeof(agents_str);

        if (current_agent) {
            // Talking to specific agent - show that agent
            char short_name[32];
            strncpy(short_name, current_agent->name, sizeof(short_name) - 1);
            short_name[sizeof(short_name) - 1] = '\0';
            char* hyphen = strchr(short_name, '-');
            if (hyphen) *hyphen = '\0';
            if (short_name[0] >= 'a' && short_name[0] <= 'z') short_name[0] -= 32;
            snprintf(agents_ptr, agents_remaining, "%s", short_name);
        } else if (working_count > 0) {
            // Multiple agents working - show up to 3, then ...
            size_t show_count = working_count > 3 ? 3 : working_count;
            for (size_t i = 0; i < show_count; i++) {
                char short_name[32];
                strncpy(short_name, working_agents[i]->name, sizeof(short_name) - 1);
                short_name[sizeof(short_name) - 1] = '\0';
                char* hyphen = strchr(short_name, '-');
                if (hyphen) *hyphen = '\0';
                if (short_name[0] >= 'a' && short_name[0] <= 'z') short_name[0] -= 32;

                int written = snprintf(agents_ptr, agents_remaining, "%s%s",
                                        i > 0 ? ", " : "", short_name);
                if (written > 0 && (size_t)written < agents_remaining) {
                    agents_ptr += written;
                    agents_remaining -= (size_t)written;
                }
            }
            if (working_count > 3) {
                snprintf(agents_ptr, agents_remaining, ", ...");
            }
        } else {
            // Default: Ali
            snprintf(agents_ptr, agents_remaining, "Ali");
        }

        // Build prompt: Convergio (Agents) [Project] > (ALL BOLD, single theme color)
        // \001 and \002 wrap non-printing ANSI codes for readline (GNU readline required!)
        // Structure: [BOLD][COLOR]entire prompt text[RESET] - no mid-prompt resets
        if (current_proj) {
            snprintf(prompt, sizeof(prompt),
                "\001\033[1m%s\002Convergio (%s) [%s] >\001\033[0m\002 ",
                t->prompt_name, agents_str, current_proj->name);
        } else {
            snprintf(prompt, sizeof(prompt),
                "\001\033[1m%s\002Convergio (%s) >\001\033[0m\002 ",
                t->prompt_name, agents_str);
        }

        line = readline(prompt);

        // Reset color after user input
        printf("\033[0m");

        if (!line) {
            // EOF
            break;
        }

        // Add to history if non-empty
        if (strlen(line) > 0) {
            add_history(line);
            repl_parse_and_execute(line);
        }

        free(line);
    }

    // Cleanup
    printf("\nShutting down Convergio...\n");

    // Show final cost report
    char* final_report = cost_get_report();
    if (final_report) {
        printf("%s", final_report);
        free(final_report);
    }

    // Shutdown projects
    projects_shutdown();

    // Shutdown Plan Database
    plan_db_shutdown();

    // Shutdown Output Service
    output_service_shutdown();

    // Shutdown agent configs and orchestrator
    extern void agent_config_shutdown(void);
    agent_config_shutdown();
    orchestrator_shutdown();

    if (g_assistant) {
        nous_destroy_agent(g_assistant);
    }

    // Record session end in telemetry
    telemetry_record_session_end();
    
    // Shutdown telemetry (flushes pending events)
    telemetry_shutdown();
    
    nous_gpu_shutdown();
    nous_scheduler_shutdown();
    nous_shutdown();
    auth_shutdown();
    convergio_config_shutdown();

    // Cleanup libcurl globally - must be last curl operation
    curl_global_cleanup();

    printf("Goodbye.\n");
    return 0;
}
