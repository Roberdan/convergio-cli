/**
 * CONVERGIO NATIVE - Progress Manager
 *
 * Service for managing student progress, XP, streaks, achievements, and gamification.
 * Handles persistence and provides business logic for the progress system.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import SwiftUI
import Combine

// MARK: - Progress Manager

@MainActor
class ProgressManager: ObservableObject {
    // MARK: - Published Properties

    @Published var xpSystem: XPSystem
    @Published var streak: Streak
    @Published var studyStats: StudyStats
    @Published var subjectMasteries: [SubjectMastery]
    @Published var achievements: [Achievement]

    // MARK: - Notifications

    /// Published when level up occurs
    @Published var levelUpNotification: (level: Int, xpGained: Int)?

    /// Published when achievement is unlocked
    @Published var achievementUnlocked: Achievement?

    // MARK: - Persistence

    private let userDefaults = UserDefaults.standard
    private let xpSystemKey = "com.convergio.xpSystem"
    private let streakKey = "com.convergio.streak"
    private let studyStatsKey = "com.convergio.studyStats"
    private let subjectMasteriesKey = "com.convergio.subjectMasteries"
    private let achievementsKey = "com.convergio.achievements"

    // MARK: - Initialization

    init() {
        // Load from persistence or use defaults
        self.xpSystem = ProgressManager.loadXPSystem()
        self.streak = ProgressManager.loadStreak()
        self.studyStats = ProgressManager.loadStudyStats()
        self.subjectMasteries = ProgressManager.loadSubjectMasteries()
        self.achievements = ProgressManager.loadAchievements()
    }

    // MARK: - Study Session Management

    /// Record a study session
    func recordStudySession(
        subject: Subject,
        duration: Int, // in minutes
        topicsCompleted: Int = 0
    ) {
        // Update streak
        streak.recordStudySession()
        saveStreak()

        // Update stats
        studyStats.sessionsThisWeek += 1
        studyStats.addStudyTime(minutes: duration)
        studyStats.recordSubject(subject.rawValue)

        if StudyStats.isNightOwlTime {
            studyStats.recordNightOwlSession()
        }
        saveStudyStats()

        // Update subject mastery
        updateSubjectMastery(subject: subject, topicsCompleted: topicsCompleted)

        // Award XP
        let baseXP = calculateSessionXP(duration: duration)
        addXP(baseXP, source: "Study Session")

        // Check for new achievements
        checkAchievements()
    }

    /// Calculate XP for a study session
    private func calculateSessionXP(duration: Int) -> Int {
        // Base: 10 XP per 15 minutes
        let baseXP = (duration / 15) * 10

        // Bonus for longer sessions (up to 1 hour)
        let bonusMultiplier: Double = duration >= 60 ? 1.5 : 1.0

        return Int(Double(baseXP) * bonusMultiplier)
    }

    // MARK: - XP Management

    /// Add XP and check for level up
    func addXP(_ amount: Int, source: String) {
        let leveledUp = xpSystem.addXP(amount)
        saveXPSystem()

        if leveledUp {
            // Trigger level up notification
            levelUpNotification = (level: xpSystem.level, xpGained: amount)
            Logger.shared.info("Level up! Now level \(xpSystem.level)")

            // Clear notification after 5 seconds
            Task {
                try? await Task.sleep(nanoseconds: 5_000_000_000)
                self.levelUpNotification = nil
            }
        }

        Logger.shared.debug("Added \(amount) XP from \(source). Total: \(xpSystem.totalXP)")
    }

    // MARK: - Subject Mastery Management

    /// Update mastery for a subject
    func updateSubjectMastery(subject: Subject, topicsCompleted: Int) {
        if let index = subjectMasteries.firstIndex(where: { $0.subject == subject }) {
            // Update existing mastery
            subjectMasteries[index].topicsCompleted += topicsCompleted
            subjectMasteries[index].updateMastery()
            subjectMasteries[index].recordStudy()
        } else {
            // Create new mastery
            var mastery = SubjectMastery(
                subject: subject,
                topicsCompleted: topicsCompleted,
                totalTopics: 20 // Default
            )
            mastery.updateMastery()
            mastery.recordStudy()
            subjectMasteries.append(mastery)
        }

        saveSubjectMasteries()
    }

    /// Get mastery for a specific subject
    func getMastery(for subject: Subject) -> SubjectMastery? {
        subjectMasteries.first(where: { $0.subject == subject })
    }

    // MARK: - Question Tracking

    /// Record a question asked
    func recordQuestion() {
        studyStats.recordQuestion()
        saveStudyStats()

        // Check for question-related achievements
        checkAchievements()
    }

    // MARK: - Achievement Management

    /// Check all achievements and unlock if requirements are met
    func checkAchievements() {
        for index in achievements.indices {
            guard !achievements[index].isUnlocked else { continue }

            let requirement = achievements[index].requirement
            var currentValue = 0

            // Get current value based on requirement type
            switch requirement.type {
            case .studySessions:
                currentValue = studyStats.sessionsThisWeek
            case .questionsAsked:
                currentValue = studyStats.questionsAsked
            case .streakDays:
                currentValue = streak.currentStreak
            case .subjectMastery:
                currentValue = subjectMasteries.map { $0.masteryPercentage }.max() ?? 0
            case .differentSubjects:
                currentValue = studyStats.subjectsStudied.count
            case .nightOwlSessions:
                currentValue = studyStats.nightOwlSessions
            case .totalXP:
                currentValue = xpSystem.totalXP
            case .studyMinutes:
                currentValue = studyStats.totalStudyMinutes
            }

            // Check if requirement is met
            if achievements[index].requirementMet(current: currentValue) {
                unlockAchievement(at: index)
            }
        }
    }

    /// Unlock an achievement
    private func unlockAchievement(at index: Int) {
        guard !achievements[index].isUnlocked else { return }

        achievements[index].unlock()
        let achievement = achievements[index]

        // Award XP
        addXP(achievement.xpReward, source: "Achievement: \(achievement.title)")

        // Trigger notification
        achievementUnlocked = achievement
        Logger.shared.info("Achievement unlocked: \(achievement.title)")

        // Save
        saveAchievements()

        // Clear notification after 5 seconds
        Task {
            try? await Task.sleep(nanoseconds: 5_000_000_000)
            self.achievementUnlocked = nil
        }
    }

    /// Get progress for an achievement
    func getAchievementProgress(_ achievement: Achievement) -> (current: Int, target: Int, progress: Double) {
        let requirement = achievement.requirement
        var currentValue = 0

        switch requirement.type {
        case .studySessions:
            currentValue = studyStats.sessionsThisWeek
        case .questionsAsked:
            currentValue = studyStats.questionsAsked
        case .streakDays:
            currentValue = streak.currentStreak
        case .subjectMastery:
            currentValue = subjectMasteries.map { $0.masteryPercentage }.max() ?? 0
        case .differentSubjects:
            currentValue = studyStats.subjectsStudied.count
        case .nightOwlSessions:
            currentValue = studyStats.nightOwlSessions
        case .totalXP:
            currentValue = xpSystem.totalXP
        case .studyMinutes:
            currentValue = studyStats.totalStudyMinutes
        }

        return (
            current: currentValue,
            target: requirement.target,
            progress: achievement.progress(current: currentValue)
        )
    }

    // MARK: - Statistics

    /// Get recent achievements (last 5 unlocked)
    func getRecentAchievements() -> [Achievement] {
        achievements
            .filter { $0.isUnlocked }
            .sorted { ($0.unlockedDate ?? .distantPast) > ($1.unlockedDate ?? .distantPast) }
            .prefix(5)
            .map { $0 }
    }

    /// Get unlocked achievements count
    var unlockedAchievementsCount: Int {
        achievements.filter { $0.isUnlocked }.count
    }

    /// Get total achievements count
    var totalAchievementsCount: Int {
        achievements.count
    }

    /// Get next goal (next closest achievement)
    func getNextGoal() -> Achievement? {
        achievements
            .filter { !$0.isUnlocked }
            .sorted { achievement1, achievement2 in
                let progress1 = getAchievementProgress(achievement1).progress
                let progress2 = getAchievementProgress(achievement2).progress
                return progress1 > progress2
            }
            .first
    }

    // MARK: - Persistence Methods

    private static func loadXPSystem() -> XPSystem {
        if let data = UserDefaults.standard.data(forKey: "com.convergio.xpSystem"),
           let decoded = try? JSONDecoder().decode(XPSystem.self, from: data) {
            return decoded
        }
        return XPSystem()
    }

    private func saveXPSystem() {
        if let encoded = try? JSONEncoder().encode(xpSystem) {
            userDefaults.set(encoded, forKey: xpSystemKey)
        }
    }

    private static func loadStreak() -> Streak {
        if let data = UserDefaults.standard.data(forKey: "com.convergio.streak"),
           let decoded = try? JSONDecoder().decode(Streak.self, from: data) {
            return decoded
        }
        return Streak()
    }

    private func saveStreak() {
        if let encoded = try? JSONEncoder().encode(streak) {
            userDefaults.set(encoded, forKey: streakKey)
        }
    }

    private static func loadStudyStats() -> StudyStats {
        if let data = UserDefaults.standard.data(forKey: "com.convergio.studyStats"),
           let decoded = try? JSONDecoder().decode(StudyStats.self, from: data) {
            return decoded
        }
        return StudyStats()
    }

    private func saveStudyStats() {
        if let encoded = try? JSONEncoder().encode(studyStats) {
            userDefaults.set(encoded, forKey: studyStatsKey)
        }
    }

    private static func loadSubjectMasteries() -> [SubjectMastery] {
        if let data = UserDefaults.standard.data(forKey: "com.convergio.subjectMasteries"),
           let decoded = try? JSONDecoder().decode([SubjectMastery].self, from: data) {
            return decoded
        }
        return []
    }

    private func saveSubjectMasteries() {
        if let encoded = try? JSONEncoder().encode(subjectMasteries) {
            userDefaults.set(encoded, forKey: subjectMasteriesKey)
        }
    }

    private static func loadAchievements() -> [Achievement] {
        if let data = UserDefaults.standard.data(forKey: "com.convergio.achievements"),
           let decoded = try? JSONDecoder().decode([Achievement].self, from: data) {
            // Merge with all achievements to include any new ones
            let loadedDict = Dictionary(uniqueKeysWithValues: decoded.map { ($0.title, $0) })
            return Achievement.allAchievements.map { achievement in
                loadedDict[achievement.title] ?? achievement
            }
        }
        return Achievement.allAchievements
    }

    private func saveAchievements() {
        if let encoded = try? JSONEncoder().encode(achievements) {
            userDefaults.set(encoded, forKey: achievementsKey)
        }
    }

    // MARK: - Reset (for testing)

    func resetProgress() {
        xpSystem = XPSystem()
        streak = Streak()
        studyStats = StudyStats()
        subjectMasteries = []
        achievements = Achievement.allAchievements

        saveXPSystem()
        saveStreak()
        saveStudyStats()
        saveSubjectMasteries()
        saveAchievements()

        Logger.shared.info("Progress reset to defaults")
    }
}

// MARK: - Preview

extension ProgressManager {
    static let preview: ProgressManager = {
        let manager = ProgressManager()
        manager.xpSystem = XPSystem.preview
        manager.streak = Streak.preview
        manager.studyStats = StudyStats.preview
        manager.subjectMasteries = SubjectMastery.previewList
        manager.achievements = Achievement.previewList
        return manager
    }()
}
