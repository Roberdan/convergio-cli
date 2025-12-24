/**
 * CONVERGIO EDUCATION - TEXT-TO-SPEECH & AUDIO ENGINE
 *
 * Generates audio summaries and provides TTS support
 * using macOS AVSpeechSynthesizer with accessibility adaptations.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/education.h"
#include "nous/orchestrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define TTS_DEFAULT_SPEED 1.0f
#define TTS_MIN_SPEED 0.5f
#define TTS_MAX_SPEED 2.0f
#define TTS_DEFAULT_VOICE_IT "Alice"
#define TTS_DEFAULT_VOICE_EN "Samantha"

// ============================================================================
// TYPES
// ============================================================================

typedef enum {
    TTS_VOICE_ITALIAN,
    TTS_VOICE_ENGLISH,
    TTS_VOICE_FRENCH,
    TTS_VOICE_GERMAN,
    TTS_VOICE_SPANISH
} TTSLanguage;

typedef struct {
    const char* voice_name;
    TTSLanguage language;
    float speed;  // 0.5 - 2.0
    float pitch;  // 0.5 - 2.0
    float volume; // 0.0 - 1.0
    bool highlight_words;
    int pause_between_sentences_ms;
} TTSSettings;

typedef struct {
    char* text;
    char* audio_path;
    int duration_seconds;
    TTSSettings settings;
} AudioOutput;

// ============================================================================
// ACCESSIBILITY ADAPTATIONS
// ============================================================================

static TTSSettings get_tts_settings(const EducationAccessibility* a) {
    TTSSettings settings = {.voice_name = TTS_DEFAULT_VOICE_IT,
                            .language = TTS_VOICE_ITALIAN,
                            .speed = TTS_DEFAULT_SPEED,
                            .pitch = 1.0f,
                            .volume = 1.0f,
                            .highlight_words = false,
                            .pause_between_sentences_ms = 300};

    if (!a)
        return settings;

    // Apply user's TTS preferences
    if (a->tts_speed > 0) {
        settings.speed = a->tts_speed;
        if (settings.speed < TTS_MIN_SPEED)
            settings.speed = TTS_MIN_SPEED;
        if (settings.speed > TTS_MAX_SPEED)
            settings.speed = TTS_MAX_SPEED;
    }

    if (a->tts_voice) {
        settings.voice_name = a->tts_voice;
    }

    // Dyslexia adaptations
    if (a->dyslexia) {
        settings.highlight_words = true;
        settings.pause_between_sentences_ms = 500;
        if (a->dyslexia_severity >= SEVERITY_MODERATE) {
            settings.speed = settings.speed * 0.9f; // Slightly slower
        }
    }

    // ADHD adaptations
    if (a->adhd) {
        settings.speed = settings.speed * 1.1f; // Slightly faster to maintain attention
        settings.pause_between_sentences_ms = 200;
    }

    return settings;
}

// ============================================================================
// TTS GENERATION (macOS)
// ============================================================================

/**
 * Convert text to speech using macOS 'say' command
 * Returns path to generated audio file
 */
char* tts_generate_audio(const char* text, const TTSSettings* settings, const char* output_path) {
    if (!text || !settings)
        return NULL;

    // Determine output path
    char* audio_path;
    if (output_path) {
        audio_path = strdup(output_path);
    } else {
        audio_path = malloc(256);
        snprintf(audio_path, 256, "/tmp/tts_%d_%ld.m4a", getpid(), (long)time(NULL));
    }

    // Calculate rate (words per minute)
    // Normal speech is ~150-180 wpm, 'say' default is ~175
    int rate = (int)(175 * settings->speed);

    // Build command
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "say -v '%s' -r %d -o '%s' --file-format=m4af '%s' 2>/dev/null",
             settings->voice_name, rate, audio_path, text);

    int result = system(cmd);

    if (result != 0) {
        // Fallback to default voice
        snprintf(cmd, sizeof(cmd), "say -r %d -o '%s' --file-format=m4af '%s' 2>/dev/null", rate,
                 audio_path, text);
        result = system(cmd);
    }

    if (result != 0) {
        free(audio_path);
        return NULL;
    }

    return audio_path;
}

/**
 * Speak text immediately (non-blocking)
 */
int tts_speak(const char* text, const TTSSettings* settings) {
    if (!text)
        return -1;

    TTSSettings default_settings = {.voice_name = TTS_DEFAULT_VOICE_IT, .speed = TTS_DEFAULT_SPEED};

    if (!settings)
        settings = &default_settings;

    int rate = (int)(175 * settings->speed);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "say -v '%s' -r %d '%s' &", settings->voice_name, rate, text);

    return system(cmd);
}

/**
 * Stop any ongoing speech
 */
int tts_stop(void) {
    return system("killall say 2>/dev/null");
}

// ============================================================================
// AUDIO SUMMARY GENERATION
// ============================================================================

static const char* AUDIO_SUMMARY_PROMPT =
    "Create a spoken summary of the following content:\n\n"
    "%s\n\n"
    "Requirements:\n"
    "- Length: %s (short=1-2 min, medium=3-5 min, long=5-10 min)\n"
    "- Style: Conversational and clear\n"
    "- Include key points and takeaways\n"
    "%s"
    "\nWrite the summary as if speaking directly to a student.";

typedef enum { SUMMARY_SHORT, SUMMARY_MEDIUM, SUMMARY_LONG } SummaryLength;

/**
 * Generate audio summary using LLM + TTS
 */
AudioOutput* audio_generate_summary(const char* content, const char* topic, SummaryLength length,
                                    const EducationAccessibility* access) {
    if (!content)
        return NULL;

    AudioOutput* output = calloc(1, sizeof(AudioOutput));
    if (!output)
        return NULL;

    output->settings = get_tts_settings(access);

    // Determine length string
    const char* length_str = "medium";
    switch (length) {
    case SUMMARY_SHORT:
        length_str = "short";
        break;
    case SUMMARY_LONG:
        length_str = "long";
        break;
    default:
        break;
    }

    // Accessibility requirements
    char access_req[256] = "";
    size_t remaining = sizeof(access_req) - 1;
    if (access) {
        if (access->dyslexia) {
            strncat(access_req, "- Use simple, clear language\n", remaining);
            remaining = sizeof(access_req) - strlen(access_req) - 1;
            strncat(access_req, "- Short sentences\n", remaining);
            remaining = sizeof(access_req) - strlen(access_req) - 1;
        }
        if (access->adhd) {
            strncat(access_req, "- Keep it engaging and dynamic\n", remaining);
            remaining = sizeof(access_req) - strlen(access_req) - 1;
            strncat(access_req, "- Vary the pace\n", remaining);
        }
    }

    // Build prompt
    size_t prompt_size = strlen(AUDIO_SUMMARY_PROMPT) + strlen(content) + strlen(length_str) +
                         strlen(access_req) + 100;
    char* prompt = malloc(prompt_size);

    if (prompt) {
        snprintf(prompt, prompt_size, AUDIO_SUMMARY_PROMPT, content, length_str, access_req);

        // Call LLM to get summary text
        TokenUsage usage = {0};
        char* response =
            llm_chat("You are an expert educational narrator. Create spoken summaries that are "
                     "clear, engaging, and appropriate for audio. Write naturally as if speaking "
                     "directly to the listener. Do not include any formatting or markup.",
                     prompt, &usage);

        if (response && strlen(response) > 0) {
            // Use LLM-generated summary
            output->text = response;
        } else {
            // Fallback: use the content as-is (truncated)
            if (response)
                free(response);
            size_t max_len = 2000;
            size_t content_len = strlen(content);
            if (content_len > max_len)
                content_len = max_len;

            output->text = malloc(content_len + 1);
            strncpy(output->text, content, content_len);
            output->text[content_len] = '\0';
        }

        free(prompt);
    }

    // Generate audio
    if (output->text) {
        output->audio_path = tts_generate_audio(output->text, &output->settings, NULL);

        // Estimate duration (rough: ~2.5 words per second at normal speed)
        int word_count = 0;
        const char* p = output->text;
        while (*p) {
            if (*p == ' ' || *p == '\n')
                word_count++;
            p++;
        }
        output->duration_seconds = (int)(word_count / (2.5f * output->settings.speed));
    }

    return output;
}

// ============================================================================
// SYNCHRONIZED TEXT HIGHLIGHTING
// ============================================================================

typedef struct {
    int start_char;
    int end_char;
    float start_time;
    float end_time;
} WordTiming;

typedef struct {
    char* text;
    WordTiming* timings;
    int timing_count;
} SyncedText;

/**
 * Generate word timings for synchronized highlighting
 */
SyncedText* tts_generate_synced_text(const char* text, const TTSSettings* settings) {
    if (!text)
        return NULL;

    SyncedText* synced = calloc(1, sizeof(SyncedText));
    if (!synced)
        return NULL;

    synced->text = strdup(text);

    // Count words
    int word_count = 1;
    for (const char* p = text; *p; p++) {
        if (*p == ' ' || *p == '\n')
            word_count++;
    }

    synced->timings = calloc(word_count, sizeof(WordTiming));
    synced->timing_count = 0;

    // Generate approximate timings
    // Average word duration = 1.0 / (2.5 * speed) seconds
    float avg_word_duration = 1.0f / (2.5f * (settings ? settings->speed : 1.0f));
    float current_time = 0;

    const char* word_start = text;
    for (const char* p = text;; p++) {
        if (*p == ' ' || *p == '\n' || *p == '\0') {
            if (p > word_start) {
                WordTiming* t = &synced->timings[synced->timing_count];
                t->start_char = word_start - text;
                t->end_char = p - text;
                t->start_time = current_time;
                t->end_time = current_time + avg_word_duration;

                current_time = t->end_time;
                synced->timing_count++;
            }
            word_start = p + 1;
        }
        if (*p == '\0')
            break;
    }

    return synced;
}

void synced_text_free(SyncedText* synced) {
    if (!synced)
        return;
    free(synced->text);
    free(synced->timings);
    free(synced);
}

// ============================================================================
// AUDIOBOOK SUPPORT
// ============================================================================

typedef struct {
    char* title;
    char* author;
    char** chapters;
    char** chapter_audio;
    int chapter_count;
    int current_chapter;
    float current_position;
} Audiobook;

/**
 * Generate audiobook from text chapters
 */
Audiobook* audiobook_create(const char* title, const char* author, const char** chapter_texts,
                            int chapter_count, const EducationAccessibility* access) {
    if (!title || !chapter_texts || chapter_count < 1)
        return NULL;

    Audiobook* book = calloc(1, sizeof(Audiobook));
    if (!book)
        return NULL;

    book->title = strdup(title);
    book->author = author ? strdup(author) : NULL;
    book->chapter_count = chapter_count;
    book->chapters = calloc(chapter_count, sizeof(char*));
    book->chapter_audio = calloc(chapter_count, sizeof(char*));

    TTSSettings settings = get_tts_settings(access);

    for (int i = 0; i < chapter_count; i++) {
        book->chapters[i] = strdup(chapter_texts[i]);
        book->chapter_audio[i] = tts_generate_audio(chapter_texts[i], &settings, NULL);
    }

    return book;
}

void audiobook_free(Audiobook* book) {
    if (!book)
        return;
    free(book->title);
    free(book->author);
    for (int i = 0; i < book->chapter_count; i++) {
        free(book->chapters[i]);
        free(book->chapter_audio[i]);
    }
    free(book->chapters);
    free(book->chapter_audio);
    free(book);
}

// ============================================================================
// CLEANUP
// ============================================================================

void audio_output_free(AudioOutput* output) {
    if (!output)
        return;
    free(output->text);
    if (output->audio_path) {
        // Optionally delete the audio file
        // unlink(output->audio_path);
        free(output->audio_path);
    }
    free(output);
}

// ============================================================================
// CLI COMMAND HANDLER
// ============================================================================

int audio_command_handler(int argc, char** argv, const EducationStudentProfile* profile) {
    if (argc < 2) {
        printf("Usage: /audio <topic> [--length short|medium|long] [--output path]\n");
        printf("       /audio speak \"text to read\"\n");
        return 1;
    }

    const char* subcommand = argv[1];

    // Handle "speak" subcommand
    if (strcmp(subcommand, "speak") == 0) {
        if (argc < 3) {
            printf("Usage: /audio speak \"text to read\"\n");
            return 1;
        }

        const EducationAccessibility* access = profile ? profile->accessibility : NULL;
        TTSSettings settings = get_tts_settings(access);

        printf("Speaking...\n");
        tts_speak(argv[2], &settings);
        return 0;
    }

    // Generate audio summary
    const char* topic = subcommand;
    SummaryLength length = SUMMARY_MEDIUM;
    const char* output = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--length") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "short") == 0)
                length = SUMMARY_SHORT;
            else if (strcmp(argv[i], "long") == 0)
                length = SUMMARY_LONG;
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output = argv[++i];
        }
    }

    printf("Generating audio summary for: %s\n", topic);

    const EducationAccessibility* access = profile ? profile->accessibility : NULL;

    AudioOutput* audio =
        audio_generate_summary("Generate appropriate content for the topic", topic, length, access);

    if (!audio || !audio->audio_path) {
        fprintf(stderr, "Failed to generate audio\n");
        if (audio)
            audio_output_free(audio);
        return 1;
    }

    printf("Audio generated: %s\n", audio->audio_path);
    printf("Duration: ~%d seconds\n", audio->duration_seconds);

    if (output) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s'", audio->audio_path, output);
        system(cmd);
        printf("Saved to: %s\n", output);
    }

    audio_output_free(audio);
    return 0;
}
