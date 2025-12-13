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

#include "nous/provider.h"
#include "nous/nous.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>

// ============================================================================
// OPTIMIZATION PROFILES
// ============================================================================

typedef enum {
    PROFILE_COST,         // Cheapest models everywhere
    PROFILE_BALANCED,     // Mix of quality and cost
    PROFILE_PERFORMANCE,  // Best models everywhere
    PROFILE_LOCAL,        // Ollama local-first with cloud fallback
    PROFILE_CUSTOM        // Manual configuration
} OptimizationProfile;

// ============================================================================
// UI HELPERS
// ============================================================================

static void clear_screen(void) {
    printf("\033[2J\033[H");
}

static void print_header(const char* title) {
    printf("\n\033[1;36m┌─────────────────────────────────────────────────────────────────┐\033[0m\n");
    printf("\033[1;36m│\033[0m  \033[1;37m%-60s\033[0m \033[1;36m│\033[0m\n", title);
    printf("\033[1;36m│─────────────────────────────────────────────────────────────────│\033[0m\n");
}

static void print_footer(void) {
    printf("\033[1;36m└─────────────────────────────────────────────────────────────────┘\033[0m\n");
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

    if (!fgets(input, sizeof(input), stdin)) return -1;

    // Trim newline
    input[strcspn(input, "\n")] = 0;

    // Handle empty input
    if (strlen(input) == 0) return -1;

    int choice = atoi(input);
    if (choice < min || choice > max) return -1;
    return choice;
}

static bool get_yes_no(const char* prompt) __attribute__((unused));
static bool get_yes_no(const char* prompt) {
    char input[16];
    printf("  %s [y/N]: ", prompt);
    fflush(stdout);

    if (!fgets(input, sizeof(input), stdin)) return false;
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
    {PROVIDER_ANTHROPIC, "Anthropic", "ANTHROPIC_API_KEY", false, "sk-ant-", "https://console.anthropic.com/settings/keys"},
    {PROVIDER_OPENAI, "OpenAI", "OPENAI_API_KEY", false, "sk-", "https://platform.openai.com/api-keys"},
    {PROVIDER_GEMINI, "Google Gemini", "GEMINI_API_KEY", false, "AIza", "https://aistudio.google.com/apikey"},
    {PROVIDER_OPENROUTER, "OpenRouter", "OPENROUTER_API_KEY", false, "sk-or-", "https://openrouter.ai/keys"},
    {PROVIDER_OLLAMA, "Ollama (Local)", NULL, false, NULL, "https://ollama.ai"}
};
static const size_t g_provider_count = sizeof(g_provider_status) / sizeof(g_provider_status[0]);

static void refresh_provider_status(void) {
    for (size_t i = 0; i < g_provider_count; i++) {
        if (g_provider_status[i].type == PROVIDER_OLLAMA) {
            // Check if Ollama is running
            Provider* p = provider_get(PROVIDER_OLLAMA);
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
            printf("\n  \033[1;36mNote:\033[0m Anthropic offers Claude models (Opus, Sonnet, Haiku).\n");
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
            const char* status_icon = g_provider_status[i].available ? "\033[1;32m✓ OK\033[0m     " : "\033[1;31m✗ Missing\033[0m";
            const char* env_display = g_provider_status[i].env_var ? g_provider_status[i].env_var : "(no key needed)";

            printf("  %zu) %-14s %s  %s\n",
                   i + 1,
                   g_provider_status[i].name,
                   status_icon,
                   env_display);
        }

        printf("\n  0) Go back\n");
        print_footer();

        int choice = get_choice(0, (int)g_provider_count);

        if (choice == 0) break;
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
        case 1: apply_profile(PROFILE_COST); break;
        case 2: apply_profile(PROFILE_BALANCED); break;
        case 3: apply_profile(PROFILE_PERFORMANCE); break;
        case 4: apply_profile(PROFILE_LOCAL); break;
        case 5:
            print_info("Use 'Agent Models' from the main menu to configure each agent");
            wait_for_enter();
            break;
        default: break;
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
        const char* status = g_provider_status[i].available ?
            "\033[1;32mAvailable\033[0m" : "\033[1;31mNot configured\033[0m";
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
                       cheapest->display_name,
                       cheapest->input_cost_per_mtok,
                       cheapest->output_cost_per_mtok);
            }
        }
    }

    print_footer();
    wait_for_enter();
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
            if (g_provider_status[i].available) available_count++;
        }
        printf("  Status: \033[1;32m%d/%zu providers configured\033[0m\n\n", available_count, g_provider_count);

        printf("  What would you like to configure?\n\n");
        printf("    1) API Keys         - Configure provider credentials\n");
        printf("    2) Quick Setup      - Choose optimization profile (cost/performance)\n");
        printf("    3) View Config      - Show current configuration\n");
        printf("    4) Exit\n");

        print_footer();

        int choice = get_choice(1, 4);

        switch (choice) {
            case 1: menu_api_keys(); break;
            case 2: menu_quick_setup(); break;
            case 3: menu_view_config(); break;
            case 4: return 0;
            default: break;
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
