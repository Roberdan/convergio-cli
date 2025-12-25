/**
 * CONVERGIO NATIVE - Progress Dashboard
 *
 * Main dashboard showing student progress, XP, streaks, achievements, and goals.
 * Designed to motivate students through gamification without dark patterns.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

// MARK: - Progress Dashboard

struct ProgressDashboard: View {
    @StateObject private var progressManager = ProgressManager()
    @State private var showAchievements = false

    var body: some View {
        ScrollView {
            VStack(spacing: 20) {
                // Header
                headerSection

                // XP and Level
                xpSection

                // Streak
                streakSection

                // Study Stats
                studyStatsSection

                // Subject Mastery
                subjectMasterySection

                // Recent Achievements
                recentAchievementsSection

                // Next Goals
                nextGoalsSection
            }
            .padding(24)
        }
        .navigationTitle("Your Progress")
        .background(backgroundGradient)
        .sheet(isPresented: $showAchievements) {
            AchievementsView(progressManager: progressManager)
        }
        .overlay(alignment: .top) {
            levelUpNotification
        }
        .overlay(alignment: .top) {
            achievementNotification
        }
    }

    // MARK: - Header

    private var headerSection: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text("Your Learning Journey")
                    .font(.title2.bold())
                Text("Keep up the great work!")
                    .font(.subheadline)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            // Achievements button
            Button {
                showAchievements = true
            } label: {
                HStack(spacing: 6) {
                    Image(systemName: "trophy.fill")
                    Text("\(progressManager.unlockedAchievementsCount)/\(progressManager.totalAchievementsCount)")
                        .font(.subheadline.bold())
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 6)
                .background(Color.yellow.opacity(0.2))
                .foregroundStyle(.yellow)
                .clipShape(Capsule())
            }
            .buttonStyle(.plain)
        }
    }

    // MARK: - XP Section

    private var xpSection: some View {
        GlassCard(tint: .blue) {
            VStack(spacing: 16) {
                // Level badge
                HStack(spacing: 12) {
                    // Level icon
                    ZStack {
                        Circle()
                            .fill(
                                LinearGradient(
                                    colors: [.blue, .purple],
                                    startPoint: .topLeading,
                                    endPoint: .bottomTrailing
                                )
                            )
                            .frame(width: 60, height: 60)

                        Text("\(progressManager.xpSystem.level)")
                            .font(.title.bold())
                            .foregroundStyle(.white)
                    }

                    VStack(alignment: .leading, spacing: 4) {
                        Text("Level \(progressManager.xpSystem.level)")
                            .font(.title3.bold())

                        Text("\(progressManager.xpSystem.totalXP) total XP")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }

                    Spacer()
                }

                // Progress to next level
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        Text("Next Level")
                            .font(.caption.bold())
                        Spacer()
                        Text("\(progressManager.xpSystem.xpToNextLevel) XP to go")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }

                    GlassProgressBar(
                        progress: progressManager.xpSystem.progressToNextLevel,
                        tint: .blue,
                        height: 10
                    )
                }
            }
            .padding(16)
        }
    }

    // MARK: - Streak Section

    private var streakSection: some View {
        GlassCard(tint: .orange) {
            HStack(spacing: 16) {
                // Flame icon
                ZStack {
                    Circle()
                        .fill(
                            LinearGradient(
                                colors: [.orange, .red],
                                startPoint: .top,
                                endPoint: .bottom
                            )
                        )
                        .frame(width: 50, height: 50)

                    Image(systemName: "flame.fill")
                        .font(.title2)
                        .foregroundStyle(.white)
                }

                VStack(alignment: .leading, spacing: 4) {
                    Text("Study Streak")
                        .font(.headline)

                    HStack(spacing: 12) {
                        // Current streak
                        VStack(alignment: .leading, spacing: 2) {
                            Text("\(progressManager.streak.currentStreak)")
                                .font(.title2.bold())
                            Text("Current")
                                .font(.caption2)
                                .foregroundStyle(.secondary)
                        }

                        Divider()
                            .frame(height: 30)

                        // Longest streak
                        VStack(alignment: .leading, spacing: 2) {
                            Text("\(progressManager.streak.longestStreak)")
                                .font(.title2.bold())
                            Text("Best")
                                .font(.caption2)
                                .foregroundStyle(.secondary)
                        }
                    }
                }

                Spacer()

                // Active indicator
                if progressManager.streak.isActive {
                    GlassBadge(
                        text: "Active",
                        icon: "checkmark.circle.fill",
                        tint: .green
                    )
                }
            }
            .padding(16)
        }
    }

    // MARK: - Study Stats Section

    private var studyStatsSection: some View {
        GlassCard(tint: .green) {
            VStack(spacing: 12) {
                Text("This Week")
                    .font(.headline)
                    .frame(maxWidth: .infinity, alignment: .leading)

                LazyVGrid(columns: [
                    GridItem(.flexible()),
                    GridItem(.flexible())
                ], spacing: 16) {
                    StatCard(
                        title: "Study Time",
                        value: formatMinutes(progressManager.studyStats.totalStudyMinutes),
                        icon: "clock.fill",
                        color: .blue
                    )

                    StatCard(
                        title: "Sessions",
                        value: "\(progressManager.studyStats.sessionsThisWeek)",
                        icon: "book.fill",
                        color: .purple
                    )

                    StatCard(
                        title: "Questions",
                        value: "\(progressManager.studyStats.questionsAsked)",
                        icon: "questionmark.circle.fill",
                        color: .orange
                    )

                    StatCard(
                        title: "Subjects",
                        value: "\(progressManager.studyStats.subjectsStudied.count)",
                        icon: "square.grid.2x2.fill",
                        color: .green
                    )
                }
            }
            .padding(16)
        }
    }

    // MARK: - Subject Mastery Section

    private var subjectMasterySection: some View {
        GlassCard {
            VStack(alignment: .leading, spacing: 12) {
                Text("Subject Mastery")
                    .font(.headline)

                if progressManager.subjectMasteries.isEmpty {
                    Text("Start studying to track your mastery!")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .padding(.vertical, 8)
                } else {
                    ForEach(progressManager.subjectMasteries.prefix(5)) { mastery in
                        MasteryRow(mastery: mastery)
                    }
                }
            }
            .padding(16)
        }
    }

    // MARK: - Recent Achievements Section

    private var recentAchievementsSection: some View {
        GlassCard(tint: .yellow) {
            VStack(alignment: .leading, spacing: 12) {
                HStack {
                    Text("Recent Achievements")
                        .font(.headline)

                    Spacer()

                    Button("View All") {
                        showAchievements = true
                    }
                    .font(.caption.bold())
                    .foregroundStyle(.yellow)
                }

                let recentAchievements = progressManager.getRecentAchievements()

                if recentAchievements.isEmpty {
                    Text("Complete your first achievement!")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .padding(.vertical, 8)
                } else {
                    HStack(spacing: 12) {
                        ForEach(recentAchievements.prefix(4)) { achievement in
                            AchievementBadge(achievement: achievement, size: 50)
                        }
                    }
                }
            }
            .padding(16)
        }
    }

    // MARK: - Next Goals Section

    private var nextGoalsSection: some View {
        GlassCard(tint: .purple) {
            VStack(alignment: .leading, spacing: 12) {
                Text("Next Goals")
                    .font(.headline)

                if let nextGoal = progressManager.getNextGoal() {
                    let progress = progressManager.getAchievementProgress(nextGoal)

                    HStack(spacing: 12) {
                        Image(systemName: nextGoal.icon)
                            .font(.title2)
                            .foregroundStyle(nextGoal.category.color)
                            .frame(width: 40, height: 40)
                            .background(nextGoal.category.color.opacity(0.2))
                            .clipShape(Circle())

                        VStack(alignment: .leading, spacing: 4) {
                            Text(nextGoal.title)
                                .font(.subheadline.bold())

                            Text(nextGoal.requirement.progressDescription(current: progress.current))
                                .font(.caption)
                                .foregroundStyle(.secondary)

                            GlassProgressBar(
                                progress: progress.progress,
                                tint: nextGoal.category.color,
                                height: 6
                            )
                        }
                    }
                } else {
                    Text("All achievements unlocked!")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .padding(.vertical, 8)
                }
            }
            .padding(16)
        }
    }

    // MARK: - Background

    private var backgroundGradient: some View {
        LinearGradient(
            colors: [
                Color.blue.opacity(0.05),
                Color.purple.opacity(0.05)
            ],
            startPoint: .topLeading,
            endPoint: .bottomTrailing
        )
        .ignoresSafeArea()
    }

    // MARK: - Level Up Notification

    @ViewBuilder
    private var levelUpNotification: some View {
        if let notification = progressManager.levelUpNotification {
            GlassCard(tint: .yellow) {
                HStack(spacing: 12) {
                    Image(systemName: "star.fill")
                        .font(.title)
                        .foregroundStyle(.yellow)

                    VStack(alignment: .leading, spacing: 2) {
                        Text("Level Up!")
                            .font(.headline)
                        Text("You're now level \(notification.level)")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }

                    Spacer()

                    Text("+\(notification.xpGained) XP")
                        .font(.caption.bold())
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(Color.yellow.opacity(0.2))
                        .clipShape(Capsule())
                }
                .padding(12)
            }
            .frame(maxWidth: 400)
            .padding(.top, 16)
            .transition(.move(edge: .top).combined(with: .opacity))
            .animation(.spring(duration: 0.5), value: progressManager.levelUpNotification != nil)
        }
    }

    // MARK: - Achievement Notification

    @ViewBuilder
    private var achievementNotification: some View {
        if let achievement = progressManager.achievementUnlocked {
            GlassCard(tint: achievement.category.color) {
                HStack(spacing: 12) {
                    Image(systemName: achievement.icon)
                        .font(.title)
                        .foregroundStyle(achievement.category.color)

                    VStack(alignment: .leading, spacing: 2) {
                        Text("Achievement Unlocked!")
                            .font(.headline)
                        Text(achievement.title)
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }

                    Spacer()

                    Text("+\(achievement.xpReward) XP")
                        .font(.caption.bold())
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(achievement.category.color.opacity(0.2))
                        .clipShape(Capsule())
                }
                .padding(12)
            }
            .frame(maxWidth: 400)
            .padding(.top, 16)
            .transition(.move(edge: .top).combined(with: .opacity))
            .animation(.spring(duration: 0.5), value: progressManager.achievementUnlocked != nil)
        }
    }

    // MARK: - Helper

    private func formatMinutes(_ minutes: Int) -> String {
        let hours = minutes / 60
        let mins = minutes % 60
        if hours > 0 {
            return "\(hours)h \(mins)m"
        } else {
            return "\(mins)m"
        }
    }
}

// MARK: - Stat Card

private struct StatCard: View {
    let title: String
    let value: String
    let icon: String
    let color: Color

    var body: some View {
        VStack(spacing: 8) {
            Image(systemName: icon)
                .font(.title3)
                .foregroundStyle(color)

            Text(value)
                .font(.title3.bold())

            Text(title)
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, 12)
        .background(color.opacity(0.1))
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

// MARK: - Mastery Row

private struct MasteryRow: View {
    let mastery: SubjectMastery

    var body: some View {
        VStack(spacing: 6) {
            HStack {
                HStack(spacing: 8) {
                    Image(systemName: mastery.subject.icon)
                        .foregroundStyle(mastery.subject.color)
                        .frame(width: 20)

                    Text(mastery.subject.rawValue)
                        .font(.subheadline)
                }

                Spacer()

                HStack(spacing: 4) {
                    Image(systemName: mastery.masteryTier.icon)
                        .font(.caption2)

                    Text("\(mastery.masteryPercentage)%")
                        .font(.caption.bold())
                }
                .foregroundStyle(mastery.masteryTier.color)
            }

            GlassProgressBar(
                progress: mastery.masteryPercent,
                tint: mastery.subject.color,
                height: 6
            )
        }
    }
}

// MARK: - Achievement Badge

private struct AchievementBadge: View {
    let achievement: Achievement
    let size: CGFloat

    var body: some View {
        ZStack {
            Circle()
                .fill(
                    LinearGradient(
                        colors: [
                            achievement.category.color,
                            achievement.category.color.opacity(0.7)
                        ],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .frame(width: size, height: size)

            Image(systemName: achievement.icon)
                .font(.title3)
                .foregroundStyle(.white)
        }
    }
}

// MARK: - Preview

#Preview {
    ProgressDashboard()
}
