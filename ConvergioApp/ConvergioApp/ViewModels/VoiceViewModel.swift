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

    @Published var errorMessage: String?

    // Emotion tracking
    @Published private(set) var emotionDistribution: [EmotionType: Double] = [:]
    @Published private(set) var emotionHistory: [EmotionHistoryEntry] = []

    // Session metadata
    @Published private(set) var sessionStartTime: Date?
    @Published private(set) var sessionDuration: TimeInterval = 0

    // Current maestro for voice configuration
    @Published private(set) var currentMaestro: Maestro?

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
    }

    // MARK: - Session Management

    /// Start voice session with optional maestro configuration
    func startSession(maestro: Maestro? = nil) async {
        logger.info("Starting voice session...")

        self.currentMaestro = maestro

        do {
            // Connect to OpenAI Realtime with maestro configuration
            try await voiceManager.connect(apiKey: apiKey, maestro: maestro)

            // Start listening
            try await voiceManager.startListening()

            // Initialize session metadata
            sessionStartTime = Date()
            startSessionTimer()

            if let maestro = maestro {
                logger.info("Voice session started with maestro: \(maestro.name) (voice: \(maestro.voice.rawValue))")
            } else {
                logger.info("Voice session started successfully")
            }

        } catch {
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
            logger.info("Voice state changed to: \(state.rawValue)")
        }
    }

    nonisolated func voiceManager(_ manager: VoiceManager, didDetectEmotion emotion: EmotionType, confidence: Double) {
        Task { @MainActor in
            logger.info("Emotion detected: \(emotion.displayName) (\(String(format: "%.2f", confidence)))")
        }
    }

    nonisolated func voiceManager(_ manager: VoiceManager, didReceiveTranscript text: String, isFinal: Bool) {
        Task { @MainActor in
            if isFinal {
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
                // Interim transcript
                currentTranscript = text
                logger.debug("Interim transcript: \(text)")
            }
        }
    }

    nonisolated func voiceManager(_ manager: VoiceManager, didReceiveResponse text: String) {
        Task { @MainActor in
            currentResponse = text

            let entry = TranscriptEntry(
                text: text,
                isUser: false,
                emotion: currentEmotion
            )
            transcriptHistory.append(entry)

            logger.info("Assistant response: \(text)")
        }
    }

    nonisolated func voiceManager(_ manager: VoiceManager, didEncounterError error: Error) {
        Task { @MainActor in
            errorMessage = error.localizedDescription
            logger.error("Voice manager error: \(error.localizedDescription)")
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
