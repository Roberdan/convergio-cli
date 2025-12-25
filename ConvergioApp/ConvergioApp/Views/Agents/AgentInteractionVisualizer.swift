/**
 * CONVERGIO NATIVE - Agent Interaction Visualizer
 *
 * A futuristic real-time visualization of AI agent collaboration.
 * Shows agent states, communication flows, and convergence progress.
 *
 * Design: Inspired by neural network visualizations and sci-fi interfaces.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

// MARK: - Agent Interaction Visualizer

struct AgentInteractionVisualizer: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel
    @State private var animationPhase: Double = 0

    // Timer for continuous animation
    let timer = Timer.publish(every: 0.05, on: .main, in: .common).autoconnect()

    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Background gradient
                RadialGradient(
                    colors: [
                        Color.purple.opacity(0.1),
                        Color.blue.opacity(0.05),
                        Color.black.opacity(0.02)
                    ],
                    center: .center,
                    startRadius: 0,
                    endRadius: geometry.size.width / 2
                )

                // Connection lines between agents
                AgentConnectionsView(
                    agents: orchestratorVM.agents,
                    activeAgents: orchestratorVM.activeAgents,
                    size: geometry.size,
                    animationPhase: animationPhase
                )

                // Agent nodes in circular layout
                AgentNodesView(
                    agents: orchestratorVM.agents,
                    activeAgents: orchestratorVM.activeAgents,
                    size: geometry.size,
                    animationPhase: animationPhase
                )

                // Central orchestrator hub
                CentralHubView(
                    isProcessing: orchestratorVM.isLoading,
                    animationPhase: animationPhase
                )
                .position(x: geometry.size.width / 2, y: geometry.size.height / 2)

                // Status overlay
                VStack {
                    HStack {
                        AgentStatusLegend()
                        Spacer()
                        ProcessingIndicator(isActive: orchestratorVM.isLoading)
                    }
                    .padding()

                    Spacer()

                    // Active agents ticker
                    if !orchestratorVM.activeAgents.isEmpty {
                        ActiveAgentsTicker(agents: orchestratorVM.activeAgents)
                            .padding()
                    }
                }
            }
        }
        .onReceive(timer) { _ in
            animationPhase += 0.02
            if animationPhase > .pi * 2 {
                animationPhase = 0
            }
        }
    }
}

// MARK: - Agent Connections View

struct AgentConnectionsView: View {
    let agents: [Agent]
    let activeAgents: [Agent]
    let size: CGSize
    let animationPhase: Double

    var body: some View {
        Canvas { context, canvasSize in
            let center = CGPoint(x: canvasSize.width / 2, y: canvasSize.height / 2)
            let radius = min(canvasSize.width, canvasSize.height) * 0.35

            // Draw connections from center to each agent
            for (index, agent) in agents.enumerated() {
                let angle = angleFor(index: index, total: agents.count)
                let agentPos = positionOnCircle(center: center, radius: radius, angle: angle)

                // Connection line
                var path = Path()
                path.move(to: center)
                path.addLine(to: agentPos)

                let isActive = activeAgents.contains(agent)
                let opacity = isActive ? 0.6 : 0.15

                // Animated pulse effect for active connections
                if isActive {
                    let pulseOffset = (animationPhase + Double(index) * 0.3).truncatingRemainder(dividingBy: 1.0)
                    let pulsePoint = interpolate(from: center, to: agentPos, t: pulseOffset)

                    // Draw pulse
                    context.fill(
                        Circle().path(in: CGRect(x: pulsePoint.x - 4, y: pulsePoint.y - 4, width: 8, height: 8)),
                        with: .color(agent.role.color.opacity(0.8))
                    )
                }

                context.stroke(
                    path,
                    with: .color(isActive ? agent.role.color.opacity(opacity) : .gray.opacity(opacity)),
                    lineWidth: isActive ? 2 : 1
                )
            }

            // Draw inter-agent connections for active agents
            let activeIndices = activeAgents.compactMap { agent in
                agents.firstIndex(of: agent)
            }

            for i in 0..<activeIndices.count {
                for j in (i + 1)..<activeIndices.count {
                    let idx1 = activeIndices[i]
                    let idx2 = activeIndices[j]

                    let angle1 = angleFor(index: idx1, total: agents.count)
                    let angle2 = angleFor(index: idx2, total: agents.count)

                    let pos1 = positionOnCircle(center: center, radius: radius, angle: angle1)
                    let pos2 = positionOnCircle(center: center, radius: radius, angle: angle2)

                    var path = Path()
                    path.move(to: pos1)

                    // Curved connection through center
                    let midPoint = CGPoint(
                        x: center.x + (pos1.x - center.x + pos2.x - center.x) * 0.3,
                        y: center.y + (pos1.y - center.y + pos2.y - center.y) * 0.3
                    )
                    path.addQuadCurve(to: pos2, control: midPoint)

                    context.stroke(
                        path,
                        with: .color(.purple.opacity(0.3)),
                        style: StrokeStyle(lineWidth: 1, dash: [4, 4])
                    )
                }
            }
        }
    }

    private func angleFor(index: Int, total: Int) -> Double {
        return (Double(index) / Double(total)) * .pi * 2 - .pi / 2
    }

    private func positionOnCircle(center: CGPoint, radius: CGFloat, angle: Double) -> CGPoint {
        CGPoint(
            x: center.x + CGFloat(cos(angle)) * radius,
            y: center.y + CGFloat(sin(angle)) * radius
        )
    }

    private func interpolate(from: CGPoint, to: CGPoint, t: Double) -> CGPoint {
        CGPoint(
            x: from.x + (to.x - from.x) * CGFloat(t),
            y: from.y + (to.y - from.y) * CGFloat(t)
        )
    }
}

// MARK: - Agent Nodes View

struct AgentNodesView: View {
    let agents: [Agent]
    let activeAgents: [Agent]
    let size: CGSize
    let animationPhase: Double

    var body: some View {
        let center = CGPoint(x: size.width / 2, y: size.height / 2)
        let radius = min(size.width, size.height) * 0.35

        ForEach(Array(agents.enumerated()), id: \.element.id) { index, agent in
            let angle = angleFor(index: index, total: agents.count)
            let position = positionOnCircle(center: center, radius: radius, angle: angle)
            let isActive = activeAgents.contains(agent)

            AgentNodeView(
                agent: agent,
                isActive: isActive,
                animationPhase: animationPhase
            )
            .position(position)
        }
    }

    private func angleFor(index: Int, total: Int) -> Double {
        return (Double(index) / Double(total)) * .pi * 2 - .pi / 2
    }

    private func positionOnCircle(center: CGPoint, radius: CGFloat, angle: Double) -> CGPoint {
        CGPoint(
            x: center.x + CGFloat(cos(angle)) * radius,
            y: center.y + CGFloat(sin(angle)) * radius
        )
    }
}

// MARK: - Agent Node View

struct AgentNodeView: View {
    let agent: Agent
    let isActive: Bool
    let animationPhase: Double

    @State private var isHovering = false

    var body: some View {
        ZStack {
            // Outer glow for active agents
            if isActive {
                Circle()
                    .fill(agent.role.color.opacity(0.3))
                    .frame(width: 70, height: 70)
                    .scaleEffect(1.0 + sin(animationPhase * 3) * 0.1)
                    .blur(radius: 10)
            }

            // Background circle
            Circle()
                .fill(
                    RadialGradient(
                        colors: [
                            agent.role.color.opacity(isActive ? 0.4 : 0.2),
                            agent.role.color.opacity(isActive ? 0.2 : 0.05)
                        ],
                        center: .center,
                        startRadius: 0,
                        endRadius: 30
                    )
                )
                .frame(width: 60, height: 60)

            // Border
            Circle()
                .stroke(
                    agent.role.color.opacity(isActive ? 0.8 : 0.3),
                    lineWidth: isActive ? 2 : 1
                )
                .frame(width: 60, height: 60)

            // Icon
            VStack(spacing: 2) {
                Image(systemName: agent.role.iconName)
                    .font(.system(size: 20))
                    .foregroundStyle(isActive ? .white : .secondary)
                    .symbolEffect(.pulse, isActive: isActive)

                Text(agent.name)
                    .font(.caption2)
                    .fontWeight(.medium)
                    .foregroundStyle(isActive ? .white : .secondary)
            }

            // Status indicator
            if isActive {
                Circle()
                    .fill(.green)
                    .frame(width: 10, height: 10)
                    .overlay(
                        Circle()
                            .stroke(.white, lineWidth: 1)
                    )
                    .offset(x: 22, y: -22)
                    .transition(.scale)
            }
        }
        .scaleEffect(isHovering ? 1.1 : 1.0)
        .animation(.easeInOut(duration: 0.2), value: isHovering)
        .onHover { hovering in
            isHovering = hovering
        }
        .help("\(agent.name) - \(agent.role.displayName)\n\(isActive ? "Currently working" : "Idle")")
    }
}

// MARK: - Central Hub View

struct CentralHubView: View {
    let isProcessing: Bool
    let animationPhase: Double

    var body: some View {
        ZStack {
            // Outer rotating ring
            if isProcessing {
                Circle()
                    .trim(from: 0, to: 0.7)
                    .stroke(
                        LinearGradient(
                            colors: [.purple, .blue, .purple.opacity(0)],
                            startPoint: .leading,
                            endPoint: .trailing
                        ),
                        style: StrokeStyle(lineWidth: 3, lineCap: .round)
                    )
                    .frame(width: 100, height: 100)
                    .rotationEffect(.radians(animationPhase * 2))
            }

            // Inner glow
            Circle()
                .fill(
                    RadialGradient(
                        colors: [
                            Color.purple.opacity(isProcessing ? 0.6 : 0.3),
                            Color.blue.opacity(isProcessing ? 0.3 : 0.1),
                            Color.clear
                        ],
                        center: .center,
                        startRadius: 10,
                        endRadius: 50
                    )
                )
                .frame(width: 80, height: 80)
                .scaleEffect(isProcessing ? 1.0 + sin(animationPhase * 4) * 0.1 : 1.0)

            // Core
            Circle()
                .fill(
                    LinearGradient(
                        colors: [.purple, .blue],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .frame(width: 50, height: 50)

            // Icon
            Image(systemName: "brain.head.profile")
                .font(.system(size: 24))
                .foregroundStyle(.white)
                .symbolEffect(.pulse, isActive: isProcessing)

            // Status text
            if isProcessing {
                Text("Converging")
                    .font(.caption2)
                    .fontWeight(.semibold)
                    .foregroundStyle(.purple)
                    .offset(y: 50)
            }
        }
    }
}

// MARK: - Agent Status Legend

struct AgentStatusLegend: View {
    var body: some View {
        HStack(spacing: 16) {
            LegendItem(color: .green, label: "Active")
            LegendItem(color: .gray, label: "Idle")
            LegendItem(color: .purple, label: "Converging")
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(.ultraThinMaterial)
        .clipShape(Capsule())
    }
}

struct LegendItem: View {
    let color: Color
    let label: String

    var body: some View {
        HStack(spacing: 4) {
            Circle()
                .fill(color)
                .frame(width: 8, height: 8)
            Text(label)
                .font(.caption2)
                .foregroundStyle(.secondary)
        }
    }
}

// MARK: - Processing Indicator

struct ProcessingIndicator: View {
    let isActive: Bool

    var body: some View {
        HStack(spacing: 6) {
            Circle()
                .fill(isActive ? .green : .gray)
                .frame(width: 8, height: 8)
                .overlay(
                    Circle()
                        .stroke(isActive ? .green.opacity(0.5) : .clear, lineWidth: 4)
                        .scaleEffect(isActive ? 2 : 1)
                        .opacity(isActive ? 0 : 1)
                        .animation(.easeOut(duration: 1).repeatForever(autoreverses: false), value: isActive)
                )

            Text(isActive ? "Processing" : "Ready")
                .font(.caption)
                .fontWeight(.medium)
                .foregroundStyle(isActive ? .green : .secondary)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 6)
        .background(.ultraThinMaterial)
        .clipShape(Capsule())
    }
}

// MARK: - Active Agents Ticker

struct ActiveAgentsTicker: View {
    let agents: [Agent]

    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: "waveform")
                .foregroundStyle(.purple)
                .symbolEffect(.variableColor.iterative)

            Text("Active:")
                .font(.caption)
                .foregroundStyle(.secondary)

            ForEach(agents) { agent in
                HStack(spacing: 4) {
                    Image(systemName: agent.role.iconName)
                        .font(.caption)
                    Text(agent.name)
                        .font(.caption)
                        .fontWeight(.medium)
                }
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(agent.role.color.opacity(0.2))
                .clipShape(Capsule())
            }
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 10)
        .background(.ultraThinMaterial)
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

// MARK: - Agent Role Extension for Colors

extension AgentRole {
    var color: Color {
        switch self {
        case .orchestrator: return .purple
        case .analyst: return .blue
        case .coder: return .green
        case .writer: return .orange
        case .critic: return .red
        case .planner: return .indigo
        case .executor: return .cyan
        case .memory: return .mint
        }
    }
}

// MARK: - Preview

#Preview("Agent Visualizer") {
    AgentInteractionVisualizer()
        .environmentObject(OrchestratorViewModel.preview)
        .frame(width: 800, height: 600)
        .background(Color(nsColor: .windowBackgroundColor))
}
