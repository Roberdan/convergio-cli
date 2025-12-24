/**
 * CONVERGIO NATIVE - Achievement Model
 *
 * Achievement system for gamification and student motivation.
 * Designed to celebrate progress without creating addictive patterns.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import SwiftUI

// MARK: - Achievement Category

enum AchievementCategory: String, Codable, CaseIterable {
    case study = "Study"
    case mastery = "Mastery"
    case streak = "Streak"
    case social = "Social"
    case exploration = "Exploration"

    var icon: String {
        switch self {
        case .study: return "book.fill"
        case .mastery: return "star.fill"
        case .streak: return "flame.fill"
        case .social: return "person.2.fill"
        case .exploration: return "safari.fill"
        }
    }

    var color: Color {
        switch self {
        case .study: return .blue
        case .mastery: return .yellow
        case .streak: return .orange
        case .social: return .green
        case .exploration: return .purple
        }
    }
}

// MARK: - Achievement Requirement

/// Defines what needs to be done to unlock an achievement
struct AchievementRequirement: Codable, Hashable {
    let type: RequirementType
    let target: Int

    enum RequirementType: String, Codable {
        case studySessions
        case questionsAsked
        case streakDays
        case subjectMastery
        case differentSubjects
        case nightOwlSessions
        case totalXP
        case studyMinutes
    }

    /// Get current progress description
    func progressDescription(current: Int) -> String {
        switch type {
        case .studySessions:
            return "\(current)/\(target) sessions"
        case .questionsAsked:
            return "\(current)/\(target) questions"
        case .streakDays:
            return "\(current)/\(target) days"
        case .subjectMastery:
            return "\(current)% mastery"
        case .differentSubjects:
            return "\(current)/\(target) subjects"
        case .nightOwlSessions:
            return "\(current)/\(target) night sessions"
        case .totalXP:
            return "\(current)/\(target) XP"
        case .studyMinutes:
            return "\(current)/\(target) minutes"
        }
    }
}

// MARK: - Achievement

struct Achievement: Identifiable, Codable, Hashable {
    let id: UUID
    let title: String
    let description: String
    let icon: String
    let category: AchievementCategory
    let requirement: AchievementRequirement
    var unlockedDate: Date?
    let xpReward: Int

    init(
        title: String,
        description: String,
        icon: String,
        category: AchievementCategory,
        requirement: AchievementRequirement,
        unlockedDate: Date? = nil,
        xpReward: Int = 50
    ) {
        self.id = UUID()
        self.title = title
        self.description = description
        self.icon = icon
        self.category = category
        self.requirement = requirement
        self.unlockedDate = unlockedDate
        self.xpReward = xpReward
    }

    /// Whether this achievement is unlocked
    var isUnlocked: Bool {
        unlockedDate != nil
    }

    /// Unlock this achievement
    mutating func unlock() {
        if !isUnlocked {
            unlockedDate = Date()
        }
    }

    /// Get progress towards this achievement (0.0 to 1.0)
    func progress(current: Int) -> Double {
        guard !isUnlocked else { return 1.0 }
        return min(1.0, Double(current) / Double(requirement.target))
    }

    /// Check if requirement is met
    func requirementMet(current: Int) -> Bool {
        current >= requirement.target
    }
}

// MARK: - All Achievements

extension Achievement {
    static let allAchievements: [Achievement] = [
        // MARK: Study Achievements
        Achievement(
            title: "First Step",
            description: "Complete your first study session",
            icon: "figure.walk",
            category: .study,
            requirement: AchievementRequirement(type: .studySessions, target: 1),
            xpReward: 25
        ),
        Achievement(
            title: "Getting Started",
            description: "Complete 5 study sessions",
            icon: "book",
            category: .study,
            requirement: AchievementRequirement(type: .studySessions, target: 5),
            xpReward: 50
        ),
        Achievement(
            title: "Dedicated Student",
            description: "Complete 25 study sessions",
            icon: "book.fill",
            category: .study,
            requirement: AchievementRequirement(type: .studySessions, target: 25),
            xpReward: 100
        ),
        Achievement(
            title: "Scholar",
            description: "Complete 100 study sessions",
            icon: "graduationcap.fill",
            category: .study,
            requirement: AchievementRequirement(type: .studySessions, target: 100),
            xpReward: 250
        ),
        Achievement(
            title: "Marathon Learner",
            description: "Study for 10 hours total",
            icon: "timer",
            category: .study,
            requirement: AchievementRequirement(type: .studyMinutes, target: 600),
            xpReward: 150
        ),

        // MARK: Mastery Achievements
        Achievement(
            title: "First Mastery",
            description: "Reach 50% mastery in any subject",
            icon: "star.circle",
            category: .mastery,
            requirement: AchievementRequirement(type: .subjectMastery, target: 50),
            xpReward: 75
        ),
        Achievement(
            title: "Subject Master",
            description: "Reach 100% mastery in one subject",
            icon: "star.circle.fill",
            category: .mastery,
            requirement: AchievementRequirement(type: .subjectMastery, target: 100),
            xpReward: 200
        ),
        Achievement(
            title: "Renaissance Person",
            description: "Study 5 different subjects",
            icon: "brain.head.profile",
            category: .mastery,
            requirement: AchievementRequirement(type: .differentSubjects, target: 5),
            xpReward: 150
        ),
        Achievement(
            title: "Polymath",
            description: "Study 10 different subjects",
            icon: "brain",
            category: .mastery,
            requirement: AchievementRequirement(type: .differentSubjects, target: 10),
            xpReward: 300
        ),

        // MARK: Streak Achievements
        Achievement(
            title: "Consistent",
            description: "Maintain a 3-day study streak",
            icon: "flame",
            category: .streak,
            requirement: AchievementRequirement(type: .streakDays, target: 3),
            xpReward: 50
        ),
        Achievement(
            title: "Week Warrior",
            description: "Maintain a 7-day study streak",
            icon: "flame.fill",
            category: .streak,
            requirement: AchievementRequirement(type: .streakDays, target: 7),
            xpReward: 100
        ),
        Achievement(
            title: "Two Weeks Strong",
            description: "Maintain a 14-day study streak",
            icon: "bolt.fill",
            category: .streak,
            requirement: AchievementRequirement(type: .streakDays, target: 14),
            xpReward: 200
        ),
        Achievement(
            title: "Month Master",
            description: "Maintain a 30-day study streak",
            icon: "calendar.badge.checkmark",
            category: .streak,
            requirement: AchievementRequirement(type: .streakDays, target: 30),
            xpReward: 500
        ),

        // MARK: Social/Curiosity Achievements
        Achievement(
            title: "Curious Mind",
            description: "Ask your first question",
            icon: "questionmark.circle",
            category: .social,
            requirement: AchievementRequirement(type: .questionsAsked, target: 1),
            xpReward: 25
        ),
        Achievement(
            title: "Inquisitive",
            description: "Ask 10 questions",
            icon: "questionmark.circle.fill",
            category: .social,
            requirement: AchievementRequirement(type: .questionsAsked, target: 10),
            xpReward: 75
        ),
        Achievement(
            title: "Question Master",
            description: "Ask 50 questions",
            icon: "bubble.left.and.bubble.right.fill",
            category: .social,
            requirement: AchievementRequirement(type: .questionsAsked, target: 50),
            xpReward: 150
        ),

        // MARK: Exploration Achievements
        Achievement(
            title: "Night Owl",
            description: "Study after 9pm once",
            icon: "moon.stars.fill",
            category: .exploration,
            requirement: AchievementRequirement(type: .nightOwlSessions, target: 1),
            xpReward: 50
        ),
        Achievement(
            title: "Midnight Scholar",
            description: "Study after 9pm 10 times",
            icon: "moon.fill",
            category: .exploration,
            requirement: AchievementRequirement(type: .nightOwlSessions, target: 10),
            xpReward: 100
        ),

        // MARK: XP Milestones
        Achievement(
            title: "Rising Star",
            description: "Reach 500 total XP",
            icon: "star.leadinghalf.filled",
            category: .study,
            requirement: AchievementRequirement(type: .totalXP, target: 500),
            xpReward: 100
        ),
        Achievement(
            title: "Elite Learner",
            description: "Reach 1000 total XP",
            icon: "crown",
            category: .study,
            requirement: AchievementRequirement(type: .totalXP, target: 1000),
            xpReward: 200
        ),
        Achievement(
            title: "Legend",
            description: "Reach 5000 total XP",
            icon: "crown.fill",
            category: .study,
            requirement: AchievementRequirement(type: .totalXP, target: 5000),
            xpReward: 500
        )
    ]

    /// Get achievements by category
    static func achievements(for category: AchievementCategory) -> [Achievement] {
        allAchievements.filter { $0.category == category }
    }

    /// Get unlocked achievements count
    static func unlockedCount(in achievements: [Achievement]) -> Int {
        achievements.filter { $0.isUnlocked }.count
    }

    /// Get total achievements count
    static var totalCount: Int {
        allAchievements.count
    }

    /// Preview instances
    static let preview = Achievement(
        title: "First Step",
        description: "Complete your first study session",
        icon: "figure.walk",
        category: .study,
        requirement: AchievementRequirement(type: .studySessions, target: 1),
        unlockedDate: Date(),
        xpReward: 25
    )

    static let previewLocked = Achievement(
        title: "Scholar",
        description: "Complete 100 study sessions",
        icon: "graduationcap.fill",
        category: .study,
        requirement: AchievementRequirement(type: .studySessions, target: 100),
        xpReward: 250
    )

    static let previewList: [Achievement] = [
        Achievement(
            title: "First Step",
            description: "Complete your first study session",
            icon: "figure.walk",
            category: .study,
            requirement: AchievementRequirement(type: .studySessions, target: 1),
            unlockedDate: Date(),
            xpReward: 25
        ),
        Achievement(
            title: "Week Warrior",
            description: "Maintain a 7-day study streak",
            icon: "flame.fill",
            category: .streak,
            requirement: AchievementRequirement(type: .streakDays, target: 7),
            unlockedDate: Date(),
            xpReward: 100
        ),
        Achievement(
            title: "Curious Mind",
            description: "Ask your first question",
            icon: "questionmark.circle",
            category: .social,
            requirement: AchievementRequirement(type: .questionsAsked, target: 1),
            unlockedDate: Date(),
            xpReward: 25
        ),
        Achievement(
            title: "Scholar",
            description: "Complete 100 study sessions",
            icon: "graduationcap.fill",
            category: .study,
            requirement: AchievementRequirement(type: .studySessions, target: 100),
            xpReward: 250
        ),
        Achievement(
            title: "Night Owl",
            description: "Study after 9pm once",
            icon: "moon.stars.fill",
            category: .exploration,
            requirement: AchievementRequirement(type: .nightOwlSessions, target: 1),
            xpReward: 50
        )
    ]
}
