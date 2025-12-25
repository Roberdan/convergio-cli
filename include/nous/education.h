/**
 * @file education.h
 * @brief MyConvergio Education Pack - Public API
 *
 * Education module providing historical master teachers with accessibility
 * adaptations and comprehensive didactic toolkit for students.
 *
 * @see docs/adr/015-education-pack.md
 * @see docs/plans/EducationPackPlan.md
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#ifndef NOUS_EDUCATION_H
#define NOUS_EDUCATION_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONSTANTS
// ============================================================================

#define EDUCATION_MAX_NAME_LEN 64
#define EDUCATION_MAX_CURRICULUM_LEN 128
#define EDUCATION_MAX_NOTES_LEN 1024
#define EDUCATION_MAX_TOPIC_LEN 256
#define EDUCATION_MAX_MAESTRI 14

// Maestro IDs
#define MAESTRO_SOCRATE     "ED01"
#define MAESTRO_EUCLIDE     "ED02"
#define MAESTRO_FEYNMAN     "ED03"
#define MAESTRO_ERODOTO     "ED04"
#define MAESTRO_HUMBOLDT    "ED05"
#define MAESTRO_MANZONI     "ED06"
#define MAESTRO_DARWIN      "ED07"
#define MAESTRO_LEONARDO    "ED08"
#define MAESTRO_MOZART      "ED09"
#define MAESTRO_SHAKESPEARE "ED10"
#define MAESTRO_CICERONE    "ED11"
#define MAESTRO_SMITH       "ED12"
#define MAESTRO_LOVELACE    "ED13"
#define MAESTRO_IPPOCRATE   "ED14"

// ============================================================================
// ENUMS
// ============================================================================

/**
 * @brief Severity levels for accessibility conditions
 */
typedef enum {
    SEVERITY_NONE = 0,
    SEVERITY_MILD = 1,
    SEVERITY_MODERATE = 2,
    SEVERITY_SEVERE = 3
} EducationSeverity;

/**
 * @brief ADHD subtypes
 */
typedef enum {
    ADHD_NONE = 0,
    ADHD_INATTENTIVE = 1,
    ADHD_HYPERACTIVE = 2,
    ADHD_COMBINED = 3
} EducationAdhdType;

/**
 * @brief Preferred input method
 */
typedef enum {
    INPUT_KEYBOARD = 0,
    INPUT_VOICE = 1,
    INPUT_BOTH = 2,
    INPUT_TOUCH = 3,
    INPUT_SWITCH = 4,
    INPUT_EYE_TRACKING = 5
} EducationInputMethod;

/**
 * @brief Preferred output method
 */
typedef enum {
    OUTPUT_TEXT = 0,
    OUTPUT_TTS = 1,
    OUTPUT_BOTH = 2,
    OUTPUT_VISUAL = 3,
    OUTPUT_AUDIO = 4,
    OUTPUT_BRAILLE = 5,
    OUTPUT_HAPTIC = 6
} EducationOutputMethod;

/**
 * @brief Goal types
 */
typedef enum {
    GOAL_SHORT_TERM = 0,
    GOAL_MEDIUM_TERM = 1,
    GOAL_LONG_TERM = 2
} EducationGoalType;

/**
 * @brief Goal status
 */
typedef enum {
    GOAL_ACTIVE = 0,
    GOAL_ACHIEVED = 1,
    GOAL_ABANDONED = 2
} EducationGoalStatus;

/**
 * @brief Toolkit output types
 */
typedef enum {
    TOOLKIT_MINDMAP = 0,
    TOOLKIT_QUIZ = 1,
    TOOLKIT_FLASHCARD = 2,
    TOOLKIT_AUDIO = 3,
    TOOLKIT_NOTE = 4,
    TOOLKIT_SUMMARY = 5,
    TOOLKIT_FORMULA = 6,
    TOOLKIT_GRAPH = 7,
    TOOLKIT_FLOWCHART = 8,
    TOOLKIT_TIMELINE = 9
} EducationToolkitType;

// ============================================================================
// STRUCTURES
// ============================================================================

/**
 * @brief Accessibility settings for a student
 *
 * All fields are dynamically managed by the database layer.
 */
typedef struct {
    // Conditions - severity levels and flags
    bool dyslexia;
    EducationSeverity dyslexia_severity;
    bool dyscalculia;
    EducationSeverity dyscalculia_severity;
    bool cerebral_palsy;
    EducationSeverity cerebral_palsy_severity;
    bool adhd;
    EducationAdhdType adhd_type;
    EducationSeverity adhd_severity;
    bool autism;
    EducationSeverity autism_severity;
    bool visual_impairment;
    bool hearing_impairment;

    // Preferences
    EducationInputMethod preferred_input;
    EducationOutputMethod preferred_output;
    bool tts_enabled;
    float tts_speed;  // 0.5 - 2.0
    float tts_pitch;  // -1.0 to 1.0 (0.0 = default)
    char* tts_voice;  // Voice name for TTS (dynamically allocated)
    bool high_contrast;
    bool reduce_motion;
} EducationAccessibility;

/**
 * @brief Student profile
 *
 * All string fields are dynamically allocated. Caller must use
 * education_profile_free() to properly release memory.
 */
typedef struct {
    int64_t id;
    char* name;                   // Dynamically allocated
    int age;
    int grade_level;              // 1-13 (elementari through liceo)
    char* curriculum_id;          // Dynamically allocated
    char* parent_name;            // Dynamically allocated
    char* parent_email;           // Dynamically allocated
    char* preferred_language;     // Dynamically allocated (default: "it")
    char* study_method;           // Dynamically allocated
    EducationAccessibility* accessibility;  // Pointer to accessibility settings
    bool is_active;
    time_t created_at;
    time_t updated_at;
    time_t last_session_at;
} EducationStudentProfile;

/**
 * @brief Student goal
 */
typedef struct {
    int64_t id;
    int64_t student_id;
    EducationGoalType goal_type;
    char description[EDUCATION_MAX_NOTES_LEN];
    time_t target_date;
    EducationGoalStatus status;
    time_t created_at;
} EducationGoal;

/**
 * @brief Learning progress for a topic
 *
 * All string fields are dynamically allocated. Use education_progress_free()
 * to release memory.
 */
typedef struct {
    int64_t id;
    int64_t student_id;
    char* maestro_id;             // Dynamically allocated
    char* subject;                // Dynamically allocated
    char* topic;                  // Dynamically allocated
    char* subtopic;               // Dynamically allocated
    float skill_level;            // 0.0 - 1.0
    float confidence;             // 0.0 - 1.0
    int total_time_spent;         // Total time in minutes
    int interaction_count;
    float quiz_score_avg;
    time_t last_interaction;
} EducationProgress;

/**
 * @brief Learning session record
 */
typedef struct {
    int64_t id;
    int64_t student_id;
    char maestro_id[32];
    char topic[EDUCATION_MAX_TOPIC_LEN];
    time_t started_at;
    time_t ended_at;
    int duration_minutes;
    float engagement_score;  // 0.0 - 1.0
    float comprehension_score;  // 0.0 - 1.0
    char notes[EDUCATION_MAX_NOTES_LEN];
} EducationSession;

/**
 * @brief Toolkit output (saved mind maps, quizzes, etc.)
 */
typedef struct {
    int64_t id;
    int64_t student_id;
    EducationToolkitType tool_type;
    char topic[EDUCATION_MAX_TOPIC_LEN];
    char* content;  // Dynamic allocation
    char format[32];
    time_t created_at;
    time_t last_accessed;
} EducationToolkitOutput;

/**
 * @brief Flashcard review data for spaced repetition
 */
typedef struct {
    int64_t id;
    int64_t toolkit_output_id;
    int card_index;
    float ease_factor;  // SM-2 ease factor (default 2.5)
    int interval_days;
    time_t next_review;
    int review_count;
    int last_quality;  // 0-5 quality rating
} EducationFlashcardReview;

// ============================================================================
// LIBRETTO DELLO STUDENTE - STRUCTURES
// ============================================================================

/**
 * @brief Grade type
 */
typedef enum {
    GRADE_TYPE_QUIZ = 0,        // Quiz/test result
    GRADE_TYPE_HOMEWORK = 1,    // Homework evaluation
    GRADE_TYPE_ORAL = 2,        // Oral examination
    GRADE_TYPE_PROJECT = 3,     // Project evaluation
    GRADE_TYPE_PARTICIPATION = 4 // Class participation
} EducationGradeType;

/**
 * @brief A grade entry in the student gradebook
 */
typedef struct {
    int64_t id;
    int64_t student_id;
    char maestro_id[32];         // ED01-ED14
    char subject[64];           // Subject name
    char topic[EDUCATION_MAX_TOPIC_LEN];
    EducationGradeType grade_type;
    float grade;                // 1.0 - 10.0 (Italian system)
    float grade_percentage;     // 0-100 for quizzes
    char comment[EDUCATION_MAX_NOTES_LEN];  // Teacher feedback
    int questions_total;        // For quiz: total questions
    int questions_correct;      // For quiz: correct answers
    time_t recorded_at;
} EducationGrade;

/**
 * @brief Daily log entry for activity tracking
 */
typedef struct {
    int64_t id;
    int64_t student_id;
    char maestro_id[32];
    char subject[64];
    char activity_type[32];     // "study", "quiz", "homework", "flashcards", etc.
    char topic[EDUCATION_MAX_TOPIC_LEN];
    char notes[EDUCATION_MAX_NOTES_LEN];
    int duration_minutes;
    int xp_earned;
    time_t started_at;
    time_t ended_at;
} EducationDailyLogEntry;

/**
 * @brief Subject average with trend
 */
typedef struct {
    char subject[64];
    char maestro_id[32];
    float average_grade;
    int grade_count;
    int total_study_minutes;
    float trend;                // Positive = improving, negative = declining
    float last_30_days_avg;
    float previous_30_days_avg;
} EducationSubjectStats;

/**
 * @brief Progress report summary
 */
typedef struct {
    int64_t student_id;
    char student_name[EDUCATION_MAX_NAME_LEN];
    time_t period_start;
    time_t period_end;
    float overall_average;
    int total_study_hours;
    int total_sessions;
    int quizzes_taken;
    int goals_achieved;
    int current_streak;
    int subject_count;
    EducationSubjectStats* subjects;  // Array of subject stats
} EducationProgressReport;

/**
 * @brief Curriculum subject definition
 */
typedef struct {
    char id[32];
    char maestro_id[32];  // e.g., "shakespeare-inglese", "socrate-filosofia"
    int hours_per_week;
    int topic_count;
    char** topics;  // Array of topic strings
} EducationSubject;

/**
 * @brief Curriculum definition
 */
typedef struct {
    char id[EDUCATION_MAX_CURRICULUM_LEN];
    char name[EDUCATION_MAX_NAME_LEN];
    char country[8];
    int year;
    int subject_count;
    EducationSubject* subjects;
} EducationCurriculum;

/**
 * @brief Options for creating a student profile
 */
typedef struct {
    const char* name;
    int age;
    int grade_level;              // 1-13
    const char* curriculum_id;
    const char* parent_name;
    const char* parent_email;
    EducationAccessibility* accessibility;
} EducationCreateOptions;

/**
 * @brief Options for updating a student profile
 * Same as EducationCreateOptions - NULL fields are not updated
 */
typedef EducationCreateOptions EducationUpdateOptions;

/**
 * @brief Filter options for listing progress
 */
typedef struct {
    int64_t student_id;
    const char* maestro_id;
    float min_skill_level;
    time_t since;
} EducationProgressFilter;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * @brief Initialize the education module
 * @return 0 on success, -1 on error
 */
int education_init(void);

/**
 * @brief Shutdown the education module
 */
void education_shutdown(void);

/**
 * @brief Check if education module is ready
 * @return true if initialized
 */
bool education_is_ready(void);

// ============================================================================
// STUDENT PROFILE API
// ============================================================================

/**
 * @brief Create a new student profile
 * @param options Profile creation options
 * @return Profile ID on success, -1 on error
 */
int64_t education_profile_create(const EducationCreateOptions* options);

/**
 * @brief Get a student profile by ID
 * @param id Profile ID
 * @return Profile or NULL if not found (caller must free)
 */
EducationStudentProfile* education_profile_get(int64_t id);

/**
 * @brief Get the active student profile
 * @return Active profile or NULL (caller must free)
 */
EducationStudentProfile* education_profile_get_active(void);

/**
 * @brief Set the active student profile
 * @param id Profile ID to activate
 * @return 0 on success, -1 on error
 */
int education_profile_set_active(int64_t id);

/**
 * @brief Update a student profile
 * @param id Profile ID
 * @param options Updated options (NULL fields are not updated)
 * @return 0 on success, -1 on error
 */
int education_profile_update(int64_t id, const EducationCreateOptions* options);

/**
 * @brief Delete a student profile
 * @param id Profile ID
 * @return 0 on success, -1 on error
 */
int education_profile_delete(int64_t id);

/**
 * @brief List all student profiles
 * @param count Output: number of profiles
 * @return Array of profiles (caller must free)
 */
EducationStudentProfile** education_profile_list(int* count);

/**
 * @brief Free a student profile
 * @param profile Profile to free
 */
void education_profile_free(EducationStudentProfile* profile);

/**
 * @brief Free a list of student profiles
 * @param profiles Array of profiles
 * @param count Number of profiles
 */
void education_profile_list_free(EducationStudentProfile** profiles, int count);

/**
 * @brief Get the number of student profiles
 * @return Number of profiles (0 if none or error)
 */
int education_profile_count(void);

/**
 * @brief Check if this is the first run (no profiles exist)
 * @return true if no profiles exist, false otherwise
 */
bool education_is_first_run(void);

// ============================================================================
// ACCESSIBILITY API
// ============================================================================

/**
 * @brief Get accessibility settings for a student
 * @param student_id Student profile ID
 * @return Accessibility settings or NULL (caller must free)
 */
EducationAccessibility* education_accessibility_get(int64_t student_id);

/**
 * @brief Update accessibility settings
 * @param student_id Student profile ID
 * @param settings New settings
 * @return 0 on success, -1 on error
 */
int education_accessibility_update(int64_t student_id, const EducationAccessibility* settings);

/**
 * @brief Get adapted text for a student's accessibility needs
 * @param student_id Student profile ID
 * @param text Original text
 * @return Adapted text (caller must free)
 */
char* education_accessibility_adapt_text(int64_t student_id, const char* text);

/**
 * @brief Check if TTS should be used for a student
 * @param student_id Student profile ID
 * @return true if TTS preferred
 */
bool education_accessibility_wants_tts(int64_t student_id);

/**
 * @brief Get recommended font settings for a student
 * @param student_id Student profile ID
 * @param font_family Output: font family name
 * @param font_size Output: font size
 * @return 0 on success, -1 on error
 */
int education_accessibility_get_font(int64_t student_id, char* font_family, int* font_size);

// ============================================================================
// ACCESSIBILITY RUNTIME ADAPTATIONS
// ============================================================================

// Dyslexia (DY01-07)
const char* a11y_get_font(const EducationAccessibility* access);
float a11y_get_line_spacing(const EducationAccessibility* access);
int a11y_get_max_line_width(const EducationAccessibility* access);
char* a11y_wrap_text(const char* text, int max_width);
const char* a11y_get_background_color(const EducationAccessibility* access);
const char* a11y_get_background_ansi(const EducationAccessibility* access);
bool a11y_wants_tts_highlight(const EducationAccessibility* access);
char* a11y_syllabify_word(const char* word);
char* a11y_syllabify_text(const char* text);

// Dyscalculia (DC01-06)
char* a11y_format_number_colored(double number, bool use_colors);
char* a11y_generate_place_value_blocks(int number);
bool a11y_disable_math_timer(const EducationAccessibility* access);

// Cerebral Palsy (CP01-05)
bool a11y_prefers_voice_input(const EducationAccessibility* access);
int a11y_get_timeout_multiplier(const EducationAccessibility* access);
int a11y_get_adjusted_timeout(const EducationAccessibility* access, int base_timeout);
bool a11y_suggest_break(const EducationAccessibility* access, int minutes_elapsed);

// ADHD (AD01-06)
int a11y_get_max_bullets(const EducationAccessibility* access);
char* a11y_limit_bullets(const char* text, int max_bullets);
char* a11y_generate_progress_bar(int current, int total, int width);
const char* a11y_get_celebration_message(int achievement_level);
bool a11y_enhance_gamification(const EducationAccessibility* access);

// Autism (AU01-06)
bool a11y_avoid_metaphors(const EducationAccessibility* access);
bool a11y_contains_metaphors(const char* text);
const char* a11y_get_structure_prefix(const char* section_type);
char* a11y_get_topic_change_warning(const char* old_topic, const char* new_topic);
bool a11y_avoid_social_pressure(const EducationAccessibility* access);
bool a11y_reduce_motion(const EducationAccessibility* access);

// Combined
char* a11y_adapt_text_full(const char* text, const EducationAccessibility* access);

// ============================================================================
// GOALS API
// ============================================================================

/**
 * @brief Add a goal for a student
 * @param student_id Student profile ID
 * @param goal_type Goal type
 * @param description Goal description
 * @param target_date Target date (0 for no deadline)
 * @return Goal ID on success, -1 on error
 */
int64_t education_goal_add(int64_t student_id, EducationGoalType goal_type,
                           const char* description, time_t target_date);

/**
 * @brief Get goals for a student
 * @param student_id Student profile ID
 * @param count Output: number of goals
 * @return Array of goals (caller must free)
 */
EducationGoal** education_goal_list(int64_t student_id, int* count);

/**
 * @brief Mark a goal as achieved
 * @param goal_id Goal ID
 * @return 0 on success, -1 on error
 */
int education_goal_achieve(int64_t goal_id);

/**
 * @brief Delete a goal
 * @param goal_id Goal ID
 * @return 0 on success, -1 on error
 */
int education_goal_delete(int64_t goal_id);

/**
 * @brief Free a list of goals
 * @param goals Array of goals
 * @param count Number of goals
 */
void education_goal_list_free(EducationGoal** goals, int count);

// ============================================================================
// PROGRESS API
// ============================================================================

/**
 * @brief Record learning progress for a topic
 * @param student_id Student profile ID
 * @param maestro_id Maestro ID (ED01, ED02, etc.)
 * @param topic Topic name
 * @param skill_level New skill level (0.0 - 1.0)
 * @param time_spent Time spent in minutes
 * @return 0 on success, -1 on error
 */
int education_progress_record(int64_t student_id, const char* maestro_id,
                              const char* topic, float skill_level, int time_spent);

/**
 * @brief Get progress for a specific topic
 * @param student_id Student profile ID
 * @param maestro_id Maestro ID
 * @param topic Topic name
 * @return Progress or NULL if not found (caller must free)
 */
EducationProgress* education_progress_get(int64_t student_id, const char* topic);

/**
 * @brief List progress entries with filter
 * @param filter Filter options
 * @param count Output: number of entries
 * @return Array of progress entries (caller must free)
 */
EducationProgress** education_progress_list(const EducationProgressFilter* filter, int* count);

/**
 * @brief Record quiz result
 * @param student_id Student profile ID
 * @param maestro_id Maestro ID
 * @param topic Topic name
 * @param correct Number of correct answers
 * @param total Total number of questions
 * @return 0 on success, -1 on error
 */
int education_progress_record_quiz(int64_t student_id, const char* maestro_id,
                                   const char* topic, int correct, int total);

/**
 * @brief Free a progress entry
 * @param progress Progress to free
 */
void education_progress_free(EducationProgress* progress);

/**
 * @brief Free a list of progress entries
 * @param progress Array of progress entries
 * @param count Number of entries
 */
void education_progress_list_free(EducationProgress** progress, int count);

// ============================================================================
// SESSION API
// ============================================================================

/**
 * @brief Start a learning session
 * @param student_id Student profile ID
 * @param maestro_id Maestro ID
 * @param topic Topic name
 * @return Session ID on success, -1 on error
 */
int64_t education_session_start(int64_t student_id, const char* session_type,
                                const char* subject, const char* topic);

/**
 * @brief End a learning session
 * @param session_id Session ID
 * @param xp_earned XP earned during session
 * @return 0 on success, -1 on error
 */
int education_session_end(int64_t session_id, int xp_earned);

/**
 * @brief Get recent sessions for a student
 * @param student_id Student profile ID
 * @param limit Maximum number of sessions
 * @param count Output: number of sessions
 * @return Array of sessions (caller must free)
 */
EducationSession** education_session_list(int64_t student_id, int limit, int* count);

/**
 * @brief Free a list of sessions
 * @param sessions Array of sessions
 * @param count Number of sessions
 */
void education_session_list_free(EducationSession** sessions, int count);

// ============================================================================
// CURRICULUM API
// ============================================================================

/**
 * @brief Load a curriculum from JSON file
 * @param curriculum_id Curriculum ID (e.g., "it_liceo_scientifico_1")
 * @return Curriculum or NULL on error (caller must free)
 */
EducationCurriculum* education_curriculum_load(const char* curriculum_id);

/**
 * @brief List available curricula
 * @param count Output: number of curricula
 * @return Array of curriculum IDs (caller must free)
 */
char** education_curriculum_list(int* count);

/**
 * @brief Get subjects for a curriculum year
 * @param curriculum_id Curriculum ID
 * @param year Year (1-5)
 * @param count Output: number of subjects
 * @return Array of subjects (caller must free)
 */
EducationSubject** education_curriculum_get_subjects(const char* curriculum_id, int year, int* count);

/**
 * @brief Free a curriculum
 * @param curriculum Curriculum to free
 */
void education_curriculum_free(EducationCurriculum* curriculum);

/**
 * @brief Free a list of curriculum IDs
 * @param curricula Array of curriculum IDs
 * @param count Number of IDs
 */
void education_curriculum_list_free(char** curricula, int count);

// ============================================================================
// TOOLKIT API
// ============================================================================

/**
 * @brief Save a toolkit output (mind map, quiz, etc.)
 * @param student_id Student profile ID
 * @param type Tool type
 * @param topic Topic name
 * @param content Output content
 * @param format Output format (e.g., "mermaid", "json", "m4a")
 * @return Output ID on success, -1 on error
 */
int64_t education_toolkit_save(int64_t student_id, EducationToolkitType type,
                               const char* topic, const char* content, const char* format);

/**
 * @brief Get a toolkit output by ID
 * @param output_id Output ID
 * @return Toolkit output or NULL (caller must free)
 */
EducationToolkitOutput* education_toolkit_get(int64_t output_id);

/**
 * @brief List toolkit outputs for a student
 * @param student_id Student profile ID
 * @param type Tool type (or -1 for all types)
 * @param count Output: number of outputs
 * @return Array of outputs (caller must free)
 */
EducationToolkitOutput** education_toolkit_list(int64_t student_id, int type, int* count);

/**
 * @brief Delete a toolkit output
 * @param output_id Output ID
 * @return 0 on success, -1 on error
 */
int education_toolkit_delete(int64_t output_id);

/**
 * @brief Free a toolkit output
 * @param output Output to free
 */
void education_toolkit_free(EducationToolkitOutput* output);

/**
 * @brief Free a list of toolkit outputs
 * @param outputs Array of outputs
 * @param count Number of outputs
 */
void education_toolkit_list_free(EducationToolkitOutput** outputs, int count);

// ============================================================================
// SPACED REPETITION API (SM-2 Algorithm)
// ============================================================================

/**
 * @brief Get next flashcard for review
 * @param student_id Student profile ID
 * @return Flashcard review data or NULL if none due (caller must free)
 */
EducationFlashcardReview* education_flashcard_next(int64_t student_id);

/**
 * @brief Record flashcard review result
 * @param review_id Review ID
 * @param quality Quality rating (0-5, where 5 is perfect recall)
 * @return 0 on success, -1 on error
 */
int education_flashcard_review(int64_t review_id, int quality);

/**
 * @brief Get count of flashcards due for review
 * @param student_id Student profile ID
 * @return Number of cards due
 */
int education_flashcard_due_count(int64_t student_id);

/**
 * @brief Create flashcard reviews from a toolkit output
 * @param toolkit_output_id Toolkit output ID (must be TOOLKIT_FLASHCARD type)
 * @param card_count Number of cards in the deck
 * @return 0 on success, -1 on error
 */
int education_flashcard_create_reviews(int64_t toolkit_output_id, int card_count);

/**
 * @brief Free a flashcard review
 * @param review Review to free
 */
void education_flashcard_free(EducationFlashcardReview* review);

// ============================================================================
// MAESTRO API
// ============================================================================

/**
 * @brief Get maestro info by ID
 * @param maestro_id Maestro ID (ED01, ED02, etc.)
 * @param name Output: maestro name
 * @param subject Output: subject name
 * @return 0 on success, -1 on error
 */
int education_maestro_get_info(const char* maestro_id, char* name, char* subject);

/**
 * @brief Get all maestri for a curriculum
 * @param curriculum_id Curriculum ID
 * @param count Output: number of maestri
 * @return Array of maestro IDs (caller must free)
 */
char** education_maestro_list_for_curriculum(const char* curriculum_id, int* count);

/**
 * @brief Broadcast student profile to all maestri
 * @param student_id Student profile ID
 * @return 0 on success, -1 on error
 */
int education_maestro_broadcast_profile(int64_t student_id);

// ============================================================================
// SETUP WIZARD API
// ============================================================================

/**
 * @brief Start the setup wizard
 * @return 0 on success, -1 on error
 */
int education_setup_start(void);

/**
 * @brief Check if setup is complete
 * @return true if a student profile exists
 */
bool education_setup_is_complete(void);

// ============================================================================
// ADAPTIVE LEARNING API (S18)
// ============================================================================

/**
 * @brief Analyze student learning patterns and return insights
 * @param student_id Student profile ID
 * @return JSON string with analysis (caller must free), NULL on error
 */
char* education_adaptive_analyze(int64_t student_id);

/**
 * @brief Update student profile based on adaptive analysis
 * @param student_id Student profile ID
 * @return 0 on success, -1 on error
 */
int education_adaptive_update_profile(int64_t student_id);

/**
 * @brief Suggest next topic based on learning progress
 * @param student_id Student profile ID
 * @param subject Subject name
 * @return JSON string with next topic suggestion (caller must free), NULL on error
 */
char* education_adaptive_next_topic(int64_t student_id, const char* subject);

// ============================================================================
// LIBRETTO DELLO STUDENTE API
// ============================================================================

/**
 * @brief Add a grade to the student gradebook
 * @param student_id Student profile ID
 * @param maestro_id Maestro ID (ED01-ED14)
 * @param subject Subject name
 * @param topic Topic name
 * @param grade_type Type of grade (quiz, homework, oral, etc.)
 * @param grade Grade value (1.0-10.0 Italian system)
 * @param comment Teacher feedback/comment
 * @return Grade ID on success, -1 on error
 */
int64_t libretto_add_grade(int64_t student_id, const char* maestro_id,
                           const char* subject, const char* topic,
                           EducationGradeType grade_type, float grade,
                           const char* comment);

/**
 * @brief Add a grade from a quiz result
 * @param student_id Student profile ID
 * @param maestro_id Maestro ID
 * @param subject Subject name
 * @param topic Topic name
 * @param correct Number of correct answers
 * @param total Total number of questions
 * @param comment Optional comment
 * @return Grade ID on success, -1 on error
 */
int64_t libretto_add_quiz_grade(int64_t student_id, const char* maestro_id,
                                const char* subject, const char* topic,
                                int correct, int total, const char* comment);

/**
 * @brief Add a daily log entry
 * @param student_id Student profile ID
 * @param maestro_id Maestro ID (or NULL for general activity)
 * @param activity_type Activity type ("study", "quiz", "homework", etc.)
 * @param subject Subject name (or NULL)
 * @param topic Topic name (or NULL)
 * @param duration_minutes Duration in minutes
 * @param notes Optional notes
 * @return Log entry ID on success, -1 on error
 */
int64_t libretto_add_log_entry(int64_t student_id, const char* maestro_id,
                               const char* activity_type, const char* subject,
                               const char* topic, int duration_minutes,
                               const char* notes);

/**
 * @brief Get grades for a student, optionally filtered by subject and period
 * @param student_id Student profile ID
 * @param subject Subject name filter (NULL for all subjects)
 * @param from_date Start date filter (0 for no limit)
 * @param to_date End date filter (0 for no limit)
 * @param count Output: number of grades
 * @return Array of grades (caller must free with libretto_grades_free)
 */
EducationGrade** libretto_get_grades(int64_t student_id, const char* subject,
                                     time_t from_date, time_t to_date, int* count);

/**
 * @brief Get daily log entries for a student
 * @param student_id Student profile ID
 * @param from_date Start date filter (0 for no limit)
 * @param to_date End date filter (0 for no limit)
 * @param count Output: number of entries
 * @return Array of log entries (caller must free with libretto_logs_free)
 */
EducationDailyLogEntry** libretto_get_daily_log(int64_t student_id,
                                                 time_t from_date, time_t to_date,
                                                 int* count);

/**
 * @brief Get average grade for a subject
 * @param student_id Student profile ID
 * @param subject Subject name (NULL for overall average)
 * @param from_date Start date filter (0 for no limit)
 * @param to_date End date filter (0 for no limit)
 * @return Average grade, or -1 on error
 */
float libretto_get_average(int64_t student_id, const char* subject,
                           time_t from_date, time_t to_date);

/**
 * @brief Get comprehensive progress report
 * @param student_id Student profile ID
 * @param from_date Period start (0 for last 30 days)
 * @param to_date Period end (0 for now)
 * @return Progress report (caller must free with libretto_report_free)
 */
EducationProgressReport* libretto_get_progress_report(int64_t student_id,
                                                       time_t from_date,
                                                       time_t to_date);

/**
 * @brief Get study time statistics per subject
 * @param student_id Student profile ID
 * @param from_date Start date (0 for all time)
 * @param to_date End date (0 for now)
 * @param count Output: number of subjects
 * @return Array of subject stats (caller must free)
 */
EducationSubjectStats** libretto_get_study_stats(int64_t student_id,
                                                  time_t from_date, time_t to_date,
                                                  int* count);

/**
 * @brief Free a grades array
 * @param grades Array of grades
 * @param count Number of grades
 */
void libretto_grades_free(EducationGrade** grades, int count);

/**
 * @brief Free a daily log array
 * @param logs Array of log entries
 * @param count Number of entries
 */
void libretto_logs_free(EducationDailyLogEntry** logs, int count);

/**
 * @brief Free a progress report
 * @param report Report to free
 */
void libretto_report_free(EducationProgressReport* report);

/**
 * @brief Free a subject stats array
 * @param stats Array of stats
 * @param count Number of stats
 */
void libretto_stats_free(EducationSubjectStats** stats, int count);

// ============================================================================
// ALI PRESIDE API (FASE 7 - School Principal Coordination)
// ============================================================================

/**
 * @brief Statistics for a single maestro/subject
 */
typedef struct {
    char maestro_id[32];
    char maestro_name[32];
    char subject[32];
    float average_grade;
    int grade_count;
    float trend;
    int study_minutes;
    int session_count;
} PresideMaestroStats;

/**
 * @brief Complete student dashboard for the preside
 */
typedef struct {
    int64_t student_id;
    char student_name[64];
    float overall_average;
    int total_study_hours;
    int total_sessions;
    int goals_achieved;
    int goals_pending;
    int current_streak;
    PresideMaestroStats* maestro_stats;
    int maestro_count;
    char* concerns;
    char* strengths;
} PresideStudentDashboard;

/**
 * @brief Types of student concerns
 */
typedef enum {
    PRESIDE_CONCERN_LOW_GRADE = 0,
    PRESIDE_CONCERN_DECLINING_TREND = 1,
    PRESIDE_CONCERN_LOW_ENGAGEMENT = 2,
    PRESIDE_CONCERN_MISSED_GOALS = 3,
    PRESIDE_CONCERN_BREAK_STREAK = 4
} PresideConcernType;

/**
 * @brief A specific student concern
 */
typedef struct {
    PresideConcernType type;
    char subject[32];
    char description[256];
    int severity;
    time_t detected_at;
} PresideStudentConcern;

/**
 * @brief A difficult case requiring escalation
 */
typedef struct {
    int64_t student_id;
    char student_name[64];
    PresideStudentConcern* concerns;
    int concern_count;
} PresideDifficultCase;

/**
 * @brief Virtual class council session
 */
typedef struct {
    int64_t student_id;
    char student_name[64];
    char* agenda;
    char* discussion_points;
    char* recommendations;
    time_t scheduled_at;
} PresideClassCouncil;

/**
 * @brief Get comprehensive student dashboard for preside (AL02)
 * @param student_id Student profile ID
 * @return Dashboard or NULL (caller must free with preside_dashboard_free)
 */
PresideStudentDashboard* preside_get_dashboard(int64_t student_id);

/**
 * @brief Free a student dashboard
 * @param dashboard Dashboard to free
 */
void preside_dashboard_free(PresideStudentDashboard* dashboard);

/**
 * @brief Print dashboard to console (ASCII format)
 * @param dashboard Dashboard to print
 */
void preside_print_dashboard(const PresideStudentDashboard* dashboard);

/**
 * @brief Prepare virtual class council for student (AL03)
 * @param student_id Student profile ID
 * @return Council or NULL (caller must free with preside_class_council_free)
 */
PresideClassCouncil* preside_prepare_class_council(int64_t student_id);

/**
 * @brief Free a class council
 * @param council Council to free
 */
void preside_class_council_free(PresideClassCouncil* council);

/**
 * @brief Generate automatic weekly report (AL04)
 * @param student_id Student profile ID
 * @return Report string (caller must free)
 */
char* preside_generate_weekly_report(int64_t student_id);

/**
 * @brief Detect difficult cases requiring escalation (AL05)
 * @param student_id Student profile ID
 * @return Difficult case or NULL if no concerns (caller must free)
 */
PresideDifficultCase* preside_detect_difficult_case(int64_t student_id);

/**
 * @brief Free a difficult case
 * @param dc Difficult case to free
 */
void preside_difficult_case_free(PresideDifficultCase* dc);

/**
 * @brief Generate parent communication message (AL06)
 * @param student_id Student profile ID
 * @param include_concerns Whether to include concerns in message
 * @return Message string (caller must free)
 */
char* preside_generate_parent_message(int64_t student_id, bool include_concerns);

/**
 * @brief Get shared context for maestri about student (CM01)
 * @param student_id Student profile ID
 * @return Context string (caller must free)
 */
char* preside_get_shared_context(int64_t student_id);

/**
 * @brief Suggest interdisciplinary connections for topic (CM02-03)
 * @param student_id Student profile ID
 * @param topic Topic to analyze
 * @return Suggestion string (caller must free)
 */
char* preside_suggest_interdisciplinary(int64_t student_id, const char* topic);

// ============================================================================
// HTML INTERACTIVE GENERATOR API (TK85-TK96)
// ============================================================================

/**
 * @brief Types of HTML content that can be generated
 */
typedef enum {
    HTML_CONTENT_GENERIC = 0,     /**< Generic lesson page */
    HTML_CONTENT_GEOMETRY = 1,    /**< Euclide: Interactive geometry (Canvas/SVG) */
    HTML_CONTENT_PHYSICS = 2,     /**< Feynman: Physics simulations */
    HTML_CONTENT_TIMELINE = 3,    /**< Erodoto: Interactive timelines */
    HTML_CONTENT_MUSIC = 4,       /**< Mozart: Sheet music with playback */
    HTML_CONTENT_ART = 5,         /**< Leonardo: Art gallery with zoom */
    HTML_CONTENT_EVOLUTION = 6,   /**< Darwin: Interactive evolution trees */
    HTML_CONTENT_BIOLOGY = 7,     /**< Darwin: Biology diagrams (cells, organisms) */
    HTML_CONTENT_GRAPH = 8,       /**< Euclide: Math function graphs */
    HTML_CONTENT_QUIZ = 9,        /**< All maestri: Interactive quizzes */
    HTML_CONTENT_FLASHCARD = 10   /**< All maestri: Flashcard decks */
} HtmlContentType;

/**
 * @brief Save LLM-generated HTML to a file
 *
 * The maestri generate HTML/CSS/JS via LLM and use this to save it.
 * Files are saved to ~/.convergio/education/lessons/
 *
 * @param html_content Complete HTML document string from LLM
 * @param topic Topic name for filename
 * @return Path to saved file (caller must free), or NULL on error
 */
char* html_save(const char* html_content, const char* topic);

/**
 * @brief Save HTML and open in browser (main workflow for maestri)
 *
 * @param html_content Complete HTML document string from LLM
 * @param topic Topic name for filename
 * @return Path to saved file (caller must free), or NULL on error
 */
char* html_save_and_open(const char* html_content, const char* topic);

/**
 * @brief Generate an interactive HTML page for a topic (LEGACY)
 *
 * Prefer using html_save() with LLM-generated content.
 *
 * @param topic Topic to explain
 * @param type Type of content to generate
 * @return Path to generated HTML file (caller must free), or NULL on error
 */
char* html_generate(const char* topic, HtmlContentType type);

/**
 * @brief Open generated HTML file in default browser
 *
 * @param filepath Path to HTML file
 * @return 0 on success, -1 on error
 */
int html_open_in_browser(const char* filepath);

/**
 * @brief Generate and immediately open in browser (convenience function)
 *
 * @param topic Topic to explain
 * @param type Type of content
 * @return Path to generated file (caller must free), or NULL on error
 */
char* html_generate_and_open(const char* topic, HtmlContentType type);

/**
 * @brief Generate geometry visualization (TK87 - Euclide)
 *
 * Creates interactive Canvas-based geometry demonstrations.
 * Examples: Pythagorean theorem, circle properties, polygons.
 *
 * @param topic Geometry topic
 * @return HTML string (caller must free)
 */
char* html_generate_geometry(const char* topic);

/**
 * @brief Generate physics simulation (TK88 - Feynman)
 *
 * Creates animated physics simulations.
 * Examples: pendulum, waves, projectile motion.
 *
 * @param topic Physics topic
 * @return HTML string (caller must free)
 */
char* html_generate_physics(const char* topic);

/**
 * @brief Generate interactive timeline (TK91 - Erodoto)
 *
 * Creates scrollable, clickable historical timelines.
 *
 * @param topic Historical period
 * @return HTML string (caller must free)
 */
char* html_generate_timeline(const char* topic);

/**
 * @brief Generate generic lesson page
 *
 * Creates a simple lesson page with embedded content.
 *
 * @param topic Lesson topic
 * @param content_html HTML content to embed
 * @return HTML string (caller must free)
 */
char* html_generate_lesson(const char* topic, const char* content_html);

/**
 * @brief Get LLM prompt template for visual generation (TL10)
 *
 * Returns a prompt string that guides the LLM to generate appropriate
 * HTML visualizations for the given content type.
 *
 * @param type Type of visual to generate
 * @return Constant prompt string (do not free)
 */
const char* html_get_template_prompt(HtmlContentType type);

/** @brief Get geometry visual prompt template */
const char* html_template_prompt_geometry(void);

/** @brief Get timeline visual prompt template */
const char* html_template_prompt_timeline(void);

/** @brief Get physics diagram prompt template */
const char* html_template_prompt_physics(void);

/** @brief Get biology diagram prompt template */
const char* html_template_prompt_biology(void);

/** @brief Get math graph prompt template */
const char* html_template_prompt_math_graph(void);

/** @brief Get quiz generation prompt template */
const char* html_template_prompt_quiz(void);

/** @brief Get flashcard generation prompt template */
const char* html_template_prompt_flashcards(void);

// ============================================================================
// MASTERY LEARNING API (Phase 11 - Learning Science)
// ============================================================================

/**
 * @brief Skill mastery status levels
 */
typedef enum {
    MASTERY_SKILL_NOT_STARTED = 0,
    MASTERY_SKILL_ATTEMPTED = 1,
    MASTERY_SKILL_FAMILIAR = 2,
    MASTERY_SKILL_PROFICIENT = 3,
    MASTERY_SKILL_MASTERED = 4
} MasterySkillStatus;

/**
 * @brief Individual skill mastery tracking
 */
typedef struct {
    int64_t id;
    int64_t student_id;
    char* skill_id;           // e.g., "math.fractions.addition"
    char* skill_name;         // Display name
    char* parent_skill_id;    // Parent in skill tree
    int attempts;
    int correct;
    float mastery_level;      // 0.0 - 1.0
    float current_difficulty; // Adaptive difficulty
    MasterySkillStatus status;
    time_t last_practice;
    time_t mastered_at;
} MasterySkill;

/**
 * @brief List of mastery skills
 */
typedef struct {
    MasterySkill* skills;
    int count;
    int capacity;
} MasterySkillList;

/**
 * @brief Get mastery level for a skill (0.0 - 1.0)
 */
float education_mastery_get_level(int64_t student_id, const char* skill_id);

/**
 * @brief Check if skill is mastered (80%+)
 */
bool education_mastery_is_mastered(int64_t student_id, const char* skill_id);

/**
 * @brief Identify skill gaps for a student in a subject
 * @return List of skills below proficient level (caller must free with mastery_free_skills)
 */
MasterySkillList* mastery_identify_gaps(int64_t student_id, const char* subject);

/**
 * @brief Free a mastery skill list
 */
void mastery_free_skills(MasterySkillList* list);

/**
 * @brief Check if student can advance to next skill (80% mastery gate)
 */
bool mastery_can_advance(int64_t student_id, const char* target_skill_id,
                         const char** prerequisite_skills, int prereq_count);

/**
 * @brief Print mastery visualization for a skill
 */
void mastery_print_skill(int64_t student_id, const char* skill_id, const char* skill_name);

/**
 * @brief Print mastery summary for a subject
 */
void mastery_print_subject_summary(int64_t student_id, const char* subject);

/**
 * @brief Print full mastery tree visualization
 */
void mastery_print_tree(int64_t student_id);

// ============================================================================
// GAMIFICATION API
// ============================================================================

/**
 * @brief Engagement statistics for gamification
 */
typedef struct {
    int current_streak;           // Days in current streak
    int longest_streak;           // Longest streak ever
    int streak_freezes_available; // Streak freeze tokens
    bool has_weekend_amulet;      // Weekend doesn't break streak
    time_t last_activity;         // Last activity timestamp
    int total_xp;                 // Total XP earned
    int level;                    // Current level
    int daily_challenges_completed; // Challenges done today
} EducationEngagementStats;

/**
 * @brief Get engagement stats for a student
 * @param student_id Student profile ID
 * @return Stats struct (caller must free with education_engagement_free)
 */
EducationEngagementStats* education_engagement_get_stats(int64_t student_id);

/**
 * @brief Check if student studied today (for streak)
 * @param student_id Student profile ID
 * @return 1 if studied today, 0 otherwise
 */
int education_engagement_check_streak(int64_t student_id);

/**
 * @brief Award XP to a student
 * @param student_id Student profile ID
 * @param xp XP amount to award
 * @param reason Reason for XP (for history)
 * @return 0 on success, -1 on error
 */
int education_engagement_award_xp(int64_t student_id, int xp, const char* reason);

/**
 * @brief Get celebration message for event type
 * @param event_type Event type (0-9)
 * @return Celebration message string
 */
const char* education_engagement_get_celebration(int event_type);

/**
 * @brief Free engagement stats
 * @param stats Stats to free
 */
void education_engagement_free(EducationEngagementStats* stats);

/**
 * @brief Add XP to student (legacy wrapper)
 * @param student_id Student profile ID
 * @param xp_amount XP amount
 * @param reason Reason string
 * @return 0 on success, -1 on error
 */
int education_xp_add(int64_t student_id, int xp_amount, const char* reason);

// ============================================================================
// EDUCATION STARTUP
// ============================================================================

/**
 * @brief Show Ali's welcome message at startup
 *
 * Detects first-time users (no profile) and shows appropriate greeting.
 * For new users: Shows welcome and prompts for /setup
 * For returning users: Shows personalized greeting with name
 *
 * @return 0 on success, -1 on error
 */
int education_show_welcome(void);

// ============================================================================
// ERROR INTERPRETER API (Friendly error messages for students)
// ============================================================================

/**
 * @brief Transform a technical error message into a friendly, empathetic message
 *
 * In Education edition, converts cryptic error messages into human-friendly
 * messages that match each maestro's personality. Students never see stack traces.
 *
 * @param error_msg Original error message
 * @param agent_id Agent ID (e.g., "euclide-matematica") for personality matching
 * @return Newly allocated friendly message string (caller must free), or NULL on error
 */
char* education_interpret_error(const char* error_msg, const char* agent_id);

/**
 * @brief Check if an error message should be interpreted
 *
 * Only returns true in Education edition and for recognized error patterns.
 *
 * @param error_msg Error message to check
 * @return true if message should be interpreted, false otherwise
 */
bool education_should_interpret_error(const char* error_msg);

// ============================================================================
// DOCUMENT UPLOAD API (School Materials)
// ============================================================================

/**
 * @brief Open interactive file picker for document upload
 *
 * Shows a student-friendly file browser restricted to Desktop, Documents,
 * and Downloads folders. Only shows supported file types.
 *
 * @return Full path to selected file (caller must free), or NULL if cancelled
 */
char* document_file_picker(void);

/**
 * @brief Upload a document to Claude Files API
 *
 * @param filepath Full path to file to upload
 * @return true on success, false on failure
 */
bool document_upload(const char* filepath);

/**
 * @brief List all uploaded documents
 */
void document_list(void);

/**
 * @brief Select an uploaded document by index
 *
 * @param index 1-based index of document to select
 * @return true on success
 */
bool document_select(int index);

/**
 * @brief Clear all uploaded documents
 */
void document_clear(void);

/**
 * @brief Get the file_id of the current document
 *
 * @return file_id string or NULL if no document active
 */
const char* document_get_current_file_id(void);

/**
 * @brief Get the filename of the current document
 *
 * @return filename string or NULL if no document active
 */
const char* document_get_current_filename(void);

/**
 * @brief Check if a document is currently active
 *
 * @return true if a document is selected
 */
bool document_is_active(void);

/**
 * @brief Command handler for /upload and /doc commands
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return 0 on success, non-zero on error
 */
int document_command_handler(int argc, char** argv);

// ============================================================================
// INTERNAL API (FOR ANNA INTEGRATION)
// ============================================================================

/**
 * @brief Get direct access to education database handle
 *
 * INTERNAL USE ONLY - Used by Anna integration to access inbox table.
 *
 * @return sqlite3 database handle or NULL if not initialized
 */
struct sqlite3;
struct sqlite3* education_get_db_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* NOUS_EDUCATION_H */
