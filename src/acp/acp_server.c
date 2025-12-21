/**
 * ACP Server - Agent Client Protocol for Zed
 *
 * Implements JSON-RPC 2.0 over stdio for Zed integration.
 * This is a standalone binary that exposes Convergio agents via ACP.
 */

#include "nous/acp.h"
#include "nous/orchestrator.h"
#include "nous/config.h"
#include "nous/embedded_agents.h"
#include "nous/nous.h"
#include "nous/updater.h"
#include "nous/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <dirent.h>
#include <cjson/cJSON.h>

// ============================================================================
// CONTEXT SHARING (F2: Ali consapevole di tutte le conversazioni)
// ============================================================================

#define CONTEXT_DIR "~/.convergio/agent_context"
#define MAX_CONTEXT_SIZE 2048
#define SESSIONS_DIR "~/.convergio/sessions"

// ============================================================================
// RESOURCE LIMITS (Phase 11 S6)
// ============================================================================

#define MAX_OPEN_FILES 256
#define MAX_MEMORY_MB 512
#define MAX_SESSION_BUFFER_SIZE (1024 * 1024)  // 1MB per session

// Global counter for total memory used across all session buffers
static size_t g_total_buffer_memory = 0;
static pthread_mutex_t g_memory_mutex = PTHREAD_MUTEX_INITIALIZER;

// ============================================================================
// CRASH RECOVERY (Phase 11 S5)
// ============================================================================

#define PID_FILE "~/.convergio/acp.pid"

// Expand ~ to home directory (forward declaration needed for crash recovery)
static char* expand_path(const char* path);

// Check if a process is running
static bool is_process_running(pid_t pid) {
    if (pid <= 0) return false;

    // Use kill(pid, 0) to check if process exists
    // Returns 0 if process exists, -1 with errno=ESRCH if not
    return (kill(pid, 0) == 0);
}

// Read PID from PID file
static pid_t read_pid_file(void) {
    char* pid_path = expand_path(PID_FILE);
    FILE* f = fopen(pid_path, "r");
    free(pid_path);

    if (!f) {
        return -1;
    }

    pid_t pid = -1;
    if (fscanf(f, "%d", &pid) != 1) {
        pid = -1;
    }
    fclose(f);

    return pid;
}

// Write current PID to PID file
static int write_pid_file(void) {
    char* pid_path = expand_path(PID_FILE);
    FILE* f = fopen(pid_path, "w");
    if (!f) {
        fprintf(stderr, "[ACP] Warning: Failed to create PID file: %s\n", pid_path);
        free(pid_path);
        return -1;
    }

    fprintf(f, "%d\n", getpid());
    fclose(f);
    free(pid_path);
    return 0;
}

// Remove PID file
static void remove_pid_file(void) {
    char* pid_path = expand_path(PID_FILE);
    unlink(pid_path);
    free(pid_path);
}

// Clean up orphaned session files (older than 24 hours)
static void cleanup_orphaned_sessions(void) {
    char* sessions_dir = expand_path(SESSIONS_DIR);
    DIR* d = opendir(sessions_dir);
    if (!d) {
        free(sessions_dir);
        return;
    }

    int cleaned_count = 0;
    struct dirent* entry;
    time_t now = time(NULL);

    while ((entry = readdir(d)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Only process .json files
        if (!strstr(entry->d_name, ".json")) {
            continue;
        }

        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", sessions_dir, entry->d_name);

        struct stat st;
        if (stat(filepath, &st) == 0) {
            // Check if file is older than 24 hours
            // This is a safety measure to avoid deleting active sessions
            if (now - st.st_mtime > 86400) {
                unlink(filepath);
                cleaned_count++;
            }
        }
    }

    closedir(d);
    free(sessions_dir);

    if (cleaned_count > 0) {
        fprintf(stderr, "[ACP] Cleaned up %d orphaned session file(s)\n", cleaned_count);
    }
}

// Clean up any lock files in ~/.convergio
static void cleanup_lock_files(void) {
    char* convergio_dir = expand_path("~/.convergio");
    DIR* d = opendir(convergio_dir);
    if (!d) {
        free(convergio_dir);
        return;
    }

    int cleaned_count = 0;
    struct dirent* entry;

    while ((entry = readdir(d)) != NULL) {
        // Only process .lock files
        if (!strstr(entry->d_name, ".lock")) {
            continue;
        }

        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", convergio_dir, entry->d_name);
        unlink(filepath);
        cleaned_count++;
    }

    closedir(d);
    free(convergio_dir);

    if (cleaned_count > 0) {
        fprintf(stderr, "[ACP] Cleaned up %d orphaned lock file(s)\n", cleaned_count);
    }
}

// Perform crash recovery check on startup
static void crash_recovery_check(void) {
    pid_t old_pid = read_pid_file();

    if (old_pid > 0) {
        // Check if the old process is still running
        if (is_process_running(old_pid)) {
            fprintf(stderr, "[ACP] Warning: Another ACP server instance is already running (PID: %d)\n", old_pid);
            fprintf(stderr, "[ACP] If this is incorrect, remove %s and restart\n", PID_FILE);
            // Continue anyway - the PID file will be overwritten
        } else {
            // Process crashed - perform cleanup
            fprintf(stderr, "[ACP] Detected crashed ACP server session (PID: %d)\n", old_pid);
            fprintf(stderr, "[ACP] Performing crash recovery...\n");

            // Clean up orphaned resources
            cleanup_orphaned_sessions();
            cleanup_lock_files();

            fprintf(stderr, "[ACP] Crash recovery complete\n");
        }
    }

    // Write new PID file
    write_pid_file();
}

// Expand ~ to home directory
static char* expand_path(const char* path) {
    if (path[0] != '~') {
        return strdup(path);
    }
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";
    size_t len = strlen(home) + strlen(path);
    char* expanded = malloc(len);
    snprintf(expanded, len, "%s%s", home, path + 1);
    return expanded;
}

// Ensure context directory exists
static void ensure_context_dir(void) {
    char* dir = expand_path(CONTEXT_DIR);
    mkdir(dir, 0755);
    free(dir);
}

// Save agent conversation context (summary of last interaction)
static void save_agent_context(const char* agent_name, const char* user_prompt, const char* agent_response) {
    if (!agent_name || !user_prompt || !agent_response) return;

    ensure_context_dir();

    char* dir = expand_path(CONTEXT_DIR);
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s.json", dir, agent_name);
    free(dir);

    // Create JSON with context
    cJSON* ctx = cJSON_CreateObject();
    cJSON_AddStringToObject(ctx, "agent", agent_name);
    cJSON_AddNumberToObject(ctx, "timestamp", (double)time(NULL));

    // Truncate to reasonable size
    char user_summary[MAX_CONTEXT_SIZE];
    char agent_summary[MAX_CONTEXT_SIZE];
    strncpy(user_summary, user_prompt, MAX_CONTEXT_SIZE - 1);
    user_summary[MAX_CONTEXT_SIZE - 1] = '\0';
    strncpy(agent_summary, agent_response, MAX_CONTEXT_SIZE - 1);
    agent_summary[MAX_CONTEXT_SIZE - 1] = '\0';

    cJSON_AddStringToObject(ctx, "last_user_message", user_summary);
    cJSON_AddStringToObject(ctx, "last_agent_response", agent_summary);

    char* json = cJSON_Print(ctx);
    cJSON_Delete(ctx);

    FILE* f = fopen(filepath, "w");
    if (f) {
        fputs(json, f);
        fclose(f);
    }
    free(json);
}

// Load all agent contexts for Ali to be aware of other conversations
static char* load_all_agent_contexts(void) {
    char* dir = expand_path(CONTEXT_DIR);
    DIR* d = opendir(dir);
    if (!d) {
        free(dir);
        return NULL;
    }

    // Build context summary
    size_t capacity = 8192;
    char* summary = malloc(capacity);
    size_t len = 0;
    len += snprintf(summary + len, capacity - len,
        "\n## Recent Agent Conversations (Context for Ali)\n\n");

    struct dirent* entry;
    int count = 0;

    while ((entry = readdir(d)) != NULL) {
        if (strstr(entry->d_name, ".json") == NULL) continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);

        FILE* f = fopen(filepath, "r");
        if (!f) continue;

        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        char* content = malloc(fsize + 1);
        fread(content, 1, fsize, f);
        content[fsize] = '\0';
        fclose(f);

        cJSON* ctx = cJSON_Parse(content);
        free(content);
        if (!ctx) continue;

        cJSON* agent = cJSON_GetObjectItem(ctx, "agent");
        cJSON* user_msg = cJSON_GetObjectItem(ctx, "last_user_message");
        cJSON* agent_resp = cJSON_GetObjectItem(ctx, "last_agent_response");

        if (agent && user_msg && agent_resp &&
            cJSON_IsString(agent) && cJSON_IsString(user_msg) && cJSON_IsString(agent_resp)) {
            len += snprintf(summary + len, capacity - len,
                "### %s\n"
                "**User asked**: %.200s%s\n"
                "**Agent replied**: %.300s%s\n\n",
                agent->valuestring,
                user_msg->valuestring,
                strlen(user_msg->valuestring) > 200 ? "..." : "",
                agent_resp->valuestring,
                strlen(agent_resp->valuestring) > 300 ? "..." : "");
            count++;
        }

        cJSON_Delete(ctx);

        if (len > capacity - 1024) break;  // Prevent overflow
    }

    closedir(d);
    free(dir);

    if (count == 0) {
        free(summary);
        return NULL;
    }

    return summary;
}

// Global server state
static ACPServer g_server = {0};
static volatile sig_atomic_t g_running = 1;

// Current streaming session for callbacks
static char g_current_session_id[64] = {0};

// Forward declarations
static void handle_signal(int sig);
static void dispatch_request(cJSON* request);
static char* generate_session_id(void);

// ============================================================================
// SIGNAL HANDLING
// ============================================================================

static void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
}

// Crash signal handler - attempt graceful cleanup
static void handle_crash_signal(int sig) {
    // Signal-safe write
    const char* msg = "\n[ACP] Crash detected, cleaning up...\n";
    (void)write(STDERR_FILENO, msg, strlen(msg));

    // Re-raise with default handler for core dump
    signal(sig, SIG_DFL);
    raise(sig);
}

// Free all session resources (called on shutdown)
static void cleanup_sessions(void) {
    for (int i = 0; i < g_server.session_count; i++) {
        ACPSession* session = &g_server.sessions[i];

        // Cancel and join any active worker thread
        if (session->has_worker) {
            session->worker_cancelled = true;
            pthread_join(session->worker_thread, NULL);
            session->has_worker = false;
        }

        // Destroy session mutex
        pthread_mutex_destroy(&session->mutex);

        // Free message contents
        for (int j = 0; j < session->message_count; j++) {
            if (session->messages[j].content) {
                free(session->messages[j].content);
                session->messages[j].content = NULL;
            }
        }
        session->message_count = 0;

        // Free background buffer and update memory tracking
        if (session->background_buffer) {
            pthread_mutex_lock(&g_memory_mutex);
            if (g_total_buffer_memory >= session->background_buffer_cap) {
                g_total_buffer_memory -= session->background_buffer_cap;
            } else {
                g_total_buffer_memory = 0;
            }
            pthread_mutex_unlock(&g_memory_mutex);

            free(session->background_buffer);
            session->background_buffer = NULL;
            session->background_buffer_len = 0;
            session->background_buffer_cap = 0;
        }

        session->active = false;
    }
    g_server.session_count = 0;
}

// ============================================================================
// ASYNC PROMPT PROCESSING (Phase 12 fix)
// ============================================================================

// Global mutex for thread-safe stdout writes
static pthread_mutex_t g_stdout_mutex = PTHREAD_MUTEX_INITIALIZER;

// Worker thread arguments
typedef struct {
    ACPSession* session;
    char* prompt_text;
    int request_id;
    char* selected_agent;
    char session_id[64];
} PromptWorkerArgs;

// Thread-local session ID for stream_callback
static __thread char tl_current_session_id[64] = {0};

// Forward declarations
static void process_prompt_internal(PromptWorkerArgs* args);
static void worker_stream_callback(const char* chunk, void* user_data);

// Worker stream callback - uses thread-local session ID
static void worker_stream_callback(const char* chunk, void* user_data) {
    PromptWorkerArgs* args = (PromptWorkerArgs*)user_data;
    if (!chunk || strlen(chunk) == 0) return;

    // Check if session is in background mode (thread-safe)
    pthread_mutex_lock(&args->session->mutex);
    bool is_background = args->session->is_background;
    pthread_mutex_unlock(&args->session->mutex);

    if (is_background) {
        // Buffer the chunk instead of sending (with resource limits)
        pthread_mutex_lock(&args->session->mutex);

        size_t chunk_len = strlen(chunk);
        size_t needed = args->session->background_buffer_len + chunk_len + 1;

        // Check session buffer size limit
        if (needed > MAX_SESSION_BUFFER_SIZE) {
            pthread_mutex_unlock(&args->session->mutex);
            LOG_WARN(LOG_CAT_SYSTEM, "Session buffer would exceed MAX_SESSION_BUFFER_SIZE, dropping chunk");
            return;
        }

        if (!args->session->background_buffer) {
            args->session->background_buffer_cap = (needed > ACP_BACKGROUND_BUFFER_SIZE)
                ? needed * 2 : ACP_BACKGROUND_BUFFER_SIZE;

            // Check global memory limit before allocating
            pthread_mutex_lock(&g_memory_mutex);
            size_t total_after_alloc = g_total_buffer_memory + args->session->background_buffer_cap;
            if (total_after_alloc > MAX_MEMORY_MB * 1024 * 1024) {
                pthread_mutex_unlock(&g_memory_mutex);
                pthread_mutex_unlock(&args->session->mutex);
                LOG_WARN(LOG_CAT_SYSTEM, "Total buffer memory would exceed MAX_MEMORY_MB, dropping chunk");
                return;
            }
            g_total_buffer_memory += args->session->background_buffer_cap;
            pthread_mutex_unlock(&g_memory_mutex);

            args->session->background_buffer = malloc(args->session->background_buffer_cap);
            args->session->background_buffer[0] = '\0';
            args->session->background_buffer_len = 0;
        } else if (needed > args->session->background_buffer_cap) {
            size_t old_cap = args->session->background_buffer_cap;
            args->session->background_buffer_cap = needed * 2;

            // Check if new capacity would exceed session limit
            if (args->session->background_buffer_cap > MAX_SESSION_BUFFER_SIZE) {
                args->session->background_buffer_cap = MAX_SESSION_BUFFER_SIZE;
            }

            // Check global memory limit before expanding
            pthread_mutex_lock(&g_memory_mutex);
            size_t additional_memory = args->session->background_buffer_cap - old_cap;
            size_t total_after_realloc = g_total_buffer_memory + additional_memory;
            if (total_after_realloc > MAX_MEMORY_MB * 1024 * 1024) {
                pthread_mutex_unlock(&g_memory_mutex);
                pthread_mutex_unlock(&args->session->mutex);
                LOG_WARN(LOG_CAT_SYSTEM, "Total buffer memory would exceed MAX_MEMORY_MB, dropping chunk");
                return;
            }
            g_total_buffer_memory += additional_memory;
            pthread_mutex_unlock(&g_memory_mutex);

            args->session->background_buffer = realloc(args->session->background_buffer,
                                                        args->session->background_buffer_cap);
        }

        memcpy(args->session->background_buffer + args->session->background_buffer_len,
               chunk, chunk_len + 1);
        args->session->background_buffer_len += chunk_len;

        pthread_mutex_unlock(&args->session->mutex);
        return;
    }

    // Build session/update notification (ACP schema format)
    cJSON* params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "sessionId", args->session_id);

    cJSON* update = cJSON_CreateObject();
    cJSON_AddStringToObject(update, "sessionUpdate", "agent_message_chunk");

    cJSON* content = cJSON_CreateObject();
    cJSON_AddStringToObject(content, "type", "text");
    cJSON_AddStringToObject(content, "text", chunk);
    cJSON_AddItemToObject(update, "content", content);

    cJSON_AddItemToObject(params, "update", update);

    char* params_json = cJSON_PrintUnformatted(params);
    acp_send_notification("session/update", params_json);
    free(params_json);
    cJSON_Delete(params);
}

// Process prompt in worker thread
static void process_prompt_internal(PromptWorkerArgs* args) {
    ACPSession* session = args->session;
    char* response = NULL;

    // Build conversation history context from session (for resume support)
    char* history_context = NULL;

    pthread_mutex_lock(&session->mutex);
    int msg_count = session->message_count;
    pthread_mutex_unlock(&session->mutex);

    if (msg_count > 0) {
        size_t ctx_size = 16384;
        history_context = malloc(ctx_size);
        size_t ctx_len = 0;
        ctx_len += snprintf(history_context + ctx_len, ctx_size - ctx_len,
            "\n[Previous conversation history - continue from where we left off]\n");

        pthread_mutex_lock(&session->mutex);
        int start = session->message_count > 10 ? session->message_count - 10 : 0;
        for (int i = start; i < session->message_count; i++) {
            const char* role = session->messages[i].role;
            const char* content = session->messages[i].content;
            if (content) {
                size_t max_len = 500;
                ctx_len += snprintf(history_context + ctx_len, ctx_size - ctx_len,
                    "\n**%s**: %.500s%s\n",
                    strcmp(role, "user") == 0 ? "You" : "Assistant",
                    content,
                    strlen(content) > max_len ? "..." : "");
            }
            if (ctx_len > ctx_size - 1024) break;
        }
        pthread_mutex_unlock(&session->mutex);

        ctx_len += snprintf(history_context + ctx_len, ctx_size - ctx_len,
            "\n[End of history - now responding to new message]\n\n");
    }

    // Route to specific agent or orchestrator
    if (args->selected_agent && args->selected_agent[0] != '\0') {
        ManagedAgent* agent = agent_find_by_name(args->selected_agent);
        if (agent) {
            char* enhanced_prompt = args->prompt_text;
            if (history_context) {
                size_t ep_len = strlen(args->prompt_text) + strlen(history_context) + 64;
                enhanced_prompt = malloc(ep_len);
                snprintf(enhanced_prompt, ep_len, "%s%s", history_context, args->prompt_text);
            }

            response = orchestrator_agent_chat(agent, enhanced_prompt);

            if (enhanced_prompt != args->prompt_text) free(enhanced_prompt);

            if (response) {
                worker_stream_callback(response, args);
                save_agent_context(args->selected_agent, args->prompt_text, response);
            }
        } else {
            response = orchestrator_process_stream(args->prompt_text,
                (void (*)(const char*, void*))worker_stream_callback, args);
        }
    } else {
        // Default: orchestrator (Ali) with streaming
        char* agent_contexts = load_all_agent_contexts();

        char* historical_memory = NULL;
        MemorySearchResult memory_result = {0};
        if (memory_load_recent(10, &memory_result) == 0 && memory_result.count > 0) {
            historical_memory = memory_build_context(&memory_result, 8192);
            memory_free_result(&memory_result);
        }

        size_t enhanced_len = strlen(args->prompt_text) + 1024;
        if (agent_contexts) enhanced_len += strlen(agent_contexts);
        if (historical_memory) enhanced_len += strlen(historical_memory);
        if (history_context) enhanced_len += strlen(history_context);

        char* enhanced_prompt = malloc(enhanced_len);
        size_t pos = 0;

        if (historical_memory) {
            pos += snprintf(enhanced_prompt + pos, enhanced_len - pos,
                "[Historical Memory - Cross-Session Context]\n%s\n---\n\n",
                historical_memory);
            free(historical_memory);
        }

        if (agent_contexts) {
            pos += snprintf(enhanced_prompt + pos, enhanced_len - pos,
                "[Recent Agent Conversations]\n%s\n---\n\n",
                agent_contexts);
            free(agent_contexts);
        }

        if (history_context) {
            pos += snprintf(enhanced_prompt + pos, enhanced_len - pos, "%s", history_context);
        }

        pos += snprintf(enhanced_prompt + pos, enhanced_len - pos,
            "[User Message]\n%s", args->prompt_text);

        response = orchestrator_process_stream(enhanced_prompt,
            (void (*)(const char*, void*))worker_stream_callback, args);
        free(enhanced_prompt);
    }

    // Save messages to session history (thread-safe)
    pthread_mutex_lock(&session->mutex);
    acp_session_add_message(session, "user", args->prompt_text);
    if (response) {
        acp_session_add_message(session, "assistant", response);
    }
    pthread_mutex_unlock(&session->mutex);

    // Persist session to disk
    acp_session_save(session);

    // Generate memory summary for significant conversations
    pthread_mutex_lock(&session->mutex);
    if (session->message_count >= 4 && session->message_count % 4 == 0) {
        const char* messages[ACP_MAX_MESSAGES];
        const char* roles[ACP_MAX_MESSAGES];
        for (int i = 0; i < session->message_count; i++) {
            messages[i] = session->messages[i].content;
            roles[i] = session->messages[i].role;
        }
        int count = session->message_count;
        char agent_name[64];
        strncpy(agent_name, session->agent_name, sizeof(agent_name) - 1);
        pthread_mutex_unlock(&session->mutex);

        MemoryEntry mem_entry;
        if (memory_generate_summary(agent_name, messages, roles, count, &mem_entry) == 0) {
            memory_save(&mem_entry);
        }
    } else {
        pthread_mutex_unlock(&session->mutex);
    }

    if (history_context) {
        free(history_context);
    }

    // Send final response
    cJSON* result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "stopReason", "end_turn");

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(args->request_id, result_json);
    free(result_json);
    cJSON_Delete(result);

    if (response) {
        free(response);
    }

    // If session was in background, send completion notification
    pthread_mutex_lock(&session->mutex);
    bool is_background = session->is_background;
    size_t buffered_len = session->background_buffer_len;
    pthread_mutex_unlock(&session->mutex);

    if (is_background) {
        cJSON* notify_params = cJSON_CreateObject();
        cJSON_AddStringToObject(notify_params, "sessionId", args->session_id);
        cJSON_AddStringToObject(notify_params, "status", "completed");
        cJSON_AddBoolToObject(notify_params, "hasBufferedContent", buffered_len > 0);
        cJSON_AddNumberToObject(notify_params, "bufferedLength", (double)buffered_len);

        char* notify_json = cJSON_PrintUnformatted(notify_params);
        acp_send_notification("session/backgroundComplete", notify_json);
        free(notify_json);
        cJSON_Delete(notify_params);

        LOG_INFO(LOG_CAT_SYSTEM, "Background session %s completed, buffered %zu bytes",
                 args->session_id, buffered_len);
    }
}

// Worker thread function
static void* prompt_worker_thread(void* arg) {
    PromptWorkerArgs* args = (PromptWorkerArgs*)arg;
    ACPSession* session = args->session;

    // Set thread-local session ID
    strncpy(tl_current_session_id, args->session_id, sizeof(tl_current_session_id) - 1);

    // Process the prompt (this is the blocking part)
    process_prompt_internal(args);

    // Mark session as no longer processing
    // NOTE: has_worker stays true until cleanup_sessions() joins this thread
    pthread_mutex_lock(&session->mutex);
    session->is_processing = false;
    pthread_mutex_unlock(&session->mutex);

    // Clear thread-local session ID
    tl_current_session_id[0] = '\0';

    // Free worker args
    if (args->prompt_text) free(args->prompt_text);
    if (args->selected_agent) free(args->selected_agent);
    free(args);

    return NULL;
}

// ============================================================================
// JSON-RPC RESPONSE HELPERS
// ============================================================================

void acp_send_response(int id, const char* result_json) {
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(response, "id", id);

    if (result_json) {
        cJSON* result = cJSON_Parse(result_json);
        if (result) {
            cJSON_AddItemToObject(response, "result", result);
        } else {
            cJSON_AddNullToObject(response, "result");
        }
    } else {
        cJSON_AddNullToObject(response, "result");
    }

    char* json = cJSON_PrintUnformatted(response);

    // Thread-safe stdout write
    pthread_mutex_lock(&g_stdout_mutex);
    fprintf(stdout, "%s\n", json);
    fflush(stdout);
    pthread_mutex_unlock(&g_stdout_mutex);

    free(json);
    cJSON_Delete(response);
}

void acp_send_error(int id, int code, const char* message) {
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(response, "id", id);

    cJSON* error = cJSON_CreateObject();
    cJSON_AddNumberToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);
    cJSON_AddItemToObject(response, "error", error);

    char* json = cJSON_PrintUnformatted(response);

    // Thread-safe stdout write
    pthread_mutex_lock(&g_stdout_mutex);
    fprintf(stdout, "%s\n", json);
    fflush(stdout);
    pthread_mutex_unlock(&g_stdout_mutex);

    free(json);
    cJSON_Delete(response);
}

void acp_send_notification(const char* method, const char* params_json) {
    cJSON* notification = cJSON_CreateObject();
    cJSON_AddStringToObject(notification, "jsonrpc", "2.0");
    cJSON_AddStringToObject(notification, "method", method);

    if (params_json) {
        cJSON* params = cJSON_Parse(params_json);
        if (params) {
            cJSON_AddItemToObject(notification, "params", params);
        }
    }

    char* json = cJSON_PrintUnformatted(notification);

    // Thread-safe stdout write
    pthread_mutex_lock(&g_stdout_mutex);
    fprintf(stdout, "%s\n", json);
    fflush(stdout);
    pthread_mutex_unlock(&g_stdout_mutex);

    free(json);
    cJSON_Delete(notification);
}

// ============================================================================
// SESSION MANAGEMENT
// ============================================================================

// Get session file path
static char* get_session_filepath(const char* session_id) {
    char* dir = expand_path(SESSIONS_DIR);
    mkdir(dir, 0755);

    size_t filepath_len = strlen(dir) + strlen(session_id) + 10;
    char* filepath = malloc(filepath_len);
    snprintf(filepath, filepath_len, "%s/%s.json", dir, session_id);
    free(dir);
    return filepath;
}

// Find most recent session for an agent name - now uses SQLite unified persistence
static char* find_session_by_agent_name(const char* agent_name) {
    // Use SQLite unified persistence to find session by agent name
    // This enables CLI and Zed to share the same conversation history
    return persistence_get_or_create_agent_session(agent_name);
}

// Save session to disk
int acp_session_save(ACPSession* session) {
    if (!session || !session->active) return -1;

    char* filepath = get_session_filepath(session->session_id);

    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "session_id", session->session_id);
    cJSON_AddStringToObject(root, "agent_name", session->agent_name);
    cJSON_AddStringToObject(root, "cwd", session->cwd);
    cJSON_AddNumberToObject(root, "timestamp", (double)time(NULL));

    // Save message history
    cJSON* messages = cJSON_CreateArray();
    for (int i = 0; i < session->message_count; i++) {
        cJSON* msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", session->messages[i].role);
        cJSON_AddStringToObject(msg, "content", session->messages[i].content ? session->messages[i].content : "");
        cJSON_AddNumberToObject(msg, "timestamp", (double)session->messages[i].timestamp);
        cJSON_AddItemToArray(messages, msg);
    }
    cJSON_AddItemToObject(root, "messages", messages);

    char* json = cJSON_Print(root);
    cJSON_Delete(root);

    FILE* f = fopen(filepath, "w");
    if (f) {
        fputs(json, f);
        fclose(f);
        free(json);
        free(filepath);
        return 0;
    }

    free(json);
    free(filepath);
    return -1;
}

// Load session from SQLite unified persistence (fallback to JSON for backward compatibility)
ACPSession* acp_session_load(const char* session_id) {
    if (g_server.session_count >= ACP_MAX_SESSIONS) {
        return NULL;
    }

    ACPSession* session = &g_server.sessions[g_server.session_count++];
    memset(session, 0, sizeof(ACPSession));

    strncpy(session->session_id, session_id, sizeof(session->session_id) - 1);
    session->active = true;
    session->orchestrator_ctx = NULL;

    // Try to load messages from SQLite unified persistence
    size_t msg_count = 0;
    ACPHistoryMessage* history = persistence_load_acp_messages(session_id, ACP_MAX_MESSAGES, &msg_count);

    if (history && msg_count > 0) {
        // Successfully loaded from SQLite
        for (size_t i = 0; i < msg_count && session->message_count < ACP_MAX_MESSAGES; i++) {
            strncpy(session->messages[session->message_count].role,
                    history[i].role, sizeof(session->messages[0].role) - 1);
            session->messages[session->message_count].content =
                history[i].content ? strdup(history[i].content) : NULL;
            session->messages[session->message_count].timestamp = history[i].timestamp;
            session->message_count++;
        }
        persistence_free_acp_messages(history, msg_count);
        return session;
    }

    // Fallback: try to load from legacy JSON file
    char* filepath = get_session_filepath(session_id);
    FILE* f = fopen(filepath, "r");
    if (!f) {
        free(filepath);
        return session; // No messages found, but session is valid (empty)
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = malloc(fsize + 1);
    fread(content, 1, fsize, f);
    content[fsize] = '\0';
    fclose(f);
    free(filepath);

    cJSON* root = cJSON_Parse(content);
    free(content);
    if (!root) return session;

    // Load basic info from JSON
    cJSON* item = cJSON_GetObjectItem(root, "agent_name");
    if (item && cJSON_IsString(item)) {
        strncpy(session->agent_name, item->valuestring, sizeof(session->agent_name) - 1);
    }

    item = cJSON_GetObjectItem(root, "cwd");
    if (item && cJSON_IsString(item)) {
        strncpy(session->cwd, item->valuestring, sizeof(session->cwd) - 1);
    }

    // Load and migrate message history from JSON to SQLite
    cJSON* messages = cJSON_GetObjectItem(root, "messages");
    if (messages && cJSON_IsArray(messages)) {
        cJSON* msg;
        cJSON_ArrayForEach(msg, messages) {
            if (session->message_count >= ACP_MAX_MESSAGES) break;

            cJSON* role = cJSON_GetObjectItem(msg, "role");
            cJSON* content_item = cJSON_GetObjectItem(msg, "content");
            cJSON* ts = cJSON_GetObjectItem(msg, "timestamp");

            if (role && cJSON_IsString(role) && content_item && cJSON_IsString(content_item)) {
                strncpy(session->messages[session->message_count].role,
                        role->valuestring, sizeof(session->messages[0].role) - 1);
                session->messages[session->message_count].content = strdup(content_item->valuestring);
                session->messages[session->message_count].timestamp =
                    ts && cJSON_IsNumber(ts) ? (long)ts->valuedouble : time(NULL);

                // Migrate to SQLite unified persistence
                persistence_save_acp_message(session_id, session->agent_name,
                    role->valuestring, content_item->valuestring);

                session->message_count++;
            }
        }
    }

    cJSON_Delete(root);
    return session;
}

// Add message to session history (in-memory and SQLite unified persistence)
void acp_session_add_message(ACPSession* session, const char* role, const char* content) {
    if (!session || !role || !content) return;
    if (session->message_count >= ACP_MAX_MESSAGES) {
        // Shift messages to make room (remove oldest)
        if (session->messages[0].content) free(session->messages[0].content);
        memmove(&session->messages[0], &session->messages[1],
                sizeof(ACPMessage) * (ACP_MAX_MESSAGES - 1));
        session->message_count--;
    }

    int idx = session->message_count++;
    strncpy(session->messages[idx].role, role, sizeof(session->messages[0].role) - 1);
    session->messages[idx].content = strdup(content);
    session->messages[idx].timestamp = time(NULL);

    // Also save to SQLite unified persistence (CLI + Zed share same storage)
    persistence_save_acp_message(session->session_id, session->agent_name, role, content);
}

static char* generate_session_id(void) {
    static char id[64];
    snprintf(id, sizeof(id), "sess_%d_%ld", g_server.session_count + 1, time(NULL));
    return id;
}

static ACPSession* find_session(const char* session_id) {
    for (int i = 0; i < g_server.session_count; i++) {
        if (g_server.sessions[i].active &&
            strcmp(g_server.sessions[i].session_id, session_id) == 0) {
            return &g_server.sessions[i];
        }
    }
    return NULL;
}

static ACPSession* create_session(const char* cwd, const char* agent_name) {
    if (g_server.session_count >= ACP_MAX_SESSIONS) {
        return NULL;
    }

    ACPSession* session = &g_server.sessions[g_server.session_count++];
    memset(session, 0, sizeof(ACPSession));

    // Set agent name first (needed for SQLite session lookup)
    if (agent_name) {
        strncpy(session->agent_name, agent_name, sizeof(session->agent_name) - 1);
    } else if (g_server.selected_agent[0] != '\0') {
        snprintf(session->agent_name, sizeof(session->agent_name), "Convergio-%s", g_server.selected_agent);
    }

    // Use SQLite unified persistence to get/create session ID
    // This enables CLI and Zed to share the same session storage
    char* sqlite_session_id = persistence_get_or_create_agent_session(session->agent_name);
    if (sqlite_session_id) {
        strncpy(session->session_id, sqlite_session_id, sizeof(session->session_id) - 1);
        free(sqlite_session_id);
    } else {
        // Fallback to local session ID generation
        strncpy(session->session_id, generate_session_id(), sizeof(session->session_id) - 1);
    }

    strncpy(session->cwd, cwd ? cwd : ".", sizeof(session->cwd) - 1);
    session->active = true;
    session->orchestrator_ctx = NULL;
    session->message_count = 0;

    // Initialize async processing state
    session->has_worker = false;
    session->worker_cancelled = false;
    pthread_mutex_init(&session->mutex, NULL);

    return session;
}

// ============================================================================
// PROTOCOL HANDLERS
// ============================================================================

void acp_handle_initialize(int request_id, const char* params_json) {
    (void)params_json;

    g_server.initialized = true;
    g_server.protocol_version = ACP_PROTOCOL_VERSION;

    // Determine agent name and title
    const char* agent_name = "convergio";
    char agent_title[128] = "Convergio AI Assistant";

    if (g_server.selected_agent[0] != '\0') {
        // Specific agent selected
        ManagedAgent* agent = agent_find_by_name(g_server.selected_agent);
        if (agent) {
            agent_name = agent->name;
            snprintf(agent_title, sizeof(agent_title), "Convergio: %s", agent->name);
        }
    }

    // Build response following exact ACP schema
    cJSON* result = cJSON_CreateObject();
    cJSON_AddNumberToObject(result, "protocolVersion", ACP_PROTOCOL_VERSION);

    // agentInfo (required fields: name, version; optional: title)
    cJSON* agent_info = cJSON_CreateObject();
    cJSON_AddStringToObject(agent_info, "name", agent_name);
    cJSON_AddStringToObject(agent_info, "version", convergio_get_version());
    cJSON_AddStringToObject(agent_info, "title", agent_title);
    cJSON_AddItemToObject(result, "agentInfo", agent_info);

    // agentCapabilities (ACP schema format)
    cJSON* caps = cJSON_CreateObject();
    cJSON_AddBoolToObject(caps, "loadSession", false);

    cJSON* mcp_caps = cJSON_CreateObject();
    cJSON_AddBoolToObject(mcp_caps, "http", false);
    cJSON_AddBoolToObject(mcp_caps, "sse", false);
    cJSON_AddItemToObject(caps, "mcpCapabilities", mcp_caps);

    cJSON* prompt_caps = cJSON_CreateObject();
    cJSON_AddBoolToObject(prompt_caps, "image", false);
    cJSON_AddBoolToObject(prompt_caps, "audio", false);
    cJSON_AddBoolToObject(prompt_caps, "embeddedContext", true);  // X3: Enable editor context
    cJSON_AddItemToObject(caps, "promptCapabilities", prompt_caps);

    cJSON* session_caps = cJSON_CreateObject();
    cJSON_AddItemToObject(caps, "sessionCapabilities", session_caps);

    cJSON_AddItemToObject(result, "agentCapabilities", caps);

    // authMethods (empty array = no auth required)
    cJSON* auth = cJSON_CreateArray();
    cJSON_AddItemToObject(result, "authMethods", auth);

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);
}

void acp_handle_session_new(int request_id, const char* params_json) {
    char cwd[1024] = ".";
    char resume_session_id[64] = {0};

    if (params_json) {
        cJSON* params = cJSON_Parse(params_json);
        if (params) {
            cJSON* cwd_item = cJSON_GetObjectItem(params, "cwd");
            if (cwd_item && cJSON_IsString(cwd_item)) {
                strncpy(cwd, cwd_item->valuestring, sizeof(cwd) - 1);
            }
            // Check for resume session ID (optional, for explicit resume)
            cJSON* resume_item = cJSON_GetObjectItem(params, "resumeSessionId");
            if (resume_item && cJSON_IsString(resume_item)) {
                strncpy(resume_session_id, resume_item->valuestring, sizeof(resume_session_id) - 1);
            }
            cJSON_Delete(params);
        }
    }

    ACPSession* session = NULL;
    bool resumed = false;

    // Try to resume existing session by explicit ID
    if (resume_session_id[0] != '\0') {
        session = find_session(resume_session_id);
        if (!session) {
            session = acp_session_load(resume_session_id);
        }
        if (session) {
            resumed = true;
            if (cwd[0] != '.' || cwd[1] != '\0') {
                strncpy(session->cwd, cwd, sizeof(session->cwd) - 1);
            }
        }
    }

    // If no explicit session ID, try to auto-resume by agent name
    if (!session && g_server.selected_agent[0] != '\0') {
        char agent_name[128];
        snprintf(agent_name, sizeof(agent_name), "Convergio-%s", g_server.selected_agent);

        char* found_session_id = find_session_by_agent_name(agent_name);
        if (found_session_id) {
            // Check if already in memory
            session = find_session(found_session_id);
            if (!session) {
                session = acp_session_load(found_session_id);
            }
            if (session) {
                resumed = true;
                if (cwd[0] != '.' || cwd[1] != '\0') {
                    strncpy(session->cwd, cwd, sizeof(session->cwd) - 1);
                }
            }
            free(found_session_id);
        }
    }

    // Create new session if not resuming
    if (!session) {
        session = create_session(cwd, NULL);
        if (session) {
            // Save new session immediately so it persists even if no prompts are sent
            acp_session_save(session);
        }
    }

    if (!session) {
        acp_send_error(request_id, -32000, "Max sessions reached");
        return;
    }

    // Initialize workspace for tools
    extern void tools_init_workspace(const char* path);
    tools_init_workspace(session->cwd);

    // Build response
    cJSON* result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "sessionId", session->session_id);

    // If resumed, send previous messages as context
    if (resumed && session->message_count > 0) {
        cJSON* history = cJSON_CreateArray();
        for (int i = 0; i < session->message_count; i++) {
            cJSON* msg = cJSON_CreateObject();
            cJSON_AddStringToObject(msg, "role", session->messages[i].role);
            cJSON_AddStringToObject(msg, "content", session->messages[i].content ? session->messages[i].content : "");
            cJSON_AddItemToArray(history, msg);
        }
        cJSON_AddItemToObject(result, "history", history);
        cJSON_AddBoolToObject(result, "resumed", true);
        cJSON_AddNumberToObject(result, "messageCount", session->message_count);
    }

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);

    // Send session/history notification for resumed sessions with history
    // This is a custom Convergio extension - standard ACP clients will ignore it
    // LAZY LOAD: Only send last N messages initially, client can request more
    if (resumed && session->message_count > 0) {
        cJSON* history_params = cJSON_CreateObject();
        cJSON_AddStringToObject(history_params, "sessionId", session->session_id);

        // Calculate lazy load range: send only last ACP_LAZY_LOAD_INITIAL messages
        int total_count = session->message_count;
        int start_index = (total_count > ACP_LAZY_LOAD_INITIAL)
                          ? (total_count - ACP_LAZY_LOAD_INITIAL) : 0;
        int messages_sent = total_count - start_index;
        bool has_more = (start_index > 0);

        cJSON* messages = cJSON_CreateArray();
        for (int i = start_index; i < total_count; i++) {
            cJSON* msg = cJSON_CreateObject();
            cJSON_AddStringToObject(msg, "role", session->messages[i].role);
            cJSON_AddStringToObject(msg, "content",
                session->messages[i].content ? session->messages[i].content : "");
            cJSON_AddNumberToObject(msg, "timestamp", session->messages[i].timestamp);
            cJSON_AddNumberToObject(msg, "index", i);  // Include index for loadMore
            cJSON_AddItemToArray(messages, msg);
        }
        cJSON_AddItemToObject(history_params, "messages", messages);
        cJSON_AddBoolToObject(history_params, "compacted", false);

        // Lazy load metadata
        cJSON_AddBoolToObject(history_params, "hasMore", has_more);
        cJSON_AddNumberToObject(history_params, "totalCount", total_count);
        cJSON_AddNumberToObject(history_params, "startIndex", start_index);

        char* history_json = cJSON_PrintUnformatted(history_params);
        acp_send_notification("session/history", history_json);
        free(history_json);
        cJSON_Delete(history_params);

        LOG_INFO(LOG_CAT_SYSTEM, "Sent session/history notification with %d/%d messages (lazy load, hasMore=%s)",
                 messages_sent, total_count, has_more ? "true" : "false");
    }
}

// Buffer chunk for background session
static void buffer_chunk_for_session(ACPSession* session, const char* chunk) {
    if (!session || !chunk) return;

    size_t chunk_len = strlen(chunk);
    size_t needed = session->background_buffer_len + chunk_len + 1;

    // Check session buffer size limit
    if (needed > MAX_SESSION_BUFFER_SIZE) {
        LOG_WARN(LOG_CAT_SYSTEM, "Session buffer would exceed MAX_SESSION_BUFFER_SIZE (%zu > %d), dropping chunk",
                 needed, MAX_SESSION_BUFFER_SIZE);
        return;
    }

    // Allocate or expand buffer
    if (!session->background_buffer) {
        session->background_buffer_cap = (needed > ACP_BACKGROUND_BUFFER_SIZE)
                                          ? needed : ACP_BACKGROUND_BUFFER_SIZE;

        // Check global memory limit before allocating
        pthread_mutex_lock(&g_memory_mutex);
        size_t total_after_alloc = g_total_buffer_memory + session->background_buffer_cap;
        if (total_after_alloc > MAX_MEMORY_MB * 1024 * 1024) {
            pthread_mutex_unlock(&g_memory_mutex);
            LOG_WARN(LOG_CAT_SYSTEM, "Total buffer memory would exceed MAX_MEMORY_MB (%zu MB > %d MB), dropping chunk",
                     total_after_alloc / (1024 * 1024), MAX_MEMORY_MB);
            return;
        }
        g_total_buffer_memory += session->background_buffer_cap;
        pthread_mutex_unlock(&g_memory_mutex);

        session->background_buffer = malloc(session->background_buffer_cap);
        session->background_buffer[0] = '\0';
        session->background_buffer_len = 0;
    } else if (needed > session->background_buffer_cap) {
        size_t old_cap = session->background_buffer_cap;
        session->background_buffer_cap = needed * 2;

        // Check if new capacity would exceed session limit
        if (session->background_buffer_cap > MAX_SESSION_BUFFER_SIZE) {
            session->background_buffer_cap = MAX_SESSION_BUFFER_SIZE;
        }

        // Check global memory limit before expanding
        pthread_mutex_lock(&g_memory_mutex);
        size_t additional_memory = session->background_buffer_cap - old_cap;
        size_t total_after_realloc = g_total_buffer_memory + additional_memory;
        if (total_after_realloc > MAX_MEMORY_MB * 1024 * 1024) {
            pthread_mutex_unlock(&g_memory_mutex);
            LOG_WARN(LOG_CAT_SYSTEM, "Total buffer memory would exceed MAX_MEMORY_MB (%zu MB > %d MB), dropping chunk",
                     total_after_realloc / (1024 * 1024), MAX_MEMORY_MB);
            return;
        }
        g_total_buffer_memory += additional_memory;
        pthread_mutex_unlock(&g_memory_mutex);

        session->background_buffer = realloc(session->background_buffer,
                                              session->background_buffer_cap);
    }

    // Append chunk using memcpy (safer than strcat)
    memcpy(session->background_buffer + session->background_buffer_len, chunk, chunk_len + 1);
    session->background_buffer_len += chunk_len;
}

// Streaming callback for orchestrator
static void stream_callback(const char* chunk, void* user_data) {
    (void)user_data;

    if (!chunk || strlen(chunk) == 0) return;

    // Check if session is in background mode
    ACPSession* session = find_session(g_current_session_id);
    if (session && session->is_background) {
        // Buffer the chunk instead of sending
        buffer_chunk_for_session(session, chunk);
        return;
    }

    // Build session/update notification (ACP schema format)
    cJSON* params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "sessionId", g_current_session_id);

    cJSON* update = cJSON_CreateObject();
    cJSON_AddStringToObject(update, "sessionUpdate", "agent_message_chunk");

    // Content block: { "type": "text", "text": "..." }
    cJSON* content = cJSON_CreateObject();
    cJSON_AddStringToObject(content, "type", "text");
    cJSON_AddStringToObject(content, "text", chunk);
    cJSON_AddItemToObject(update, "content", content);

    cJSON_AddItemToObject(params, "update", update);

    char* params_json = cJSON_PrintUnformatted(params);
    acp_send_notification("session/update", params_json);
    free(params_json);
    cJSON_Delete(params);
}

void acp_handle_session_prompt(int request_id, const char* params_json) {
    if (!params_json) {
        acp_send_error(request_id, -32602, "Missing params");
        return;
    }

    cJSON* params = cJSON_Parse(params_json);
    if (!params) {
        acp_send_error(request_id, -32700, "Parse error");
        return;
    }

    // Get session ID
    cJSON* session_id_item = cJSON_GetObjectItem(params, "sessionId");
    if (!session_id_item || !cJSON_IsString(session_id_item)) {
        cJSON_Delete(params);
        acp_send_error(request_id, -32602, "Missing sessionId");
        return;
    }

    // Copy session ID before freeing params (avoid use-after-free)
    char session_id[64] = {0};
    strncpy(session_id, session_id_item->valuestring, sizeof(session_id) - 1);

    ACPSession* session = find_session(session_id);
    if (!session) {
        cJSON_Delete(params);
        acp_send_error(request_id, -32000, "Session not found");
        return;
    }

    // Check if session has an active or finished worker
    pthread_mutex_lock(&session->mutex);
    if (session->has_worker) {
        if (session->is_processing) {
            // Worker is still running - reject new prompt
            pthread_mutex_unlock(&session->mutex);
            cJSON_Delete(params);
            acp_send_error(request_id, -32000, "Session busy - prompt already in progress");
            return;
        } else {
            // Worker finished but wasn't joined yet - join it now
            pthread_t old_thread = session->worker_thread;
            session->has_worker = false;
            pthread_mutex_unlock(&session->mutex);

            pthread_join(old_thread, NULL);
            LOG_DEBUG(LOG_CAT_SYSTEM, "Joined completed worker thread for session %s", session_id);
        }
    } else {
        pthread_mutex_unlock(&session->mutex);
    }

    // Extract prompt text and context (ACP format: prompt[])
    cJSON* prompt_array = cJSON_GetObjectItem(params, "prompt");
    char prompt_text[24576] = {0};  // Buffer for prompt + context combined
    char context_text[8192] = {0};  // Buffer for embedded context

    if (prompt_array && cJSON_IsArray(prompt_array)) {
        cJSON* item;
        cJSON_ArrayForEach(item, prompt_array) {
            cJSON* type = cJSON_GetObjectItem(item, "type");
            if (!type || !cJSON_IsString(type)) continue;

            if (strcmp(type->valuestring, "text") == 0) {
                // ACP format: { "type": "text", "text": "..." }
                cJSON* text = cJSON_GetObjectItem(item, "text");
                if (text && cJSON_IsString(text)) {
                    strncat(prompt_text, text->valuestring, sizeof(prompt_text) - strlen(prompt_text) - 1);
                }
            } else if (strcmp(type->valuestring, "context") == 0) {
                // X3: Handle embedded context (file, selection, cursor)
                // ACP format: { "type": "context", "path": "...", "content": "...", "selection": {...} }
                cJSON* path = cJSON_GetObjectItem(item, "path");
                cJSON* content = cJSON_GetObjectItem(item, "content");
                cJSON* selection = cJSON_GetObjectItem(item, "selection");

                size_t ctx_remaining = sizeof(context_text) - strlen(context_text) - 1;
                if (path && cJSON_IsString(path) && ctx_remaining > 0) {
                    char ctx_header[512];
                    snprintf(ctx_header, sizeof(ctx_header), "\n[File: %s]\n", path->valuestring);
                    strncat(context_text, ctx_header, ctx_remaining);
                }
                if (content && cJSON_IsString(content)) {
                    ctx_remaining = sizeof(context_text) - strlen(context_text) - 1;
                    if (ctx_remaining > 0) {
                        strncat(context_text, content->valuestring, ctx_remaining);
                        ctx_remaining = sizeof(context_text) - strlen(context_text) - 1;
                        if (ctx_remaining > 0) {
                            strncat(context_text, "\n", ctx_remaining);
                        }
                    }
                }
                if (selection && cJSON_IsObject(selection)) {
                    cJSON* sel_text = cJSON_GetObjectItem(selection, "text");
                    if (sel_text && cJSON_IsString(sel_text)) {
                        ctx_remaining = sizeof(context_text) - strlen(context_text) - 1;
                        if (ctx_remaining > 20) {
                            char sel_header[64] = "\n[Selection]:\n";
                            strncat(context_text, sel_header, ctx_remaining);
                            ctx_remaining = sizeof(context_text) - strlen(context_text) - 1;
                            if (ctx_remaining > 0) {
                                strncat(context_text, sel_text->valuestring, ctx_remaining);
                                ctx_remaining = sizeof(context_text) - strlen(context_text) - 1;
                                if (ctx_remaining > 0) {
                                    strncat(context_text, "\n", ctx_remaining);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Prepend context to prompt if available
    if (strlen(context_text) > 0) {
        char combined[24576];
        snprintf(combined, sizeof(combined), "[Editor Context]%s\n[User Message]\n%s",
                 context_text, prompt_text);
        strncpy(prompt_text, combined, sizeof(prompt_text) - 1);
    }

    cJSON_Delete(params);

    if (strlen(prompt_text) == 0) {
        acp_send_error(request_id, -32602, "Empty prompt");
        return;
    }

    // ASYNC EXECUTION: Spawn worker thread for prompt processing
    // This allows the main loop to continue reading stdin (e.g., session/background)
    PromptWorkerArgs* args = malloc(sizeof(PromptWorkerArgs));
    if (!args) {
        acp_send_error(request_id, -32000, "Failed to allocate worker args");
        return;
    }

    args->session = session;
    args->prompt_text = strdup(prompt_text);
    args->request_id = request_id;
    args->selected_agent = g_server.selected_agent[0] != '\0'
                           ? strdup(g_server.selected_agent)
                           : NULL;
    strncpy(args->session_id, session_id, sizeof(args->session_id) - 1);

    // Mark session as processing before spawning thread
    pthread_mutex_lock(&session->mutex);
    session->is_processing = true;
    session->worker_cancelled = false;
    pthread_mutex_unlock(&session->mutex);

    // Spawn worker thread
    int rc = pthread_create(&session->worker_thread, NULL, prompt_worker_thread, args);
    if (rc != 0) {
        // Thread creation failed - clean up and report error
        pthread_mutex_lock(&session->mutex);
        session->is_processing = false;
        pthread_mutex_unlock(&session->mutex);

        free(args->prompt_text);
        if (args->selected_agent) free(args->selected_agent);
        free(args);

        acp_send_error(request_id, -32000, "Failed to spawn worker thread");
        return;
    }

    // Mark session as having an active worker
    pthread_mutex_lock(&session->mutex);
    session->has_worker = true;
    pthread_mutex_unlock(&session->mutex);

    // NOTE: Thread is NOT detached - cleanup_sessions() will join it on shutdown
    // This ensures proper cleanup and avoids undefined behavior

    LOG_INFO(LOG_CAT_SYSTEM, "Spawned worker thread for session %s", session_id);

    // NOTE: Response will be sent by worker thread when complete
    // Main loop can now continue reading stdin for other requests
}

void acp_handle_session_cancel(int request_id, const char* params_json) {
    const char* session_id = NULL;
    char session_id_from_params[64] = {0};

    // Parse session_id from params if provided
    if (params_json) {
        cJSON* params = cJSON_Parse(params_json);
        if (params) {
            cJSON* sid = cJSON_GetObjectItem(params, "sessionId");
            if (sid && cJSON_IsString(sid)) {
                strncpy(session_id_from_params, sid->valuestring, sizeof(session_id_from_params) - 1);
                session_id = session_id_from_params;
            }
            cJSON_Delete(params);
        }
    }

    // Fallback to current session if no session_id provided
    if (!session_id || session_id[0] == '\0') {
        session_id = g_current_session_id;
    }

    // Find and cancel the session
    bool cancelled = false;
    if (session_id && session_id[0] != '\0') {
        ACPSession* session = find_session(session_id);
        if (session) {
            // Cancel any active worker thread
            pthread_mutex_lock(&session->mutex);
            if (session->has_worker) {
                session->worker_cancelled = true;
                pthread_t worker = session->worker_thread;
                bool is_processing = session->is_processing;
                session->has_worker = false;
                pthread_mutex_unlock(&session->mutex);

                // Wait for worker to finish (it should check worker_cancelled flag)
                // Note: This may block briefly while worker finishes current operation
                if (is_processing) {
                    LOG_INFO(LOG_CAT_SYSTEM, "Waiting for worker thread to finish for cancel...");
                }
                pthread_join(worker, NULL);
            } else {
                pthread_mutex_unlock(&session->mutex);
            }

            // Free message memory
            for (int i = 0; i < session->message_count; i++) {
                if (session->messages[i].content) {
                    free(session->messages[i].content);
                    session->messages[i].content = NULL;
                }
            }
            session->message_count = 0;

            // Free background buffer and update memory tracking
            if (session->background_buffer) {
                pthread_mutex_lock(&g_memory_mutex);
                if (g_total_buffer_memory >= session->background_buffer_cap) {
                    g_total_buffer_memory -= session->background_buffer_cap;
                } else {
                    g_total_buffer_memory = 0;
                }
                pthread_mutex_unlock(&g_memory_mutex);

                free(session->background_buffer);
                session->background_buffer = NULL;
                session->background_buffer_len = 0;
                session->background_buffer_cap = 0;
            }

            // Mark session as inactive
            session->active = false;
            session->is_processing = false;

            // Remove persistence file
            char* filepath = get_session_filepath(session_id);
            if (filepath) {
                unlink(filepath);
                free(filepath);
            }

            // Clear current session if it was the cancelled one
            if (strcmp(g_current_session_id, session_id) == 0) {
                g_current_session_id[0] = '\0';
            }

            cancelled = true;
            LOG_DEBUG(LOG_CAT_SYSTEM, "Session cancelled: %s", session_id);
        }
    }

    cJSON* result = cJSON_CreateObject();
    cJSON_AddBoolToObject(result, "cancelled", cancelled);

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);
}

// ============================================================================
// LAZY LOAD: Load more history messages
// ============================================================================

void acp_handle_session_load_more(int request_id, const char* params_json) {
    if (!params_json) {
        acp_send_error(request_id, -32602, "Missing params");
        return;
    }

    cJSON* params = cJSON_Parse(params_json);
    if (!params) {
        acp_send_error(request_id, -32700, "Parse error");
        return;
    }

    // Get session ID
    cJSON* session_id_item = cJSON_GetObjectItem(params, "sessionId");
    if (!session_id_item || !cJSON_IsString(session_id_item)) {
        cJSON_Delete(params);
        acp_send_error(request_id, -32602, "Missing sessionId");
        return;
    }

    const char* session_id = session_id_item->valuestring;
    ACPSession* session = find_session(session_id);

    if (!session) {
        cJSON_Delete(params);
        acp_send_error(request_id, -32602, "Session not found");
        return;
    }

    // Get beforeIndex (load messages before this index)
    cJSON* before_index_item = cJSON_GetObjectItem(params, "beforeIndex");
    int before_index = before_index_item && cJSON_IsNumber(before_index_item)
                       ? before_index_item->valueint
                       : session->message_count;

    // Get limit (how many messages to load)
    cJSON* limit_item = cJSON_GetObjectItem(params, "limit");
    int limit = limit_item && cJSON_IsNumber(limit_item)
                ? limit_item->valueint
                : ACP_LAZY_LOAD_INITIAL;

    cJSON_Delete(params);

    // Calculate range to load
    int end_index = before_index;
    int start_index = (end_index > limit) ? (end_index - limit) : 0;
    bool has_more = (start_index > 0);

    // Build response
    cJSON* result = cJSON_CreateObject();
    cJSON* messages = cJSON_CreateArray();

    for (int i = start_index; i < end_index && i < session->message_count; i++) {
        cJSON* msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", session->messages[i].role);
        cJSON_AddStringToObject(msg, "content",
            session->messages[i].content ? session->messages[i].content : "");
        cJSON_AddNumberToObject(msg, "timestamp", session->messages[i].timestamp);
        cJSON_AddNumberToObject(msg, "index", i);
        cJSON_AddItemToArray(messages, msg);
    }

    cJSON_AddItemToObject(result, "messages", messages);
    cJSON_AddBoolToObject(result, "hasMore", has_more);
    cJSON_AddNumberToObject(result, "startIndex", start_index);
    cJSON_AddNumberToObject(result, "totalCount", session->message_count);

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);

    LOG_INFO(LOG_CAT_SYSTEM, "Loaded %d more messages (index %d-%d, hasMore=%s)",
             end_index - start_index, start_index, end_index - 1,
             has_more ? "true" : "false");
}

// ============================================================================
// BACKGROUND EXECUTION: Allow agents to continue when user switches
// ============================================================================

void acp_handle_session_background(int request_id, const char* params_json) {
    if (!params_json) {
        acp_send_error(request_id, -32602, "Missing params");
        return;
    }

    cJSON* params = cJSON_Parse(params_json);
    if (!params) {
        acp_send_error(request_id, -32700, "Parse error");
        return;
    }

    cJSON* session_id_item = cJSON_GetObjectItem(params, "sessionId");
    if (!session_id_item || !cJSON_IsString(session_id_item)) {
        cJSON_Delete(params);
        acp_send_error(request_id, -32602, "Missing sessionId");
        return;
    }

    const char* session_id = session_id_item->valuestring;
    ACPSession* session = find_session(session_id);
    cJSON_Delete(params);

    if (!session) {
        acp_send_error(request_id, -32602, "Session not found");
        return;
    }

    session->is_background = true;
    LOG_INFO(LOG_CAT_SYSTEM, "Session %s moved to background", session_id);

    cJSON* result = cJSON_CreateObject();
    cJSON_AddBoolToObject(result, "success", true);
    cJSON_AddBoolToObject(result, "isProcessing", session->is_processing);

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);
}

void acp_handle_session_foreground(int request_id, const char* params_json) {
    if (!params_json) {
        acp_send_error(request_id, -32602, "Missing params");
        return;
    }

    cJSON* params = cJSON_Parse(params_json);
    if (!params) {
        acp_send_error(request_id, -32700, "Parse error");
        return;
    }

    cJSON* session_id_item = cJSON_GetObjectItem(params, "sessionId");
    if (!session_id_item || !cJSON_IsString(session_id_item)) {
        cJSON_Delete(params);
        acp_send_error(request_id, -32602, "Missing sessionId");
        return;
    }

    const char* session_id = session_id_item->valuestring;
    ACPSession* session = find_session(session_id);
    cJSON_Delete(params);

    if (!session) {
        acp_send_error(request_id, -32602, "Session not found");
        return;
    }

    session->is_background = false;
    LOG_INFO(LOG_CAT_SYSTEM, "Session %s moved to foreground", session_id);

    cJSON* result = cJSON_CreateObject();
    cJSON_AddBoolToObject(result, "success", true);
    cJSON_AddBoolToObject(result, "isProcessing", session->is_processing);

    // Include buffered content if any
    if (session->background_buffer && session->background_buffer_len > 0) {
        cJSON_AddStringToObject(result, "bufferedContent", session->background_buffer);
        cJSON_AddNumberToObject(result, "bufferedLength", (double)session->background_buffer_len);

        // Clear buffer after sending and update memory tracking
        pthread_mutex_lock(&g_memory_mutex);
        if (g_total_buffer_memory >= session->background_buffer_cap) {
            g_total_buffer_memory -= session->background_buffer_cap;
        } else {
            g_total_buffer_memory = 0;
        }
        pthread_mutex_unlock(&g_memory_mutex);

        free(session->background_buffer);
        session->background_buffer = NULL;
        session->background_buffer_len = 0;
        session->background_buffer_cap = 0;
    }

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);
}

void acp_handle_session_status(int request_id, const char* params_json) {
    if (!params_json) {
        acp_send_error(request_id, -32602, "Missing params");
        return;
    }

    cJSON* params = cJSON_Parse(params_json);
    if (!params) {
        acp_send_error(request_id, -32700, "Parse error");
        return;
    }

    cJSON* session_id_item = cJSON_GetObjectItem(params, "sessionId");
    if (!session_id_item || !cJSON_IsString(session_id_item)) {
        cJSON_Delete(params);
        acp_send_error(request_id, -32602, "Missing sessionId");
        return;
    }

    const char* session_id = session_id_item->valuestring;
    ACPSession* session = find_session(session_id);
    cJSON_Delete(params);

    if (!session) {
        acp_send_error(request_id, -32602, "Session not found");
        return;
    }

    cJSON* result = cJSON_CreateObject();
    cJSON_AddBoolToObject(result, "active", session->active);
    cJSON_AddBoolToObject(result, "isBackground", session->is_background);
    cJSON_AddBoolToObject(result, "isProcessing", session->is_processing);
    cJSON_AddNumberToObject(result, "messageCount", session->message_count);
    cJSON_AddNumberToObject(result, "bufferedLength", (double)session->background_buffer_len);

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);
}

// ============================================================================
// REQUEST DISPATCHER
// ============================================================================

static void dispatch_request(cJSON* request) {
    cJSON* id_item = cJSON_GetObjectItem(request, "id");
    cJSON* method_item = cJSON_GetObjectItem(request, "method");
    cJSON* params_item = cJSON_GetObjectItem(request, "params");

    int request_id = id_item ? id_item->valueint : 0;
    const char* method = method_item ? method_item->valuestring : "";

    char* params_json = NULL;
    if (params_item) {
        params_json = cJSON_PrintUnformatted(params_item);
    }

    // Dispatch based on method
    if (strcmp(method, "initialize") == 0) {
        acp_handle_initialize(request_id, params_json);
    } else if (strcmp(method, "session/new") == 0) {
        acp_handle_session_new(request_id, params_json);
    } else if (strcmp(method, "session/prompt") == 0) {
        acp_handle_session_prompt(request_id, params_json);
    } else if (strcmp(method, "session/cancel") == 0) {
        acp_handle_session_cancel(request_id, params_json);
    } else if (strcmp(method, "session/loadMore") == 0) {
        acp_handle_session_load_more(request_id, params_json);
    } else if (strcmp(method, "session/background") == 0) {
        acp_handle_session_background(request_id, params_json);
    } else if (strcmp(method, "session/foreground") == 0) {
        acp_handle_session_foreground(request_id, params_json);
    } else if (strcmp(method, "session/status") == 0) {
        acp_handle_session_status(request_id, params_json);
    } else if (strcmp(method, "shutdown") == 0) {
        acp_send_response(request_id, "{}");
        g_running = 0;
    } else {
        acp_send_error(request_id, -32601, "Method not found");
    }

    if (params_json) {
        free(params_json);
    }
}

// ============================================================================
// MAIN LOOP
// ============================================================================

int acp_server_init(void) {
    // Perform crash recovery check (Phase 11 S5)
    crash_recovery_check();

    // Set resource limits (Phase 11 S6)
    struct rlimit rlim;
    rlim.rlim_cur = MAX_OPEN_FILES;
    rlim.rlim_max = MAX_OPEN_FILES;
    if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
        fprintf(stderr, "Warning: Failed to set RLIMIT_NOFILE to %d: %s\n",
                MAX_OPEN_FILES, strerror(errno));
        // Non-fatal, continue with system defaults
    } else {
        LOG_DEBUG(LOG_CAT_SYSTEM, "Set RLIMIT_NOFILE to %d", MAX_OPEN_FILES);
    }

    // Initialize config
    if (convergio_config_init() != 0) {
        fprintf(stderr, "Failed to initialize config\n");
        return -1;
    }

    // Initialize auth
    extern int auth_init(void);
    if (auth_init() != 0) {
        fprintf(stderr, "Warning: No API key configured\n");
    }

    // Initialize core systems
    if (nous_init() != 0) {
        fprintf(stderr, "Failed to initialize nous\n");
        return -1;
    }

    // Initialize orchestrator with generous budget for ACP sessions
    if (orchestrator_init(100.0) != 0) {
        fprintf(stderr, "Failed to initialize orchestrator\n");
        return -1;
    }

    // Initialize memory system for Ali's historical memory
    if (memory_init() != 0) {
        fprintf(stderr, "Warning: Failed to initialize memory system\n");
        // Non-fatal, continue without memory
    }

    // Reset session to clear any budget_exceeded state from previous runs
    extern void cost_reset_session(void);
    cost_reset_session();

    return 0;
}

int acp_server_run(void) {
    char line[ACP_MAX_LINE_LENGTH];
    size_t line_pos = 0;

    // Setup signal handlers for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Setup crash signal handlers for cleanup attempt
    signal(SIGSEGV, handle_crash_signal);
    signal(SIGABRT, handle_crash_signal);

    // Main loop - read JSON-RPC from stdin byte by byte
    int eof_count = 0;
    const int MAX_EOF_RETRIES = 10;  // Exit after 1 second of EOF

    while (g_running) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);

        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (n == 0) {
            // EOF - parent process likely terminated
            eof_count++;
            if (eof_count >= MAX_EOF_RETRIES) {
                // Too many consecutive EOFs, exit cleanly
                break;
            }
            usleep(100000);  // Wait 100ms before retry
            continue;
        }

        // Reset EOF counter on successful read
        eof_count = 0;

        // Build line
        if (c == '\n') {
            line[line_pos] = '\0';

            // Skip empty lines
            if (line_pos == 0) {
                continue;
            }

            // Parse JSON
            cJSON* request = cJSON_Parse(line);
            if (!request) {
                acp_send_error(-1, -32700, "Parse error");
            } else {
                dispatch_request(request);
                cJSON_Delete(request);
            }

            line_pos = 0;
        } else if (line_pos < sizeof(line) - 1) {
            line[line_pos++] = c;
        }
    }

    return 0;
}

void acp_server_shutdown(void) {
    // Clean up session memory (message contents, background buffers)
    cleanup_sessions();

    // Remove PID file (Phase 11 S5)
    remove_pid_file();

    memory_shutdown();
    orchestrator_shutdown();
    nous_shutdown();
    convergio_config_shutdown();
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

static void print_usage(const char* prog) {
    fprintf(stderr, "Usage: %s [--agent <name>] [--list-agents]\n", prog);
    fprintf(stderr, "  --agent <name>   Route to specific agent (default: ali)\n");
    fprintf(stderr, "  --list-agents    List available agents and exit\n");
}

static void list_agents(void) {
    // Quick init just to load agents
    convergio_config_init();
    nous_init();
    orchestrator_init(1.0);  // Minimal budget for listing

    ManagedAgent* agents[64];
    size_t count = agent_get_all(agents, 64);

    fprintf(stdout, "Available agents (%zu):\n", count);
    for (size_t i = 0; i < count; i++) {
        fprintf(stdout, "  %-20s  %s\n", agents[i]->name,
                agents[i]->description ? agents[i]->description : "");
    }

    orchestrator_shutdown();
    nous_shutdown();
    convergio_config_shutdown();
}

int main(int argc, char** argv) {
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--agent") == 0 && i + 1 < argc) {
            strncpy(g_server.selected_agent, argv[++i], sizeof(g_server.selected_agent) - 1);
        } else if (strcmp(argv[i], "--list-agents") == 0) {
            list_agents();
            return 0;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    // Disable buffering for real-time communication
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (acp_server_init() != 0) {
        return 1;
    }

    int result = acp_server_run();

    acp_server_shutdown();

    return result;
}
