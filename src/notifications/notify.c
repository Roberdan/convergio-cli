/**
 * CONVERGIO NOTIFICATION SYSTEM
 *
 * Native macOS notifications with:
 * - Multiple backends (terminal-notifier, osascript, log)
 * - Automatic fallback chain
 * - Background daemon for scheduled reminders
 * - Health monitoring
 *
 * Apple Silicon Optimized:
 * - E-cores only for daemon work (QOS_CLASS_UTILITY)
 * - Timer coalescing for power efficiency
 * - Adaptive polling (60s → 300s when idle)
 * - Memory-mapped database access
 *
 * Part of Anna Executive Assistant feature.
 * See: ADR-009
 */

#include "nous/notify.h"
#include "nous/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dispatch/dispatch.h>
#include <os/log.h>
#include <mach/mach.h>
#include <sqlite3.h>
#include <pthread.h>
#include <stdatomic.h>
#include <mach-o/dyld.h>

extern char **environ;

// Global database connection (from persistence module)
extern sqlite3* g_db;
extern pthread_mutex_t g_db_mutex;

// ============================================================================
// CONSTANTS
// ============================================================================

#define NOTIFY_LOG_PATH "/tmp/convergio-notifications.log"
#define DAEMON_PID_FILE "/tmp/convergio-daemon.pid"
#define LAUNCH_AGENT_LABEL "io.convergio.daemon"
#define LAUNCH_AGENT_PLIST "~/Library/LaunchAgents/io.convergio.daemon.plist"

// Polling intervals
#define POLL_INTERVAL_NORMAL_NS  (60 * NSEC_PER_SEC)   // 60 seconds
#define POLL_INTERVAL_IDLE_NS    (300 * NSEC_PER_SEC)  // 5 minutes when no pending
#define POLL_INTERVAL_FAST_NS    (30 * NSEC_PER_SEC)   // 30 seconds when many pending
#define TIMER_LEEWAY_NS          (10 * NSEC_PER_SEC)   // 10 second leeway for coalescing

// Batch size for notification processing
#define MAX_BATCH_SIZE 16

// ============================================================================
// STATIC DATA
// ============================================================================

// Logging subsystem (uses Apple's unified logging)
static os_log_t g_notify_log = NULL;

// Available methods (detected at init)
static struct {
    bool terminal_notifier;
    bool osascript;
    bool terminal;
    bool sound;
    bool log;
} g_available_methods = {0};

// Daemon state
static struct {
    dispatch_source_t timer;
    dispatch_queue_t queue;
    sqlite3* db;
    sqlite3_stmt* check_stmt;
    sqlite3_stmt* update_stmt;
    sqlite3_stmt* task_title_stmt;
    _Atomic bool running;
    _Atomic int pending_count;
    uint64_t poll_interval_ns;
    time_t started_at;
    int sent_count;
    int failed_count;
    char last_error[256];
} g_daemon = {0};

// Memory-efficient notification batch (static allocation)
typedef struct {
    int64_t id;
    int64_t task_id;
    char title[256];
    char body[512];
    char sound[64];
} NotificationBatch;

static NotificationBatch g_batch[MAX_BATCH_SIZE];

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static NotifyResult send_via_terminal_notifier(const NotifyOptions* opts);
static NotifyResult send_via_osascript(const NotifyOptions* opts);
static NotifyResult send_via_terminal(const NotifyOptions* opts);
static NotifyResult send_via_sound(const NotifyOptions* opts);
static NotifyResult send_via_log(const NotifyOptions* opts);
static void check_method_availability(void);
static char* escape_quotes(const char* str, char quote_char);
static int daemon_init(void);
static void daemon_check_pending(void);
static void adjust_poll_interval(void);

// ============================================================================
// INITIALIZATION
// ============================================================================

int notify_init(void) {
    // Create os_log for efficient logging
    g_notify_log = os_log_create("io.convergio", "notify");

    // Check which notification methods are available
    check_method_availability();

    os_log_info(g_notify_log, "Notification system initialized");
    os_log_info(g_notify_log, "Available methods: terminal-notifier=%d osascript=%d terminal=%d sound=%d log=%d",
                g_available_methods.terminal_notifier,
                g_available_methods.osascript,
                g_available_methods.terminal,
                g_available_methods.sound,
                g_available_methods.log);

    return 0;
}

void notify_shutdown(void) {
    os_log_info(g_notify_log, "Notification system shutdown");
}

// Check which notification methods are available on this system
static void check_method_availability(void) {
    // terminal-notifier: Check if installed
    g_available_methods.terminal_notifier =
        (system("which terminal-notifier > /dev/null 2>&1") == 0);

    // osascript: Always available on macOS
    g_available_methods.osascript = true;

    // Terminal: Check if we have a TTY
    g_available_methods.terminal = isatty(STDERR_FILENO);

    // Sound: Always available via afplay
    g_available_methods.sound = true;

    // Log: Always available
    g_available_methods.log = true;
}

// ============================================================================
// IMMEDIATE NOTIFICATIONS
// ============================================================================

NotifyResult notify_send(const NotifyOptions* options) {
    if (!options || !options->title || !options->body) {
        return NOTIFY_ERROR_INVALID_ARGS;
    }

    NotifyResult result;

    // Try terminal-notifier first (best UX)
    if (g_available_methods.terminal_notifier) {
        result = send_via_terminal_notifier(options);
        if (result == NOTIFY_SUCCESS) {
            os_log_debug(g_notify_log, "Sent via terminal-notifier");
            return result;
        }
    }

    // Fallback to osascript
    if (g_available_methods.osascript) {
        result = send_via_osascript(options);
        if (result == NOTIFY_SUCCESS) {
            os_log_debug(g_notify_log, "Sent via osascript");
            return result;
        }
    }

    // Fallback to terminal output
    if (g_available_methods.terminal) {
        result = send_via_terminal(options);
        if (result == NOTIFY_SUCCESS) {
            return result;
        }
    }

    // Last resort: log only
    return send_via_log(options);
}

NotifyResult notify_send_simple(const char* title, const char* body) {
    NotifyOptions opts = {
        .title = title,
        .body = body,
        .subtitle = NULL,
        .sound = NULL,
        .group = "convergio",
        .action_url = NULL,
        .timeout_ms = 0
    };
    return notify_send(&opts);
}

NotifyResult notify_send_with_method(NotifyMethod method, const NotifyOptions* options) {
    if (!options || !options->title || !options->body) {
        return NOTIFY_ERROR_INVALID_ARGS;
    }

    switch (method) {
        case NOTIFY_METHOD_NATIVE:
            if (g_available_methods.terminal_notifier) {
                return send_via_terminal_notifier(options);
            }
            return send_via_osascript(options);

        case NOTIFY_METHOD_OSASCRIPT:
            return send_via_osascript(options);

        case NOTIFY_METHOD_TERMINAL:
            return send_via_terminal(options);

        case NOTIFY_METHOD_SOUND:
            return send_via_sound(options);

        case NOTIFY_METHOD_LOG:
            return send_via_log(options);

        default:
            return NOTIFY_ERROR_INVALID_ARGS;
    }
}

// ============================================================================
// NOTIFICATION BACKENDS
// ============================================================================

// Escape quotes in a string for shell commands
static char* escape_quotes(const char* str, char quote_char) {
    if (!str) return strdup("");

    size_t len = strlen(str);
    size_t escaped_len = len;

    // Count quotes to escape
    for (size_t i = 0; i < len; i++) {
        if (str[i] == quote_char) escaped_len++;
    }

    char* escaped = malloc(escaped_len + 1);
    if (!escaped) return strdup("");

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (str[i] == quote_char) {
            escaped[j++] = '\\';
        }
        escaped[j++] = str[i];
    }
    escaped[j] = '\0';

    return escaped;
}

// Method 1: terminal-notifier (best UX, supports actions)
static NotifyResult send_via_terminal_notifier(const NotifyOptions* opts) {
    char cmd[4096];
    char* escaped_title = escape_quotes(opts->title, '\'');
    char* escaped_body = escape_quotes(opts->body, '\'');
    char* escaped_subtitle = opts->subtitle ? escape_quotes(opts->subtitle, '\'') : NULL;

    int written = snprintf(cmd, sizeof(cmd),
        "terminal-notifier "
        "-title '%s' "
        "-message '%s' "
        "%s%s%s "
        "-sound '%s' "
        "-sender io.convergio.cli "
        "-group '%s' "
        "%s%s%s",
        escaped_title,
        escaped_body,
        escaped_subtitle ? "-subtitle '" : "",
        escaped_subtitle ? escaped_subtitle : "",
        escaped_subtitle ? "'" : "",
        opts->sound ? opts->sound : "Glass",
        opts->group ? opts->group : "convergio",
        opts->action_url ? "-open '" : "",
        opts->action_url ? opts->action_url : "",
        opts->action_url ? "'" : "");

    free(escaped_title);
    free(escaped_body);
    free(escaped_subtitle);

    if (written >= (int)sizeof(cmd)) {
        return NOTIFY_ERROR_INVALID_ARGS;
    }

    int result = system(cmd);
    return (result == 0) ? NOTIFY_SUCCESS : NOTIFY_ERROR_UNKNOWN;
}

// Method 2: osascript (built-in, no dependencies)
static NotifyResult send_via_osascript(const NotifyOptions* opts) {
    char cmd[2048];
    char* escaped_title = escape_quotes(opts->title, '"');
    char* escaped_body = escape_quotes(opts->body, '"');
    char* escaped_subtitle = opts->subtitle ? escape_quotes(opts->subtitle, '"') : NULL;

    int written = snprintf(cmd, sizeof(cmd),
        "osascript -e 'display notification \"%s\" "
        "with title \"%s\" "
        "%s%s%s "
        "sound name \"%s\"'",
        escaped_body,
        escaped_title,
        escaped_subtitle ? "subtitle \"" : "",
        escaped_subtitle ? escaped_subtitle : "",
        escaped_subtitle ? "\"" : "",
        opts->sound ? opts->sound : "Glass");

    free(escaped_title);
    free(escaped_body);
    free(escaped_subtitle);

    if (written >= (int)sizeof(cmd)) {
        return NOTIFY_ERROR_INVALID_ARGS;
    }

    int result = system(cmd);
    return (result == 0) ? NOTIFY_SUCCESS : NOTIFY_ERROR_UNKNOWN;
}

// Method 3: Terminal output (if TTY available)
static NotifyResult send_via_terminal(const NotifyOptions* opts) {
    if (!isatty(STDERR_FILENO)) {
        return NOTIFY_ERROR_NOT_AVAILABLE;
    }

    // Use bell and colored output
    fprintf(stderr, "\a\n");  // Bell
    fprintf(stderr, "\033[1;33m╔═══════════════════════════════════════════════════╗\033[0m\n");
    fprintf(stderr, "\033[1;33m║ 🔔 %s\033[0m\n", opts->title);
    if (opts->subtitle) {
        fprintf(stderr, "\033[1;33m║    %s\033[0m\n", opts->subtitle);
    }
    fprintf(stderr, "\033[33m║ %s\033[0m\n", opts->body);
    fprintf(stderr, "\033[1;33m╚═══════════════════════════════════════════════════╝\033[0m\n");

    return NOTIFY_SUCCESS;
}

// Method 4: Sound only
static NotifyResult send_via_sound(const NotifyOptions* opts) {
    const char* sound = opts->sound ? opts->sound : "Glass";
    char cmd[256];

    snprintf(cmd, sizeof(cmd),
             "afplay /System/Library/Sounds/%s.aiff 2>/dev/null || "
             "afplay /System/Library/Sounds/Glass.aiff 2>/dev/null",
             sound);

    int result = system(cmd);
    return (result == 0) ? NOTIFY_SUCCESS : NOTIFY_ERROR_UNKNOWN;
}

// Method 5: Log file only (last resort)
static NotifyResult send_via_log(const NotifyOptions* opts) {
    FILE* log = fopen(NOTIFY_LOG_PATH, "a");
    if (!log) return NOTIFY_ERROR_UNKNOWN;

    time_t now = time(NULL);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(log, "[%s] %s", timestr, opts->title);
    if (opts->subtitle) {
        fprintf(log, " - %s", opts->subtitle);
    }
    fprintf(log, ": %s\n", opts->body);
    fclose(log);

    // Also try to print to stderr if available
    if (isatty(STDERR_FILENO)) {
        fprintf(stderr, "[CONVERGIO REMINDER] %s: %s\n", opts->title, opts->body);
    }

    return NOTIFY_SUCCESS;
}

// ============================================================================
// METHOD AVAILABILITY
// ============================================================================

bool notify_is_available(NotifyMethod method) {
    switch (method) {
        case NOTIFY_METHOD_NATIVE:
            return g_available_methods.terminal_notifier || g_available_methods.osascript;
        case NOTIFY_METHOD_OSASCRIPT:
            return g_available_methods.osascript;
        case NOTIFY_METHOD_TERMINAL:
            return g_available_methods.terminal;
        case NOTIFY_METHOD_SOUND:
            return g_available_methods.sound;
        case NOTIFY_METHOD_LOG:
            return g_available_methods.log;
        default:
            return false;
    }
}

NotifyMethod notify_get_best_method(void) {
    if (g_available_methods.terminal_notifier) return NOTIFY_METHOD_NATIVE;
    if (g_available_methods.osascript) return NOTIFY_METHOD_OSASCRIPT;
    if (g_available_methods.terminal) return NOTIFY_METHOD_TERMINAL;
    if (g_available_methods.sound) return NOTIFY_METHOD_SOUND;
    return NOTIFY_METHOD_LOG;
}

const char* notify_method_to_string(NotifyMethod method) {
    switch (method) {
        case NOTIFY_METHOD_NATIVE: return "native";
        case NOTIFY_METHOD_OSASCRIPT: return "osascript";
        case NOTIFY_METHOD_TERMINAL: return "terminal";
        case NOTIFY_METHOD_SOUND: return "sound";
        case NOTIFY_METHOD_LOG: return "log";
        default: return "unknown";
    }
}

// ============================================================================
// SCHEDULED NOTIFICATIONS
// ============================================================================

int64_t notify_schedule(int64_t task_id, time_t fire_at, NotifyMethod method) {
    sqlite3* db = g_db;
    if (!db) return -1;

    const char* sql =
        "INSERT INTO notification_queue (task_id, scheduled_at, method, status) "
        "VALUES (?, datetime(?, 'unixepoch'), ?, 'pending')";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, task_id);
    sqlite3_bind_int64(stmt, 2, fire_at);
    sqlite3_bind_text(stmt, 3, notify_method_to_string(method), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return -1;
    }

    return sqlite3_last_insert_rowid(db);
}

int64_t notify_schedule_custom(int64_t task_id, time_t fire_at,
                                const char* title, const char* body) {
    // For custom notifications, we store them with task_id=0 and the title/body
    // in a separate custom_notifications table
    // For now, just use the standard scheduling
    return notify_schedule(task_id, fire_at, NOTIFY_METHOD_NATIVE);
}

int notify_cancel(int64_t notification_id) {
    sqlite3* db = g_db;
    if (!db) return -1;

    const char* sql = "DELETE FROM notification_queue WHERE id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, notification_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int notify_snooze(int64_t notification_id, time_t new_time) {
    sqlite3* db = g_db;
    if (!db) return -1;

    const char* sql =
        "UPDATE notification_queue "
        "SET scheduled_at = datetime(?, 'unixepoch'), status = 'snoozed', retry_count = 0 "
        "WHERE id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, new_time);
    sqlite3_bind_int64(stmt, 2, notification_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int notify_snooze_for(int64_t notification_id, int seconds) {
    return notify_snooze(notification_id, time(NULL) + seconds);
}

ScheduledNotification** notify_list_pending(int* count) {
    sqlite3* db = g_db;
    if (!db) {
        *count = 0;
        return NULL;
    }

    const char* sql =
        "SELECT id, task_id, scheduled_at, method, status, retry_count, max_retries, "
        "last_error, sent_at, acknowledged_at "
        "FROM notification_queue WHERE status IN ('pending', 'snoozed') "
        "ORDER BY scheduled_at ASC";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *count = 0;
        return NULL;
    }

    // Count results first
    int total = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) total++;
    sqlite3_reset(stmt);

    if (total == 0) {
        sqlite3_finalize(stmt);
        *count = 0;
        return NULL;
    }

    ScheduledNotification** list = calloc((size_t)total, sizeof(ScheduledNotification*));
    int i = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW && i < total) {
        ScheduledNotification* n = calloc(1, sizeof(ScheduledNotification));

        n->id = sqlite3_column_int64(stmt, 0);
        n->task_id = sqlite3_column_int64(stmt, 1);

        const char* scheduled = (const char*)sqlite3_column_text(stmt, 2);
        if (scheduled) {
            struct tm tm = {0};
            strptime(scheduled, "%Y-%m-%d %H:%M:%S", &tm);
            n->scheduled_at = mktime(&tm);
        }

        const char* method_str = (const char*)sqlite3_column_text(stmt, 3);
        if (method_str) {
            if (strcmp(method_str, "native") == 0) n->method = NOTIFY_METHOD_NATIVE;
            else if (strcmp(method_str, "osascript") == 0) n->method = NOTIFY_METHOD_OSASCRIPT;
            else if (strcmp(method_str, "terminal") == 0) n->method = NOTIFY_METHOD_TERMINAL;
            else if (strcmp(method_str, "sound") == 0) n->method = NOTIFY_METHOD_SOUND;
            else n->method = NOTIFY_METHOD_LOG;
        }

        const char* status = (const char*)sqlite3_column_text(stmt, 4);
        if (status) {
            if (strcmp(status, "pending") == 0) n->status = NOTIFY_STATUS_PENDING;
            else if (strcmp(status, "sent") == 0) n->status = NOTIFY_STATUS_SENT;
            else if (strcmp(status, "failed") == 0) n->status = NOTIFY_STATUS_FAILED;
            else if (strcmp(status, "acknowledged") == 0) n->status = NOTIFY_STATUS_ACKNOWLEDGED;
            else if (strcmp(status, "snoozed") == 0) n->status = NOTIFY_STATUS_SNOOZED;
        }

        n->retry_count = sqlite3_column_int(stmt, 5);
        n->max_retries = sqlite3_column_int(stmt, 6);

        const char* error = (const char*)sqlite3_column_text(stmt, 7);
        if (error) n->last_error = strdup(error);

        list[i++] = n;
    }

    sqlite3_finalize(stmt);
    *count = i;
    return list;
}

ScheduledNotification* notify_get(int64_t id) {
    sqlite3* db = g_db;
    if (!db) return NULL;

    const char* sql =
        "SELECT id, task_id, scheduled_at, method, status, retry_count, max_retries, "
        "last_error, sent_at, acknowledged_at "
        "FROM notification_queue WHERE id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    ScheduledNotification* n = calloc(1, sizeof(ScheduledNotification));
    n->id = sqlite3_column_int64(stmt, 0);
    n->task_id = sqlite3_column_int64(stmt, 1);

    const char* scheduled = (const char*)sqlite3_column_text(stmt, 2);
    if (scheduled) {
        struct tm tm = {0};
        strptime(scheduled, "%Y-%m-%d %H:%M:%S", &tm);
        n->scheduled_at = mktime(&tm);
    }

    const char* method_str = (const char*)sqlite3_column_text(stmt, 3);
    if (method_str) {
        if (strcmp(method_str, "native") == 0) n->method = NOTIFY_METHOD_NATIVE;
        else if (strcmp(method_str, "osascript") == 0) n->method = NOTIFY_METHOD_OSASCRIPT;
        else if (strcmp(method_str, "terminal") == 0) n->method = NOTIFY_METHOD_TERMINAL;
        else if (strcmp(method_str, "sound") == 0) n->method = NOTIFY_METHOD_SOUND;
        else n->method = NOTIFY_METHOD_LOG;
    }

    const char* status = (const char*)sqlite3_column_text(stmt, 4);
    if (status) {
        if (strcmp(status, "pending") == 0) n->status = NOTIFY_STATUS_PENDING;
        else if (strcmp(status, "sent") == 0) n->status = NOTIFY_STATUS_SENT;
        else if (strcmp(status, "failed") == 0) n->status = NOTIFY_STATUS_FAILED;
        else if (strcmp(status, "acknowledged") == 0) n->status = NOTIFY_STATUS_ACKNOWLEDGED;
        else if (strcmp(status, "snoozed") == 0) n->status = NOTIFY_STATUS_SNOOZED;
    }

    n->retry_count = sqlite3_column_int(stmt, 5);
    n->max_retries = sqlite3_column_int(stmt, 6);

    const char* error = (const char*)sqlite3_column_text(stmt, 7);
    if (error) n->last_error = strdup(error);

    sqlite3_finalize(stmt);
    return n;
}

void notify_free(ScheduledNotification* notif) {
    if (!notif) return;
    free(notif->last_error);
    free(notif);
}

void notify_free_list(ScheduledNotification** list, int count) {
    if (!list) return;
    for (int i = 0; i < count; i++) {
        notify_free(list[i]);
    }
    free(list);
}

// ============================================================================
// DAEMON MANAGEMENT
// ============================================================================

// Initialize daemon with Apple Silicon optimizations
static int daemon_init(void) {
    // E-cores only queue with UTILITY QoS
    dispatch_queue_attr_t attr = dispatch_queue_attr_make_with_qos_class(
        DISPATCH_QUEUE_SERIAL,
        QOS_CLASS_UTILITY,  // E-cores preferred
        -10                 // Relative priority (lower = more power efficient)
    );
    g_daemon.queue = dispatch_queue_create("io.convergio.daemon", attr);

    // Open database connection
    g_daemon.db = g_db;
    if (!g_daemon.db) {
        os_log_error(g_notify_log, "Failed to get database connection");
        return -1;
    }

    // Prepare statements for reuse (hot path optimization)
    const char* check_sql =
        "SELECT n.id, n.task_id, t.title, t.description "
        "FROM notification_queue n "
        "LEFT JOIN tasks t ON n.task_id = t.id "
        "WHERE n.status IN ('pending', 'snoozed') "
        "AND datetime(n.scheduled_at) <= datetime('now') "
        "ORDER BY n.scheduled_at ASC "
        "LIMIT ?";

    if (sqlite3_prepare_v3(g_daemon.db, check_sql, -1, SQLITE_PREPARE_PERSISTENT,
                           &g_daemon.check_stmt, NULL) != SQLITE_OK) {
        os_log_error(g_notify_log, "Failed to prepare check statement");
        return -1;
    }

    const char* update_sql =
        "UPDATE notification_queue SET status = ?, sent_at = datetime('now'), "
        "last_error = ? WHERE id = ?";

    if (sqlite3_prepare_v3(g_daemon.db, update_sql, -1, SQLITE_PREPARE_PERSISTENT,
                           &g_daemon.update_stmt, NULL) != SQLITE_OK) {
        os_log_error(g_notify_log, "Failed to prepare update statement");
        sqlite3_finalize(g_daemon.check_stmt);
        return -1;
    }

    // Initial poll interval
    g_daemon.poll_interval_ns = POLL_INTERVAL_NORMAL_NS;
    g_daemon.started_at = time(NULL);

    return 0;
}

// Adaptive polling: slow down when idle
static void adjust_poll_interval(void) {
    int pending = atomic_load(&g_daemon.pending_count);

    uint64_t new_interval;
    if (pending == 0) {
        // No pending notifications: slow down to 5 minutes
        new_interval = POLL_INTERVAL_IDLE_NS;
    } else if (pending > 5) {
        // Many pending: speed up to 30 seconds
        new_interval = POLL_INTERVAL_FAST_NS;
    } else {
        // Normal: 60 seconds
        new_interval = POLL_INTERVAL_NORMAL_NS;
    }

    if (new_interval != g_daemon.poll_interval_ns) {
        g_daemon.poll_interval_ns = new_interval;

        // Update timer with new interval
        if (g_daemon.timer) {
            dispatch_source_set_timer(g_daemon.timer,
                dispatch_time(DISPATCH_TIME_NOW, (int64_t)g_daemon.poll_interval_ns),
                g_daemon.poll_interval_ns,
                TIMER_LEEWAY_NS);
        }

        os_log_debug(g_notify_log, "Poll interval changed to %llu seconds",
                     g_daemon.poll_interval_ns / NSEC_PER_SEC);
    }
}

// Check and send pending notifications (runs on E-cores)
static void daemon_check_pending(void) {
    // Reset and bind prepared statement
    sqlite3_reset(g_daemon.check_stmt);
    sqlite3_bind_int(g_daemon.check_stmt, 1, MAX_BATCH_SIZE);

    int batch_count = 0;

    // Fetch into static batch buffer (zero malloc in hot path)
    while (sqlite3_step(g_daemon.check_stmt) == SQLITE_ROW && batch_count < MAX_BATCH_SIZE) {
        NotificationBatch* n = &g_batch[batch_count];
        n->id = sqlite3_column_int64(g_daemon.check_stmt, 0);
        n->task_id = sqlite3_column_int64(g_daemon.check_stmt, 1);

        const char* title = (const char*)sqlite3_column_text(g_daemon.check_stmt, 2);
        const char* body = (const char*)sqlite3_column_text(g_daemon.check_stmt, 3);

        strlcpy(n->title, title ? title : "Reminder", sizeof(n->title));
        strlcpy(n->body, body ? body : "", sizeof(n->body));
        strlcpy(n->sound, "Glass", sizeof(n->sound));

        batch_count++;
    }

    // Update pending count for adaptive polling
    atomic_store(&g_daemon.pending_count, batch_count);
    adjust_poll_interval();

    if (batch_count == 0) return;

    os_log_info(g_notify_log, "Processing %d notifications", batch_count);

    // Send notifications (sequential to avoid overwhelming the system)
    for (int i = 0; i < batch_count; i++) {
        NotificationBatch* n = &g_batch[i];

        NotifyOptions opts = {
            .title = "Reminder",
            .subtitle = n->title,
            .body = n->body[0] ? n->body : n->title,
            .sound = n->sound,
            .group = "convergio-reminders",
            .action_url = NULL,
            .timeout_ms = 0
        };

        NotifyResult result = notify_send(&opts);

        // Update status in database
        sqlite3_reset(g_daemon.update_stmt);
        sqlite3_bind_text(g_daemon.update_stmt, 1,
                          result == NOTIFY_SUCCESS ? "sent" : "failed", -1, SQLITE_STATIC);
        sqlite3_bind_text(g_daemon.update_stmt, 2,
                          result == NOTIFY_SUCCESS ? NULL : "delivery failed", -1, SQLITE_STATIC);
        sqlite3_bind_int64(g_daemon.update_stmt, 3, n->id);
        sqlite3_step(g_daemon.update_stmt);

        if (result == NOTIFY_SUCCESS) {
            g_daemon.sent_count++;
            os_log_info(g_notify_log, "Sent notification %lld: %s", n->id, n->title);
        } else {
            g_daemon.failed_count++;
            os_log_error(g_notify_log, "Failed to send notification %lld", n->id);
            snprintf(g_daemon.last_error, sizeof(g_daemon.last_error),
                     "Failed to send notification %lld", n->id);
        }
    }
}

int notify_daemon_start(void) {
    if (atomic_load(&g_daemon.running)) {
        return 0;  // Already running
    }

    if (daemon_init() != 0) {
        return -1;
    }

    // Create timer source on E-core queue
    g_daemon.timer = dispatch_source_create(
        DISPATCH_SOURCE_TYPE_TIMER, 0, 0, g_daemon.queue);

    // Initial timer: immediate first check, then poll_interval
    dispatch_source_set_timer(g_daemon.timer,
        dispatch_time(DISPATCH_TIME_NOW, 0),      // Fire immediately
        g_daemon.poll_interval_ns,                // Then every interval
        TIMER_LEEWAY_NS);                         // Leeway for power optimization

    dispatch_source_set_event_handler(g_daemon.timer, ^{
        daemon_check_pending();
    });

    // Handle graceful shutdown
    dispatch_source_set_cancel_handler(g_daemon.timer, ^{
        if (g_daemon.check_stmt) sqlite3_finalize(g_daemon.check_stmt);
        if (g_daemon.update_stmt) sqlite3_finalize(g_daemon.update_stmt);
        g_daemon.check_stmt = NULL;
        g_daemon.update_stmt = NULL;
        os_log_info(g_notify_log, "Daemon stopped cleanly");
    });

    dispatch_resume(g_daemon.timer);
    atomic_store(&g_daemon.running, true);

    // Write PID file
    FILE* pidfile = fopen(DAEMON_PID_FILE, "w");
    if (pidfile) {
        fprintf(pidfile, "%d\n", getpid());
        fclose(pidfile);
    }

    os_log_info(g_notify_log, "Daemon started (E-cores, adaptive polling)");
    return 0;
}

int notify_daemon_stop(void) {
    if (!atomic_load(&g_daemon.running)) {
        return 0;  // Not running
    }

    atomic_store(&g_daemon.running, false);

    if (g_daemon.timer) {
        dispatch_source_cancel(g_daemon.timer);
        g_daemon.timer = NULL;
    }

    // Remove PID file
    unlink(DAEMON_PID_FILE);

    return 0;
}

int notify_daemon_restart(void) {
    notify_daemon_stop();
    return notify_daemon_start();
}

bool notify_daemon_is_running(void) {
    return atomic_load(&g_daemon.running);
}

pid_t notify_daemon_get_pid(void) {
    if (!atomic_load(&g_daemon.running)) {
        // Check PID file for external daemon
        FILE* pidfile = fopen(DAEMON_PID_FILE, "r");
        if (!pidfile) return 0;

        pid_t pid = 0;
        if (fscanf(pidfile, "%d", &pid) == 1) {
            fclose(pidfile);

            // Verify process exists
            if (kill(pid, 0) == 0) {
                return pid;
            }
        } else {
            fclose(pidfile);
        }

        return 0;
    }
    return getpid();
}

// Install LaunchAgent plist
int notify_daemon_install(void) {
    char plist_path[PATH_MAX];
    const char* home = getenv("HOME");
    if (!home) return -1;

    // Expand ~ in LAUNCH_AGENT_PLIST
    snprintf(plist_path, sizeof(plist_path), "%s/Library/LaunchAgents/io.convergio.daemon.plist", home);

    // Create directory if needed
    char dir_path[PATH_MAX];
    snprintf(dir_path, sizeof(dir_path), "%s/Library/LaunchAgents", home);
    mkdir(dir_path, 0755);

    // Get convergio executable path
    char exe_path[PATH_MAX];
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) != 0) {
        return -1;
    }

    // Write plist
    FILE* f = fopen(plist_path, "w");
    if (!f) return -1;

    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
        "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
        "<plist version=\"1.0\">\n"
        "<dict>\n"
        "    <key>Label</key>\n"
        "    <string>%s</string>\n"
        "    <key>ProgramArguments</key>\n"
        "    <array>\n"
        "        <string>%s</string>\n"
        "        <string>daemon</string>\n"
        "        <string>run</string>\n"
        "        <string>--foreground</string>\n"
        "    </array>\n"
        "    <key>RunAtLoad</key>\n"
        "    <true/>\n"
        "    <key>KeepAlive</key>\n"
        "    <dict>\n"
        "        <key>SuccessfulExit</key>\n"
        "        <false/>\n"
        "        <key>Crashed</key>\n"
        "        <true/>\n"
        "    </dict>\n"
        "    <key>ThrottleInterval</key>\n"
        "    <integer>10</integer>\n"
        "    <key>ProcessType</key>\n"
        "    <string>Background</string>\n"
        "    <key>LowPriorityIO</key>\n"
        "    <true/>\n"
        "    <key>Nice</key>\n"
        "    <integer>10</integer>\n"
        "    <key>StandardOutPath</key>\n"
        "    <string>/tmp/convergio-daemon.log</string>\n"
        "    <key>StandardErrorPath</key>\n"
        "    <string>/tmp/convergio-daemon.err</string>\n"
        "</dict>\n"
        "</plist>\n",
        LAUNCH_AGENT_LABEL, exe_path);

    fclose(f);

    // Load the agent using posix_spawn (safer than system())
    pid_t pid;
    char *argv[] = { "launchctl", "load", plist_path, NULL };
    int status = 0;

    if (posix_spawnp(&pid, "launchctl", NULL, NULL, argv, environ) != 0) {
        return -1;
    }

    if (waitpid(pid, &status, 0) == -1) {
        return -1;
    }

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

int notify_daemon_uninstall(void) {
    char plist_path[PATH_MAX];
    const char* home = getenv("HOME");
    if (!home) return -1;

    snprintf(plist_path, sizeof(plist_path), "%s/Library/LaunchAgents/io.convergio.daemon.plist", home);

    // Unload the agent using posix_spawn (safer than system())
    pid_t pid;
    char *argv[] = { "launchctl", "unload", plist_path, NULL };

    if (posix_spawnp(&pid, "launchctl", NULL, NULL, argv, environ) == 0) {
        int status;
        waitpid(pid, &status, 0);  // Ignore errors - file might not exist
    }

    // Remove plist
    unlink(plist_path);

    // Remove PID file
    unlink(DAEMON_PID_FILE);

    return 0;
}

// Run daemon in foreground (for launchd or debugging)
int notify_daemon_run_foreground(void) {
    // Initialize notification system
    if (notify_init() != 0) {
        fprintf(stderr, "Failed to initialize notification system\n");
        return -1;
    }

    // Check database connection
    if (!g_db) {
        fprintf(stderr, "Database not initialized\n");
        return -1;
    }

    // Start daemon
    if (notify_daemon_start() != 0) {
        fprintf(stderr, "Failed to start daemon\n");
        return -1;
    }

    printf("Convergio notification daemon running (PID %d)\n", getpid());
    printf("Press Ctrl+C to stop...\n");

    // Set up signal handlers using dispatch sources
    dispatch_source_t sig_int = dispatch_source_create(
        DISPATCH_SOURCE_TYPE_SIGNAL, SIGINT, 0, dispatch_get_main_queue());
    dispatch_source_t sig_term = dispatch_source_create(
        DISPATCH_SOURCE_TYPE_SIGNAL, SIGTERM, 0, dispatch_get_main_queue());

    signal(SIGINT, SIG_IGN);  // Ignore default handler
    signal(SIGTERM, SIG_IGN);

    dispatch_source_set_event_handler(sig_int, ^{
        printf("\nShutting down...\n");
        notify_daemon_stop();
        exit(0);
    });

    dispatch_source_set_event_handler(sig_term, ^{
        notify_daemon_stop();
        exit(0);
    });

    dispatch_resume(sig_int);
    dispatch_resume(sig_term);

    // Run forever
    dispatch_main();

    return 0;
}

// ============================================================================
// HEALTH MONITORING
// ============================================================================

NotifyHealth* notify_get_health(void) {
    NotifyHealth* health = calloc(1, sizeof(NotifyHealth));

    health->daemon_running = notify_daemon_is_running();
    health->daemon_pid = notify_daemon_get_pid();
    health->daemon_started_at = g_daemon.started_at;
    health->last_check_at = time(NULL);
    health->active_method = notify_get_best_method();

    // Get pending count from database
    sqlite3* db = g_db;
    if (db) {
        const char* sql = "SELECT COUNT(*) FROM notification_queue WHERE status = 'pending'";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                health->pending_count = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }

        // Get sent/failed counts from last 24 hours
        sql = "SELECT "
              "SUM(CASE WHEN status = 'sent' THEN 1 ELSE 0 END), "
              "SUM(CASE WHEN status = 'failed' THEN 1 ELSE 0 END) "
              "FROM notification_queue "
              "WHERE datetime(sent_at) > datetime('now', '-1 day')";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                health->sent_last_24h = sqlite3_column_int(stmt, 0);
                health->failed_last_24h = sqlite3_column_int(stmt, 1);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Copy last error
    if (g_daemon.last_error[0]) {
        health->last_error = strdup(g_daemon.last_error);
    }

    // Get memory usage
    struct task_basic_info info;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    kern_return_t kr = task_info(mach_task_self(), TASK_BASIC_INFO,
                                  (task_info_t)&info, &count);
    health->memory_usage_bytes = (kr == KERN_SUCCESS) ? info.resident_size : 0;

    return health;
}

void notify_free_health(NotifyHealth* health) {
    if (!health) return;
    free(health->last_error);
    free(health);
}

void notify_print_health(void) {
    NotifyHealth* health = notify_get_health();
    if (!health) {
        printf("Failed to get health information\n");
        return;
    }

    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║        NOTIFICATION SYSTEM HEALTH                 ║\n");
    printf("╠═══════════════════════════════════════════════════╣\n");

    printf("║ Daemon Status: %s%-23s\033[0m ║\n",
           health->daemon_running ? "\033[32m" : "\033[31m",
           health->daemon_running ? "RUNNING" : "STOPPED");

    if (health->daemon_running) {
        printf("║ Daemon PID:    %-23d ║\n", health->daemon_pid);

        time_t uptime = time(NULL) - health->daemon_started_at;
        int hours = (int)(uptime / 3600);
        int mins = (int)((uptime % 3600) / 60);
        printf("║ Uptime:        %dh %dm                         ║\n", hours, mins);
    }

    printf("║ Active Method: %-23s ║\n", notify_method_to_string(health->active_method));
    printf("║ Pending:       %-23d ║\n", health->pending_count);
    printf("║ Sent (24h):    %-23d ║\n", health->sent_last_24h);
    printf("║ Failed (24h):  %-23d ║\n", health->failed_last_24h);

    if (health->memory_usage_bytes > 0) {
        double mb = (double)health->memory_usage_bytes / (1024.0 * 1024.0);
        printf("║ Memory:        %.1f MB                        ║\n", mb);
    }

    if (health->last_error) {
        printf("║ Last Error:    %-23.23s ║\n", health->last_error);
    }

    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");

    // Method availability
    printf("Available Methods:\n");
    printf("  terminal-notifier: %s\n", g_available_methods.terminal_notifier ? "yes" : "no");
    printf("  osascript:         %s\n", g_available_methods.osascript ? "yes" : "no");
    printf("  terminal:          %s\n", g_available_methods.terminal ? "yes" : "no");
    printf("  sound:             %s\n", g_available_methods.sound ? "yes" : "no");
    printf("  log:               %s\n", g_available_methods.log ? "yes" : "no");

    notify_free_health(health);
}

// ============================================================================
// STATISTICS
// ============================================================================

NotifyStats notify_get_stats(void) {
    NotifyStats stats = {0};

    sqlite3* db = g_db;
    if (!db) return stats;

    const char* sql =
        "SELECT "
        "  (SELECT COUNT(*) FROM notification_queue WHERE status IN ('pending', 'snoozed')), "
        "  (SELECT COUNT(*) FROM notification_queue WHERE status = 'sent' AND date(sent_at) = date('now')), "
        "  (SELECT COUNT(*) FROM notification_queue WHERE status = 'sent' AND date(sent_at) > date('now', '-7 days')), "
        "  (SELECT COUNT(*) FROM notification_queue WHERE status = 'failed' AND date(sent_at) = date('now')), "
        "  (SELECT COUNT(*) FROM notification_queue WHERE status = 'snoozed')";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_pending = sqlite3_column_int(stmt, 0);
            stats.total_sent_today = sqlite3_column_int(stmt, 1);
            stats.total_sent_week = sqlite3_column_int(stmt, 2);
            stats.total_failed_today = sqlite3_column_int(stmt, 3);
            stats.total_snoozed = sqlite3_column_int(stmt, 4);
        }
        sqlite3_finalize(stmt);
    }

    return stats;
}
