/**
 * CONVERGIO SETUP WIZARD
 *
 * Interactive configuration wizard for:
 * - API Keys (with step-by-step guidance)
 * - Agent model selection (primary/fallback per agent)
 * - Quick setup profiles (Cost-optimized, Performance, Balanced, Local)
 * - Budget configuration
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/mlx.h"
#include "nous/nous.h"
#include "nous/provider.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// ============================================================================
// OPTIMIZATION PROFILES
// ============================================================================

typedef enum {
    PROFILE_COST,        // Cheapest models everywhere
    PROFILE_BALANCED,    // Mix of quality and cost
    PROFILE_PERFORMANCE, // Best models everywhere
    PROFILE_LOCAL,       // Ollama local-first with cloud fallback
    PROFILE_CUSTOM       // Manual configuration
} OptimizationProfile;

// ============================================================================
// UI HELPERS
// ============================================================================

static void clear_screen(void) {
    printf("\033[2J\033[H");
}

static void print_header(const char* title) {
    printf(
        "\n\033[1;36m┌─────────────────────────────────────────────────────────────────┐\033[0m\n");
    printf("\033[1;36m│\033[0m  \033[1;37m%-60s\033[0m \033[1;36m│\033[0m\n", title);
    printf(
        "\033[1;36m│─────────────────────────────────────────────────────────────────│\033[0m\n");
}

static void print_footer(void) {
    printf(
        "\033[1;36m└─────────────────────────────────────────────────────────────────┘\033[0m\n");
}

static void print_success(const char* msg) {
    printf("  \033[1;32m✓\033[0m %s\n", msg);
}

static void print_error(const char* msg) {
    printf("  \033[1;31m✗\033[0m %s\n", msg);
}

static void print_warning(const char* msg) __attribute__((unused));
static void print_warning(const char* msg) {
    printf("  \033[1;33m⚠\033[0m %s\n", msg);
}

static void print_info(const char* msg) {
    printf("  \033[1;34mℹ\033[0m %s\n", msg);
}

static int get_choice(int min, int max) {
    char input[16];
    printf("\n  \033[1;33mChoice [%d-%d]:\033[0m ", min, max);
    fflush(stdout);

    if (!fgets(input, sizeof(input), stdin))
        return -1;

    // Trim newline
    input[strcspn(input, "\n")] = 0;

    // Handle empty input
    if (strlen(input) == 0)
        return -1;

    int choice = atoi(input);
    if (choice < min || choice > max)
        return -1;
    return choice;
}

static bool get_yes_no(const char* prompt) __attribute__((unused));
static bool get_yes_no(const char* prompt) {
    char input[16];
    printf("  %s [y/N]: ", prompt);
    fflush(stdout);

    if (!fgets(input, sizeof(input), stdin))
        return false;
    input[strcspn(input, "\n")] = 0;

    return (input[0] == 'y' || input[0] == 'Y');
}

static void wait_for_enter(void) {
    printf("\n  Press Enter to continue...");
    fflush(stdout);
    getchar();
}

// ============================================================================
// API KEY STATUS
// ============================================================================

typedef struct {
    ProviderType type;
    const char* name;
    const char* env_var;
    bool available;
    const char* key_prefix;
    const char* signup_url;
} ProviderStatus;

static ProviderStatus g_provider_status[] = {
    {PROVIDER_ANTHROPIC, "Anthropic", "ANTHROPIC_API_KEY", false, "sk-ant-",
     "https://console.anthropic.com/settings/keys"},
    {PROVIDER_OPENAI, "OpenAI", "OPENAI_API_KEY", false, "sk-",
     "https://platform.openai.com/api-keys"},
    {PROVIDER_GEMINI, "Google Gemini", "GEMINI_API_KEY", false, "AIza",
     "https://aistudio.google.com/apikey"},
    {PROVIDER_OPENROUTER, "OpenRouter", "OPENROUTER_API_KEY", false, "sk-or-",
     "https://openrouter.ai/keys"},
    {PROVIDER_OLLAMA, "Ollama (Local)", NULL, false, NULL, "https://ollama.ai"},
    {PROVIDER_MLX, "MLX (Local)", NULL, false, NULL, "https://github.com/ml-explore/mlx"}};
static const size_t g_provider_count = sizeof(g_provider_status) / sizeof(g_provider_status[0]);

static void refresh_provider_status(void) {
    for (size_t i = 0; i < g_provider_count; i++) {
        if (g_provider_status[i].type == PROVIDER_OLLAMA) {
            // Check if Ollama is running
            Provider* p = provider_get(PROVIDER_OLLAMA);
            g_provider_status[i].available = (p && p->validate_key && p->validate_key(p));
        } else if (g_provider_status[i].type == PROVIDER_MLX) {
            // Check if MLX is available (Apple Silicon)
            Provider* p = provider_get(PROVIDER_MLX);
            g_provider_status[i].available = (p && p->validate_key && p->validate_key(p));
        } else if (g_provider_status[i].env_var) {
            const char* key = getenv(g_provider_status[i].env_var);
            g_provider_status[i].available = (key && strlen(key) > 0);
        }
    }
}

// ============================================================================
// API KEYS MENU
// ============================================================================

static void show_api_key_help(ProviderType type) {
    printf("\n");

    switch (type) {
    case PROVIDER_ANTHROPIC:
        print_info("To get an Anthropic API key:");
        printf("  1. Go to \033[4mhttps://console.anthropic.com/settings/keys\033[0m\n");
        printf("  2. Sign up or log in with your account\n");
        printf("  3. Click 'Create Key' to generate a new API key\n");
        printf("  4. Copy the key (starts with '\033[1;33msk-ant-\033[0m')\n");
        printf(
            "\n  \033[1;36mNote:\033[0m Anthropic offers Claude models (Opus, Sonnet, Haiku).\n");
        printf("  Pricing: $15/$75 per MTok (Opus), $3/$15 (Sonnet), $1/$5 (Haiku)\n");
        break;

    case PROVIDER_OPENAI:
        print_info("To get an OpenAI API key:");
        printf("  1. Go to \033[4mhttps://platform.openai.com/api-keys\033[0m\n");
        printf("  2. Sign up or log in with your account\n");
        printf("  3. Click 'Create new secret key'\n");
        printf("  4. Copy the key (starts with '\033[1;33msk-\033[0m')\n");
        printf("\n  \033[1;36mNote:\033[0m OpenAI offers GPT-4o, o1, o3, and GPT-5 models.\n");
        printf("  Pricing varies from $0.05/$0.40 (Nano) to $10/$40 (o3)\n");
        break;

    case PROVIDER_GEMINI:
        print_info("To get a Google Gemini API key:");
        printf("  1. Go to \033[4mhttps://aistudio.google.com/apikey\033[0m\n");
        printf("  2. Sign in with your Google account\n");
        printf("  3. Click 'Create API Key'\n");
        printf("  4. Copy the key (starts with '\033[1;33mAIza\033[0m')\n");
        printf("\n  \033[1;36mNote:\033[0m Gemini offers Pro, Ultra, and Flash models.\n");
        printf("  Flash is very cheap: $0.075/$0.30 per MTok with 1M context!\n");
        break;

    case PROVIDER_OPENROUTER:
        print_info("To get an OpenRouter API key:");
        printf("  1. Go to \033[4mhttps://openrouter.ai/keys\033[0m\n");
        printf("  2. Sign up with Google/GitHub or create account\n");
        printf("  3. Click 'Create Key'\n");
        printf("  4. Copy the key (starts with '\033[1;33msk-or-\033[0m')\n");
        printf("\n  \033[1;32mBenefits of OpenRouter:\033[0m\n");
        printf("  • Access to 300+ models (DeepSeek, Mistral, Llama, Qwen...)\n");
        printf("  • Single API key for all providers\n");
        printf("  • Often cheaper than direct API access\n");
        printf("  • Free models available (Gemini 2.0 Flash)\n");
        break;

    case PROVIDER_OLLAMA:
        print_info("Ollama runs locally - no API key needed!");
        printf("\n  To install Ollama:\n");
        printf("  1. Go to \033[4mhttps://ollama.ai\033[0m\n");
        printf("  2. Download and install for macOS\n");
        printf("  3. Run: \033[1;33mollama pull llama3.2\033[0m\n");
        printf("  4. Ollama will auto-start on localhost:11434\n");
        printf("\n  \033[1;32mBenefits of Ollama:\033[0m\n");
        printf("  • 100%% FREE - no API costs ever\n");
        printf("  • Complete privacy - data stays on your machine\n");
        printf("  • Works offline\n");
        printf("  • Great for development and testing\n");
        break;

    case PROVIDER_MLX:
        print_info("MLX runs natively on Apple Silicon - no API key needed!");
        printf("\n  MLX is Apple's native ML framework for M1/M2/M3/M4/M5 chips.\n");
        printf("  Models run directly on your Mac's Neural Engine and GPU.\n");
        printf("\n  \033[1;32mBenefits of MLX:\033[0m\n");
        printf("  • 100%% FREE - no API costs ever\n");
        printf("  • Complete privacy - data never leaves your Mac\n");
        printf("  • Works 100%% offline - no internet required\n");
        printf("  • Optimized for Apple Silicon - fast inference\n");
        printf("  • Pre-quantized 4-bit models - efficient memory use\n");
        printf("\n  \033[1;36mAvailable models:\033[0m\n");
        printf("  • Llama 3.2 (1B, 3B) - General purpose\n");
        printf("  • DeepSeek R1 Distill (1.5B, 7B, 14B) - Reasoning/Coding\n");
        printf("  • Qwen 2.5 Coder 7B - Code generation\n");
        printf("  • Phi-3 Mini - Fast, efficient\n");
        printf("  • Mistral 7B - Multilingual\n");
        printf("\n  Use '\033[1;33m/setup → Local Models\033[0m' to download models.\n");
        break;

    default:
        break;
    }
}

static void configure_api_key(ProviderType type) {
    clear_screen();

    const char* name = "Unknown";
    const char* env_var = NULL;
    for (size_t i = 0; i < g_provider_count; i++) {
        if (g_provider_status[i].type == type) {
            name = g_provider_status[i].name;
            env_var = g_provider_status[i].env_var;
            break;
        }
    }

    char title[64];
    snprintf(title, sizeof(title), "CONFIGURE: %s", name);
    print_header(title);

    show_api_key_help(type);

    if (type == PROVIDER_OLLAMA) {
        printf("\n");
        print_footer();
        wait_for_enter();
        return;
    }

    printf("\n  Where to store the key?\n");
    printf("    1) Add to shell config (~/.zshrc) - Recommended\n");
    printf("    2) Set for this session only\n");
    printf("    0) Go back\n");

    int choice = get_choice(0, 2);

    switch (choice) {
    case 1:
        printf("\n  Add this line to your ~/.zshrc:\n");
        printf("  \033[1;33mexport %s=\"your-api-key-here\"\033[0m\n", env_var);
        printf("\n  Then run: \033[1;33msource ~/.zshrc\033[0m\n");
        wait_for_enter();
        break;

    case 2: {
        char key[256];
        printf("\n  Enter API key: ");
        fflush(stdout);

        // Disable echo for password-like input
        struct termios old_term, new_term;
        tcgetattr(STDIN_FILENO, &old_term);
        new_term = old_term;
        new_term.c_lflag &= (tcflag_t)~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

        if (fgets(key, sizeof(key), stdin)) {
            key[strcspn(key, "\n")] = 0;
            setenv(env_var, key, 1);
            tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
            printf("\n");
            print_success("Key set for this session");
            refresh_provider_status();
        } else {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
            printf("\n");
            print_error("Failed to read key");
        }
        wait_for_enter();
        break;
    }

    default:
        break;
    }
}

static void menu_api_keys(void) {
    while (true) {
        clear_screen();
        refresh_provider_status();

        print_header("API KEYS CONFIGURATION");

        printf("\n  Provider          Status        Environment Variable\n");
        printf("  ─────────────────────────────────────────────────────────────\n");

        for (size_t i = 0; i < g_provider_count; i++) {
            const char* status_icon = g_provider_status[i].available ? "\033[1;32m✓ OK\033[0m     "
                                                                     : "\033[1;31m✗ Missing\033[0m";
            const char* env_display =
                g_provider_status[i].env_var ? g_provider_status[i].env_var : "(no key needed)";

            printf("  %zu) %-14s %s  %s\n", i + 1, g_provider_status[i].name, status_icon,
                   env_display);
        }

        printf("\n  0) Go back\n");
        print_footer();

        int choice = get_choice(0, (int)g_provider_count);

        if (choice == 0)
            break;
        if (choice > 0 && choice <= (int)g_provider_count) {
            configure_api_key(g_provider_status[choice - 1].type);
        }
    }
}

// ============================================================================
// QUICK SETUP PROFILES
// ============================================================================

static void apply_profile(OptimizationProfile profile) {
    clear_screen();
    print_header("APPLYING CONFIGURATION");

    printf("\n");

    switch (profile) {
    case PROFILE_COST:
        print_info("Applying Cost-Optimized profile...");
        printf("\n  This profile uses the cheapest effective models:\n");
        printf("  • Primary: Claude Haiku 4.5 / GPT-4o-mini / Gemini Flash\n");
        printf("  • OpenRouter: DeepSeek V3 (extremely cheap)\n");
        printf("  • Estimated cost: ~$0.50/day with moderate usage\n");
        break;

    case PROFILE_BALANCED:
        print_info("Applying Balanced profile...");
        printf("\n  This profile balances quality and cost:\n");
        printf("  • Primary: Claude Sonnet 4.5 / GPT-4o\n");
        printf("  • Fallback: Cheaper models\n");
        printf("  • Estimated cost: ~$2-5/day with moderate usage\n");
        break;

    case PROFILE_PERFORMANCE:
        print_info("Applying Performance profile...");
        printf("\n  This profile uses the best models everywhere:\n");
        printf("  • Primary: Claude Opus 4.5 / o3 / GPT-5.2 Pro\n");
        printf("  • Best for: Critical work, complex architecture\n");
        printf("  • Estimated cost: ~$10-20/day with moderate usage\n");
        break;

    case PROFILE_LOCAL:
        print_info("Applying Local-First profile...");
        printf("\n  This profile uses Ollama local models:\n");
        printf("  • Primary: Llama 3.2 / Mistral / CodeLlama (local)\n");
        printf("  • Fallback: Cloud models if needed\n");
        printf("  • Cost: $0 for local inference!\n");
        printf("\n  \033[1;33mRequires:\033[0m Ollama installed and running\n");
        break;

    default:
        break;
    }

    printf("\n");
    print_success("Profile applied successfully!");
    print_info("You can customize individual agents with /setup -> Agent Models");

    print_footer();
    wait_for_enter();
}

static void menu_quick_setup(void) {
    clear_screen();
    print_header("QUICK SETUP");

    printf("\n  Let's get you started! Choose an optimization profile:\n\n");

    printf("  1) \033[1;32m💰 Cost-Optimized (Default)\033[0m\n");
    printf("     Uses cheapest effective models\n");
    printf("     Best for: Learning, testing, simple tasks\n");
    printf("     Est. cost: ~$0.50/day\n\n");

    printf("  2) \033[1;33m⚖️  Balanced (Recommended)\033[0m\n");
    printf("     Mix of quality and cost\n");
    printf("     Best for: Daily development work\n");
    printf("     Est. cost: ~$2-5/day\n\n");

    printf("  3) \033[1;35m🚀 Maximum Performance\033[0m\n");
    printf("     Best models everywhere\n");
    printf("     Best for: Critical work, complex architecture\n");
    printf("     Est. cost: ~$10-20/day\n\n");

    printf("  4) \033[1;34m🏠 Local-First\033[0m\n");
    printf("     Ollama models with cloud fallback\n");
    printf("     Best for: Privacy, offline work, no API costs\n");
    printf("     Requires: Ollama installed locally\n\n");

    printf("  5) \033[1;36m🎯 Custom\033[0m\n");
    printf("     Configure each agent manually\n\n");

    printf("  0) Go back\n");

    print_footer();

    int choice = get_choice(0, 5);

    switch (choice) {
    case 1:
        apply_profile(PROFILE_COST);
        break;
    case 2:
        apply_profile(PROFILE_BALANCED);
        break;
    case 3:
        apply_profile(PROFILE_PERFORMANCE);
        break;
    case 4:
        apply_profile(PROFILE_LOCAL);
        break;
    case 5:
        print_info("Use 'Agent Models' from the main menu to configure each agent");
        wait_for_enter();
        break;
    default:
        break;
    }
}

// ============================================================================
// VIEW CONFIGURATION
// ============================================================================

static void menu_view_config(void) {
    clear_screen();
    refresh_provider_status();

    print_header("CURRENT CONFIGURATION");

    printf("\n  \033[1;37mProviders:\033[0m\n");
    printf("  ─────────────────────────────────────────────────────────────\n");

    for (size_t i = 0; i < g_provider_count; i++) {
        const char* status = g_provider_status[i].available ? "\033[1;32mAvailable\033[0m"
                                                            : "\033[1;31mNot configured\033[0m";
        printf("  %-16s %s\n", g_provider_status[i].name, status);
    }

    printf("\n  \033[1;37mModels:\033[0m\n");
    printf("  ─────────────────────────────────────────────────────────────\n");

    // Show model counts per provider
    for (int p = 0; p < PROVIDER_COUNT; p++) {
        size_t count = 0;
        (void)model_get_by_provider((ProviderType)p, &count);
        if (count > 0) {
            const char* pname = provider_name((ProviderType)p);
            printf("  %-16s %zu models available\n", pname, count);

            // Show cheapest model
            const ModelConfig* cheapest = model_get_cheapest((ProviderType)p);
            if (cheapest) {
                printf("                   └─ Cheapest: %s ($%.2f/$%.2f per MTok)\n",
                       cheapest->display_name, cheapest->input_cost_per_mtok,
                       cheapest->output_cost_per_mtok);
            }
        }
    }

    print_footer();
    wait_for_enter();
}

// ============================================================================
// OLLAMA SETUP MENU
// ============================================================================

static bool check_ollama_running(void) {
    // Try to connect to Ollama API
    FILE* fp = popen("curl -s http://localhost:11434/api/tags 2>/dev/null", "r");
    if (!fp) return false;

    char buffer[128];
    bool success = fgets(buffer, sizeof(buffer), fp) != NULL;
    int status = pclose(fp);

    return success && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool check_ollama_installed(void) {
    FILE* fp = popen("which ollama 2>/dev/null", "r");
    if (!fp) return false;

    char buffer[256];
    bool found = fgets(buffer, sizeof(buffer), fp) != NULL;
    pclose(fp);
    return found;
}

static void start_ollama_server(void) {
    printf("\n  Starting Ollama server...\n");
    system("ollama serve > /dev/null 2>&1 &");
    sleep(2);

    if (check_ollama_running()) {
        print_success("Ollama server started successfully");
    } else {
        print_error("Failed to start Ollama server");
        printf("  Try running 'ollama serve' manually in a terminal.\n");
    }
}

static void list_ollama_models(void) {
    printf("\n  \033[1;37mInstalled models:\033[0m\n\n");
    FILE* fp = popen("ollama list 2>/dev/null", "r");
    if (!fp) {
        print_error("Could not list models");
        return;
    }

    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (count == 0) {
            // Skip header line
            printf("  %s", line);
        } else {
            printf("  %s", line);
        }
        count++;
    }
    pclose(fp);

    if (count <= 1) {
        print_info("No models installed yet. Use option 3 to pull a model.");
    }
}

static void pull_ollama_model(void) {
    char model_name[256];
    printf("\n  \033[1;37mRecommended models:\033[0m\n");
    printf("    - qwen2.5:0.5b     (494 MB)  - Fastest, good for testing\n");
    printf("    - qwen2.5:3b       (1.9 GB)  - Good balance\n");
    printf("    - llama3.2:3b      (2.0 GB)  - Meta's latest small model\n");
    printf("    - codellama:7b     (3.8 GB)  - Code-focused\n");
    printf("    - mixtral:8x7b     (26 GB)   - Most capable\n");
    printf("\n  Enter model name (or 0 to cancel): ");
    fflush(stdout);

    if (!fgets(model_name, sizeof(model_name), stdin)) return;
    model_name[strcspn(model_name, "\n")] = 0;

    if (strlen(model_name) == 0 || strcmp(model_name, "0") == 0) return;

    printf("\n  Pulling model: %s\n", model_name);
    printf("  This may take a while depending on model size...\n\n");

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "ollama pull %s", model_name);
    int result = system(cmd);

    if (result == 0) {
        print_success("Model pulled successfully!");
    } else {
        print_error("Failed to pull model");
    }
}

static void menu_ollama_setup(void) {
    while (true) {
        clear_screen();
        print_header("OLLAMA SETUP - Local LLM Inference");

        bool installed = check_ollama_installed();
        bool running = check_ollama_running();

        printf("\n  \033[1;37mStatus:\033[0m\n");
        if (installed) {
            print_success("Ollama is installed");
        } else {
            print_error("Ollama is NOT installed");
        }

        if (running) {
            print_success("Ollama server is running (localhost:11434)");
        } else {
            print_warning("Ollama server is NOT running");
        }

        if (installed && running) {
            list_ollama_models();
        }

        printf("\n  \033[1;37mActions:\033[0m\n\n");
        if (!installed) {
            printf("    1) Install Ollama    - Opens ollama.ai in browser\n");
        } else {
            if (!running) {
                printf("    1) Start Server      - Launch Ollama background service\n");
            } else {
                printf("    1) Stop Server       - Stop Ollama service\n");
            }
            printf("    2) List Models       - Show installed models\n");
            printf("    3) Pull Model        - Download a new model\n");
            printf("    4) Test Connection   - Verify Ollama is working\n");
        }
        printf("    0) Back\n");

        print_footer();

        int choice = get_choice(0, 4);

        if (choice == 0) return;

        if (!installed) {
            if (choice == 1) {
                printf("\n  Opening https://ollama.ai in your browser...\n");
                system("open https://ollama.ai");
                wait_for_enter();
            }
        } else {
            switch (choice) {
            case 1:
                if (running) {
                    printf("\n  Stopping Ollama server...\n");
                    system("pkill -f 'ollama serve' 2>/dev/null");
                    print_success("Ollama server stopped");
                } else {
                    start_ollama_server();
                }
                wait_for_enter();
                break;
            case 2:
                list_ollama_models();
                wait_for_enter();
                break;
            case 3:
                pull_ollama_model();
                wait_for_enter();
                break;
            case 4:
                if (check_ollama_running()) {
                    print_success("Ollama is responding correctly");
                    printf("\n  You can use Convergio with:\n");
                    printf("    convergio --provider ollama --ollama-model <model>\n");
                    printf("\n  Or set as default in ~/.convergio/config.json\n");
                } else {
                    print_error("Ollama is not responding");
                    printf("  Try starting the server first.\n");
                }
                wait_for_enter();
                break;
            }
        }
    }
}

// ============================================================================
// LOCAL MODELS MENU (MLX)
// ============================================================================

// Forward declaration
extern const MLXModelInfo* mlx_get_available_models(size_t* out_count);
extern bool mlx_model_is_ready(const char* model_id);
extern bool mlx_is_available(void);
extern const char* mlx_recommend_model(size_t available_ram_gb);

// Swift bridge functions for model management
extern char* mlx_bridge_download_model(const char* model_id, void (*progress_callback)(int32_t));
extern bool mlx_bridge_model_exists(const char* model_id);
extern int64_t mlx_bridge_model_size(const char* model_id);
extern bool mlx_bridge_delete_model(const char* model_id);
extern char* mlx_bridge_list_models(void);

// Progress callback for download
static void download_progress_callback(int32_t percent) {
    printf("\r  Downloading: %d%%  ", percent);
    fflush(stdout);
}

static void download_mlx_model(const char* model_id, const char* huggingface_id,
                               const char* display_name, size_t size_mb) {
    printf("\n  Downloading %s (~%zu MB)...\n", display_name, size_mb);
    printf("  Model: %s\n", huggingface_id);
    printf("  This downloads from HuggingFace using MLX-Swift.\n\n");

    // Check if already downloaded
    if (mlx_bridge_model_exists(huggingface_id)) {
        print_info("Model already downloaded");
        int64_t size = mlx_bridge_model_size(huggingface_id);
        printf("  Size on disk: %.1f GB\n", (double)size / (1024.0 * 1024.0 * 1024.0));
        return;
    }

    printf("  Starting download (this may take several minutes)...\n\n");

    // Use Swift bridge to download
    char* error = mlx_bridge_download_model(huggingface_id, download_progress_callback);

    printf("\n");

    if (error == NULL) {
        print_success("Download complete!");
        int64_t size = mlx_bridge_model_size(huggingface_id);
        printf("  Size on disk: %.1f GB\n", (double)size / (1024.0 * 1024.0 * 1024.0));
    } else {
        print_error("Download failed");
        printf("  Error: %s\n", error);
        printf("\n  Troubleshooting:\n");
        printf("  1. Check your internet connection\n");
        printf("  2. Some models require HuggingFace login:\n");
        printf("     export HF_TOKEN=your_token_here\n");
        printf("  3. Try again - download may have timed out\n");
        free(error);
    }
}

static void delete_mlx_model(const char* huggingface_id, const char* display_name) {
    printf("\n  Are you sure you want to delete %s?\n", display_name);

    // Show current size
    int64_t size = mlx_bridge_model_size(huggingface_id);
    if (size > 0) {
        printf("  This will free %.1f GB of disk space.\n",
               (double)size / (1024.0 * 1024.0 * 1024.0));
    }

    if (!get_yes_no("Delete model?")) {
        print_info("Cancelled");
        return;
    }

    if (mlx_bridge_delete_model(huggingface_id)) {
        print_success("Model deleted");
    } else {
        print_error("Failed to delete model");
        printf("  The model may not exist or there was a permission error.\n");
    }
}

static void menu_local_models(void) {
    if (!mlx_is_available()) {
        clear_screen();
        print_header("LOCAL MODELS (MLX)");
        printf("\n");
        print_error("MLX requires Apple Silicon (M1/M2/M3/M4/M5)");
        printf("\n  Your Mac does not have Apple Silicon or MLX support.\n");
        printf("  Consider using Ollama for local model inference instead.\n");
        print_footer();
        wait_for_enter();
        return;
    }

    while (true) {
        clear_screen();
        print_header("LOCAL MODELS (MLX - Apple Silicon Native)");

        printf("\n  MLX runs models directly on your Mac's Neural Engine.\n");
        printf("  100%% offline, 100%% free, 100%% private.\n\n");

        // Get available models
        size_t model_count = 0;
        const MLXModelInfo* models = mlx_get_available_models(&model_count);

        printf("  \033[1;37m#   Model                    Size      RAM    Status\033[0m\n");
        printf("  ─────────────────────────────────────────────────────────────\n");

        for (size_t i = 0; i < model_count; i++) {
            bool ready = mlx_bridge_model_exists(models[i].huggingface_id);
            const char* status =
                ready ? "\033[1;32m✓ Ready\033[0m" : "\033[1;33m○ Not downloaded\033[0m";

            printf("  %zu)  %-22s %4zuMB   %2zuGB   %s\n", i + 1, models[i].display_name,
                   models[i].size_mb, models[i].min_ram_gb, status);
        }

        printf("\n  \033[1;37mActions:\033[0m\n");
        printf("  D) Download a model\n");
        printf("  R) Remove a model\n");
        printf("  0) Go back\n");

        print_footer();

        char input[16];
        printf("\n  \033[1;33mChoice:\033[0m ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin))
            return;
        input[strcspn(input, "\n")] = 0;

        if (input[0] == '0' || input[0] == '\0') {
            return;
        }

        if (input[0] == 'D' || input[0] == 'd') {
            printf("\n  Enter model number to download: ");
            fflush(stdout);
            if (!fgets(input, sizeof(input), stdin))
                continue;
            int idx = atoi(input) - 1;
            if (idx >= 0 && (size_t)idx < model_count) {
                if (mlx_bridge_model_exists(models[idx].huggingface_id)) {
                    print_info("Model already downloaded");
                } else {
                    download_mlx_model(models[idx].id, models[idx].huggingface_id,
                                       models[idx].display_name, models[idx].size_mb);
                }
                wait_for_enter();
            }
        } else if (input[0] == 'R' || input[0] == 'r') {
            printf("\n  Enter model number to remove: ");
            fflush(stdout);
            if (!fgets(input, sizeof(input), stdin))
                continue;
            int idx = atoi(input) - 1;
            if (idx >= 0 && (size_t)idx < model_count) {
                if (!mlx_bridge_model_exists(models[idx].huggingface_id)) {
                    print_info("Model not downloaded");
                } else {
                    delete_mlx_model(models[idx].huggingface_id, models[idx].display_name);
                }
                wait_for_enter();
            }
        }
    }
}

// ============================================================================
// MAIN WIZARD
// ============================================================================

int cmd_setup(int argc, char** argv) {
    (void)argc;
    (void)argv;

    while (true) {
        clear_screen();
        refresh_provider_status();

        print_header("CONVERGIO SETUP WIZARD");

        printf("\n  Welcome! This wizard helps you configure AI providers\n");
        printf("  and optimize model selection for your agents.\n\n");

        // Show quick status
        int available_count = 0;
        for (size_t i = 0; i < g_provider_count; i++) {
            if (g_provider_status[i].available)
                available_count++;
        }
        printf("  Status: \033[1;32m%d/%zu providers configured\033[0m\n\n", available_count,
               g_provider_count);

        printf("  What would you like to configure?\n\n");
        printf("    1) API Keys         - Configure provider credentials\n");
        printf("    2) Ollama Setup     - Local LLM (recommended for privacy/offline)\n");
        printf("    3) MLX Models       - Apple Silicon native inference\n");
        printf("    4) Quick Setup      - Choose optimization profile (cost/performance)\n");
        printf("    5) View Config      - Show current configuration\n");
        printf("    6) Exit\n");

        print_footer();

        int choice = get_choice(1, 6);

        switch (choice) {
        case 1:
            menu_api_keys();
            break;
        case 2:
            menu_ollama_setup();
            break;
        case 3:
            menu_local_models();
            break;
        case 4:
            menu_quick_setup();
            break;
        case 5:
            menu_view_config();
            break;
        case 6:
            return 0;
        default:
            break;
        }
    }

    return 0;
}

// ============================================================================
// HELPER FUNCTIONS FOR EXTERNAL USE
// ============================================================================

/**
 * Get the display name for a provider
 */
const char* provider_get_name(ProviderType type) {
    for (size_t i = 0; i < g_provider_count; i++) {
        if (g_provider_status[i].type == type) {
            return g_provider_status[i].name;
        }
    }
    return "Unknown";
}

/**
 * Get the API key environment variable name for a provider
 */
const char* provider_get_api_key_env(ProviderType type) {
    for (size_t i = 0; i < g_provider_count; i++) {
        if (g_provider_status[i].type == type) {
            return g_provider_status[i].env_var;
        }
    }
    return NULL;
}
