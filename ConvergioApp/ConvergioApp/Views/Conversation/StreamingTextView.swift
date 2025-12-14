/**
 * CONVERGIO NATIVE - Streaming Text View
 *
 * Displays streaming text with real-time updates and typing animation.
 * Renders Markdown to AttributedString for rich text display.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

// MARK: - Streaming Text View

struct StreamingTextView: View {
    let text: String
    let isComplete: Bool

    @State private var displayedText = ""
    @State private var cursorVisible = true

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Rendered markdown text
            if let attributed = try? AttributedString(markdown: displayedText) {
                Text(attributed)
                    .textSelection(.enabled)
            } else {
                Text(displayedText)
                    .textSelection(.enabled)
            }

            // Typing cursor
            if !isComplete {
                HStack(spacing: 0) {
                    Text(" ")
                    Rectangle()
                        .fill(Color.primary)
                        .frame(width: 2, height: 16)
                        .opacity(cursorVisible ? 1 : 0)
                }
            }
        }
        .onChange(of: text) { _, newValue in
            displayedText = newValue
        }
        .onAppear {
            displayedText = text
            startCursorBlink()
        }
    }

    private func startCursorBlink() {
        guard !isComplete else { return }

        withAnimation(.easeInOut(duration: 0.5).repeatForever(autoreverses: true)) {
            cursorVisible.toggle()
        }
    }
}

// MARK: - Streaming Message Bubble

struct StreamingMessageBubble: View {
    @ObservedObject var viewModel: ConversationViewModel
    let agentName: String?

    var body: some View {
        HStack(alignment: .top, spacing: 12) {
            // Agent avatar
            if let name = agentName {
                AgentAvatarSmall(name: name)
            } else {
                Image(systemName: "sparkles")
                    .font(.title3)
                    .foregroundStyle(.purple)
                    .frame(width: 32, height: 32)
                    .background(Color.purple.opacity(0.1))
                    .clipShape(Circle())
            }

            VStack(alignment: .leading, spacing: 8) {
                // Agent name or "AI Team"
                HStack {
                    Text(agentName ?? "AI Team")
                        .font(.caption.weight(.semibold))
                        .foregroundStyle(.secondary)

                    if viewModel.isStreaming {
                        ProgressView()
                            .scaleEffect(0.5)
                            .frame(width: 12, height: 12)
                    }
                }

                // Streaming text
                StreamingTextView(
                    text: viewModel.streamingText,
                    isComplete: !viewModel.isStreaming
                )
                .font(.body)
            }

            Spacer()
        }
        .padding(12)
        .background(Color.primary.opacity(0.03))
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

// MARK: - Agent Avatar Small

struct AgentAvatarSmall: View {
    let name: String

    private var initials: String {
        String(name.prefix(1)).uppercased()
    }

    private var color: Color {
        // Simple hash-based color selection
        let colors: [Color] = [.blue, .purple, .green, .orange, .pink, .teal]
        let hash = name.unicodeScalars.reduce(0) { $0 + Int($1.value) }
        return colors[hash % colors.count]
    }

    var body: some View {
        Text(initials)
            .font(.caption.weight(.bold))
            .foregroundStyle(.white)
            .frame(width: 32, height: 32)
            .background(color.gradient)
            .clipShape(Circle())
    }
}

// MARK: - Thinking Indicator

struct ThinkingIndicator: View {
    let agentNames: [String]

    @State private var dotIndex = 0

    var body: some View {
        HStack(spacing: 12) {
            // Agent avatars (overlapping)
            HStack(spacing: -8) {
                ForEach(agentNames.prefix(3), id: \.self) { name in
                    AgentAvatarSmall(name: name)
                        .overlay(
                            Circle()
                                .stroke(Color(nsColor: .windowBackgroundColor), lineWidth: 2)
                        )
                }

                if agentNames.count > 3 {
                    Text("+\(agentNames.count - 3)")
                        .font(.caption2.weight(.bold))
                        .foregroundStyle(.white)
                        .frame(width: 32, height: 32)
                        .background(Color.gray.gradient)
                        .clipShape(Circle())
                        .overlay(
                            Circle()
                                .stroke(Color(nsColor: .windowBackgroundColor), lineWidth: 2)
                        )
                }
            }

            VStack(alignment: .leading, spacing: 4) {
                Text(thinkingText)
                    .font(.subheadline.weight(.medium))

                // Animated dots
                HStack(spacing: 4) {
                    ForEach(0..<3) { index in
                        Circle()
                            .fill(Color.purple)
                            .frame(width: 6, height: 6)
                            .scaleEffect(dotIndex == index ? 1.2 : 0.8)
                            .opacity(dotIndex == index ? 1.0 : 0.4)
                    }
                }
            }
        }
        .padding(12)
        .background(Color.purple.opacity(0.05))
        .clipShape(RoundedRectangle(cornerRadius: 12))
        .onAppear {
            startAnimation()
        }
    }

    private var thinkingText: String {
        if agentNames.count == 1 {
            return "\(agentNames[0]) is thinking..."
        } else if agentNames.count == 2 {
            return "\(agentNames[0]) and \(agentNames[1]) are collaborating..."
        } else {
            return "\(agentNames.count) agents are working..."
        }
    }

    private func startAnimation() {
        Timer.scheduledTimer(withTimeInterval: 0.3, repeats: true) { _ in
            withAnimation(.easeInOut(duration: 0.2)) {
                dotIndex = (dotIndex + 1) % 3
            }
        }
    }
}

// MARK: - Convergence Indicator

struct ConvergenceIndicator: View {
    let progress: Double
    let participantCount: Int

    var body: some View {
        HStack(spacing: 16) {
            // Convergence icon
            ZStack {
                Circle()
                    .stroke(Color.purple.opacity(0.2), lineWidth: 3)
                    .frame(width: 44, height: 44)

                Circle()
                    .trim(from: 0, to: CGFloat(progress))
                    .stroke(Color.purple, style: StrokeStyle(lineWidth: 3, lineCap: .round))
                    .frame(width: 44, height: 44)
                    .rotationEffect(.degrees(-90))

                Image(systemName: "arrow.triangle.merge")
                    .font(.body.weight(.semibold))
                    .foregroundStyle(.purple)
            }

            VStack(alignment: .leading, spacing: 4) {
                Text("Converging responses...")
                    .font(.subheadline.weight(.medium))

                Text("\(participantCount) agents contributing")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            Text("\(Int(progress * 100))%")
                .font(.headline.monospacedDigit())
                .foregroundStyle(.purple)
        }
        .padding(12)
        .background(
            LinearGradient(
                colors: [Color.purple.opacity(0.1), Color.purple.opacity(0.05)],
                startPoint: .leading,
                endPoint: .trailing
            )
        )
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

// MARK: - Preview

#Preview("Streaming Text") {
    VStack(spacing: 20) {
        StreamingTextView(
            text: "This is **streaming** text with *markdown* support...",
            isComplete: false
        )

        ThinkingIndicator(agentNames: ["Ali", "Angela", "Amy"])

        ConvergenceIndicator(progress: 0.65, participantCount: 4)
    }
    .padding()
    .frame(width: 400)
}
