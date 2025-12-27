//
//  VoiceSessionView.swift
//  ConvergioApp
//
//  Created by Roberto on 2025-12-24.
//

import SwiftUI

/// Main voice conversation UI
struct VoiceSessionView: View {

    // MARK: - Properties

    @StateObject private var viewModel: VoiceViewModel
    @Environment(\.dismiss) private var dismiss

    // Maestro for this session
    let maestro: Maestro?

    // Animation state
    @State private var waveformAnimation: Bool = false
    @State private var pulseAnimation: Bool = false

    // API key state
    private let hasApiKey: Bool

    // MARK: - Initialization

    init(maestro: Maestro? = nil, apiKey: String? = nil) {
        self.maestro = maestro
        // Try: 1) passed apiKey, 2) KeychainManager, 3) environment variable
        let key: String
        if let providedKey = apiKey, !providedKey.isEmpty {
            key = providedKey
        } else {
            // Use MainActor.assumeIsolated for KeychainManager access during init
            key = KeychainManager.shared.getKey(for: .openai) ?? ""
        }
        self.hasApiKey = !key.isEmpty
        _viewModel = StateObject(wrappedValue: VoiceViewModel(apiKey: key))
    }

    // MARK: - Body

    var body: some View {
        ZStack {
            // Background gradient
            LinearGradient(
                gradient: Gradient(colors: [
                    Color(red: 0.05, green: 0.05, blue: 0.15),
                    Color(red: 0.1, green: 0.1, blue: 0.2),
                    Color(red: 0.08, green: 0.08, blue: 0.18)
                ]),
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
            .ignoresSafeArea()

            if !hasApiKey {
                // Missing API key view
                missingApiKeyView
            } else {
                VStack(spacing: 0) {
                    // Header
                    headerView
                        .padding(.horizontal, DesignSystem.Spacing.lg)
                        .padding(.top, DesignSystem.Spacing.lg)

                    Spacer()

                    // Main content
                    mainContentView
                        .padding(.horizontal, DesignSystem.Spacing.xl)

                    Spacer()

                    // Transcript overlay
                    transcriptView
                        .padding(.horizontal, DesignSystem.Spacing.lg)
                        .padding(.bottom, DesignSystem.Spacing.sm)

                    // Debug log (always visible)
                    debugLogView
                        .padding(.horizontal, DesignSystem.Spacing.lg)
                        .padding(.bottom, DesignSystem.Spacing.sm)

                    // Controls
                    controlsView
                        .padding(.horizontal, DesignSystem.Spacing.xl)
                        .padding(.bottom, DesignSystem.Spacing.lg)
                }
            }
        }
        .onAppear {
            if hasApiKey {
                Task {
                    await viewModel.startSession(maestro: maestro)
                }
            }
        }
        .onDisappear {
            Task {
                await viewModel.endSession()
            }
        }
        .alert("Error", isPresented: .constant(viewModel.errorMessage != nil)) {
            Button("OK") {
                viewModel.clearError()
            }
        } message: {
            if let error = viewModel.errorMessage {
                Text(error)
            }
        }
    }

    // MARK: - Header

    private var headerView: some View {
        HStack {
            // Session info
            VStack(alignment: .leading, spacing: 4) {
                if let maestro = maestro {
                    Text("Sessione con \(maestro.name)")
                        .font(DesignSystem.Typography.headline)
                        .foregroundColor(.white)
                } else {
                    Text("Voice Session")
                        .font(DesignSystem.Typography.headline)
                        .foregroundColor(.white)
                }

                HStack(spacing: DesignSystem.Spacing.sm) {
                    Circle()
                        .fill(viewModel.isConnected ? DesignSystem.Colors.success : DesignSystem.Colors.error)
                        .frame(width: 8, height: 8)

                    Text(viewModel.isConnected ? "Connesso" : "Disconnesso")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }

            Spacer()

            // Emotion indicator
            EmotionIndicator(
                emotion: viewModel.currentEmotion,
                confidence: viewModel.emotionConfidence
            )

            // Close button
            Button(action: {
                dismiss()
            }) {
                Image(systemName: "xmark.circle.fill")
                    .font(DesignSystem.Typography.title2)
                    .foregroundColor(.white.opacity(0.7))
            }
            .buttonStyle(.plain)
            .padding(.leading, DesignSystem.Spacing.md)
        }
    }

    // MARK: - Main Content

    private var mainContentView: some View {
        VStack(spacing: 30) {
            // Maestro avatar
            maestroAvatarView

            // Waveform visualization
            waveformVisualizationView

            // State indicator
            stateIndicatorView
        }
    }

    private var maestroAvatarView: some View {
        ZStack {
            // Outer pulse ring
            Circle()
                .stroke(currentEmotionColor.opacity(0.3), lineWidth: 4)
                .frame(width: 200, height: 200)
                .scaleEffect(pulseAnimation ? 1.2 : 1.0)
                .opacity(pulseAnimation ? 0.0 : 1.0)
                .animation(
                    viewModel.state == .speaking ?
                        .easeInOut(duration: 1.5).repeatForever(autoreverses: false) :
                        .default,
                    value: pulseAnimation
                )

            // Main avatar circle
            Circle()
                .fill(
                    LinearGradient(
                        gradient: Gradient(colors: [
                            currentEmotionColor.opacity(0.6),
                            currentEmotionColor.opacity(0.3)
                        ]),
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .frame(width: 180, height: 180)
                .shadow(color: currentEmotionColor.opacity(0.5), radius: 20)

            // Maestro icon/image
            Image(systemName: maestro?.icon ?? "person.wave.2.fill")
                .font(.system(size: 60, weight: .medium))
                .foregroundColor(.white)
        }
        .onAppear {
            pulseAnimation = true
        }
    }

    private var waveformVisualizationView: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            ForEach(0..<40, id: \.self) { index in
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.small)
                    .fill(waveformBarColor(for: index))
                    .frame(width: 3)
                    .frame(height: waveformBarHeight(for: index))
                    .animation(DesignSystem.Animation.quick, value: viewModel.inputAudioLevels[index])
                    .animation(DesignSystem.Animation.quick, value: viewModel.outputAudioLevels[index])
            }
        }
        .frame(height: 80)
        .padding(.horizontal, DesignSystem.Spacing.sm)
    }

    /// Calculate bar height based on real audio levels
    private func waveformBarHeight(for index: Int) -> CGFloat {
        let minHeight: CGFloat = 4
        let maxHeight: CGFloat = 70

        switch viewModel.state {
        case .listening:
            // Use input audio levels when listening
            let level = CGFloat(viewModel.inputAudioLevels[index])
            return minHeight + (maxHeight - minHeight) * level
        case .speaking:
            // Use output audio levels when AI is speaking
            let level = CGFloat(viewModel.outputAudioLevels[index])
            return minHeight + (maxHeight - minHeight) * level
        case .processing:
            // Subtle animation while processing
            return minHeight + 10
        default:
            return minHeight
        }
    }

    /// Color varies based on state and intensity
    private func waveformBarColor(for index: Int) -> Color {
        switch viewModel.state {
        case .listening:
            return DesignSystem.Colors.voiceListening.opacity(0.8)
        case .speaking:
            return currentEmotionColor.opacity(0.9)
        case .processing:
            return DesignSystem.Colors.voiceProcessing.opacity(0.6)
        default:
            return DesignSystem.Colors.textTertiary.opacity(0.4)
        }
    }

    private var stateIndicatorView: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            // State icon with animation
            Image(systemName: stateIcon)
                .font(DesignSystem.Typography.title2)
                .foregroundColor(currentEmotionColor)
                .symbolEffect(.pulse, options: .repeating, value: viewModel.state)

            VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                // State text
                Text(stateText)
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(.white)
                
                // State subtitle
                Text(stateSubtext)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
        }
        .padding(.horizontal, DesignSystem.Spacing.lg)
        .padding(.vertical, DesignSystem.Spacing.md)
        .background(
            Capsule()
                .fill(DesignSystem.Colors.overlayLight)
                .overlay(
                    Capsule()
                        .stroke(currentEmotionColor.opacity(0.6), lineWidth: 1.5)
                )
        )
        .shadow(
            color: currentEmotionColor.opacity(0.3),
            radius: DesignSystem.Shadow.large.radius,
            x: DesignSystem.Shadow.large.x,
            y: DesignSystem.Shadow.large.y
        )
    }
    
    private var stateSubtext: String {
        switch viewModel.state {
        case .idle:
            return "Ready to start"
        case .listening:
            return "Speak now..."
        case .processing:
            return "AI is thinking"
        case .speaking:
            return "AI is responding"
        }
    }

    // MARK: - Debug Log

    @State private var isDebugLogExpanded = false

    private var debugLogView: some View {
        VStack(alignment: .leading, spacing: 4) {
            Button(action: {
                withAnimation(DesignSystem.Animation.smooth) {
                    isDebugLogExpanded.toggle()
                }
            }) {
                HStack {
                    Image(systemName: "ladybug.fill")
                        .foregroundColor(DesignSystem.Colors.warning)
                    Text("Debug Log")
                        .font(DesignSystem.Typography.caption.bold())
                        .foregroundColor(DesignSystem.Colors.warning)
                    Spacer()
                    Text("\(viewModel.debugLogs.count) entries")
                        .font(DesignSystem.Typography.caption2)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Image(systemName: isDebugLogExpanded ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.caption2)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            .buttonStyle(.plain)

            if isDebugLogExpanded {
                ScrollViewReader { proxy in
                    ScrollView {
                        VStack(alignment: .leading, spacing: 2) {
                            ForEach(Array(viewModel.debugLogs.enumerated()), id: \.offset) { index, log in
                                Text(log)
                                    .font(.system(size: 11, design: .monospaced))
                                    .foregroundColor(logColor(for: log))
                                    .frame(maxWidth: .infinity, alignment: .leading)
                                    .id(index)
                            }
                        }
                        .padding(DesignSystem.Spacing.sm)
                    }
                    .frame(height: 150)
                    .background(DesignSystem.Colors.overlay)
                    .cornerRadius(DesignSystem.CornerRadius.medium)
                    .transition(.opacity.combined(with: .move(edge: .bottom)))
                    .onChange(of: viewModel.debugLogs.count) { _, _ in
                        withAnimation {
                            if let lastIndex = viewModel.debugLogs.indices.last {
                                proxy.scrollTo(lastIndex, anchor: .bottom)
                            }
                        }
                    }
                }
            }
        }
    }

    private func logColor(for log: String) -> Color {
        if log.contains("ERROR") || log.contains("❌") {
            return DesignSystem.Colors.error
        } else if log.contains("🎤") {
            return DesignSystem.Colors.voiceListening
        } else if log.contains("🤖") {
            return DesignSystem.Colors.success
        } else if log.contains("NOT SET") {
            return DesignSystem.Colors.warning
        } else if log.contains("SET") || log.contains("successfully") || log.contains("ready") {
            return DesignSystem.Colors.success
        } else {
            return .white.opacity(0.8)
        }
    }

    // MARK: - Transcript

    @State private var showInterimTranscript = true
    
    private var transcriptView: some View {
        VStack(alignment: .leading, spacing: 8) {
            if !viewModel.currentTranscript.isEmpty || !viewModel.currentResponse.isEmpty {
                ScrollViewReader { proxy in
                    ScrollView {
                        VStack(alignment: .leading, spacing: 12) {
                            // User transcript (show both interim and final)
                            if !viewModel.currentTranscript.isEmpty {
                                VStack(alignment: .leading, spacing: 4) {
                                    transcriptBubble(
                                        text: viewModel.currentTranscript,
                                        isUser: true,
                                        isInterim: viewModel.state == .listening
                                    )
                                    .id("user")
                                    
                                    // Show interim indicator
                                    if viewModel.state == .listening && showInterimTranscript {
                                        HStack(spacing: DesignSystem.Spacing.xs) {
                                            Circle()
                                                .fill(DesignSystem.Colors.voiceListening)
                                                .frame(width: 4, height: 4)
                                            Text("Listening...")
                                                .font(DesignSystem.Typography.caption2)
                                                .foregroundColor(DesignSystem.Colors.textSecondary)
                                        }
                                        .padding(.leading, DesignSystem.Spacing.md)
                                    }
                                }
                            }

                            // Assistant response
                            if !viewModel.currentResponse.isEmpty {
                                transcriptBubble(
                                    text: viewModel.currentResponse,
                                    isUser: false,
                                    isInterim: viewModel.state == .speaking
                                )
                                .id("assistant")
                            }
                        }
                        .padding(DesignSystem.Spacing.md)
                    }
                    .frame(maxHeight: 200)
                    .background(DesignSystem.Colors.overlay)
                    .cornerRadius(DesignSystem.CornerRadius.large)
                    .onChange(of: viewModel.currentResponse) { _, _ in
                        withAnimation {
                            proxy.scrollTo("assistant", anchor: .bottom)
                        }
                    }
                    .onChange(of: viewModel.currentTranscript) { _, _ in
                        withAnimation {
                            proxy.scrollTo("user", anchor: .bottom)
                        }
                    }
                }
            }
        }
    }

    private func transcriptBubble(text: String, isUser: Bool, isInterim: Bool = false) -> some View {
        HStack {
            if !isUser { Spacer() }

            Text(text)
                .font(DesignSystem.Typography.body)
                .foregroundColor(.white)
                .padding(.horizontal, DesignSystem.Spacing.md)
                .padding(.vertical, DesignSystem.Spacing.sm + 2)
                .background(
                    isUser ?
                        DesignSystem.Colors.info.opacity(isInterim ? 0.5 : 0.7) :
                        currentEmotionColor.opacity(isInterim ? 0.5 : 0.7)
                )
                .cornerRadius(DesignSystem.CornerRadius.xlarge)
                .overlay(
                    RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.xlarge)
                        .stroke(isInterim ? Color.white.opacity(0.3) : Color.clear, lineWidth: 1)
                )
                .shadow(
                    color: (isUser ? DesignSystem.Colors.info : currentEmotionColor).opacity(0.3),
                    radius: DesignSystem.Shadow.large.radius,
                    x: DesignSystem.Shadow.large.x,
                    y: DesignSystem.Shadow.large.y
                )
                .frame(maxWidth: 400, alignment: isUser ? .leading : .trailing)
                .opacity(isInterim ? 0.8 : 1.0)

            if isUser { Spacer() }
        }
    }

    // MARK: - Controls

    private var controlsView: some View {
        HStack(spacing: 24) {
            // Mute button
            Button(action: {
                viewModel.toggleMute()
            }) {
                Image(systemName: viewModel.isMuted ? "mic.slash.fill" : "mic.fill")
                    .font(DesignSystem.Typography.title2)
                    .foregroundColor(.white)
                    .frame(width: 56, height: 56)
                    .background(
                        Circle()
                            .fill(viewModel.isMuted ? DesignSystem.Colors.error.opacity(0.8) : DesignSystem.Colors.overlayLight)
                    )
            }
            .buttonStyle(.plain)

            // End session button
            Button(action: {
                Task {
                    await viewModel.endSession()
                    dismiss()
                }
            }) {
                HStack(spacing: DesignSystem.Spacing.sm) {
                    Image(systemName: "phone.down.fill")
                        .font(DesignSystem.Typography.title2)

                    Text("End Session")
                        .font(DesignSystem.Typography.headline)
                }
                .foregroundColor(.white)
                .padding(.horizontal, DesignSystem.Spacing.xl)
                .padding(.vertical, DesignSystem.Spacing.md)
                .background(
                    Capsule()
                        .fill(DesignSystem.Colors.error.opacity(0.8))
                )
            }
            .buttonStyle(.plain)
        }
    }

    // MARK: - Helpers

    private var stateIcon: String {
        switch viewModel.state {
        case .idle:
            return "circle.fill"
        case .listening:
            return "ear.fill"
        case .processing:
            return "waveform.circle.fill"
        case .speaking:
            return "speaker.wave.3.fill"
        }
    }

    private var stateText: String {
        switch viewModel.state {
        case .idle:
            return "Ready"
        case .listening:
            return "Listening..."
        case .processing:
            return "Thinking..."
        case .speaking:
            return "Speaking"
        }
    }

    private var currentEmotionColor: Color {
        let rgb = viewModel.currentEmotion.color
        return Color(red: rgb.red, green: rgb.green, blue: rgb.blue)
    }

    // MARK: - Missing API Key View

    private var missingApiKeyView: some View {
        VStack(spacing: 24) {
            // Icon
            Image(systemName: "waveform.and.mic")
                .font(.system(size: 64, weight: .medium))
                .foregroundColor(DesignSystem.Colors.warning)

            // Title
            Text("Voice Configuration Required")
                .font(DesignSystem.Typography.title2.bold())
                .foregroundColor(.white)

            // Description
            Text("Voice conversations require either Azure OpenAI Realtime (recommended) or OpenAI with Realtime API access.")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .multilineTextAlignment(.center)
                .padding(.horizontal, DesignSystem.Spacing.xxl)

            // Option 1: Azure (Recommended)
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                Text("Option 1: Azure OpenAI (Recommended)")
                    .font(DesignSystem.Typography.subheadline.bold())
                    .foregroundColor(DesignSystem.Colors.info)

                instructionRow(number: 1, text: "Set AZURE_OPENAI_REALTIME_ENDPOINT")
                instructionRow(number: 2, text: "Set AZURE_OPENAI_REALTIME_API_KEY")
                instructionRow(number: 3, text: "Deployment: gpt-4o-realtime")
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.info.opacity(0.1))
            .cornerRadius(DesignSystem.CornerRadius.large)

            // Option 2: Direct OpenAI
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                Text("Option 2: Direct OpenAI")
                    .font(DesignSystem.Typography.subheadline.bold())
                    .foregroundColor(DesignSystem.Colors.success)

                instructionRow(number: 1, text: "Go to Settings (⌘,)")
                instructionRow(number: 2, text: "Select 'Providers' tab")
                instructionRow(number: 3, text: "Enter your OpenAI API key")

                Text("Note: Key must have Realtime API access")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.success.opacity(0.1))
            .cornerRadius(DesignSystem.CornerRadius.large)

            // Close button
            Button(action: {
                dismiss()
            }) {
                Text("Close")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(.white)
                    .padding(.horizontal, DesignSystem.Spacing.xl)
                    .padding(.vertical, DesignSystem.Spacing.md)
                    .background(DesignSystem.Colors.primary.opacity(0.8))
                    .cornerRadius(DesignSystem.CornerRadius.medium)
            }
            .buttonStyle(.plain)
            .padding(.top, DesignSystem.Spacing.md)
        }
        .padding(DesignSystem.Spacing.xxl)
    }

    private func instructionRow(number: Int, text: String) -> some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            Text("\(number)")
                .font(DesignSystem.Typography.caption.bold())
                .foregroundColor(.white)
                .frame(width: 24, height: 24)
                .background(Circle().fill(DesignSystem.Colors.primary))

            Text(text)
                .font(DesignSystem.Typography.subheadline)
                .foregroundColor(.white.opacity(0.9))
        }
    }
}

// MARK: - Preview

#Preview {
    VoiceSessionView(maestro: .preview, apiKey: "preview-api-key")
}
