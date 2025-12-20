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
#include <math.h>
#include <sqlite3.h>
#include <cjson/cJSON.h>

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

// External init functions from learning science modules
extern int fsrs_init_db(void);
extern int mastery_init_db(void);

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
    "    tts_pitch REAL DEFAULT 0.0 CHECK(tts_pitch >= -1.0 AND tts_pitch <= 1.0),\n"
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
    "-- LIBRETTO DELLO STUDENTE - GRADEBOOK TABLE (LB01)\n"
    "-- Student grades with teacher feedback and analytics\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS student_gradebook (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    maestro_id TEXT NOT NULL,\n"
    "    subject TEXT NOT NULL,\n"
    "    topic TEXT,\n"
    "    grade_type TEXT NOT NULL CHECK(grade_type IN ('quiz', 'homework', 'oral', 'project', 'participation')),\n"
    "    grade REAL NOT NULL CHECK(grade >= 1.0 AND grade <= 10.0),\n"
    "    grade_percentage REAL CHECK(grade_percentage >= 0 AND grade_percentage <= 100),\n"
    "    comment TEXT,\n"
    "    questions_total INTEGER,\n"
    "    questions_correct INTEGER,\n"
    "    recorded_at INTEGER DEFAULT (strftime('%s','now'))\n"
    ");\n"
    "\n"
    "-- =====================================================================\n"
    "-- LIBRETTO DELLO STUDENTE - DAILY LOG TABLE (LB02)\n"
    "-- Daily activity tracking for study analytics\n"
    "-- =====================================================================\n"
    "CREATE TABLE IF NOT EXISTS daily_log (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    student_id INTEGER NOT NULL REFERENCES student_profiles(id) ON DELETE CASCADE,\n"
    "    maestro_id TEXT,\n"
    "    subject TEXT,\n"
    "    activity_type TEXT NOT NULL CHECK(activity_type IN ('study', 'quiz', 'homework', 'flashcards', 'review', 'project', 'exploration', 'break')),\n"
    "    topic TEXT,\n"
    "    notes TEXT,\n"
    "    duration_minutes INTEGER DEFAULT 0,\n"
    "    xp_earned INTEGER DEFAULT 0,\n"
    "    started_at INTEGER DEFAULT (strftime('%s','now')),\n"
    "    ended_at INTEGER\n"
    ");\n"
    "\n"
    "CREATE INDEX IF NOT EXISTS idx_gradebook_student ON student_gradebook(student_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_gradebook_subject ON student_gradebook(subject);\n"
    "CREATE INDEX IF NOT EXISTS idx_gradebook_date ON student_gradebook(recorded_at);\n"
    "CREATE INDEX IF NOT EXISTS idx_gradebook_maestro ON student_gradebook(maestro_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_daily_log_student ON daily_log(student_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_daily_log_date ON daily_log(started_at);\n"
    "CREATE INDEX IF NOT EXISTS idx_daily_log_subject ON daily_log(subject);\n"
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

    // Initialize learning science modules (FSRS + Mastery)
    fsrs_init_db();
    mastery_init_db();

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
              "tts_enabled, tts_speed, tts_pitch, high_contrast, reduce_animations"
              ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
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
            sqlite3_bind_double(stmt, 17, a->tts_pitch);
            sqlite3_bind_int(stmt, 18, a->high_contrast ? 1 : 0);
            sqlite3_bind_int(stmt, 19, a->reduce_motion ? 1 : 0);
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

    // Load accessibility settings with explicit column names (robust against schema changes)
    if (profile && profile->accessibility) {
        sql = "SELECT "
              "dyslexia, dyslexia_severity, dyscalculia, dyscalculia_severity, "
              "cerebral_palsy, cp_severity, adhd, adhd_type, adhd_severity, "
              "autism, autism_severity, preferred_input, preferred_output, "
              "tts_enabled, tts_speed, tts_pitch, high_contrast, reduce_animations "
              "FROM student_accessibility WHERE student_id = ?";
        rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, student_id);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                EducationAccessibility* a = profile->accessibility;
                // Column indices now match SELECT order (0-indexed)
                a->dyslexia = sqlite3_column_int(stmt, 0);
                a->dyslexia_severity = string_to_severity(sqlite3_column_int(stmt, 1));
                a->dyscalculia = sqlite3_column_int(stmt, 2);
                a->dyscalculia_severity = string_to_severity(sqlite3_column_int(stmt, 3));
                a->cerebral_palsy = sqlite3_column_int(stmt, 4);
                a->cerebral_palsy_severity = string_to_severity(sqlite3_column_int(stmt, 5));
                a->adhd = sqlite3_column_int(stmt, 6);
                a->adhd_type = string_to_adhd_type((const char*)sqlite3_column_text(stmt, 7));
                a->adhd_severity = string_to_severity(sqlite3_column_int(stmt, 8));
                a->autism = sqlite3_column_int(stmt, 9);
                a->autism_severity = string_to_severity(sqlite3_column_int(stmt, 10));
                a->preferred_input = string_to_input_method((const char*)sqlite3_column_text(stmt, 11));
                a->preferred_output = string_to_output_method((const char*)sqlite3_column_text(stmt, 12));
                a->tts_enabled = sqlite3_column_int(stmt, 13);
                a->tts_speed = (float)sqlite3_column_double(stmt, 14);
                a->tts_pitch = (float)sqlite3_column_double(stmt, 15);
                a->high_contrast = sqlite3_column_int(stmt, 16);
                a->reduce_motion = sqlite3_column_int(stmt, 17);
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

EducationStudentProfile** education_profile_list(int* count) {
    if (!g_edu_initialized || !count) return NULL;
    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // First, count profiles
    sqlite3_stmt* count_stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, "SELECT COUNT(*) FROM student_profiles", -1, &count_stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int total = 0;
    if (sqlite3_step(count_stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(count_stmt, 0);
    }
    sqlite3_finalize(count_stmt);

    if (total == 0) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    // Allocate array
    EducationStudentProfile** profiles = calloc(total, sizeof(EducationStudentProfile*));
    if (!profiles) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    // Fetch all profiles
    const char* sql =
        "SELECT p.id, p.name, p.age, p.grade_level, p.curriculum_id, "
        "p.parent_name, p.parent_email, p.preferred_language, p.learning_style, "
        "p.is_active, p.created_at, p.updated_at, p.last_session_at "
        "FROM student_profiles p ORDER BY p.name";

    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(profiles);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < total) {
        EducationStudentProfile* p = calloc(1, sizeof(EducationStudentProfile));
        if (!p) continue;

        p->id = sqlite3_column_int64(stmt, 0);
        p->name = safe_strdup((const char*)sqlite3_column_text(stmt, 1));
        p->age = sqlite3_column_int(stmt, 2);
        p->grade_level = sqlite3_column_int(stmt, 3);
        p->curriculum_id = safe_strdup((const char*)sqlite3_column_text(stmt, 4));
        p->parent_name = safe_strdup((const char*)sqlite3_column_text(stmt, 5));
        p->parent_email = safe_strdup((const char*)sqlite3_column_text(stmt, 6));
        p->preferred_language = safe_strdup((const char*)sqlite3_column_text(stmt, 7));
        p->study_method = safe_strdup((const char*)sqlite3_column_text(stmt, 8));
        p->is_active = sqlite3_column_int(stmt, 9);
        p->created_at = sqlite3_column_int64(stmt, 10);
        p->updated_at = sqlite3_column_int64(stmt, 11);
        p->last_session_at = sqlite3_column_int64(stmt, 12);

        profiles[idx++] = p;
    }
    sqlite3_finalize(stmt);

    *count = idx;
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return profiles;
}

void education_profile_list_free(EducationStudentProfile** profiles, int count) {
    if (!profiles) return;
    for (int i = 0; i < count; i++) {
        education_profile_free(profiles[i]);
    }
    free(profiles);
}

int education_profile_count(void) {
    if (!g_edu_initialized) return 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, "SELECT COUNT(*) FROM student_profiles", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return 0;
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return count;
}

bool education_is_first_run(void) {
    return education_profile_count() == 0;
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
// ADAPTIVE LEARNING API (S18)
// Learn from student interactions to personalize the experience
// ============================================================================

/**
 * Analyze student's learning patterns and return adaptive recommendations
 * Returns a JSON string with recommendations, caller must free
 */
char* education_adaptive_analyze(int64_t student_id) {
    if (!g_edu_initialized) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Query learning patterns
    const char* sql =
        "SELECT "
        "  subject, "
        "  AVG(skill_level) as avg_skill, "
        "  AVG(quiz_score_avg) as avg_quiz, "
        "  SUM(total_time_spent) as total_time, "
        "  COUNT(*) as topic_count, "
        "  MAX(last_interaction) as last_active "
        "FROM learning_progress "
        "WHERE student_id = ? "
        "GROUP BY subject "
        "ORDER BY avg_skill ASC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);

    // Build JSON response
    const size_t json_size = 8192;
    char* json = calloc(json_size, sizeof(char));
    if (!json) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    strlcat(json, "{\"student_id\":", json_size);
    char id_str[32];
    snprintf(id_str, sizeof(id_str), "%" PRId64, student_id);
    strlcat(json, id_str, json_size);
    strlcat(json, ",\"analysis\":{", json_size);

    // Weak subjects (need more attention)
    strlcat(json, "\"weak_subjects\":[", json_size);
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* subject = (const char*)sqlite3_column_text(stmt, 0);
        double avg_skill = sqlite3_column_double(stmt, 1);

        if (avg_skill < 0.5 && subject) {  // Below 50% is considered weak
            if (!first) strlcat(json, ",", json_size);
            strlcat(json, "\"", json_size);
            strlcat(json, subject, json_size);
            strlcat(json, "\"", json_size);
            first = false;
        }
    }
    strlcat(json, "],", json_size);
    sqlite3_reset(stmt);
    sqlite3_bind_int64(stmt, 1, student_id);

    // Strong subjects
    strlcat(json, "\"strong_subjects\":[", json_size);
    first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* subject = (const char*)sqlite3_column_text(stmt, 0);
        double avg_skill = sqlite3_column_double(stmt, 1);

        if (avg_skill >= 0.75 && subject) {  // Above 75% is strong
            if (!first) strlcat(json, ",", json_size);
            strlcat(json, "\"", json_size);
            strlcat(json, subject, json_size);
            strlcat(json, "\"", json_size);
            first = false;
        }
    }
    strlcat(json, "]", json_size);

    sqlite3_finalize(stmt);

    // Get recommended difficulty adjustment
    const char* diff_sql =
        "SELECT AVG(quiz_score_avg) as overall_avg FROM learning_progress WHERE student_id = ?";
    rc = sqlite3_prepare_v2(g_edu_db, diff_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            double overall = sqlite3_column_double(stmt, 0);
            strlcat(json, ",\"recommended_difficulty\":\"", json_size);
            if (overall >= 0.8) {
                strlcat(json, "hard", json_size);
            } else if (overall >= 0.5) {
                strlcat(json, "medium", json_size);
            } else {
                strlcat(json, "easy", json_size);
            }
            strlcat(json, "\"", json_size);
        }
        sqlite3_finalize(stmt);
    }

    // Get study time recommendation
    const char* time_sql =
        "SELECT AVG(julianday('now') - julianday(datetime(last_interaction, 'unixepoch'))) as days_since "
        "FROM learning_progress WHERE student_id = ?";
    rc = sqlite3_prepare_v2(g_edu_db, time_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            double days_since = sqlite3_column_double(stmt, 0);
            strlcat(json, ",\"days_since_activity\":", json_size);
            char days_str[32];
            snprintf(days_str, sizeof(days_str), "%.1f", days_since);
            strlcat(json, days_str, json_size);
        }
        sqlite3_finalize(stmt);
    }

    strlcat(json, "},\"recommendations\":[", json_size);

    // Generate recommendations based on patterns
    bool has_rec = false;

    // Check for neglected subjects
    const char* neglect_sql =
        "SELECT subject FROM learning_progress "
        "WHERE student_id = ? AND last_interaction < strftime('%s', 'now', '-7 days') "
        "GROUP BY subject LIMIT 3";
    rc = sqlite3_prepare_v2(g_edu_db, neglect_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* subject = (const char*)sqlite3_column_text(stmt, 0);
            if (subject) {
                if (has_rec) strlcat(json, ",", json_size);
                strlcat(json, "{\"type\":\"review\",\"subject\":\"", json_size);
                strlcat(json, subject, json_size);
                strlcat(json, "\",\"reason\":\"Not studied in over a week\"}", json_size);
                has_rec = true;
            }
        }
        sqlite3_finalize(stmt);
    }

    strlcat(json, "]}", json_size);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return json;
}

/**
 * Update student profile based on learning patterns (adaptive adjustment)
 * Returns 0 on success, -1 on error
 */
int education_adaptive_update_profile(int64_t student_id) {
    if (!g_edu_initialized) return -1;

    char* analysis = education_adaptive_analyze(student_id);
    if (!analysis) return -1;

    // For now, just log the analysis. In production, this would
    // update session_duration_preference, break_duration_preference,
    // and other profile settings based on observed patterns.

    // Future: Parse analysis JSON and update profile accordingly
    // - If student performs better in morning, suggest morning sessions
    // - If attention drops after 20 mins, reduce session_duration_preference
    // - If visual content gets higher scores, set learning_style to 'visual'

    free(analysis);
    return 0;
}

/**
 * Get next recommended topic for a student
 * Returns topic name, caller must free
 */
char* education_adaptive_next_topic(int64_t student_id, const char* subject) {
    if (!g_edu_initialized) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Find topics that need review (low skill or old)
    const char* sql =
        "SELECT topic FROM learning_progress "
        "WHERE student_id = ? AND subject = ? "
        "AND (skill_level < 0.7 OR last_interaction < strftime('%s', 'now', '-3 days')) "
        "ORDER BY skill_level ASC, last_interaction ASC "
        "LIMIT 1";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, subject, -1, SQLITE_STATIC);

    char* topic = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* t = (const char*)sqlite3_column_text(stmt, 0);
        if (t) topic = strdup(t);
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return topic;
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
        return strdup("[Error: No LLM provider configured. Set ANTHROPIC_API_KEY or OPENAI_API_KEY]");
    }

    // Use the chat function
    TokenUsage usage = {0};
    char* response = provider->chat(
        provider,
        EDUCATION_DEFAULT_MODEL,
        system_prompt ? system_prompt : "You are an educational assistant. Respond clearly and pedagogically.",
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

// ============================================================================
// LIBRETTO DELLO STUDENTE API (LB01-LB18)
// ============================================================================

static const char* grade_type_to_string(EducationGradeType type) {
    switch (type) {
        case GRADE_TYPE_QUIZ: return "quiz";
        case GRADE_TYPE_HOMEWORK: return "homework";
        case GRADE_TYPE_ORAL: return "oral";
        case GRADE_TYPE_PROJECT: return "project";
        case GRADE_TYPE_PARTICIPATION: return "participation";
        default: return "quiz";
    }
}

static EducationGradeType string_to_grade_type(const char* str) {
    if (!str) return GRADE_TYPE_QUIZ;
    if (strcmp(str, "homework") == 0) return GRADE_TYPE_HOMEWORK;
    if (strcmp(str, "oral") == 0) return GRADE_TYPE_ORAL;
    if (strcmp(str, "project") == 0) return GRADE_TYPE_PROJECT;
    if (strcmp(str, "participation") == 0) return GRADE_TYPE_PARTICIPATION;
    return GRADE_TYPE_QUIZ;
}

int64_t libretto_add_grade(int64_t student_id, const char* maestro_id,
                           const char* subject, const char* topic,
                           EducationGradeType grade_type, float grade,
                           const char* comment) {
    if (!g_edu_db || !subject || grade < 1.0f || grade > 10.0f) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "INSERT INTO student_gradebook (student_id, maestro_id, subject, topic, grade_type, grade, comment) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, maestro_id ? maestro_id : "ED00", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, subject, -1, SQLITE_STATIC);
    if (topic) sqlite3_bind_text(stmt, 4, topic, -1, SQLITE_STATIC);
    else sqlite3_bind_null(stmt, 4);
    sqlite3_bind_text(stmt, 5, grade_type_to_string(grade_type), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 6, grade);
    if (comment) sqlite3_bind_text(stmt, 7, comment, -1, SQLITE_STATIC);
    else sqlite3_bind_null(stmt, 7);

    rc = sqlite3_step(stmt);
    int64_t grade_id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(g_edu_db) : -1;
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return grade_id;
}

int64_t libretto_add_quiz_grade(int64_t student_id, const char* maestro_id,
                                const char* subject, const char* topic,
                                int correct, int total, const char* comment) {
    if (!g_edu_db || !subject || total <= 0) return -1;

    float percentage = (float)correct / (float)total * 100.0f;
    // Convert percentage to Italian grade (1-10 scale)
    // 0-49%: insufficiente (4-5), 50-59%: sufficiente (6), 60-69%: discreto (7)
    // 70-79%: buono (8), 80-89%: ottimo (9), 90-100%: eccellente (10)
    float grade;
    if (percentage < 50.0f) grade = 4.0f + (percentage / 50.0f);
    else if (percentage < 60.0f) grade = 6.0f;
    else if (percentage < 70.0f) grade = 7.0f;
    else if (percentage < 80.0f) grade = 8.0f;
    else if (percentage < 90.0f) grade = 9.0f;
    else grade = 10.0f;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "INSERT INTO student_gradebook (student_id, maestro_id, subject, topic, grade_type, "
        "grade, grade_percentage, questions_total, questions_correct, comment) "
        "VALUES (?, ?, ?, ?, 'quiz', ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, maestro_id ? maestro_id : "ED00", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, subject, -1, SQLITE_STATIC);
    if (topic) sqlite3_bind_text(stmt, 4, topic, -1, SQLITE_STATIC);
    else sqlite3_bind_null(stmt, 4);
    sqlite3_bind_double(stmt, 5, grade);
    sqlite3_bind_double(stmt, 6, percentage);
    sqlite3_bind_int(stmt, 7, total);
    sqlite3_bind_int(stmt, 8, correct);
    if (comment) sqlite3_bind_text(stmt, 9, comment, -1, SQLITE_STATIC);
    else sqlite3_bind_null(stmt, 9);

    rc = sqlite3_step(stmt);
    int64_t grade_id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(g_edu_db) : -1;
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return grade_id;
}

int64_t libretto_add_log_entry(int64_t student_id, const char* maestro_id,
                               const char* activity_type, const char* subject,
                               const char* topic, int duration_minutes,
                               const char* notes) {
    if (!g_edu_db || !activity_type) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    time_t now = time(NULL);
    time_t started_at = now - (duration_minutes * 60);

    const char* sql =
        "INSERT INTO daily_log (student_id, maestro_id, subject, activity_type, topic, "
        "notes, duration_minutes, started_at, ended_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    if (maestro_id) sqlite3_bind_text(stmt, 2, maestro_id, -1, SQLITE_STATIC);
    else sqlite3_bind_null(stmt, 2);
    if (subject) sqlite3_bind_text(stmt, 3, subject, -1, SQLITE_STATIC);
    else sqlite3_bind_null(stmt, 3);
    sqlite3_bind_text(stmt, 4, activity_type, -1, SQLITE_STATIC);
    if (topic) sqlite3_bind_text(stmt, 5, topic, -1, SQLITE_STATIC);
    else sqlite3_bind_null(stmt, 5);
    if (notes) sqlite3_bind_text(stmt, 6, notes, -1, SQLITE_STATIC);
    else sqlite3_bind_null(stmt, 6);
    sqlite3_bind_int(stmt, 7, duration_minutes);
    sqlite3_bind_int64(stmt, 8, started_at);
    sqlite3_bind_int64(stmt, 9, now);

    rc = sqlite3_step(stmt);
    int64_t log_id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(g_edu_db) : -1;
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return log_id;
}

EducationGrade** libretto_get_grades(int64_t student_id, const char* subject,
                                     time_t from_date, time_t to_date, int* count) {
    if (!g_edu_db || !count) return NULL;
    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Build dynamic query based on filters
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT id, maestro_id, subject, topic, grade_type, grade, grade_percentage, "
        "comment, questions_total, questions_correct, recorded_at "
        "FROM student_gradebook WHERE student_id = ?%s%s%s ORDER BY recorded_at DESC",
        subject ? " AND subject = ?" : "",
        from_date > 0 ? " AND recorded_at >= ?" : "",
        to_date > 0 ? " AND recorded_at <= ?" : "");

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int param_idx = 1;
    sqlite3_bind_int64(stmt, param_idx++, student_id);
    if (subject) sqlite3_bind_text(stmt, param_idx++, subject, -1, SQLITE_STATIC);
    if (from_date > 0) sqlite3_bind_int64(stmt, param_idx++, from_date);
    if (to_date > 0) sqlite3_bind_int64(stmt, param_idx++, to_date);

    // Count rows first
    int grade_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) grade_count++;

    if (grade_count == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    // Reset and allocate
    sqlite3_reset(stmt);
    EducationGrade** grades = calloc((size_t)grade_count, sizeof(EducationGrade*));
    if (!grades) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < grade_count) {
        EducationGrade* grade = calloc(1, sizeof(EducationGrade));
        if (!grade) continue;

        grade->id = sqlite3_column_int64(stmt, 0);
        grade->student_id = student_id;

        const char* maestro = (const char*)sqlite3_column_text(stmt, 1);
        if (maestro) strncpy(grade->maestro_id, maestro, sizeof(grade->maestro_id) - 1);

        const char* subj = (const char*)sqlite3_column_text(stmt, 2);
        if (subj) strncpy(grade->subject, subj, sizeof(grade->subject) - 1);

        const char* top = (const char*)sqlite3_column_text(stmt, 3);
        if (top) strncpy(grade->topic, top, sizeof(grade->topic) - 1);

        grade->grade_type = string_to_grade_type((const char*)sqlite3_column_text(stmt, 4));
        grade->grade = (float)sqlite3_column_double(stmt, 5);
        grade->grade_percentage = (float)sqlite3_column_double(stmt, 6);

        const char* comm = (const char*)sqlite3_column_text(stmt, 7);
        if (comm) strncpy(grade->comment, comm, sizeof(grade->comment) - 1);

        grade->questions_total = sqlite3_column_int(stmt, 8);
        grade->questions_correct = sqlite3_column_int(stmt, 9);
        grade->recorded_at = sqlite3_column_int64(stmt, 10);

        grades[i++] = grade;
    }

    *count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);

    return grades;
}

EducationDailyLogEntry** libretto_get_daily_log(int64_t student_id,
                                                 time_t from_date, time_t to_date,
                                                 int* count) {
    if (!g_edu_db || !count) return NULL;
    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT id, maestro_id, subject, activity_type, topic, notes, "
        "duration_minutes, xp_earned, started_at, ended_at "
        "FROM daily_log WHERE student_id = ?%s%s ORDER BY started_at DESC",
        from_date > 0 ? " AND started_at >= ?" : "",
        to_date > 0 ? " AND started_at <= ?" : "");

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int param_idx = 1;
    sqlite3_bind_int64(stmt, param_idx++, student_id);
    if (from_date > 0) sqlite3_bind_int64(stmt, param_idx++, from_date);
    if (to_date > 0) sqlite3_bind_int64(stmt, param_idx++, to_date);

    // Count rows
    int log_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) log_count++;

    if (log_count == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_reset(stmt);
    EducationDailyLogEntry** logs = calloc((size_t)log_count, sizeof(EducationDailyLogEntry*));
    if (!logs) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < log_count) {
        EducationDailyLogEntry* entry = calloc(1, sizeof(EducationDailyLogEntry));
        if (!entry) continue;

        entry->id = sqlite3_column_int64(stmt, 0);
        entry->student_id = student_id;

        const char* maestro = (const char*)sqlite3_column_text(stmt, 1);
        if (maestro) strncpy(entry->maestro_id, maestro, sizeof(entry->maestro_id) - 1);

        const char* subj = (const char*)sqlite3_column_text(stmt, 2);
        if (subj) strncpy(entry->subject, subj, sizeof(entry->subject) - 1);

        const char* act = (const char*)sqlite3_column_text(stmt, 3);
        if (act) strncpy(entry->activity_type, act, sizeof(entry->activity_type) - 1);

        const char* top = (const char*)sqlite3_column_text(stmt, 4);
        if (top) strncpy(entry->topic, top, sizeof(entry->topic) - 1);

        const char* notes = (const char*)sqlite3_column_text(stmt, 5);
        if (notes) strncpy(entry->notes, notes, sizeof(entry->notes) - 1);

        entry->duration_minutes = sqlite3_column_int(stmt, 6);
        entry->xp_earned = sqlite3_column_int(stmt, 7);
        entry->started_at = sqlite3_column_int64(stmt, 8);
        entry->ended_at = sqlite3_column_int64(stmt, 9);

        logs[i++] = entry;
    }

    *count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);

    return logs;
}

float libretto_get_average(int64_t student_id, const char* subject,
                           time_t from_date, time_t to_date) {
    if (!g_edu_db) return -1.0f;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT AVG(grade) FROM student_gradebook WHERE student_id = ?%s%s%s",
        subject ? " AND subject = ?" : "",
        from_date > 0 ? " AND recorded_at >= ?" : "",
        to_date > 0 ? " AND recorded_at <= ?" : "");

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1.0f;
    }

    int param_idx = 1;
    sqlite3_bind_int64(stmt, param_idx++, student_id);
    if (subject) sqlite3_bind_text(stmt, param_idx++, subject, -1, SQLITE_STATIC);
    if (from_date > 0) sqlite3_bind_int64(stmt, param_idx++, from_date);
    if (to_date > 0) sqlite3_bind_int64(stmt, param_idx++, to_date);

    float avg = -1.0f;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        avg = (float)sqlite3_column_double(stmt, 0);
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);

    return avg;
}

EducationProgressReport* libretto_get_progress_report(int64_t student_id,
                                                       time_t from_date,
                                                       time_t to_date) {
    if (!g_edu_db) return NULL;

    // Default to last 30 days
    time_t now = time(NULL);
    if (to_date == 0) to_date = now;
    if (from_date == 0) from_date = now - (30 * 24 * 60 * 60);

    EducationProgressReport* report = calloc(1, sizeof(EducationProgressReport));
    if (!report) return NULL;

    report->student_id = student_id;
    report->period_start = from_date;
    report->period_end = to_date;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Get student name
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, "SELECT name FROM student_profiles WHERE id = ?", -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* name = (const char*)sqlite3_column_text(stmt, 0);
            if (name) strncpy(report->student_name, name, sizeof(report->student_name) - 1);
        }
        sqlite3_finalize(stmt);
    }

    // Get overall average
    rc = sqlite3_prepare_v2(g_edu_db,
        "SELECT AVG(grade) FROM student_gradebook WHERE student_id = ? AND recorded_at BETWEEN ? AND ?",
        -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        sqlite3_bind_int64(stmt, 2, from_date);
        sqlite3_bind_int64(stmt, 3, to_date);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            report->overall_average = (float)sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Get total study hours
    rc = sqlite3_prepare_v2(g_edu_db,
        "SELECT SUM(duration_minutes) / 60, COUNT(*) FROM daily_log "
        "WHERE student_id = ? AND started_at BETWEEN ? AND ?",
        -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        sqlite3_bind_int64(stmt, 2, from_date);
        sqlite3_bind_int64(stmt, 3, to_date);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            report->total_study_hours = sqlite3_column_int(stmt, 0);
            report->total_sessions = sqlite3_column_int(stmt, 1);
        }
        sqlite3_finalize(stmt);
    }

    // Get quizzes taken
    rc = sqlite3_prepare_v2(g_edu_db,
        "SELECT COUNT(*) FROM student_gradebook "
        "WHERE student_id = ? AND grade_type = 'quiz' AND recorded_at BETWEEN ? AND ?",
        -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        sqlite3_bind_int64(stmt, 2, from_date);
        sqlite3_bind_int64(stmt, 3, to_date);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            report->quizzes_taken = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Get goals achieved
    rc = sqlite3_prepare_v2(g_edu_db,
        "SELECT COUNT(*) FROM student_goals "
        "WHERE student_id = ? AND status = 'achieved' AND completed_at BETWEEN ? AND ?",
        -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        sqlite3_bind_int64(stmt, 2, from_date);
        sqlite3_bind_int64(stmt, 3, to_date);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            report->goals_achieved = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Get current streak
    rc = sqlite3_prepare_v2(g_edu_db,
        "SELECT current_streak FROM gamification WHERE student_id = ?",
        -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            report->current_streak = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Get subject stats
    rc = sqlite3_prepare_v2(g_edu_db,
        "SELECT subject, maestro_id, AVG(grade), COUNT(*) "
        "FROM student_gradebook "
        "WHERE student_id = ? AND recorded_at BETWEEN ? AND ? "
        "GROUP BY subject ORDER BY AVG(grade) DESC",
        -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        sqlite3_bind_int64(stmt, 2, from_date);
        sqlite3_bind_int64(stmt, 3, to_date);

        // Count subjects
        int subject_count = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) subject_count++;

        if (subject_count > 0) {
            sqlite3_reset(stmt);
            report->subjects = calloc((size_t)subject_count, sizeof(EducationSubjectStats));
            if (report->subjects) {
                int i = 0;
                while (sqlite3_step(stmt) == SQLITE_ROW && i < subject_count) {
                    const char* subj = (const char*)sqlite3_column_text(stmt, 0);
                    if (subj) strncpy(report->subjects[i].subject, subj, sizeof(report->subjects[i].subject) - 1);

                    const char* maestro = (const char*)sqlite3_column_text(stmt, 1);
                    if (maestro) strncpy(report->subjects[i].maestro_id, maestro, sizeof(report->subjects[i].maestro_id) - 1);

                    report->subjects[i].average_grade = (float)sqlite3_column_double(stmt, 2);
                    report->subjects[i].grade_count = sqlite3_column_int(stmt, 3);
                    i++;
                }
                report->subject_count = i;
            }
        }
        sqlite3_finalize(stmt);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return report;
}

EducationSubjectStats** libretto_get_study_stats(int64_t student_id,
                                                  time_t from_date, time_t to_date,
                                                  int* count) {
    if (!g_edu_db || !count) return NULL;
    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT subject, maestro_id, SUM(duration_minutes), COUNT(*) "
        "FROM daily_log WHERE student_id = ?%s%s AND subject IS NOT NULL "
        "GROUP BY subject ORDER BY SUM(duration_minutes) DESC",
        from_date > 0 ? " AND started_at >= ?" : "",
        to_date > 0 ? " AND started_at <= ?" : "");

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int param_idx = 1;
    sqlite3_bind_int64(stmt, param_idx++, student_id);
    if (from_date > 0) sqlite3_bind_int64(stmt, param_idx++, from_date);
    if (to_date > 0) sqlite3_bind_int64(stmt, param_idx++, to_date);

    // Count rows
    int stats_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) stats_count++;

    if (stats_count == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_reset(stmt);
    EducationSubjectStats** stats = calloc((size_t)stats_count, sizeof(EducationSubjectStats*));
    if (!stats) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < stats_count) {
        EducationSubjectStats* stat = calloc(1, sizeof(EducationSubjectStats));
        if (!stat) continue;

        const char* subj = (const char*)sqlite3_column_text(stmt, 0);
        if (subj) strncpy(stat->subject, subj, sizeof(stat->subject) - 1);

        const char* maestro = (const char*)sqlite3_column_text(stmt, 1);
        if (maestro) strncpy(stat->maestro_id, maestro, sizeof(stat->maestro_id) - 1);

        stat->total_study_minutes = sqlite3_column_int(stmt, 2);
        stat->grade_count = sqlite3_column_int(stmt, 3);

        stats[i++] = stat;
    }

    *count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);

    return stats;
}

void libretto_grades_free(EducationGrade** grades, int count) {
    if (!grades) return;
    for (int i = 0; i < count; i++) {
        free(grades[i]);
    }
    free(grades);
}

void libretto_logs_free(EducationDailyLogEntry** logs, int count) {
    if (!logs) return;
    for (int i = 0; i < count; i++) {
        free(logs[i]);
    }
    free(logs);
}

void libretto_report_free(EducationProgressReport* report) {
    if (!report) return;
    free(report->subjects);
    free(report);
}

void libretto_stats_free(EducationSubjectStats** stats, int count) {
    if (!stats) return;
    for (int i = 0; i < count; i++) {
        free(stats[i]);
    }
    free(stats);
}

// ============================================================================
// TOOLKIT API (remaining functions)
// ============================================================================

static EducationToolkitType str_to_toolkit_type(const char* str) {
    if (!str) return TOOLKIT_NOTE;
    if (strcmp(str, "mindmap") == 0) return TOOLKIT_MINDMAP;
    if (strcmp(str, "quiz") == 0) return TOOLKIT_QUIZ;
    if (strcmp(str, "flashcard_deck") == 0) return TOOLKIT_FLASHCARD;
    if (strcmp(str, "audio") == 0) return TOOLKIT_AUDIO;
    if (strcmp(str, "summary") == 0) return TOOLKIT_SUMMARY;
    if (strcmp(str, "formula") == 0) return TOOLKIT_FORMULA;
    if (strcmp(str, "graph") == 0) return TOOLKIT_GRAPH;
    if (strcmp(str, "flowchart") == 0) return TOOLKIT_FLOWCHART;
    if (strcmp(str, "timeline") == 0) return TOOLKIT_TIMELINE;
    return TOOLKIT_NOTE;
}

EducationToolkitOutput* education_toolkit_get(int64_t output_id) {
    if (!g_edu_initialized) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql = "SELECT id, student_id, output_type, topic, content, format, "
                      "created_at, updated_at FROM toolkit_outputs WHERE id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, output_id);

    EducationToolkitOutput* output = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        output = calloc(1, sizeof(EducationToolkitOutput));
        if (output) {
            output->id = sqlite3_column_int64(stmt, 0);
            output->student_id = sqlite3_column_int64(stmt, 1);
            const char* type_str = (const char*)sqlite3_column_text(stmt, 2);
            output->tool_type = str_to_toolkit_type(type_str);
            const char* topic = (const char*)sqlite3_column_text(stmt, 3);
            if (topic) strncpy(output->topic, topic, sizeof(output->topic) - 1);
            const char* content = (const char*)sqlite3_column_text(stmt, 4);
            if (content) output->content = strdup(content);
            const char* format = (const char*)sqlite3_column_text(stmt, 5);
            if (format) strncpy(output->format, format, sizeof(output->format) - 1);
            output->created_at = (time_t)sqlite3_column_int64(stmt, 6);
            output->last_accessed = (time_t)sqlite3_column_int64(stmt, 7);
        }
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return output;
}

EducationToolkitOutput** education_toolkit_list(int64_t student_id, int type, int* count) {
    if (!g_edu_initialized || !count) return NULL;
    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // First count records
    const char* count_sql = (type < 0)
        ? "SELECT COUNT(*) FROM toolkit_outputs WHERE student_id = ?"
        : "SELECT COUNT(*) FROM toolkit_outputs WHERE student_id = ? AND output_type = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, count_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    if (type >= 0) {
        const char* types[] = {"mindmap", "quiz", "flashcard_deck", "audio", "summary",
                               "formula", "graph", "flowchart", "timeline"};
        const char* type_str = (type < 9) ? types[type] : "summary";
        sqlite3_bind_text(stmt, 2, type_str, -1, SQLITE_STATIC);
    }

    int total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (total == 0) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    // Fetch records
    const char* list_sql = (type < 0)
        ? "SELECT id, student_id, output_type, topic, content, format, created_at, updated_at "
          "FROM toolkit_outputs WHERE student_id = ? ORDER BY created_at DESC"
        : "SELECT id, student_id, output_type, topic, content, format, created_at, updated_at "
          "FROM toolkit_outputs WHERE student_id = ? AND output_type = ? ORDER BY created_at DESC";

    rc = sqlite3_prepare_v2(g_edu_db, list_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    if (type >= 0) {
        const char* types[] = {"mindmap", "quiz", "flashcard_deck", "audio", "summary",
                               "formula", "graph", "flowchart", "timeline"};
        const char* type_str = (type < 9) ? types[type] : "summary";
        sqlite3_bind_text(stmt, 2, type_str, -1, SQLITE_STATIC);
    }

    EducationToolkitOutput** outputs = calloc((size_t)total, sizeof(EducationToolkitOutput*));
    if (!outputs) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        EducationToolkitOutput* output = calloc(1, sizeof(EducationToolkitOutput));
        if (!output) continue;

        output->id = sqlite3_column_int64(stmt, 0);
        output->student_id = sqlite3_column_int64(stmt, 1);
        const char* type_str = (const char*)sqlite3_column_text(stmt, 2);
        output->tool_type = str_to_toolkit_type(type_str);
        const char* topic = (const char*)sqlite3_column_text(stmt, 3);
        if (topic) strncpy(output->topic, topic, sizeof(output->topic) - 1);
        const char* content = (const char*)sqlite3_column_text(stmt, 4);
        if (content) output->content = strdup(content);
        const char* format = (const char*)sqlite3_column_text(stmt, 5);
        if (format) strncpy(output->format, format, sizeof(output->format) - 1);
        output->created_at = (time_t)sqlite3_column_int64(stmt, 6);
        output->last_accessed = (time_t)sqlite3_column_int64(stmt, 7);

        outputs[i++] = output;
    }

    *count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return outputs;
}

void education_toolkit_free(EducationToolkitOutput* output) {
    if (!output) return;
    free(output->content);
    free(output);
}

void education_toolkit_list_free(EducationToolkitOutput** outputs, int count) {
    if (!outputs) return;
    for (int i = 0; i < count; i++) {
        education_toolkit_free(outputs[i]);
    }
    free(outputs);
}

// ============================================================================
// FLASHCARD API (remaining functions)
// ============================================================================

int education_flashcard_create_reviews(int64_t toolkit_output_id, int card_count) {
    if (!g_edu_initialized || card_count <= 0) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "INSERT INTO flashcard_reviews (toolkit_output_id, card_index, easiness_factor, "
        "interval_days, repetition_count, next_review_at, status) "
        "VALUES (?, ?, 2.5, 0, 0, ?, 'new')";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    time_t now = time(NULL);
    int success = 0;

    for (int i = 0; i < card_count; i++) {
        sqlite3_reset(stmt);
        sqlite3_bind_int64(stmt, 1, toolkit_output_id);
        sqlite3_bind_int(stmt, 2, i);
        sqlite3_bind_int64(stmt, 3, now);  // Due immediately

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            success++;
        }
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);

    return (success == card_count) ? 0 : -1;
}

int education_flashcard_due_count(int64_t student_id) {
    if (!g_edu_initialized) return 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Count cards due for review (next_review_at <= now)
    const char* sql =
        "SELECT COUNT(*) FROM flashcard_reviews fr "
        "JOIN toolkit_outputs t ON fr.toolkit_output_id = t.id "
        "WHERE t.student_id = ? AND fr.next_review_at <= ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return 0;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_int64(stmt, 2, (int64_t)time(NULL));

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return count;
}

// ============================================================================
// SESSION API (remaining functions)
// ============================================================================

EducationSession** education_session_list(int64_t student_id, int limit, int* count) {
    if (!g_edu_initialized || !count) return NULL;
    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "SELECT id, student_id, session_type, subject, topic, started_at, ended_at, "
        "duration_minutes, xp_earned FROM learning_sessions "
        "WHERE student_id = ? ORDER BY started_at DESC LIMIT ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_int(stmt, 2, limit > 0 ? limit : 10);

    // First pass: count results
    int total = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        total++;
    }

    if (total == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_reset(stmt);

    EducationSession** sessions = calloc((size_t)total, sizeof(EducationSession*));
    if (!sessions) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        EducationSession* session = calloc(1, sizeof(EducationSession));
        if (!session) continue;

        session->id = sqlite3_column_int64(stmt, 0);
        session->student_id = sqlite3_column_int64(stmt, 1);
        // session_type stored as text, we use maestro_id field
        const char* maestro = (const char*)sqlite3_column_text(stmt, 2);
        if (maestro) strncpy(session->maestro_id, maestro, sizeof(session->maestro_id) - 1);
        const char* topic = (const char*)sqlite3_column_text(stmt, 4);
        if (topic) strncpy(session->topic, topic, sizeof(session->topic) - 1);
        session->started_at = (time_t)sqlite3_column_int64(stmt, 5);
        session->ended_at = (time_t)sqlite3_column_int64(stmt, 6);
        session->duration_minutes = sqlite3_column_int(stmt, 7);

        sessions[i++] = session;
    }

    *count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return sessions;
}

void education_session_list_free(EducationSession** sessions, int count) {
    if (!sessions) return;
    for (int i = 0; i < count; i++) {
        free(sessions[i]);
    }
    free(sessions);
}

// ============================================================================
// ACCESSIBILITY API (remaining functions)
// ============================================================================

bool education_accessibility_wants_tts(int64_t student_id) {
    EducationStudentProfile* profile = education_profile_get(student_id);
    if (!profile) return false;

    bool wants_tts = false;
    if (profile->accessibility) {
        wants_tts = profile->accessibility->tts_enabled ||
                    profile->accessibility->preferred_output == OUTPUT_TTS ||
                    profile->accessibility->preferred_output == OUTPUT_BOTH ||
                    profile->accessibility->visual_impairment;
    }

    education_profile_free(profile);
    return wants_tts;
}

// ============================================================================
// CURRICULUM API (real JSON parsing implementation)
// ============================================================================

// Helper to read entire file into string
static char* read_file_to_string(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = malloc((size_t)size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(content, 1, (size_t)size, f);
    content[read] = '\0';
    fclose(f);
    return content;
}

// Find curriculum JSON file in various paths
static char* find_curriculum_file(const char* curriculum_id) {
    char path[512];
    const char* search_paths[] = {
        "curricula/it/%s.json",
        "../curricula/it/%s.json",
        "../../curricula/it/%s.json",
        NULL
    };

    // Try relative to cwd first
    for (const char** p = search_paths; *p; p++) {
        snprintf(path, sizeof(path), *p, curriculum_id);
        char* content = read_file_to_string(path);
        if (content) return content;
    }

    // Try relative to home directory
    const char* home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/.convergio/curricula/it/%s.json", home, curriculum_id);
        char* content = read_file_to_string(path);
        if (content) return content;
    }

    return NULL;
}

EducationCurriculum* education_curriculum_load(const char* curriculum_id) {
    if (!curriculum_id) return NULL;

    // Read JSON file
    char* json_str = find_curriculum_file(curriculum_id);
    if (!json_str) return NULL;

    // Parse JSON
    cJSON* root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) return NULL;

    // Create curriculum structure
    EducationCurriculum* curr = calloc(1, sizeof(EducationCurriculum));
    if (!curr) {
        cJSON_Delete(root);
        return NULL;
    }

    // Parse basic fields
    cJSON* id = cJSON_GetObjectItem(root, "id");
    cJSON* name = cJSON_GetObjectItem(root, "name");
    cJSON* country = cJSON_GetObjectItem(root, "country");
    cJSON* years = cJSON_GetObjectItem(root, "years");

    if (id && cJSON_IsString(id)) {
        strncpy(curr->id, id->valuestring, sizeof(curr->id) - 1);
    }
    if (name && cJSON_IsString(name)) {
        strncpy(curr->name, name->valuestring, sizeof(curr->name) - 1);
    }
    if (country && cJSON_IsString(country)) {
        strncpy(curr->country, country->valuestring, sizeof(curr->country) - 1);
    }
    if (years && cJSON_IsNumber(years)) {
        curr->year = years->valueint;
    }

    // Parse subjects
    cJSON* subjects = cJSON_GetObjectItem(root, "subjects");
    if (subjects && cJSON_IsObject(subjects)) {
        int subject_count = cJSON_GetArraySize(subjects);
        curr->subjects = calloc((size_t)subject_count, sizeof(EducationSubject));
        if (curr->subjects) {
            curr->subject_count = 0;
            cJSON* subj = NULL;
            cJSON_ArrayForEach(subj, subjects) {
                if (!cJSON_IsObject(subj)) continue;

                EducationSubject* s = &curr->subjects[curr->subject_count];
                strncpy(s->id, subj->string, sizeof(s->id) - 1);

                cJSON* maestro = cJSON_GetObjectItem(subj, "maestro");
                if (maestro && cJSON_IsString(maestro)) {
                    strncpy(s->maestro_id, maestro->valuestring, sizeof(s->maestro_id) - 1);
                }

                cJSON* hours = cJSON_GetObjectItem(subj, "hours_per_week");
                if (hours && cJSON_IsNumber(hours)) {
                    s->hours_per_week = hours->valueint;
                }

                s->topic_count = 0;
                s->topics = NULL;
                curr->subject_count++;
            }
        }
    }

    // Parse curriculum_by_year for detailed topics (first year as sample)
    cJSON* by_year = cJSON_GetObjectItem(root, "curriculum_by_year");
    if (by_year && cJSON_IsObject(by_year)) {
        cJSON* year1 = cJSON_GetObjectItem(by_year, "1");
        if (year1) {
            cJSON* year_subjects = cJSON_GetObjectItem(year1, "subjects");
            if (year_subjects && cJSON_IsObject(year_subjects)) {
                // Update subjects with topics from year 1
                cJSON* subj = NULL;
                cJSON_ArrayForEach(subj, year_subjects) {
                    if (!cJSON_IsObject(subj)) continue;

                    // Find matching subject
                    for (int i = 0; i < curr->subject_count; i++) {
                        if (strcmp(curr->subjects[i].id, subj->string) == 0) {
                            cJSON* units = cJSON_GetObjectItem(subj, "units");
                            if (units && cJSON_IsArray(units)) {
                                int topic_count = 0;
                                // Count total topics
                                cJSON* unit = NULL;
                                cJSON_ArrayForEach(unit, units) {
                                    cJSON* topics = cJSON_GetObjectItem(unit, "topics");
                                    if (topics) topic_count += cJSON_GetArraySize(topics);
                                }

                                if (topic_count > 0) {
                                    curr->subjects[i].topics = calloc((size_t)topic_count, sizeof(char*));
                                    if (curr->subjects[i].topics) {
                                        int idx = 0;
                                        cJSON_ArrayForEach(unit, units) {
                                            cJSON* topics = cJSON_GetObjectItem(unit, "topics");
                                            if (topics && cJSON_IsArray(topics)) {
                                                cJSON* topic = NULL;
                                                cJSON_ArrayForEach(topic, topics) {
                                                    if (cJSON_IsString(topic)) {
                                                        curr->subjects[i].topics[idx++] = strdup(topic->valuestring);
                                                    }
                                                }
                                            }
                                        }
                                        curr->subjects[i].topic_count = idx;
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
    return curr;
}

char** education_curriculum_list(int* count) {
    if (!count) return NULL;
    *count = 0;

    // Return list of available curricula
    const char* curricula[] = {
        "liceo_scientifico",
        "liceo_classico",
        "liceo_linguistico",
        "liceo_artistico",
        "scuola_media",
        "elementari",
        "iti_informatica",
        "iti_commerciale"
    };

    int num = 8;
    char** list = calloc((size_t)num, sizeof(char*));
    if (!list) return NULL;

    for (int i = 0; i < num; i++) {
        list[i] = strdup(curricula[i]);
    }

    *count = num;
    return list;
}

EducationSubject** education_curriculum_get_subjects(const char* curriculum_id, int year, int* count) {
    if (!count) return NULL;
    *count = 0;

    // Load curriculum
    EducationCurriculum* curr = education_curriculum_load(curriculum_id);
    if (!curr) return NULL;

    // Read JSON again for year-specific subjects
    char* json_str = find_curriculum_file(curriculum_id);
    if (!json_str) {
        education_curriculum_free(curr);
        return NULL;
    }

    cJSON* root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) {
        education_curriculum_free(curr);
        return NULL;
    }

    // Get year-specific subjects
    cJSON* by_year = cJSON_GetObjectItem(root, "curriculum_by_year");
    if (!by_year) {
        cJSON_Delete(root);
        education_curriculum_free(curr);
        return NULL;
    }

    char year_str[8];
    snprintf(year_str, sizeof(year_str), "%d", year);
    cJSON* year_data = cJSON_GetObjectItem(by_year, year_str);
    if (!year_data) {
        cJSON_Delete(root);
        education_curriculum_free(curr);
        return NULL;
    }

    cJSON* subjects = cJSON_GetObjectItem(year_data, "subjects");
    if (!subjects || !cJSON_IsObject(subjects)) {
        cJSON_Delete(root);
        education_curriculum_free(curr);
        return NULL;
    }

    int subject_count = cJSON_GetArraySize(subjects);
    EducationSubject** result = calloc((size_t)subject_count, sizeof(EducationSubject*));
    if (!result) {
        cJSON_Delete(root);
        education_curriculum_free(curr);
        return NULL;
    }

    int idx = 0;
    cJSON* subj = NULL;
    cJSON* base_subjects = cJSON_GetObjectItem(root, "subjects");

    cJSON_ArrayForEach(subj, subjects) {
        if (!cJSON_IsObject(subj)) continue;

        EducationSubject* s = calloc(1, sizeof(EducationSubject));
        if (!s) continue;

        strncpy(s->id, subj->string, sizeof(s->id) - 1);

        // Get maestro from base subjects
        if (base_subjects) {
            cJSON* base_subj = cJSON_GetObjectItem(base_subjects, subj->string);
            if (base_subj) {
                cJSON* maestro = cJSON_GetObjectItem(base_subj, "maestro");
                if (maestro && cJSON_IsString(maestro)) {
                    strncpy(s->maestro_id, maestro->valuestring, sizeof(s->maestro_id) - 1);
                }
                cJSON* hours = cJSON_GetObjectItem(base_subj, "hours_per_week");
                if (hours && cJSON_IsNumber(hours)) {
                    s->hours_per_week = hours->valueint;
                }
            }
        }

        // Get topics from units
        cJSON* units = cJSON_GetObjectItem(subj, "units");
        if (units && cJSON_IsArray(units)) {
            int topic_count = 0;
            cJSON* unit = NULL;
            cJSON_ArrayForEach(unit, units) {
                cJSON* topics = cJSON_GetObjectItem(unit, "topics");
                if (topics) topic_count += cJSON_GetArraySize(topics);
            }

            if (topic_count > 0) {
                s->topics = calloc((size_t)topic_count, sizeof(char*));
                if (s->topics) {
                    int t_idx = 0;
                    cJSON_ArrayForEach(unit, units) {
                        cJSON* topics = cJSON_GetObjectItem(unit, "topics");
                        if (topics && cJSON_IsArray(topics)) {
                            cJSON* topic = NULL;
                            cJSON_ArrayForEach(topic, topics) {
                                if (cJSON_IsString(topic)) {
                                    s->topics[t_idx++] = strdup(topic->valuestring);
                                }
                            }
                        }
                    }
                    s->topic_count = t_idx;
                }
            }
        }

        result[idx++] = s;
    }

    *count = idx;
    cJSON_Delete(root);
    education_curriculum_free(curr);
    return result;
}

void education_curriculum_free(EducationCurriculum* curriculum) {
    if (!curriculum) return;
    if (curriculum->subjects) {
        for (int i = 0; i < curriculum->subject_count; i++) {
            if (curriculum->subjects[i].topics) {
                for (int j = 0; j < curriculum->subjects[i].topic_count; j++) {
                    free(curriculum->subjects[i].topics[j]);
                }
                free(curriculum->subjects[i].topics);
            }
        }
        free(curriculum->subjects);
    }
    free(curriculum);
}

void education_curriculum_list_free(char** curricula, int count) {
    if (!curricula) return;
    for (int i = 0; i < count; i++) {
        free(curricula[i]);
    }
    free(curricula);
}

// ============================================================================
// PHASE 4: C10 CUSTOM PATH SYSTEM
// ============================================================================

typedef struct {
    char name[128];
    char subjects[20][64];
    int subject_count;
    char description[256];
} CustomCurriculumPath;

int education_curriculum_create_custom(int64_t student_id, const char* name,
                                        const char** subjects, int subject_count,
                                        const char* description) {
    if (!g_edu_initialized || !name || !subjects || subject_count <= 0) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Create custom curriculum as JSON
    char subjects_json[2048] = "[";
    for (int i = 0; i < subject_count && i < 20; i++) {
        if (i > 0) strlcat(subjects_json, ",", sizeof(subjects_json));
        strlcat(subjects_json, "\"", sizeof(subjects_json));
        strlcat(subjects_json, subjects[i], sizeof(subjects_json));
        strlcat(subjects_json, "\"", sizeof(subjects_json));
    }
    strlcat(subjects_json, "]", sizeof(subjects_json));

    const char* sql =
        "INSERT INTO curriculum_progress (student_id, curriculum_id, subject, progress, custom_path) "
        "VALUES (?, ?, 'custom', 0.0, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    char custom_json[4096];
    snprintf(custom_json, sizeof(custom_json),
             "{\"name\":\"%s\",\"subjects\":%s,\"description\":\"%s\"}",
             name, subjects_json, description ? description : "");

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, custom_json, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

char** education_curriculum_get_custom_list(int64_t student_id, int* count) {
    if (!g_edu_initialized || !count) return NULL;
    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql = "SELECT DISTINCT curriculum_id FROM curriculum_progress "
                      "WHERE student_id = ? AND custom_path IS NOT NULL";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);

    // Count first
    int total = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) total++;
    if (total == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_reset(stmt);
    char** list = calloc((size_t)total, sizeof(char*));
    if (!list) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        const char* name = (const char*)sqlite3_column_text(stmt, 0);
        if (name) list[i++] = strdup(name);
    }

    *count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return list;
}

// ============================================================================
// PHASE 4: C11 HOT-RELOAD JSON (File Watcher)
// ============================================================================

static time_t g_curriculum_last_modified = 0;
static char g_curriculum_watch_path[512] = {0};

int education_curriculum_watch_start(const char* path) {
    if (!path) return -1;
    strncpy(g_curriculum_watch_path, path, sizeof(g_curriculum_watch_path) - 1);

    struct stat st;
    if (stat(path, &st) == 0) {
        g_curriculum_last_modified = st.st_mtime;
    }
    return 0;
}

bool education_curriculum_check_reload(void) {
    if (g_curriculum_watch_path[0] == '\0') return false;

    struct stat st;
    if (stat(g_curriculum_watch_path, &st) != 0) return false;

    if (st.st_mtime > g_curriculum_last_modified) {
        g_curriculum_last_modified = st.st_mtime;
        return true;  // File changed, reload needed
    }
    return false;
}

void education_curriculum_watch_stop(void) {
    g_curriculum_watch_path[0] = '\0';
    g_curriculum_last_modified = 0;
}

// ============================================================================
// PHASE 5: F12 ACTIVE BREAK SUGGESTIONS
// ============================================================================

typedef struct {
    int break_type;  // 0=stretch, 1=walk, 2=eyes, 3=breathing, 4=hydration
    char title[64];
    char description[256];
    int duration_seconds;
} ActiveBreak;

static const ActiveBreak g_active_breaks[] = {
    {0, "Stretching Break", "Stand up and stretch your arms above your head. Roll your shoulders back 5 times.", 60},
    {1, "Walking Break", "Take a quick walk around the room or do 20 steps in place.", 120},
    {2, "Eye Rest", "Close your eyes for 20 seconds, then look at something 20 feet away for 20 seconds.", 40},
    {3, "Breathing Exercise", "Take 5 deep breaths: inhale for 4 seconds, hold for 4, exhale for 4.", 60},
    {4, "Hydration Break", "Drink a glass of water. Staying hydrated helps concentration!", 30},
    {0, "Neck Rolls", "Slowly roll your head in circles, 5 times clockwise, 5 times counter-clockwise.", 45},
    {1, "Jumping Jacks", "Do 10 jumping jacks to get your blood flowing!", 30},
    {2, "Palming", "Rub your hands together to warm them, then cup them over your closed eyes.", 60},
    {3, "Box Breathing", "Breathe in 4 sec, hold 4 sec, out 4 sec, hold 4 sec. Repeat 4 times.", 64},
    {4, "Snack Break", "Have a healthy snack like fruit or nuts to fuel your brain!", 120}
};

ActiveBreak* education_suggest_active_break(int study_minutes, bool has_adhd) {
    // For ADHD, suggest breaks more frequently
    int break_interval = has_adhd ? 15 : 25;

    if (study_minutes < break_interval) return NULL;

    // Select a random break
    int break_count = sizeof(g_active_breaks) / sizeof(g_active_breaks[0]);
    int index = (int)(time(NULL) % (unsigned long)break_count);

    ActiveBreak* result = malloc(sizeof(ActiveBreak));
    if (result) {
        *result = g_active_breaks[index];
    }
    return result;
}

void education_active_break_free(ActiveBreak* brk) {
    free(brk);
}

// ============================================================================
// PHASE 5: F17 COMPLETION CERTIFICATES
// ============================================================================

typedef struct {
    int64_t student_id;
    char student_name[128];
    char subject[64];
    char topic[128];
    char achievement[128];
    time_t date;
    char certificate_id[64];
} CompletionCertificate;

char* education_generate_certificate(int64_t student_id, const char* subject,
                                      const char* topic, const char* achievement) {
    if (!g_edu_initialized || !subject) return NULL;

    EducationStudentProfile* profile = education_profile_get(student_id);
    if (!profile) return NULL;

    // Generate unique certificate ID
    char cert_id[64];
    snprintf(cert_id, sizeof(cert_id), "CONV-%ld-%lld",
             (long)time(NULL), (long long)student_id);

    // Generate HTML certificate
    char* html = malloc(4096);
    if (!html) {
        education_profile_free(profile);
        return NULL;
    }

    snprintf(html, 4096,
        "<!DOCTYPE html>\n"
        "<html><head><meta charset='UTF-8'>\n"
        "<style>\n"
        "body{font-family:Georgia,serif;text-align:center;padding:40px;background:#f5f5dc;}\n"
        ".cert{border:8px double #8B4513;padding:40px;max-width:700px;margin:auto;background:#fffaf0;}\n"
        "h1{color:#8B4513;font-size:2.5em;margin-bottom:0;}\n"
        ".subtitle{color:#A0522D;font-style:italic;}\n"
        ".name{font-size:2em;color:#2F4F4F;margin:30px 0;border-bottom:2px solid #8B4513;display:inline-block;padding:0 20px 10px;}\n"
        ".achievement{font-size:1.2em;margin:20px 0;}\n"
        ".details{color:#555;margin:20px 0;}\n"
        ".id{font-size:0.8em;color:#888;margin-top:40px;}\n"
        ".logo{font-size:3em;margin-bottom:20px;}\n"
        "</style></head><body>\n"
        "<div class='cert'>\n"
        "<div class='logo'>🎓</div>\n"
        "<h1>Certificate of Achievement</h1>\n"
        "<p class='subtitle'>Convergio Education</p>\n"
        "<p>This certifies that</p>\n"
        "<p class='name'>%s</p>\n"
        "<p class='achievement'>has successfully completed<br><strong>%s</strong></p>\n"
        "<p class='details'>Subject: %s<br>Topic: %s</p>\n"
        "<p class='details'>Date: %s</p>\n"
        "<p class='id'>Certificate ID: %s</p>\n"
        "</div></body></html>\n",
        profile->name,
        achievement ? achievement : "Course Module",
        subject,
        topic ? topic : "General",
        __DATE__,
        cert_id
    );

    education_profile_free(profile);
    return html;
}

int education_save_certificate(int64_t student_id, const char* html) {
    if (!html) return -1;

    char path[512];
    snprintf(path, sizeof(path), "%s/.convergio/certificates", getenv("HOME"));
    mkdir(path, 0755);

    char filename[640];
    snprintf(filename, sizeof(filename), "%s/cert_%lld_%ld.html",
             path, (long long)student_id, (long)time(NULL));

    FILE* f = fopen(filename, "w");
    if (!f) return -1;

    fputs(html, f);
    fclose(f);
    return 0;
}

// ============================================================================
// PHASE 5: LB14-15 PDF EXPORT
// ============================================================================

char* libretto_export_pdf_report(int64_t student_id, const char* report_type) {
    if (!g_edu_initialized) return NULL;

    EducationStudentProfile* profile = education_profile_get(student_id);
    if (!profile) return NULL;

    // Get grades and progress
    int grade_count = 0;
    EducationGrade** grades = libretto_get_grades(student_id, NULL, 0, 0, &grade_count);

    // Generate HTML that can be printed to PDF
    char* html = malloc(16384);
    if (!html) {
        education_profile_free(profile);
        return NULL;
    }

    char* ptr = html;
    ptr += sprintf(ptr,
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>\n"
        "<style>\n"
        "body{font-family:Arial,sans-serif;padding:20px;max-width:800px;margin:auto;}\n"
        "h1{color:#2c3e50;border-bottom:2px solid #3498db;padding-bottom:10px;}\n"
        "h2{color:#34495e;margin-top:30px;}\n"
        "table{width:100%%;border-collapse:collapse;margin:20px 0;}\n"
        "th,td{border:1px solid #bdc3c7;padding:10px;text-align:left;}\n"
        "th{background:#3498db;color:white;}\n"
        "tr:nth-child(even){background:#ecf0f1;}\n"
        ".summary{background:#e8f6f3;padding:15px;border-radius:8px;margin:20px 0;}\n"
        ".footer{text-align:center;color:#7f8c8d;margin-top:40px;font-size:0.9em;}\n"
        "@media print{body{padding:0;}.no-print{display:none;}}\n"
        "</style></head><body>\n"
        "<h1>📚 Student Report Card</h1>\n"
        "<div class='summary'>\n"
        "<strong>Student:</strong> %s<br>\n"
        "<strong>Report Date:</strong> %s<br>\n"
        "<strong>Report Type:</strong> %s\n"
        "</div>\n",
        profile->name, __DATE__,
        report_type ? report_type : "Complete Report"
    );

    // Add grades table
    if (grade_count > 0) {
        ptr += sprintf(ptr,
            "<h2>Grades</h2>\n"
            "<table><tr><th>Subject</th><th>Topic</th><th>Type</th><th>Grade</th><th>Date</th></tr>\n"
        );

        for (int i = 0; i < grade_count && i < 50; i++) {
            ptr += sprintf(ptr,
                "<tr><td>%s</td><td>%s</td><td>%s</td><td>%.1f</td><td>%s</td></tr>\n",
                grades[i]->subject,
                grades[i]->topic,
                grades[i]->grade_type == GRADE_TYPE_QUIZ ? "Quiz" :
                grades[i]->grade_type == GRADE_TYPE_HOMEWORK ? "Homework" : "Oral",
                grades[i]->grade,
                ctime(&grades[i]->recorded_at)
            );
        }
        ptr += sprintf(ptr, "</table>\n");

        // Calculate average
        float total = 0;
        for (int i = 0; i < grade_count; i++) {
            total += grades[i]->grade;
        }
        float avg = total / (float)grade_count;
        ptr += sprintf(ptr,
            "<div class='summary'><strong>Overall Average:</strong> %.2f/10</div>\n",
            avg
        );

        libretto_grades_free(grades, grade_count);
    }

    ptr += sprintf(ptr,
        "<div class='footer'>\n"
        "Generated by Convergio Education<br>\n"
        "🎓 Learning made personal\n"
        "</div>\n"
        "</body></html>\n"
    );

    education_profile_free(profile);

    // Save to file
    char path[512];
    snprintf(path, sizeof(path), "%s/.convergio/reports", getenv("HOME"));
    mkdir(path, 0755);

    char filename[640];
    snprintf(filename, sizeof(filename), "%s/report_%lld_%ld.html",
             path, (long long)student_id, (long)time(NULL));

    FILE* f = fopen(filename, "w");
    if (f) {
        fputs(html, f);
        fclose(f);
    }

    return html;
}

// ============================================================================
// PHASE 5: LB16 TREND ANALYSIS
// ============================================================================

typedef struct {
    char subject[64];
    float grades[12];  // Last 12 entries
    int grade_count;
    float trend;       // Positive = improving, negative = declining
    float average;
    float best;
    float worst;
} SubjectTrend;

SubjectTrend* libretto_get_trend_analysis(int64_t student_id, const char* subject, int* count) {
    if (!g_edu_initialized || !count) return NULL;
    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql = subject ?
        "SELECT subject, grade FROM student_gradebook WHERE student_id = ? AND subject = ? ORDER BY date DESC LIMIT 100" :
        "SELECT subject, grade FROM student_gradebook WHERE student_id = ? ORDER BY date DESC LIMIT 100";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    if (subject) sqlite3_bind_text(stmt, 2, subject, -1, SQLITE_STATIC);

    // Collect data per subject
    typedef struct { char subject[64]; float grades[50]; int count; } SubjectData;
    SubjectData subjects[20];
    int subject_count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* subj = (const char*)sqlite3_column_text(stmt, 0);
        float grade = (float)sqlite3_column_double(stmt, 1);

        // Find or add subject
        int idx = -1;
        for (int i = 0; i < subject_count; i++) {
            if (strcmp(subjects[i].subject, subj) == 0) {
                idx = i;
                break;
            }
        }
        if (idx < 0 && subject_count < 20) {
            idx = subject_count++;
            strncpy(subjects[idx].subject, subj, 63);
            subjects[idx].count = 0;
        }
        if (idx >= 0 && subjects[idx].count < 50) {
            subjects[idx].grades[subjects[idx].count++] = grade;
        }
    }
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);

    if (subject_count == 0) return NULL;

    SubjectTrend* trends = calloc((size_t)subject_count, sizeof(SubjectTrend));
    if (!trends) return NULL;

    for (int i = 0; i < subject_count; i++) {
        strncpy(trends[i].subject, subjects[i].subject, 63);
        trends[i].grade_count = subjects[i].count > 12 ? 12 : subjects[i].count;

        float total = 0, best = 0, worst = 10;
        for (int j = 0; j < subjects[i].count; j++) {
            if (j < 12) trends[i].grades[j] = subjects[i].grades[j];
            total += subjects[i].grades[j];
            if (subjects[i].grades[j] > best) best = subjects[i].grades[j];
            if (subjects[i].grades[j] < worst) worst = subjects[i].grades[j];
        }
        trends[i].average = total / (float)subjects[i].count;
        trends[i].best = best;
        trends[i].worst = worst;

        // Calculate trend (linear regression slope simplified)
        if (subjects[i].count >= 3) {
            float recent_avg = (subjects[i].grades[0] + subjects[i].grades[1]) / 2.0f;
            float older_avg = (subjects[i].grades[subjects[i].count-1] + subjects[i].grades[subjects[i].count-2]) / 2.0f;
            trends[i].trend = recent_avg - older_avg;  // Positive = improving
        }
    }

    *count = subject_count;
    return trends;
}

void libretto_trend_free(SubjectTrend* trends) {
    free(trends);
}

// ============================================================================
// PHASE 5: LB17 GOALS TRACKING
// ============================================================================

typedef struct {
    int64_t id;
    int64_t student_id;
    char title[128];
    char description[256];
    char subject[64];
    float target_grade;
    float current_progress;
    time_t deadline;
    bool completed;
    time_t created_at;
} LearningGoal;

int64_t libretto_create_goal(int64_t student_id, const char* title,
                              const char* description, const char* subject,
                              float target_grade, time_t deadline) {
    if (!g_edu_initialized || !title) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "INSERT INTO student_goals (student_id, goal_type, title, description, subject, "
        "target_value, deadline, status) VALUES (?, 'grade', ?, ?, ?, ?, ?, 'active')";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, subject, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 5, (double)target_grade);
    sqlite3_bind_int64(stmt, 6, (int64_t)deadline);

    rc = sqlite3_step(stmt);
    int64_t goal_id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(g_edu_db) : -1;
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return goal_id;
}

LearningGoal** libretto_get_goals(int64_t student_id, bool active_only, int* count) {
    if (!g_edu_initialized || !count) return NULL;
    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql = active_only ?
        "SELECT id, student_id, title, description, subject, target_value, current_value, deadline, status, created_at "
        "FROM student_goals WHERE student_id = ? AND status = 'active' ORDER BY deadline" :
        "SELECT id, student_id, title, description, subject, target_value, current_value, deadline, status, created_at "
        "FROM student_goals WHERE student_id = ? ORDER BY deadline";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);

    // Count
    int total = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) total++;
    if (total == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_reset(stmt);
    LearningGoal** goals = calloc((size_t)total, sizeof(LearningGoal*));
    if (!goals) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        LearningGoal* goal = calloc(1, sizeof(LearningGoal));
        if (!goal) continue;

        goal->id = sqlite3_column_int64(stmt, 0);
        goal->student_id = sqlite3_column_int64(stmt, 1);
        const char* title = (const char*)sqlite3_column_text(stmt, 2);
        if (title) strncpy(goal->title, title, 127);
        const char* desc = (const char*)sqlite3_column_text(stmt, 3);
        if (desc) strncpy(goal->description, desc, 255);
        const char* subj = (const char*)sqlite3_column_text(stmt, 4);
        if (subj) strncpy(goal->subject, subj, 63);
        goal->target_grade = (float)sqlite3_column_double(stmt, 5);
        goal->current_progress = (float)sqlite3_column_double(stmt, 6);
        goal->deadline = (time_t)sqlite3_column_int64(stmt, 7);
        const char* status = (const char*)sqlite3_column_text(stmt, 8);
        goal->completed = (status && strcmp(status, "completed") == 0);
        goal->created_at = (time_t)sqlite3_column_int64(stmt, 9);

        goals[i++] = goal;
    }

    *count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return goals;
}

int libretto_complete_goal(int64_t goal_id) {
    if (!g_edu_initialized) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql = "UPDATE student_goals SET status = 'completed', updated_at = ? WHERE id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (int64_t)time(NULL));
    sqlite3_bind_int64(stmt, 2, goal_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

void libretto_goals_free(LearningGoal** goals, int count) {
    if (!goals) return;
    for (int i = 0; i < count; i++) free(goals[i]);
    free(goals);
}

// ============================================================================
// PHASE 5: LB18 ACHIEVEMENT NOTIFICATIONS
// ============================================================================

typedef enum {
    ACHIEVEMENT_FIRST_QUIZ,
    ACHIEVEMENT_PERFECT_SCORE,
    ACHIEVEMENT_STREAK_7,
    ACHIEVEMENT_STREAK_30,
    ACHIEVEMENT_SUBJECT_MASTERY,
    ACHIEVEMENT_GOAL_COMPLETED,
    ACHIEVEMENT_100_FLASHCARDS,
    ACHIEVEMENT_STUDY_HOURS_10,
    ACHIEVEMENT_STUDY_HOURS_50,
    ACHIEVEMENT_ALL_TEACHERS
} AchievementType;

typedef struct {
    AchievementType type;
    char title[64];
    char description[128];
    char icon[8];
    int xp_reward;
    time_t earned_at;
} Achievement;

static const Achievement g_achievements[] = {
    {ACHIEVEMENT_FIRST_QUIZ, "First Steps", "Complete your first quiz", "🎯", 10, 0},
    {ACHIEVEMENT_PERFECT_SCORE, "Perfect!", "Get 100% on any quiz", "⭐", 50, 0},
    {ACHIEVEMENT_STREAK_7, "Week Warrior", "Study 7 days in a row", "🔥", 100, 0},
    {ACHIEVEMENT_STREAK_30, "Monthly Master", "Study 30 days in a row", "💎", 500, 0},
    {ACHIEVEMENT_SUBJECT_MASTERY, "Subject Expert", "Master all topics in a subject", "🏆", 200, 0},
    {ACHIEVEMENT_GOAL_COMPLETED, "Goal Getter", "Complete a learning goal", "🎯", 75, 0},
    {ACHIEVEMENT_100_FLASHCARDS, "Memory Champion", "Review 100 flashcards", "🧠", 50, 0},
    {ACHIEVEMENT_STUDY_HOURS_10, "Dedicated Learner", "Study for 10 hours total", "📚", 100, 0},
    {ACHIEVEMENT_STUDY_HOURS_50, "Study Pro", "Study for 50 hours total", "🎓", 300, 0},
    {ACHIEVEMENT_ALL_TEACHERS, "Renaissance Student", "Learn from all 15 teachers", "🌟", 500, 0}
};

int education_check_achievements(int64_t student_id) {
    if (!g_edu_initialized) return 0;

    int new_achievements = 0;

    // Check various conditions and award achievements
    // This is a simplified check - in production would be more thorough

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Check quiz count
    const char* sql = "SELECT COUNT(*) FROM quiz_history WHERE student_id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int quiz_count = sqlite3_column_int(stmt, 0);
            if (quiz_count >= 1) {
                // Award FIRST_QUIZ achievement
                new_achievements++;
            }
        }
        sqlite3_finalize(stmt);
    }

    // Check perfect scores
    sql = "SELECT COUNT(*) FROM quiz_history WHERE student_id = ? AND score = 100";
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int perfect_count = sqlite3_column_int(stmt, 0);
            if (perfect_count >= 1) {
                new_achievements++;
            }
        }
        sqlite3_finalize(stmt);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);

    return new_achievements;
}

char* education_get_achievement_notification(AchievementType type) {
    if ((int)type >= (int)(sizeof(g_achievements) / sizeof(g_achievements[0]))) {
        return NULL;
    }

    const Achievement* a = &g_achievements[type];
    char* notification = malloc(256);
    if (notification) {
        snprintf(notification, 256,
            "%s Achievement Unlocked: %s!\n%s\n+%d XP",
            a->icon, a->title, a->description, a->xp_reward
        );
    }
    return notification;
}

// ============================================================================
// PHASE 11: MASTERY LEARNING
// ============================================================================

#define MASTERY_THRESHOLD 0.80f  // 80% = mastered

typedef struct {
    char skill_id[64];
    char skill_name[128];
    char subject[64];
    float mastery_level;
    int attempts;
    int correct;
    bool mastered;
    time_t last_practice;
} SkillMastery;

float education_mastery_get_level(int64_t student_id, const char* skill_id) {
    if (!g_edu_initialized || !skill_id) return 0.0f;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "SELECT skill_level, practice_count FROM learning_progress "
        "WHERE student_id = ? AND topic = ? ORDER BY updated_at DESC LIMIT 1";

    sqlite3_stmt* stmt;
    float level = 0.0f;

    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        sqlite3_bind_text(stmt, 2, skill_id, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            level = (float)sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return level;
}

bool education_mastery_is_mastered(int64_t student_id, const char* skill_id) {
    return education_mastery_get_level(student_id, skill_id) >= MASTERY_THRESHOLD;
}

int education_mastery_update(int64_t student_id, const char* skill_id,
                              int correct, int total) {
    if (!g_edu_initialized || !skill_id || total <= 0) return -1;

    float new_score = (float)correct / (float)total;

    // Use exponential moving average
    float current = education_mastery_get_level(student_id, skill_id);
    float alpha = 0.3f;  // Weight for new observations
    float updated = (current > 0) ? (alpha * new_score + (1 - alpha) * current) : new_score;

    return education_progress_record(student_id, "mastery", skill_id, updated, 0);
}

SkillMastery** education_mastery_get_skills(int64_t student_id, const char* subject, int* count) {
    if (!g_edu_initialized || !count) return NULL;
    *count = 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql = subject ?
        "SELECT topic, skill_level, practice_count, updated_at FROM learning_progress "
        "WHERE student_id = ? AND maestro_id = ? ORDER BY topic" :
        "SELECT topic, skill_level, practice_count, updated_at FROM learning_progress "
        "WHERE student_id = ? ORDER BY topic";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, student_id);
    if (subject) sqlite3_bind_text(stmt, 2, subject, -1, SQLITE_STATIC);

    // Count
    int total = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) total++;
    if (total == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    sqlite3_reset(stmt);
    SkillMastery** skills = calloc((size_t)total, sizeof(SkillMastery*));
    if (!skills) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return NULL;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        SkillMastery* skill = calloc(1, sizeof(SkillMastery));
        if (!skill) continue;

        const char* topic = (const char*)sqlite3_column_text(stmt, 0);
        if (topic) {
            strncpy(skill->skill_id, topic, 63);
            strncpy(skill->skill_name, topic, 127);
        }
        skill->mastery_level = (float)sqlite3_column_double(stmt, 1);
        skill->attempts = sqlite3_column_int(stmt, 2);
        skill->mastered = (skill->mastery_level >= MASTERY_THRESHOLD);
        skill->last_practice = (time_t)sqlite3_column_int64(stmt, 3);

        skills[i++] = skill;
    }

    *count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return skills;
}

void education_mastery_skills_free(SkillMastery** skills, int count) {
    if (!skills) return;
    for (int i = 0; i < count; i++) free(skills[i]);
    free(skills);
}

// ============================================================================
// PHASE 11: FSRS SPACED REPETITION (Free Spaced Repetition Scheduler 2024)
// ============================================================================

typedef struct {
    float stability;     // S: time needed for R to drop to 90%
    float difficulty;    // D: difficulty of the card (0.0-1.0)
    float retrievability; // R: probability of recall
    int state;           // 0=new, 1=learning, 2=review, 3=relearning
    int reps;            // Number of reviews
    int lapses;          // Number of times forgotten
    time_t last_review;
    time_t next_review;
} FSRSCard;

// FSRS-4.5 parameters (optimized defaults)
static const float FSRS_W[] = {
    0.4f,   // w0: initial stability for again
    0.6f,   // w1: initial stability for hard
    2.4f,   // w2: initial stability for good
    5.8f,   // w3: initial stability for easy
    4.93f,  // w4: difficulty decay
    0.94f,  // w5: stability decay
    0.86f,  // w6: retrievability coefficient
    0.01f,  // w7: difficulty coefficient
    1.49f,  // w8: stability increase
    0.14f,  // w9: difficulty increase
    0.94f,  // w10: short-term stability
    2.18f,  // w11: long-term stability
    0.05f,  // w12: short-term difficulty
    0.34f,  // w13: long-term difficulty
    1.26f,  // w14: forgetting stability
    0.29f,  // w15: difficulty recovery
    2.61f   // w16: stability recovery
};

__attribute__((unused))
static float fsrs_power_mean(float a, float b, float p) {
    // Power mean for blending stability values (FSRS v5 extension)
    return powf((powf(a, p) + powf(b, p)) / 2.0f, 1.0f / p);
}

float fsrs_calculate_retrievability(const FSRSCard* card, time_t now) {
    if (!card || card->stability <= 0) return 0.0f;

    float elapsed_days = (float)(now - card->last_review) / 86400.0f;
    float factor = 19.0f / card->stability;
    return powf(1.0f + factor * elapsed_days, -1.0f);
}

float fsrs_calculate_next_stability(const FSRSCard* card, int rating) {
    // rating: 1=again, 2=hard, 3=good, 4=easy
    if (!card) return FSRS_W[2];

    if (card->state == 0) {
        // New card
        return FSRS_W[rating - 1];
    }

    float s = card->stability;
    float d = card->difficulty;
    float r = card->retrievability;

    // Stability increase formula
    float s_recall = s * (1.0f + expf(FSRS_W[8]) *
                          (11.0f - d) *
                          powf(s, -FSRS_W[9]) *
                          (expf((1.0f - r) * FSRS_W[10]) - 1.0f));

    // Apply rating factor
    float rating_factor = 1.0f;
    if (rating == 1) rating_factor = FSRS_W[14];
    else if (rating == 2) rating_factor = 0.8f;
    else if (rating == 4) rating_factor = FSRS_W[15];

    return s_recall * rating_factor;
}

float fsrs_calculate_next_difficulty(const FSRSCard* card, int rating) {
    if (!card) return 0.3f;

    float d = card->difficulty;
    float delta = (float)(rating - 3) / 9.0f;

    float new_d = d - FSRS_W[7] * delta;
    if (new_d < 0.0f) new_d = 0.0f;
    if (new_d > 1.0f) new_d = 1.0f;

    return new_d;
}

int fsrs_calculate_interval(float stability) {
    // Days until R drops to 90%
    return (int)(stability * 0.9f * (19.0f / 9.0f));
}

FSRSCard* fsrs_create_card(void) {
    FSRSCard* card = calloc(1, sizeof(FSRSCard));
    if (card) {
        card->stability = 0;
        card->difficulty = 0.3f;
        card->retrievability = 1.0f;
        card->state = 0;  // New
        card->reps = 0;
        card->lapses = 0;
        card->last_review = 0;
        card->next_review = time(NULL);
    }
    return card;
}

int fsrs_review_card(FSRSCard* card, int rating) {
    if (!card || rating < 1 || rating > 4) return -1;

    time_t now = time(NULL);

    // Update retrievability
    card->retrievability = fsrs_calculate_retrievability(card, now);

    // Calculate new stability and difficulty
    float new_stability = fsrs_calculate_next_stability(card, rating);
    float new_difficulty = fsrs_calculate_next_difficulty(card, rating);

    // Update card state
    if (rating == 1) {
        // Again: reset to learning
        card->lapses++;
        card->state = 3;  // Relearning
        new_stability *= 0.5f;  // Halve stability on lapse
    } else if (card->state == 0 || card->state == 1) {
        // Learning -> Review
        if (rating >= 3) card->state = 2;
    }

    card->stability = new_stability;
    card->difficulty = new_difficulty;
    card->reps++;
    card->last_review = now;

    // Calculate next review
    int interval = fsrs_calculate_interval(card->stability);
    if (interval < 1) interval = 1;
    card->next_review = now + (time_t)(interval * 86400);

    return interval;
}

// ============================================================================
// PHASE 11: ENGAGEMENT MECHANICS
// ============================================================================

EducationEngagementStats* education_engagement_get_stats(int64_t student_id) {
    if (!g_edu_initialized) return NULL;

    EducationEngagementStats* stats = calloc(1, sizeof(EducationEngagementStats));
    if (!stats) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    const char* sql =
        "SELECT current_streak, longest_streak, total_xp, level "
        "FROM gamification WHERE student_id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats->current_streak = sqlite3_column_int(stmt, 0);
            stats->longest_streak = sqlite3_column_int(stmt, 1);
            stats->total_xp = sqlite3_column_int(stmt, 2);
            stats->level = sqlite3_column_int(stmt, 3);
        }
        sqlite3_finalize(stmt);
    }

    // Calculate streak freezes (1 per 7 days of streak)
    stats->streak_freezes_available = stats->current_streak / 7;
    stats->has_weekend_amulet = (stats->current_streak >= 14);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return stats;
}

int education_engagement_check_streak(int64_t student_id) {
    if (!g_edu_initialized) return 0;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Check if studied today
    time_t today_start = time(NULL);
    struct tm* tm = localtime(&today_start);
    tm->tm_hour = 0; tm->tm_min = 0; tm->tm_sec = 0;
    today_start = mktime(tm);

    const char* sql =
        "SELECT COUNT(*) FROM learning_sessions WHERE student_id = ? AND started_at >= ?";

    sqlite3_stmt* stmt;
    int studied_today = 0;

    if (sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, student_id);
        sqlite3_bind_int64(stmt, 2, (int64_t)today_start);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            studied_today = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return studied_today;
}

const char* education_engagement_get_celebration(int event_type) {
    static const char* celebrations[] = {
        "🎉 Great job! Keep going!",
        "⭐ You're on fire!",
        "🚀 Excellent work!",
        "💪 You're getting stronger!",
        "🌟 Brilliant!",
        "🎯 Perfect aim!",
        "🏆 Champion!",
        "✨ Amazing!",
        "🔥 Streak master!",
        "💎 Legendary!"
    };

    int idx = event_type % 10;
    return celebrations[idx];
}

int education_engagement_award_xp(int64_t student_id, int xp, const char* reason) {
    if (!g_edu_initialized || xp <= 0) return -1;

    CONVERGIO_MUTEX_LOCK(&g_edu_db_mutex);

    // Update total XP and check level up
    const char* sql =
        "UPDATE gamification SET total_xp = total_xp + ?, "
        "level = (total_xp + ?) / 100 + 1 "
        "WHERE student_id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, xp);
    sqlite3_bind_int(stmt, 2, xp);
    sqlite3_bind_int64(stmt, 3, student_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_edu_db_mutex);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

void education_engagement_free(EducationEngagementStats* stats) {
    free(stats);
}
