/**
 * CONVERGIO NATIVE - Voice View Model
 *
 * View model for voice session management using OpenAI Realtime Audio API.
 * Supports maestro-specific voice configuration.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import SwiftUI
import Combine

/// View model for voice session management
@MainActor
class VoiceViewModel: ObservableObject {

    // MARK: - Published Properties

    @Published private(set) var state: VoiceState = .idle
    @Published private(set) var currentEmotion: EmotionType = .neutral
    @Published private(set) var emotionConfidence: Double = 0.0
    @Published private(set) var isMuted: Bool = false
    @Published private(set) var isConnected: Bool = false

    @Published private(set) var currentTranscript: String = ""
    @Published private(set) var currentResponse: String = ""
    @Published private(set) var transcriptHistory: [TranscriptEntry] = []

    // Accumulated response text (for streaming deltas)
    private var accumulatedResponse: String = ""

    @Published var errorMessage: String?

    // Debug log for UI display
    @Published private(set) var debugLogs: [String] = []

    // Emotion tracking
    @Published private(set) var emotionDistribution: [EmotionType: Double] = [:]
    @Published private(set) var emotionHistory: [EmotionHistoryEntry] = []

    // Session metadata
    @Published private(set) var sessionStartTime: Date?
    @Published private(set) var sessionDuration: TimeInterval = 0

    // Current maestro for voice configuration
    @Published private(set) var currentMaestro: Maestro?

    // Real-time audio levels for waveform visualization
    @Published private(set) var inputAudioLevels: [Float] = Array(repeating: 0, count: 40)
    @Published private(set) var outputAudioLevels: [Float] = Array(repeating: 0, count: 40)

    // MARK: - Properties

    private let voiceManager: VoiceManager
    private let apiKey: String
    private let logger = Logger.shared

    private var cancellables = Set<AnyCancellable>()
    private var sessionTimer: Timer?

    // MARK: - Nested Types

    struct TranscriptEntry: Identifiable, Codable {
        let id: UUID
        let timestamp: Date
        let text: String
        let isUser: Bool
        let emotion: EmotionType?

        init(id: UUID = UUID(), timestamp: Date = Date(), text: String, isUser: Bool, emotion: EmotionType? = nil) {
            self.id = id
            self.timestamp = timestamp
            self.text = text
            self.isUser = isUser
            self.emotion = emotion
        }
    }

    struct EmotionHistoryEntry: Identifiable {
        let id = UUID()
        let timestamp: Date
        let emotion: EmotionType
        let confidence: Double
    }

    // MARK: - Initialization

    init(apiKey: String) {
        self.apiKey = apiKey
        self.voiceManager = VoiceManager()

        setupVoiceManager()
        observeVoiceManager()
    }

    // MARK: - Setup

    private func setupVoiceManager() {
        voiceManager.delegate = self
        voiceManager.onDebugLog = { [weak self] message in
            Task { @MainActor [weak self] in
                self?.addDebugLog(message)
            }
        }
    }

    private func observeVoiceManager() {
        // Observe voice manager state changes
        voiceManager.$state
            .receive(on: DispatchQueue.main)
            .sink { [weak self] state in
                self?.state = state
            }
            .store(in: &cancellables)

        voiceManager.$currentEmotion
            .receive(on: DispatchQueue.main)
            .sink { [weak self] emotion in
                self?.currentEmotion = emotion
                self?.updateEmotionDistribution(emotion)
            }
            .store(in: &cancellables)

        voiceManager.$emotionConfidence
            .receive(on: DispatchQueue.main)
            .sink { [weak self] confidence in
                self?.emotionConfidence = confidence
            }
            .store(in: &cancellables)

        voiceManager.$isMuted
            .receive(on: DispatchQueue.main)
            .sink { [weak self] isMuted in
                self?.isMuted = isMuted
            }
            .store(in: &cancellables)

        voiceManager.$isConnected
            .receive(on: DispatchQueue.main)
            .sink { [weak self] isConnected in
                self?.isConnected = isConnected
            }
            .store(in: &cancellables)

        // Observe audio levels for waveform visualization
        voiceManager.$inputAudioLevels
            .receive(on: DispatchQueue.main)
            .sink { [weak self] levels in
                self?.inputAudioLevels = levels
            }
            .store(in: &cancellables)

        voiceManager.$outputAudioLevels
            .receive(on: DispatchQueue.main)
            .sink { [weak self] levels in
                self?.outputAudioLevels = levels
            }
            .store(in: &cancellables)
    }

    // MARK: - Session Management

    /// Start voice session with optional maestro configuration
    func startSession(maestro: Maestro? = nil) async {
        logger.info("Starting voice session...")
        addDebugLog("Starting voice session...")

        self.currentMaestro = maestro

        // Log Azure configuration
        let azureEndpoint = KeychainManager.shared.getKey(for: .azureRealtimeEndpoint)
        let azureKey = KeychainManager.shared.getKey(for: .azureRealtimeKey)
        let azureDeployment = KeychainManager.shared.getKey(for: .azureRealtimeDeployment)

        addDebugLog("Azure Endpoint: \(azureEndpoint != nil ? "SET (\(azureEndpoint!.prefix(30))...)" : "NOT SET")")
        addDebugLog("Azure API Key: \(azureKey != nil ? "SET (\(azureKey!.count) chars)" : "NOT SET")")
        addDebugLog("Azure Deployment: \(azureDeployment ?? "NOT SET")")
        addDebugLog("OpenAI Key: \(apiKey.isEmpty ? "NOT SET" : "SET (\(apiKey.count) chars)")")

        do {
            addDebugLog("Connecting to realtime API...")
            // Connect to OpenAI Realtime with maestro configuration
            try await voiceManager.connect(apiKey: apiKey, maestro: maestro)
            addDebugLog("Connected successfully!")

            addDebugLog("Starting audio capture...")
            // Start listening
            try await voiceManager.startListening()
            addDebugLog("Audio capture started!")

            // Initialize session metadata
            sessionStartTime = Date()
            startSessionTimer()

            if let maestro = maestro {
                addDebugLog("Session ready with maestro: \(maestro.name)")
                logger.info("Voice session started with maestro: \(maestro.name) (voice: \(maestro.voice.rawValue))")
            } else {
                addDebugLog("Session ready (no maestro)")
                logger.info("Voice session started successfully")
            }

        } catch {
            addDebugLog("ERROR: \(error.localizedDescription)")
            logger.error("Failed to start voice session: \(error.localizedDescription)")
            errorMessage = "Failed to start voice session: \(error.localizedDescription)"
        }
    }

    /// End voice session
    func endSession() async {
        logger.info("Ending voice session...")

        // Stop listening
        await voiceManager.stopListening()

        // Disconnect
        await voiceManager.disconnect()

        // Stop session timer
        stopSessionTimer()

        // Save session data (optional)
        await saveSessionData()

        logger.info("Voice session ended")
    }

    /// Toggle mute/unmute
    func toggleMute() {
        voiceManager.toggleMute()
    }

    /// Send text message (for testing or fallback)
    func sendText(_ text: String) async {
        do {
            try await voiceManager.sendText(text)

            // Add to transcript
            let entry = TranscriptEntry(
                text: text,
                isUser: true,
                emotion: currentEmotion
            )
            transcriptHistory.append(entry)

        } catch {
            logger.error("Failed to send text: \(error.localizedDescription)")
            errorMessage = "Failed to send text: \(error.localizedDescription)"
        }
    }

    // MARK: - Error Handling

    func clearError() {
        errorMessage = nil
    }

    /// Add debug log entry (visible in UI)
    func addDebugLog(_ message: String) {
        let timestamp = DateFormatter.localizedString(from: Date(), dateStyle: .none, timeStyle: .medium)
        let entry = "[\(timestamp)] \(message)"
        debugLogs.append(entry)
        // Keep only last 50 entries
        if debugLogs.count > 50 {
            debugLogs.removeFirst()
        }
        logger.debug("Voice Debug: \(message)")
    }

    // MARK: - Private Methods

    private func updateEmotionDistribution(_ emotion: EmotionType) {
        // Update emotion distribution
        let currentValue = emotionDistribution[emotion] ?? 0.0
        emotionDistribution[emotion] = min(1.0, currentValue + 0.1)

        // Decay other emotions
        for otherEmotion in EmotionType.allCases where otherEmotion != emotion {
            if let value = emotionDistribution[otherEmotion] {
                emotionDistribution[otherEmotion] = max(0.0, value - 0.05)
            }
        }

        // Add to history
        let historyEntry = EmotionHistoryEntry(
            timestamp: Date(),
            emotion: emotion,
            confidence: emotionConfidence
        )
        emotionHistory.append(historyEntry)

        // Limit history size
        if emotionHistory.count > 100 {
            emotionHistory.removeFirst()
        }
    }

    private func startSessionTimer() {
        sessionTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            guard let self = self, let startTime = self.sessionStartTime else { return }

            Task { @MainActor in
                self.sessionDuration = Date().timeIntervalSince(startTime)
            }
        }
    }

    private func stopSessionTimer() {
        sessionTimer?.invalidate()
        sessionTimer = nil
    }

    private func saveSessionData() async {
        // Save session data to persistent storage
        // This could be implemented to save to UserDefaults, Core Data, or a remote server

        let sessionData: [String: Any] = [
            "start_time": sessionStartTime?.timeIntervalSince1970 ?? 0,
            "duration": sessionDuration,
            "transcript_count": transcriptHistory.count,
            "emotion_distribution": emotionDistribution.mapValues { $0 },
            "dominant_emotion": currentEmotion.rawValue
        ]

        logger.info("Session data prepared for storage: \(sessionData)")

        // TODO: Implement actual persistence logic
    }

    // MARK: - Computed Properties

    var sessionDurationFormatted: String {
        let minutes = Int(sessionDuration) / 60
        let seconds = Int(sessionDuration) % 60
        return String(format: "%02d:%02d", minutes, seconds)
    }

    var dominantEmotion: EmotionType {
        emotionDistribution.max(by: { $0.value < $1.value })?.key ?? .neutral
    }

    var transcriptCount: Int {
        transcriptHistory.count
    }

    var userTranscriptCount: Int {
        transcriptHistory.filter { $0.isUser }.count
    }

    var assistantTranscriptCount: Int {
        transcriptHistory.filter { !$0.isUser }.count
    }
}

// MARK: - VoiceManagerDelegate

extension VoiceViewModel: VoiceManagerDelegate {

    nonisolated func voiceManager(_ manager: VoiceManager, didChangeState state: VoiceState) {
        Task { @MainActor in
            addDebugLog("State → \(state.rawValue)")
            logger.info("Voice state changed to: \(state.rawValue)")
        }
    }

    nonisolated func voiceManager(_ manager: VoiceManager, didDetectEmotion emotion: EmotionType, confidence: Double) {
        Task { @MainActor in
            addDebugLog("Emotion: \(emotion.displayName) (\(String(format: "%.0f%%", confidence * 100)))")
            logger.info("Emotion detected: \(emotion.displayName) (\(String(format: "%.2f", confidence)))")
        }
    }

    nonisolated func voiceManager(_ manager: VoiceManager, didReceiveTranscript text: String, isFinal: Bool) {
        Task { @MainActor in
            if isFinal {
                addDebugLog("🎤 User (final): \(text)")
                // Final transcript
                currentTranscript = text

                let entry = TranscriptEntry(
                    text: text,
                    isUser: true,
                    emotion: currentEmotion
                )
                transcriptHistory.append(entry)

                logger.info("Final transcript: \(text)")
            } else {
                addDebugLog("🎤 User (interim): \(text)")
                // Interim transcript
                currentTranscript = text
                logger.debug("Interim transcript: \(text)")
            }
        }
    }

    nonisolated func voiceManager(_ manager: VoiceManager, didReceiveResponse text: String) {
        Task { @MainActor in
            // ACCUMULATE deltas instead of overwriting
            // Each delta is a small piece of the streaming response (e.g., "S", "o", "m", "e")
            accumulatedResponse += text
            currentResponse = accumulatedResponse

            // Only log short responses or every 50 chars to reduce noise
            if accumulatedResponse.count < 20 || accumulatedResponse.count % 50 == 0 {
                addDebugLog("🤖 AI (streaming): \(accumulatedResponse.prefix(100))...")
            }

            // DON'T add to transcript history here - wait for response completion
            // Each delta would create a separate entry otherwise

            logger.debug("Assistant response delta: \(text)")
        }
    }

    nonisolated func voiceManager(_ manager: VoiceManager, didCompleteResponse: Void) {
        Task { @MainActor in
            // Response streaming is complete - save to transcript history
            if !accumulatedResponse.isEmpty {
                addDebugLog("🤖 AI (complete): \(accumulatedResponse.prefix(100))...")

                let entry = TranscriptEntry(
                    text: accumulatedResponse,
                    isUser: false,
                    emotion: currentEmotion
                )
                transcriptHistory.append(entry)

                logger.info("Assistant response complete: \(accumulatedResponse.prefix(100))")

                // Reset for next response
                accumulatedResponse = ""
            }
        }
    }

    nonisolated func voiceManager(_ manager: VoiceManager, didEncounterError error: Error) {
        Task { @MainActor in
            addDebugLog("❌ Error: \(error.localizedDescription)")
            errorMessage = error.localizedDescription
            logger.error("Voice manager error: \(error.localizedDescription)")

            // Reset accumulated response on error
            accumulatedResponse = ""
        }
    }
}

// MARK: - Session Statistics

extension VoiceViewModel {

    /// Get session statistics
    func getSessionStatistics() -> SessionStatistics {
        SessionStatistics(
            duration: sessionDuration,
            transcriptCount: transcriptCount,
            userTranscriptCount: userTranscriptCount,
            assistantTranscriptCount: assistantTranscriptCount,
            emotionDistribution: emotionDistribution,
            dominantEmotion: dominantEmotion,
            emotionHistory: emotionHistory
        )
    }

    struct SessionStatistics {
        let duration: TimeInterval
        let transcriptCount: Int
        let userTranscriptCount: Int
        let assistantTranscriptCount: Int
        let emotionDistribution: [EmotionType: Double]
        let dominantEmotion: EmotionType
        let emotionHistory: [EmotionHistoryEntry]

        var averageEmotionConfidence: Double {
            guard !emotionHistory.isEmpty else { return 0.0 }
            return emotionHistory.reduce(0.0) { $0 + $1.confidence } / Double(emotionHistory.count)
        }

        var emotionChanges: Int {
            var changes = 0
            var previousEmotion: EmotionType?

            for entry in emotionHistory {
                if let previous = previousEmotion, previous != entry.emotion {
                    changes += 1
                }
                previousEmotion = entry.emotion
            }

            return changes
        }
    }
}
