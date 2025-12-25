/**
 * NOUS Intent Parser
 *
 * Transforms natural language into executable semantic structures
 * Uses a semantic-first approach rather than syntactic parsing
 */

#include "nous/nous.h"
#include <arm_neon.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// INTENT PATTERNS - Semantic fingerprints
// ============================================================================

typedef struct {
    const char* pattern;
    IntentKind kind;
    float base_confidence;
} IntentPattern;

// These patterns are semantic anchors, not rigid syntax
static const IntentPattern INTENT_PATTERNS[] = {
    // Creation intents
    {"voglio", INTENT_KIND_CREATE, 0.7f},
    {"crea", INTENT_KIND_CREATE, 0.9f},
    {"genera", INTENT_KIND_CREATE, 0.85f},
    {"costruisci", INTENT_KIND_CREATE, 0.85f},
    {"fai", INTENT_KIND_CREATE, 0.6f},
    {"scrivi", INTENT_KIND_CREATE, 0.8f},
    {"nuovo", INTENT_KIND_CREATE, 0.5f},

    // Transformation intents
    {"trasforma", INTENT_KIND_TRANSFORM, 0.9f},
    {"cambia", INTENT_KIND_TRANSFORM, 0.85f},
    {"modifica", INTENT_KIND_TRANSFORM, 0.85f},
    {"aggiorna", INTENT_KIND_TRANSFORM, 0.8f},
    {"converti", INTENT_KIND_TRANSFORM, 0.9f},
    {"migliora", INTENT_KIND_TRANSFORM, 0.75f},

    // Search intents
    {"trova", INTENT_KIND_FIND, 0.9f},
    {"cerca", INTENT_KIND_FIND, 0.9f},
    {"dove", INTENT_KIND_FIND, 0.7f},
    {"quale", INTENT_KIND_FIND, 0.6f},
    {"mostra", INTENT_KIND_FIND, 0.7f},

    // Connection intents
    {"collega", INTENT_KIND_CONNECT, 0.9f},
    {"connetti", INTENT_KIND_CONNECT, 0.9f},
    {"unisci", INTENT_KIND_CONNECT, 0.85f},
    {"relaziona", INTENT_KIND_CONNECT, 0.85f},
    {"associa", INTENT_KIND_CONNECT, 0.8f},

    // Understanding intents
    {"spiega", INTENT_KIND_UNDERSTAND, 0.9f},
    {"capisco", INTENT_KIND_UNDERSTAND, 0.7f},
    {"perché", INTENT_KIND_UNDERSTAND, 0.8f},
    {"come funziona", INTENT_KIND_UNDERSTAND, 0.9f},
    {"cosa significa", INTENT_KIND_UNDERSTAND, 0.85f},

    // Collaboration intents
    {"insieme", INTENT_KIND_COLLABORATE, 0.7f},
    {"collabora", INTENT_KIND_COLLABORATE, 0.9f},
    {"aiuta", INTENT_KIND_COLLABORATE, 0.75f},
    {"lavoriamo", INTENT_KIND_COLLABORATE, 0.8f},

    // Emotional intents
    {"mi sento", INTENT_KIND_FEEL, 0.85f},
    {"sono felice", INTENT_KIND_FEEL, 0.8f},
    {"sono preoccupato", INTENT_KIND_FEEL, 0.8f},
    {"non mi piace", INTENT_KIND_FEEL, 0.7f},

    {NULL, 0, 0.0f}};

// ============================================================================
// URGENCY MARKERS
// ============================================================================

typedef struct {
    const char* marker;
    float urgency_boost;
} UrgencyMarker;

static const UrgencyMarker URGENCY_MARKERS[] = {
    {"urgente", 0.9f},         {"subito", 0.85f},          {"adesso", 0.8f},
    {"immediatamente", 0.95f}, {"prima possibile", 0.75f}, {"quando puoi", 0.3f},
    {"con calma", 0.1f},       {"appena riesci", 0.4f},    {NULL, 0.0f}};

// ============================================================================
// TEXT UTILITIES (SIMD-optimized)
// ============================================================================

// Convert to lowercase using NEON
static void to_lowercase_neon(char* str, size_t len) {
    size_t i = 0;

    // Process 16 bytes at a time
    for (; i + 16 <= len; i += 16) {
        uint8x16_t chars = vld1q_u8((uint8_t*)&str[i]);

        // Create masks for uppercase letters (A-Z: 65-90)
        uint8x16_t lower_bound = vcgtq_u8(chars, vdupq_n_u8(64)); // > 64
        uint8x16_t upper_bound = vcltq_u8(chars, vdupq_n_u8(91)); // < 91
        uint8x16_t is_upper = vandq_u8(lower_bound, upper_bound);

        // Add 32 to uppercase letters to convert to lowercase
        uint8x16_t offset = vandq_u8(is_upper, vdupq_n_u8(32));
        chars = vaddq_u8(chars, offset);

        vst1q_u8((uint8_t*)&str[i], chars);
    }

    // Handle remaining bytes
    for (; i < len; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] += 32;
        }
    }
}

// Fast substring search using NEON (Boyer-Moore-Horspool simplified)
static const char* find_substring(const char* haystack, size_t hay_len, const char* needle,
                                  size_t needle_len) {
    if (needle_len == 0)
        return haystack;
    if (needle_len > hay_len)
        return NULL;

    // Use NEON for first-character scan
    uint8x16_t first_char = vdupq_n_u8((uint8_t)needle[0]);

    for (size_t i = 0; i <= hay_len - needle_len;) {
        // Scan for first character using SIMD
        if (i + 16 <= hay_len) {
            uint8x16_t chunk = vld1q_u8((const uint8_t*)&haystack[i]);
            uint8x16_t matches = vceqq_u8(chunk, first_char);

            uint64_t match_bits =
                vget_lane_u64(vreinterpret_u64_u8(vget_low_u8(matches)), 0) |
                ((uint64_t)vget_lane_u64(vreinterpret_u64_u8(vget_high_u8(matches)), 0) << 8);

            if (match_bits == 0) {
                i += 16;
                continue;
            }

            // Check matches
            for (size_t j = 0; j < 16 && i + j <= hay_len - needle_len; j++) {
                if (haystack[i + j] == needle[0]) {
                    if (memcmp(&haystack[i + j], needle, needle_len) == 0) {
                        return &haystack[i + j];
                    }
                }
            }
            i += 16;
        } else {
            if (haystack[i] == needle[0] && memcmp(&haystack[i], needle, needle_len) == 0) {
                return &haystack[i];
            }
            i++;
        }
    }

    return NULL;
}

// ============================================================================
// INTENT PARSING
// ============================================================================

ParsedIntent* nous_parse_intent(const char* input, size_t len) {
    if (!input || len == 0)
        return NULL;

    ParsedIntent* intent = calloc(1, sizeof(ParsedIntent));
    if (!intent)
        return NULL;

    // Copy and normalize input
    char* normalized = malloc(len + 1);
    if (!normalized) {
        free(intent);
        return NULL;
    }
    memcpy(normalized, input, len);
    normalized[len] = '\0';
    to_lowercase_neon(normalized, len);

    // Store raw input
    intent->raw_input = malloc(len + 1);
    if (intent->raw_input) {
        memcpy(intent->raw_input, input, len + 1);
        intent->raw_len = len;
    }

    // Phase 1: Detect intent kind
    float best_confidence = 0.0f;
    IntentKind detected_kind = INTENT_KIND_CREATE; // Default

    for (const IntentPattern* p = INTENT_PATTERNS; p->pattern != NULL; p++) {
        if (find_substring(normalized, len, p->pattern, strlen(p->pattern))) {
            if (p->base_confidence > best_confidence) {
                best_confidence = p->base_confidence;
                detected_kind = p->kind;
            }
        }
    }

    intent->kind = detected_kind;
    intent->confidence = best_confidence;

    // Phase 2: Detect urgency
    float urgency = 0.5f; // Default: medium
    for (const UrgencyMarker* m = URGENCY_MARKERS; m->marker != NULL; m++) {
        if (find_substring(normalized, len, m->marker, strlen(m->marker))) {
            urgency = m->urgency_boost;
            break;
        }
    }
    intent->urgency = urgency;

    // Phase 3: Check for ambiguity
    if (best_confidence < 0.6f) {
        // Prepare clarification questions
        intent->questions = malloc(3 * sizeof(char*));
        if (intent->questions) {
            switch (detected_kind) {
            case INTENT_KIND_CREATE:
                intent->questions[0] = strdup("Cosa vorresti creare esattamente?");
                intent->question_count = 1;
                break;
            case INTENT_KIND_TRANSFORM:
                intent->questions[0] = strdup("Cosa vuoi trasformare?");
                intent->questions[1] = strdup("In cosa vuoi trasformarlo?");
                intent->question_count = 2;
                break;
            case INTENT_KIND_FIND:
                intent->questions[0] = strdup("Cosa stai cercando?");
                intent->question_count = 1;
                break;
            default:
                intent->questions[0] = strdup("Puoi spiegare meglio cosa desideri?");
                intent->question_count = 1;
            }
        }
    }

    free(normalized);
    return intent;
}

void nous_free_intent(ParsedIntent* intent) {
    if (!intent)
        return;

    free(intent->raw_input);

    if (intent->questions) {
        for (size_t i = 0; i < intent->question_count; i++) {
            free(intent->questions[i]);
        }
        free(intent->questions);
    }

    free(intent);
}

// ============================================================================
// INTENT EXECUTION
// ============================================================================

int nous_execute_intent(ParsedIntent* intent) {
    if (!intent || !nous_is_ready())
        return -1;

    // If ambiguous, signal need for clarification
    if (intent->confidence < 0.6f && intent->question_count > 0) {
        return INTENT_PARSE_AMBIGUOUS;
    }

    switch (intent->kind) {
    case INTENT_KIND_CREATE: {
        // Create a new semantic node representing the created thing
        SemanticID node = nous_create_node(SEMANTIC_TYPE_ENTITY, intent->raw_input);
        if (node == SEMANTIC_ID_NULL)
            return -1;
        intent->object = node;
        break;
    }

    case INTENT_KIND_FIND: {
        // This would trigger similarity search
        // For now, create a query node
        SemanticID query = nous_create_node(SEMANTIC_TYPE_INTENT, intent->raw_input);
        if (query == SEMANTIC_ID_NULL)
            return -1;
        intent->subject = query;
        break;
    }

    case INTENT_KIND_TRANSFORM:
    case INTENT_KIND_CONNECT:
    case INTENT_KIND_UNDERSTAND:
    case INTENT_KIND_COLLABORATE:
    case INTENT_KIND_FEEL:
        // These require more complex handling
        // Create placeholder semantic nodes
        intent->subject = nous_create_node(SEMANTIC_TYPE_INTENT, intent->raw_input);
        break;
    }

    return INTENT_PARSE_OK;
}

// ============================================================================
// STREAMING PARSER (for real-time input)
// ============================================================================

typedef struct {
    char* buffer;
    size_t len;
    size_t capacity;
    float running_confidence;
    IntentKind tentative_kind;
    bool needs_more;
} StreamingParser;

StreamingParser* nous_parser_create(void) {
    StreamingParser* parser = calloc(1, sizeof(StreamingParser));
    if (!parser)
        return NULL;

    parser->capacity = 1024;
    parser->buffer = malloc(parser->capacity);
    if (!parser->buffer) {
        free(parser);
        return NULL;
    }

    parser->tentative_kind = INTENT_KIND_CREATE;
    parser->running_confidence = 0.0f;
    parser->needs_more = true;

    return parser;
}

// Feed characters as user types (for real-time understanding)
IntentParseResult nous_parser_feed(StreamingParser* parser, const char* chars, size_t len) {
    if (!parser || !chars)
        return INTENT_PARSE_ERROR;

    // Grow buffer if needed
    if (parser->len + len >= parser->capacity) {
        size_t new_cap = parser->capacity * 2;
        char* new_buf = realloc(parser->buffer, new_cap);
        if (!new_buf)
            return INTENT_PARSE_ERROR;
        parser->buffer = new_buf;
        parser->capacity = new_cap;
    }

    memcpy(parser->buffer + parser->len, chars, len);
    parser->len += len;

    // Quick analysis without full parse
    char* normalized = malloc(parser->len + 1);
    if (!normalized)
        return INTENT_PARSE_ERROR;
    memcpy(normalized, parser->buffer, parser->len);
    normalized[parser->len] = '\0';
    to_lowercase_neon(normalized, parser->len);

    float best_conf = 0.0f;
    for (const IntentPattern* p = INTENT_PATTERNS; p->pattern != NULL; p++) {
        if (find_substring(normalized, parser->len, p->pattern, strlen(p->pattern))) {
            if (p->base_confidence > best_conf) {
                best_conf = p->base_confidence;
                parser->tentative_kind = p->kind;
            }
        }
    }

    parser->running_confidence = best_conf;
    parser->needs_more = (best_conf < 0.7f);

    free(normalized);

    if (best_conf >= 0.7f)
        return INTENT_PARSE_OK;
    if (best_conf >= 0.4f)
        return INTENT_PARSE_INCOMPLETE;
    return INTENT_PARSE_AMBIGUOUS;
}

ParsedIntent* nous_parser_finalize(StreamingParser* parser) {
    if (!parser)
        return NULL;

    ParsedIntent* intent = nous_parse_intent(parser->buffer, parser->len);

    // Clear buffer for reuse
    parser->len = 0;
    parser->running_confidence = 0.0f;
    parser->needs_more = true;

    return intent;
}

void nous_parser_destroy(StreamingParser* parser) {
    if (!parser)
        return;
    free(parser->buffer);
    free(parser);
}
