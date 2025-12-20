/**
 * CONVERGIO EDUCATION - VOICE MODE
 *
 * CLI voice mode that integrates WebSocket, audio capture, and playback.
 * Provides natural conversational interface with maestri.
 *
 * ADR: ADR-003-voice-cli-conversational-ux.md
 *
 * Usage: /voice [maestro] [topic]
 * Build: make VOICE=1
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef CONVERGIO_VOICE_ENABLED

#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include "nous/education.h"

// Forward declaration for accessibility functions
extern EducationStudentProfile* education_profile_get_active(void);
extern float a11y_get_speech_rate(const EducationAccessibility* access, float maestro_default);
extern float a11y_get_pitch_offset(const EducationAccessibility* access, float maestro_default);

// ============================================================================
// VOICE MODE STATE
// ============================================================================

typedef enum {
    VOICE_MODE_IDLE,
    VOICE_MODE_LISTENING,
    VOICE_MODE_PROCESSING,
    VOICE_MODE_SPEAKING,
    VOICE_MODE_ERROR
} VoiceModeState;

typedef struct {
    VoiceModeState state;
    bool active;
    bool muted;

    char maestro_id[64];
    char maestro_name[64];
    char topic[256];

    char *transcript_user;
    char *transcript_maestro;
    bool transcript_visible;

    struct termios orig_termios;
    bool raw_mode;

    pthread_mutex_t state_mutex;

    // Accessibility settings (loaded from student profile)
    float speech_rate;           // TTS speech rate (0.5-2.0)
    float pitch_offset;          // TTS pitch offset (-1.0 to 1.0)
    int break_interval_minutes;  // Suggested break interval
    bool tts_highlight;          // Highlight words during TTS
    bool high_contrast;          // High contrast mode
    EducationStudentProfile* profile;  // Active student profile

} VoiceModeContext;

static VoiceModeContext g_voice_mode = {0};
static volatile bool g_voice_mode_running = false;

// Forward declarations for voice subsystems
extern bool voice_audio_init(void);
extern void voice_audio_cleanup(void);
extern bool voice_audio_start_capture(void (*)(const int16_t*, size_t, void*), void*);
extern void voice_audio_stop_capture(void);
extern bool voice_audio_start_playback(void);
extern void voice_audio_stop_playback(void);
extern void voice_audio_play(const int16_t*, size_t);

// WebSocket types and functions
typedef struct VoiceWebSocket VoiceWebSocket;
typedef enum { WS_DISCONNECTED, WS_CONNECTING, WS_CONNECTED, WS_AUTHENTICATING, WS_READY, WS_ERROR } WsState;

extern VoiceWebSocket *voice_ws_create(void);
extern void voice_ws_destroy(VoiceWebSocket *ws);
extern bool voice_ws_connect(VoiceWebSocket *ws);
extern void voice_ws_disconnect(VoiceWebSocket *ws);
extern void voice_ws_send_audio(VoiceWebSocket *ws, const uint8_t *audio, size_t length);
extern void voice_ws_cancel_response(VoiceWebSocket *ws);
extern void voice_ws_set_maestro(VoiceWebSocket *ws, const char *maestro_id, const char *instructions);
extern void voice_ws_set_callbacks(VoiceWebSocket *ws,
                                   void (*on_audio)(const uint8_t*, size_t, void*),
                                   void (*on_transcript)(const char*, bool, bool, void*),
                                   void (*on_state)(WsState, void*),
                                   void (*on_error)(const char*, void*),
                                   void *user_data);
extern WsState voice_ws_get_state(VoiceWebSocket *ws);

static VoiceWebSocket *g_ws = NULL;

// ============================================================================
// TERMINAL RAW MODE
// ============================================================================

static void enter_raw_mode(void) {
    if (g_voice_mode.raw_mode) return;
    tcgetattr(STDIN_FILENO, &g_voice_mode.orig_termios);
    struct termios raw = g_voice_mode.orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    g_voice_mode.raw_mode = true;
    printf("\033[?25l");
    fflush(stdout);
}

static void exit_raw_mode(void) {
    if (!g_voice_mode.raw_mode) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_voice_mode.orig_termios);
    g_voice_mode.raw_mode = false;
    printf("\033[?25h");
    fflush(stdout);
}

// ============================================================================
// UI RENDERING
// ============================================================================

static const char *state_indicator(VoiceModeState state) {
    switch (state) {
        case VOICE_MODE_IDLE:       return "\033[90m░░░░░░░░░░░░░░░░░░░░\033[0m";
        case VOICE_MODE_LISTENING:  return "\033[32m▁▂▃▅▆▇█▇▆▅▃▂▁▂▃▅▆▇\033[0m";
        case VOICE_MODE_PROCESSING: return "\033[33m●●●○○○●●●○○○●●●○○○\033[0m";
        case VOICE_MODE_SPEAKING:   return "\033[34m████████████░░░░░░░░\033[0m";
        case VOICE_MODE_ERROR:      return "\033[31m✗✗✗✗✗✗✗✗✗✗✗✗✗✗✗✗✗✗\033[0m";
        default:                    return "                    ";
    }
}

static const char *state_label(VoiceModeState state) {
    switch (state) {
        case VOICE_MODE_IDLE:       return "Listening...";
        case VOICE_MODE_LISTENING:  return "Hearing you...";
        case VOICE_MODE_PROCESSING: return "Thinking...";
        case VOICE_MODE_SPEAKING:   return "Speaking...";
        case VOICE_MODE_ERROR:      return "Error";
        default:                    return "";
    }
}

static void render_ui(void) {
    printf("\033[2J\033[H");
    printf("\033[1;36m╔═══════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m║\033[0m  🎓 \033[1m%s\033[0m - Maestro di %s",
           g_voice_mode.maestro_name[0] ? g_voice_mode.maestro_name : "Euclide",
           g_voice_mode.topic[0] ? g_voice_mode.topic : "Matematica");
    int header_len = 10 + (int)strlen(g_voice_mode.maestro_name) + (int)strlen(g_voice_mode.topic);
    for (int i = header_len; i < 55; i++) printf(" ");
    printf("\033[1;36m║\033[0m\n");
    printf("\033[1;36m║\033[0m  Voice mode active. Just speak naturally.                  \033[1;36m║\033[0m\n");
    printf("\033[1;36m║\033[0m  Press ESC to exit, M to mute mic, T to toggle transcript. \033[1;36m║\033[0m\n");
    printf("\033[1;36m╚═══════════════════════════════════════════════════════════╝\033[0m\n\n");
    printf("  %s  %s\n\n", state_indicator(g_voice_mode.state), state_label(g_voice_mode.state));
    if (g_voice_mode.muted) printf("  \033[31m🔇 MUTED\033[0m\n\n");
    if (g_voice_mode.transcript_visible) {
        if (g_voice_mode.transcript_user && g_voice_mode.transcript_user[0])
            printf("  \033[1mYou:\033[0m \"%s\"\n", g_voice_mode.transcript_user);
        if (g_voice_mode.transcript_maestro && g_voice_mode.transcript_maestro[0])
            printf("  \033[1m%s:\033[0m \"%s\"\n",
                   g_voice_mode.maestro_name[0] ? g_voice_mode.maestro_name : "Euclide",
                   g_voice_mode.transcript_maestro);
    }
    printf("\n\033[90mESC: Exit | M: Mute | T: Transcript | S: Save\033[0m\n");
    fflush(stdout);
}

// ============================================================================
// CALLBACKS
// ============================================================================

static void on_audio_captured(const int16_t *samples, size_t count, void *user_data) {
    (void)user_data;
    if (!g_voice_mode_running || g_voice_mode.muted || !g_ws) return;
    voice_ws_send_audio(g_ws, (const uint8_t *)samples, count * sizeof(int16_t));
    if (g_voice_mode.state == VOICE_MODE_IDLE) {
        pthread_mutex_lock(&g_voice_mode.state_mutex);
        g_voice_mode.state = VOICE_MODE_LISTENING;
        pthread_mutex_unlock(&g_voice_mode.state_mutex);
        render_ui();
    }
}

static void on_audio_received(const uint8_t *data, size_t length, void *user_data) {
    (void)user_data;
    if (!g_voice_mode_running) return;
    voice_audio_play((const int16_t *)data, length / sizeof(int16_t));
    pthread_mutex_lock(&g_voice_mode.state_mutex);
    if (g_voice_mode.state != VOICE_MODE_SPEAKING) {
        g_voice_mode.state = VOICE_MODE_SPEAKING;
        pthread_mutex_unlock(&g_voice_mode.state_mutex);
        render_ui();
    } else {
        pthread_mutex_unlock(&g_voice_mode.state_mutex);
    }
}

static void on_transcript(const char *text, bool is_user, bool is_final, void *user_data) {
    (void)user_data;
    (void)is_final;
    if (!g_voice_mode_running) return;
    pthread_mutex_lock(&g_voice_mode.state_mutex);
    if (is_user) {
        free(g_voice_mode.transcript_user);
        g_voice_mode.transcript_user = strdup(text);
    } else {
        free(g_voice_mode.transcript_maestro);
        g_voice_mode.transcript_maestro = strdup(text);
    }
    pthread_mutex_unlock(&g_voice_mode.state_mutex);
    if (g_voice_mode.transcript_visible) render_ui();
}

static void on_ws_state(WsState state, void *user_data) {
    (void)user_data;
    pthread_mutex_lock(&g_voice_mode.state_mutex);
    if (state == WS_READY) g_voice_mode.state = VOICE_MODE_IDLE;
    else if (state == WS_ERROR) g_voice_mode.state = VOICE_MODE_ERROR;
    pthread_mutex_unlock(&g_voice_mode.state_mutex);
    render_ui();
}

static void on_ws_error(const char *message, void *user_data) {
    (void)user_data;
    (void)message;
    pthread_mutex_lock(&g_voice_mode.state_mutex);
    g_voice_mode.state = VOICE_MODE_ERROR;
    pthread_mutex_unlock(&g_voice_mode.state_mutex);
    render_ui();
}

// ============================================================================
// KEYBOARD HANDLER
// ============================================================================

static void handle_keypress(char key) {
    switch (key) {
        case 27: g_voice_mode_running = false; break;
        case 'm': case 'M': g_voice_mode.muted = !g_voice_mode.muted; render_ui(); break;
        case 't': case 'T': g_voice_mode.transcript_visible = !g_voice_mode.transcript_visible; render_ui(); break;
        default: break;
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

int voice_mode_start(const char *maestro_id, const char *topic) {
    memset(&g_voice_mode, 0, sizeof(g_voice_mode));
    pthread_mutex_init(&g_voice_mode.state_mutex, NULL);

    // Load accessibility profile if available
    g_voice_mode.profile = education_profile_get_active();
    const EducationAccessibility* access = g_voice_mode.profile ?
        &g_voice_mode.profile->accessibility : NULL;

    // Apply accessibility settings
    g_voice_mode.speech_rate = a11y_get_speech_rate(access, 1.0f);
    g_voice_mode.pitch_offset = a11y_get_pitch_offset(access, 0.0f);
    g_voice_mode.tts_highlight = access && access->dyslexia && access->tts_enabled;
    g_voice_mode.high_contrast = access && access->high_contrast;

    // Set break interval based on conditions
    if (access && access->adhd) {
        g_voice_mode.break_interval_minutes = 10;
    } else if (access && access->cerebral_palsy) {
        g_voice_mode.break_interval_minutes = 15;
    } else {
        g_voice_mode.break_interval_minutes = 30;
    }

    // Transcript visible by default, or always on if screen reader needed
    g_voice_mode.transcript_visible = true;
    if (access && access->screen_reader) {
        g_voice_mode.transcript_visible = true;  // Always show for screen reader users
    }

    if (maestro_id && maestro_id[0]) {
        strncpy(g_voice_mode.maestro_id, maestro_id, sizeof(g_voice_mode.maestro_id) - 1);
        strncpy(g_voice_mode.maestro_name, maestro_id, sizeof(g_voice_mode.maestro_name) - 1);
        if (g_voice_mode.maestro_name[0] >= 'a' && g_voice_mode.maestro_name[0] <= 'z')
            g_voice_mode.maestro_name[0] -= 32;
    } else {
        strcpy(g_voice_mode.maestro_id, "euclide-matematica");
        strcpy(g_voice_mode.maestro_name, "Euclide");
    }

    strncpy(g_voice_mode.topic, topic && topic[0] ? topic : "Mathematics", sizeof(g_voice_mode.topic) - 1);
    g_voice_mode.state = VOICE_MODE_IDLE;

    if (!voice_audio_init()) {
        fprintf(stderr, "Failed to initialize audio\n");
        return 1;
    }

    g_ws = voice_ws_create();
    if (!g_ws) {
        fprintf(stderr, "Failed to create WebSocket client\n");
        voice_audio_cleanup();
        return 1;
    }

    voice_ws_set_callbacks(g_ws, on_audio_received, on_transcript, on_ws_state, on_ws_error, NULL);

    char instructions[2048];

    // Build accessibility-aware instructions
    const char* speech_pace = "";
    if (g_voice_mode.speech_rate < 0.8f) {
        speech_pace = "Speak slowly and clearly, pausing between sentences. ";
    } else if (g_voice_mode.speech_rate > 1.2f) {
        speech_pace = "Speak at a brisk, efficient pace. ";
    } else {
        speech_pace = "Speak at a natural, conversational pace. ";
    }

    const char* accessibility_hints = "";
    if (access && access->dyslexia) {
        accessibility_hints = "Use short sentences and simple vocabulary. Repeat key concepts. ";
    } else if (access && access->adhd) {
        accessibility_hints = "Keep explanations brief and engaging. Use frequent checkpoints. ";
    } else if (access && access->autism) {
        accessibility_hints = "Be literal and direct. Avoid metaphors and idioms. Give clear structure. ";
    }

    snprintf(instructions, sizeof(instructions),
        "You are %s, an educational AI maestro. %s%s"
        "Help students learn about %s. Be patient, encouraging, and use analogies. "
        "If interrupted, acknowledge naturally and wait for the question.",
        g_voice_mode.maestro_name, speech_pace, accessibility_hints, g_voice_mode.topic);
    voice_ws_set_maestro(g_ws, g_voice_mode.maestro_id, instructions);

    if (!voice_ws_connect(g_ws)) {
        fprintf(stderr, "Failed to connect to voice service\n");
        voice_ws_destroy(g_ws);
        voice_audio_cleanup();
        return 1;
    }

    if (!voice_audio_start_playback()) {
        fprintf(stderr, "Failed to start audio playback\n");
        voice_ws_destroy(g_ws);
        voice_audio_cleanup();
        return 1;
    }

    if (!voice_audio_start_capture(on_audio_captured, NULL)) {
        fprintf(stderr, "Failed to start audio capture\n");
        voice_audio_stop_playback();
        voice_ws_destroy(g_ws);
        voice_audio_cleanup();
        return 1;
    }

    enter_raw_mode();
    g_voice_mode_running = true;
    g_voice_mode.active = true;
    render_ui();

    while (g_voice_mode_running) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) handle_keypress(c);
        usleep(10000);
    }

    exit_raw_mode();
    voice_audio_stop_capture();
    voice_audio_stop_playback();
    voice_ws_disconnect(g_ws);
    voice_ws_destroy(g_ws);
    voice_audio_cleanup();

    free(g_voice_mode.transcript_user);
    free(g_voice_mode.transcript_maestro);
    pthread_mutex_destroy(&g_voice_mode.state_mutex);

    printf("\nVoice mode ended.\n");
    return 0;
}

#else /* !CONVERGIO_VOICE_ENABLED */

int voice_mode_start(const char *maestro_id, const char *topic) {
    (void)maestro_id;
    (void)topic;
    fprintf(stderr, "\033[33mVoice mode not available. Rebuild with: make VOICE=1\033[0m\n");
    fprintf(stderr, "Dependency: brew install libwebsockets openssl\n");
    return 1;
}

#endif /* CONVERGIO_VOICE_ENABLED */
