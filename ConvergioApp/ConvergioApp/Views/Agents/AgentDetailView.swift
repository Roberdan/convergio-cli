/**
 * CONVERGIO NATIVE - Agent Detail View
 *
 * Expanded view showing full agent profile, capabilities,
 * and recent activity.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

struct AgentDetailView: View {
    @ObservedObject var agent: Agent
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        ScrollView {
            VStack(spacing: 24) {
                // Header with avatar
                AgentHeaderSection(agent: agent)

                Divider()

                // Capabilities
                AgentCapabilitiesSection(agent: agent)

                Divider()

                // Current status
                AgentStatusSection(agent: agent)

                // Recent activity (placeholder)
                AgentActivitySection(agent: agent)
            }
            .padding(24)
        }
        .frame(minWidth: 400, minHeight: 500)
        .background(VisualEffectBlur(material: .sidebar, blendingMode: .behindWindow))
        .toolbar {
            ToolbarItem(placement: .cancellationAction) {
                Button("Done") {
                    dismiss()
                }
            }
        }
    }
}

// MARK: - Header Section

private struct AgentHeaderSection: View {
    @ObservedObject var agent: Agent

    var body: some View {
        HStack(spacing: 20) {
            // Large avatar
            ZStack {
                Circle()
                    .fill(roleColor.gradient)
                    .frame(width: 80, height: 80)

                Image(systemName: agent.role.iconName)
                    .font(.system(size: 36))
                    .foregroundStyle(.white)
            }

            VStack(alignment: .leading, spacing: 6) {
                Text(agent.name)
                    .font(.title.weight(.bold))

                Text(agent.description)
                    .font(.body)
                    .foregroundStyle(.secondary)

                HStack(spacing: 8) {
                    GlassBadge(
                        text: agent.role.displayName,
                        icon: agent.role.iconName,
                        tint: roleColor
                    )

                    if agent.workState.isActive {
                        GlassBadge(
                            text: agent.workState.displayName,
                            icon: "bolt.fill",
                            tint: .green
                        )
                    }
                }
            }

            Spacer()
        }
    }

    private var roleColor: Color {
        switch agent.role {
        case .orchestrator: return .purple
        case .analyst: return .blue
        case .coder: return .green
        case .writer: return .orange
        case .critic: return .red
        case .planner: return .indigo
        case .executor: return .teal
        case .memory: return .mint
        }
    }
}

// MARK: - Capabilities Section

private struct AgentCapabilitiesSection: View {
    let agent: Agent

    // Define capabilities based on role
    private var capabilities: [Capability] {
        switch agent.role {
        case .orchestrator:
            return [
                Capability(name: "Task Coordination", icon: "arrow.triangle.branch", description: "Coordinates tasks across the team"),
                Capability(name: "Delegation", icon: "person.2.fill", description: "Assigns work to specialized agents"),
                Capability(name: "Synthesis", icon: "arrow.triangle.merge", description: "Combines outputs into coherent responses"),
                Capability(name: "Quality Control", icon: "checkmark.seal.fill", description: "Ensures response quality"),
            ]
        case .analyst:
            return [
                Capability(name: "Research", icon: "magnifyingglass", description: "Deep research and analysis"),
                Capability(name: "Data Analysis", icon: "chart.bar.fill", description: "Statistical analysis and insights"),
                Capability(name: "Pattern Recognition", icon: "brain", description: "Identifies trends and patterns"),
            ]
        case .coder:
            return [
                Capability(name: "Code Generation", icon: "chevron.left.forwardslash.chevron.right", description: "Writes production-ready code"),
                Capability(name: "Code Review", icon: "eye.fill", description: "Reviews and improves code quality"),
                Capability(name: "Debugging", icon: "ant.fill", description: "Identifies and fixes bugs"),
                Capability(name: "Architecture", icon: "building.2.fill", description: "Designs system architecture"),
            ]
        case .writer:
            return [
                Capability(name: "Content Creation", icon: "doc.text.fill", description: "Creates compelling content"),
                Capability(name: "Editing", icon: "pencil", description: "Refines and polishes text"),
                Capability(name: "Storytelling", icon: "book.fill", description: "Crafts engaging narratives"),
            ]
        case .critic:
            return [
                Capability(name: "Quality Review", icon: "checkmark.circle.fill", description: "Reviews work for quality"),
                Capability(name: "Fact Checking", icon: "checkmark.seal", description: "Verifies accuracy"),
                Capability(name: "Feedback", icon: "bubble.left.and.bubble.right.fill", description: "Provides constructive feedback"),
            ]
        case .planner:
            return [
                Capability(name: "Task Breakdown", icon: "list.bullet.indent", description: "Breaks complex tasks into steps"),
                Capability(name: "Prioritization", icon: "arrow.up.arrow.down", description: "Prioritizes tasks effectively"),
                Capability(name: "Timeline Planning", icon: "calendar", description: "Creates realistic timelines"),
            ]
        case .executor:
            return [
                Capability(name: "Tool Usage", icon: "wrench.and.screwdriver.fill", description: "Executes tools and actions"),
                Capability(name: "API Integration", icon: "link", description: "Integrates with external APIs"),
                Capability(name: "Automation", icon: "gearshape.2.fill", description: "Automates repetitive tasks"),
            ]
        case .memory:
            return [
                Capability(name: "Context Retrieval", icon: "tray.full.fill", description: "Retrieves relevant context"),
                Capability(name: "Knowledge Storage", icon: "externaldrive.fill", description: "Stores important information"),
                Capability(name: "Semantic Search", icon: "text.magnifyingglass", description: "Finds related information"),
            ]
        }
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Capabilities")
                .font(.headline)

            LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 12) {
                ForEach(capabilities, id: \.name) { capability in
                    CapabilityCard(capability: capability)
                }
            }
        }
    }
}

private struct Capability {
    let name: String
    let icon: String
    let description: String
}

private struct CapabilityCard: View {
    let capability: Capability

    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: capability.icon)
                .font(.title3)
                .foregroundStyle(.blue)
                .frame(width: 32)

            VStack(alignment: .leading, spacing: 2) {
                Text(capability.name)
                    .font(.subheadline.weight(.medium))

                Text(capability.description)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .lineLimit(2)
            }
        }
        .padding(12)
        .background(Color.primary.opacity(0.05))
        .clipShape(RoundedRectangle(cornerRadius: 10))
    }
}

// MARK: - Status Section

private struct AgentStatusSection: View {
    @ObservedObject var agent: Agent

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Current Status")
                .font(.headline)

            HStack(spacing: 16) {
                AgentStatusIndicator(
                    label: "State",
                    value: agent.workState.displayName,
                    icon: stateIcon,
                    color: stateColor
                )

                if let task = agent.currentTask {
                    AgentStatusIndicator(
                        label: "Current Task",
                        value: task,
                        icon: "doc.text.fill",
                        color: .blue
                    )
                }
            }
        }
    }

    private var stateIcon: String {
        switch agent.workState {
        case .idle: return "moon.fill"
        case .thinking: return "brain"
        case .executing: return "bolt.fill"
        case .waiting: return "hourglass"
        case .communicating: return "bubble.left.and.bubble.right.fill"
        }
    }

    private var stateColor: Color {
        switch agent.workState {
        case .idle: return .gray
        case .thinking: return .blue
        case .executing: return .orange
        case .waiting: return .yellow
        case .communicating: return .purple
        }
    }
}

private struct AgentStatusIndicator: View {
    let label: String
    let value: String
    let icon: String
    let color: Color

    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(label)
                .font(.caption)
                .foregroundStyle(.secondary)

            HStack(spacing: 8) {
                Image(systemName: icon)
                    .foregroundStyle(color)
                Text(value)
                    .font(.subheadline.weight(.medium))
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(color.opacity(0.1))
            .clipShape(RoundedRectangle(cornerRadius: 8))
        }
    }
}

// MARK: - Activity Section

private struct AgentActivitySection: View {
    let agent: Agent

    // Placeholder activity data
    private let recentActivity: [ActivityItem] = [
        ActivityItem(action: "Completed analysis", time: "2 min ago", icon: "checkmark.circle.fill"),
        ActivityItem(action: "Started new task", time: "5 min ago", icon: "play.circle.fill"),
        ActivityItem(action: "Collaborated with Amy", time: "8 min ago", icon: "person.2.fill"),
    ]

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Recent Activity")
                .font(.headline)

            ForEach(recentActivity, id: \.action) { item in
                HStack(spacing: 12) {
                    Image(systemName: item.icon)
                        .foregroundStyle(.secondary)
                        .frame(width: 24)

                    Text(item.action)
                        .font(.subheadline)

                    Spacer()

                    Text(item.time)
                        .font(.caption)
                        .foregroundStyle(.tertiary)
                }
                .padding(.vertical, 6)

                if item.action != recentActivity.last?.action {
                    Divider()
                }
            }
        }
        .padding(16)
        .background(Color.primary.opacity(0.03))
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

private struct ActivityItem {
    let action: String
    let time: String
    let icon: String
}

// MARK: - Preview

#Preview {
    AgentDetailView(agent: Agent.preview)
}
