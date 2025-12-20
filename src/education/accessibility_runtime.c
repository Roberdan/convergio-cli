/**
 * CONVERGIO EDUCATION - ACCESSIBILITY RUNTIME ADAPTATIONS
 *
 * Central module for runtime accessibility adaptations based on student profile.
 * Implements FASE 6 A11y requirements:
 * - Dislessia (DY01-07)
 * - Discalculia (DC01-06)
 * - Paralisi Cerebrale (CP01-05)
 * - ADHD (AD01-06)
 * - Autismo (AU01-06)
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#include "nous/education.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// ============================================================================
// CONSTANTS
// ============================================================================

// DY01: OpenDyslexic font name
#define FONT_OPENDYSLEXIC "OpenDyslexic"
#define FONT_DEFAULT "SF Pro"

// DY02: Line spacing multiplier
#define DYSLEXIA_LINE_SPACING 1.5f
#define DEFAULT_LINE_SPACING 1.0f

// DY03: Maximum characters per line for dyslexia
#define DYSLEXIA_MAX_CHARS_PER_LINE 60
#define DEFAULT_MAX_CHARS_PER_LINE 120

// DY04: Background colors
#define BG_CREAM "#FFF8DC"
#define BG_DEFAULT "#FFFFFF"

// AD01: Maximum bullet points for ADHD responses
#define ADHD_MAX_BULLETS 4
#define DEFAULT_MAX_BULLETS 10

// AD02: Progress bar width
#define PROGRESS_BAR_WIDTH 40

// CP03: Extended timeouts (seconds)
#define CP_TIMEOUT_MULTIPLIER 3
#define DEFAULT_TIMEOUT 30

// ============================================================================
// ANSI ESCAPE CODES FOR TERMINAL OUTPUT
// ============================================================================

#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_CREAM_BG "\033[48;2;255;248;220m"
#define ANSI_HIGH_CONTRAST "\033[97;40m"

// ============================================================================
// EXTERNAL DEPENDENCIES
// ============================================================================

extern EducationStudentProfile* education_profile_get_active(void);

// ============================================================================
// DYSLEXIA ADAPTATIONS (DY01-07)
// ============================================================================

/**
 * DY01: Get appropriate font for student
 */
const char* a11y_get_font(const EducationAccessibility* access) {
    if (access && access->dyslexia) {
        return FONT_OPENDYSLEXIC;
    }
    return FONT_DEFAULT;
}

/**
 * DY02: Get line spacing multiplier
 */
float a11y_get_line_spacing(const EducationAccessibility* access) {
    if (access && access->dyslexia) {
        return DYSLEXIA_LINE_SPACING;
    }
    return DEFAULT_LINE_SPACING;
}

/**
 * DY03: Get maximum chars per line
 */
int a11y_get_max_line_width(const EducationAccessibility* access) {
    if (access && access->dyslexia) {
        return DYSLEXIA_MAX_CHARS_PER_LINE;
    }
    return DEFAULT_MAX_CHARS_PER_LINE;
}

/**
 * DY03: Wrap text to max line width
 */
char* a11y_wrap_text(const char* text, int max_width) {
    if (!text || max_width <= 0) return NULL;

    size_t len = strlen(text);
    // Worst case: every char needs a newline
    char* result = malloc(len * 2 + 1);
    if (!result) return NULL;

    int line_pos = 0;
    size_t out_pos = 0;
    size_t last_space = 0;

    for (size_t i = 0; i < len; i++) {
        if (text[i] == ' ') {
            last_space = out_pos;
        }
        if (text[i] == '\n') {
            line_pos = 0;
            last_space = 0;
        }

        result[out_pos++] = text[i];
        line_pos++;

        if (line_pos >= max_width && text[i] != '\n') {
            if (last_space > 0) {
                // Break at last space
                result[last_space] = '\n';
                line_pos = (int)(out_pos - last_space - 1);
            } else {
                // Force break
                result[out_pos++] = '\n';
                line_pos = 0;
            }
            last_space = 0;
        }
    }
    result[out_pos] = '\0';
    return result;
}

/**
 * DY04: Get background color
 */
const char* a11y_get_background_color(const EducationAccessibility* access) {
    if (access && access->dyslexia) {
        return BG_CREAM;
    }
    if (access && access->high_contrast) {
        return "#000000";  // Black for high contrast
    }
    return BG_DEFAULT;
}

/**
 * DY04: Get ANSI background escape sequence
 */
const char* a11y_get_background_ansi(const EducationAccessibility* access) {
    if (access && access->dyslexia) {
        return ANSI_CREAM_BG;
    }
    if (access && access->high_contrast) {
        return ANSI_HIGH_CONTRAST;
    }
    return "";
}

/**
 * DY05: Check if TTS with highlighting should be used
 */
bool a11y_wants_tts_highlight(const EducationAccessibility* access) {
    return access && access->dyslexia && access->tts_enabled;
}

/**
 * DY06: Syllabify Italian word
 */
char* a11y_syllabify_word(const char* word) {
    if (!word || strlen(word) < 2) return strdup(word ? word : "");

    size_t len = strlen(word);
    // Max: original + separator after each char
    char* result = malloc(len * 2 + 1);
    if (!result) return NULL;

    size_t out = 0;
    const char* vowels = "aeiouAEIOU";

    for (size_t i = 0; i < len; i++) {
        result[out++] = word[i];

        // Simple Italian syllabification rules
        if (i < len - 1) {
            bool curr_vowel = (strchr(vowels, word[i]) != NULL);
            bool next_vowel = (strchr(vowels, word[i+1]) != NULL);
            bool next_next_vowel = (i < len - 2) ? (strchr(vowels, word[i+2]) != NULL) : false;

            // Break after vowel if followed by consonant + vowel
            if (curr_vowel && !next_vowel && (i < len - 2) && next_next_vowel) {
                result[out++] = '-';
            }
            // Break between two consonants if second is followed by vowel
            else if (!curr_vowel && !next_vowel && (i < len - 2) && next_next_vowel) {
                result[out++] = '-';
            }
        }
    }
    result[out] = '\0';
    return result;
}

/**
 * DY06: Syllabify entire text
 */
char* a11y_syllabify_text(const char* text) {
    if (!text) return NULL;

    size_t len = strlen(text);
    char* result = malloc(len * 2 + 1);
    if (!result) return NULL;

    size_t out = 0;
    size_t word_start = 0;

    for (size_t i = 0; i <= len; i++) {
        if (!isalpha((unsigned char)text[i]) || i == len) {
            if (i > word_start) {
                // Extract word
                size_t word_len = i - word_start;
                char* word = malloc(word_len + 1);
                if (word) {
                    strncpy(word, text + word_start, word_len);
                    word[word_len] = '\0';

                    // Syllabify word
                    char* syllabified = a11y_syllabify_word(word);
                    if (syllabified) {
                        size_t syl_len = strlen(syllabified);
                        strncpy(result + out, syllabified, len * 2 - out);
                        out += syl_len;
                        free(syllabified);
                    }
                    free(word);
                }
            }
            // Copy non-letter character
            if (i < len) {
                result[out++] = text[i];
            }
            word_start = i + 1;
        }
    }
    result[out] = '\0';
    return result;
}

// ============================================================================
// DYSCALCULIA ADAPTATIONS (DC01-06)
// ============================================================================

/**
 * DC01/DC02: Format number with color-coded place values
 */
char* a11y_format_number_colored(double number, bool use_colors) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.0f", number);

    if (!use_colors) return strdup(buffer);

    size_t len = strlen(buffer);
    // Each digit can have ANSI codes (~10 chars)
    char* result = malloc(len * 15 + 20);
    if (!result) return strdup(buffer);

    result[0] = '\0';

    for (size_t i = 0; i < len; i++) {
        if (buffer[i] == '-') {
            strncat(result, "\033[36m-\033[0m", 20);  // Cyan for negative
            continue;
        }
        if (buffer[i] == '.') {
            strncat(result, "\033[33m.\033[0m", 20);  // Yellow for decimal
            continue;
        }

        int place = (int)(len - i - 1);  // 0=units, 1=tens, etc.
        const char* color;

        switch (place % 3) {
            case 0: color = "\033[34m"; break;  // Blue - units
            case 1: color = "\033[32m"; break;  // Green - tens
            case 2: color = "\033[31m"; break;  // Red - hundreds
            default: color = "\033[0m"; break;
        }

        char digit_buf[20];
        snprintf(digit_buf, sizeof(digit_buf), "%s%c\033[0m", color, buffer[i]);
        strncat(result, digit_buf, 20);
    }

    return result;
}

/**
 * DC01: Generate visual blocks for number
 */
char* a11y_generate_place_value_blocks(int number) {
    if (number < 0) number = -number;

    int thousands = number / 1000;
    int hundreds = (number % 1000) / 100;
    int tens = (number % 100) / 10;
    int units = number % 10;

    // Allocate enough space
    char* result = malloc(1024);
    if (!result) return NULL;

    result[0] = '\0';

    if (thousands > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Migliaia: ");
        strncat(result, buf, 64);
        for (int i = 0; i < thousands; i++) strncat(result, "[████] ", 10);
        strncat(result, "\n", 2);
    }

    if (hundreds > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Centinaia: ");
        strncat(result, buf, 64);
        for (int i = 0; i < hundreds; i++) strncat(result, "[███] ", 10);
        strncat(result, "\n", 2);
    }

    if (tens > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Decine: ");
        strncat(result, buf, 64);
        for (int i = 0; i < tens; i++) strncat(result, "[██] ", 8);
        strncat(result, "\n", 2);
    }

    if (units > 0 || (thousands == 0 && hundreds == 0 && tens == 0)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Unita: ");
        strncat(result, buf, 64);
        for (int i = 0; i < units; i++) strncat(result, "[█] ", 6);
        strncat(result, "\n", 2);
    }

    return result;
}

/**
 * DC05: Check if timer should be disabled for math
 */
bool a11y_disable_math_timer(const EducationAccessibility* access) {
    return access && access->dyscalculia;
}

// ============================================================================
// CEREBRAL PALSY ADAPTATIONS (CP01-05)
// ============================================================================

/**
 * CP01: Check if voice input is primary
 */
bool a11y_prefers_voice_input(const EducationAccessibility* access) {
    return access && access->cerebral_palsy &&
           access->preferred_input == INPUT_VOICE;
}

/**
 * CP03: Get timeout multiplier
 */
int a11y_get_timeout_multiplier(const EducationAccessibility* access) {
    if (access && access->cerebral_palsy) {
        switch (access->cerebral_palsy_severity) {
            case SEVERITY_SEVERE: return 5;
            case SEVERITY_MODERATE: return 3;
            case SEVERITY_MILD: return 2;
            default: return 1;
        }
    }
    return 1;
}

/**
 * CP03: Get adjusted timeout
 */
int a11y_get_adjusted_timeout(const EducationAccessibility* access, int base_timeout) {
    return base_timeout * a11y_get_timeout_multiplier(access);
}

/**
 * CP04: Check if break should be suggested
 */
bool a11y_suggest_break(const EducationAccessibility* access, int minutes_elapsed) {
    if (!access) return false;

    // CP users: suggest break every 15 minutes
    if (access->cerebral_palsy && minutes_elapsed >= 15 && minutes_elapsed % 15 == 0) {
        return true;
    }
    // ADHD users: suggest break every 10 minutes
    if (access->adhd && minutes_elapsed >= 10 && minutes_elapsed % 10 == 0) {
        return true;
    }
    return false;
}

// ============================================================================
// ADHD ADAPTATIONS (AD01-06)
// ============================================================================

/**
 * AD01: Get maximum number of bullet points
 */
int a11y_get_max_bullets(const EducationAccessibility* access) {
    if (access && access->adhd) {
        switch (access->adhd_severity) {
            case SEVERITY_SEVERE: return 2;
            case SEVERITY_MODERATE: return 3;
            case SEVERITY_MILD: return 4;
            default: return ADHD_MAX_BULLETS;
        }
    }
    return DEFAULT_MAX_BULLETS;
}

/**
 * AD01: Truncate text to bullet point limit
 */
char* a11y_limit_bullets(const char* text, int max_bullets) {
    if (!text || max_bullets <= 0) return strdup(text ? text : "");

    char* result = strdup(text);
    if (!result) return NULL;

    int bullet_count = 0;
    char* ptr = result;
    char* last_bullet_end = result;

    while (*ptr && bullet_count < max_bullets) {
        if (*ptr == '-' || *ptr == '*' || *ptr == '\xe2') {  // UTF-8 bullet
            bullet_count++;
        }
        if (*ptr == '\n' && bullet_count > 0) {
            last_bullet_end = ptr;
        }
        ptr++;
    }

    if (bullet_count >= max_bullets && last_bullet_end < ptr) {
        *last_bullet_end = '\0';
    }

    return result;
}

/**
 * AD02: Generate ASCII progress bar
 */
char* a11y_generate_progress_bar(int current, int total, int width) {
    if (total <= 0) total = 1;
    if (width <= 0) width = PROGRESS_BAR_WIDTH;

    char* result = malloc(width + 20);
    if (!result) return NULL;

    int filled = (current * width) / total;
    if (filled > width) filled = width;

    result[0] = '[';
    for (int i = 0; i < width; i++) {
        result[i + 1] = (i < filled) ? '#' : '-';
    }
    result[width + 1] = ']';

    int percent = (current * 100) / total;
    snprintf(result + width + 2, 10, " %d%%", percent);

    return result;
}

/**
 * AD03: Generate micro-celebration message
 */
const char* a11y_get_celebration_message(int achievement_level) {
    static const char* messages[] = {
        "Ottimo! Continua cosi!",
        "Fantastico! Stai andando alla grande!",
        "Wow! Sei un campione!",
        "Incredibile! Hai fatto centro!",
        "Perfetto! Sei inarrestabile!"
    };

    if (achievement_level < 0) achievement_level = 0;
    if (achievement_level > 4) achievement_level = 4;

    return messages[achievement_level];
}

/**
 * AD06: Check if gamification should be enhanced
 */
bool a11y_enhance_gamification(const EducationAccessibility* access) {
    return access && access->adhd;
}

// ============================================================================
// AUTISM ADAPTATIONS (AU01-06)
// ============================================================================

/**
 * AU01: Check if metaphors should be avoided
 */
bool a11y_avoid_metaphors(const EducationAccessibility* access) {
    return access && access->autism;
}

/**
 * AU01: List of common metaphors to avoid
 */
static const char* METAPHORS_TO_AVOID[] = {
    "come se",
    "tipo",
    "praticamente",
    "in un certo senso",
    "per modo di dire",
    NULL
};

/**
 * AU01: Check text for metaphors
 */
bool a11y_contains_metaphors(const char* text) {
    if (!text) return false;

    for (int i = 0; METAPHORS_TO_AVOID[i] != NULL; i++) {
        if (strstr(text, METAPHORS_TO_AVOID[i]) != NULL) {
            return true;
        }
    }
    return false;
}

/**
 * AU02: Get structure prefix for autism
 */
const char* a11y_get_structure_prefix(const char* section_type) {
    if (!section_type) return "";

    if (strcmp(section_type, "intro") == 0) {
        return "INTRODUZIONE: Ora spiegheremo il seguente argomento.\n";
    }
    if (strcmp(section_type, "main") == 0) {
        return "SPIEGAZIONE PRINCIPALE:\n";
    }
    if (strcmp(section_type, "example") == 0) {
        return "ESEMPIO PRATICO:\n";
    }
    if (strcmp(section_type, "summary") == 0) {
        return "RIASSUNTO DEI PUNTI CHIAVE:\n";
    }
    if (strcmp(section_type, "next") == 0) {
        return "PROSSIMO ARGOMENTO:\n";
    }
    return "";
}

/**
 * AU03: Generate topic change warning
 */
char* a11y_get_topic_change_warning(const char* old_topic, const char* new_topic) {
    char* result = malloc(256);
    if (!result) return NULL;

    snprintf(result, 256,
        "\n[ATTENZIONE: Cambio argomento]\n"
        "Abbiamo finito di parlare di: %s\n"
        "Ora parleremo di: %s\n"
        "Sei pronto per continuare?\n\n",
        old_topic ? old_topic : "argomento precedente",
        new_topic ? new_topic : "nuovo argomento");

    return result;
}

/**
 * AU05: Check if social pressure should be avoided
 */
bool a11y_avoid_social_pressure(const EducationAccessibility* access) {
    return access && access->autism;
}

/**
 * AU06: Check if motion should be reduced
 */
bool a11y_reduce_motion(const EducationAccessibility* access) {
    return access && access->reduce_motion;
}

// ============================================================================
// COMBINED ADAPTATION HELPERS
// ============================================================================

/**
 * Apply all text adaptations based on accessibility profile
 */
char* a11y_adapt_text_full(const char* text, const EducationAccessibility* access) {
    if (!text) return NULL;

    char* result = strdup(text);
    if (!result) return NULL;

    // DY03: Wrap text to max line width
    if (access && access->dyslexia) {
        int max_width = a11y_get_max_line_width(access);
        char* wrapped = a11y_wrap_text(result, max_width);
        if (wrapped) {
            free(result);
            result = wrapped;
        }
    }

    // AD01: Limit bullet points for ADHD
    if (access && access->adhd) {
        int max_bullets = a11y_get_max_bullets(access);
        char* limited = a11y_limit_bullets(result, max_bullets);
        if (limited) {
            free(result);
            result = limited;
        }
    }

    return result;
}

// ============================================================================
// VOICE ACCESSIBILITY (FASE 10)
// ============================================================================

/**
 * Get effective speech rate combining maestro default and user preference
 * @param access User accessibility settings
 * @param maestro_default Maestro's base speech rate (typically 0.9-1.1)
 * @return Final speech rate clamped to 0.5-2.0
 */
float a11y_get_speech_rate(const EducationAccessibility* access, float maestro_default) {
    float rate = maestro_default > 0 ? maestro_default : 1.0f;

    if (access && access->tts_speed > 0) {
        // User preference modifies maestro's default
        rate = rate * access->tts_speed;
    }

    // Clamp to valid range
    if (rate < 0.5f) rate = 0.5f;
    if (rate > 2.0f) rate = 2.0f;

    return rate;
}

/**
 * Get effective pitch offset combining maestro default and user preference
 * @param access User accessibility settings
 * @param maestro_default Maestro's pitch offset (-1.0 to 1.0)
 * @return Final pitch offset clamped to -1.0 to 1.0
 */
float a11y_get_pitch_offset(const EducationAccessibility* access, float maestro_default) {
    float pitch = maestro_default;

    if (access) {
        // Add user's preference to maestro's default
        pitch = pitch + access->tts_pitch;
    }

    // Clamp to valid range
    if (pitch < -1.0f) pitch = -1.0f;
    if (pitch > 1.0f) pitch = 1.0f;

    return pitch;
}

/**
 * Get all accessibility adaptations as a struct
 */
typedef struct {
    // Display
    const char* font_family;
    float line_spacing;
    int max_line_width;
    const char* background_color;
    const char* ansi_bg;
    bool reduce_motion;

    // Audio/Voice
    bool use_tts;
    bool tts_highlight;
    float tts_speed;
    float tts_pitch;

    // Math
    bool show_place_blocks;
    bool color_numbers;
    bool disable_timer;

    // Content
    int max_bullets;
    bool avoid_metaphors;
    bool enhance_gamification;

    // Timing
    int timeout_multiplier;
    int break_interval_minutes;

    // Input
    bool prefer_voice;
    bool voice_commands;
} A11ySettings;

A11ySettings a11y_get_all_settings(const EducationAccessibility* access) {
    A11ySettings settings = {
        .font_family = a11y_get_font(access),
        .line_spacing = a11y_get_line_spacing(access),
        .max_line_width = a11y_get_max_line_width(access),
        .background_color = a11y_get_background_color(access),
        .ansi_bg = a11y_get_background_ansi(access),
        .reduce_motion = a11y_reduce_motion(access),
        .use_tts = access ? access->tts_enabled : false,
        .tts_highlight = a11y_wants_tts_highlight(access),
        .tts_speed = access ? access->tts_speed : 1.0f,
        .tts_pitch = access ? access->tts_pitch : 0.0f,
        .show_place_blocks = access ? access->dyscalculia : false,
        .color_numbers = access ? access->dyscalculia : false,
        .disable_timer = a11y_disable_math_timer(access),
        .max_bullets = a11y_get_max_bullets(access),
        .avoid_metaphors = a11y_avoid_metaphors(access),
        .enhance_gamification = a11y_enhance_gamification(access),
        .timeout_multiplier = a11y_get_timeout_multiplier(access),
        .break_interval_minutes = (access && access->adhd) ? 10 :
                                  (access && access->cerebral_palsy) ? 15 : 30,
        .prefer_voice = a11y_prefers_voice_input(access),
        .voice_commands = access && access->cerebral_palsy
    };

    return settings;
}
