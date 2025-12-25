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

    private let audioEngine = AVAudioEngine()
    private var inputNode: AVAudioInputNode?
    private var webSocket: OpenAIRealtimeWebSocket?

    // MARK: - Playback Node (Reusable - prevents memory leak)
    // IMPORTANT: Reuse a single AVAudioPlayerNode instead of creating new ones
    // per audio chunk. Creating new nodes causes massive memory leaks (24GB+).
    private var playbackNode: AVAudioPlayerNode?
    private var isPlaybackNodeAttached: Bool = false

    private let audioFormat: AVAudioFormat
    private let logger = Logger.shared

    // Debug callback for UI
    var onDebugLog: ((String) -> Void)?

    // Audio buffer counter for debug
    private var audioBufferCount: Int = 0
    private var lastAudioLogTime: Date = Date.distantPast

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
        webSocket?.onDebugLog = { [weak self] message in
            self?.onDebugLog?(message)
        }

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

        // Cleanup playback resources to prevent memory leaks
        cleanupPlaybackNode()

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

        // Log audio device info
        #if os(macOS)
        logAudioDevices()
        #endif

        // Get native input format
        let inputFormat = inputNode.outputFormat(forBus: 0)
        onDebugLog?("🎤 Mic format: \(Int(inputFormat.sampleRate))Hz, \(inputFormat.channelCount)ch")

        // Verify format is valid
        if inputFormat.channelCount == 0 || inputFormat.sampleRate == 0 {
            onDebugLog?("❌ Invalid audio format! No input device?")
            throw VoiceError.audioEngineError("Invalid input format - no microphone available")
        }

        // Install tap with native format, convert manually
        inputNode.installTap(onBus: 0, bufferSize: 4800, format: inputFormat) { [weak self] buffer, time in
            guard let self = self else { return }
            // Convert Float32 -> Int16 and resample 48kHz -> 24kHz
            if let pcmData = self.convertToPCM16(buffer: buffer) {
                Task { @MainActor in
                    await self.sendPCMData(pcmData)
                }
            }
        }

        audioEngine.prepare()
    }

    /// Convert Float32 buffer to PCM16 Data at 24kHz
    private func convertToPCM16(buffer: AVAudioPCMBuffer) -> Data? {
        guard let floatData = buffer.floatChannelData else {
            onDebugLog?("❌ floatChannelData is nil!")
            return nil
        }

        let inputSampleRate = buffer.format.sampleRate
        let outputSampleRate = 24000.0
        let ratio = outputSampleRate / inputSampleRate

        let inputFrames = Int(buffer.frameLength)
        let outputFrames = Int(Double(inputFrames) * ratio)

        // Calculate audio levels for waveform visualization (40 bars)
        updateInputAudioLevels(floatData: floatData, frameCount: inputFrames)

        // Debug: check raw input levels
        if audioBufferCount < 5 {
            var maxVal: Float = 0
            var minVal: Float = 0
            for i in 0..<min(inputFrames, 1000) {
                let val = floatData[0][i]
                if val > maxVal { maxVal = val }
                if val < minVal { minVal = val }
            }
            onDebugLog?("🔊 Raw input: frames=\(inputFrames), levels[\(String(format: "%.6f", minVal))...\(String(format: "%.6f", maxVal))]")
        }

        var pcmData = Data(capacity: outputFrames * 2) // 2 bytes per Int16

        // Simple linear interpolation resampling + float to int16 conversion
        for i in 0..<outputFrames {
            let srcIndex = Double(i) / ratio
            let srcIndexInt = Int(srcIndex)
            let frac = Float(srcIndex - Double(srcIndexInt))

            // Get samples (handle boundary)
            let sample1 = floatData[0][min(srcIndexInt, inputFrames - 1)]
            let sample2 = floatData[0][min(srcIndexInt + 1, inputFrames - 1)]

            // Interpolate
            let interpolated = sample1 + (sample2 - sample1) * frac

            // Convert to Int16 (-32768 to 32767)
            let clamped = max(-1.0, min(1.0, interpolated))
            let int16Value = Int16(clamped * 32767.0)

            // Append as little-endian
            withUnsafeBytes(of: int16Value.littleEndian) { pcmData.append(contentsOf: $0) }
        }

        return pcmData
    }

    private func sendPCMData(_ data: Data) async {
        guard !isMuted, isConnected else { return }

        audioBufferCount += 1

        // Log with audio levels
        if audioBufferCount == 1 || audioBufferCount == 10 || audioBufferCount % 100 == 0 {
            let samples = data.withUnsafeBytes { ptr -> (Int16, Int16) in
                let int16Ptr = ptr.bindMemory(to: Int16.self)
                var maxSample: Int16 = 0
                var minSample: Int16 = 0
                for i in 0..<min(int16Ptr.count, 100) {
                    if int16Ptr[i] > maxSample { maxSample = int16Ptr[i] }
                    if int16Ptr[i] < minSample { minSample = int16Ptr[i] }
                }
                return (minSample, maxSample)
            }
            onDebugLog?("📤 #\(audioBufferCount): \(data.count)B, levels[\(samples.0)...\(samples.1)]")
        }

        await webSocket?.sendPCMData(data)
    }

    /// Calculate RMS audio levels for waveform visualization (40 bars)
    private func updateInputAudioLevels(floatData: UnsafePointer<UnsafeMutablePointer<Float>>, frameCount: Int) {
        let barCount = 40
        let samplesPerBar = max(1, frameCount / barCount)
        var newLevels: [Float] = []

        for bar in 0..<barCount {
            let startSample = bar * samplesPerBar
            let endSample = min(startSample + samplesPerBar, frameCount)

            // Calculate RMS for this segment
            var sumSquares: Float = 0
            for i in startSample..<endSample {
                let sample = floatData[0][i]
                sumSquares += sample * sample
            }
            let rms = sqrt(sumSquares / Float(endSample - startSample))

            // Normalize to 0-1 range with some amplification for visibility
            // Audio typically peaks at 0.3-0.5, so multiply by 2-3 for visual effect
            let normalized = min(1.0, rms * 3.0)
            newLevels.append(normalized)
        }

        // Update on main thread
        Task { @MainActor in
            // Apply smoothing (mix 30% old + 70% new for smoother animation)
            for i in 0..<barCount {
                self.inputAudioLevels[i] = self.inputAudioLevels[i] * 0.3 + newLevels[i] * 0.7
            }
        }
    }

    /// Update output audio levels when AI is speaking
    func updateOutputAudioLevels(from buffer: AVAudioPCMBuffer) {
        guard let floatData = buffer.floatChannelData else { return }
        let frameCount = Int(buffer.frameLength)
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

        // Apply smoothing
        for i in 0..<barCount {
            outputAudioLevels[i] = outputAudioLevels[i] * 0.3 + newLevels[i] * 0.7
        }
    }

    /// Reset audio levels to zero (when not active)
    func resetAudioLevels() {
        inputAudioLevels = Array(repeating: 0, count: 40)
        outputAudioLevels = Array(repeating: 0, count: 40)
    }

    private func processAudioBuffer(_ buffer: AVAudioPCMBuffer) async {
        guard !isMuted, isConnected else { return }

        audioBufferCount += 1

        // Log every 50 buffers (about every 2 seconds at typical rates)
        let now = Date()
        if now.timeIntervalSince(lastAudioLogTime) >= 2.0 {
            lastAudioLogTime = now
            let debugMsg = "📊 Audio: \(audioBufferCount) buffers sent, format: \(buffer.format.sampleRate)Hz"
            onDebugLog?(debugMsg)
        }

        // Convert buffer to required format if needed
        guard let convertedBuffer = convertBufferToRequiredFormat(buffer) else {
            onDebugLog?("❌ Audio conversion failed!")
            logger.error("Failed to convert audio buffer")
            return
        }

        // Send audio data to WebSocket
        await webSocket?.sendAudio(convertedBuffer)
    }

    private func convertBufferToRequiredFormat(_ buffer: AVAudioPCMBuffer) -> AVAudioPCMBuffer? {
        // Log input format once
        if audioBufferCount == 1 {
            onDebugLog?("🔊 Input format: \(buffer.format.sampleRate)Hz, \(buffer.format.channelCount)ch, \(buffer.format.commonFormat.rawValue)")
            onDebugLog?("🎯 Target format: \(audioFormat.sampleRate)Hz, \(audioFormat.channelCount)ch, PCM16")
        }

        // Check if conversion is needed
        guard buffer.format.sampleRate != audioFormat.sampleRate ||
              buffer.format.commonFormat != audioFormat.commonFormat else {
            return buffer
        }

        // Create converter
        guard let converter = AVAudioConverter(from: buffer.format, to: audioFormat) else {
            onDebugLog?("❌ Failed to create audio converter")
            logger.error("Failed to create audio converter")
            return nil
        }

        // Calculate output buffer size based on sample rate ratio
        let ratio = audioFormat.sampleRate / buffer.format.sampleRate
        let capacity = AVAudioFrameCount(Double(buffer.frameLength) * ratio)

        guard let outputBuffer = AVAudioPCMBuffer(pcmFormat: audioFormat, frameCapacity: capacity) else {
            onDebugLog?("❌ Failed to create output buffer")
            logger.error("Failed to create output buffer")
            return nil
        }

        var error: NSError?
        var hasData = true
        let inputBlock: AVAudioConverterInputBlock = { inNumPackets, outStatus in
            if hasData {
                hasData = false
                outStatus.pointee = .haveData
                return buffer
            } else {
                outStatus.pointee = .noDataNow
                return nil
            }
        }

        let status = converter.convert(to: outputBuffer, error: &error, withInputFrom: inputBlock)

        if status == .error {
            onDebugLog?("❌ Conversion error: \(error?.localizedDescription ?? "unknown")")
            logger.error("Audio conversion failed: \(error?.localizedDescription ?? "unknown error")")
            return nil
        }

        // Log conversion success once
        if audioBufferCount == 1 {
            onDebugLog?("✅ Audio conversion working: \(buffer.frameLength) → \(outputBuffer.frameLength) frames")
        }

        return outputBuffer
    }

    private func requestMicrophonePermission() async -> Bool {
        #if os(macOS)
        let status = AVCaptureDevice.authorizationStatus(for: .audio)
        onDebugLog?("🔐 Mic permission status: \(status.rawValue) (\(statusName(status)))")

        return await withCheckedContinuation { continuation in
            switch status {
            case .authorized:
                self.onDebugLog?("✅ Microphone authorized")
                continuation.resume(returning: true)
            case .notDetermined:
                self.onDebugLog?("⏳ Requesting microphone permission...")
                AVCaptureDevice.requestAccess(for: .audio) { [weak self] granted in
                    self?.onDebugLog?(granted ? "✅ Permission granted" : "❌ Permission denied")
                    continuation.resume(returning: granted)
                }
            case .denied:
                self.onDebugLog?("❌ Microphone DENIED - open System Settings!")
                self.openMicrophoneSettings()
                continuation.resume(returning: false)
            case .restricted:
                self.onDebugLog?("⚠️ Microphone restricted by system")
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

    #if os(macOS)
    private func logAudioDevices() {
        // Get default input device
        var defaultInputID: AudioDeviceID = 0
        var propertySize = UInt32(MemoryLayout<AudioDeviceID>.size)
        var propertyAddress = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDefaultInputDevice,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain
        )

        let status = AudioObjectGetPropertyData(
            AudioObjectID(kAudioObjectSystemObject),
            &propertyAddress,
            0,
            nil,
            &propertySize,
            &defaultInputID
        )

        if status == noErr {
            // Get device name
            var nameSize: UInt32 = 256
            var name = [CChar](repeating: 0, count: Int(nameSize))
            var nameAddress = AudioObjectPropertyAddress(
                mSelector: kAudioDevicePropertyDeviceNameCFString,
                mScope: kAudioObjectPropertyScopeGlobal,
                mElement: kAudioObjectPropertyElementMain
            )

            var cfName: CFString?
            var cfNameSize = UInt32(MemoryLayout<CFString?>.size)

            if AudioObjectGetPropertyData(defaultInputID, &nameAddress, 0, nil, &cfNameSize, &cfName) == noErr,
               let deviceName = cfName as String? {
                onDebugLog?("🎙️ Default input: \(deviceName) (ID: \(defaultInputID))")
            } else {
                onDebugLog?("🎙️ Default input ID: \(defaultInputID)")
            }
        } else {
            onDebugLog?("⚠️ Could not get default input device (status: \(status))")
        }
    }
    #endif

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
            // Notify delegate that response is complete
            delegate?.voiceManager(self, didCompleteResponse: ())
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

    /// Setup reusable playback node - call once before playing audio
    /// This prevents memory leaks from creating new nodes for each audio chunk
    private func setupPlaybackNodeIfNeeded() {
        guard playbackNode == nil else { return }

        // Create playback format (24kHz Float32 for output)
        guard let playbackFormat = AVAudioFormat(
            commonFormat: .pcmFormatFloat32,
            sampleRate: Self.sampleRate,
            channels: Self.channels,
            interleaved: false
        ) else {
            logger.error("Failed to create playback format")
            return
        }

        let node = AVAudioPlayerNode()
        playbackNode = node

        // Attach to engine
        audioEngine.attach(node)
        isPlaybackNodeAttached = true

        // Connect to output mixer
        audioEngine.connect(node, to: audioEngine.mainMixerNode, format: playbackFormat)

        logger.info("Playback node created and attached (reusable)")
    }

    /// Play audio data using the reusable player node
    /// IMPORTANT: This reuses a single AVAudioPlayerNode to prevent memory leaks.
    /// Previous implementation created a new node for EVERY audio chunk, causing 24GB+ memory usage.
    private func playAudio(_ audioData: Data) async {
        // Ensure playback node is ready
        setupPlaybackNodeIfNeeded()

        guard let node = playbackNode else {
            logger.error("Playback node not available")
            return
        }

        // Create audio buffer from data
        guard let buffer = createAudioBuffer(from: audioData) else {
            logger.error("Failed to create audio buffer for playback")
            return
        }

        // Update output audio levels for waveform visualization
        updateOutputAudioLevels(from: buffer)

        // Schedule buffer on the REUSABLE node (don't create new nodes!)
        // Using completion handler without .interrupts to allow buffer queue
        node.scheduleBuffer(buffer, completionHandler: nil)

        // Start playing if not already
        if !node.isPlaying {
            node.play()
        }
    }

    /// Stop playback and reset node
    func stopPlayback() {
        playbackNode?.stop()
    }

    /// Cleanup playback resources when disconnecting
    private func cleanupPlaybackNode() {
        if let node = playbackNode, isPlaybackNodeAttached {
            node.stop()
            audioEngine.disconnectNodeOutput(node)
            audioEngine.detach(node)
            isPlaybackNodeAttached = false
        }
        playbackNode = nil
    }

    private func createAudioBuffer(from data: Data) -> AVAudioPCMBuffer? {
        // Calculate frame count from PCM16 data
        let bytesPerSample = 2 // Int16 = 2 bytes
        let frameCount = UInt32(data.count / bytesPerSample)

        // Create output format (Float32 for playback)
        guard let playbackFormat = AVAudioFormat(
            commonFormat: .pcmFormatFloat32,
            sampleRate: Self.sampleRate,
            channels: Self.channels,
            interleaved: false
        ) else {
            return nil
        }

        guard let buffer = AVAudioPCMBuffer(pcmFormat: playbackFormat, frameCapacity: frameCount) else {
            return nil
        }

        buffer.frameLength = frameCount

        // Convert Int16 PCM data to Float32
        guard let floatChannelData = buffer.floatChannelData else {
            return nil
        }

        data.withUnsafeBytes { rawBuffer in
            guard let source = rawBuffer.baseAddress?.assumingMemoryBound(to: Int16.self) else { return }
            let destination = floatChannelData[0]

            // Convert Int16 (-32768 to 32767) to Float (-1.0 to 1.0)
            for i in 0..<Int(frameCount) {
                destination[i] = Float(source[i]) / 32768.0
            }
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
