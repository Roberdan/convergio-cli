/**
 * CONVERGIO NATIVE - Voice Manager
 *
 * Voice Manager for handling audio capture, playback, and WebSocket
 * communication with OpenAI Realtime Audio API.
 *
 * Uses OpenAI gpt-4o-realtime-preview-2024-12-17 model
 * Audio format: PCM16 @ 24kHz
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import AVFoundation
import Combine

/// Voice state enumeration
enum VoiceState: String, Codable {
    case idle
    case listening
    case processing
    case speaking
}

/// Emotion type enumeration based on educational context
enum EmotionType: String, Codable, CaseIterable {
    case neutral
    case joy
    case excitement
    case curiosity
    case confusion
    case frustration
    case anxiety
    case boredom
    case distraction

    var displayName: String {
        switch self {
        case .neutral: return "Neutral"
        case .joy: return "Joy"
        case .excitement: return "Excitement"
        case .curiosity: return "Curiosity"
        case .confusion: return "Confusion"
        case .frustration: return "Frustration"
        case .anxiety: return "Anxiety"
        case .boredom: return "Boredom"
        case .distraction: return "Distraction"
        }
    }

    var color: (red: Double, green: Double, blue: Double) {
        switch self {
        case .neutral: return (0.5, 0.5, 0.5)
        case .joy: return (1.0, 0.85, 0.0)
        case .excitement: return (1.0, 0.4, 0.0)
        case .curiosity: return (0.0, 0.7, 1.0)
        case .confusion: return (1.0, 0.6, 0.0)
        case .frustration: return (0.9, 0.2, 0.2)
        case .anxiety: return (0.6, 0.0, 0.8)
        case .boredom: return (0.4, 0.4, 0.4)
        case .distraction: return (0.0, 0.8, 0.6)
        }
    }
}

/// Voice event delegate protocol
protocol VoiceManagerDelegate: AnyObject {
    func voiceManager(_ manager: VoiceManager, didChangeState state: VoiceState)
    func voiceManager(_ manager: VoiceManager, didDetectEmotion emotion: EmotionType, confidence: Double)
    func voiceManager(_ manager: VoiceManager, didReceiveTranscript text: String, isFinal: Bool)
    func voiceManager(_ manager: VoiceManager, didReceiveResponse text: String)
    func voiceManager(_ manager: VoiceManager, didEncounterError error: Error)
}

/// Voice Manager for handling audio capture, playback, and WebSocket communication with OpenAI Realtime
@MainActor
class VoiceManager: NSObject, ObservableObject {

    // MARK: - Published Properties

    @Published private(set) var state: VoiceState = .idle
    @Published private(set) var currentEmotion: EmotionType = .neutral
    @Published private(set) var emotionConfidence: Double = 0.0
    @Published private(set) var isMuted: Bool = false
    @Published private(set) var isConnected: Bool = false

    // MARK: - Properties

    weak var delegate: VoiceManagerDelegate?

    private let audioEngine = AVAudioEngine()
    private var inputNode: AVAudioInputNode?
    private var webSocket: OpenAIRealtimeWebSocket?

    private let audioFormat: AVAudioFormat
    private let logger = Logger.shared

    // Audio settings for OpenAI Realtime (24kHz, 16-bit PCM, mono)
    private static let sampleRate: Double = 24000.0
    private static let channels: AVAudioChannelCount = 1
    private static let bitDepth: Int = 16

    // Current maestro configuration
    private var currentMaestro: Maestro?

    // MARK: - Initialization

    override init() {
        // Initialize audio format for OpenAI Realtime requirements
        guard let format = AVAudioFormat(
            commonFormat: .pcmFormatInt16,
            sampleRate: Self.sampleRate,
            channels: Self.channels,
            interleaved: true
        ) else {
            fatalError("Failed to create audio format")
        }
        self.audioFormat = format

        super.init()

        setupAudioSession()
    }

    // MARK: - Audio Session Setup

    private func setupAudioSession() {
        #if os(macOS)
        // macOS doesn't require AVAudioSession configuration
        logger.info("Audio session ready for macOS")
        #else
        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playAndRecord, mode: .default, options: [.defaultToSpeaker, .allowBluetooth])
            try session.setActive(true)
            logger.info("Audio session configured successfully")
        } catch {
            logger.error("Failed to configure audio session: \(error.localizedDescription)")
        }
        #endif
    }

    // MARK: - Public Methods

    /// Connect to OpenAI Realtime API with optional maestro configuration
    /// Prefers Azure OpenAI if configured, falls back to direct OpenAI
    func connect(apiKey: String, maestro: Maestro? = nil) async throws {
        guard !isConnected else {
            logger.warning("Already connected to OpenAI Realtime")
            return
        }

        self.currentMaestro = maestro

        // Check for Azure configuration first (preferred for GDPR compliance)
        let azureEndpoint = KeychainManager.shared.getKey(for: .azureRealtimeEndpoint)
        let azureApiKey = KeychainManager.shared.getKey(for: .azureRealtimeKey)
        let azureDeployment = KeychainManager.shared.getKey(for: .azureRealtimeDeployment) ?? "gpt-4o-realtime"

        if let endpoint = azureEndpoint, !endpoint.isEmpty,
           let azKey = azureApiKey, !azKey.isEmpty {
            // Use Azure OpenAI
            logger.info("Connecting to Azure OpenAI Realtime...")
            webSocket = OpenAIRealtimeWebSocket(
                azureApiKey: azKey,
                endpoint: endpoint,
                deployment: azureDeployment
            )
        } else if !apiKey.isEmpty {
            // Fall back to direct OpenAI
            logger.info("Connecting to OpenAI Realtime (direct)...")
            webSocket = OpenAIRealtimeWebSocket(apiKey: apiKey)
        } else {
            throw VoiceError.webSocketError("No API key configured. Please configure Azure OpenAI or OpenAI in Settings → Providers.")
        }

        webSocket?.delegate = self

        // Configure voice and system prompt based on maestro
        let voice: OpenAIVoice = maestro.map { OpenAIVoice(rawValue: $0.voice.rawValue) ?? .sage } ?? .sage
        let systemPrompt = maestro?.voiceInstructions ?? ""

        try await webSocket?.connect(voice: voice, systemPrompt: systemPrompt)

        isConnected = true
        logger.info("Connected to voice service successfully with voice: \(voice.rawValue)")
    }

    /// Disconnect from OpenAI Realtime API
    func disconnect() async {
        guard isConnected else { return }

        logger.info("Disconnecting from OpenAI Realtime...")

        await stopListening()
        await webSocket?.disconnect()

        isConnected = false
        currentMaestro = nil
        updateState(.idle)
        logger.info("Disconnected from OpenAI Realtime")
    }

    /// Start listening to user voice
    func startListening() async throws {
        guard isConnected else {
            throw VoiceError.notConnected
        }

        guard state == .idle else {
            logger.warning("Cannot start listening in current state: \(state)")
            return
        }

        logger.info("Starting voice listening...")

        // Request microphone permission
        let permission = await requestMicrophonePermission()
        guard permission else {
            throw VoiceError.permissionDenied
        }

        // Setup audio engine
        try setupAudioEngine()

        // Start audio engine
        try audioEngine.start()

        updateState(.listening)
        logger.info("Voice listening started")
    }

    /// Stop listening to user voice
    func stopListening() async {
        guard state == .listening || state == .processing else { return }

        logger.info("Stopping voice listening...")

        audioEngine.stop()
        audioEngine.reset()

        updateState(.idle)
        logger.info("Voice listening stopped")
    }

    /// Toggle mute/unmute
    func toggleMute() {
        isMuted.toggle()
        logger.info("Voice \(isMuted ? "muted" : "unmuted")")
    }

    /// Cancel current response (for barge-in)
    func cancelResponse() async {
        await webSocket?.cancelResponse()
    }

    /// Send text input (for testing or text-based fallback)
    func sendText(_ text: String) async throws {
        guard isConnected else {
            throw VoiceError.notConnected
        }

        await webSocket?.sendText(text)
    }

    // MARK: - Private Methods

    private func setupAudioEngine() throws {
        inputNode = audioEngine.inputNode

        guard let inputNode = inputNode else {
            throw VoiceError.audioEngineError("Input node not available")
        }

        let inputFormat = inputNode.outputFormat(forBus: 0)

        // Install tap on input node to capture audio
        inputNode.installTap(onBus: 0, bufferSize: 4096, format: inputFormat) { [weak self] buffer, time in
            Task { @MainActor [weak self] in
                await self?.processAudioBuffer(buffer)
            }
        }

        audioEngine.prepare()
    }

    private func processAudioBuffer(_ buffer: AVAudioPCMBuffer) async {
        guard !isMuted, isConnected else { return }

        // Convert buffer to required format if needed
        guard let convertedBuffer = convertBufferToRequiredFormat(buffer) else {
            logger.error("Failed to convert audio buffer")
            return
        }

        // Send audio data to WebSocket
        await webSocket?.sendAudio(convertedBuffer)
    }

    private func convertBufferToRequiredFormat(_ buffer: AVAudioPCMBuffer) -> AVAudioPCMBuffer? {
        // Check if conversion is needed
        guard buffer.format != audioFormat else {
            return buffer
        }

        // Create converter
        guard let converter = AVAudioConverter(from: buffer.format, to: audioFormat) else {
            logger.error("Failed to create audio converter")
            return nil
        }

        // Calculate output buffer size
        let capacity = AVAudioFrameCount(audioFormat.sampleRate) * buffer.frameLength / AVAudioFrameCount(buffer.format.sampleRate)

        guard let outputBuffer = AVAudioPCMBuffer(pcmFormat: audioFormat, frameCapacity: capacity) else {
            logger.error("Failed to create output buffer")
            return nil
        }

        var error: NSError?
        let inputBlock: AVAudioConverterInputBlock = { inNumPackets, outStatus in
            outStatus.pointee = .haveData
            return buffer
        }

        let status = converter.convert(to: outputBuffer, error: &error, withInputFrom: inputBlock)

        guard status != .error else {
            logger.error("Audio conversion failed: \(error?.localizedDescription ?? "unknown error")")
            return nil
        }

        return outputBuffer
    }

    private func requestMicrophonePermission() async -> Bool {
        #if os(macOS)
        return await withCheckedContinuation { continuation in
            switch AVCaptureDevice.authorizationStatus(for: .audio) {
            case .authorized:
                continuation.resume(returning: true)
            case .notDetermined:
                AVCaptureDevice.requestAccess(for: .audio) { granted in
                    continuation.resume(returning: granted)
                }
            default:
                continuation.resume(returning: false)
            }
        }
        #else
        return await withCheckedContinuation { continuation in
            AVAudioSession.sharedInstance().requestRecordPermission { granted in
                continuation.resume(returning: granted)
            }
        }
        #endif
    }

    private func updateState(_ newState: VoiceState) {
        state = newState
        delegate?.voiceManager(self, didChangeState: newState)
    }

    private func updateEmotion(_ emotion: EmotionType, confidence: Double) {
        currentEmotion = emotion
        emotionConfidence = confidence
        delegate?.voiceManager(self, didDetectEmotion: emotion, confidence: confidence)
    }
}

// MARK: - OpenAIRealtimeDelegate

extension VoiceManager: OpenAIRealtimeDelegate {

    nonisolated func realtimeDidConnect(_ realtime: OpenAIRealtimeWebSocket) {
        Task { @MainActor in
            logger.info("OpenAI Realtime connected")
        }
    }

    nonisolated func realtimeDidDisconnect(_ realtime: OpenAIRealtimeWebSocket, error: Error?) {
        Task { @MainActor in
            isConnected = false
            updateState(.idle)
            if let error = error {
                logger.error("OpenAI Realtime disconnected with error: \(error.localizedDescription)")
            } else {
                logger.info("OpenAI Realtime disconnected")
            }
        }
    }

    nonisolated func realtime(_ realtime: OpenAIRealtimeWebSocket, didReceiveTranscript text: String, isFinal: Bool) {
        Task { @MainActor in
            if isFinal {
                updateState(.processing)
            }
            delegate?.voiceManager(self, didReceiveTranscript: text, isFinal: isFinal)
        }
    }

    nonisolated func realtime(_ realtime: OpenAIRealtimeWebSocket, didReceiveResponse text: String) {
        Task { @MainActor in
            updateState(.speaking)
            delegate?.voiceManager(self, didReceiveResponse: text)
        }
    }

    nonisolated func realtime(_ realtime: OpenAIRealtimeWebSocket, didReceiveAudio audioData: Data) {
        Task { @MainActor in
            // Play received audio
            await playAudio(audioData)
        }
    }

    nonisolated func realtime(_ realtime: OpenAIRealtimeWebSocket, didCompleteResponse: Void) {
        Task { @MainActor in
            updateState(.listening)
        }
    }

    nonisolated func realtime(_ realtime: OpenAIRealtimeWebSocket, didEncounterError error: Error) {
        Task { @MainActor in
            logger.error("OpenAI Realtime error: \(error.localizedDescription)")
            delegate?.voiceManager(self, didEncounterError: error)
        }
    }
}

// MARK: - Audio Playback

extension VoiceManager {

    private func playAudio(_ audioData: Data) async {
        // Create audio buffer from data
        guard let buffer = createAudioBuffer(from: audioData) else {
            logger.error("Failed to create audio buffer for playback")
            return
        }

        // Create player node
        let playerNode = AVAudioPlayerNode()
        audioEngine.attach(playerNode)

        // Connect player node to output
        audioEngine.connect(playerNode, to: audioEngine.mainMixerNode, format: buffer.format)

        // Schedule buffer
        playerNode.scheduleBuffer(buffer) { [weak self] in
            Task { @MainActor [weak self] in
                self?.audioEngine.detach(playerNode)
            }
        }

        // Play
        playerNode.play()
    }

    private func createAudioBuffer(from data: Data) -> AVAudioPCMBuffer? {
        let frameCount = UInt32(data.count) / audioFormat.streamDescription.pointee.mBytesPerFrame

        guard let buffer = AVAudioPCMBuffer(pcmFormat: audioFormat, frameCapacity: frameCount) else {
            return nil
        }

        buffer.frameLength = frameCount

        let channels = UnsafeBufferPointer(start: buffer.int16ChannelData, count: Int(audioFormat.channelCount))
        data.withUnsafeBytes { rawBuffer in
            guard let source = rawBuffer.baseAddress?.assumingMemoryBound(to: Int16.self) else { return }
            guard audioFormat.channelCount > 0 else { return }
            channels[0].update(from: source, count: Int(frameCount))
        }

        return buffer
    }
}

// MARK: - Errors

enum VoiceError: LocalizedError {
    case notConnected
    case permissionDenied
    case audioEngineError(String)
    case webSocketError(String)

    var errorDescription: String? {
        switch self {
        case .notConnected:
            return "Not connected to voice service"
        case .permissionDenied:
            return "Microphone permission denied"
        case .audioEngineError(let message):
            return "Audio engine error: \(message)"
        case .webSocketError(let message):
            return "WebSocket error: \(message)"
        }
    }
}
