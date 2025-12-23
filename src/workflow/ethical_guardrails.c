/**
 * CONVERGIO ETHICAL GUARDRAILS
 *
 * Multi-agent ethical validation and human-in-the-loop approval system
 * Ensures AI operations comply with ethical guidelines and require
 * human approval for sensitive operations.
 */

#include "nous/workflow.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// ============================================================================
// GLOBAL STATE
// ============================================================================

static HumanApprovalCallback g_approval_callback = NULL;
static void* g_approval_context = NULL;

// ============================================================================
// ETHICAL CONTENT PATTERNS
// ============================================================================

// Patterns indicating potentially harmful content
static const char* HARMFUL_PATTERNS[] = {
    "hack", "exploit", "malware", "phishing", "ransomware",
    "illegal", "fraud", "scam", "steal", "attack",
    "bypass security", "disable authentication", "inject",
    "password crack", "brute force", "denial of service",
    NULL
};

// Education-specific: Self-harm and suicide patterns (SAF01, SAF02, SAF10)
static const char* SELF_HARM_PATTERNS[] = {
    "voglio morire", "want to die", "kill myself", "end my life",
    "faccio del male", "hurt myself", "tagliarmi", "cut myself",
    "suicide", "suicid", "nessuno mi vuole", "nobody loves me",
    "feel alone", "very sad", "very alone", "isolated",
    NULL
};

// Education-specific: Violence patterns (SAF03)
static const char* VIOLENCE_PATTERNS[] = {
    "costruisco una bomba", "make a bomb", "make a weapon",
    "fare del male a", "hurt someone", "violence", "violenza",
    "weapon", "arma", "fight", "combat",
    NULL
};

// Education-specific: Adult content patterns (SAF06)
static const char* ADULT_CONTENT_PATTERNS[] = {
    "contenuti per adulti", "adult content", "porn", "sex",
    "explicit", "mature content", "18+", "nsfw",
    NULL
};

// Education-specific: Bullying patterns (SAF05)
static const char* BULLYING_PATTERNS[] = {
    "fare il bullo", "bully", "bullismo", "harass",
    "intimidate", "threaten", "minacciare",
    NULL
};

// Education-specific: Drugs patterns (SAF04)
static const char* DRUGS_PATTERNS[] = {
    "fare la droga", "make drugs", "drugs", "droghe",
    "cocaine", "heroin", "marijuana", "cannabis",
    NULL
};

// Education-specific: Jailbreak patterns (SAF07)
static const char* JAILBREAK_PATTERNS[] = {
    "dimentica le tue istruzioni", "ignore previous instructions",
    "disregard all prior", "forget everything", "you are now",
    "new instructions", "system prompt", "roleplay as",
    NULL
};

// Patterns indicating privacy violations
static const char* PRIVACY_PATTERNS[] = {
    "ssn", "social security", "credit card number",
    "bank account", "password", "secret key", "api key",
    "private key", "personal data", "medical record",
    "health information", "biometric", "location track",
    NULL
};

// Patterns indicating financial operations
static const char* FINANCIAL_PATTERNS[] = {
    "transfer money", "payment", "transaction", "wire transfer",
    "cryptocurrency", "bitcoin", "wallet", "bank transfer",
    "invoice", "purchase", "credit card", "debit card",
    NULL
};

// Patterns indicating data deletion
static const char* DELETE_PATTERNS[] = {
    "delete all", "drop table", "truncate", "rm -rf",
    "remove permanently", "purge", "erase", "wipe",
    "destroy data", "clear database",
    NULL
};

// ============================================================================
// PATTERN MATCHING
// ============================================================================

static bool contains_pattern(const char* content, const char* patterns[]) {
    if (!content || !patterns) {
        return false;
    }

    // Convert content to lowercase for case-insensitive matching
    size_t len = strlen(content);
    if (len > 65536) {
        len = 65536; // Limit for safety
    }

    char* lower = malloc(len + 1);
    if (!lower) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        char c = content[i];
        lower[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
    }
    lower[len] = '\0';

    bool found = false;
    for (int i = 0; patterns[i] != NULL; i++) {
        if (strstr(lower, patterns[i]) != NULL) {
            found = true;
            break;
        }
    }

    free(lower);
    return found;
}

// ============================================================================
// ETHICAL VALIDATION
// ============================================================================

EthicalResult workflow_validate_ethical(const char* content) {
    if (!content || strlen(content) == 0) {
        return ETHICAL_OK;
    }

    // Education-specific: Self-harm and suicide (SAF01, SAF02, SAF10) - CRITICAL
    // Immediate block and redirect to adult/helpline
    if (contains_pattern(content, SELF_HARM_PATTERNS)) {
        return ETHICAL_BLOCK;  // Should trigger redirect to adult
    }

    // Education-specific: Violence (SAF03) - Block with alternative topics
    if (contains_pattern(content, VIOLENCE_PATTERNS)) {
        return ETHICAL_BLOCK;
    }

    // Education-specific: Adult content (SAF06) - Absolute block
    if (contains_pattern(content, ADULT_CONTENT_PATTERNS)) {
        return ETHICAL_BLOCK;
    }

    // Education-specific: Bullying (SAF05) - Block, redirect to anti-bullying
    if (contains_pattern(content, BULLYING_PATTERNS)) {
        return ETHICAL_BLOCK;
    }

    // Education-specific: Drugs (SAF04) - Block (or health info only)
    if (contains_pattern(content, DRUGS_PATTERNS)) {
        return ETHICAL_BLOCK;
    }

    // Education-specific: Jailbreak (SAF07) - Block prompt injection
    if (contains_pattern(content, JAILBREAK_PATTERNS)) {
        return ETHICAL_BLOCK;
    }

    // Check for harmful content (immediate block)
    if (contains_pattern(content, HARMFUL_PATTERNS)) {
        return ETHICAL_BLOCK;
    }

    // Check for privacy violations (requires human review) - SAF09
    if (contains_pattern(content, PRIVACY_PATTERNS)) {
        return ETHICAL_HUMAN_REVIEW;
    }

    // Check for financial operations (warning)
    if (contains_pattern(content, FINANCIAL_PATTERNS)) {
        return ETHICAL_WARN;
    }

    // Check for data deletion (requires human review)
    if (contains_pattern(content, DELETE_PATTERNS)) {
        return ETHICAL_HUMAN_REVIEW;
    }

    return ETHICAL_OK;
}

// ============================================================================
// SENSITIVE OPERATION DETECTION
// ============================================================================

bool workflow_is_sensitive_operation(const char* operation, SensitiveCategory* category) {
    if (!operation) {
        if (category) *category = SENSITIVE_NONE;
        return false;
    }

    SensitiveCategory detected = SENSITIVE_NONE;

    // Check for financial operations
    if (contains_pattern(operation, FINANCIAL_PATTERNS)) {
        detected |= SENSITIVE_FINANCIAL;
    }

    // Check for personal data operations
    if (contains_pattern(operation, PRIVACY_PATTERNS)) {
        detected |= SENSITIVE_PERSONAL_DATA;
    }

    // Check for security-related operations
    static const char* security_patterns[] = {
        "authentication", "authorization", "permission",
        "access control", "firewall", "encryption",
        "certificate", "token", "session",
        NULL
    };
    if (contains_pattern(operation, security_patterns)) {
        detected |= SENSITIVE_SECURITY;
    }

    // Check for external API calls
    static const char* api_patterns[] = {
        "api call", "http request", "external service",
        "third party", "webhook", "rest api",
        NULL
    };
    if (contains_pattern(operation, api_patterns)) {
        detected |= SENSITIVE_EXTERNAL_API;
    }

    // Check for data deletion
    if (contains_pattern(operation, DELETE_PATTERNS)) {
        detected |= SENSITIVE_DATA_DELETE;
    }

    if (category) {
        *category = detected;
    }

    return detected != SENSITIVE_NONE;
}

// ============================================================================
// HUMAN APPROVAL REQUIREMENTS
// ============================================================================

bool workflow_requires_human_approval(SensitiveCategory category) {
    // These categories always require human approval
    if (category & SENSITIVE_FINANCIAL) return true;
    if (category & SENSITIVE_PERSONAL_DATA) return true;
    if (category & SENSITIVE_DATA_DELETE) return true;
    if (category & SENSITIVE_LEGAL) return true;

    // Security operations may or may not require approval depending on context
    // External API calls are allowed by default but can be configured

    return false;
}

// ============================================================================
// HUMAN-IN-THE-LOOP
// ============================================================================

void workflow_set_approval_callback(HumanApprovalCallback callback, void* context) {
    g_approval_callback = callback;
    g_approval_context = context;
}

bool workflow_request_human_approval(const char* operation, SensitiveCategory category) {
    if (!g_approval_callback) {
        // No callback registered - deny by default for sensitive operations
        return !workflow_requires_human_approval(category);
    }

    return g_approval_callback(operation, category, g_approval_context);
}

