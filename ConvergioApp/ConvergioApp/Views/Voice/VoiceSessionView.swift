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
                    Color(red: 0.1, green: 0.1, blue: 0.2)
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
                        .padding(.horizontal, 20)
                        .padding(.top, 20)

                    Spacer()

                    // Main content
                    mainContentView
                        .padding(.horizontal, 40)

                    Spacer()

                    // Transcript overlay
                    transcriptView
                        .padding(.horizontal, 20)
                        .padding(.bottom, 20)

                    // Controls
                    controlsView
                        .padding(.horizontal, 40)
                        .padding(.bottom, 40)
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
                        .font(.headline)
                        .foregroundColor(.white)
                } else {
                    Text("Voice Session")
                        .font(.headline)
                        .foregroundColor(.white)
                }

                HStack(spacing: 8) {
                    Circle()
                        .fill(viewModel.isConnected ? Color.green : Color.red)
                        .frame(width: 8, height: 8)

                    Text(viewModel.isConnected ? "Connesso" : "Disconnesso")
                        .font(.caption)
                        .foregroundColor(.gray)
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
                    .font(.title2)
                    .foregroundColor(.white.opacity(0.7))
            }
            .buttonStyle(.plain)
            .padding(.leading, 16)
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
                .font(.system(size: 60))
                .foregroundColor(.white)
        }
        .onAppear {
            pulseAnimation = true
        }
    }

    private var waveformVisualizationView: some View {
        HStack(spacing: 4) {
            ForEach(0..<40, id: \.self) { index in
                RoundedRectangle(cornerRadius: 2)
                    .fill(currentEmotionColor)
                    .frame(width: 3)
                    .frame(height: viewModel.state == .listening || viewModel.state == .speaking ?
                           CGFloat.random(in: 20...60) : 4)
                    .animation(
                        viewModel.state == .listening || viewModel.state == .speaking ?
                            .easeInOut(duration: 0.3).repeatForever(autoreverses: true).delay(Double(index) * 0.02) :
                            .default,
                        value: waveformAnimation
                    )
            }
        }
        .frame(height: 80)
        .onChange(of: viewModel.state) { _, newState in
            waveformAnimation.toggle()
        }
    }

    private var stateIndicatorView: some View {
        HStack(spacing: 12) {
            // State icon
            Image(systemName: stateIcon)
                .font(.title3)
                .foregroundColor(currentEmotionColor)

            // State text
            Text(stateText)
                .font(.headline)
                .foregroundColor(.white)
        }
        .padding(.horizontal, 24)
        .padding(.vertical, 12)
        .background(
            Capsule()
                .fill(Color.white.opacity(0.1))
                .overlay(
                    Capsule()
                        .stroke(currentEmotionColor.opacity(0.5), lineWidth: 1)
                )
        )
    }

    // MARK: - Transcript

    private var transcriptView: some View {
        VStack(alignment: .leading, spacing: 8) {
            if !viewModel.currentTranscript.isEmpty || !viewModel.currentResponse.isEmpty {
                ScrollViewReader { proxy in
                    ScrollView {
                        VStack(alignment: .leading, spacing: 12) {
                            // User transcript
                            if !viewModel.currentTranscript.isEmpty {
                                transcriptBubble(
                                    text: viewModel.currentTranscript,
                                    isUser: true
                                )
                                .id("user")
                            }

                            // Assistant response
                            if !viewModel.currentResponse.isEmpty {
                                transcriptBubble(
                                    text: viewModel.currentResponse,
                                    isUser: false
                                )
                                .id("assistant")
                            }
                        }
                        .padding()
                    }
                    .frame(maxHeight: 200)
                    .background(Color.black.opacity(0.3))
                    .cornerRadius(12)
                    .onChange(of: viewModel.currentResponse) { _, _ in
                        withAnimation {
                            proxy.scrollTo("assistant", anchor: .bottom)
                        }
                    }
                }
            }
        }
    }

    private func transcriptBubble(text: String, isUser: Bool) -> some View {
        HStack {
            if !isUser { Spacer() }

            Text(text)
                .font(.body)
                .foregroundColor(.white)
                .padding(.horizontal, 16)
                .padding(.vertical, 10)
                .background(
                    isUser ?
                        Color.blue.opacity(0.6) :
                        currentEmotionColor.opacity(0.6)
                )
                .cornerRadius(16)
                .frame(maxWidth: 300, alignment: isUser ? .leading : .trailing)

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
                    .font(.title2)
                    .foregroundColor(.white)
                    .frame(width: 56, height: 56)
                    .background(
                        Circle()
                            .fill(viewModel.isMuted ? Color.red.opacity(0.8) : Color.white.opacity(0.2))
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
                HStack(spacing: 8) {
                    Image(systemName: "phone.down.fill")
                        .font(.title3)

                    Text("End Session")
                        .font(.headline)
                }
                .foregroundColor(.white)
                .padding(.horizontal, 32)
                .padding(.vertical, 16)
                .background(
                    Capsule()
                        .fill(Color.red.opacity(0.8))
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
                .font(.system(size: 64))
                .foregroundColor(.orange)

            // Title
            Text("Voice Configuration Required")
                .font(.title2.bold())
                .foregroundColor(.white)

            // Description
            Text("Voice conversations require either Azure OpenAI Realtime (recommended) or OpenAI with Realtime API access.")
                .font(.body)
                .foregroundColor(.gray)
                .multilineTextAlignment(.center)
                .padding(.horizontal, 40)

            // Option 1: Azure (Recommended)
            VStack(alignment: .leading, spacing: 12) {
                Text("Option 1: Azure OpenAI (Recommended)")
                    .font(.subheadline.bold())
                    .foregroundColor(.cyan)

                instructionRow(number: 1, text: "Set AZURE_OPENAI_REALTIME_ENDPOINT")
                instructionRow(number: 2, text: "Set AZURE_OPENAI_REALTIME_API_KEY")
                instructionRow(number: 3, text: "Deployment: gpt-4o-realtime")
            }
            .padding()
            .background(Color.cyan.opacity(0.1))
            .cornerRadius(12)

            // Option 2: Direct OpenAI
            VStack(alignment: .leading, spacing: 12) {
                Text("Option 2: Direct OpenAI")
                    .font(.subheadline.bold())
                    .foregroundColor(.green)

                instructionRow(number: 1, text: "Go to Settings (⌘,)")
                instructionRow(number: 2, text: "Select 'Providers' tab")
                instructionRow(number: 3, text: "Enter your OpenAI API key")

                Text("Note: Key must have Realtime API access")
                    .font(.caption)
                    .foregroundColor(.gray)
            }
            .padding()
            .background(Color.green.opacity(0.1))
            .cornerRadius(12)

            // Close button
            Button(action: {
                dismiss()
            }) {
                Text("Close")
                    .font(.headline)
                    .foregroundColor(.white)
                    .padding(.horizontal, 32)
                    .padding(.vertical, 12)
                    .background(Color.blue.opacity(0.8))
                    .cornerRadius(10)
            }
            .buttonStyle(.plain)
            .padding(.top, 16)
        }
        .padding(40)
    }

    private func instructionRow(number: Int, text: String) -> some View {
        HStack(spacing: 12) {
            Text("\(number)")
                .font(.caption.bold())
                .foregroundColor(.white)
                .frame(width: 24, height: 24)
                .background(Circle().fill(Color.blue))

            Text(text)
                .font(.subheadline)
                .foregroundColor(.white.opacity(0.9))
        }
    }
}

// MARK: - Preview

#Preview {
    VoiceSessionView(maestro: .preview, apiKey: "preview-api-key")
}
