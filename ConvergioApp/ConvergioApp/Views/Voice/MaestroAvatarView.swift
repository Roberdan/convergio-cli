/**
 * CONVERGIO NATIVE - MaestroAvatarView
 *
 * Animated avatar display for the 17 historical maestros.
 * Shows speaking animations, emotion indicators, and status.
 *
 * Part of the Scuola 2026 Voice System (Task 0.3.7)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

// MARK: - Student Emotion

enum StudentEmotion: String, CaseIterable {
    case neutral
    case curious
    case confused
    case frustrated
    case happy
    case focused

    var emoji: String {
        switch self {
        case .neutral: return "😐"
        case .curious: return "🤔"
        case .confused: return "😕"
        case .frustrated: return "😤"
        case .happy: return "😊"
        case .focused: return "🧐"
        }
    }

    var color: Color {
        switch self {
        case .neutral: return .gray
        case .curious: return .blue
        case .confused: return .orange
        case .frustrated: return .red
        case .happy: return .green
        case .focused: return .purple
        }
    }
}

// MARK: - Speaking State

enum MaestroSpeakingState {
    case idle
    case listening
    case thinking
    case speaking
    case paused

    var pulseScale: CGFloat {
        switch self {
        case .idle: return 1.0
        case .listening: return 1.05
        case .thinking: return 1.02
        case .speaking: return 1.08
        case .paused: return 1.0
        }
    }

    var glowOpacity: Double {
        switch self {
        case .idle: return 0
        case .listening: return 0.3
        case .thinking: return 0.4
        case .speaking: return 0.6
        case .paused: return 0.1
        }
    }

    var animationDuration: Double {
        switch self {
        case .idle: return 0
        case .listening: return 0.8
        case .thinking: return 1.2
        case .speaking: return 0.4
        case .paused: return 0
        }
    }

    var statusText: String {
        switch self {
        case .idle: return "Ready"
        case .listening: return "Listening..."
        case .thinking: return "Thinking..."
        case .speaking: return "Speaking"
        case .paused: return "Paused"
        }
    }
}

// MARK: - Avatar Style

enum MaestroAvatarStyle {
    case small   // 40pt - for lists
    case medium  // 80pt - for cards
    case large   // 120pt - for voice session
    case hero    // 200pt - for detail view

    var size: CGFloat {
        switch self {
        case .small: return 40
        case .medium: return 80
        case .large: return 120
        case .hero: return 200
        }
    }

    var borderWidth: CGFloat {
        switch self {
        case .small: return 2
        case .medium: return 3
        case .large: return 4
        case .hero: return 5
        }
    }

    var fontSize: CGFloat {
        switch self {
        case .small: return 16
        case .medium: return 32
        case .large: return 48
        case .hero: return 80
        }
    }

    var showName: Bool {
        switch self {
        case .small, .medium: return false
        case .large, .hero: return true
        }
    }
}

// MARK: - MaestroAvatarView

struct MaestroAvatarView: View {
    let maestro: Maestro
    let style: MaestroAvatarStyle
    var speakingState: MaestroSpeakingState = .idle
    var emotion: StudentEmotion = .neutral
    var showStatus: Bool = false

    @State private var isPulsing = false
    @State private var rotationAngle: Double = 0

    var body: some View {
        VStack(spacing: 8) {
            ZStack {
                // Glow effect
                if speakingState != .idle {
                    Circle()
                        .fill(maestro.color.opacity(speakingState.glowOpacity))
                        .frame(width: style.size * 1.4, height: style.size * 1.4)
                        .blur(radius: style.size * 0.2)
                        .scaleEffect(isPulsing ? 1.1 : 1.0)
                }

                // Thinking ring animation
                if speakingState == .thinking {
                    Circle()
                        .stroke(
                            AngularGradient(
                                gradient: Gradient(colors: [
                                    maestro.color.opacity(0),
                                    maestro.color.opacity(0.5),
                                    maestro.color
                                ]),
                                center: .center
                            ),
                            lineWidth: style.borderWidth
                        )
                        .frame(width: style.size + 10, height: style.size + 10)
                        .rotationEffect(.degrees(rotationAngle))
                }

                // Avatar circle
                Circle()
                    .fill(avatarGradient)
                    .frame(width: style.size, height: style.size)
                    .overlay(
                        Circle()
                            .strokeBorder(maestro.color, lineWidth: style.borderWidth)
                    )
                    .scaleEffect(isPulsing ? speakingState.pulseScale : 1.0)

                // Icon/Initial
                avatarContent

                // Speaking indicator
                if speakingState == .speaking {
                    speakingWaves
                }

                // Emotion badge (for larger styles)
                if style != .small && emotion != .neutral {
                    emotionBadge
                        .offset(x: style.size * 0.35, y: style.size * 0.35)
                }
            }
            .frame(width: style.size * 1.5, height: style.size * 1.5)

            // Name and status
            if style.showName {
                VStack(spacing: 2) {
                    Text(maestro.name)
                        .font(.system(size: style.size * 0.14, weight: .semibold))
                        .foregroundColor(.primary)

                    if showStatus {
                        Text(speakingState.statusText)
                            .font(.system(size: style.size * 0.1))
                            .foregroundColor(.secondary)
                    }
                }
            }
        }
        .animation(.easeInOut(duration: speakingState.animationDuration).repeatForever(autoreverses: true), value: isPulsing)
        .onAppear { startAnimations() }
        .onChange(of: speakingState) { startAnimations() }
    }

    // MARK: - Subviews

    private var avatarGradient: LinearGradient {
        LinearGradient(
            colors: [
                maestro.color.opacity(0.2),
                maestro.color.opacity(0.4)
            ],
            startPoint: .topLeading,
            endPoint: .bottomTrailing
        )
    }

    @ViewBuilder
    private var avatarContent: some View {
        // Try to load image, fallback to initial
        if !maestro.avatarName.isEmpty, let _ = NSImage(named: maestro.avatarName) {
            Image(maestro.avatarName)
                .resizable()
                .aspectRatio(contentMode: .fill)
                .frame(width: style.size * 0.9, height: style.size * 0.9)
                .clipShape(Circle())
        } else {
            // Fallback: show subject icon or initial
            Text(maestro.icon)
                .font(.system(size: style.fontSize))
        }
    }

    private var speakingWaves: some View {
        ZStack {
            ForEach(0..<3, id: \.self) { index in
                Circle()
                    .stroke(maestro.color.opacity(0.3 - Double(index) * 0.1), lineWidth: 2)
                    .frame(
                        width: style.size + CGFloat(index + 1) * 15,
                        height: style.size + CGFloat(index + 1) * 15
                    )
                    .scaleEffect(isPulsing ? 1.2 : 1.0)
                    .animation(
                        .easeOut(duration: 0.6)
                            .repeatForever(autoreverses: false)
                            .delay(Double(index) * 0.2),
                        value: isPulsing
                    )
            }
        }
    }

    private var emotionBadge: some View {
        Circle()
            .fill(emotion.color)
            .frame(width: style.size * 0.25, height: style.size * 0.25)
            .overlay(
                Text(emotion.emoji)
                    .font(.system(size: style.size * 0.12))
            )
            .overlay(
                Circle()
                    .stroke(Color.white, lineWidth: 2)
            )
            .shadow(radius: 2)
    }

    // MARK: - Animations

    private func startAnimations() {
        isPulsing = speakingState != .idle

        if speakingState == .thinking {
            withAnimation(.linear(duration: 2).repeatForever(autoreverses: false)) {
                rotationAngle = 360
            }
        } else {
            rotationAngle = 0
        }
    }
}

// MARK: - Compact Voice Avatar (for inline use)

struct CompactMaestroAvatar: View {
    let maestro: Maestro
    var isActive: Bool = false
    var size: CGFloat = 32

    var body: some View {
        ZStack {
            Circle()
                .fill(maestro.color.opacity(isActive ? 0.3 : 0.1))
                .frame(width: size, height: size)

            Text(maestro.icon)
                .font(.system(size: size * 0.5))
        }
        .overlay(
            Circle()
                .strokeBorder(
                    isActive ? maestro.color : Color.gray.opacity(0.3),
                    lineWidth: 2
                )
        )
    }
}

// MARK: - Animated Speaking Avatar

struct AnimatedSpeakingAvatar: View {
    let maestro: Maestro
    @Binding var audioLevel: Float
    var size: CGFloat = 120

    @State private var wavePhase: CGFloat = 0

    var body: some View {
        ZStack {
            // Audio-reactive rings
            ForEach(0..<4, id: \.self) { index in
                Circle()
                    .stroke(
                        maestro.color.opacity(0.2 - Double(index) * 0.05),
                        lineWidth: 3
                    )
                    .frame(
                        width: size + CGFloat(index) * 20 * CGFloat(1 + audioLevel),
                        height: size + CGFloat(index) * 20 * CGFloat(1 + audioLevel)
                    )
                    .scaleEffect(1 + CGFloat(audioLevel) * 0.1)
            }

            // Main avatar
            MaestroAvatarView(
                maestro: maestro,
                style: .large,
                speakingState: audioLevel > 0.1 ? .speaking : .idle
            )
        }
        .animation(.easeOut(duration: 0.1), value: audioLevel)
    }
}

// MARK: - Maestro Selection Strip

struct MaestroSelectionStrip: View {
    let maestros: [Maestro]
    @Binding var selectedMaestro: Maestro?

    var body: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 12) {
                ForEach(maestros) { maestro in
                    Button {
                        selectedMaestro = maestro
                    } label: {
                        VStack(spacing: 4) {
                            MaestroAvatarView(
                                maestro: maestro,
                                style: .small,
                                speakingState: selectedMaestro?.id == maestro.id ? .listening : .idle
                            )

                            Text(maestro.name)
                                .font(.caption2)
                                .foregroundColor(
                                    selectedMaestro?.id == maestro.id ? .primary : .secondary
                                )
                        }
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(.horizontal)
        }
    }
}

// MARK: - Previews

#if DEBUG
struct MaestroAvatarView_Previews: PreviewProvider {
    static var previews: some View {
        VStack(spacing: 40) {
            HStack(spacing: 20) {
                MaestroAvatarView(maestro: Maestro.preview, style: .small)
                MaestroAvatarView(maestro: Maestro.preview, style: .medium)
                MaestroAvatarView(maestro: Maestro.preview, style: .large)
            }

            HStack(spacing: 20) {
                MaestroAvatarView(maestro: Maestro.preview, style: .medium, speakingState: .listening)
                MaestroAvatarView(maestro: Maestro.preview, style: .medium, speakingState: .thinking)
                MaestroAvatarView(maestro: Maestro.preview, style: .medium, speakingState: .speaking)
            }

            MaestroAvatarView(
                maestro: Maestro.preview,
                style: .hero,
                speakingState: .speaking,
                emotion: .curious,
                showStatus: true
            )
        }
        .padding()
        .frame(width: 600, height: 700)
    }
}
#endif
