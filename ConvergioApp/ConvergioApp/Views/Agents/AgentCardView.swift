/**
 * CONVERGIO NATIVE - Agent Card View
 *
 * Liquid Glass styled agent card with state visualization.
 * Shows agent status, role, and interactive animations.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

struct AgentCardView: View {
    @ObservedObject var agent: Agent
    @State private var isHovered = false
    @State private var isPulsing = false

    @Namespace private var namespace

    var body: some View {
        VStack(spacing: 8) {
            // Avatar with status ring
            ZStack {
                // Background glow for active states
                if agent.workState.isActive {
                    Circle()
                        .fill(stateColor.opacity(0.3))
                        .frame(width: 80, height: 80)
                        .blur(radius: 10)
                        .scaleEffect(isPulsing ? 1.2 : 1.0)
                }

                // Status ring
                Circle()
                    .stroke(stateColor, lineWidth: 3)
                    .frame(width: 60, height: 60)

                // Avatar icon
                Image(systemName: agent.role.iconName)
                    .font(.system(size: 28))
                    .foregroundStyle(.primary)
            }
            .frame(width: 70, height: 70)

            // Name
            Text(agent.name)
                .font(.headline)
                .lineLimit(1)

            // Role or status
            Text(agent.workState.isActive ? agent.workState.displayName : agent.role.displayName)
                .font(.caption)
                .foregroundStyle(.secondary)
                .lineLimit(1)

            // Current task (if any)
            if let task = agent.currentTask {
                Text(task)
                    .font(.caption2)
                    .foregroundStyle(.tertiary)
                    .lineLimit(2)
                    .multilineTextAlignment(.center)
            }
        }
        .padding()
        .frame(width: 140, height: 160)
        .background(cardBackground)
        .clipShape(RoundedRectangle(cornerRadius: 16))
        .overlay(
            RoundedRectangle(cornerRadius: 16)
                .stroke(isHovered ? stateColor.opacity(0.5) : .clear, lineWidth: 2)
        )
        .scaleEffect(isHovered ? 1.05 : 1.0)
        .animation(.spring(duration: 0.3), value: isHovered)
        .onHover { hovering in
            isHovered = hovering
        }
        .onAppear {
            if agent.workState.isActive {
                startPulsing()
            }
        }
        .onChange(of: agent.workState) { _, newState in
            if newState.isActive {
                startPulsing()
            } else {
                isPulsing = false
            }
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

    @ViewBuilder
    private var cardBackground: some View {
        if agent.workState.isActive {
            LinearGradient(
                colors: [
                    stateColor.opacity(0.15),
                    stateColor.opacity(0.05)
                ],
                startPoint: .top,
                endPoint: .bottom
            )
        } else {
            Color.secondary.opacity(0.1)
        }
    }

    private func startPulsing() {
        withAnimation(
            .easeInOut(duration: 1.5)
            .repeatForever(autoreverses: true)
        ) {
            isPulsing = true
        }
    }
}

// MARK: - Liquid Glass Version (for macOS 26+)

@available(macOS 26.0, *)
struct LiquidGlassAgentCard: View {
    @ObservedObject var agent: Agent
    @Namespace private var namespace

    var body: some View {
        VStack(spacing: 8) {
            // Avatar with glass effect
            ZStack {
                Circle()
                    .stroke(stateColor, lineWidth: 3)
                    .frame(width: 60, height: 60)

                Image(systemName: agent.role.iconName)
                    .font(.system(size: 28))
                    .foregroundStyle(.white)
            }
            // .glassEffect(.regular.tint(stateColor.opacity(0.3)), in: .circle)
            // .glassEffectID("avatar-\(agent.id)", in: namespace)
            .frame(width: 70, height: 70)

            Text(agent.name)
                .font(.headline)

            Text(agent.workState.isActive ? agent.workState.displayName : agent.role.displayName)
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .padding()
        .frame(width: 140, height: 160)
        // .glassEffect(.regular.tint(stateColor.opacity(0.2)).interactive())
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

// MARK: - Convergence View

struct ConvergenceView: View {
    let activeAgents: [Agent]
    let progress: Double

    @Namespace private var convergenceNamespace

    var body: some View {
        ZStack {
            // Agent bubbles arranged in a circle
            ForEach(Array(activeAgents.enumerated()), id: \.element.id) { index, agent in
                AgentBubble(agent: agent)
                    .offset(bubbleOffset(index: index, total: activeAgents.count))
                    .opacity(1.0 - (progress * 0.3))
            }

            // Central convergence point
            if progress > 0.5 {
                VStack(spacing: 4) {
                    Image(systemName: "arrow.triangle.merge")
                        .font(.title)
                        .foregroundStyle(.white)

                    Text("Converging...")
                        .font(.caption)
                        .foregroundStyle(.white)
                }
                .padding()
                .background(.purple.gradient)
                .clipShape(Circle())
                .scaleEffect(progress)
                .transition(.scale.combined(with: .opacity))
            }
        }
        .frame(width: 300, height: 300)
        .animation(.spring(duration: 0.6), value: progress)
    }

    private func bubbleOffset(index: Int, total: Int) -> CGSize {
        let angle = (Double(index) / Double(total)) * 2 * .pi - .pi / 2
        let radius = 100.0 * (1 - progress * 0.7)
        return CGSize(
            width: cos(angle) * radius,
            height: sin(angle) * radius
        )
    }
}

// MARK: - Agent Bubble

struct AgentBubble: View {
    @ObservedObject var agent: Agent

    var body: some View {
        VStack(spacing: 4) {
            Image(systemName: agent.role.iconName)
                .font(.title2)
                .foregroundStyle(.white)

            Text(agent.name)
                .font(.caption2)
                .foregroundStyle(.white)
        }
        .padding(8)
        .background(bubbleColor.gradient)
        .clipShape(Circle())
    }

    private var bubbleColor: Color {
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

// MARK: - Preview

#Preview {
    VStack(spacing: 20) {
        HStack(spacing: 20) {
            ForEach(Agent.previewAgents.prefix(3)) { agent in
                AgentCardView(agent: agent)
            }
        }

        ConvergenceView(
            activeAgents: Array(Agent.previewAgents.prefix(4)),
            progress: 0.7
        )
    }
    .padding()
}
