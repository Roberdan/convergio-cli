/**
 * CONVERGIO EDUCATION - LINGUISTIC TOOLS
 *
 * Dictionary, grammar analysis, verb conjugation, and pronunciation tools
 * for language learning with accessibility support.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/education.h"
#include "nous/orchestrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Helper to extract JSON string value
static char* extract_json_string(const char* json, const char* key) {
    if (!json || !key) return NULL;

    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);

    char* pos = strstr(json, search);
    if (!pos) return NULL;

    pos = strchr(pos, ':');
    if (!pos) return NULL;

    while (*pos && (*pos == ':' || *pos == ' ' || *pos == '\t')) pos++;
    if (*pos != '"') return NULL;

    pos++;  // Skip opening quote
    char* end = strchr(pos, '"');
    if (!end) return NULL;

    size_t len = end - pos;
    char* result = malloc(len + 1);
    if (result) {
        strncpy(result, pos, len);
        result[len] = '\0';
    }
    return result;
}

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_DEFINITIONS 10
#define MAX_EXAMPLES 5
#define MAX_CONJUGATIONS 48
#define MAX_IPA_LENGTH 256

// ============================================================================
// TYPES
// ============================================================================

typedef enum {
    LANG_ITALIAN,
    LANG_ENGLISH,
    LANG_SPANISH,
    LANG_FRENCH,
    LANG_GERMAN,
    LANG_LATIN
} Language;

typedef enum {
    POS_NOUN,
    POS_VERB,
    POS_ADJECTIVE,
    POS_ADVERB,
    POS_PRONOUN,
    POS_PREPOSITION,
    POS_CONJUNCTION,
    POS_ARTICLE
} PartOfSpeech;

typedef struct {
    char* word;
    Language language;
    PartOfSpeech part_of_speech;
    char* definitions[MAX_DEFINITIONS];
    int definition_count;
    char* examples[MAX_EXAMPLES];
    int example_count;
    char* etymology;
    char* synonyms;
    char* antonyms;
    char* ipa_pronunciation;
} DictionaryEntry;

typedef struct {
    char* tense;
    char* person;
    char* form;
} VerbConjugation;

typedef struct {
    char* verb;
    Language language;
    VerbConjugation conjugations[MAX_CONJUGATIONS];
    int conjugation_count;
    bool is_irregular;
    char* infinitive;
} VerbTable;

typedef struct {
    char* sentence;
    Language language;
    char* parsed_structure;
    char* subject;
    char* predicate;
    char* objects;
    char* modifiers;
    char* clause_type;
    int word_count;
    char** word_analysis;
} GrammarAnalysis;

// ============================================================================
// ACCESSIBILITY SETTINGS
// ============================================================================

typedef struct {
    bool use_tts;
    float tts_speed;
    bool simplified_definitions;
    bool show_etymology;
    bool show_examples;
    bool highlight_syllables;
    bool color_coded_grammar;
} LinguisticAccessibility;

static LinguisticAccessibility get_linguistic_accessibility(const EducationAccessibility* a) {
    LinguisticAccessibility la = {
        .use_tts = false,
        .tts_speed = 1.0f,
        .simplified_definitions = false,
        .show_etymology = true,
        .show_examples = true,
        .highlight_syllables = false,
        .color_coded_grammar = true
    };

    if (!a) return la;

    if (a->dyslexia) {
        la.use_tts = a->tts_enabled;
        la.tts_speed = a->tts_speed > 0 ? a->tts_speed : 0.9f;
        la.simplified_definitions = true;
        la.highlight_syllables = true;
    }

    if (a->autism) {
        la.simplified_definitions = true;
        la.show_etymology = false;  // Reduce information overload
    }

    if (a->adhd) {
        la.show_examples = true;  // Concrete examples help focus
    }

    if (a->high_contrast) {
        la.color_coded_grammar = true;
    }

    return la;
}

// ============================================================================
// DICTIONARY LOOKUP
// ============================================================================

/**
 * Look up word in dictionary (supports multiple languages)
 */
DictionaryEntry* dictionary_lookup(const char* word, Language language,
                                    const EducationAccessibility* access) {
    if (!word) return NULL;

    DictionaryEntry* entry = calloc(1, sizeof(DictionaryEntry));
    if (!entry) return NULL;

    entry->word = strdup(word);
    entry->language = language;

    // Get accessibility settings
    LinguisticAccessibility la = get_linguistic_accessibility(access);

    // Language names for prompt
    const char* lang_names[] = {"Italian", "English", "Spanish", "French", "German", "Latin"};
    const char* lang_name = lang_names[language];

    // Build dictionary prompt
    char prompt[1024];
    snprintf(prompt, sizeof(prompt),
        "Define the %s word \"%s\". Respond in JSON format:\n"
        "{\n"
        "  \"part_of_speech\": \"noun|verb|adjective|adverb|pronoun|preposition|conjunction|article\",\n"
        "  \"definition\": \"main definition\",\n"
        "  \"example\": \"example sentence\",\n"
        "  \"ipa\": \"/phonetic transcription/\",\n"
        "  \"etymology\": \"word origin\",\n"
        "  \"synonyms\": \"word1, word2\"\n"
        "}%s",
        lang_name, word,
        la.simplified_definitions ? "\nUse simple, clear language suitable for students." : "");

    // Call LLM for dictionary lookup
    TokenUsage usage = {0};
    char* response = llm_chat(
        "You are a linguistic expert. Provide accurate dictionary definitions in JSON format only.",
        prompt,
        &usage
    );

    if (response) {
        // Parse response
        char* pos = extract_json_string(response, "part_of_speech");
        if (pos) {
            if (strstr(pos, "verb")) entry->part_of_speech = POS_VERB;
            else if (strstr(pos, "adjective")) entry->part_of_speech = POS_ADJECTIVE;
            else if (strstr(pos, "adverb")) entry->part_of_speech = POS_ADVERB;
            else if (strstr(pos, "pronoun")) entry->part_of_speech = POS_PRONOUN;
            else if (strstr(pos, "preposition")) entry->part_of_speech = POS_PREPOSITION;
            else if (strstr(pos, "conjunction")) entry->part_of_speech = POS_CONJUNCTION;
            else if (strstr(pos, "article")) entry->part_of_speech = POS_ARTICLE;
            else entry->part_of_speech = POS_NOUN;
            free(pos);
        }

        char* def = extract_json_string(response, "definition");
        if (def) {
            entry->definitions[0] = def;
            entry->definition_count = 1;
        }

        char* example = extract_json_string(response, "example");
        if (example) {
            entry->examples[0] = example;
            entry->example_count = 1;
        }

        entry->ipa_pronunciation = extract_json_string(response, "ipa");
        entry->etymology = extract_json_string(response, "etymology");
        entry->synonyms = extract_json_string(response, "synonyms");

        free(response);
    }

    return entry;
}

/**
 * Display dictionary entry with accessibility adaptations
 */
void dictionary_display(const DictionaryEntry* entry,
                        const EducationAccessibility* access) {
    if (!entry) return;

    LinguisticAccessibility la = get_linguistic_accessibility(access);

    // Colors for terminal output
    const char* BOLD = "\033[1m";
    const char* RESET = "\033[0m";
    const char* BLUE = "\033[34m";
    const char* GREEN = "\033[32m";
    const char* YELLOW = "\033[33m";

    printf("\n%s%s%s\n", BOLD, entry->word, RESET);

    if (entry->ipa_pronunciation) {
        printf("%sPronunciation:%s %s\n", BLUE, RESET, entry->ipa_pronunciation);
    }

    // Part of speech
    const char* pos_str[] = {
        "noun", "verb", "adjective", "adverb",
        "pronoun", "preposition", "conjunction", "article"
    };
    printf("%s(%s)%s\n\n", GREEN, pos_str[entry->part_of_speech], RESET);

    // Definitions
    for (int i = 0; i < entry->definition_count; i++) {
        printf("  %d. %s\n", i + 1, entry->definitions[i]);
    }

    // Examples
    if (la.show_examples && entry->example_count > 0) {
        printf("\n%sExamples:%s\n", YELLOW, RESET);
        for (int i = 0; i < entry->example_count; i++) {
            printf("  - %s\n", entry->examples[i]);
        }
    }

    // Etymology
    if (la.show_etymology && entry->etymology) {
        printf("\n%sEtymology:%s %s\n", YELLOW, RESET, entry->etymology);
    }

    // Synonyms and antonyms
    if (entry->synonyms) {
        printf("\n%sSynonyms:%s %s\n", GREEN, RESET, entry->synonyms);
    }
    if (entry->antonyms) {
        printf("%sAntonyms:%s %s\n", GREEN, RESET, entry->antonyms);
    }

    printf("\n");
}

/**
 * Free dictionary entry
 */
void dictionary_free(DictionaryEntry* entry) {
    if (!entry) return;
    free(entry->word);
    for (int i = 0; i < entry->definition_count; i++) {
        free(entry->definitions[i]);
    }
    for (int i = 0; i < entry->example_count; i++) {
        free(entry->examples[i]);
    }
    free(entry->etymology);
    free(entry->synonyms);
    free(entry->antonyms);
    free(entry->ipa_pronunciation);
    free(entry);
}

// ============================================================================
// GRAMMAR ANALYSIS
// ============================================================================

/**
 * Analyze sentence grammar structure
 */
GrammarAnalysis* grammar_analyze(const char* sentence, Language language,
                                  const EducationAccessibility* access) {
    if (!sentence) return NULL;

    GrammarAnalysis* analysis = calloc(1, sizeof(GrammarAnalysis));
    if (!analysis) return NULL;

    analysis->sentence = strdup(sentence);
    analysis->language = language;

    // Count words
    const char* p = sentence;
    int count = 0;
    bool in_word = false;

    while (*p) {
        if (isspace(*p)) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            count++;
        }
        p++;
    }
    analysis->word_count = count;

    // Language names for prompt
    const char* lang_names[] = {"Italian", "English", "Spanish", "French", "German", "Latin"};
    const char* lang_name = lang_names[language];

    // Build grammar analysis prompt
    char prompt[2048];
    snprintf(prompt, sizeof(prompt),
        "Analyze the grammatical structure of this %s sentence:\n\"%s\"\n\n"
        "Respond in JSON format:\n"
        "{\n"
        "  \"structure\": \"grammatical pattern e.g. Subject + Verb + Object\",\n"
        "  \"subject\": \"the subject of the sentence\",\n"
        "  \"predicate\": \"the verb phrase\",\n"
        "  \"objects\": \"direct/indirect objects if any\",\n"
        "  \"modifiers\": \"adjectives, adverbs, phrases\",\n"
        "  \"clause_type\": \"Declarative|Interrogative|Imperative|Exclamatory\"\n"
        "}",
        lang_name, sentence);

    // Call LLM for grammar analysis
    TokenUsage usage = {0};
    char* response = llm_chat(
        "You are a grammar expert. Analyze sentences and provide detailed grammatical breakdowns in JSON format.",
        prompt,
        &usage
    );

    if (response) {
        char* structure = extract_json_string(response, "structure");
        analysis->parsed_structure = structure ? structure : strdup("Subject + Verb + Object");

        char* subject = extract_json_string(response, "subject");
        analysis->subject = subject ? subject : strdup("(Subject)");

        char* predicate = extract_json_string(response, "predicate");
        analysis->predicate = predicate ? predicate : strdup("(Verb phrase)");

        char* objects = extract_json_string(response, "objects");
        analysis->objects = objects ? objects : strdup("(none)");

        analysis->modifiers = extract_json_string(response, "modifiers");

        char* clause = extract_json_string(response, "clause_type");
        analysis->clause_type = clause ? clause : strdup("Declarative");

        free(response);
    } else {
        // Fallback if LLM unavailable
        analysis->parsed_structure = strdup("Subject + Verb + Object");
        analysis->subject = strdup("(Analysis unavailable)");
        analysis->predicate = strdup("(Analysis unavailable)");
        analysis->objects = strdup("(Analysis unavailable)");
        analysis->clause_type = strdup("Unknown");
    }

    return analysis;
}

/**
 * Display grammar analysis with color coding
 */
void grammar_display(const GrammarAnalysis* analysis,
                     const EducationAccessibility* access) {
    if (!analysis) return;

    LinguisticAccessibility la = get_linguistic_accessibility(access);

    const char* BOLD = "\033[1m";
    const char* RESET = "\033[0m";
    const char* BLUE = "\033[34m";    // Subject
    const char* GREEN = "\033[32m";   // Verb
    const char* YELLOW = "\033[33m";  // Object
    const char* CYAN = "\033[36m";    // Modifier

    printf("\n%sGrammar Analysis%s\n", BOLD, RESET);
    printf("Sentence: %s\n\n", analysis->sentence);

    printf("Word count: %d\n", analysis->word_count);
    printf("Structure: %s\n", analysis->parsed_structure);
    printf("Clause type: %s\n\n", analysis->clause_type);

    if (la.color_coded_grammar) {
        printf("%sSubject:%s %s\n", BLUE, RESET, analysis->subject);
        printf("%sPredicate:%s %s\n", GREEN, RESET, analysis->predicate);
        printf("%sObject:%s %s\n", YELLOW, RESET, analysis->objects);
        if (analysis->modifiers) {
            printf("%sModifiers:%s %s\n", CYAN, RESET, analysis->modifiers);
        }
    } else {
        printf("Subject: %s\n", analysis->subject);
        printf("Predicate: %s\n", analysis->predicate);
        printf("Object: %s\n", analysis->objects);
        if (analysis->modifiers) {
            printf("Modifiers: %s\n", analysis->modifiers);
        }
    }

    printf("\n");
}

/**
 * Free grammar analysis
 */
void grammar_free(GrammarAnalysis* analysis) {
    if (!analysis) return;
    free(analysis->sentence);
    free(analysis->parsed_structure);
    free(analysis->subject);
    free(analysis->predicate);
    free(analysis->objects);
    free(analysis->modifiers);
    free(analysis->clause_type);
    if (analysis->word_analysis) {
        for (int i = 0; i < analysis->word_count; i++) {
            free(analysis->word_analysis[i]);
        }
        free(analysis->word_analysis);
    }
    free(analysis);
}

// ============================================================================
// VERB CONJUGATION
// ============================================================================

/**
 * Generate conjugation table for a verb
 */
VerbTable* verb_conjugate(const char* verb, Language language,
                           const EducationAccessibility* access) {
    if (!verb) return NULL;

    VerbTable* table = calloc(1, sizeof(VerbTable));
    if (!table) return NULL;

    table->verb = strdup(verb);
    table->language = language;
    table->infinitive = strdup(verb);
    table->conjugation_count = 0;

    // Language names for prompt
    const char* lang_names[] = {"Italian", "English", "Spanish", "French", "German", "Latin"};
    const char* lang_name = lang_names[language];

    // Build conjugation prompt
    char prompt[1024];
    snprintf(prompt, sizeof(prompt),
        "Conjugate the %s verb \"%s\" in present, past, and future tenses.\n"
        "For each form, provide: tense, person (io/tu/lui/noi/voi/loro for Italian, I/you/he/we/they for English), form.\n"
        "Indicate if the verb is irregular.\n"
        "Respond in JSON format:\n"
        "{\n"
        "  \"irregular\": true/false,\n"
        "  \"conjugations\": [\n"
        "    {\"tense\": \"Present\", \"person\": \"io\", \"form\": \"parlo\"},\n"
        "    ...\n"
        "  ]\n"
        "}",
        lang_name, verb);

    // Call LLM for conjugation
    TokenUsage usage = {0};
    char* response = llm_chat(
        "You are a linguistics expert specializing in verb conjugation. Provide accurate conjugations in JSON format.",
        prompt,
        &usage
    );

    if (response) {
        // Check if irregular
        if (strstr(response, "\"irregular\": true") || strstr(response, "\"irregular\":true")) {
            table->is_irregular = true;
        }

        // Parse conjugations array
        char* ptr = strstr(response, "\"conjugations\"");
        if (ptr) {
            ptr = strchr(ptr, '[');
            if (ptr) {
                // Parse each conjugation entry
                while ((ptr = strstr(ptr, "{")) != NULL && table->conjugation_count < MAX_CONJUGATIONS) {
                    char* tense = extract_json_string(ptr, "tense");
                    char* person = extract_json_string(ptr, "person");
                    char* form = extract_json_string(ptr, "form");

                    if (tense && person && form) {
                        table->conjugations[table->conjugation_count].tense = tense;
                        table->conjugations[table->conjugation_count].person = person;
                        table->conjugations[table->conjugation_count].form = form;
                        table->conjugation_count++;
                    } else {
                        free(tense);
                        free(person);
                        free(form);
                    }

                    ptr = strchr(ptr, '}');
                    if (!ptr) break;
                    ptr++;
                }
            }
        }

        free(response);
    }

    // Fallback if no conjugations parsed
    if (table->conjugation_count == 0) {
        table->conjugations[0].tense = strdup("Present");
        table->conjugations[0].person = strdup("(all)");
        table->conjugations[0].form = strdup(verb);
        table->conjugation_count = 1;
    }

    return table;
}

/**
 * Display verb conjugation table
 */
void verb_display(const VerbTable* table,
                  const EducationAccessibility* access) {
    if (!table) return;

    const char* BOLD = "\033[1m";
    const char* RESET = "\033[0m";
    const char* GREEN = "\033[32m";

    printf("\n%sVerb Conjugation: %s%s\n", BOLD, table->verb, RESET);
    if (table->is_irregular) {
        printf("%s(Irregular verb)%s\n", GREEN, RESET);
    }
    printf("\n");

    // Group by tense
    const char* current_tense = NULL;
    for (int i = 0; i < table->conjugation_count; i++) {
        const VerbConjugation* conj = &table->conjugations[i];

        if (!current_tense || strcmp(current_tense, conj->tense) != 0) {
            current_tense = conj->tense;
            printf("%s%s:%s\n", BOLD, current_tense, RESET);
        }

        printf("  %-10s %s\n", conj->person, conj->form);
    }

    printf("\n");
}

/**
 * Free verb table
 */
void verb_free(VerbTable* table) {
    if (!table) return;
    free(table->verb);
    free(table->infinitive);
    for (int i = 0; i < table->conjugation_count; i++) {
        free(table->conjugations[i].tense);
        free(table->conjugations[i].person);
        free(table->conjugations[i].form);
    }
    free(table);
}

// ============================================================================
// PRONUNCIATION (IPA)
// ============================================================================

/**
 * Get IPA pronunciation for a word
 */
char* pronunciation_ipa(const char* word, Language language,
                        const EducationAccessibility* access) {
    if (!word) return NULL;

    // Language names for prompt
    const char* lang_names[] = {"Italian", "English", "Spanish", "French", "German", "Latin"};
    const char* lang_name = lang_names[language];

    // Build IPA prompt
    char prompt[512];
    snprintf(prompt, sizeof(prompt),
        "Provide the IPA (International Phonetic Alphabet) transcription for the %s word \"%s\".\n"
        "Respond with ONLY the IPA transcription in slashes, like: /həˈloʊ/\n"
        "Do not include any other text.",
        lang_name, word);

    // Call LLM for IPA
    TokenUsage usage = {0};
    char* response = llm_chat(
        "You are a phonetics expert. Provide accurate IPA transcriptions.",
        prompt,
        &usage
    );

    char* result = NULL;

    if (response) {
        // Find IPA between slashes
        char* start = strchr(response, '/');
        if (start) {
            char* end = strchr(start + 1, '/');
            if (end) {
                size_t len = end - start + 1;
                result = malloc(len + 1);
                if (result) {
                    strncpy(result, start, len);
                    result[len] = '\0';
                }
            }
        }

        // If no slashes found, use whole response trimmed
        if (!result && strlen(response) > 0 && strlen(response) < MAX_IPA_LENGTH) {
            result = strdup(response);
        }

        free(response);
    }

    // Fallback if LLM unavailable
    if (!result) {
        result = malloc(MAX_IPA_LENGTH);
        if (result) {
            snprintf(result, MAX_IPA_LENGTH, "/%s/", word);
        }
    }

    return result;
}

/**
 * Display pronunciation with syllable breakdown
 */
void pronunciation_display(const char* word, const char* ipa,
                           const EducationAccessibility* access) {
    if (!word || !ipa) return;

    LinguisticAccessibility la = get_linguistic_accessibility(access);

    const char* BOLD = "\033[1m";
    const char* RESET = "\033[0m";
    const char* BLUE = "\033[34m";
    const char* YELLOW = "\033[33m";

    printf("\n%sPronunciation%s\n", BOLD, RESET);
    printf("Word: %s%s%s\n", BLUE, word, RESET);
    printf("IPA:  %s\n", ipa);

    if (la.highlight_syllables) {
        // Break into syllables (simplified)
        printf("\n%sSyllables:%s ", YELLOW, RESET);

        // Simple syllable detection (would be more sophisticated in production)
        // For now, just split on common patterns
        const char* p = word;
        bool vowel_seen = false;

        while (*p) {
            if (strchr("aeiouAEIOU", *p)) {
                if (!vowel_seen) {
                    printf("%s", BLUE);
                    vowel_seen = true;
                }
            } else if (vowel_seen && strchr("bcdfghjklmnpqrstvwxyzBCDFGHJKLMNPQRSTVWXYZ", *p)) {
                printf("%s", RESET);
                printf("-");
                vowel_seen = false;
            }
            printf("%c", *p);
            p++;
        }
        printf("%s\n", RESET);
    }

    printf("\n");
}

// ============================================================================
// CLI COMMAND HANDLERS
// ============================================================================

/**
 * Handle /define command
 */
int linguistic_define_handler(int argc, char** argv,
                               const EducationStudentProfile* profile) {
    if (argc < 2) {
        printf("Usage: /define <word> [--lang en|it|es|fr|de|la]\n");
        return 1;
    }

    const char* word = argv[1];
    Language lang = LANG_ENGLISH;

    // Parse language option
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc) {
            const char* lang_str = argv[++i];
            if (strcmp(lang_str, "it") == 0) lang = LANG_ITALIAN;
            else if (strcmp(lang_str, "es") == 0) lang = LANG_SPANISH;
            else if (strcmp(lang_str, "fr") == 0) lang = LANG_FRENCH;
            else if (strcmp(lang_str, "de") == 0) lang = LANG_GERMAN;
            else if (strcmp(lang_str, "la") == 0) lang = LANG_LATIN;
        }
    }

    const EducationAccessibility* access = profile ? profile->accessibility : NULL;

    DictionaryEntry* entry = dictionary_lookup(word, lang, access);
    if (entry) {
        dictionary_display(entry, access);
        dictionary_free(entry);
        return 0;
    } else {
        fprintf(stderr, "Word not found: %s\n", word);
        return 1;
    }
}

/**
 * Handle /conjugate command
 */
int linguistic_conjugate_handler(int argc, char** argv,
                                  const EducationStudentProfile* profile) {
    if (argc < 2) {
        printf("Usage: /conjugate <verb> [--lang en|it|es|fr|de|la]\n");
        return 1;
    }

    const char* verb = argv[1];
    Language lang = LANG_ITALIAN;  // Default to Italian

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc) {
            const char* lang_str = argv[++i];
            if (strcmp(lang_str, "en") == 0) lang = LANG_ENGLISH;
            else if (strcmp(lang_str, "es") == 0) lang = LANG_SPANISH;
            else if (strcmp(lang_str, "fr") == 0) lang = LANG_FRENCH;
            else if (strcmp(lang_str, "de") == 0) lang = LANG_GERMAN;
            else if (strcmp(lang_str, "la") == 0) lang = LANG_LATIN;
        }
    }

    const EducationAccessibility* access = profile ? profile->accessibility : NULL;

    VerbTable* table = verb_conjugate(verb, lang, access);
    if (table) {
        verb_display(table, access);
        verb_free(table);
        return 0;
    } else {
        fprintf(stderr, "Could not conjugate verb: %s\n", verb);
        return 1;
    }
}

/**
 * Handle /pronounce command
 */
int linguistic_pronounce_handler(int argc, char** argv,
                                  const EducationStudentProfile* profile) {
    if (argc < 2) {
        printf("Usage: /pronounce <word> [--lang en|it|es|fr|de]\n");
        return 1;
    }

    const char* word = argv[1];
    Language lang = LANG_ENGLISH;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc) {
            const char* lang_str = argv[++i];
            if (strcmp(lang_str, "it") == 0) lang = LANG_ITALIAN;
            else if (strcmp(lang_str, "es") == 0) lang = LANG_SPANISH;
            else if (strcmp(lang_str, "fr") == 0) lang = LANG_FRENCH;
            else if (strcmp(lang_str, "de") == 0) lang = LANG_GERMAN;
        }
    }

    const EducationAccessibility* access = profile ? profile->accessibility : NULL;

    char* ipa = pronunciation_ipa(word, lang, access);
    if (ipa) {
        pronunciation_display(word, ipa, access);

        // If TTS enabled, speak the word
        if (access && access->tts_enabled) {
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "say '%s'", word);
            system(cmd);
        }

        free(ipa);
        return 0;
    } else {
        fprintf(stderr, "Could not get pronunciation for: %s\n", word);
        return 1;
    }
}

/**
 * Handle /grammar command
 */
int linguistic_grammar_handler(int argc, char** argv,
                                const EducationStudentProfile* profile) {
    if (argc < 2) {
        printf("Usage: /grammar \"<sentence>\" [--lang en|it|es|fr|de]\n");
        return 1;
    }

    const char* sentence = argv[1];
    Language lang = LANG_ENGLISH;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc) {
            const char* lang_str = argv[++i];
            if (strcmp(lang_str, "it") == 0) lang = LANG_ITALIAN;
            else if (strcmp(lang_str, "es") == 0) lang = LANG_SPANISH;
            else if (strcmp(lang_str, "fr") == 0) lang = LANG_FRENCH;
            else if (strcmp(lang_str, "de") == 0) lang = LANG_GERMAN;
        }
    }

    const EducationAccessibility* access = profile ? profile->accessibility : NULL;

    GrammarAnalysis* analysis = grammar_analyze(sentence, lang, access);
    if (analysis) {
        grammar_display(analysis, access);
        grammar_free(analysis);
        return 0;
    } else {
        fprintf(stderr, "Could not analyze sentence\n");
        return 1;
    }
}
