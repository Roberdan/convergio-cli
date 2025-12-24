/**
 * CONVERGIO NATIVE - Progress Model
 *
 * Models for tracking student progress, XP system, streaks, and subject mastery.
 * Designed to motivate students through gamification without dark patterns.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import SwiftUI

// MARK: - XP System

/// Experience points system with levels
struct XPSystem: Codable, Hashable {
    var totalXP: Int
    var level: Int
    var xpToNextLevel: Int

    init(totalXP: Int = 0) {
        self.totalXP = totalXP
        self.level = XPSystem.calculateLevel(from: totalXP)
        self.xpToNextLevel = XPSystem.calculateXPToNextLevel(from: totalXP)
    }

    /// Calculate level from total XP
    static func calculateLevel(from totalXP: Int) -> Int {
        // Level thresholds: exponential growth to keep it challenging but achievable
        // Level 1: 0-100 XP
        // Level 2: 100-300 XP
        // Level 3: 300-600 XP
        // Level 4: 600-1000 XP
        // Level 5: 1000-1500 XP
        // And so on...

        if totalXP < 100 { return 1 }
        if totalXP < 300 { return 2 }
        if totalXP < 600 { return 3 }
        if totalXP < 1000 { return 4 }
        if totalXP < 1500 { return 5 }
        if totalXP < 2100 { return 6 }
        if totalXP < 2800 { return 7 }
        if totalXP < 3600 { return 8 }
        if totalXP < 4500 { return 9 }
        if totalXP < 5500 { return 10 }

        // Beyond level 10: +1000 XP per level
        return 10 + (totalXP - 5500) / 1000
    }

    /// Calculate XP needed to reach next level
    static func calculateXPToNextLevel(from totalXP: Int) -> Int {
        let currentLevel = calculateLevel(from: totalXP)
        let nextLevelThreshold = levelThreshold(for: currentLevel + 1)
        return nextLevelThreshold - totalXP
    }

    /// Get the XP threshold for a specific level
    static func levelThreshold(for level: Int) -> Int {
        switch level {
        case 1: return 0
        case 2: return 100
        case 3: return 300
        case 4: return 600
        case 5: return 1000
        case 6: return 1500
        case 7: return 2100
        case 8: return 2800
        case 9: return 3600
        case 10: return 4500
        case 11: return 5500
        default: return 5500 + (level - 11) * 1000
        }
    }

    /// Get progress percentage to next level
    var progressToNextLevel: Double {
        let currentLevelThreshold = XPSystem.levelThreshold(for: level)
        let nextLevelThreshold = XPSystem.levelThreshold(for: level + 1)
        let xpInCurrentLevel = totalXP - currentLevelThreshold
        let xpNeededForLevel = nextLevelThreshold - currentLevelThreshold
        return Double(xpInCurrentLevel) / Double(xpNeededForLevel)
    }

    /// Add XP and return true if leveled up
    mutating func addXP(_ amount: Int) -> Bool {
        let oldLevel = level
        totalXP += amount
        level = XPSystem.calculateLevel(from: totalXP)
        xpToNextLevel = XPSystem.calculateXPToNextLevel(from: totalXP)
        return level > oldLevel
    }
}

// MARK: - Streak System

/// Daily study streak tracking
struct Streak: Codable, Hashable {
    var currentStreak: Int
    var longestStreak: Int
    var lastStudyDate: Date?

    init(currentStreak: Int = 0, longestStreak: Int = 0, lastStudyDate: Date? = nil) {
        self.currentStreak = currentStreak
        self.longestStreak = longestStreak
        self.lastStudyDate = lastStudyDate
    }

    /// Check if streak is active (studied today or yesterday)
    var isActive: Bool {
        guard let lastDate = lastStudyDate else { return false }
        let calendar = Calendar.current
        let today = calendar.startOfDay(for: Date())
        let lastStudy = calendar.startOfDay(for: lastDate)
        let daysDiff = calendar.dateComponents([.day], from: lastStudy, to: today).day ?? 0
        return daysDiff <= 1
    }

    /// Update streak based on a new study session
    mutating func recordStudySession(on date: Date = Date()) {
        let calendar = Calendar.current
        let today = calendar.startOfDay(for: date)

        if let lastDate = lastStudyDate {
            let lastStudy = calendar.startOfDay(for: lastDate)
            let daysDiff = calendar.dateComponents([.day], from: lastStudy, to: today).day ?? 0

            if daysDiff == 0 {
                // Same day, no change to streak
                return
            } else if daysDiff == 1 {
                // Consecutive day, increment streak
                currentStreak += 1
                if currentStreak > longestStreak {
                    longestStreak = currentStreak
                }
            } else {
                // Streak broken, reset to 1
                currentStreak = 1
            }
        } else {
            // First study session
            currentStreak = 1
            longestStreak = 1
        }

        lastStudyDate = date
    }
}

// MARK: - Subject Mastery

/// Mastery tracking for a specific subject
struct SubjectMastery: Codable, Hashable, Identifiable {
    let id: UUID
    let subject: Subject
    var masteryPercent: Double // 0.0 to 1.0
    var topicsCompleted: Int
    var totalTopics: Int
    var lastStudied: Date?

    init(
        subject: Subject,
        masteryPercent: Double = 0.0,
        topicsCompleted: Int = 0,
        totalTopics: Int = 10,
        lastStudied: Date? = nil
    ) {
        self.id = UUID()
        self.subject = subject
        self.masteryPercent = min(1.0, max(0.0, masteryPercent))
        self.topicsCompleted = topicsCompleted
        self.totalTopics = totalTopics
        self.lastStudied = lastStudied
    }

    /// Calculated mastery level as percentage
    var masteryPercentage: Int {
        Int(masteryPercent * 100)
    }

    /// Mastery tier
    var masteryTier: MasteryTier {
        switch masteryPercent {
        case 0..<0.25: return .beginner
        case 0.25..<0.5: return .intermediate
        case 0.5..<0.75: return .advanced
        case 0.75..<1.0: return .expert
        default: return .master
        }
    }

    /// Update mastery based on completed topics
    mutating func updateMastery() {
        if totalTopics > 0 {
            masteryPercent = Double(topicsCompleted) / Double(totalTopics)
        }
    }

    /// Record study session
    mutating func recordStudy() {
        lastStudied = Date()
    }
}

// MARK: - Mastery Tier

enum MasteryTier: String, Codable {
    case beginner = "Beginner"
    case intermediate = "Intermediate"
    case advanced = "Advanced"
    case expert = "Expert"
    case master = "Master"

    var color: Color {
        switch self {
        case .beginner: return .gray
        case .intermediate: return .blue
        case .advanced: return .purple
        case .expert: return .orange
        case .master: return .yellow
        }
    }

    var icon: String {
        switch self {
        case .beginner: return "star"
        case .intermediate: return "star.fill"
        case .advanced: return "star.circle.fill"
        case .expert: return "trophy"
        case .master: return "crown.fill"
        }
    }
}

// MARK: - Study Stats

/// Overall study statistics
struct StudyStats: Codable, Hashable {
    var totalStudyMinutes: Int
    var sessionsThisWeek: Int
    var questionsAsked: Int
    var subjectsStudied: Set<String>
    var nightOwlSessions: Int // Sessions after 9pm

    init(
        totalStudyMinutes: Int = 0,
        sessionsThisWeek: Int = 0,
        questionsAsked: Int = 0,
        subjectsStudied: Set<String> = [],
        nightOwlSessions: Int = 0
    ) {
        self.totalStudyMinutes = totalStudyMinutes
        self.sessionsThisWeek = sessionsThisWeek
        self.questionsAsked = questionsAsked
        self.subjectsStudied = subjectsStudied
        self.nightOwlSessions = nightOwlSessions
    }

    /// Add study time
    mutating func addStudyTime(minutes: Int) {
        totalStudyMinutes += minutes
    }

    /// Record a question
    mutating func recordQuestion() {
        questionsAsked += 1
    }

    /// Record subject studied
    mutating func recordSubject(_ subject: String) {
        subjectsStudied.insert(subject)
    }

    /// Record night owl session (after 9pm)
    mutating func recordNightOwlSession() {
        nightOwlSessions += 1
    }

    /// Check if current time is after 9pm
    static var isNightOwlTime: Bool {
        let hour = Calendar.current.component(.hour, from: Date())
        return hour >= 21 || hour < 6
    }
}

// MARK: - Preview Data

extension XPSystem {
    static let preview = XPSystem(totalXP: 450)
    static let previewHighLevel = XPSystem(totalXP: 3500)
}

extension Streak {
    static let preview = Streak(currentStreak: 7, longestStreak: 12, lastStudyDate: Date())
    static let previewInactive = Streak(currentStreak: 0, longestStreak: 5, lastStudyDate: Date().addingTimeInterval(-3 * 24 * 60 * 60))
}

extension SubjectMastery {
    static let preview = SubjectMastery(
        subject: .matematica,
        masteryPercent: 0.65,
        topicsCompleted: 13,
        totalTopics: 20,
        lastStudied: Date()
    )

    static let previewList: [SubjectMastery] = [
        SubjectMastery(subject: .matematica, masteryPercent: 0.75, topicsCompleted: 15, totalTopics: 20),
        SubjectMastery(subject: .fisica, masteryPercent: 0.60, topicsCompleted: 12, totalTopics: 20),
        SubjectMastery(subject: .italiano, masteryPercent: 0.45, topicsCompleted: 9, totalTopics: 20),
        SubjectMastery(subject: .inglese, masteryPercent: 0.30, topicsCompleted: 6, totalTopics: 20),
        SubjectMastery(subject: .storia, masteryPercent: 0.85, topicsCompleted: 17, totalTopics: 20)
    ]
}

extension StudyStats {
    static let preview = StudyStats(
        totalStudyMinutes: 420,
        sessionsThisWeek: 12,
        questionsAsked: 48,
        subjectsStudied: ["Matematica", "Fisica", "Italiano", "Storia"],
        nightOwlSessions: 3
    )
}
