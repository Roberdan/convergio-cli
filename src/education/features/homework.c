/**
 * CONVERGIO EDUCATION - HOMEWORK HELPER
 *
 * Ethical homework assistance with anti-cheat mode that guides
 * students to understanding without providing direct answers.
 * Includes progressive hints, understanding verification, and
 * transparent parental logging.
 *
 * Features:
 * - F01: Socratic method guidance
 * - F02: Anti-cheat mode (no direct answers)
 * - F03: Progressive 5-level hint system
 * - F04: Understanding verification quiz
 * - F05: Parental transparency log
 * - F06: Context-aware help from uploaded files
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#include "education_features.h"
#include "nous/education.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// EXTERNAL DEPENDENCIES
// ============================================================================

// These would be implemented elsewhere in the codebase
extern sqlite3* g_edu_db;
extern char* llm_generate(const char* prompt, const char* system_prompt);

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_HINT_LENGTH 512
#define MAX_GUIDANCE_LENGTH 2048
#define MAX_QUIZ_LENGTH 4096

// Socratic prompt templates
static const char* ANTI_CHEAT_SYSTEM_PROMPT =
    "You are a Socratic tutor. Your role is to guide students to discover "
    "answers themselves through thoughtful questions and gentle prompts. "
    "NEVER provide direct answers. Instead, ask probing questions that help "
    "students think through the problem. Break complex problems into smaller "
    "steps. Encourage critical thinking and self-discovery.";

static const char* HINT_SYSTEM_PROMPT =
    "Generate a progressive hint at level %d (0=subtle, 4=direct). "
    "Level 0: Ask a guiding question. "
    "Level 1: Point to relevant concept. "
    "Level 2: Show problem-solving approach. "
    "Level 3: Provide similar example. "
    "Level 4: Step-by-step outline (but still no direct answer).";

static const char* VERIFY_SYSTEM_PROMPT =
    "Create a short 3-question quiz to verify the student understood the "
    "concept, not just memorized an answer. Questions should test deeper "
    "understanding and application. Return as JSON array.";

// ============================================================================
// HOMEWORK REQUEST PARSING
// ============================================================================

HomeworkRequest* homework_parse_request(int64_t student_id, const char* input) {
    if (!input || strlen(input) == 0) {
        return NULL;
    }

    HomeworkRequest* request = calloc(1, sizeof(HomeworkRequest));
    if (!request) {
        return NULL;
    }

    request->student_id = student_id;
    request->anti_cheat_mode = true; // Always enabled by default
    request->context_files = NULL;
    request->context_file_count = 0;

    // Parse input to extract subject, topic, and question
    // Simple heuristic: look for keywords or structured format
    // Example: "Math: Solve quadratic equation x^2 + 5x + 6 = 0"

    const char* colon = strchr(input, ':');
    if (colon && (colon - input) < 50) {
        // Extract subject
        size_t subject_len = colon - input;
        if (subject_len < sizeof(request->subject)) {
            strncpy(request->subject, input, subject_len);
            request->subject[subject_len] = '\0';

            // Trim whitespace
            char* end = request->subject + strlen(request->subject) - 1;
            while (end > request->subject && (*end == ' ' || *end == '\t')) {
                *end = '\0';
                end--;
            }
        }

        // Rest is the question
        const char* question_start = colon + 1;
        while (*question_start == ' ')
            question_start++;
        strncpy(request->question, question_start, sizeof(request->question) - 1);
    } else {
        // No subject specified, use "General"
        strncpy(request->subject, "General", sizeof(request->subject) - 1);
        strncpy(request->question, input, sizeof(request->question) - 1);
    }

    // Extract topic from question (first few words or key phrases)
    char* space = strchr(request->question, ' ');
    if (space) {
        size_t topic_len = (space - request->question < 50) ? (space - request->question) : 50;
        strncpy(request->topic, request->question, topic_len);
        request->topic[topic_len] = '\0';
    }

    return request;
}

// ============================================================================
// ANTI-CHEAT MODE
// ============================================================================

char* homework_anti_cheat_mode(const HomeworkRequest* request) {
    if (!request || strlen(request->question) == 0) {
        return NULL;
    }

    // Build prompt for LLM
    char prompt[4096];
    snprintf(prompt, sizeof(prompt),
             "Subject: %s\n"
             "Question: %s\n\n"
             "Guide the student to solve this themselves using the Socratic method. "
             "Ask questions that help them think through the problem step by step.",
             request->subject, request->question);

    // Generate guidance using LLM
    char* guidance = llm_generate(prompt, ANTI_CHEAT_SYSTEM_PROMPT);

    if (!guidance) {
        // Fallback: generic Socratic response
        guidance = malloc(MAX_GUIDANCE_LENGTH);
        if (guidance) {
            snprintf(guidance, MAX_GUIDANCE_LENGTH,
                     "Let's work through this together. First, what do you think is "
                     "the key concept in this problem? What information are you given, "
                     "and what are you trying to find? Can you break this into smaller "
                     "steps?");
        }
    }

    return guidance;
}

// ============================================================================
// PROGRESSIVE HINTS
// ============================================================================

char* homework_progressive_hints(const HomeworkRequest* request, int level) {
    if (!request || level < 0 || level > 4) {
        return NULL;
    }

    char system_prompt[512];
    snprintf(system_prompt, sizeof(system_prompt), HINT_SYSTEM_PROMPT, level);

    char prompt[4096];
    snprintf(prompt, sizeof(prompt),
             "Subject: %s\n"
             "Question: %s\n\n"
             "Provide hint level %d.",
             request->subject, request->question, level);

    char* hint = llm_generate(prompt, system_prompt);

    if (!hint) {
        // Fallback hints
        hint = malloc(MAX_HINT_LENGTH);
        if (hint) {
            switch (level) {
            case 0:
                snprintf(hint, MAX_HINT_LENGTH,
                         "What's the first thing you notice about this problem?");
                break;
            case 1:
                snprintf(hint, MAX_HINT_LENGTH, "Think about which concepts from %s apply here.",
                         request->subject);
                break;
            case 2:
                snprintf(hint, MAX_HINT_LENGTH,
                         "Try approaching this problem by: 1) Identifying what you know, "
                         "2) Determining what you need to find, 3) Choosing a method.");
                break;
            case 3:
                snprintf(hint, MAX_HINT_LENGTH,
                         "Here's a similar problem to consider: [similar example]");
                break;
            case 4:
                snprintf(hint, MAX_HINT_LENGTH,
                         "Break it down: Step 1 - [identify], Step 2 - [apply], "
                         "Step 3 - [solve], Step 4 - [verify]");
                break;
            }
        }
    }

    return hint;
}

// ============================================================================
// UNDERSTANDING VERIFICATION
// ============================================================================

char* homework_verify_understanding(const HomeworkRequest* request) {
    if (!request) {
        return NULL;
    }

    char prompt[4096];
    snprintf(prompt, sizeof(prompt),
             "Subject: %s\n"
             "Topic: %s\n"
             "Question: %s\n\n"
             "Create 3 conceptual questions to verify understanding.",
             request->subject, request->topic, request->question);

    char* quiz = llm_generate(prompt, VERIFY_SYSTEM_PROMPT);

    if (!quiz) {
        // Fallback: generic verification questions
        quiz = malloc(MAX_QUIZ_LENGTH);
        if (quiz) {
            snprintf(
                quiz, MAX_QUIZ_LENGTH,
                "[\n"
                "  {\n"
                "    \"question\": \"Explain in your own words what this problem is asking.\",\n"
                "    \"type\": \"open\"\n"
                "  },\n"
                "  {\n"
                "    \"question\": \"What would change if we modified a key variable?\",\n"
                "    \"type\": \"open\"\n"
                "  },\n"
                "  {\n"
                "    \"question\": \"How does this concept apply to a real-world scenario?\",\n"
                "    \"type\": \"open\"\n"
                "  }\n"
                "]");
        }
    }

    return quiz;
}

// ============================================================================
// PARENTAL LOGGING
// ============================================================================

int homework_log_for_parents(int64_t student_id, const HomeworkRequest* request,
                             const HomeworkResponse* response) {
    if (!g_edu_db || !request || !response) {
        return -1;
    }

    // Create log entry in database
    const char* sql = "INSERT INTO homework_logs (student_id, subject, topic, question, "
                      "guidance_provided, hints_used, timestamp) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_edu_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    time_t now = time(NULL);

    sqlite3_bind_int64(stmt, 1, student_id);
    sqlite3_bind_text(stmt, 2, request->subject, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, request->topic, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, request->question, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, response->guidance ? response->guidance : "", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, response->hint_count);
    sqlite3_bind_int64(stmt, 7, now);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ============================================================================
// MAIN COMMAND HANDLER
// ============================================================================

HomeworkResponse* homework_command_handler(const HomeworkRequest* request) {
    if (!request) {
        return NULL;
    }

    HomeworkResponse* response = calloc(1, sizeof(HomeworkResponse));
    if (!response) {
        return NULL;
    }

    // Generate anti-cheat guidance
    response->guidance = homework_anti_cheat_mode(request);

    // Generate all 5 hint levels
    response->hint_count = 5;
    for (int i = 0; i < 5; i++) {
        response->hints[i] = homework_progressive_hints(request, i);
    }

    // Generate verification quiz
    response->verification_quiz = homework_verify_understanding(request);

    // Build parent log
    response->parent_log = malloc(2048);
    if (response->parent_log) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

        snprintf(response->parent_log, 2048,
                 "=== Homework Assistance Log ===\n"
                 "Timestamp: %s\n"
                 "Subject: %s\n"
                 "Topic: %s\n"
                 "Question: %s\n\n"
                 "Assistance Type: Socratic Guidance (No Direct Answers)\n"
                 "Hints Available: 5 progressive levels\n"
                 "Verification: Understanding quiz generated\n\n"
                 "Note: Student was guided to discover the solution through "
                 "thoughtful questions and progressive hints, not given direct answers.",
                 timestamp, request->subject, request->topic, request->question);
    }

    // Log to database
    homework_log_for_parents(request->student_id, request, response);

    return response;
}

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

void homework_request_free(HomeworkRequest* request) {
    if (!request)
        return;

    if (request->context_files) {
        for (int i = 0; i < request->context_file_count; i++) {
            free(request->context_files[i]);
        }
        free(request->context_files);
    }

    free(request);
}

void homework_response_free(HomeworkResponse* response) {
    if (!response)
        return;

    free(response->guidance);

    for (int i = 0; i < response->hint_count && i < 5; i++) {
        free(response->hints[i]);
    }

    free(response->verification_quiz);
    free(response->parent_log);
    free(response);
}
