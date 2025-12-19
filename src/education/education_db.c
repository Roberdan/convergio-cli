/**
 * CONVERGIO EDUCATION DATABASE
 *
 * SQLite-backed student profiles, learning progress, accessibility settings,
 * and toolkit outputs with thread-safe access.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/education.h"
#include "nous/debug_mutex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <sqlite3.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define EDU_DB_BUSY_TIMEOUT_MS 5000
#define EDU_DB_MAX_RETRIES 3
#define EDU_DB_RETRY_DELAY_MS 100

// SM-2 Algorithm Constants
#define SM2_MIN_EASINESS 1.3f
#define SM2_INITIAL_INTERVAL 1
#define SM2_SECOND_INTERVAL 6

// ============================================================================
// GLOBAL STATE
// ============================================================================

sqlite3* g_edu_db = NULL;  // Exported for feature modules
CONVERGIO_MUTEX_DECLARE(g_edu_db_mutex);
static bool g_edu_initialized = false;
static char g_edu_db_path[PATH_MAX] = {0};
static EducationStudentProfile* g_active_profile = NULL;

// ============================================================================
// DATABASE SCHEMA
// ============================================================================

static const char* EDUCATION_SCHEMA_SQL =
    "PRAGMA journal_mode=WAL;\n"
    "PRAGMA busy_timeout=5000;\n"
    "PRAGMA synchronous=NORMAL;\n"
    "PRAGMA foreign_keys=ON;\n"
    "\n"
    "-- =====================================================================\n"
    "-- STUDENT PROFILES TABLE (S10)\n"
    "-- Core student information and study preferences\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS student_profiles (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    name TEXT NOT NULL,\n"
    "    age INTEGER,\n"
    "    grade_level INTEGER CHECK(grade_level >= 1 AND grade_level <= 13),\n"
    "    curriculum_id TEXT,\n"
    "    parent_name TEXT,\n"
    "    parent_email TEXT,\n"
    "    preferred_language TEXT DEFAULT 'it',\n"
    "    study_method TEXT,\n"
    "    learning_style TEXT CHECK(learning_style IN ('visual', 'auditory', 'kinesthetic', 'reading', 'mixed')),\n"
    "    session_duration_preference INTEGER DEFAULT 25,\n"
    "    break_duration_preference INTEGER DEFAULT 5,\n"
    "    is_active INTEGER DEFAULT 1,\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    updated_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    last_session_at INTEGER\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- ACCESSIBILITY SETTINGS TABLE (S12)\n"
    "-- Detailed accessibility needs per student\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS student_accessibility (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    -- Dyslexia settings\n"
    "    dyslexia INTEGER DEFAULT 0,\n"
    "    dyslexia_severity INTEGER DEFAULT 0 CHECK(dyslexia_severity >= 0 AND dyslexia_severity <= 3),\n"
    "    use_dyslexic_font INTEGER DEFAULT 0,\n"
    "    line_spacing REAL DEFAULT 1.0,\n"
    "    max_chars_per_line INTEGER DEFAULT 80,\n"
    "    use_cream_background INTEGER DEFAULT 0,\n"
    "    syllable_highlighting INTEGER DEFAULT 0,\n"
    "    -- Dyscalculia settings\n"
    "    dyscalculia INTEGER DEFAULT 0,\n"
    "    dyscalculia_severity INTEGER DEFAULT 0,\n"
    "    use_color_coded_digits INTEGER DEFAULT 0,\n"
    "    use_visual_blocks INTEGER DEFAULT 0,\n"
    "    always_show_steps INTEGER DEFAULT 0,\n"
    "    disable_math_timer INTEGER DEFAULT 0,\n"
    "    -- Cerebral palsy settings\n"
    "    cerebral_palsy INTEGER DEFAULT 0,\n"
    "    cp_severity INTEGER DEFAULT 0,\n"
    "    voice_input_primary INTEGER DEFAULT 0,\n"
    "    extended_timeouts INTEGER DEFAULT 0,\n"
    "    large_click_areas INTEGER DEFAULT 0,\n"
    "    -- ADHD settings\n"
    "    adhd INTEGER DEFAULT 0,\n"
    "    adhd_type TEXT CHECK(adhd_type IN ('inattentive', 'hyperactive', 'combined', NULL)),\n"
    "    adhd_severity INTEGER DEFAULT 0 CHECK(adhd_severity >= 0 AND adhd_severity <= 3),\n"
    "    use_short_responses INTEGER DEFAULT 0,\n"
    "    show_progress_bar INTEGER DEFAULT 1,\n"
    "    use_micro_celebrations INTEGER DEFAULT 0,\n"
    "    distraction_parking INTEGER DEFAULT 0,\n"
    "    focus_mode_single_element INTEGER DEFAULT 0,\n"
    "    enhanced_gamification INTEGER DEFAULT 0,\n"
    "    -- Autism settings\n"
    "    autism INTEGER DEFAULT 0,\n"
    "    autism_severity INTEGER DEFAULT 0,\n"
    "    no_metaphors INTEGER DEFAULT 0,\n"
    "    predictable_structure INTEGER DEFAULT 0,\n"
    "    topic_change_warnings INTEGER DEFAULT 0,\n"
    "    allow_detailed_mode INTEGER DEFAULT 0,\n"
    "    no_social_pressure INTEGER DEFAULT 0,\n"
    "    -- General preferences\n"
    "    preferred_input TEXT DEFAULT 'keyboard' CHECK(preferred_input IN ('keyboard', 'voice', 'touch', 'switch', 'eye_tracking')),\n"
    "    preferred_output TEXT DEFAULT 'visual' CHECK(preferred_output IN ('visual', 'audio', 'braille', 'haptic')),\n"
    "    tts_enabled INTEGER DEFAULT 0,\n"
    "    tts_speed REAL DEFAULT 1.0 CHECK(tts_speed >= 0.5 AND tts_speed <= 2.0),\n"
    "    tts_voice TEXT,\n"
    "    high_contrast INTEGER DEFAULT 0,\n"
    "    font_size_multiplier REAL DEFAULT 1.0,\n"
    "    reduce_animations INTEGER DEFAULT 0,\n"
    "    reduce_sounds INTEGER DEFAULT 0,\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    updated_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    UNIQUE(student_id)\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- STUDENT GOALS TABLE (S13)\n"
    "-- Personal learning goals and objectives\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS student_goals (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    goal_type TEXT NOT NULL CHECK(goal_type IN ('short_term', 'medium_term', 'long_term', 'exam', 'personal')),\n"
    "    subject TEXT,\n"
    "    description TEXT NOT NULL,\n"
    "    target_date INTEGER,\n"
    "    progress_percent INTEGER DEFAULT 0 CHECK(progress_percent >= 0 AND progress_percent <= 100),\n"
    "    status TEXT DEFAULT 'active' CHECK(status IN ('active', 'completed', 'abandoned', 'on_hold')),\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    updated_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    completed_at INTEGER\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- LEARNING PROGRESS TABLE (S11)\n"
    "-- Tracks skill level per subject/topic over time\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS learning_progress (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    maestro_id TEXT NOT NULL,\n"
    "    subject TEXT NOT NULL,\n"
    "    topic TEXT NOT NULL,\n"
    "    subtopic TEXT,\n"
    "    skill_level REAL DEFAULT 0.0 CHECK(skill_level >= 0.0 AND skill_level <= 1.0),\n"
    "    confidence REAL DEFAULT 0.0 CHECK(confidence >= 0.0 AND confidence <= 1.0),\n"
    "    total_time_spent INTEGER DEFAULT 0,\n"
    "    interaction_count INTEGER DEFAULT 0,\n"
    "    quiz_score_avg REAL,\n"
    "    last_interaction INTEGER,\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    updated_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    UNIQUE(student_id, maestro_id, topic)\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- LEARNING SESSIONS TABLE (S14)\n"
    "-- Individual study sessions with Pomodoro tracking\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS learning_sessions (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    maestro_id TEXT,\n"
    "    session_type TEXT NOT NULL CHECK(session_type IN ('study', 'quiz', 'homework', 'review', 'flashcards', 'exploration')),\n"
    "    subject TEXT,\n"
    "    topic TEXT,\n"
    "    started_at INTEGER NOT NULL DEFAULT (strftime('%s','now')),\n"
    "    ended_at INTEGER,\n"
    "    duration_seconds INTEGER,\n"
    "    pomodoro_count INTEGER DEFAULT 0,\n"
    "    breaks_taken INTEGER DEFAULT 0,\n"
    "    focus_score REAL CHECK(focus_score >= 0.0 AND focus_score <= 1.0),\n"
    "    notes TEXT,\n"
    "    xp_earned INTEGER DEFAULT 0,\n"
    "    completed INTEGER DEFAULT 0\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- TOOLKIT OUTPUTS TABLE (S15)\n"
    "-- Saved mind maps, quizzes, audio files, etc.\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS toolkit_outputs (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    output_type TEXT NOT NULL CHECK(output_type IN ('mindmap', 'quiz', 'flashcard_deck', 'audio', 'summary', 'formula', 'graph', 'flowchart', 'timeline')),\n"
    "    subject TEXT,\n"
    "    topic TEXT NOT NULL,\n"
    "    title TEXT,\n"
    "    content TEXT NOT NULL,\n"
    "    format TEXT CHECK(format IN ('svg', 'png', 'pdf', 'm4a', 'mp3', 'json', 'md', 'html', 'mermaid')),\n"
    "    file_path TEXT,\n"
    "    is_favorite INTEGER DEFAULT 0,\n"
    "    view_count INTEGER DEFAULT 0,\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    updated_at INTEGER DEFAULT (strftime('%s','now'))\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- FLASHCARD DECKS TABLE\n"
    "-- Collections of flashcards for spaced repetition\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS flashcard_decks (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    subject TEXT,\n"
    "    topic TEXT NOT NULL,\n"
    "    title TEXT NOT NULL,\n"
    "    description TEXT,\n"
    "    card_count INTEGER DEFAULT 0,\n"
    "    mastered_count INTEGER DEFAULT 0,\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    updated_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    last_reviewed_at INTEGER\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- FLASHCARD REVIEWS TABLE\n"
    "-- SM-2 Spaced Repetition tracking per card\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS flashcard_reviews (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    deck_id INTEGER NOT NULL REFERENCES flashcard_decks(id) ON DELETE CASCADE,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    front TEXT NOT NULL,\n"
    "    back TEXT NOT NULL,\n"
    "    hint TEXT,\n"
    "    mnemonic TEXT,\n"
    "    -- SM-2 algorithm fields\n"
    "    easiness_factor REAL DEFAULT 2.5 CHECK(easiness_factor >= 1.3),\n"
    "    interval_days INTEGER DEFAULT 0,\n"
    "    repetition_count INTEGER DEFAULT 0,\n"
    "    next_review_at INTEGER,\n"
    "    last_review_at INTEGER,\n"
    "    last_quality INTEGER CHECK(last_quality >= 0 AND last_quality <= 5),\n"
    "    -- Status\n"
    "    status TEXT DEFAULT 'new' CHECK(status IN ('new', 'learning', 'reviewing', 'mastered', 'suspended')),\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now'))\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- QUIZ HISTORY TABLE\n"
    "-- Records of all quizzes taken\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS quiz_history (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    subject TEXT NOT NULL,\n"
    "    topic TEXT NOT NULL,\n"
    "    quiz_type TEXT CHECK(quiz_type IN ('multiple_choice', 'true_false', 'open', 'sequence', 'matching', 'cloze', 'image_identify')),\n"
    "    question_count INTEGER NOT NULL,\n"
    "    correct_count INTEGER NOT NULL,\n"
    "    score_percent REAL NOT NULL,\n"
    "    time_taken_seconds INTEGER,\n"
    "    difficulty_level TEXT CHECK(difficulty_level IN ('easy', 'medium', 'hard', 'adaptive')),\n"
    "    questions_json TEXT,\n"
    "    answers_json TEXT,\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now'))\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- GAMIFICATION TABLE\n"
    "-- XP, levels, badges, streaks\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS gamification (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    total_xp INTEGER DEFAULT 0,\n"
    "    current_level INTEGER DEFAULT 1,\n"
    "    current_streak INTEGER DEFAULT 0,\n"
    "    longest_streak INTEGER DEFAULT 0,\n"
    "    last_activity_date TEXT,\n"
    "    badges_json TEXT DEFAULT '[]',\n"
    "    achievements_json TEXT DEFAULT '[]',\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    updated_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    UNIQUE(student_id)\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- CURRICULUM PROGRESS TABLE\n"
    "-- Tracks completion of curriculum units\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS curriculum_progress (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    curriculum_id TEXT NOT NULL,\n"
    "    year INTEGER NOT NULL,\n"
    "    subject TEXT NOT NULL,\n"
    "    unit_id TEXT NOT NULL,\n"
    "    unit_title TEXT,\n"
    "    status TEXT DEFAULT 'not_started' CHECK(status IN ('not_started', 'in_progress', 'completed', 'skipped')),\n"
    "    completion_percent INTEGER DEFAULT 0,\n"
    "    started_at INTEGER,\n"
    "    completed_at INTEGER,\n"
    "    UNIQUE(student_id, curriculum_id, year, unit_id)\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- INBOX TABLE\n"
    "-- Quick capture for thoughts, ideas, homework notes\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS inbox (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    content TEXT NOT NULL,\n"
    "    source TEXT DEFAULT 'cli' CHECK(source IN ('cli', 'voice', 'agent', 'reminder')),\n"
    "    processed INTEGER DEFAULT 0,\n"
    "    processed_to_task_id INTEGER,\n"
    "    created_at INTEGER DEFAULT (strftime('%s','now'))\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- INDEXES FOR PERFORMANCE\n"
    "-- =====================================================================\n"
    "CREATE INDEX IF NOT EXISTS idx_progress_student ON learning_progress(student_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_progress_topic ON learning_progress(topic);\n"
    "CREATE INDEX IF NOT EXISTS idx_progress_maestro ON learning_progress(maestro_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_sessions_student ON learning_sessions(student_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_sessions_date ON learning_sessions(started_at);\n"
    "CREATE INDEX IF NOT EXISTS idx_toolkit_student ON toolkit_outputs(student_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_toolkit_type ON toolkit_outputs(output_type);\n"
    "CREATE INDEX IF NOT EXISTS idx_flashcard_next ON flashcard_reviews(next_review_at);\n"
    "CREATE INDEX IF NOT EXISTS idx_flashcard_status ON flashcard_reviews(status);\n"
    "CREATE INDEX IF NOT EXISTS idx_goals_student ON student_goals(student_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_goals_status ON student_goals(status);\n"
    "CREATE INDEX IF NOT EXISTS idx_quiz_student ON quiz_history(student_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_quiz_topic ON quiz_history(topic);\n"
    "CREATE INDEX IF NOT EXISTS idx_curriculum_student ON curriculum_progress(student_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_inbox_unprocessed ON inbox(student_id, processed);\n"
    "\n"
    "-- =====================================================================\n"
    "-- HOMEWORK LOGS TABLE (F05)\n"
    "-- Transparent logging of homework assistance for parents\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS homework_logs (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    subject TEXT NOT NULL,\n"
    "    topic TEXT,\n"
    "    question TEXT NOT NULL,\n"
    "    guidance_provided TEXT,\n"
    "    hints_used INTEGER DEFAULT 0,\n"
    "    timestamp INTEGER NOT NULL,\n"
    "    verification_completed INTEGER DEFAULT 0\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- SUBJECT TIME TRACKING TABLE (F10)\n"
    "-- Track time spent per subject for analytics\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS subject_time_tracking (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    subject TEXT NOT NULL,\n"
    "    total_minutes INTEGER DEFAULT 0,\n"
    "    last_studied INTEGER,\n"
    "    UNIQUE(student_id, subject)\n"
    ");\n"
    "\n"
    "CREATE INDEX IF NOT EXISTS idx_homework_student ON homework_logs(student_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_homework_timestamp ON homework_logs(timestamp);\n"
    "CREATE INDEX IF NOT EXISTS idx_time_tracking_student ON subject_time_tracking(student_id);\n"
    "\n"
    "-- =====================================================================\n"
    "-- FTS5 FULL-TEXT SEARCH\n"
    "-- =====================================================================\n"
    "CREATE VIRTUAL TABLE IF NOT EXISTS toolkit_fts USING fts5(\n"
    "    title, topic, content,\n"
    "    content='toolkit_outputs',\n"
    "    content_rowid='id'\n"
    ");\n"
    "\n"
    "CREATE VIRTUAL TABLE IF NOT EXISTS flashcard_fts USING fts5(\n"
    "    front, back, hint, mnemonic,\n"
    "    content='flashcard_reviews',\n"
    "    content_rowid='id'\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- TRIGGERS FOR FTS SYNC\n"
    "-- =====================================================================\n"
    "CREATE TRIGGER IF NOT EXISTS toolkit_ai AFTER INSERT ON toolkit_outputs BEGIN\n"
    "    INSERT INTO toolkit_fts(rowid, title, topic, content) VALUES (NEW.id, NEW.title, NEW.topic, NEW.content);\n"
    "END;\n"
    "\n"
    "CREATE TRIGGER IF NOT EXISTS toolkit_ad AFTER DELETE ON toolkit_outputs BEGIN\n"
    "    INSERT INTO toolkit_fts(toolkit_fts, rowid, title, topic, content) VALUES ('delete', OLD.id, OLD.title, OLD.topic, OLD.content);\n"
    "END;\n"
    "\n"
    "CREATE TRIGGER IF NOT EXISTS toolkit_au AFTER UPDATE ON toolkit_outputs BEGIN\n"
    "    INSERT INTO toolkit_fts(toolkit_fts, rowid, title, topic, content) VALUES ('delete', OLD.id, OLD.title, OLD.topic, OLD.content);\n"
    "    INSERT INTO toolkit_fts(rowid, title, topic, content) VALUES (NEW.id, NEW.title, NEW.topic, NEW.content);\n"
    "END;\n"
    "\n"
    "CREATE TRIGGER IF NOT EXISTS flashcard_ai AFTER INSERT ON flashcard_reviews BEGIN\n"
    "    INSERT INTO flashcard_fts(rowid, front, back, hint, mnemonic) VALUES (NEW.id, NEW.front, NEW.back, NEW.hint, NEW.mnemonic);\n"
    "END;\n"
    "\n"
    "CREATE TRIGGER IF NOT EXISTS flashcard_ad AFTER DELETE ON flashcard_reviews BEGIN\n"
    "    INSERT INTO flashcard_fts(flashcard_fts, rowid, front, back, hint, mnemonic) VALUES ('delete', OLD.id, OLD.front, OLD.back, OLD.hint, OLD.mnemonic);\n"
    "END;\n"
    "\n"
    "-- =====================================================================\n"
    "-- UPDATE TIMESTAMP TRIGGERS\n"
    "-- =====================================================================\n"
    "CREATE TRIGGER IF NOT EXISTS update_profile_timestamp \n"
    "AFTER UPDATE ON student_profiles\n"
    "BEGIN\n"
    "    UPDATE student_profiles SET updated_at = strftime('%s','now') WHERE id = NEW.id;\n"
    "END;\n"
    "\n"
    "CREATE TRIGGER IF NOT EXISTS update_accessibility_timestamp \n"
    "AFTER UPDATE ON student_accessibility\n"
    "BEGIN\n"
    "    UPDATE student_accessibility SET updated_at = strftime('%s','now') WHERE id = NEW.id;\n"
    "END;\n"
    "\n"
    "CREATE TRIGGER IF NOT EXISTS update_progress_timestamp \n"
    "AFTER UPDATE ON learning_progress\n"
    "BEGIN\n"
    "    UPDATE learning_progress SET updated_at = strftime('%s','now') WHERE id = NEW.id;\n"
    "END;\n"
    "\n"
    "CREATE TRIGGER IF NOT EXISTS update_goals_timestamp \n"
    "AFTER UPDATE ON student_goals\n"
    "BEGIN\n"
    "    UPDATE student_goals SET updated_at = strftime('%s','now') WHERE id = NEW.id;\n"
    "END;\n";

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static char* safe_strdup(const char* str) {
    return str ? strdup(str) : NULL;
}

static const char* severity_to_string(EducationSeverity severity) {
    switch (severity) {
        case SEVERITY_MILD: return "mild";
        case SEVERITY_MODERATE: return "moderate";
        case SEVERITY_SEVERE: return "severe";
        default: return "none";
    }
}

static EducationSeverity string_to_severity(int level) {
    switch (level) {
        case 1: return SEVERITY_MILD;
        case 2: return SEVERITY_MODERATE;
        case 3: return SEVERITY_SEVERE;
        default: return SEVERITY_NONE;
    }
}

static const char* input_method_to_string(EducationInputMethod method) {
    switch (method) {
        case INPUT_VOICE: return "voice";
        case INPUT_TOUCH: return "touch";
        case INPUT_SWITCH: return "switch";
        case INPUT_EYE_TRACKING: return "eye_tracking";
        default: return "keyboard";
    }
}

static EducationInputMethod string_to_input_method(const char* str) {
    if (!str) return INPUT_KEYBOARD;
    if (strcmp(str, "voice") == 0) return INPUT_VOICE;
    if (strcmp(str, "touch") == 0) return INPUT_TOUCH;
    if (strcmp(str, "switch") == 0) return INPUT_SWITCH;
    if (strcmp(str, "eye_tracking") == 0) return INPUT_EYE_TRACKING;
    return INPUT_KEYBOARD;
}

static const char* output_method_to_string(EducationOutputMethod method) {
    switch (method) {
        case OUTPUT_AUDIO: return "audio";
        case OUTPUT_BRAILLE: return "braille";
        case OUTPUT_HAPTIC: return "haptic";
        default: return "visual";
    }
}

static EducationOutputMethod string_to_output_method(const char* str) {
    if (!str) return OUTPUT_VISUAL;
    if (strcmp(str, "audio") == 0) return OUTPUT_AUDIO;
    if (strcmp(str, "braille") == 0) return OUTPUT_BRAILLE;
    if (strcmp(str, "haptic") == 0) return OUTPUT_HAPTIC;
    return OUTPUT_VISUAL;
}

static const char* adhd_type_to_string(EducationAdhdType type) {
    switch (type) {
        case ADHD_INATTENTIVE: return "inattentive";
        case ADHD_HYPERACTIVE: return "hyperactive";
        case ADHD_COMBINED: return "combined";
        default: return NULL;
    }
}

static EducationAdhdType string_to_adhd_type(const char* str) {
    if (!str) return ADHD_NONE;
    if (strcmp(str, "inattentive") == 0) return ADHD_INATTENTIVE;
    if (strcmp(str, "hyperactive") == 0) return ADHD_HYPERACTIVE;
    if (strcmp(str, "combined") == 0) return ADHD_COMBINED;
    return ADHD_NONE;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

int education_init(void) {
    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    if (g_edu_initialized) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return 0;
    }

    // Determine database path
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(g_edu_db_path, sizeof(g_edu_db_path), "%s/.convergio/education.db", home);

    // Create directory if needed
    char dir_path[PATH_MAX];
    snprintf(dir_path, sizeof(dir_path), "%s/.convergio", home);
    mkdir(dir_path, 0755);

    // Open database
    int rc = sqlite3_open_v2(g_edu_db_path, &g_edu_db,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                             NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[education] Failed to open database: %s\n", sqlite3_errmsg(g_edu_db));
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    // Apply schema
    char* err_msg = NULL;
    rc = sqlite3_exec(g_edu_db, EDUCATION_SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[education] Schema error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(g_edu_db);
        g_edu_db = NULL;
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    g_edu_initialized = true;
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return 0;
}

void education_shutdown(void) {
    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    if (g_active_profile) {
        education_profile_free(g_active_profile);
        g_active_profile = NULL;
    }

    if (g_edu_db) {
        sqlite3_close(g_edu_db);
        g_edu_db = NULL;
    }

    g_edu_initialized = false;
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
}

bool education_is_initialized(void) {
    return g_edu_initialized;
}

// ============================================================================
// PROFILE MANAGEMENT (S16)
// ============================================================================

int64_t education_profile_create(const EducationCreateOptions* options) {
    if (!g_edu_initialized || !options || !options->name) {
        return -1;
    }

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Insert profile
    const char* sql =
        "INSERT INTO student_profiles (name, age, grade_level, curriculum_id, parent_name, parent_email, learning_style) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, options->name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, options->age);
    sqlite3_bind_int(stmt, 3, options->grade_level);
    if (options->curriculum_id) {
        sqlite3_bind_text(stmt, 4, options->curriculum_id, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 4);
    }
    if (options->parent_name) {
        sqlite3_bind_text(stmt, 5, options->parent_name, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 5);
    }
    if (options->parent_email) {
        sqlite3_bind_text(stmt, 6, options->parent_email, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 6);
    }
    sqlite3_bind_text(stmt, 7, "mixed", -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    int64_t student_id = sqlite3_last_insert_rowid(g_edu_db);

    // Create accessibility settings (with provided values if available)
    if (options->accessibility) {
        const EducationAccessibility* a = options->accessibility;
        sql = "INSERT INTO student_accessibility ("
              "student_id, dyslexia, dyslexia_severity, dyscalculia, dyscalculia_severity, "
              "cerebral_palsy, cp_severity, adhd, adhd_type, adhd_severity, "
              "autism, autism_severity, preferred_input, preferred_output, "
              "tts_enabled, tts_speed, high_contrast, reduce_animations"
              ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, student_id);
            sqlite3_bind_int(stmt, 2, a->dyslexia ? 1 : 0);
            sqlite3_bind_int(stmt, 3, (int)a->dyslexia_severity);
            sqlite3_bind_int(stmt, 4, a->dyscalculia ? 1 : 0);
            sqlite3_bind_int(stmt, 5, (int)a->dyscalculia_severity);
            sqlite3_bind_int(stmt, 6, a->cerebral_palsy ? 1 : 0);
            sqlite3_bind_int(stmt, 7, (int)a->cerebral_palsy_severity);
            sqlite3_bind_int(stmt, 8, a->adhd ? 1 : 0);
            const char* adhd_type_str = adhd_type_to_string(a->adhd_type);
            if (adhd_type_str) {
                sqlite3_bind_text(stmt, 9, adhd_type_str, -1, SQLITE_STATIC);
            } else {
                sqlite3_bind_null(stmt, 9);
            }
            sqlite3_bind_int(stmt, 10, (int)a->adhd_severity);
            sqlite3_bind_int(stmt, 11, a->autism ? 1 : 0);
            sqlite3_bind_int(stmt, 12, (int)a->autism_severity);
            const char* input_str = input_method_to_string(a->preferred_input);
            sqlite3_bind_text(stmt, 13, input_str ? input_str : "keyboard", -1, SQLITE_STATIC);
            const char* output_str = output_method_to_string(a->preferred_output);
            sqlite3_bind_text(stmt, 14, output_str ? output_str : "visual", -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 15, a->tts_enabled ? 1 : 0);
            sqlite3_bind_double(stmt, 16, a->tts_speed > 0 ? a->tts_speed : 1.0);
            sqlite3_bind_int(stmt, 17, a->high_contrast ? 1 : 0);
            sqlite3_bind_int(stmt, 18, a->reduce_motion ? 1 : 0);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    } else {
        // Create default accessibility settings
        sql = "INSERT INTO student_accessibility (student_id) VALUES (?)";
        rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, student_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    // Create default gamification entry
    sql = "INSERT INTO gamification (student_id) VALUES (?)";
    rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return student_id;
}

EducationStudentProfile* education_profile_get(int64_t student_id) {
    if (!g_edu_initialized) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "SELECT p.id, p.name, p.age, p.grade_level, p.curriculum_id, p.parent_name, "
        "p.parent_email, p.preferred_language, p.study_method, p.learning_style, "
        "p.session_duration_preference, p.break_duration_preference, p.is_active, "
        "p.created_at, p.updated_at, p.last_session_at "
        "FROM student_profiles p WHERE p.id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);

    EducationStudentProfile* profile = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        profile = calloc(1, sizeof(EducationStudentProfile));
        if (profile) {
            profile->id = sqlite3_column_int64(stmt, 0);
            profile->name = safe_strdup((const char*)sqlite3_column_text(stmt, 1));
            profile->age = sqlite3_column_int(stmt, 2);
            profile->grade_level = sqlite3_column_int(stmt, 3);
            profile->curriculum_id = safe_strdup((const char*)sqlite3_column_text(stmt, 4));
            profile->parent_name = safe_strdup((const char*)sqlite3_column_text(stmt, 5));
            profile->parent_email = safe_strdup((const char*)sqlite3_column_text(stmt, 6));
            profile->preferred_language = safe_strdup((const char*)sqlite3_column_text(stmt, 7));
            profile->study_method = safe_strdup((const char*)sqlite3_column_text(stmt, 8));
            // Initialize accessibility with defaults
            profile->accessibility = calloc(1, sizeof(EducationAccessibility));
            profile->is_active = sqlite3_column_int(stmt, 12);
            profile->created_at = sqlite3_column_int64(stmt, 13);
            profile->updated_at = sqlite3_column_int64(stmt, 14);
            profile->last_session_at = sqlite3_column_int64(stmt, 15);
        }
    }
    sqlite3_finalize(stmt);

    // Load accessibility settings
    if (profile && profile->accessibility) {
        sql = "SELECT * FROM student_accessibility WHERE student_id = ?";
        rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, student_id);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                EducationAccessibility* a = profile->accessibility;
                // Load all fields (column indices based on schema order)
                // Schema: id=0, student_id=1, dyslexia=2, dyslexia_severity=3, ...
                // adhd_type=21, adhd_severity=22, autism=29, autism_severity=30
                // preferred_input=36, preferred_output=37, tts_enabled=38, tts_speed=39
                // high_contrast=41, reduce_animations=43
                a->dyslexia = sqlite3_column_int(stmt, 2);
                a->dyslexia_severity = string_to_severity(sqlite3_column_int(stmt, 3));
                a->dyscalculia = sqlite3_column_int(stmt, 9);
                a->dyscalculia_severity = string_to_severity(sqlite3_column_int(stmt, 10));
                a->cerebral_palsy = sqlite3_column_int(stmt, 15);
                a->cerebral_palsy_severity = string_to_severity(sqlite3_column_int(stmt, 16));
                a->adhd = sqlite3_column_int(stmt, 20);
                a->adhd_type = string_to_adhd_type((const char*)sqlite3_column_text(stmt, 21));
                a->adhd_severity = string_to_severity(sqlite3_column_int(stmt, 22));
                a->autism = sqlite3_column_int(stmt, 29);
                a->autism_severity = string_to_severity(sqlite3_column_int(stmt, 30));
                a->preferred_input = string_to_input_method((const char*)sqlite3_column_text(stmt, 36));
                a->preferred_output = string_to_output_method((const char*)sqlite3_column_text(stmt, 37));
                a->tts_enabled = sqlite3_column_int(stmt, 38);
                a->tts_speed = (float)sqlite3_column_double(stmt, 39);
                a->high_contrast = sqlite3_column_int(stmt, 41);
                a->reduce_motion = sqlite3_column_int(stmt, 43);
            }
            sqlite3_finalize(stmt);
        }
    }

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return profile;
}

EducationStudentProfile* education_profile_get_active(void) {
    return g_active_profile;
}

int education_profile_set_active(int64_t student_id) {
    if (!g_edu_initialized) return -1;

    // Free existing active profile
    if (g_active_profile) {
        education_profile_free(g_active_profile);
        g_active_profile = NULL;
    }

    // Load new profile
    g_active_profile = education_profile_get(student_id);
    return g_active_profile ? 0 : -1;
}

int education_profile_update(int64_t student_id, const EducationUpdateOptions* options) {
    if (!g_edu_initialized || !options) return -1;

    // TODO: Implement dynamic update based on options
    (void)student_id;  // Will be used in implementation
    return 0;
}

int education_profile_delete(int64_t student_id) {
    if (!g_edu_initialized) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, "DELETE FROM student_profiles WHERE id = ?", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

void education_profile_free(EducationStudentProfile* profile) {
    if (!profile) return;
    free(profile->name);
    free(profile->curriculum_id);
    free(profile->parent_name);
    free(profile->parent_email);
    free(profile->preferred_language);
    free(profile->study_method);
    free(profile->accessibility);
    free(profile);
}

// ============================================================================
// ACCESSIBILITY MANAGEMENT (S16)
// ============================================================================

int education_accessibility_update(int64_t student_id, const EducationAccessibility* settings) {
    if (!g_edu_initialized || !settings) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "UPDATE student_accessibility SET "
        "dyslexia = ?, dyslexia_severity = ?, "
        "dyscalculia = ?, dyscalculia_severity = ?, "
        "cerebral_palsy = ?, cp_severity = ?, "
        "adhd = ?, adhd_type = ?, "
        "autism = ?, autism_severity = ?, "
        "preferred_input = ?, preferred_output = ?, "
        "tts_enabled = ?, tts_speed = ? "
        "WHERE student_id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, settings->dyslexia);
    sqlite3_bind_int(stmt, 2, (int)settings->dyslexia_severity);
    sqlite3_bind_int(stmt, 3, settings->dyscalculia);
    sqlite3_bind_int(stmt, 4, (int)settings->dyscalculia_severity);
    sqlite3_bind_int(stmt, 5, settings->cerebral_palsy);
    sqlite3_bind_int(stmt, 6, (int)settings->cerebral_palsy_severity);
    sqlite3_bind_int(stmt, 7, settings->adhd);
    sqlite3_bind_text(stmt, 8, adhd_type_to_string(settings->adhd_type), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 9, settings->autism);
    sqlite3_bind_int(stmt, 10, (int)settings->autism_severity);
    sqlite3_bind_text(stmt, 11, input_method_to_string(settings->preferred_input), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, output_method_to_string(settings->preferred_output), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 13, settings->tts_enabled);
    sqlite3_bind_double(stmt, 14, settings->tts_speed);
    sqlite3_bind_int64(stmt, 15, student_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

EducationAccessibility* education_accessibility_get(int64_t student_id) {
    EducationStudentProfile* profile = education_profile_get(student_id);
    if (!profile) return NULL;

    EducationAccessibility* settings = profile->accessibility;
    profile->accessibility = NULL; // Prevent double-free
    education_profile_free(profile);

    return settings;
}

// ============================================================================
// LEARNING PROGRESS (S17, S18)
// ============================================================================

int education_progress_record(int64_t student_id, const char* maestro_id,
                              const char* topic, float skill_level, int time_spent) {
    if (!g_edu_initialized || !maestro_id || !topic) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Upsert progress
    const char* sql =
        "INSERT INTO learning_progress (student_id, maestro_id, subject, topic, skill_level, total_time_spent, interaction_count, last_interaction) "
        "VALUES (?, ?, ?, ?, ?, ?, 1, strftime('%s','now')) "
        "ON CONFLICT(student_id, maestro_id, topic) DO UPDATE SET "
        "skill_level = ?, total_time_spent = total_time_spent + ?, "
        "interaction_count = interaction_count + 1, last_interaction = strftime('%s','now')";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    // Extract subject from maestro_id (e.g., "socrate-filosofia" -> "filosofia")
    const char* subject = strrchr(maestro_id, '-');
    subject = subject ? subject + 1 : maestro_id;

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, maestro_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, subject, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, topic, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 5, skill_level);
    sqlite3_bind_int(stmt, 6, time_spent);
    sqlite3_bind_double(stmt, 7, skill_level);
    sqlite3_bind_int(stmt, 8, time_spent);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

EducationProgress* education_progress_get(int64_t student_id, const char* topic) {
    if (!g_edu_initialized || !topic) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "SELECT id, maestro_id, subject, topic, skill_level, confidence, "
        "total_time_spent, interaction_count, quiz_score_avg, last_interaction "
        "FROM learning_progress WHERE student_id = ? AND topic = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, topic, -1, SQLITE_STATIC);

    EducationProgress* progress = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        progress = calloc(1, sizeof(EducationProgress));
        if (progress) {
            progress->id = sqlite3_column_int64(stmt, 0);
            progress->student_id = student_id;
            progress->maestro_id = safe_strdup((const char*)sqlite3_column_text(stmt, 1));
            progress->subject = safe_strdup((const char*)sqlite3_column_text(stmt, 2));
            progress->topic = safe_strdup((const char*)sqlite3_column_text(stmt, 3));
            progress->skill_level = (float)sqlite3_column_double(stmt, 4);
            progress->confidence = (float)sqlite3_column_double(stmt, 5);
            progress->total_time_spent = sqlite3_column_int(stmt, 6);
            progress->interaction_count = sqlite3_column_int(stmt, 7);
            progress->quiz_score_avg = (float)sqlite3_column_double(stmt, 8);
            progress->last_interaction = sqlite3_column_int64(stmt, 9);
        }
    }
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return progress;
}

void education_progress_free(EducationProgress* progress) {
    if (!progress) return;
    free(progress->maestro_id);
    free(progress->subject);
    free(progress->topic);
    free(progress->subtopic);
    free(progress);
}

// ============================================================================
// TOOLKIT OUTPUTS
// ============================================================================

int64_t education_toolkit_save(int64_t student_id, EducationToolkitType type,
                               const char* topic, const char* content, const char* format) {
    if (!g_edu_initialized || !topic || !content) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* type_str;
    switch (type) {
        case TOOLKIT_MINDMAP: type_str = "mindmap"; break;
        case TOOLKIT_QUIZ: type_str = "quiz"; break;
        case TOOLKIT_FLASHCARD: type_str = "flashcard_deck"; break;
        case TOOLKIT_AUDIO: type_str = "audio"; break;
        case TOOLKIT_SUMMARY: type_str = "summary"; break;
        case TOOLKIT_FORMULA: type_str = "formula"; break;
        case TOOLKIT_GRAPH: type_str = "graph"; break;
        case TOOLKIT_FLOWCHART: type_str = "flowchart"; break;
        case TOOLKIT_TIMELINE: type_str = "timeline"; break;
        default: type_str = "summary"; break;
    }

    const char* sql =
        "INSERT INTO toolkit_outputs (student_id, output_type, topic, content, format) "
        "VALUES (?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, type_str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, topic, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, content, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, format, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int64_t output_id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(g_edu_db) : -1;
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return output_id;
}

// ============================================================================
// FLASHCARD SM-2 ALGORITHM
// ============================================================================

int education_flashcard_review(int64_t review_id, int quality) {
    if (!g_edu_initialized || quality < 0 || quality > 5) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Get current card state
    const char* sql = "SELECT easiness_factor, interval_days, repetition_count FROM flashcard_reviews WHERE id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, review_id);

    float ef = 2.5f;
    int interval = 0;
    int reps = 0;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ef = (float)sqlite3_column_double(stmt, 0);
        interval = sqlite3_column_int(stmt, 1);
        reps = sqlite3_column_int(stmt, 2);
    }
    sqlite3_finalize(stmt);

    // SM-2 Algorithm
    if (quality >= 3) {
        // Correct response
        if (reps == 0) {
            interval = SM2_INITIAL_INTERVAL;
        } else if (reps == 1) {
            interval = SM2_SECOND_INTERVAL;
        } else {
            interval = (int)(interval * ef);
        }
        reps++;
    } else {
        // Incorrect response - reset
        reps = 0;
        interval = SM2_INITIAL_INTERVAL;
    }

    // Update easiness factor
    ef = ef + (0.1f - (5 - quality) * (0.08f + (5 - quality) * 0.02f));
    if (ef < SM2_MIN_EASINESS) ef = SM2_MIN_EASINESS;

    // Calculate next review timestamp
    time_t now = time(NULL);
    time_t next_review = now + (interval * 24 * 60 * 60);

    // Determine status
    const char* status;
    if (reps == 0) status = "learning";
    else if (interval >= 21) status = "mastered";
    else status = "reviewing";

    // Update database
    sql = "UPDATE flashcard_reviews SET "
          "easiness_factor = ?, interval_days = ?, repetition_count = ?, "
          "next_review_at = ?, last_review_at = ?, last_quality = ?, status = ? "
          "WHERE id = ?";

    rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_double(stmt, 1, ef);
    sqlite3_bind_int(stmt, 2, interval);
    sqlite3_bind_int(stmt, 3, reps);
    sqlite3_bind_int64(stmt, 4, next_review);
    sqlite3_bind_int64(stmt, 5, now);
    sqlite3_bind_int(stmt, 6, quality);
    sqlite3_bind_text(stmt, 7, status, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 8, review_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ============================================================================
// SESSION MANAGEMENT
// ============================================================================

int64_t education_session_start(int64_t student_id, const char* session_type,
                                const char* subject, const char* topic) {
    if (!g_edu_initialized || !session_type) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "INSERT INTO learning_sessions (student_id, session_type, subject, topic) "
        "VALUES (?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, session_type, -1, SQLITE_STATIC);
    if (subject) sqlite3_bind_text(stmt, 3, subject, -1, SQLITE_STATIC);
    else sqlite3_bind_null(stmt, 3);
    if (topic) sqlite3_bind_text(stmt, 4, topic, -1, SQLITE_STATIC);
    else sqlite3_bind_null(stmt, 4);

    rc = sqlite3_step(stmt);
    int64_t session_id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(g_edu_db) : -1;
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return session_id;
}

int education_session_end(int64_t session_id, int xp_earned) {
    if (!g_edu_initialized) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "UPDATE learning_sessions SET "
        "ended_at = strftime('%s','now'), "
        "duration_seconds = strftime('%s','now') - started_at, "
        "completed = 1, xp_earned = ? "
        "WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, xp_earned);
    sqlite3_bind_int64(stmt, 2, session_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ============================================================================
// GAMIFICATION
// ============================================================================

int education_xp_add(int64_t student_id, int xp_amount, const char* reason) {
    if (!g_edu_initialized || xp_amount <= 0) return -1;

    (void)reason;  // Reserved for future XP history logging
    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Update XP and check for level up
    const char* sql =
        "UPDATE gamification SET "
        "total_xp = total_xp + ?, "
        "current_level = (total_xp + ?) / 1000 + 1 "
        "WHERE student_id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, xp_amount);
    sqlite3_bind_int(stmt, 2, xp_amount);
    sqlite3_bind_int64(stmt, 3, student_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int education_streak_update(int64_t student_id) {
    if (!g_edu_initialized) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Get current streak info
    const char* sql = "SELECT current_streak, longest_streak, last_activity_date FROM gamification WHERE student_id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);

    int current_streak = 0;
    int longest_streak = 0;
    const char* last_date = NULL;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        current_streak = sqlite3_column_int(stmt, 0);
        longest_streak = sqlite3_column_int(stmt, 1);
        last_date = (const char*)sqlite3_column_text(stmt, 2);
    }
    sqlite3_finalize(stmt);

    // Get today's date
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    char today[11];
    strftime(today, sizeof(today), "%Y-%m-%d", tm_now);

    // Update streak
    if (last_date && strcmp(last_date, today) == 0) {
        // Same day, no change
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return current_streak;
    }

    // Check if yesterday
    time_t yesterday = now - 86400;
    struct tm* tm_yesterday = localtime(&yesterday);
    char yesterday_str[11];
    strftime(yesterday_str, sizeof(yesterday_str), "%Y-%m-%d", tm_yesterday);

    if (last_date && strcmp(last_date, yesterday_str) == 0) {
        current_streak++;
    } else {
        current_streak = 1;
    }

    if (current_streak > longest_streak) {
        longest_streak = current_streak;
    }

    // Update database
    sql = "UPDATE gamification SET current_streak = ?, longest_streak = ?, last_activity_date = ? WHERE student_id = ?";
    rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, current_streak);
        sqlite3_bind_int(stmt, 2, longest_streak);
        sqlite3_bind_text(stmt, 3, today, -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 4, student_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return current_streak;
}

// ============================================================================
// PROFILE BROADCAST (S17)
// ============================================================================

char* education_profile_to_json(const EducationStudentProfile* profile) {
    if (!profile) return NULL;

    // Build JSON string (simplified)
    size_t buf_size = 4096;
    char* json = malloc(buf_size);
    if (!json) return NULL;

    EducationAccessibility* a = profile->accessibility;

    snprintf(json, buf_size,
        "{"
        "\"id\":%" PRId64 ","
        "\"name\":\"%s\","
        "\"age\":%d,"
        "\"grade_level\":%d,"
        "\"curriculum_id\":\"%s\","
        "\"accessibility\":{"
        "\"dyslexia\":%s,"
        "\"dyslexia_severity\":\"%s\","
        "\"dyscalculia\":%s,"
        "\"cerebral_palsy\":%s,"
        "\"adhd\":%s,"
        "\"adhd_type\":\"%s\","
        "\"autism\":%s,"
        "\"preferred_input\":\"%s\","
        "\"preferred_output\":\"%s\","
        "\"tts_enabled\":%s,"
        "\"tts_speed\":%.2f"
        "}"
        "}",
        profile->id,
        profile->name ? profile->name : "",
        profile->age,
        profile->grade_level,
        profile->curriculum_id ? profile->curriculum_id : "",
        a && a->dyslexia ? "true" : "false",
        a ? severity_to_string(a->dyslexia_severity) : "none",
        a && a->dyscalculia ? "true" : "false",
        a && a->cerebral_palsy ? "true" : "false",
        a && a->adhd ? "true" : "false",
        a ? (adhd_type_to_string(a->adhd_type) ? adhd_type_to_string(a->adhd_type) : "none") : "none",
        a && a->autism ? "true" : "false",
        a ? input_method_to_string(a->preferred_input) : "keyboard",
        a ? output_method_to_string(a->preferred_output) : "visual",
        a && a->tts_enabled ? "true" : "false",
        a ? a->tts_speed : 1.0f
    );

    return json;
}

// ============================================================================
// DATABASE ACCESS FOR ANNA INTEGRATION
// ============================================================================

sqlite3* education_get_db_handle(void) {
    return g_edu_db;
}

// ============================================================================
// GOAL MANAGEMENT
// ============================================================================

int64_t education_goal_add(int64_t student_id, EducationGoalType goal_type,
                           const char* description, time_t target_date) {
    if (!g_edu_db || !description) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "INSERT INTO student_goals (student_id, goal_type, description, target_date, status, created_at) "
        "VALUES (?, ?, ?, ?, 'active', strftime('%s','now'))";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    const char* goal_type_str = "short_term";
    switch (goal_type) {
        case GOAL_SHORT_TERM: goal_type_str = "short_term"; break;
        case GOAL_MEDIUM_TERM: goal_type_str = "medium_term"; break;
        case GOAL_LONG_TERM: goal_type_str = "long_term"; break;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, goal_type_str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, description, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, (int64_t)target_date);

    rc = sqlite3_step(stmt);
    int64_t goal_id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(g_edu_db) : -1;

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);

    return goal_id;
}

EducationGoal** education_goal_list(int64_t student_id, int* count) {
    if (!g_edu_db || !count) return NULL;
    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "SELECT id, goal_type, description, target_date, status, created_at "
        "FROM student_goals WHERE student_id = ? AND status = 'active' ORDER BY created_at DESC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);

    // Count rows first
    int goal_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) goal_count++;

    if (goal_count == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    // Reset and allocate
    sqlite3_reset(stmt);
    EducationGoal** goals = calloc((size_t)goal_count, sizeof(EducationGoal*));
    if (!goals) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < goal_count) {
        EducationGoal* goal = calloc(1, sizeof(EducationGoal));
        if (!goal) continue;

        goal->id = sqlite3_column_int64(stmt, 0);
        goal->student_id = student_id;

        const char* type_str = (const char*)sqlite3_column_text(stmt, 1);
        if (type_str) {
            if (strcmp(type_str, "medium_term") == 0) goal->goal_type = GOAL_MEDIUM_TERM;
            else if (strcmp(type_str, "long_term") == 0) goal->goal_type = GOAL_LONG_TERM;
            else goal->goal_type = GOAL_SHORT_TERM;
        }

        const char* desc = (const char*)sqlite3_column_text(stmt, 2);
        if (desc) {
            strncpy(goal->description, desc, EDUCATION_MAX_NOTES_LEN - 1);
            goal->description[EDUCATION_MAX_NOTES_LEN - 1] = '\0';
        }

        goal->target_date = sqlite3_column_int64(stmt, 3);

        const char* status_str = (const char*)sqlite3_column_text(stmt, 4);
        if (status_str) {
            if (strcmp(status_str, "achieved") == 0) goal->status = GOAL_ACHIEVED;
            else if (strcmp(status_str, "abandoned") == 0) goal->status = GOAL_ABANDONED;
            else goal->status = GOAL_ACTIVE;
        }

        goals[i++] = goal;
    }

    *count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);

    return goals;
}

int education_goal_achieve(int64_t goal_id) {
    if (!g_edu_db) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql = "UPDATE student_goals SET status = 'achieved' WHERE id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, goal_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int education_goal_delete(int64_t goal_id) {
    if (!g_edu_db) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql = "DELETE FROM student_goals WHERE id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, goal_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

void education_goal_list_free(EducationGoal** goals, int count) {
    if (!goals) return;
    for (int i = 0; i < count; i++) {
        if (goals[i]) {
            // description is a fixed array, no need to free
            free(goals[i]);
        }
    }
    free(goals);
}

// ============================================================================
// MAESTRO BROADCAST (Ali Preside Integration)
// ============================================================================

int education_maestro_broadcast_profile(int64_t student_id) {
    // This function notifies all maestri about a student profile
    // For now, it's a stub that logs the broadcast
    (void)student_id;
    printf("[education] Profile broadcast to all maestri (stub implementation)\n");
    return 0;
}

// ============================================================================
// LLM GENERATION (Uses Convergio Provider System)
// ============================================================================

#include "nous/provider.h"

// Default model for education - use a cost-effective model
#define EDUCATION_DEFAULT_MODEL "claude-3-5-haiku-20241022"

__attribute__((weak))
char* llm_generate(const char* prompt, const char* system_prompt) {
    if (!prompt) return NULL;

    // Get Claude provider
    Provider* provider = provider_get(PROVIDER_ANTHROPIC);
    if (!provider) {
        // Try OpenAI as fallback
        provider = provider_get(PROVIDER_OPENAI);
    }
    if (!provider) {
        // Try Ollama for local models
        provider = provider_get(PROVIDER_OLLAMA);
    }

    if (!provider || !provider->initialized) {
        return strdup("[Errore: Nessun provider LLM configurato. Configura ANTHROPIC_API_KEY o OPENAI_API_KEY]");
    }

    // Use the chat function
    TokenUsage usage = {0};
    char* response = provider->chat(
        provider,
        EDUCATION_DEFAULT_MODEL,
        system_prompt ? system_prompt : "Sei un assistente educativo italiano. Rispondi in modo chiaro e pedagogico.",
        prompt,
        &usage
    );

    if (!response) {
        ProviderErrorInfo* err = provider->get_last_error(provider);
        if (err && err->message) {
            char error_msg[512];
            snprintf(error_msg, sizeof(error_msg), "[Errore LLM: %s]", err->message);
            return strdup(error_msg);
        }
        return strdup("[Errore: Generazione LLM fallita]");
    }

    return response;
}
