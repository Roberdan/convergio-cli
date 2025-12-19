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
    bool autism;
    EducationSeverity autism_severity;
    bool visual_impairment;
    bool hearing_impairment;

    // Preferences
    EducationInputMethod preferred_input;
    EducationOutputMethod preferred_output;
    bool tts_enabled;
    float tts_speed;  // 0.5 - 2.0
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
    char maestro_id[8];
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

/**
 * @brief Curriculum subject definition
 */
typedef struct {
    char id[32];
    char maestro_id[8];
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
