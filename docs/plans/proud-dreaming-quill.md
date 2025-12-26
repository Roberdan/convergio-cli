# Piano: Implementazione OpenRouter + Ollama Provider

## Obiettivo
Aggiungere due nuovi provider a ConvergioCLI:
1. **OpenRouter** - Accesso a 300+ modelli via API OpenAI-compatibile
2. **Ollama** - Modelli locali senza costi API

---

## File da Modificare/Creare

### Nuovi File
| File | Righe Stimate | Descrizione |
|------|---------------|-------------|
| `src/providers/openrouter.c` | ~400 | Adapter OpenRouter (basato su openai.c) |
| `src/providers/ollama.c` | ~350 | Adapter Ollama locale |

### File da Modificare
| File | Modifiche |
|------|-----------|
| `include/nous/provider.h` | +2 enum (PROVIDER_OPENROUTER, PROVIDER_OLLAMA già esiste) |
| `src/providers/provider.c` | +registrazione provider, +array modelli, +lookup |
| `src/providers/tools.c` | +build/parse tool calls per OpenRouter |
| `src/providers/streaming.c` | +case SSE per OpenRouter/Ollama |
| `config/models.json` | +sezioni openrouter e ollama |
| `Makefile` | +compilazione nuovi .c |

---

## Step 1: Aggiornare provider.h

```c
typedef enum {
    PROVIDER_ANTHROPIC   = 0,
    PROVIDER_OPENAI      = 1,
    PROVIDER_GEMINI      = 2,
    PROVIDER_OPENROUTER  = 3,  // NUOVO
    PROVIDER_OLLAMA      = 4,  // NUOVO (già definito ma non implementato)
    PROVIDER_COUNT       = 5
} ProviderType;
```

Aggiungere export functions:
```c
Provider* openrouter_provider_create(void);
Provider* ollama_provider_create(void);
```

---

## Step 2: Implementare openrouter.c

### Caratteristiche
- **Base URL**: `https://openrouter.ai/api/v1`
- **Auth**: `Authorization: Bearer {OPENROUTER_API_KEY}`
- **Header extra**: `HTTP-Referer: https://convergio.dev` (richiesto da OpenRouter)
- **Formato**: 100% OpenAI-compatibile

### Struttura
```c
typedef struct {
    bool initialized;
    CURL* curl;
    ProviderErrorInfo last_error;
    pthread_mutex_t mutex;
} OpenRouterProviderData;
```

### Funzioni da implementare
- `openrouter_init()` - Verifica OPENROUTER_API_KEY
- `openrouter_shutdown()` - Cleanup
- `openrouter_validate_key()` - Check env var
- `openrouter_chat()` - Riuso logica OpenAI con URL/headers diversi
- `openrouter_chat_with_tools()` - Tool calling OpenAI-style
- `openrouter_stream_chat()` - SSE streaming
- `openrouter_provider_create()` - Factory

### Differenze da OpenAI
1. URL base diverso
2. Header `HTTP-Referer` obbligatorio
3. Header `X-Title` opzionale (nome app)
4. Model ID include provider: `anthropic/claude-3-opus`

---

## Step 3: Implementare ollama.c

### Caratteristiche
- **Base URL**: `http://localhost:11434` (configurabile)
- **Auth**: Nessuna (locale)
- **Formato**: Ollama API (simile a OpenAI ma non identico)

### Endpoint Ollama
- `POST /api/generate` - Generazione testo
- `POST /api/chat` - Chat con history
- `GET /api/tags` - Lista modelli installati

### Request Format
```json
{
  "model": "llama3.2",
  "prompt": "...",
  "system": "...",
  "stream": false,
  "options": {
    "num_ctx": 4096
  }
}
```

### Response Format
```json
{
  "response": "...",
  "done": true,
  "eval_count": 123,
  "prompt_eval_count": 45
}
```

### Funzioni da implementare
- `ollama_init()` - Verifica che Ollama sia in esecuzione
- `ollama_shutdown()` - Cleanup
- `ollama_validate_key()` - Sempre true (no key), ma ping localhost
- `ollama_chat()` - POST /api/generate
- `ollama_stream_chat()` - Streaming con `"stream": true`
- `ollama_list_models()` - GET /api/tags
- `ollama_provider_create()` - Factory

### Gestione Errori Specifica
- `PROVIDER_ERR_NETWORK` se Ollama non raggiungibile
- Log warning se porta diversa da default

---

## Step 4: Aggiornare provider.c

### Array modelli OpenRouter
```c
static ModelConfig g_openrouter_models[] = {
    {
        .id = "deepseek/deepseek-r1",
        .display_name = "DeepSeek R1",
        .provider = PROVIDER_OPENROUTER,
        .input_cost_per_mtok = 0.55,
        .output_cost_per_mtok = 2.19,
        .context_window = 64000,
        .supports_tools = true,
        .tier = COST_TIER_CHEAP
    },
    {
        .id = "mistralai/mistral-large-2411",
        .display_name = "Mistral Large",
        .provider = PROVIDER_OPENROUTER,
        .input_cost_per_mtok = 2.0,
        .output_cost_per_mtok = 6.0,
        .context_window = 128000,
        .supports_tools = true,
        .tier = COST_TIER_MID
    },
    {
        .id = "meta-llama/llama-3.3-70b-instruct",
        .display_name = "Llama 3.3 70B",
        .provider = PROVIDER_OPENROUTER,
        .input_cost_per_mtok = 0.40,
        .output_cost_per_mtok = 0.40,
        .context_window = 131072,
        .supports_tools = true,
        .tier = COST_TIER_CHEAP
    },
    {
        .id = "qwen/qwen-2.5-72b-instruct",
        .display_name = "Qwen 2.5 72B",
        .provider = PROVIDER_OPENROUTER,
        .input_cost_per_mtok = 0.35,
        .output_cost_per_mtok = 0.40,
        .context_window = 131072,
        .supports_tools = true,
        .tier = COST_TIER_CHEAP
    }
};
```

### Array modelli Ollama
```c
static ModelConfig g_ollama_models[] = {
    {
        .id = "llama3.2",
        .display_name = "Llama 3.2 (Local)",
        .provider = PROVIDER_OLLAMA,
        .input_cost_per_mtok = 0.0,  // Gratuito
        .output_cost_per_mtok = 0.0,
        .context_window = 131072,
        .supports_tools = false,
        .tier = COST_TIER_CHEAP
    },
    {
        .id = "mistral",
        .display_name = "Mistral 7B (Local)",
        .provider = PROVIDER_OLLAMA,
        .input_cost_per_mtok = 0.0,
        .output_cost_per_mtok = 0.0,
        .context_window = 32768,
        .supports_tools = false,
        .tier = COST_TIER_CHEAP
    },
    {
        .id = "codellama",
        .display_name = "Code Llama (Local)",
        .provider = PROVIDER_OLLAMA,
        .input_cost_per_mtok = 0.0,
        .output_cost_per_mtok = 0.0,
        .context_window = 16384,
        .supports_tools = false,
        .tier = COST_TIER_CHEAP
    }
};
```

### Registry Init
```c
g_providers[PROVIDER_OPENROUTER] = openrouter_provider_create();
g_providers[PROVIDER_OLLAMA] = ollama_provider_create();
```

### Provider Names
```c
static const char* g_provider_names[] = {
    [PROVIDER_ANTHROPIC] = "anthropic",
    [PROVIDER_OPENAI] = "openai",
    [PROVIDER_GEMINI] = "gemini",
    [PROVIDER_OPENROUTER] = "openrouter",
    [PROVIDER_OLLAMA] = "ollama"
};
```

---

## Step 5: Aggiornare config/models.json

```json
{
  "providers": {
    "openrouter": {
      "name": "OpenRouter",
      "api_key_env": "OPENROUTER_API_KEY",
      "base_url": "https://openrouter.ai/api/v1",
      "docs_url": "https://openrouter.ai/docs",
      "models": {
        "deepseek/deepseek-r1": {
          "display_name": "DeepSeek R1",
          "api_id": "deepseek/deepseek-r1",
          "input_cost": 0.55,
          "output_cost": 2.19,
          "context_window": 64000,
          "supports_tools": true,
          "tier": "cheap",
          "best_for": ["reasoning", "coding", "math"]
        },
        "mistralai/mistral-large-2411": {
          "display_name": "Mistral Large",
          "api_id": "mistralai/mistral-large-2411",
          "input_cost": 2.0,
          "output_cost": 6.0,
          "context_window": 128000,
          "supports_tools": true,
          "tier": "mid",
          "best_for": ["multilingual", "european", "GDPR"]
        },
        "meta-llama/llama-3.3-70b-instruct": {
          "display_name": "Llama 3.3 70B",
          "api_id": "meta-llama/llama-3.3-70b-instruct",
          "input_cost": 0.40,
          "output_cost": 0.40,
          "context_window": 131072,
          "supports_tools": true,
          "tier": "cheap",
          "best_for": ["open-source", "general purpose"]
        }
      }
    },
    "ollama": {
      "name": "Ollama (Local)",
      "api_key_env": null,
      "base_url": "http://localhost:11434",
      "docs_url": "https://ollama.ai/docs",
      "models": {
        "llama3.2": {
          "display_name": "Llama 3.2 (Local)",
          "api_id": "llama3.2",
          "input_cost": 0.0,
          "output_cost": 0.0,
          "context_window": 131072,
          "supports_tools": false,
          "tier": "cheap",
          "best_for": ["offline", "privacy", "no-cost"]
        },
        "mistral": {
          "display_name": "Mistral 7B (Local)",
          "api_id": "mistral",
          "input_cost": 0.0,
          "output_cost": 0.0,
          "context_window": 32768,
          "supports_tools": false,
          "tier": "cheap"
        },
        "codellama": {
          "display_name": "Code Llama (Local)",
          "api_id": "codellama",
          "input_cost": 0.0,
          "output_cost": 0.0,
          "context_window": 16384,
          "supports_tools": false,
          "tier": "cheap",
          "best_for": ["coding", "offline"]
        }
      }
    }
  }
}
```

---

## Step 6: Aggiornare Makefile

```makefile
PROVIDER_SRCS = src/providers/provider.c \
                src/providers/anthropic.c \
                src/providers/openai.c \
                src/providers/gemini.c \
                src/providers/openrouter.c \
                src/providers/ollama.c \
                src/providers/streaming.c \
                src/providers/retry.c \
                src/providers/tokens.c \
                src/providers/tools.c
```

---

## Step 7: Aggiornare tools.c (per OpenRouter)

OpenRouter usa formato OpenAI, quindi:
```c
// In build_tools_json()
case PROVIDER_OPENROUTER:
    return build_openai_tools_json(tools, count);

// In parse_tool_calls()
case PROVIDER_OPENROUTER:
    return parse_openai_tool_calls(response, out_count);
```

---

## Step 8: Aggiornare streaming.c

```c
// Nel SSE parser, aggiungere case:
case PROVIDER_OPENROUTER:
    // Stesso formato OpenAI
    return parse_openai_sse_chunk(chunk, out_text);

case PROVIDER_OLLAMA:
    // Formato Ollama (JSON line per line)
    return parse_ollama_stream_chunk(chunk, out_text);
```

---

## Step 9: Test

### Test OpenRouter
```bash
export OPENROUTER_API_KEY="sk-or-..."
convergio --provider openrouter --model deepseek/deepseek-r1
> Test query
```

### Test Ollama
```bash
# Assicurarsi che Ollama sia in esecuzione
ollama serve &
ollama pull llama3.2

convergio --provider ollama --model llama3.2
> Test query
```

### Test Fallback
```json
// In models.json agent_defaults
"test_agent": {
  "primary": "ollama/llama3.2",
  "fallback": "openrouter/deepseek/deepseek-r1"
}
```

---

## Ordine di Implementazione

1. **provider.h** - Aggiungere enum e forward declarations
2. **openrouter.c** - Implementare (riuso 80% da openai.c)
3. **ollama.c** - Implementare
4. **provider.c** - Registrazione + array modelli
5. **tools.c** - Aggiungere case OpenRouter
6. **streaming.c** - Aggiungere case per entrambi
7. **Makefile** - Aggiungere compilazione
8. **config/models.json** - Aggiungere configurazioni
9. **Test manuali** - Verificare funzionamento

---

## Stima Effort

| Componente | Effort |
|------------|--------|
| openrouter.c | ~400 righe (80% riuso openai.c) |
| ollama.c | ~350 righe |
| Modifiche esistenti | ~150 righe |
| **Totale** | **~900 righe** |

---

## Variabili Ambiente Richieste

```bash
# OpenRouter
OPENROUTER_API_KEY=sk-or-v1-...

# Ollama (opzionale, per URL custom)
OLLAMA_HOST=http://localhost:11434
```

---

## Note Importanti

1. **OpenRouter** richiede header `HTTP-Referer` - senza non funziona
2. **Ollama** deve essere in esecuzione localmente - gestire graceful error
3. I model ID OpenRouter includono il provider originale (`anthropic/claude-3-opus`)
4. Ollama non supporta tool calling nativamente - `supports_tools = false`
5. Costi Ollama = 0 (locale) - ottimo per testing e sviluppo

---

## Step 10: Wizard di Configurazione Guidata

### Nuovo Comando: `/setup`

Aggiungere comando interattivo per configurare provider e modelli per agente.

### File da Creare/Modificare
| File | Descrizione |
|------|-------------|
| `src/core/commands/setup_wizard.c` | ~500 righe - Wizard completo |
| `src/core/commands/commands.c` | +entry nella tabella comandi |
| `include/nous/commands.h` | +dichiarazione cmd_setup |

### Flusso del Wizard

```
┌─────────────────────────────────────────────────────────────────┐
│  CONVERGIO SETUP WIZARD                                         │
│─────────────────────────────────────────────────────────────────│
│                                                                 │
│  Welcome! This wizard will help you configure AI providers      │
│  and models for your agents.                                    │
│                                                                 │
│  What would you like to configure?                              │
│                                                                 │
│    1) API Keys         - Configure provider credentials         │
│    2) Agent Models     - Set primary/fallback models per agent  │
│    3) Quick Setup      - Guided setup for new users             │
│    4) View Config      - Show current configuration             │
│    5) Exit                                                      │
│                                                                 │
│  Choice [1-5]: _                                                │
└─────────────────────────────────────────────────────────────────┘
```

### Menu 1: Configurazione API Keys

```
┌─────────────────────────────────────────────────────────────────┐
│  API KEYS CONFIGURATION                                         │
│─────────────────────────────────────────────────────────────────│
│                                                                 │
│  Provider          Status        API Key                        │
│  ─────────────────────────────────────────────────────────────  │
│  1) Anthropic      ✓ OK          ANTHROPIC_API_KEY              │
│  2) OpenAI         ✓ OK          OPENAI_API_KEY                 │
│  3) Google Gemini  ✗ Missing     GOOGLE_API_KEY                 │
│  4) OpenRouter     ✗ Missing     OPENROUTER_API_KEY             │
│  5) Ollama         ✓ Local       (no key needed)                │
│                                                                 │
│  Select provider to configure [1-5] or 0 to go back: _          │
└─────────────────────────────────────────────────────────────────┘
```

Se l'utente seleziona un provider mancante:

```
┌─────────────────────────────────────────────────────────────────┐
│  CONFIGURE: OpenRouter                                          │
│─────────────────────────────────────────────────────────────────│
│                                                                 │
│  OpenRouter provides access to 300+ AI models through a         │
│  unified API. It's useful for:                                  │
│  • Access to DeepSeek, Mistral, Llama, Qwen models              │
│  • Automatic fallback between providers                         │
│  • Competitive pricing                                          │
│                                                                 │
│  To get an API key:                                             │
│  1. Go to https://openrouter.ai/keys                            │
│  2. Sign up or log in                                           │
│  3. Create a new API key                                        │
│  4. Copy the key (starts with 'sk-or-')                         │
│                                                                 │
│  Where to store the key?                                        │
│    1) macOS Keychain (Recommended - most secure)                │
│    2) Environment variable (~/.zshrc or ~/.bashrc)              │
│    3) Enter now (will guide you)                                │
│                                                                 │
│  Choice [1-3]: _                                                │
└─────────────────────────────────────────────────────────────────┘
```

### Menu 2: Configurazione Modelli per Agente

```
┌─────────────────────────────────────────────────────────────────┐
│  AGENT MODEL CONFIGURATION                                      │
│─────────────────────────────────────────────────────────────────│
│                                                                 │
│  Select an agent to configure:                                  │
│                                                                 │
│  #   Agent        Role                    Current Model         │
│  ─────────────────────────────────────────────────────────────  │
│  1)  Ali          Chief of Staff          claude-opus-4.5       │
│  2)  Baccio       Architect               claude-opus-4.5       │
│  3)  Marco        Developer               claude-sonnet-4.5     │
│  4)  Luca         Security Expert         o3                    │
│  5)  Thor         Reviewer                gpt-5.2-instant       │
│  6)  Nina         Data Analyst            gemini-3.0-pro        │
│  ... (more agents)                                              │
│                                                                 │
│  Select agent [1-49] or 0 to go back: _                         │
└─────────────────────────────────────────────────────────────────┘
```

Quando l'utente seleziona un agente:

```
┌─────────────────────────────────────────────────────────────────┐
│  CONFIGURE: Marco (Developer)                                   │
│─────────────────────────────────────────────────────────────────│
│                                                                 │
│  Marco specializes in code generation, debugging, and           │
│  software development tasks.                                    │
│                                                                 │
│  Current Configuration:                                         │
│  • Primary:  anthropic/claude-sonnet-4.5                        │
│  • Fallback: openai/gpt-5-codex                                 │
│                                                                 │
│  What would you like to change?                                 │
│    1) Change primary model                                      │
│    2) Change fallback model                                     │
│    3) View recommended models for this role                     │
│    4) Go back                                                   │
│                                                                 │
│  Choice [1-4]: _                                                │
└─────────────────────────────────────────────────────────────────┘
```

### Selezione Modello (con filtri intelligenti)

```
┌─────────────────────────────────────────────────────────────────┐
│  SELECT PRIMARY MODEL FOR: Marco                                │
│─────────────────────────────────────────────────────────────────│
│                                                                 │
│  Filter by:                                                     │
│    Provider: [All] Anthropic | OpenAI | Gemini | OpenRouter     │
│    Tier:     [All] Premium | Mid | Cheap                        │
│    Feature:  [All] Tools | Vision | Long Context                │
│                                                                 │
│  Available Models (sorted by recommendation for coding):        │
│                                                                 │
│  #   Model                    Provider    Cost/MTok   Context   │
│  ─────────────────────────────────────────────────────────────  │
│  1)  claude-sonnet-4.5       Anthropic   $3/$15      1M    ⭐   │
│  2)  gpt-5-codex             OpenAI      $2/$10      200K  ⭐   │
│  3)  claude-opus-4.5         Anthropic   $15/$75     200K       │
│  4)  deepseek-r1             OpenRouter  $0.55/$2    64K        │
│  5)  llama-3.3-70b           OpenRouter  $0.40/$0.40 131K       │
│  6)  codellama (local)       Ollama      FREE        16K        │
│                                                                 │
│  ⭐ = Recommended for this agent's role                         │
│  ⚠  Models grayed out if API key missing                        │
│                                                                 │
│  Select model [1-N] or 0 to go back: _                          │
└─────────────────────────────────────────────────────────────────┘
```

### Menu 3: Quick Setup (per nuovi utenti)

```
┌─────────────────────────────────────────────────────────────────┐
│  QUICK SETUP                                                    │
│─────────────────────────────────────────────────────────────────│
│                                                                 │
│  Let's get you started! Choose a setup profile:                 │
│                                                                 │
│  1) 💰 Budget-Friendly                                          │
│     Uses cheapest models (GPT-4.1-mini, Gemini Flash)           │
│     Best for: Learning, testing, simple tasks                   │
│     Est. cost: ~$0.50/day with moderate usage                   │
│                                                                 │
│  2) ⚖️  Balanced (Recommended)                                   │
│     Mix of quality and cost (Sonnet 4.5, GPT-5.2)               │
│     Best for: Daily development work                            │
│     Est. cost: ~$2-5/day with moderate usage                    │
│                                                                 │
│  3) 🚀 Maximum Quality                                          │
│     Best models everywhere (Opus 4.5, o3, GPT-5.2-Pro)          │
│     Best for: Critical work, complex architecture               │
│     Est. cost: ~$10-20/day with moderate usage                  │
│                                                                 │
│  4) 🏠 Local-First                                               │
│     Ollama models with cloud fallback                           │
│     Best for: Privacy, offline work, no API costs               │
│     Requires: Ollama installed locally                          │
│                                                                 │
│  5) 🎯 Custom (configure each agent manually)                   │
│                                                                 │
│  Choice [1-5]: _                                                │
└─────────────────────────────────────────────────────────────────┘
```

### Implementazione Tecnica

```c
// src/core/commands/setup_wizard.c

#include "nous/commands.h"
#include "nous/provider.h"
#include "nous/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static void clear_screen(void) {
    printf("\033[2J\033[H");
}

static void print_header(const char* title) {
    const Theme* t = theme_get_current();
    printf("\n%s┌─────────────────────────────────────────────────────────────────┐%s\n",
           t->border, theme_reset());
    printf("%s│  %s%-60s%s │%s\n", t->border, t->heading, title, t->border, theme_reset());
    printf("%s│─────────────────────────────────────────────────────────────────│%s\n",
           t->border, theme_reset());
}

static int get_choice(int min, int max) {
    char input[16];
    printf("\n  Choice [%d-%d]: ", min, max);
    if (!fgets(input, sizeof(input), stdin)) return -1;
    int choice = atoi(input);
    if (choice < min || choice > max) return -1;
    return choice;
}

static bool check_api_key(ProviderType type) {
    const char* env = provider_get_api_key_env(type);
    if (!env) return true;  // Ollama non richiede key
    return getenv(env) != NULL;
}

// ============================================================================
// API KEY CONFIGURATION
// ============================================================================

static void show_api_key_help(ProviderType type) {
    switch (type) {
        case PROVIDER_ANTHROPIC:
            printf("\n  To get an Anthropic API key:\n");
            printf("  1. Go to https://console.anthropic.com/settings/keys\n");
            printf("  2. Sign up or log in\n");
            printf("  3. Create a new API key\n");
            printf("  4. Copy the key (starts with 'sk-ant-')\n");
            break;
        case PROVIDER_OPENAI:
            printf("\n  To get an OpenAI API key:\n");
            printf("  1. Go to https://platform.openai.com/api-keys\n");
            printf("  2. Sign up or log in\n");
            printf("  3. Create a new API key\n");
            printf("  4. Copy the key (starts with 'sk-')\n");
            break;
        case PROVIDER_OPENROUTER:
            printf("\n  To get an OpenRouter API key:\n");
            printf("  1. Go to https://openrouter.ai/keys\n");
            printf("  2. Sign up or log in (can use Google/GitHub)\n");
            printf("  3. Create a new API key\n");
            printf("  4. Copy the key (starts with 'sk-or-')\n");
            printf("\n  Benefits of OpenRouter:\n");
            printf("  • Access to 300+ models (DeepSeek, Mistral, Llama, etc.)\n");
            printf("  • Single API key for all providers\n");
            printf("  • Often cheaper than direct API access\n");
            break;
        case PROVIDER_GEMINI:
            printf("\n  To get a Google Gemini API key:\n");
            printf("  1. Go to https://aistudio.google.com/apikey\n");
            printf("  2. Sign in with Google account\n");
            printf("  3. Create a new API key\n");
            printf("  4. Copy the key (starts with 'AIza')\n");
            break;
        case PROVIDER_OLLAMA:
            printf("\n  Ollama runs locally - no API key needed!\n");
            printf("\n  To install Ollama:\n");
            printf("  1. Go to https://ollama.ai\n");
            printf("  2. Download and install for macOS\n");
            printf("  3. Run: ollama pull llama3.2\n");
            printf("  4. Ollama will auto-start on localhost:11434\n");
            break;
        default:
            break;
    }
}

static void configure_api_key(ProviderType type) {
    clear_screen();
    print_header(provider_get_name(type));

    show_api_key_help(type);

    printf("\n  Where to store the key?\n");
    printf("    1) macOS Keychain (Recommended - most secure)\n");
    printf("    2) Add to shell config (~/.zshrc)\n");
    printf("    3) Enter now for this session only\n");
    printf("    0) Go back\n");

    int choice = get_choice(0, 3);

    switch (choice) {
        case 1:
            // Usa oauth.m per salvare in Keychain
            printf("\n  Opening Keychain setup...\n");
            // keychain_store_api_key(type);
            break;
        case 2:
            printf("\n  Add this line to your ~/.zshrc:\n");
            printf("  export %s=\"your-api-key-here\"\n",
                   provider_get_api_key_env(type));
            printf("\n  Then run: source ~/.zshrc\n");
            break;
        case 3: {
            char key[256];
            printf("\n  Enter API key: ");
            if (fgets(key, sizeof(key), stdin)) {
                key[strcspn(key, "\n")] = 0;
                setenv(provider_get_api_key_env(type), key, 1);
                printf("  ✓ Key set for this session\n");
            }
            break;
        }
    }
}

// ============================================================================
// MODEL SELECTION
// ============================================================================

typedef struct {
    const char* model_id;
    const char* display_name;
    ProviderType provider;
    double input_cost;
    double output_cost;
    size_t context;
    bool available;  // API key presente
    bool recommended; // Per questo ruolo
} ModelOption;

static void show_model_selector(const char* agent_name, bool is_primary) {
    clear_screen();

    char title[64];
    snprintf(title, sizeof(title), "SELECT %s MODEL FOR: %s",
             is_primary ? "PRIMARY" : "FALLBACK", agent_name);
    print_header(title);

    // Ottieni modelli disponibili
    ModelOption models[64];
    size_t count = 0;

    // Popola da tutti i provider
    for (int p = 0; p < PROVIDER_COUNT; p++) {
        bool key_ok = check_api_key(p);
        size_t pcount;
        const ModelConfig* pmodels = model_get_by_provider(p, &pcount);

        for (size_t i = 0; i < pcount && count < 64; i++) {
            models[count].model_id = pmodels[i].id;
            models[count].display_name = pmodels[i].display_name;
            models[count].provider = p;
            models[count].input_cost = pmodels[i].input_cost_per_mtok;
            models[count].output_cost = pmodels[i].output_cost_per_mtok;
            models[count].context = pmodels[i].context_window;
            models[count].available = key_ok;
            models[count].recommended = false; // TODO: basato su ruolo
            count++;
        }
    }

    // Stampa tabella
    printf("\n  #   Model                    Provider    Cost/MTok   Context\n");
    printf("  ─────────────────────────────────────────────────────────────\n");

    for (size_t i = 0; i < count; i++) {
        const char* status = models[i].available ? "" : " ⚠";
        const char* star = models[i].recommended ? " ⭐" : "";

        printf("  %2zu) %-22s %-11s $%.2f/$%.2f  %zuK%s%s\n",
               i + 1,
               models[i].display_name,
               provider_get_name(models[i].provider),
               models[i].input_cost,
               models[i].output_cost,
               models[i].context / 1000,
               star, status);
    }

    printf("\n  ⚠ = API key missing (configure in API Keys menu)\n");

    int choice = get_choice(0, (int)count);
    if (choice > 0) {
        // Salva configurazione
        char model_path[128];
        snprintf(model_path, sizeof(model_path), "%s/%s",
                 provider_get_name(models[choice-1].provider),
                 models[choice-1].model_id);

        // router_set_agent_model(agent_name, model_path, is_primary);
        printf("\n  ✓ Set %s model for %s to: %s\n",
               is_primary ? "primary" : "fallback",
               agent_name, model_path);
    }
}

// ============================================================================
// MAIN WIZARD
// ============================================================================

int cmd_setup(int argc, char** argv) {
    (void)argc; (void)argv;

    while (true) {
        clear_screen();
        print_header("CONVERGIO SETUP WIZARD");

        printf("\n  Welcome! This wizard will help you configure AI providers\n");
        printf("  and models for your agents.\n\n");
        printf("  What would you like to configure?\n\n");
        printf("    1) API Keys         - Configure provider credentials\n");
        printf("    2) Agent Models     - Set primary/fallback models per agent\n");
        printf("    3) Quick Setup      - Guided setup for new users\n");
        printf("    4) View Config      - Show current configuration\n");
        printf("    5) Exit\n");

        int choice = get_choice(1, 5);

        switch (choice) {
            case 1: menu_api_keys(); break;
            case 2: menu_agent_models(); break;
            case 3: menu_quick_setup(); break;
            case 4: menu_view_config(); break;
            case 5: return 0;
            default: break;
        }
    }

    return 0;
}
```

### Registrazione Comando

In `commands.c`:
```c
static const ReplCommand COMMANDS[] = {
    // ... existing commands ...
    {"setup",       "Configure providers and agent models",  cmd_setup},
    // ...
};
```

### Persistenza Configurazione

La configurazione viene salvata in `~/.convergio/agent_models.json`:

```json
{
  "version": 1,
  "last_updated": "2025-12-13T10:30:00Z",
  "agents": {
    "ali": {
      "primary": "anthropic/claude-opus-4.5",
      "fallback": "openai/gpt-5.2-pro"
    },
    "marco": {
      "primary": "openrouter/deepseek/deepseek-r1",
      "fallback": "ollama/codellama"
    }
  }
}
```

### Stima Effort Aggiuntivo

| Componente | Righe |
|------------|-------|
| setup_wizard.c | ~500 |
| Modifiche commands.c | ~20 |
| agent_models.json persistence | ~100 |
| **Totale Step 10** | **~620** |

### Totale Progetto Aggiornato

| Componente | Righe |
|------------|-------|
| openrouter.c | ~400 |
| ollama.c | ~350 |
| Modifiche provider esistenti | ~150 |
| setup_wizard.c | ~500 |
| Persistenza config | ~100 |
| **TOTALE** | **~1500 righe** |
