/**
 * CONVERGIO NATIVE - Voice Manager
 *
 * Voice Manager for handling audio capture, playback, and WebSocket
 * communication with OpenAI Realtime Audio API.
 *
 * ARCHITECTURE (v2 - Fixed):
 * - AVAudioEngine runs continuously during session for BOTH input AND output
 * - Input tap is installed/removed without stopping engine
 * - Single reusable AVAudioPlayerNode for all playback (prevents memory leaks)
 * - Proper format conversion: 48kHz Float32 (macOS native) → 24kHz PCM16 (OpenAI)
 *
 * Uses OpenAI gpt-4o-realtime-preview-2024-12-17 model
 * Audio format: PCM16 @ 24kHz
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import AVFoundation
import Combine
#if os(macOS)
import AppKit
import CoreAudio
#endif

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
    func voiceManager(_ manager: VoiceManager, didCompleteResponse: Void)
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

    // Real-time audio levels for waveform visualization (0.0 to 1.0)
    @Published private(set) var inputAudioLevels: [Float] = Array(repeating: 0, count: 40)
    @Published private(set) var outputAudioLevels: [Float] = Array(repeating: 0, count: 40)

    // MARK: - Properties

    weak var delegate: VoiceManagerDelegate?

    // Audio engine - runs continuously during session
    private let audioEngine = AVAudioEngine()
    private var isEngineRunning: Bool = false
    private var isInputTapInstalled: Bool = false

    // Playback node - single instance, reused for all playback
    private var playerNode: AVAudioPlayerNode?

    // WebSocket connection
    private var webSocket: OpenAIRealtimeWebSocket?

    private let logger = Logger.shared

    // Debug callback for UI
    var onDebugLog: ((String) -> Void)?

    // Audio buffer counter for debug
    private var audioBufferCount: Int = 0
    private var playbackBufferCount: Int = 0

    // Audio settings for OpenAI Realtime (24kHz, 16-bit PCM, mono)
    private static let openAISampleRate: Double = 24000.0
    private static let channels: AVAudioChannelCount = 1

    // Playback format (Float32 for AVAudioEngine)
    private var playbackFormat: AVAudioFormat!

    // Current maestro configuration
    private var currentMaestro: Maestro?

    // MARK: - Initialization

    override init() {
        super.init()

        // Create playback format (24kHz Float32 mono - what AVAudioEngine uses)
        guard let format = AVAudioFormat(
            commonFormat: .pcmFormatFloat32,
            sampleRate: Self.openAISampleRate,
            channels: Self.channels,
            interleaved: false
        ) else {
            fatalError("Failed to create playback format")
        }
        self.playbackFormat = format

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
            try session.setCategory(.playAndRecord, mode: .voiceChat, options: [.defaultToSpeaker, .allowBluetooth])
            try session.setActive(true)
            logger.info("Audio session configured successfully")
        } catch {
            logger.error("Failed to configure audio session: \(error.localizedDescription)")
        }
        #endif
    }

    // MARK: - Public Methods

    /// Connect to OpenAI Realtime API with optional maestro configuration
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
            logger.info("Connecting to Azure OpenAI Realtime...")
            onDebugLog?("🔌 Connecting to Azure OpenAI...")
            webSocket = OpenAIRealtimeWebSocket(
                azureApiKey: azKey,
                endpoint: endpoint,
                deployment: azureDeployment
            )
        } else if !apiKey.isEmpty {
            logger.info("Connecting to OpenAI Realtime (direct)...")
            onDebugLog?("🔌 Connecting to OpenAI...")
            webSocket = OpenAIRealtimeWebSocket(apiKey: apiKey)
        } else {
            throw VoiceError.webSocketError("No API key configured. Please configure Azure OpenAI or OpenAI in Settings → Providers.")
        }

        webSocket?.delegate = self
        webSocket?.onDebugLog = { [weak self] message in
            self?.onDebugLog?(message)
        }

        // Configure voice and system prompt based on maestro
        let voice: OpenAIVoice = maestro.map { OpenAIVoice(rawValue: $0.voice.rawValue) ?? .sage } ?? .sage
        let systemPrompt = maestro?.voiceInstructions ?? ""

        try await webSocket?.connect(voice: voice, systemPrompt: systemPrompt)

        // Setup audio engine with playback node BEFORE starting
        try setupAudioEngineForSession()

        isConnected = true
        logger.info("Connected to voice service successfully with voice: \(voice.rawValue)")
        onDebugLog?("✅ Connected! Voice: \(voice.rawValue)")
    }

    /// Disconnect from OpenAI Realtime API
    func disconnect() async {
        guard isConnected else { return }

        logger.info("Disconnecting from OpenAI Realtime...")
        onDebugLog?("🔌 Disconnecting...")

        // Stop input if listening
        await stopListening()

        // Disconnect WebSocket
        await webSocket?.disconnect()

        // Cleanup audio engine
        cleanupAudioEngine()

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

        guard state == .idle || state == .speaking else {
            logger.warning("Cannot start listening in current state: \(state)")
            return
        }

        logger.info("Starting voice listening...")
        onDebugLog?("🎤 Starting microphone...")

        // Request microphone permission
        let permission = await requestMicrophonePermission()
        guard permission else {
            throw VoiceError.permissionDenied
        }

        // Install input tap (engine should already be running)
        try installInputTap()

        updateState(.listening)
        logger.info("Voice listening started")
        onDebugLog?("🎤 Listening...")
    }

    /// Stop listening to user voice (but keep engine running for playback!)
    func stopListening() async {
        guard state == .listening || state == .processing else { return }

        logger.info("Stopping voice listening...")

        // Remove input tap but DON'T stop engine (needed for playback)
        removeInputTap()

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
        playerNode?.stop()
        await webSocket?.cancelResponse()
    }

    /// Send text input (for testing or text-based fallback)
    func sendText(_ text: String) async throws {
        guard isConnected else {
            throw VoiceError.notConnected
        }

        await webSocket?.sendText(text)
    }

    // MARK: - Audio Engine Setup

    /// Setup audio engine for the entire session (input + output)
    private func setupAudioEngineForSession() throws {
        guard !isEngineRunning else { return }

        onDebugLog?("🔧 Setting up audio engine...")

        // Create and attach player node for output
        let player = AVAudioPlayerNode()
        playerNode = player
        audioEngine.attach(player)

        // Connect player to output mixer with 24kHz format
        audioEngine.connect(player, to: audioEngine.mainMixerNode, format: playbackFormat)

        // Prepare and start engine
        audioEngine.prepare()

        do {
            try audioEngine.start()
            isEngineRunning = true
            onDebugLog?("✅ Audio engine running")
            logger.info("Audio engine started successfully")
        } catch {
            onDebugLog?("❌ Engine start failed: \(error.localizedDescription)")
            throw VoiceError.audioEngineError("Failed to start audio engine: \(error.localizedDescription)")
        }
    }

    /// Install input tap for microphone capture
    private func installInputTap() throws {
        guard !isInputTapInstalled else { return }

        let inputNode = audioEngine.inputNode
        let inputFormat = inputNode.outputFormat(forBus: 0)

        onDebugLog?("🎤 Mic format: \(Int(inputFormat.sampleRate))Hz, \(inputFormat.channelCount)ch")

        // Verify format is valid
        guard inputFormat.channelCount > 0, inputFormat.sampleRate > 0 else {
            throw VoiceError.audioEngineError("Invalid input format - no microphone available")
        }

        // Install tap with native format
        inputNode.installTap(onBus: 0, bufferSize: 4800, format: inputFormat) { [weak self] buffer, _ in
            guard let self = self, !self.isMuted, self.isConnected else { return }

            // Convert and send audio
            if let pcmData = self.convertToPCM16(buffer: buffer) {
                Task { @MainActor in
                    await self.sendAudioData(pcmData)
                }
            }
        }

        isInputTapInstalled = true
        onDebugLog?("✅ Microphone tap installed")
    }

    /// Remove input tap (but keep engine running)
    private func removeInputTap() {
        guard isInputTapInstalled else { return }

        audioEngine.inputNode.removeTap(onBus: 0)
        isInputTapInstalled = false
        onDebugLog?("🎤 Microphone tap removed")
    }

    /// Cleanup audio engine completely
    private func cleanupAudioEngine() {
        // Remove input tap
        if isInputTapInstalled {
            audioEngine.inputNode.removeTap(onBus: 0)
            isInputTapInstalled = false
        }

        // Stop player
        playerNode?.stop()

        // Stop and reset engine
        if isEngineRunning {
            audioEngine.stop()
            isEngineRunning = false
        }

        // Detach player
        if let player = playerNode {
            audioEngine.detach(player)
        }
        playerNode = nil

        audioEngine.reset()
        audioBufferCount = 0
        playbackBufferCount = 0

        onDebugLog?("🔧 Audio engine cleaned up")
    }

    // MARK: - Audio Conversion

    /// Convert Float32 buffer to PCM16 Data at 24kHz
    private func convertToPCM16(buffer: AVAudioPCMBuffer) -> Data? {
        guard let floatData = buffer.floatChannelData else { return nil }

        let inputSampleRate = buffer.format.sampleRate
        let outputSampleRate = Self.openAISampleRate
        let ratio = outputSampleRate / inputSampleRate

        let inputFrames = Int(buffer.frameLength)
        let outputFrames = Int(Double(inputFrames) * ratio)

        // Calculate audio levels for visualization
        updateInputLevels(floatData: floatData, frameCount: inputFrames)

        var pcmData = Data(capacity: outputFrames * 2)

        // Resample and convert Float32 → Int16
        for i in 0..<outputFrames {
            let srcIndex = Double(i) / ratio
            let srcIndexInt = Int(srcIndex)
            let frac = Float(srcIndex - Double(srcIndexInt))

            let sample1 = floatData[0][min(srcIndexInt, inputFrames - 1)]
            let sample2 = floatData[0][min(srcIndexInt + 1, inputFrames - 1)]

            let interpolated = sample1 + (sample2 - sample1) * frac
            let clamped = max(-1.0, min(1.0, interpolated))
            let int16Value = Int16(clamped * 32767.0)

            withUnsafeBytes(of: int16Value.littleEndian) { pcmData.append(contentsOf: $0) }
        }

        return pcmData
    }

    /// Send PCM16 audio data to WebSocket
    private func sendAudioData(_ data: Data) async {
        audioBufferCount += 1

        if audioBufferCount == 1 || audioBufferCount % 100 == 0 {
            onDebugLog?("📤 Sent \(audioBufferCount) audio buffers")
        }

        await webSocket?.sendPCMData(data)
    }

    // MARK: - Audio Playback

    /// Play received audio data
    private func playAudioData(_ audioData: Data) {
        guard let player = playerNode, isEngineRunning else {
            onDebugLog?("⚠️ Cannot play: engine not running")
            return
        }

        // Convert PCM16 data to Float32 buffer
        guard let buffer = createPlaybackBuffer(from: audioData) else {
            onDebugLog?("⚠️ Failed to create playback buffer")
            return
        }

        playbackBufferCount += 1

        // Update output audio levels
        if let floatData = buffer.floatChannelData {
            updateOutputLevels(floatData: floatData, frameCount: Int(buffer.frameLength))
        }

        // Schedule buffer for playback
        player.scheduleBuffer(buffer, completionHandler: nil)

        // Start playing if not already
        if !player.isPlaying {
            player.play()
            onDebugLog?("🔊 Started audio playback")
        }

        if playbackBufferCount == 1 || playbackBufferCount % 20 == 0 {
            onDebugLog?("🔊 Playing buffer #\(playbackBufferCount)")
        }
    }

    /// Create playback buffer from PCM16 data
    private func createPlaybackBuffer(from data: Data) -> AVAudioPCMBuffer? {
        let bytesPerSample = 2  // Int16
        let frameCount = UInt32(data.count / bytesPerSample)

        guard let buffer = AVAudioPCMBuffer(pcmFormat: playbackFormat, frameCapacity: frameCount) else {
            return nil
        }

        buffer.frameLength = frameCount

        guard let floatChannelData = buffer.floatChannelData else {
            return nil
        }

        // Convert Int16 → Float32
        data.withUnsafeBytes { rawBuffer in
            guard let source = rawBuffer.baseAddress?.assumingMemoryBound(to: Int16.self) else { return }
            let destination = floatChannelData[0]

            for i in 0..<Int(frameCount) {
                destination[i] = Float(source[i]) / 32768.0
            }
        }

        return buffer
    }

    // MARK: - Audio Level Visualization

    private func updateInputLevels(floatData: UnsafePointer<UnsafeMutablePointer<Float>>, frameCount: Int) {
        let barCount = 40
        let samplesPerBar = max(1, frameCount / barCount)
        var newLevels: [Float] = []

        for bar in 0..<barCount {
            let startSample = bar * samplesPerBar
            let endSample = min(startSample + samplesPerBar, frameCount)

            var sumSquares: Float = 0
            for i in startSample..<endSample {
                let sample = floatData[0][i]
                sumSquares += sample * sample
            }
            let rms = sqrt(sumSquares / Float(endSample - startSample))
            let normalized = min(1.0, rms * 3.0)
            newLevels.append(normalized)
        }

        for i in 0..<barCount {
            inputAudioLevels[i] = inputAudioLevels[i] * 0.3 + newLevels[i] * 0.7
        }
    }

    private func updateOutputLevels(floatData: UnsafePointer<UnsafeMutablePointer<Float>>, frameCount: Int) {
        let barCount = 40
        let samplesPerBar = max(1, frameCount / barCount)
        var newLevels: [Float] = []

        for bar in 0..<barCount {
            let startSample = bar * samplesPerBar
            let endSample = min(startSample + samplesPerBar, frameCount)

            var sumSquares: Float = 0
            for i in startSample..<endSample {
                let sample = floatData[0][i]
                sumSquares += sample * sample
            }
            let rms = sqrt(sumSquares / Float(endSample - startSample))
            let normalized = min(1.0, rms * 3.0)
            newLevels.append(normalized)
        }

        for i in 0..<barCount {
            outputAudioLevels[i] = outputAudioLevels[i] * 0.3 + newLevels[i] * 0.7
        }
    }

    func resetAudioLevels() {
        inputAudioLevels = Array(repeating: 0, count: 40)
        outputAudioLevels = Array(repeating: 0, count: 40)
    }

    // MARK: - Microphone Permission

    private func requestMicrophonePermission() async -> Bool {
        #if os(macOS)
        let status = AVCaptureDevice.authorizationStatus(for: .audio)
        onDebugLog?("🔐 Mic permission: \(statusName(status))")

        return await withCheckedContinuation { continuation in
            switch status {
            case .authorized:
                continuation.resume(returning: true)
            case .notDetermined:
                AVCaptureDevice.requestAccess(for: .audio) { granted in
                    continuation.resume(returning: granted)
                }
            case .denied, .restricted:
                self.openMicrophoneSettings()
                continuation.resume(returning: false)
            @unknown default:
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

    private func statusName(_ status: AVAuthorizationStatus) -> String {
        switch status {
        case .notDetermined: return "notDetermined"
        case .restricted: return "restricted"
        case .denied: return "denied"
        case .authorized: return "authorized"
        @unknown default: return "unknown"
        }
    }

    private func openMicrophoneSettings() {
        #if os(macOS)
        if let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone") {
            NSWorkspace.shared.open(url)
        }
        #endif
    }

    // MARK: - State Management

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
                onDebugLog?("❌ Disconnected: \(error.localizedDescription)")
            } else {
                logger.info("OpenAI Realtime disconnected")
            }
        }
    }

    nonisolated func realtime(_ realtime: OpenAIRealtimeWebSocket, didReceiveTranscript text: String, isFinal: Bool) {
        Task { @MainActor in
            if isFinal {
                updateState(.processing)
                onDebugLog?("🎤 You: \(text)")
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
            // Play received audio immediately
            playAudioData(audioData)
        }
    }

    nonisolated func realtime(_ realtime: OpenAIRealtimeWebSocket, didCompleteResponse: Void) {
        Task { @MainActor in
            updateState(.listening)
            delegate?.voiceManager(self, didCompleteResponse: ())
            onDebugLog?("✅ Response complete")
        }
    }

    nonisolated func realtime(_ realtime: OpenAIRealtimeWebSocket, didEncounterError error: Error) {
        Task { @MainActor in
            logger.error("OpenAI Realtime error: \(error.localizedDescription)")
            onDebugLog?("❌ Error: \(error.localizedDescription)")
            delegate?.voiceManager(self, didEncounterError: error)
        }
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
