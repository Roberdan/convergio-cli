/**
 * CONVERGIO NATIVE - Cost Dashboard View
 *
 * Real-time cost tracking and budget visualization.
 * Shows session costs, token usage, and budget progress.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

struct CostDashboardView: View {
    @ObservedObject var viewModel: OrchestratorViewModel

    @State private var showingDetailedUsage = false

    var body: some View {
        VStack(spacing: 20) {
            // Header
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Cost Dashboard")
                        .font(.title2.weight(.semibold))
                    Text("Real-time spending & budget tracking")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }

                Spacer()

                GlassButton(title: "Refresh", icon: "arrow.clockwise", action: {
                    Task {
                        await viewModel.refresh()
                    }
                }, tint: .secondary)
            }

            // Main cost display
            HStack(spacing: 24) {
                // Session cost
                CostMetricCard(
                    title: "Session Cost",
                    value: viewModel.costInfo.sessionCost,
                    icon: "clock.fill",
                    tint: .blue
                )

                // Budget progress
                BudgetProgressCard(
                    spent: viewModel.costInfo.sessionCost,
                    limit: viewModel.costInfo.budgetLimit,
                    isOverBudget: viewModel.costInfo.isOverBudget
                )

                // Total cost
                CostMetricCard(
                    title: "Total Spent",
                    value: viewModel.costInfo.totalCost,
                    icon: "chart.line.uptrend.xyaxis",
                    tint: .purple
                )
            }

            Divider()

            // Token usage section
            VStack(alignment: .leading, spacing: 12) {
                HStack {
                    Text("Token Usage")
                        .font(.headline)

                    Spacer()

                    Button {
                        showingDetailedUsage.toggle()
                    } label: {
                        Image(systemName: showingDetailedUsage ? "chevron.up" : "chevron.down")
                            .foregroundStyle(.secondary)
                    }
                    .buttonStyle(.plain)
                }

                // Session tokens
                TokenUsageRow(
                    label: "Session",
                    usage: viewModel.costInfo.sessionUsage
                )

                if showingDetailedUsage {
                    // Total tokens
                    TokenUsageRow(
                        label: "All Time",
                        usage: viewModel.costInfo.totalUsage
                    )

                    // Cache efficiency
                    CacheEfficiencyRow(
                        cached: viewModel.costInfo.sessionUsage.cachedTokens,
                        total: viewModel.costInfo.sessionUsage.inputTokens
                    )
                }
            }

            Spacer()

            // Budget warning (if applicable)
            if viewModel.costInfo.budgetUsagePercent > 0.8 {
                BudgetWarningBanner(
                    remaining: viewModel.costInfo.budgetRemaining,
                    isOverBudget: viewModel.costInfo.isOverBudget
                )
            }
        }
        .padding(24)
        .frame(minWidth: 400, minHeight: 300)
    }
}

// MARK: - Cost Metric Card

private struct CostMetricCard: View {
    let title: String
    let value: Double
    let icon: String
    let tint: Color

    var body: some View {
        GlassCard(tint: tint) {
            VStack(spacing: 8) {
                HStack {
                    Image(systemName: icon)
                        .foregroundStyle(tint)
                    Spacer()
                }

                Spacer()

                Text(formatCurrency(value))
                    .font(.system(size: 28, weight: .bold, design: .rounded))
                    .foregroundStyle(.primary)

                Text(title)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            .padding(16)
        }
        .frame(width: 140, height: 120)
    }

    private func formatCurrency(_ value: Double) -> String {
        if value < 1.0 {
            return String(format: "$%.3f", value)
        } else {
            return String(format: "$%.2f", value)
        }
    }
}

// MARK: - Budget Progress Card

private struct BudgetProgressCard: View {
    let spent: Double
    let limit: Double
    let isOverBudget: Bool

    private var progress: Double {
        guard limit > 0 else { return 0 }
        return min(1.0, spent / limit)
    }

    private var progressColor: Color {
        if isOverBudget { return .red }
        if progress > 0.8 { return .orange }
        return .green
    }

    var body: some View {
        GlassCard(tint: progressColor) {
            VStack(spacing: 12) {
                // Circular progress
                ZStack {
                    Circle()
                        .stroke(progressColor.opacity(0.2), lineWidth: 8)

                    Circle()
                        .trim(from: 0, to: CGFloat(progress))
                        .stroke(progressColor, style: StrokeStyle(lineWidth: 8, lineCap: .round))
                        .rotationEffect(.degrees(-90))

                    VStack(spacing: 2) {
                        Text("\(Int(progress * 100))%")
                            .font(.system(size: 18, weight: .bold, design: .rounded))
                        Text("used")
                            .font(.caption2)
                            .foregroundStyle(.secondary)
                    }
                }
                .frame(width: 70, height: 70)

                Text("Budget")
                    .font(.caption)
                    .foregroundStyle(.secondary)

                Text(String(format: "$%.2f limit", limit))
                    .font(.caption2)
                    .foregroundStyle(.tertiary)
            }
            .padding(16)
        }
        .frame(width: 140, height: 160)
    }
}

// MARK: - Token Usage Row

private struct TokenUsageRow: View {
    let label: String
    let usage: TokenUsage

    var body: some View {
        HStack(spacing: 16) {
            Text(label)
                .font(.subheadline)
                .foregroundStyle(.secondary)
                .frame(width: 60, alignment: .leading)

            HStack(spacing: 12) {
                TokenBadge(label: "In", value: usage.inputTokens, color: .blue)
                TokenBadge(label: "Out", value: usage.outputTokens, color: .green)
                TokenBadge(label: "Cache", value: usage.cachedTokens, color: .orange)
            }

            Spacer()

            Text(formatTokenCost(usage.estimatedCost))
                .font(.subheadline.monospacedDigit())
                .foregroundStyle(.primary)
        }
        .padding(.vertical, 8)
        .padding(.horizontal, 12)
        .background(Color.primary.opacity(0.03))
        .clipShape(RoundedRectangle(cornerRadius: 8))
    }

    private func formatTokenCost(_ cost: Double) -> String {
        if cost < 1.0 {
            return String(format: "$%.4f", cost)
        } else {
            return String(format: "$%.2f", cost)
        }
    }
}

// MARK: - Token Badge

private struct TokenBadge: View {
    let label: String
    let value: Int
    let color: Color

    var body: some View {
        VStack(spacing: 2) {
            Text(formatTokenCount(value))
                .font(.caption.monospacedDigit().weight(.medium))
            Text(label)
                .font(.caption2)
                .foregroundStyle(.secondary)
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(color.opacity(0.1))
        .foregroundStyle(color)
        .clipShape(RoundedRectangle(cornerRadius: 6))
    }

    private func formatTokenCount(_ count: Int) -> String {
        if count >= 1_000_000 {
            return String(format: "%.1fM", Double(count) / 1_000_000)
        } else if count >= 1_000 {
            return String(format: "%.1fK", Double(count) / 1_000)
        } else {
            return "\(count)"
        }
    }
}

// MARK: - Cache Efficiency Row

private struct CacheEfficiencyRow: View {
    let cached: Int
    let total: Int

    private var efficiency: Double {
        guard total > 0 else { return 0 }
        return Double(cached) / Double(total)
    }

    var body: some View {
        HStack {
            Text("Cache Efficiency")
                .font(.subheadline)
                .foregroundStyle(.secondary)

            Spacer()

            GlassProgressBar(progress: efficiency, tint: .orange, height: 6)
                .frame(width: 100)

            Text("\(Int(efficiency * 100))%")
                .font(.subheadline.monospacedDigit())
                .foregroundStyle(.orange)
        }
        .padding(.vertical, 8)
        .padding(.horizontal, 12)
        .background(Color.primary.opacity(0.03))
        .clipShape(RoundedRectangle(cornerRadius: 8))
    }
}

// MARK: - Budget Warning Banner

private struct BudgetWarningBanner: View {
    let remaining: Double
    let isOverBudget: Bool

    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: isOverBudget ? "exclamationmark.triangle.fill" : "exclamationmark.circle.fill")
                .font(.title2)

            VStack(alignment: .leading, spacing: 2) {
                Text(isOverBudget ? "Budget Exceeded" : "Approaching Budget Limit")
                    .font(.subheadline.weight(.semibold))

                if isOverBudget {
                    Text("Processing is paused. Increase budget to continue.")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                } else {
                    Text(String(format: "$%.2f remaining", remaining))
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }

            Spacer()

            GlassButton(title: "Adjust", icon: "slider.horizontal.3", action: {
                // TODO: Show budget adjustment sheet
            }, tint: isOverBudget ? .red : .orange)
        }
        .padding(16)
        .background(
            (isOverBudget ? Color.red : Color.orange).opacity(0.1)
        )
        .clipShape(RoundedRectangle(cornerRadius: 12))
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(isOverBudget ? Color.red.opacity(0.3) : Color.orange.opacity(0.3), lineWidth: 1)
        )
    }
}

// MARK: - Preview

#Preview {
    CostDashboardView(viewModel: .preview)
        .frame(width: 500, height: 450)
}
