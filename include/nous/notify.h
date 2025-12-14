/**
 * CONVERGIO NOTIFICATION SYSTEM
 *
 * Native macOS notifications with:
 * - Multiple backends (terminal-notifier, osascript, log)
 * - Automatic fallback chain
 * - Background daemon for scheduled reminders
 * - Health monitoring
 *
 * Part of Anna Executive Assistant feature.
 * See: ADR-009
 */

#ifndef NOUS_NOTIFY_H
#define NOUS_NOTIFY_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ENUMS
// ============================================================================

// Notification methods (in priority order for fallback)
typedef enum {
    NOTIFY_METHOD_NATIVE = 0,      // terminal-notifier (best UX)
    NOTIFY_METHOD_OSASCRIPT = 1,   // osascript (built-in)
    NOTIFY_METHOD_TERMINAL = 2,    // Print to terminal if active
    NOTIFY_METHOD_SOUND = 3,       // Sound only
    NOTIFY_METHOD_LOG = 4          // Log file only (last resort)
} NotifyMethod;

// Notification result codes
typedef enum {
    NOTIFY_SUCCESS = 0,
    NOTIFY_ERROR_NOT_AVAILABLE = -1,
    NOTIFY_ERROR_PERMISSION_DENIED = -2,
    NOTIFY_ERROR_TIMEOUT = -3,
    NOTIFY_ERROR_INVALID_ARGS = -4,
    NOTIFY_ERROR_UNKNOWN = -99
} NotifyResult;

// Scheduled notification status
typedef enum {
    NOTIFY_STATUS_PENDING = 0,
    NOTIFY_STATUS_SENT = 1,
    NOTIFY_STATUS_FAILED = 2,
    NOTIFY_STATUS_ACKNOWLEDGED = 3,
    NOTIFY_STATUS_SNOOZED = 4
} NotifyStatus;

// ============================================================================
// STRUCTURES
// ============================================================================

// Notification options for immediate send
typedef struct {
    const char* title;         // Required
    const char* body;          // Required
    const char* subtitle;      // Optional
    const char* sound;         // Sound name, NULL for default "Glass"
    const char* group;         // Notification group ID for coalescing
    const char* action_url;    // URL to open on click
    int timeout_ms;            // 0 = system default
} NotifyOptions;

// Scheduled notification record
typedef struct {
    int64_t id;
    int64_t task_id;
    time_t scheduled_at;
    NotifyMethod method;
    NotifyStatus status;
    int retry_count;
    int max_retries;
    char* last_error;
    time_t sent_at;
    time_t acknowledged_at;
} ScheduledNotification;

// Daemon health information
typedef struct {
    bool daemon_running;
    pid_t daemon_pid;
    time_t daemon_started_at;
    time_t last_check_at;
    int pending_count;
    int sent_last_24h;
    int failed_last_24h;
    NotifyMethod active_method;
    char* last_error;
    size_t memory_usage_bytes;
} NotifyHealth;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize the notification subsystem.
 * Detects available notification methods.
 * @return 0 on success, -1 on error
 */
int notify_init(void);

/**
 * Shutdown the notification subsystem.
 * Does NOT stop the daemon (use notify_daemon_stop for that).
 */
void notify_shutdown(void);

// ============================================================================
// IMMEDIATE NOTIFICATIONS
// ============================================================================

/**
 * Send a notification immediately with full options.
 * Automatically falls back through available methods.
 * @param options Notification options
 * @return NOTIFY_SUCCESS or error code
 */
NotifyResult notify_send(const NotifyOptions* options);

/**
 * Send a simple notification (title + body only).
 * @param title Notification title
 * @param body Notification body
 * @return NOTIFY_SUCCESS or error code
 */
NotifyResult notify_send_simple(const char* title, const char* body);

/**
 * Send notification using a specific method (no fallback).
 * @param method Specific method to use
 * @param options Notification options
 * @return NOTIFY_SUCCESS or error code
 */
NotifyResult notify_send_with_method(NotifyMethod method, const NotifyOptions* options);

// ============================================================================
// METHOD AVAILABILITY
// ============================================================================

/**
 * Check if a notification method is available.
 * @param method Method to check
 * @return true if available
 */
bool notify_is_available(NotifyMethod method);

/**
 * Get the best available notification method.
 * @return Best available method
 */
NotifyMethod notify_get_best_method(void);

/**
 * Get method name as string.
 * @param method Method enum
 * @return Method name
 */
const char* notify_method_to_string(NotifyMethod method);

// ============================================================================
// SCHEDULED NOTIFICATIONS
// ============================================================================

/**
 * Schedule a notification for a task.
 * Stores in database for daemon to process.
 * @param task_id Associated task ID
 * @param fire_at When to send the notification
 * @param method Preferred method (will fallback if unavailable)
 * @return Notification ID, -1 on error
 */
int64_t notify_schedule(int64_t task_id, time_t fire_at, NotifyMethod method);

/**
 * Schedule a notification with custom title/body.
 * @param task_id Associated task ID (0 for standalone)
 * @param fire_at When to send
 * @param title Notification title
 * @param body Notification body
 * @return Notification ID, -1 on error
 */
int64_t notify_schedule_custom(int64_t task_id, time_t fire_at,
                                const char* title, const char* body);

/**
 * Cancel a scheduled notification.
 * @param notification_id Notification ID
 * @return 0 on success, -1 on error
 */
int notify_cancel(int64_t notification_id);

/**
 * Snooze a notification to a new time.
 * @param notification_id Notification ID
 * @param new_time New fire time
 * @return 0 on success, -1 on error
 */
int notify_snooze(int64_t notification_id, time_t new_time);

/**
 * Snooze a notification for a duration.
 * @param notification_id Notification ID
 * @param seconds Seconds to snooze
 * @return 0 on success, -1 on error
 */
int notify_snooze_for(int64_t notification_id, int seconds);

/**
 * List pending scheduled notifications.
 * @param count Output: number of notifications
 * @return Array of notifications (caller must free with notify_free_list)
 */
ScheduledNotification** notify_list_pending(int* count);

/**
 * Get a scheduled notification by ID.
 * @param id Notification ID
 * @return Notification or NULL if not found
 */
ScheduledNotification* notify_get(int64_t id);

/**
 * Free a notification.
 */
void notify_free(ScheduledNotification* notif);

/**
 * Free an array of notifications.
 */
void notify_free_list(ScheduledNotification** list, int count);

// ============================================================================
// DAEMON MANAGEMENT
// ============================================================================

/**
 * Start the notification daemon.
 * Installs LaunchAgent and starts the daemon.
 * @return 0 on success, -1 on error
 */
int notify_daemon_start(void);

/**
 * Stop the notification daemon.
 * @return 0 on success, -1 on error
 */
int notify_daemon_stop(void);

/**
 * Restart the notification daemon.
 * @return 0 on success, -1 on error
 */
int notify_daemon_restart(void);

/**
 * Check if daemon is running.
 * @return true if running
 */
bool notify_daemon_is_running(void);

/**
 * Get daemon PID.
 * @return PID or 0 if not running
 */
pid_t notify_daemon_get_pid(void);

/**
 * Install the LaunchAgent plist.
 * Called automatically by daemon_start if needed.
 * @return 0 on success, -1 on error
 */
int notify_daemon_install(void);

/**
 * Uninstall the LaunchAgent plist.
 * @return 0 on success, -1 on error
 */
int notify_daemon_uninstall(void);

/**
 * Run daemon in foreground (for debugging or launchd).
 * This function blocks until shutdown signal.
 * @return 0 on clean exit, -1 on error
 */
int notify_daemon_run_foreground(void);

// ============================================================================
// HEALTH MONITORING
// ============================================================================

/**
 * Get daemon health information.
 * @return Health struct (caller must free with notify_free_health)
 */
NotifyHealth* notify_get_health(void);

/**
 * Free health struct.
 */
void notify_free_health(NotifyHealth* health);

/**
 * Print health information to stdout.
 */
void notify_print_health(void);

// ============================================================================
// STATISTICS
// ============================================================================

typedef struct {
    int total_pending;
    int total_sent_today;
    int total_sent_week;
    int total_failed_today;
    int total_snoozed;
} NotifyStats;

/**
 * Get notification statistics.
 */
NotifyStats notify_get_stats(void);

#ifdef __cplusplus
}
#endif

#endif // NOUS_NOTIFY_H
