//
//  ADHDModeManager.swift
//  ConvergioApp
//
//  Created on 2025-12-24.
//  ADHD-focused features: session timers, breaks, distraction-free mode
//

import Foundation
import SwiftUI
import Combine
import UserNotifications

/// ADHD session state
enum ADHDSessionState: String, Codable {
    case idle
    case working
    case breakTime
    case completed
}

/// ADHD session configuration
struct ADHDSessionConfig: Codable {
    var workDuration: TimeInterval = 15 * 60  // 15 minutes default
    var breakDuration: TimeInterval = 5 * 60  // 5 minutes default
    var longBreakDuration: TimeInterval = 15 * 60  // 15 minutes
    var sessionsUntilLongBreak: Int = 4  // Pomodoro-style

    var enableSoundAlerts: Bool = true
    var enableNotifications: Bool = true
    var enableGamification: Bool = true
    var xpPerSession: Int = 50
}

/// ADHD session statistics
struct ADHDSessionStats: Codable {
    var totalSessions: Int = 0
    var completedSessions: Int = 0
    var totalWorkTime: TimeInterval = 0
    var totalBreakTime: TimeInterval = 0
    var currentStreak: Int = 0
    var longestStreak: Int = 0
    var totalXPEarned: Int = 0
    var lastSessionDate: Date?
}

/// ADHD Mode Manager - Handles session timers, breaks, and gamification
@MainActor
class ADHDModeManager: ObservableObject {
    static let shared = ADHDModeManager()

    @Published var isEnabled: Bool = false
    @Published var currentState: ADHDSessionState = .idle
    @Published var config: ADHDSessionConfig
    @Published var stats: ADHDSessionStats

    @Published var timeRemaining: TimeInterval = 0
    @Published var sessionProgress: Double = 0  // 0.0 to 1.0
    @Published var distractionFreeMode: Bool = false

    private var timer: Timer?
    private var sessionStartTime: Date?
    private var cancellables = Set<AnyCancellable>()

    private let configKey = "com.convergio.adhd.config"
    private let statsKey = "com.convergio.adhd.stats"

    private init() {
        self.config = ADHDModeManager.loadConfig()
        self.stats = ADHDModeManager.loadStats()
        self.timeRemaining = config.workDuration

        requestNotificationPermissions()
    }

    // MARK: - Persistence

    private static func loadConfig() -> ADHDSessionConfig {
        guard let data = UserDefaults.standard.data(forKey: "com.convergio.adhd.config"),
              let config = try? JSONDecoder().decode(ADHDSessionConfig.self, from: data) else {
            return ADHDSessionConfig()
        }
        return config
    }

    private func saveConfig() {
        if let data = try? JSONEncoder().encode(config) {
            UserDefaults.standard.set(data, forKey: configKey)
        }
    }

    private static func loadStats() -> ADHDSessionStats {
        guard let data = UserDefaults.standard.data(forKey: "com.convergio.adhd.stats"),
              let stats = try? JSONDecoder().decode(ADHDSessionStats.self, from: data) else {
            return ADHDSessionStats()
        }
        return stats
    }

    private func saveStats() {
        if let data = try? JSONEncoder().encode(stats) {
            UserDefaults.standard.set(data, forKey: statsKey)
        }
    }

    // MARK: - Session Control

    /// Start a work session
    func startSession() {
        guard currentState == .idle || currentState == .completed else {
            Logger.shared.warning("Cannot start session: already in progress")
            return
        }

        currentState = .working
        timeRemaining = config.workDuration
        sessionStartTime = Date()
        sessionProgress = 0

        stats.totalSessions += 1
        saveStats()

        startTimer()
        sendNotification(
            title: "Focus Session Started",
            body: "Work for \(Int(config.workDuration / 60)) minutes. You've got this!"
        )

        if distractionFreeMode {
            enableDistractionFreeMode()
        }

        Logger.shared.info("ADHD session started: \(config.workDuration)s")
    }

    /// Start a break
    func startBreak(isLongBreak: Bool = false) {
        currentState = .breakTime

        let duration = isLongBreak ? config.longBreakDuration : config.breakDuration
        timeRemaining = duration
        sessionStartTime = Date()
        sessionProgress = 0

        startTimer()
        sendNotification(
            title: isLongBreak ? "Long Break Time!" : "Break Time!",
            body: "Relax for \(Int(duration / 60)) minutes. Stretch, hydrate, breathe."
        )

        if distractionFreeMode {
            disableDistractionFreeMode()
        }

        Logger.shared.info("ADHD break started: \(duration)s (long: \(isLongBreak))")
    }

    /// Pause current session
    func pauseSession() {
        guard currentState == .working || currentState == .breakTime else { return }

        timer?.invalidate()
        timer = nil

        Logger.shared.info("ADHD session paused")
    }

    /// Resume paused session
    func resumeSession() {
        guard currentState == .working || currentState == .breakTime else { return }

        startTimer()
        Logger.shared.info("ADHD session resumed")
    }

    /// Stop and reset session
    func stopSession() {
        timer?.invalidate()
        timer = nil

        currentState = .idle
        timeRemaining = config.workDuration
        sessionProgress = 0
        sessionStartTime = nil

        if distractionFreeMode {
            disableDistractionFreeMode()
        }

        Logger.shared.info("ADHD session stopped")
    }

    /// Complete current session
    func completeSession() {
        guard currentState == .working else { return }

        timer?.invalidate()
        timer = nil

        stats.completedSessions += 1
        stats.totalWorkTime += config.workDuration

        // Update streak
        updateStreak()

        // Award XP
        if config.enableGamification {
            awardXP(config.xpPerSession)
        }

        // Check if it's time for a long break
        let shouldTakeLongBreak = (stats.completedSessions % config.sessionsUntilLongBreak) == 0

        currentState = .completed
        saveStats()

        sendNotification(
            title: "Session Completed!",
            body: "Great work! You earned \(config.xpPerSession) XP. Ready for a break?"
        )

        Logger.shared.info("ADHD session completed. Total: \(stats.completedSessions)")

        // Auto-start break if enabled
        Task {
            try? await Task.sleep(for: .seconds(2))
            await startBreak(isLongBreak: shouldTakeLongBreak)
        }
    }

    // MARK: - Timer Management

    private func startTimer() {
        timer?.invalidate()

        timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            Task { @MainActor [weak self] in
                guard let self = self else { return }

                self.timeRemaining -= 1

                // Update progress
                let totalDuration = self.currentState == .working ?
                    self.config.workDuration :
                    (self.stats.completedSessions % self.config.sessionsUntilLongBreak == 0 ?
                        self.config.longBreakDuration : self.config.breakDuration)

                self.sessionProgress = 1.0 - (self.timeRemaining / totalDuration)

                // Check if session completed
                if self.timeRemaining <= 0 {
                    if self.currentState == .working {
                        self.completeSession()
                    } else if self.currentState == .breakTime {
                        self.endBreak()
                    }
                }

                // 1-minute warning
                if self.timeRemaining == 60 {
                    self.sendNotification(
                        title: "1 Minute Remaining",
                        body: self.currentState == .working ?
                            "Almost done! Finish strong!" :
                            "Break almost over. Get ready to focus!"
                    )
                }
            }
        }
    }

    private func endBreak() {
        timer?.invalidate()
        timer = nil

        stats.totalBreakTime += currentState == .breakTime ?
            config.breakDuration : config.longBreakDuration

        currentState = .idle
        timeRemaining = config.workDuration
        sessionProgress = 0

        saveStats()

        sendNotification(
            title: "Break Over",
            body: "Ready to start another focused session?"
        )

        Logger.shared.info("ADHD break ended")
    }

    // MARK: - Distraction-Free Mode

    private func enableDistractionFreeMode() {
        // Hide unnecessary UI elements
        NotificationCenter.default.post(
            name: NSNotification.Name("ADHDDistractionFreeModeEnabled"),
            object: nil
        )
        Logger.shared.info("Distraction-free mode enabled")
    }

    private func disableDistractionFreeMode() {
        NotificationCenter.default.post(
            name: NSNotification.Name("ADHDDistractionFreeModeDisabled"),
            object: nil
        )
        Logger.shared.info("Distraction-free mode disabled")
    }

    // MARK: - Gamification

    private func awardXP(_ amount: Int) {
        stats.totalXPEarned += amount

        NotificationCenter.default.post(
            name: NSNotification.Name("ADHDXPAwarded"),
            object: amount
        )

        Logger.shared.info("Awarded \(amount) XP. Total: \(stats.totalXPEarned)")
    }

    private func updateStreak() {
        let calendar = Calendar.current
        let today = calendar.startOfDay(for: Date())

        if let lastSession = stats.lastSessionDate {
            let lastSessionDay = calendar.startOfDay(for: lastSession)
            let daysSinceLastSession = calendar.dateComponents([.day], from: lastSessionDay, to: today).day ?? 0

            if daysSinceLastSession == 0 {
                // Same day, streak continues
            } else if daysSinceLastSession == 1 {
                // Consecutive day
                stats.currentStreak += 1
                stats.longestStreak = max(stats.longestStreak, stats.currentStreak)
            } else {
                // Streak broken
                stats.currentStreak = 1
            }
        } else {
            stats.currentStreak = 1
        }

        stats.lastSessionDate = Date()
        saveStats()
    }

    // MARK: - Notifications

    private func requestNotificationPermissions() {
        UNUserNotificationCenter.current().requestAuthorization(options: [.alert, .sound, .badge]) { granted, error in
            if granted {
                Logger.shared.info("ADHD notification permissions granted")
            } else if let error = error {
                Logger.shared.error("ADHD notification permission error: \(error.localizedDescription)")
            }
        }
    }

    private func sendNotification(title: String, body: String) {
        guard config.enableNotifications else { return }

        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.sound = config.enableSoundAlerts ? .default : nil

        let request = UNNotificationRequest(
            identifier: UUID().uuidString,
            content: content,
            trigger: nil  // Immediate delivery
        )

        UNUserNotificationCenter.current().add(request) { error in
            if let error = error {
                Logger.shared.error("Failed to send ADHD notification: \(error.localizedDescription)")
            }
        }
    }

    // MARK: - Statistics

    /// Get completion rate
    func getCompletionRate() -> Double {
        guard stats.totalSessions > 0 else { return 0 }
        return Double(stats.completedSessions) / Double(stats.totalSessions)
    }

    /// Get formatted time remaining
    func getFormattedTimeRemaining() -> String {
        let minutes = Int(timeRemaining) / 60
        let seconds = Int(timeRemaining) % 60
        return String(format: "%02d:%02d", minutes, seconds)
    }

    /// Get total work hours
    func getTotalWorkHours() -> Double {
        return stats.totalWorkTime / 3600
    }

    /// Reset statistics
    func resetStats() {
        stats = ADHDSessionStats()
        saveStats()
        Logger.shared.info("ADHD statistics reset")
    }

    // MARK: - Configuration

    /// Update work duration
    func setWorkDuration(minutes: Int) {
        config.workDuration = TimeInterval(minutes * 60)
        saveConfig()
        if currentState == .idle {
            timeRemaining = config.workDuration
        }
    }

    /// Update break duration
    func setBreakDuration(minutes: Int) {
        config.breakDuration = TimeInterval(minutes * 60)
        saveConfig()
    }

    /// Toggle distraction-free mode
    func toggleDistractionFreeMode() {
        distractionFreeMode.toggle()
        if currentState == .working {
            if distractionFreeMode {
                enableDistractionFreeMode()
            } else {
                disableDistractionFreeMode()
            }
        }
    }
}

// MARK: - SwiftUI Environment Key

private struct ADHDModeManagerKey: EnvironmentKey {
    @MainActor static let defaultValue = ADHDModeManager.shared
}

extension EnvironmentValues {
    var adhdModeManager: ADHDModeManager {
        get { self[ADHDModeManagerKey.self] }
        set { self[ADHDModeManagerKey.self] = newValue }
    }
}
