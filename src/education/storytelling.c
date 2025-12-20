/**
 * CONVERGIO EDUCATION - STORYTELLING ENGINE
 *
 * Chris Maestro's storytelling support system.
 * Provides analysis tools for public speaking practice:
 * - Filler word detection
 * - Pacing analysis
 * - 18-minute TED timer
 * - Story hook suggestions
 *
 * Phase: FASE 12 - Storytelling Integration
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

// ============================================================================
// CONSTANTS
// ============================================================================

// Italian filler words
static const char* FILLER_WORDS_IT[] = {
    "ehm", "uhm", "ah", "eh", "oh", "mah",
    "cioe", "praticamente", "fondamentalmente", "insomma",
    "tipo", "diciamo", "ecco", "allora", "quindi",
    "comunque", "niente", "vabbene", "capito", "no?",
    NULL
};

// English filler words
static const char* FILLER_WORDS_EN[] = {
    "um", "uh", "ah", "er", "eh",
    "like", "basically", "actually", "literally", "honestly",
    "you know", "I mean", "sort of", "kind of", "right",
    "so", "well", "anyway", "okay", "yeah",
    NULL
};

// TED talk timing (seconds)
#define TED_SECTION_HOOK     180   // 0-3 min
#define TED_SECTION_MAIN     180   // 3-6 min
#define TED_SECTION_PILLARS  540   // 6-15 min
#define TED_SECTION_ACTION   180   // 15-18 min
#define TED_TOTAL_TIME       1080  // 18 min

// Pacing thresholds (words per minute)
#define PACING_TOO_SLOW     100
#define PACING_IDEAL_MIN    120
#define PACING_IDEAL_MAX    150
#define PACING_TOO_FAST     180

// ============================================================================
// TYPES
// ============================================================================

typedef struct {
    char* word;
    int count;
} FillerCount;

typedef struct {
    FillerCount* fillers;
    int filler_count;
    int total_filler_words;
    int total_words;
    float filler_percentage;
} FillerAnalysis;

typedef struct {
    int words_per_minute;
    const char* rating;     // "too_slow", "ideal", "too_fast"
    const char* suggestion;
    int total_words;
    int duration_seconds;
} PacingAnalysis;

typedef struct {
    int elapsed_seconds;
    int remaining_seconds;
    const char* current_section;
    int section_remaining;
    float progress_percent;
    bool overtime;
} TEDTimerState;

// ============================================================================
// STORY HOOKS PER MAESTRO
// ============================================================================

typedef struct {
    const char* maestro_id;
    const char** hooks;
} MaestroHooks;

static const char* SOCRATE_HOOKS[] = {
    "Immagina di essere ad Atene, 2400 anni fa, in una piazza affollata...",
    "Un giovane un giorno chiese: 'Maestro, cos'e la verita?'",
    "Sai perche Socrate fu condannato a morte?",
    NULL
};

static const char* EUCLIDE_HOOKS[] = {
    "Un re chiese una volta: 'Esiste una via facile per la geometria?'",
    "Pensa a un mondo senza numeri. Impossibile, vero?",
    "Tutto inizio con un punto, una linea, e una domanda...",
    NULL
};

static const char* FEYNMAN_HOOKS[] = {
    "Sai cosa succede quando lasci cadere una palla?",
    "Un giorno in un ristorante, mi chiesi: perche l'acqua gira cosi nel lavandino?",
    "Il segreto dell'universo e nascosto in una tazza di caffe...",
    NULL
};

static const char* ERODOTO_HOOKS[] = {
    "In un tempo lontano, un impero dominava il mondo conosciuto...",
    "Questa storia inizia 3000 anni fa, sulle rive del Nilo...",
    "Ti sei mai chiesto perche parliamo di 'vittoria di Pirro'?",
    NULL
};

static const char* DARWIN_HOOKS[] = {
    "Navigando verso isole remote, notai qualcosa di strano...",
    "Immagina di essere un fringuello su un'isola deserta...",
    "Cosa hanno in comune il tuo naso e la proboscide dell'elefante?",
    NULL
};

static const char* LEONARDO_HOOKS[] = {
    "Nel mio studio, circondato da macchine impossibili, vidi il futuro...",
    "Un uccello mi insegno a volare. In teoria.",
    "Prima di Instagram, c'era il mio taccuino...",
    NULL
};

static const char* MANZONI_HOOKS[] = {
    "Era una notte buia e tempestosa... no, aspetta, era un lago calmo.",
    "Quel ramo del lago di Como... ti dice qualcosa?",
    "Una promessa non mantenuta cambio la storia d'Italia.",
    NULL
};

static const char* SHAKESPEARE_HOOKS[] = {
    "To be or not to be - ma cosa significa davvero?",
    "Un principe, un fantasma, e una vendetta...",
    "Tutti il mondo e un palcoscenico. Ma tu, che ruolo interpreti?",
    NULL
};

static const char* MOZART_HOOKS[] = {
    "Avevo 5 anni quando scrissi la mia prima sinfonia...",
    "La musica non e nelle note, ma nel silenzio tra di esse.",
    "Senti questa melodia? L'ho sognata stanotte.",
    NULL
};

static const char* CICERONE_HOOKS[] = {
    "Cittadini di Roma! Fino a quando abuserai della nostra pazienza?",
    "Tre parole possono cambiare il mondo: Veni, Vidi, Vici.",
    "L'arte della parola e l'arma piu potente.",
    NULL
};

static const char* SMITH_HOOKS[] = {
    "Perche un fabbricante di spilli e il segreto della ricchezza?",
    "La mano invisibile - no, non e magia, e economia.",
    "Cosa succederebbe se tutti pensassero solo a se stessi?",
    NULL
};

static const char* LOVELACE_HOOKS[] = {
    "Mio padre era un poeta. Io preferivo le macchine.",
    "Immagina un telaio che pensa...",
    "Prima che esistesse la parola 'computer', io lo sognavo.",
    NULL
};

static const char* IPPOCRATE_HOOKS[] = {
    "Primo, non nuocere. Ma cosa significa curare?",
    "Il corpo umano e un tempio - sai come prendertene cura?",
    "2500 anni fa, la medicina era magia. Poi cambio tutto.",
    NULL
};

static const MaestroHooks MAESTRO_HOOKS[] = {
    {"socrate-filosofia", SOCRATE_HOOKS},
    {"euclide-matematica", EUCLIDE_HOOKS},
    {"feynman-fisica", FEYNMAN_HOOKS},
    {"erodoto-storia", ERODOTO_HOOKS},
    {"darwin-scienze", DARWIN_HOOKS},
    {"leonardo-arte", LEONARDO_HOOKS},
    {"manzoni-italiano", MANZONI_HOOKS},
    {"shakespeare-inglese", SHAKESPEARE_HOOKS},
    {"mozart-musica", MOZART_HOOKS},
    {"cicerone-latino", CICERONE_HOOKS},
    {"smith-economia", SMITH_HOOKS},
    {"lovelace-informatica", LOVELACE_HOOKS},
    {"ippocrate-salute", IPPOCRATE_HOOKS},
    {NULL, NULL}
};

// ============================================================================
// FILLER WORD ANALYSIS
// ============================================================================

/**
 * Count words in text
 */
static int count_words(const char* text) {
    if (!text || !*text) return 0;

    int count = 0;
    bool in_word = false;

    while (*text) {
        if (isspace((unsigned char)*text)) {
            if (in_word) {
                count++;
                in_word = false;
            }
        } else {
            in_word = true;
        }
        text++;
    }

    if (in_word) count++;
    return count;
}

/**
 * Convert string to lowercase
 */
static char* to_lower(const char* str) {
    if (!str) return NULL;

    size_t len = strlen(str);
    char* lower = malloc(len + 1);
    if (!lower) return NULL;

    for (size_t i = 0; i <= len; i++) {
        lower[i] = tolower((unsigned char)str[i]);
    }
    return lower;
}

/**
 * Count occurrences of a word in text
 */
static int count_word_occurrences(const char* text, const char* word) {
    if (!text || !word) return 0;

    char* lower_text = to_lower(text);
    char* lower_word = to_lower(word);
    if (!lower_text || !lower_word) {
        free(lower_text);
        free(lower_word);
        return 0;
    }

    int count = 0;
    size_t word_len = strlen(lower_word);
    const char* p = lower_text;

    while ((p = strstr(p, lower_word)) != NULL) {
        // Check word boundaries
        bool start_ok = (p == lower_text || !isalpha((unsigned char)*(p - 1)));
        bool end_ok = !isalpha((unsigned char)*(p + word_len));

        if (start_ok && end_ok) {
            count++;
        }
        p++;
    }

    free(lower_text);
    free(lower_word);
    return count;
}

/**
 * Analyze filler words in speech transcript
 */
FillerAnalysis* storytelling_analyze_fillers(const char* text, const char* language) {
    if (!text) return NULL;

    FillerAnalysis* analysis = calloc(1, sizeof(FillerAnalysis));
    if (!analysis) return NULL;

    const char** filler_list = (language && strcmp(language, "en") == 0)
        ? FILLER_WORDS_EN : FILLER_WORDS_IT;

    // Count total words
    analysis->total_words = count_words(text);

    // Count each filler word
    int max_fillers = 30;
    analysis->fillers = calloc(max_fillers, sizeof(FillerCount));
    if (!analysis->fillers) {
        free(analysis);
        return NULL;
    }

    for (const char** filler = filler_list; *filler != NULL; filler++) {
        int count = count_word_occurrences(text, *filler);
        if (count > 0 && analysis->filler_count < max_fillers) {
            analysis->fillers[analysis->filler_count].word = strdup(*filler);
            analysis->fillers[analysis->filler_count].count = count;
            analysis->total_filler_words += count;
            analysis->filler_count++;
        }
    }

    // Calculate percentage
    analysis->filler_percentage = analysis->total_words > 0
        ? (float)analysis->total_filler_words * 100.0f / (float)analysis->total_words
        : 0.0f;

    return analysis;
}

/**
 * Free filler analysis
 */
void storytelling_free_filler_analysis(FillerAnalysis* analysis) {
    if (!analysis) return;
    if (analysis->fillers) {
        for (int i = 0; i < analysis->filler_count; i++) {
            free(analysis->fillers[i].word);
        }
        free(analysis->fillers);
    }
    free(analysis);
}

/**
 * Get filler analysis feedback
 */
const char* storytelling_filler_feedback(float percentage) {
    if (percentage < 1.0f) {
        return "Eccellente! Quasi nessun riempitivo. Parli in modo chiaro e diretto.";
    } else if (percentage < 3.0f) {
        return "Buono! Pochi riempitivi. Con un po' di pratica sarai perfetto.";
    } else if (percentage < 5.0f) {
        return "Discreto. Prova a fare pause silenziose invece di usare 'ehm' e 'cioe'.";
    } else if (percentage < 10.0f) {
        return "Da migliorare. I riempitivi distraggono. Prova a rallentare e respirare.";
    } else {
        return "Attenzione! Troppi riempitivi. Esercitati a fare pause silenziose.";
    }
}

// ============================================================================
// PACING ANALYSIS
// ============================================================================

/**
 * Analyze speaking pace
 */
PacingAnalysis* storytelling_analyze_pacing(const char* text, int duration_seconds) {
    if (!text || duration_seconds <= 0) return NULL;

    PacingAnalysis* analysis = calloc(1, sizeof(PacingAnalysis));
    if (!analysis) return NULL;

    analysis->total_words = count_words(text);
    analysis->duration_seconds = duration_seconds;

    // Calculate WPM
    float minutes = (float)duration_seconds / 60.0f;
    analysis->words_per_minute = (int)((float)analysis->total_words / minutes);

    // Determine rating
    if (analysis->words_per_minute < PACING_TOO_SLOW) {
        analysis->rating = "too_slow";
        analysis->suggestion = "Stai parlando troppo lentamente. Il pubblico potrebbe perdere interesse. "
                               "Prova ad aumentare leggermente il ritmo.";
    } else if (analysis->words_per_minute < PACING_IDEAL_MIN) {
        analysis->rating = "slow";
        analysis->suggestion = "Ritmo leggermente lento. Va bene per concetti complessi, "
                               "ma prova a variare il tempo.";
    } else if (analysis->words_per_minute <= PACING_IDEAL_MAX) {
        analysis->rating = "ideal";
        analysis->suggestion = "Ritmo perfetto! Mantieni questa velocita e ricorda di fare pause "
                               "per enfatizzare i punti importanti.";
    } else if (analysis->words_per_minute < PACING_TOO_FAST) {
        analysis->rating = "fast";
        analysis->suggestion = "Stai parlando un po' veloce. Rallenta leggermente e "
                               "lascia respirare le tue parole.";
    } else {
        analysis->rating = "too_fast";
        analysis->suggestion = "Troppo veloce! Il pubblico non riesce a seguirti. "
                               "Respira, rallenta, fai pause.";
    }

    return analysis;
}

/**
 * Free pacing analysis
 */
void storytelling_free_pacing_analysis(PacingAnalysis* analysis) {
    free(analysis);
}

// ============================================================================
// TED TIMER
// ============================================================================

static time_t g_ted_start_time = 0;

/**
 * Start TED talk timer
 */
void storytelling_ted_timer_start(void) {
    g_ted_start_time = time(NULL);
}

/**
 * Get TED timer state
 */
TEDTimerState storytelling_ted_timer_state(void) {
    TEDTimerState state = {0};

    if (g_ted_start_time == 0) {
        state.current_section = "not_started";
        state.remaining_seconds = TED_TOTAL_TIME;
        return state;
    }

    time_t now = time(NULL);
    state.elapsed_seconds = (int)(now - g_ted_start_time);
    state.remaining_seconds = TED_TOTAL_TIME - state.elapsed_seconds;
    state.progress_percent = (float)state.elapsed_seconds * 100.0f / (float)TED_TOTAL_TIME;

    if (state.elapsed_seconds > TED_TOTAL_TIME) {
        state.overtime = true;
        state.current_section = "overtime";
        state.section_remaining = 0;
    } else if (state.elapsed_seconds < TED_SECTION_HOOK) {
        state.current_section = "hook";
        state.section_remaining = TED_SECTION_HOOK - state.elapsed_seconds;
    } else if (state.elapsed_seconds < TED_SECTION_HOOK + TED_SECTION_MAIN) {
        state.current_section = "main_idea";
        state.section_remaining = TED_SECTION_HOOK + TED_SECTION_MAIN - state.elapsed_seconds;
    } else if (state.elapsed_seconds < TED_SECTION_HOOK + TED_SECTION_MAIN + TED_SECTION_PILLARS) {
        state.current_section = "three_pillars";
        state.section_remaining = TED_SECTION_HOOK + TED_SECTION_MAIN + TED_SECTION_PILLARS - state.elapsed_seconds;
    } else {
        state.current_section = "call_to_action";
        state.section_remaining = TED_TOTAL_TIME - state.elapsed_seconds;
    }

    return state;
}

/**
 * Stop TED timer
 */
void storytelling_ted_timer_stop(void) {
    g_ted_start_time = 0;
}

/**
 * Get section label in Italian
 */
const char* storytelling_section_label(const char* section) {
    if (!section) return "";
    if (strcmp(section, "hook") == 0) return "HOOK - Cattura l'attenzione";
    if (strcmp(section, "main_idea") == 0) return "IDEA PRINCIPALE - Il tuo throughline";
    if (strcmp(section, "three_pillars") == 0) return "TRE PILASTRI - I tuoi punti chiave";
    if (strcmp(section, "call_to_action") == 0) return "CALL TO ACTION - Chiusura memorabile";
    if (strcmp(section, "overtime") == 0) return "TEMPO SCADUTO!";
    if (strcmp(section, "not_started") == 0) return "Pronto per iniziare";
    return section;
}

// ============================================================================
// STORY HOOKS
// ============================================================================

/**
 * Get a random story hook for a maestro
 */
const char* storytelling_get_hook(const char* maestro_id) {
    if (!maestro_id) return NULL;

    for (const MaestroHooks* mh = MAESTRO_HOOKS; mh->maestro_id != NULL; mh++) {
        if (strcmp(mh->maestro_id, maestro_id) == 0) {
            // Count hooks
            int count = 0;
            while (mh->hooks[count] != NULL) count++;

            if (count > 0) {
                // Random selection
                int idx = rand() % count;
                return mh->hooks[idx];
            }
        }
    }
    return NULL;
}

/**
 * Get all story hooks for a maestro
 */
const char** storytelling_get_all_hooks(const char* maestro_id) {
    if (!maestro_id) return NULL;

    for (const MaestroHooks* mh = MAESTRO_HOOKS; mh->maestro_id != NULL; mh++) {
        if (strcmp(mh->maestro_id, maestro_id) == 0) {
            return mh->hooks;
        }
    }
    return NULL;
}
