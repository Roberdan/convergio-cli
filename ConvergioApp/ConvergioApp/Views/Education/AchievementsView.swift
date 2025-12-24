/**
 * CONVERGIO NATIVE - Achievements View
 *
 * View displaying all achievements with locked/unlocked states.
 * Shows progress toward locked achievements and details on tap.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

// MARK: - Achievements View

struct AchievementsView: View {
    @ObservedObject var progressManager: ProgressManager
    @Environment(\.dismiss) private var dismiss

    @State private var selectedCategory: AchievementCategory?
    @State private var selectedAchievement: Achievement?

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(spacing: 24) {
                    // Header stats
                    headerSection

                    // Category filter
                    categoryFilter

                    // Achievements grid
                    achievementsGrid
                }
                .padding(24)
            }
            .navigationTitle("Achievements")
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Done") {
                        dismiss()
                    }
                }
            }
            .background(backgroundGradient)
            .sheet(item: $selectedAchievement) { achievement in
                AchievementDetailView(
                    achievement: achievement,
                    progressManager: progressManager
                )
            }
        }
    }

    // MARK: - Header Section

    private var headerSection: some View {
        GlassCard(tint: .yellow) {
            HStack(spacing: 20) {
                // Trophy icon
                ZStack {
                    Circle()
                        .fill(
                            LinearGradient(
                                colors: [.yellow, .orange],
                                startPoint: .topLeading,
                                endPoint: .bottomTrailing
                            )
                        )
                        .frame(width: 60, height: 60)

                    Image(systemName: "trophy.fill")
                        .font(.title2)
                        .foregroundStyle(.white)
                }

                VStack(alignment: .leading, spacing: 4) {
                    Text("Achievements")
                        .font(.title2.bold())

                    Text("\(progressManager.unlockedAchievementsCount) of \(progressManager.totalAchievementsCount) unlocked")
                        .font(.subheadline)
                        .foregroundStyle(.secondary)

                    // Progress bar
                    GlassProgressBar(
                        progress: Double(progressManager.unlockedAchievementsCount) / Double(progressManager.totalAchievementsCount),
                        tint: .yellow,
                        height: 6
                    )
                }

                Spacer()
            }
            .padding(16)
        }
    }

    // MARK: - Category Filter

    private var categoryFilter: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 12) {
                // All category
                CategoryButton(
                    title: "All",
                    icon: "square.grid.2x2",
                    color: .primary,
                    isSelected: selectedCategory == nil
                ) {
                    selectedCategory = nil
                }

                // Category buttons
                ForEach(AchievementCategory.allCases, id: \.self) { category in
                    CategoryButton(
                        title: category.rawValue,
                        icon: category.icon,
                        color: category.color,
                        isSelected: selectedCategory == category
                    ) {
                        selectedCategory = category
                    }
                }
            }
        }
    }

    // MARK: - Achievements Grid

    private var achievementsGrid: some View {
        LazyVGrid(
            columns: [
                GridItem(.adaptive(minimum: 150, maximum: 200), spacing: 16)
            ],
            spacing: 16
        ) {
            ForEach(filteredAchievements) { achievement in
                AchievementCard(
                    achievement: achievement,
                    progressManager: progressManager
                )
                .onTapGesture {
                    selectedAchievement = achievement
                }
            }
        }
    }

    // MARK: - Filtered Achievements

    private var filteredAchievements: [Achievement] {
        if let category = selectedCategory {
            return progressManager.achievements.filter { $0.category == category }
        }
        return progressManager.achievements
    }

    // MARK: - Background

    private var backgroundGradient: some View {
        LinearGradient(
            colors: [
                Color.yellow.opacity(0.05),
                Color.orange.opacity(0.05)
            ],
            startPoint: .topLeading,
            endPoint: .bottomTrailing
        )
        .ignoresSafeArea()
    }
}

// MARK: - Category Button

private struct CategoryButton: View {
    let title: String
    let icon: String
    let color: Color
    let isSelected: Bool
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            HStack(spacing: 6) {
                Image(systemName: icon)
                    .font(.caption)

                Text(title)
                    .font(.caption.bold())
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(
                isSelected
                    ? color.opacity(0.2)
                    : Color.primary.opacity(0.05)
            )
            .foregroundStyle(isSelected ? color : .primary)
            .clipShape(Capsule())
            .overlay(
                Capsule()
                    .stroke(
                        isSelected ? color.opacity(0.5) : Color.clear,
                        lineWidth: 1
                    )
            )
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Achievement Card

private struct AchievementCard: View {
    let achievement: Achievement
    @ObservedObject var progressManager: ProgressManager

    private var progress: (current: Int, target: Int, progress: Double) {
        progressManager.getAchievementProgress(achievement)
    }

    var body: some View {
        GlassCard(tint: achievement.category.color) {
            VStack(spacing: 12) {
                // Icon
                ZStack {
                    Circle()
                        .fill(
                            achievement.isUnlocked
                                ? LinearGradient(
                                    colors: [
                                        achievement.category.color,
                                        achievement.category.color.opacity(0.7)
                                    ],
                                    startPoint: .topLeading,
                                    endPoint: .bottomTrailing
                                )
                                : LinearGradient(
                                    colors: [
                                        Color.gray.opacity(0.3),
                                        Color.gray.opacity(0.2)
                                    ],
                                    startPoint: .topLeading,
                                    endPoint: .bottomTrailing
                                )
                        )
                        .frame(width: 60, height: 60)

                    Image(systemName: achievement.icon)
                        .font(.title2)
                        .foregroundStyle(
                            achievement.isUnlocked
                                ? .white
                                : .secondary
                        )
                }

                // Title
                Text(achievement.title)
                    .font(.subheadline.bold())
                    .multilineTextAlignment(.center)
                    .lineLimit(2)
                    .foregroundStyle(
                        achievement.isUnlocked
                            ? .primary
                            : .secondary
                    )

                // Progress or unlock date
                if achievement.isUnlocked {
                    if let date = achievement.unlockedDate {
                        Text(formatDate(date))
                            .font(.caption2)
                            .foregroundStyle(.secondary)
                    }
                } else {
                    VStack(spacing: 4) {
                        Text(achievement.requirement.progressDescription(current: progress.current))
                            .font(.caption2)
                            .foregroundStyle(.secondary)

                        GlassProgressBar(
                            progress: progress.progress,
                            tint: achievement.category.color,
                            height: 4
                        )
                    }
                }

                // XP reward
                HStack(spacing: 4) {
                    Image(systemName: "star.fill")
                        .font(.caption2)
                    Text("+\(achievement.xpReward) XP")
                        .font(.caption2.bold())
                }
                .foregroundStyle(achievement.category.color)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(achievement.category.color.opacity(0.1))
                .clipShape(Capsule())
            }
            .padding(12)
        }
        .opacity(achievement.isUnlocked ? 1.0 : 0.7)
        .scaleEffect(achievement.isUnlocked ? 1.0 : 0.95)
    }

    private func formatDate(_ date: Date) -> String {
        let formatter = RelativeDateTimeFormatter()
        formatter.unitsStyle = .abbreviated
        return formatter.localizedString(for: date, relativeTo: Date())
    }
}

// MARK: - Achievement Detail View

struct AchievementDetailView: View {
    let achievement: Achievement
    @ObservedObject var progressManager: ProgressManager
    @Environment(\.dismiss) private var dismiss

    private var progress: (current: Int, target: Int, progress: Double) {
        progressManager.getAchievementProgress(achievement)
    }

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(spacing: 24) {
                    // Large icon
                    ZStack {
                        Circle()
                            .fill(
                                achievement.isUnlocked
                                    ? LinearGradient(
                                        colors: [
                                            achievement.category.color,
                                            achievement.category.color.opacity(0.7)
                                        ],
                                        startPoint: .topLeading,
                                        endPoint: .bottomTrailing
                                    )
                                    : LinearGradient(
                                        colors: [
                                            Color.gray.opacity(0.3),
                                            Color.gray.opacity(0.2)
                                        ],
                                        startPoint: .topLeading,
                                        endPoint: .bottomTrailing
                                    )
                            )
                            .frame(width: 120, height: 120)

                        Image(systemName: achievement.icon)
                            .font(.system(size: 50))
                            .foregroundStyle(
                                achievement.isUnlocked
                                    ? .white
                                    : .secondary
                            )
                    }
                    .shadow(color: achievement.category.color.opacity(0.3), radius: 20)

                    // Title and description
                    VStack(spacing: 8) {
                        Text(achievement.title)
                            .font(.title.bold())
                            .multilineTextAlignment(.center)

                        Text(achievement.description)
                            .font(.body)
                            .foregroundStyle(.secondary)
                            .multilineTextAlignment(.center)
                    }

                    // Details card
                    GlassCard(tint: achievement.category.color) {
                        VStack(spacing: 16) {
                            // Category
                            DetailRow(
                                label: "Category",
                                value: achievement.category.rawValue,
                                icon: achievement.category.icon,
                                color: achievement.category.color
                            )

                            Divider()

                            // XP Reward
                            DetailRow(
                                label: "XP Reward",
                                value: "+\(achievement.xpReward) XP",
                                icon: "star.fill",
                                color: .yellow
                            )

                            Divider()

                            // Progress or unlock date
                            if achievement.isUnlocked {
                                if let date = achievement.unlockedDate {
                                    DetailRow(
                                        label: "Unlocked",
                                        value: formatFullDate(date),
                                        icon: "checkmark.circle.fill",
                                        color: .green
                                    )
                                }
                            } else {
                                VStack(alignment: .leading, spacing: 8) {
                                    DetailRow(
                                        label: "Progress",
                                        value: achievement.requirement.progressDescription(current: progress.current),
                                        icon: "chart.bar.fill",
                                        color: achievement.category.color
                                    )

                                    GlassProgressBar(
                                        progress: progress.progress,
                                        tint: achievement.category.color,
                                        height: 8
                                    )
                                }
                            }
                        }
                        .padding(16)
                    }

                    Spacer()
                }
                .padding(24)
            }
            .navigationTitle("Achievement")
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Close") {
                        dismiss()
                    }
                }
            }
            .background(
                LinearGradient(
                    colors: [
                        achievement.category.color.opacity(0.1),
                        achievement.category.color.opacity(0.05)
                    ],
                    startPoint: .topLeading,
                    endPoint: .bottomTrailing
                )
                .ignoresSafeArea()
            )
        }
    }

    private func formatFullDate(_ date: Date) -> String {
        let formatter = DateFormatter()
        formatter.dateStyle = .medium
        formatter.timeStyle = .short
        return formatter.string(from: date)
    }
}

// MARK: - Detail Row

private struct DetailRow: View {
    let label: String
    let value: String
    let icon: String
    let color: Color

    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: icon)
                .foregroundStyle(color)
                .frame(width: 24)

            Text(label)
                .font(.subheadline)
                .foregroundStyle(.secondary)

            Spacer()

            Text(value)
                .font(.subheadline.bold())
        }
    }
}

// MARK: - Preview

#Preview {
    AchievementsView(progressManager: .preview)
}

#Preview("Detail") {
    AchievementDetailView(
        achievement: .preview,
        progressManager: .preview
    )
}
