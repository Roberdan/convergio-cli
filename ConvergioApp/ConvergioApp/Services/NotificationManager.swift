/**
 * CONVERGIO NATIVE - Notification Manager
 *
 * Handles native macOS notifications for:
 * - Task completion
 * - Agent activity updates
 * - Budget warnings
 * - Error alerts
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import UserNotifications
import AppKit

/// Manages native macOS notifications
final class NotificationManager: NSObject, ObservableObject {
    static let shared = NotificationManager()

    /// Whether notifications are enabled
    @Published var isEnabled: Bool = true

    /// Authorization status
    @Published private(set) var authorizationStatus: UNAuthorizationStatus = .notDetermined

    private let center = UNUserNotificationCenter.current()

    private override init() {
        super.init()
        center.delegate = self
        checkAuthorization()
    }

    // MARK: - Authorization

    /// Request notification permissions
    func requestAuthorization() async -> Bool {
        do {
            let granted = try await center.requestAuthorization(options: [.alert, .sound, .badge])
            await MainActor.run {
                self.authorizationStatus = granted ? .authorized : .denied
            }
            return granted
        } catch {
            print("Notification authorization error: \(error)")
            return false
        }
    }

    /// Check current authorization status
    func checkAuthorization() {
        center.getNotificationSettings { settings in
            DispatchQueue.main.async {
                self.authorizationStatus = settings.authorizationStatus
            }
        }
    }

    // MARK: - Send Notifications

    /// Send a task completion notification
    func sendTaskCompletion(title: String, body: String, agentName: String? = nil) {
        guard isEnabled && authorizationStatus == .authorized else { return }

        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.sound = .default
        content.categoryIdentifier = "TASK_COMPLETE"

        if let agentName = agentName {
            content.userInfo["agentName"] = agentName
        }

        scheduleNotification(content: content, identifier: "task-\(UUID().uuidString)")
    }

    /// Send a budget warning notification
    func sendBudgetWarning(remaining: Double, isExceeded: Bool) {
        guard isEnabled && authorizationStatus == .authorized else { return }

        let content = UNMutableNotificationContent()

        if isExceeded {
            content.title = "Budget Exceeded"
            content.body = "Your session budget has been exceeded. Processing is paused."
            content.sound = UNNotificationSound.defaultCritical
        } else {
            content.title = "Budget Warning"
            content.body = String(format: "Only $%.2f remaining in your budget.", remaining)
            content.sound = .default
        }

        content.categoryIdentifier = "BUDGET_WARNING"
        content.userInfo["budgetRemaining"] = remaining
        content.userInfo["isExceeded"] = isExceeded

        scheduleNotification(content: content, identifier: "budget-warning")
    }

    /// Send an agent activity notification
    func sendAgentActivity(agentName: String, activity: String) {
        guard isEnabled && authorizationStatus == .authorized else { return }

        let content = UNMutableNotificationContent()
        content.title = agentName
        content.body = activity
        content.sound = nil // Silent for activity updates
        content.categoryIdentifier = "AGENT_ACTIVITY"
        content.userInfo["agentName"] = agentName

        scheduleNotification(content: content, identifier: "agent-\(agentName)-\(Date().timeIntervalSince1970)")
    }

    /// Send an error notification
    func sendError(title: String, message: String) {
        guard isEnabled && authorizationStatus == .authorized else { return }

        let content = UNMutableNotificationContent()
        content.title = title
        content.body = message
        content.sound = UNNotificationSound.defaultCritical
        content.categoryIdentifier = "ERROR"

        scheduleNotification(content: content, identifier: "error-\(UUID().uuidString)")
    }

    /// Send a convergence complete notification
    func sendConvergenceComplete(agentCount: Int, responsePreview: String) {
        guard isEnabled && authorizationStatus == .authorized else { return }

        let content = UNMutableNotificationContent()
        content.title = "Response Ready"
        content.subtitle = "\(agentCount) agents converged"
        content.body = String(responsePreview.prefix(100)) + (responsePreview.count > 100 ? "..." : "")
        content.sound = .default
        content.categoryIdentifier = "CONVERGENCE"

        scheduleNotification(content: content, identifier: "convergence-\(UUID().uuidString)")
    }

    // MARK: - Private

    private func scheduleNotification(content: UNMutableNotificationContent, identifier: String) {
        let trigger = UNTimeIntervalNotificationTrigger(timeInterval: 0.1, repeats: false)
        let request = UNNotificationRequest(identifier: identifier, content: content, trigger: trigger)

        center.add(request) { error in
            if let error = error {
                print("Failed to schedule notification: \(error)")
            }
        }
    }

    /// Register notification categories and actions
    func registerCategories() {
        // Task complete category with actions
        let viewAction = UNNotificationAction(
            identifier: "VIEW_ACTION",
            title: "View",
            options: [.foreground]
        )

        let dismissAction = UNNotificationAction(
            identifier: "DISMISS_ACTION",
            title: "Dismiss",
            options: []
        )

        let taskCategory = UNNotificationCategory(
            identifier: "TASK_COMPLETE",
            actions: [viewAction, dismissAction],
            intentIdentifiers: [],
            options: []
        )

        // Budget warning category
        let adjustAction = UNNotificationAction(
            identifier: "ADJUST_BUDGET",
            title: "Adjust Budget",
            options: [.foreground]
        )

        let budgetCategory = UNNotificationCategory(
            identifier: "BUDGET_WARNING",
            actions: [adjustAction, dismissAction],
            intentIdentifiers: [],
            options: []
        )

        // Convergence category
        let copyAction = UNNotificationAction(
            identifier: "COPY_ACTION",
            title: "Copy Response",
            options: []
        )

        let convergenceCategory = UNNotificationCategory(
            identifier: "CONVERGENCE",
            actions: [viewAction, copyAction],
            intentIdentifiers: [],
            options: []
        )

        center.setNotificationCategories([taskCategory, budgetCategory, convergenceCategory])
    }
}

// MARK: - UNUserNotificationCenterDelegate

extension NotificationManager: UNUserNotificationCenterDelegate {
    func userNotificationCenter(
        _ center: UNUserNotificationCenter,
        willPresent notification: UNNotification
    ) async -> UNNotificationPresentationOptions {
        // Show notifications even when app is in foreground
        return [.banner, .sound]
    }

    func userNotificationCenter(
        _ center: UNUserNotificationCenter,
        didReceive response: UNNotificationResponse
    ) async {
        let userInfo = response.notification.request.content.userInfo

        switch response.actionIdentifier {
        case "VIEW_ACTION":
            // Bring app to front
            NSApp.activate(ignoringOtherApps: true)

        case "ADJUST_BUDGET":
            // Post notification to show budget adjustment
            NotificationCenter.default.post(
                name: .showBudgetAdjustment,
                object: nil
            )

        case "COPY_ACTION":
            // Copy response to clipboard
            if let content = userInfo["responseContent"] as? String {
                NSPasteboard.general.clearContents()
                NSPasteboard.general.setString(content, forType: .string)
            }

        default:
            break
        }
    }
}

// MARK: - Notification Names

extension Notification.Name {
    static let showBudgetAdjustment = Notification.Name("com.convergio.showBudgetAdjustment")
}
