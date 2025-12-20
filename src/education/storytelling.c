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
 * Phase: Phase 12 - Storytelling Integration
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

static const char* SOCRATES_HOOKS[] = {
    "Imagine being in Athens, 2400 years ago, in a crowded plaza...",
    "A young man once asked: 'Master, what is truth?'",
    "Do you know why Socrates was sentenced to death?",
    NULL
};

static const char* EUCLID_HOOKS[] = {
    "A king once asked: 'Is there an easy path to geometry?'",
    "Think of a world without numbers. Impossible, right?",
    "It all began with a point, a line, and a question...",
    NULL
};

static const char* FEYNMAN_HOOKS[] = {
    "Do you know what happens when you drop a ball?",
    "One day in a restaurant, I wondered: why does water swirl that way in the sink?",
    "The secret of the universe is hidden in a cup of coffee...",
    NULL
};

static const char* HERODOTUS_HOOKS[] = {
    "In a time long ago, an empire ruled the known world...",
    "This story begins 3000 years ago, on the banks of the Nile...",
    "Have you ever wondered why we speak of a 'Pyrrhic victory'?",
    NULL
};

static const char* DARWIN_HOOKS[] = {
    "Sailing to remote islands, I noticed something strange...",
    "Imagine being a finch on a deserted island...",
    "What do your nose and an elephant's trunk have in common?",
    NULL
};

static const char* LEONARDO_HOOKS[] = {
    "In my studio, surrounded by impossible machines, I saw the future...",
    "A bird taught me to fly. In theory.",
    "Before Instagram, there was my sketchbook...",
    NULL
};

static const char* MANZONI_HOOKS[] = {
    "It was a dark and stormy night... no wait, it was a calm lake.",
    "That branch of Lake Como... does it ring a bell?",
    "A broken promise changed the history of Italy.",
    NULL
};

static const char* SHAKESPEARE_HOOKS[] = {
    "To be or not to be - but what does it really mean?",
    "A prince, a ghost, and a revenge...",
    "All the world's a stage. But what role do you play?",
    NULL
};

static const char* MOZART_HOOKS[] = {
    "I was 5 years old when I wrote my first symphony...",
    "Music is not in the notes, but in the silence between them.",
    "Hear this melody? I dreamed it last night.",
    NULL
};

static const char* CICERO_HOOKS[] = {
    "Citizens of Rome! How long will you abuse our patience?",
    "Three words can change the world: Veni, Vidi, Vici.",
    "The art of speech is the most powerful weapon.",
    NULL
};

static const char* SMITH_HOOKS[] = {
    "Why is a pin-maker the secret to wealth?",
    "The invisible hand - no, it's not magic, it's economics.",
    "What would happen if everyone only thought of themselves?",
    NULL
};

static const char* LOVELACE_HOOKS[] = {
    "My father was a poet. I preferred machines.",
    "Imagine a loom that thinks...",
    "Before the word 'computer' existed, I dreamed of it.",
    NULL
};

static const char* HIPPOCRATES_HOOKS[] = {
    "First, do no harm. But what does it mean to heal?",
    "The human body is a temple - do you know how to take care of it?",
    "2500 years ago, medicine was magic. Then everything changed.",
    NULL
};

static const char* HUMBOLDT_HOOKS[] = {
    "I climbed the highest peaks to understand the Earth...",
    "Everything is connected - from a tiny flower to the cosmos.",
    "The Amazon taught me more than any library ever could.",
    NULL
};

static const char* CHRIS_HOOKS[] = {
    "The best talks are not about you - they're about your audience.",
    "Every great story starts with a single question...",
    "What if you could change someone's life in 18 minutes?",
    NULL
};

static const MaestroHooks MAESTRO_HOOKS[] = {
    {"socrate-filosofia", SOCRATES_HOOKS},
    {"euclide-matematica", EUCLID_HOOKS},
    {"feynman-fisica", FEYNMAN_HOOKS},
    {"erodoto-storia", HERODOTUS_HOOKS},
    {"darwin-scienze", DARWIN_HOOKS},
    {"leonardo-arte", LEONARDO_HOOKS},
    {"manzoni-italiano", MANZONI_HOOKS},
    {"shakespeare-inglese", SHAKESPEARE_HOOKS},
    {"mozart-musica", MOZART_HOOKS},
    {"cicerone-civica", CICERO_HOOKS},
    {"smith-economia", SMITH_HOOKS},
    {"lovelace-informatica", LOVELACE_HOOKS},
    {"ippocrate-corpo", HIPPOCRATES_HOOKS},
    {"humboldt-geografia", HUMBOLDT_HOOKS},
    {"chris-storytelling", CHRIS_HOOKS},
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
        return "Excellent! Almost no fillers. You speak clearly and directly.";
    } else if (percentage < 3.0f) {
        return "Good! Few fillers. With a bit of practice you'll be perfect.";
    } else if (percentage < 5.0f) {
        return "Decent. Try making silent pauses instead of using 'um' and 'like'.";
    } else if (percentage < 10.0f) {
        return "Needs improvement. Fillers are distracting. Try to slow down and breathe.";
    } else {
        return "Warning! Too many fillers. Practice making silent pauses.";
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
        analysis->suggestion = "You're speaking too slowly. The audience may lose interest. "
                               "Try to increase your pace slightly.";
    } else if (analysis->words_per_minute < PACING_IDEAL_MIN) {
        analysis->rating = "slow";
        analysis->suggestion = "Slightly slow pace. Good for complex concepts, "
                               "but try to vary your tempo.";
    } else if (analysis->words_per_minute <= PACING_IDEAL_MAX) {
        analysis->rating = "ideal";
        analysis->suggestion = "Perfect pace! Maintain this speed and remember to pause "
                               "to emphasize important points.";
    } else if (analysis->words_per_minute < PACING_TOO_FAST) {
        analysis->rating = "fast";
        analysis->suggestion = "You're speaking a bit fast. Slow down slightly and "
                               "let your words breathe.";
    } else {
        analysis->rating = "too_fast";
        analysis->suggestion = "Too fast! The audience can't follow. "
                               "Breathe, slow down, pause.";
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
 * Get section label
 */
const char* storytelling_section_label(const char* section) {
    if (!section) return "";
    if (strcmp(section, "hook") == 0) return "HOOK - Capture attention";
    if (strcmp(section, "main_idea") == 0) return "MAIN IDEA - Your throughline";
    if (strcmp(section, "three_pillars") == 0) return "THREE PILLARS - Your key points";
    if (strcmp(section, "call_to_action") == 0) return "CALL TO ACTION - Memorable close";
    if (strcmp(section, "overtime") == 0) return "TIME'S UP!";
    if (strcmp(section, "not_started") == 0) return "Ready to start";
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

// ============================================================================
// H.A.I.L. FRAMEWORK
// ============================================================================

static const char* HAIL_DESCRIPTIONS[] = {
    [0] = "Honesty: Be authentic and transparent. Don't pretend to be someone else.",
    [1] = "Authenticity: Be yourself. Your unique voice is your strength.",
    [2] = "Integrity: Be consistent with your values. Say what you think.",
    [3] = "Love: Show genuine interest in your audience. Speak to help them."
};

const char* storytelling_hail_description(int quality) {
    if (quality < 0 || quality > 3) return "";
    return HAIL_DESCRIPTIONS[quality];
}

float storytelling_evaluate_hail(const char* text, float scores[4]) {
    if (!text || !scores) return 0.0f;

    // Initialize scores
    for (int i = 0; i < 4; i++) scores[i] = 0.5f;

    char* lower = to_lower(text);
    if (!lower) return 0.5f;

    int total_words = count_words(text);
    if (total_words == 0) {
        free(lower);
        return 0.5f;
    }

    // H - Honesty: Check for hedging words (negative indicator)
    const char* hedging[] = {"maybe", "perhaps", "I don't know", "I think", "I guess", NULL};
    int hedge_count = 0;
    for (const char** h = hedging; *h; h++) {
        hedge_count += count_word_occurrences(text, *h);
    }
    scores[0] = 1.0f - (float)hedge_count * 0.1f;
    if (scores[0] < 0.2f) scores[0] = 0.2f;

    // A - Authenticity: Check for personal pronouns (positive indicator)
    int personal = count_word_occurrences(text, "I") +
                   count_word_occurrences(text, "my") +
                   count_word_occurrences(text, "me") +
                   count_word_occurrences(text, "myself");
    scores[1] = (float)personal / (float)total_words * 20.0f;
    if (scores[1] > 1.0f) scores[1] = 1.0f;
    if (scores[1] < 0.3f) scores[1] = 0.3f;

    // I - Integrity: Check for decisive language
    int decisive = count_word_occurrences(text, "believe") +
                   count_word_occurrences(text, "I firmly believe") +
                   count_word_occurrences(text, "it's important") +
                   count_word_occurrences(text, "must");
    scores[2] = 0.5f + (float)decisive * 0.1f;
    if (scores[2] > 1.0f) scores[2] = 1.0f;

    // L - Love: Check for audience engagement
    int engagement = count_word_occurrences(text, "you") +
                     count_word_occurrences(text, "your") +
                     count_word_occurrences(text, "together") +
                     count_word_occurrences(text, "we") +
                     count_word_occurrences(text, "us");
    scores[3] = (float)engagement / (float)total_words * 15.0f;
    if (scores[3] > 1.0f) scores[3] = 1.0f;
    if (scores[3] < 0.2f) scores[3] = 0.2f;

    free(lower);

    // Overall score
    return (scores[0] + scores[1] + scores[2] + scores[3]) / 4.0f;
}

// ============================================================================
// C.N.E.P.R. STRUCTURE
// ============================================================================

int storytelling_cnepr_duration(int section) {
    switch (section) {
        case 0: return 120;   // Connection: 2 min
        case 1: return 300;   // Narration: 5 min
        case 2: return 360;   // Explanation: 6 min
        case 3: return 180;   // Persuasion: 3 min
        case 4: return 120;   // Revelation: 2 min
        default: return 0;
    }
}

const char* storytelling_cnepr_name(int section) {
    switch (section) {
        case 0: return "Connection";
        case 1: return "Narration";
        case 2: return "Explanation";
        case 3: return "Persuasion";
        case 4: return "Revelation";
        default: return "";
    }
}

const char* storytelling_cnepr_tips(int section) {
    switch (section) {
        case 0:
            return "Capture attention. Use a provocative question, "
                   "a surprising statistic, or a personal anecdote.";
        case 1:
            return "Tell a personal or relevant story. "
                   "Show vulnerability. Create empathy with the audience.";
        case 2:
            return "Explain the core concept. Use simple analogies. "
                   "Break it into maximum three key points.";
        case 3:
            return "Convince with data, examples, and testimonials. "
                   "Show the problem and the solution.";
        case 4:
            return "Close with a memorable call-to-action. "
                   "Return to the opening theme. Leave a strong mental image.";
        default:
            return "";
    }
}
