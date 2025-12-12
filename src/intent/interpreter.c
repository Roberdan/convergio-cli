/**
 * INTENT Language Interpreter
 *
 * A semantic interpreter that understands intent, not syntax.
 * Uses pattern matching and semantic similarity to parse natural expressions.
 */

#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// ============================================================================
// INTERPRETER STATE
// ============================================================================

typedef struct {
    // Current execution context
    SemanticID current_space;
    SemanticID current_agent;
    SemanticID current_subject;

    // Conversation history (for context)
    char** history;
    size_t history_count;
    size_t history_capacity;

    // Variables/bindings
    char** var_names;
    SemanticID* var_values;
    size_t var_count;
    size_t var_capacity;

    // Execution flags
    bool in_block;
    int block_depth;
    char* block_type;  // "agente", "spazio", "quando", etc.

    // Output callback
    void (*output_fn)(const char* text, void* ctx);
    void* output_ctx;

} IntentInterpreter;

static IntentInterpreter* g_interp = NULL;

// ============================================================================
// SEMANTIC PATTERNS
// ============================================================================

typedef enum {
    PATTERN_CREATE_AGENT,
    PATTERN_CREATE_SPACE,
    PATTERN_DEFINE_PERSONALITY,
    PATTERN_ADD_SKILL,
    PATTERN_CONNECT,
    PATTERN_TRANSFORM,
    PATTERN_FIND,
    PATTERN_INVITE,
    PATTERN_PROPOSE,
    PATTERN_VOTE,
    PATTERN_REMEMBER,
    PATTERN_FEEL,
    PATTERN_WHEN,
    PATTERN_SHOW,
    PATTERN_EXPLAIN,
    PATTERN_UNKNOWN
} PatternType;

typedef struct {
    const char* trigger;
    PatternType type;
    const char* description;
} SemanticPattern;

static const SemanticPattern PATTERNS[] = {
    // Creation
    {"agente", PATTERN_CREATE_AGENT, "Define a new agent"},
    {"spazio", PATTERN_CREATE_SPACE, "Define a new space"},
    {"crea", PATTERN_CREATE_AGENT, "Create something"},
    {"nuovo", PATTERN_CREATE_AGENT, "New entity"},

    // Properties
    {"personalità", PATTERN_DEFINE_PERSONALITY, "Set personality traits"},
    {"competenze", PATTERN_ADD_SKILL, "Add skills"},
    {"essenza", PATTERN_DEFINE_PERSONALITY, "Define essence"},

    // Relationships
    {"collega", PATTERN_CONNECT, "Connect concepts"},
    {"connetti", PATTERN_CONNECT, "Connect concepts"},
    {"percorso", PATTERN_CONNECT, "Define a path"},

    // Transformation
    {"trasforma", PATTERN_TRANSFORM, "Transform something"},
    {"converti", PATTERN_TRANSFORM, "Convert something"},

    // Search
    {"trova", PATTERN_FIND, "Find something"},
    {"cerca", PATTERN_FIND, "Search for something"},
    {"mostra", PATTERN_SHOW, "Show something"},

    // Collaboration
    {"invita", PATTERN_INVITE, "Invite to space"},
    {"proposta", PATTERN_PROPOSE, "Make a proposal"},
    {"voto", PATTERN_VOTE, "Cast a vote"},

    // Memory
    {"ricorda", PATTERN_REMEMBER, "Remember something"},
    {"impara", PATTERN_REMEMBER, "Learn from something"},

    // Emotion
    {"mi sento", PATTERN_FEEL, "Express feeling"},
    {"sono", PATTERN_FEEL, "State of being"},

    // Events
    {"quando", PATTERN_WHEN, "When condition"},
    {"ogni", PATTERN_WHEN, "Recurring event"},

    // Understanding
    {"spiega", PATTERN_EXPLAIN, "Explain something"},
    {"perché", PATTERN_EXPLAIN, "Ask why"},
    {"come", PATTERN_EXPLAIN, "Ask how"},

    {NULL, PATTERN_UNKNOWN, NULL}
};

// ============================================================================
// TOKENIZER
// ============================================================================

typedef struct {
    char* text;
    size_t len;
    bool is_string;    // Quoted string
    bool is_keyword;   // Known keyword
    bool is_identifier;
    bool is_number;
    float number_value;
} Token;

typedef struct {
    Token* tokens;
    size_t count;
    size_t capacity;
} TokenStream;

static void token_stream_init(TokenStream* ts) {
    ts->capacity = 64;
    ts->tokens = calloc(ts->capacity, sizeof(Token));
    ts->count = 0;
}

static void token_stream_add(TokenStream* ts, Token tok) {
    if (ts->count >= ts->capacity) {
        ts->capacity *= 2;
        ts->tokens = realloc(ts->tokens, ts->capacity * sizeof(Token));
    }
    ts->tokens[ts->count++] = tok;
}

static void token_stream_free(TokenStream* ts) {
    for (size_t i = 0; i < ts->count; i++) {
        free(ts->tokens[i].text);
    }
    free(ts->tokens);
}

static void tokenize(const char* input, TokenStream* out) {
    token_stream_init(out);

    const char* p = input;
    while (*p) {
        // Skip whitespace
        while (*p && isspace(*p)) p++;
        if (!*p) break;

        Token tok = {0};

        // Handle quoted strings
        if (*p == '"' || *p == '\'') {
            char quote = *p++;
            const char* start = p;
            while (*p && *p != quote) p++;

            tok.len = p - start;
            tok.text = malloc(tok.len + 1);
            memcpy(tok.text, start, tok.len);
            tok.text[tok.len] = '\0';
            tok.is_string = true;

            if (*p == quote) p++;
            token_stream_add(out, tok);
            continue;
        }

        // Handle special characters
        if (strchr(":,()[]{}→", *p) || (*p == '-' && *(p+1) == '>')) {
            tok.len = (*p == '-') ? 2 : 1;
            tok.text = malloc(tok.len + 1);
            memcpy(tok.text, p, tok.len);
            tok.text[tok.len] = '\0';
            p += tok.len;
            token_stream_add(out, tok);
            continue;
        }

        // Handle words/identifiers
        const char* start = p;
        while (*p && !isspace(*p) && !strchr(":,()[]{}\"'", *p)) p++;

        tok.len = p - start;
        tok.text = malloc(tok.len + 1);
        memcpy(tok.text, start, tok.len);
        tok.text[tok.len] = '\0';

        // Check if number
        char* endptr;
        float num = strtof(tok.text, &endptr);
        if (*endptr == '\0') {
            tok.is_number = true;
            tok.number_value = num;
        }

        // Check if keyword
        for (const SemanticPattern* pat = PATTERNS; pat->trigger; pat++) {
            if (strcasecmp(tok.text, pat->trigger) == 0) {
                tok.is_keyword = true;
                break;
            }
        }

        if (!tok.is_number && !tok.is_keyword) {
            tok.is_identifier = true;
        }

        token_stream_add(out, tok);
    }
}

// ============================================================================
// PATTERN MATCHING
// ============================================================================

static PatternType detect_pattern(const char* line) {
    // Convert to lowercase for matching
    char* lower = strdup(line);
    if (!lower) return PATTERN_UNKNOWN;
    for (char* p = lower; *p; p++) {
        *p = (char)tolower((unsigned char)*p);
    }

    PatternType result = PATTERN_UNKNOWN;
    size_t best_pos = SIZE_MAX;

    for (const SemanticPattern* pat = PATTERNS; pat->trigger; pat++) {
        char* found = strstr(lower, pat->trigger);
        if (found) {
            size_t pos = found - lower;
            if (pos < best_pos) {
                best_pos = pos;
                result = pat->type;
            }
        }
    }

    free(lower);
    return result;
}

// ============================================================================
// EXECUTION
// ============================================================================

static void output(const char* text) {
    if (g_interp && g_interp->output_fn) {
        g_interp->output_fn(text, g_interp->output_ctx);
    } else {
        printf("%s\n", text);
    }
}

static int execute_create_agent(TokenStream* ts) {
    // Find agent name
    char* name = NULL;
    char* essence = NULL;

    for (size_t i = 0; i < ts->count; i++) {
        if (ts->tokens[i].is_identifier && !name) {
            name = strdup(ts->tokens[i].text);
        }
        if (ts->tokens[i].is_string && !essence) {
            essence = strdup(ts->tokens[i].text);
        }
    }

    if (!name) {
        output("Nome dell'agente non specificato.");
        return -1;
    }

    if (!essence) {
        essence = strdup("agente generico");
    }

    NousAgent* agent = nous_create_agent(name, essence);
    if (!agent) {
        output("Errore nella creazione dell'agente.");
        free(name);
        free(essence);
        return -1;
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Agente \"%s\" creato.", name);
    output(msg);

    g_interp->current_agent = agent->id;

    free(name);
    free(essence);
    return 0;
}

static int execute_create_space(TokenStream* ts) {
    char* name = NULL;
    char* purpose = NULL;

    for (size_t i = 0; i < ts->count; i++) {
        if (ts->tokens[i].is_identifier && !name) {
            name = strdup(ts->tokens[i].text);
        }
        if (ts->tokens[i].is_string && !purpose) {
            purpose = strdup(ts->tokens[i].text);
        }
    }

    if (!name) {
        output("Nome dello spazio non specificato.");
        return -1;
    }

    if (!purpose) {
        purpose = strdup("spazio collaborativo");
    }

    NousSpace* space = nous_create_space(name, purpose);
    if (!space) {
        output("Errore nella creazione dello spazio.");
        free(name);
        free(purpose);
        return -1;
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Spazio \"%s\" creato.", name);
    output(msg);

    g_interp->current_space = space->id;

    free(name);
    free(purpose);
    return 0;
}

static int execute_connect(TokenStream* ts) {
    // Find two identifiers to connect
    char* from = NULL;
    char* to = NULL;
    float strength = 0.8f;

    for (size_t i = 0; i < ts->count; i++) {
        if (ts->tokens[i].is_string || ts->tokens[i].is_identifier) {
            if (!from) {
                from = ts->tokens[i].text;
            } else if (!to) {
                to = ts->tokens[i].text;
            }
        }
        if (ts->tokens[i].is_number) {
            strength = ts->tokens[i].number_value;
            if (strength > 1.0f) strength /= 100.0f;  // Convert percentage
        }
    }

    if (!from || !to) {
        output("Specifica cosa collegare: collega A con B");
        return -1;
    }

    // Create semantic nodes if they don't exist
    SemanticID from_id = nous_create_node(SEMANTIC_TYPE_CONCEPT, from);
    SemanticID to_id = nous_create_node(SEMANTIC_TYPE_CONCEPT, to);

    nous_connect(from_id, to_id, strength);

    char msg[256];
    snprintf(msg, sizeof(msg), "Collegato \"%s\" con \"%s\" (forza: %.0f%%)",
             from, to, strength * 100);
    output(msg);

    return 0;
}

static int execute_find(TokenStream* ts) {
    // Build search query from tokens (with bounds checking)
    char query[512] = {0};
    size_t query_len = 0;
    for (size_t i = 0; i < ts->count && query_len < sizeof(query) - 2; i++) {
        if (ts->tokens[i].is_string || ts->tokens[i].is_identifier) {
            if (query_len > 0 && query_len < sizeof(query) - 1) {
                query[query_len++] = ' ';
            }
            size_t tok_len = strlen(ts->tokens[i].text);
            size_t copy_len = (query_len + tok_len < sizeof(query) - 1) ? tok_len : (sizeof(query) - 1 - query_len);
            memcpy(query + query_len, ts->tokens[i].text, copy_len);
            query_len += copy_len;
        }
    }
    query[query_len] = '\0';

    if (strlen(query) == 0) {
        output("Cosa vuoi cercare?");
        return -1;
    }

    // Create query embedding and search
    // For now, just acknowledge
    char msg[256];
    snprintf(msg, sizeof(msg), "Cerco: \"%s\"...", query);
    output(msg);

    // TODO(#1): Implement actual semantic search

    output("(Ricerca semantica non ancora implementata)");
    return 0;
}

static int execute_feel(TokenStream* ts) {
    char* feeling = NULL;

    for (size_t i = 0; i < ts->count; i++) {
        if (ts->tokens[i].is_string || ts->tokens[i].is_identifier) {
            feeling = ts->tokens[i].text;
            break;
        }
    }

    if (!feeling) {
        output("Come ti senti?");
        return 0;
    }

    // Create emotional node
    SemanticID feeling_node = nous_create_node(SEMANTIC_TYPE_FEELING, feeling);

    // Empathetic response based on feeling
    char msg[256];
    if (strstr(feeling, "frustrat") || strstr(feeling, "stress")) {
        snprintf(msg, sizeof(msg),
                 "Capisco che ti senti %s. Vuoi parlarne, o preferisci una pausa?",
                 feeling);
    } else if (strstr(feeling, "ispirat") || strstr(feeling, "creativ")) {
        snprintf(msg, sizeof(msg),
                 "Bellissimo! Catturiamo questa energia. Cosa vuoi creare?");
    } else if (strstr(feeling, "stanc") || strstr(feeling, "esaust")) {
        snprintf(msg, sizeof(msg),
                 "Forse è il momento di una pausa. Il riposo è parte del processo.");
    } else {
        snprintf(msg, sizeof(msg),
                 "Grazie per condividere come ti senti. Sono qui se vuoi parlare.");
    }

    output(msg);
    (void)feeling_node;  // Use the node in future
    return 0;
}

static int execute_remember(TokenStream* ts) {
    // Build memory from tokens (with bounds checking)
    char memory[512] = {0};
    size_t mem_len = 0;
    for (size_t i = 0; i < ts->count; i++) {
        if (ts->tokens[i].is_string) {
            size_t tok_len = strlen(ts->tokens[i].text);
            size_t copy_len = (tok_len < sizeof(memory) - 1) ? tok_len : (sizeof(memory) - 1);
            memcpy(memory, ts->tokens[i].text, copy_len);
            mem_len = copy_len;
            memory[mem_len] = '\0';
            break;
        }
    }

    if (mem_len == 0) {
        // Join all non-keyword tokens
        for (size_t i = 0; i < ts->count && mem_len < sizeof(memory) - 2; i++) {
            if (!ts->tokens[i].is_keyword && ts->tokens[i].text) {
                if (mem_len > 0 && mem_len < sizeof(memory) - 1) {
                    memory[mem_len++] = ' ';
                }
                size_t tok_len = strlen(ts->tokens[i].text);
                size_t copy_len = (mem_len + tok_len < sizeof(memory) - 1) ? tok_len : (sizeof(memory) - 1 - mem_len);
                memcpy(memory + mem_len, ts->tokens[i].text, copy_len);
                mem_len += copy_len;
            }
        }
        memory[mem_len] = '\0';
    }

    if (strlen(memory) == 0) {
        output("Cosa devo ricordare?");
        return 0;
    }

    // Create memory node
    SemanticID mem_node = nous_create_node(SEMANTIC_TYPE_MEMORY, memory);

    char msg[256];
    snprintf(msg, sizeof(msg), "Ricorderò: \"%s\"", memory);
    output(msg);

    (void)mem_node;
    return 0;
}

static int execute_explain(TokenStream* ts) {
    char* topic = NULL;

    for (size_t i = 0; i < ts->count; i++) {
        if (ts->tokens[i].is_string || ts->tokens[i].is_identifier) {
            topic = ts->tokens[i].text;
            break;
        }
    }

    if (!topic) {
        output("Cosa vuoi che ti spieghi?");
        return 0;
    }

    char msg[256];
    snprintf(msg, sizeof(msg),
             "Per spiegarti \"%s\" avrei bisogno di più contesto. "
             "Puoi essere più specifico?", topic);
    output(msg);

    return 0;
}

static int execute_line(const char* line) {
    if (!line || strlen(line) == 0) return 0;

    // Handle block continuation
    if (g_interp->in_block) {
        // Check for block end (dedent)
        if (line[0] != ' ' && line[0] != '\t') {
            g_interp->in_block = false;
            g_interp->block_depth = 0;
            free(g_interp->block_type);
            g_interp->block_type = NULL;
        }
    }

    // Add to history
    if (g_interp->history_count >= g_interp->history_capacity) {
        g_interp->history_capacity = g_interp->history_capacity * 2 + 16;
        g_interp->history = realloc(g_interp->history,
                                    g_interp->history_capacity * sizeof(char*));
    }
    g_interp->history[g_interp->history_count++] = strdup(line);

    // Detect pattern
    PatternType pattern = detect_pattern(line);

    // Tokenize
    TokenStream ts;
    tokenize(line, &ts);

    int result = 0;

    switch (pattern) {
        case PATTERN_CREATE_AGENT:
            result = execute_create_agent(&ts);
            break;
        case PATTERN_CREATE_SPACE:
            result = execute_create_space(&ts);
            break;
        case PATTERN_CONNECT:
            result = execute_connect(&ts);
            break;
        case PATTERN_FIND:
        case PATTERN_SHOW:
            result = execute_find(&ts);
            break;
        case PATTERN_FEEL:
            result = execute_feel(&ts);
            break;
        case PATTERN_REMEMBER:
            result = execute_remember(&ts);
            break;
        case PATTERN_EXPLAIN:
            result = execute_explain(&ts);
            break;
        case PATTERN_UNKNOWN:
        default:
            // Try to make sense of it anyway
            if (ts.count > 0) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Non sono sicuro di aver capito. "
                         "Puoi riformulare?");
                output(msg);
            }
            break;
    }

    token_stream_free(&ts);
    return result;
}

// ============================================================================
// PUBLIC API
// ============================================================================

int nous_intent_init(void) {
    if (g_interp) return 0;

    g_interp = calloc(1, sizeof(IntentInterpreter));
    if (!g_interp) return -1;

    g_interp->history_capacity = 256;
    g_interp->history = calloc(g_interp->history_capacity, sizeof(char*));

    g_interp->var_capacity = 64;
    g_interp->var_names = calloc(g_interp->var_capacity, sizeof(char*));
    g_interp->var_values = calloc(g_interp->var_capacity, sizeof(SemanticID));

    return 0;
}

void nous_intent_shutdown(void) {
    if (!g_interp) return;

    for (size_t i = 0; i < g_interp->history_count; i++) {
        free(g_interp->history[i]);
    }
    free(g_interp->history);

    for (size_t i = 0; i < g_interp->var_count; i++) {
        free(g_interp->var_names[i]);
    }
    free(g_interp->var_names);
    free(g_interp->var_values);

    free(g_interp->block_type);
    free(g_interp);
    g_interp = NULL;
}

void nous_intent_set_output(void (*fn)(const char*, void*), void* ctx) {
    if (g_interp) {
        g_interp->output_fn = fn;
        g_interp->output_ctx = ctx;
    }
}

int nous_intent_execute(const char* code) {
    if (!g_interp || !code) return -1;

    // Split into lines and execute each
    char* input = strdup(code);
    char* line = strtok(input, "\n");

    while (line) {
        // Skip comments
        if (line[0] == '/' && line[1] == '/') {
            line = strtok(NULL, "\n");
            continue;
        }

        // Skip empty lines
        char* trimmed = line;
        while (*trimmed && isspace(*trimmed)) trimmed++;
        if (*trimmed == '\0') {
            line = strtok(NULL, "\n");
            continue;
        }

        int result = execute_line(trimmed);
        if (result != 0) {
            free(input);
            return result;
        }

        line = strtok(NULL, "\n");
    }

    free(input);
    return 0;
}

// Interactive REPL for the INTENT language
int nous_intent_repl(void) {
    if (!g_interp) {
        if (nous_intent_init() != 0) return -1;
    }

    output("INTENT Interpreter v0.1");
    output("Esprimi le tue intenzioni in linguaggio naturale.");
    output("");

    char line[1024];
    while (1) {
        printf("intent> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';

        // Exit commands
        if (strcmp(line, "esci") == 0 || strcmp(line, "quit") == 0) break;

        nous_intent_execute(line);
    }

    return 0;
}
