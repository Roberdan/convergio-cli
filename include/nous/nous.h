/**
 * CONVERGIO KERNEL
 *
 * A semantic kernel for human-AI symbiosis
 * Optimized for all Apple Silicon chips (M1, M2, M3, M4)
 *
 * The name "Convergio" represents the convergence of:
 * - Human intention and AI understanding
 * - Semantic meaning and computational power
 * - Individual thought and collective intelligence
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_H
#define CONVERGIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __APPLE__
#include <os/lock.h>
#include <dispatch/dispatch.h>
#include <Accelerate/Accelerate.h>
#endif

// Include hardware detection for dynamic configuration
#include "nous/hardware.h"

// ============================================================================
// APPLE SILICON CONSTANTS (common to all chips)
// ============================================================================

// Cache line size is 128 bytes on all Apple Silicon
#define NOUS_CACHE_LINE           128

// Page size is 16KB on all Apple Silicon
#define NOUS_PAGE_SIZE            16384

// SIMD widths for NEON (same on all Apple Silicon)
#define NOUS_SIMD_WIDTH_128       16      // 128-bit NEON registers
#define NOUS_SIMD_WIDTH_F32       4       // 4x float32 per register
#define NOUS_SIMD_WIDTH_F16       8       // 8x float16 per register

// ============================================================================
// DEBUG LOGGING SYSTEM
// ============================================================================

typedef enum {
    LOG_LEVEL_NONE = 0,    // No logging
    LOG_LEVEL_ERROR = 1,   // Errors only
    LOG_LEVEL_WARN = 2,    // Warnings + errors
    LOG_LEVEL_INFO = 3,    // Info + warnings + errors
    LOG_LEVEL_DEBUG = 4,   // Everything including debug
    LOG_LEVEL_TRACE = 5    // Maximum verbosity
} LogLevel;

typedef enum {
    LOG_CAT_SYSTEM,        // System/kernel operations
    LOG_CAT_AGENT,         // Agent lifecycle & delegation
    LOG_CAT_TOOL,          // Tool execution
    LOG_CAT_API,           // Claude API calls
    LOG_CAT_MEMORY,        // Memory/persistence operations
    LOG_CAT_MSGBUS,        // Message bus communication
    LOG_CAT_COST           // Cost tracking
} LogCategory;

// Global log level (set by --debug flag or debug command)
extern LogLevel g_log_level;

// Logging functions
void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
void nous_log_set_level(LogLevel level);
LogLevel nous_log_get_level(void);
const char* nous_log_level_name(LogLevel level);

// Convenience macros
#define LOG_ERROR(cat, ...) nous_log(LOG_LEVEL_ERROR, cat, __VA_ARGS__)
#define LOG_WARN(cat, ...)  nous_log(LOG_LEVEL_WARN, cat, __VA_ARGS__)
#define LOG_INFO(cat, ...)  nous_log(LOG_LEVEL_INFO, cat, __VA_ARGS__)
#define LOG_DEBUG(cat, ...) nous_log(LOG_LEVEL_DEBUG, cat, __VA_ARGS__)
#define LOG_TRACE(cat, ...) nous_log(LOG_LEVEL_TRACE, cat, __VA_ARGS__)

// ============================================================================
// SEMANTIC PRIMITIVES
// ============================================================================

/**
 * SemanticID - Universal identifier for meaning
 *
 * Structure: [timestamp:40][type:8][hash:16]
 * - Temporally ordered for causal relationships
 * - Type-tagged for fast filtering
 * - Hash suffix for uniqueness
 */
typedef uint64_t SemanticID;

#define SEMANTIC_ID_NULL          0ULL
// Layout: [timestamp:40][type:8][counter:16] -> type sits at bits 16-23
#define SEMANTIC_TYPE_MASK        0x0000000000FF0000ULL
#define SEMANTIC_TYPE_SHIFT       16

typedef enum {
    SEMANTIC_TYPE_VOID      = 0x00,
    SEMANTIC_TYPE_CONCEPT   = 0x01,  // Abstract idea
    SEMANTIC_TYPE_ENTITY    = 0x02,  // Concrete thing
    SEMANTIC_TYPE_RELATION  = 0x03,  // Connection between nodes
    SEMANTIC_TYPE_INTENT    = 0x04,  // Expressed desire
    SEMANTIC_TYPE_AGENT     = 0x05,  // AI or human actor
    SEMANTIC_TYPE_SPACE     = 0x06,  // Collaborative workspace
    SEMANTIC_TYPE_EVENT     = 0x07,  // Something that happened
    SEMANTIC_TYPE_FEELING   = 0x08,  // Emotional context
    SEMANTIC_TYPE_MEMORY    = 0x09,  // Past experience
    SEMANTIC_TYPE_PATTERN   = 0x0A,  // Recognized structure
} SemanticType;

/**
 * Embedding vector for semantic similarity
 * Uses 768 dimensions (compatible with modern transformers)
 * Aligned for SIMD operations
 */
#define NOUS_EMBEDDING_DIM        768

typedef struct __attribute__((aligned(NOUS_CACHE_LINE))) {
    _Float16 values[NOUS_EMBEDDING_DIM];  // Half precision for memory efficiency
} NousEmbedding;

/**
 * SemanticNode - The fundamental unit of meaning
 */
typedef struct NousSemanticNode {
    SemanticID id;
    SemanticType type;

    // Semantic content
    NousEmbedding embedding;          // Vector representation
    char* essence;                     // Human-readable essence (UTF-8)
    size_t essence_len;

    // Temporal information
    uint64_t created_at;              // Nanoseconds since epoch
    uint64_t last_accessed;
    uint64_t access_count;

    // Relational structure
    SemanticID* relations;            // Connected nodes
    float* relation_strengths;         // Weighted connections [0,1]
    size_t relation_count;
    size_t relation_capacity;

    // Provenance
    SemanticID creator;               // Agent or human who created this
    SemanticID context;               // Space where it exists

    // Memory management
    uint32_t ref_count;
    os_unfair_lock lock;
} NousSemanticNode;

// ============================================================================
// SEMANTIC FABRIC - The living graph of meaning
// ============================================================================

/**
 * ShardedSemanticIndex - Lock-free concurrent access
 *
 * Uses consistent hashing across available CPU cores
 * Each shard fits in L2 cache for optimal performance
 */
#define NOUS_FABRIC_SHARDS        64    // Power of 2 for fast modulo
#define NOUS_SHARD_INITIAL_CAP    4096  // Initial nodes per shard

typedef struct {
    NousSemanticNode** nodes;
    size_t count;
    size_t capacity;
    os_unfair_lock lock;

    // Cache-aligned padding to prevent false sharing
    uint8_t _padding[NOUS_CACHE_LINE - sizeof(os_unfair_lock)];
} FabricShard;

typedef struct {
    FabricShard shards[NOUS_FABRIC_SHARDS];

    // GPU-accelerated similarity search
    void* metal_device;               // id<MTLDevice>
    void* similarity_pipeline;        // id<MTLComputePipelineState>
    void* embedding_buffer;           // id<MTLBuffer> - all embeddings

    // Dispatch queues for Apple Silicon topology
    dispatch_queue_t p_core_queue;    // High-priority semantic ops
    dispatch_queue_t e_core_queue;    // Background maintenance
    dispatch_queue_t gpu_queue;       // Metal operations

    // Statistics
    _Atomic uint64_t total_nodes;
    _Atomic uint64_t total_relations;
    _Atomic uint64_t queries_processed;
} SemanticFabric;

// ============================================================================
// INTENT LANGUAGE - Expressing desire
// ============================================================================

typedef enum {
    INTENT_PARSE_OK = 0,
    INTENT_PARSE_INCOMPLETE,
    INTENT_PARSE_AMBIGUOUS,
    INTENT_PARSE_ERROR,
} IntentParseResult;

typedef enum {
    INTENT_KIND_CREATE,       // Bring something into existence
    INTENT_KIND_TRANSFORM,    // Change something
    INTENT_KIND_FIND,         // Locate something
    INTENT_KIND_CONNECT,      // Establish relationship
    INTENT_KIND_UNDERSTAND,   // Gain comprehension
    INTENT_KIND_COLLABORATE,  // Work together
    INTENT_KIND_FEEL,         // Express or sense emotion
} IntentKind;

typedef struct {
    IntentKind kind;
    SemanticID subject;       // What/who is acting
    SemanticID object;        // What's being acted upon
    SemanticID context;       // In what space
    float confidence;         // How certain the parse is [0,1]
    float urgency;           // How time-sensitive [0,1]

    // Original expression (for learning)
    char* raw_input;
    size_t raw_len;

    // Clarification needed?
    char** questions;         // If ambiguous, what to ask
    size_t question_count;
} ParsedIntent;

// ============================================================================
// AGENTS - Autonomous partners
// ============================================================================

typedef enum {
    AGENT_STATE_DORMANT,      // Resting, low power
    AGENT_STATE_LISTENING,    // Aware, waiting for input
    AGENT_STATE_THINKING,     // Processing (GPU/Neural Engine)
    AGENT_STATE_ACTING,       // Performing action
    AGENT_STATE_CONVERSING,   // Dialoguing with human/agent
} AgentState;

typedef struct NousAgent {
    SemanticID id;
    char* name;
    char* essence;            // What this agent fundamentally is

    AgentState state;

    // Personality (learned from interactions)
    NousEmbedding personality;
    float patience;           // How long before asking clarification
    float creativity;         // Willingness to suggest novel solutions
    float assertiveness;      // How strongly to advocate positions

    // Memory
    SemanticID* memories;     // Past interactions and learnings
    size_t memory_count;

    // Capabilities
    char** skills;            // What this agent can do
    size_t skill_count;

    // Relationships
    SemanticID* trusted_humans;
    float* trust_levels;
    size_t trust_count;

    // Runtime
    dispatch_queue_t work_queue;
    void* neural_context;     // Core ML model context
} NousAgent;

// ============================================================================
// SPACES - Collaborative environments
// ============================================================================

typedef struct {
    SemanticID id;
    char* name;
    char* purpose;

    // Participants
    SemanticID* agents;
    SemanticID* humans;
    size_t agent_count;
    size_t human_count;

    // Shared context
    SemanticFabric* local_fabric;  // Space-local semantic graph

    // Rhythm
    float urgency_level;      // Current temporal pressure
    uint64_t last_activity;

    // Permissions
    bool allow_external_agents;
    bool persistent;          // Survives system restart
} NousSpace;

// ============================================================================
// KERNEL API
// ============================================================================

// Lifecycle
int nous_init(void);
void nous_shutdown(void);
bool nous_is_ready(void);

// Semantic operations
SemanticID nous_create_node(SemanticType type, const char* essence);
NousSemanticNode* nous_get_node(SemanticID id);
void nous_release_node(NousSemanticNode* node);
int nous_connect(SemanticID from, SemanticID to, float strength);

// Similarity search (GPU-accelerated)
typedef struct {
    SemanticID id;
    float similarity;
} SimilarityResult;

size_t nous_find_similar(const NousEmbedding* query,
                         size_t max_results,
                         SimilarityResult* results);

// Intent processing
ParsedIntent* nous_parse_intent(const char* input, size_t len);
void nous_free_intent(ParsedIntent* intent);
int nous_execute_intent(ParsedIntent* intent);

// Agent management
NousAgent* nous_create_agent(const char* name, const char* essence);
void nous_destroy_agent(NousAgent* agent);
int nous_agent_listen(NousAgent* agent, SemanticID space);
int nous_agent_speak(NousAgent* agent, const char* message);
int nous_agent_add_skill(NousAgent* agent, const char* skill);
bool nous_agent_has_skill(NousAgent* agent, const char* skill);

// Space management
NousSpace* nous_create_space(const char* name, const char* purpose);
void nous_destroy_space(NousSpace* space);
int nous_join_space(SemanticID entity, SemanticID space);
int nous_leave_space(SemanticID entity, SemanticID space);
float nous_space_urgency(NousSpace* space);
void nous_space_set_urgency(NousSpace* space, float urgency);
size_t nous_space_participant_count(NousSpace* space);
bool nous_space_is_active(NousSpace* space);

// ============================================================================
// M3 MAX OPTIMIZATIONS
// ============================================================================

// Use AMX (Apple Matrix coprocessor) for embedding operations
void nous_amx_dot_product(const _Float16* a, const _Float16* b,
                          size_t dim, float* result);

// Batch similarity with Metal
void nous_metal_batch_similarity(const NousEmbedding* query,
                                  const NousEmbedding* candidates,
                                  size_t count,
                                  float* results);

// Neural Engine inference
int nous_neural_embed_text(const char* text, size_t len, NousEmbedding* out);
int nous_neural_generate(const char* prompt, size_t len,
                         char* output, size_t max_out);

// Claude API integration
int nous_claude_init(void);
void nous_claude_shutdown(void);
char* nous_claude_chat(const char* system_prompt, const char* user_message);
char* nous_claude_chat_with_tools(const char* system_prompt, const char* user_message,
                                   const char* tools_json, char** out_tool_calls);
char* nous_agent_think_with_claude(NousAgent* agent, const char* input);
int nous_generate_embedding(const char* text, NousEmbedding* out);

// Request cancellation (for ESC key interrupt)
void claude_cancel_request(void);
void claude_reset_cancel(void);
bool claude_is_cancelled(void);

#endif // CONVERGIO_H
