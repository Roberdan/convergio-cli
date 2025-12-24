/**
 * CONVERGIO NATIVE - OpenAI Realtime WebSocket
 *
 * WebSocket implementation for OpenAI Realtime Audio API.
 * Provides real-time voice conversation with GPT-4o.
 *
 * Based on: https://platform.openai.com/docs/guides/realtime
 * Model: gpt-4o-realtime-preview-2024-12-17
 * Audio: PCM16 @ 24kHz
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import AVFoundation

// MARK: - OpenAI Voice

enum OpenAIVoice: String, CaseIterable {
    case alloy = "alloy"
    case ash = "ash"
    case ballad = "ballad"
    case coral = "coral"
    case echo = "echo"
    case sage = "sage"
    case shimmer = "shimmer"
    case verse = "verse"

    var displayName: String {
        rawValue.capitalized
    }
}

// MARK: - Delegate Protocol

protocol OpenAIRealtimeDelegate: AnyObject {
    func realtimeDidConnect(_ realtime: OpenAIRealtimeWebSocket)
    func realtimeDidDisconnect(_ realtime: OpenAIRealtimeWebSocket, error: Error?)
    func realtime(_ realtime: OpenAIRealtimeWebSocket, didReceiveTranscript text: String, isFinal: Bool)
    func realtime(_ realtime: OpenAIRealtimeWebSocket, didReceiveResponse text: String)
    func realtime(_ realtime: OpenAIRealtimeWebSocket, didReceiveAudio audioData: Data)
    func realtime(_ realtime: OpenAIRealtimeWebSocket, didCompleteResponse: Void)
    func realtime(_ realtime: OpenAIRealtimeWebSocket, didEncounterError error: Error)
}

// MARK: - Audio Buffer Actor (Thread-safe)

private actor AudioBufferActor {
    private var buffer = Data()

    func append(_ data: Data) {
        buffer.append(data)
    }

    func clear() {
        buffer.removeAll()
    }

    func getData() -> Data {
        return buffer
    }
}

// MARK: - OpenAI Realtime WebSocket

/// Voice provider type
enum VoiceProviderType {
    case openAI
    case azure(endpoint: String, deployment: String)
}

final class OpenAIRealtimeWebSocket: NSObject {

    // MARK: - Properties

    weak var delegate: OpenAIRealtimeDelegate?

    private let apiKey: String
    private let providerType: VoiceProviderType
    private var webSocketTask: URLSessionWebSocketTask?
    private var urlSession: URLSession?
    private var isConnected = false

    private let model = "gpt-4o-realtime-preview-2024-12-17"
    private let openAIRealtimeURL = "wss://api.openai.com/v1/realtime"
    private let azureAPIVersion = "2024-10-01-preview"

    // Voice configuration
    private(set) var currentVoice: OpenAIVoice = .sage
    private(set) var systemPrompt: String = ""

    // Audio buffer for collecting audio chunks (actor-based for thread safety)
    private let audioBuffer = AudioBufferActor()

    // Connection state tracking
    private var connectionContinuation: CheckedContinuation<Void, Error>?
    private var connectionError: Error?

    // MARK: - Initialization

    /// Initialize with direct OpenAI API key
    init(apiKey: String) {
        self.apiKey = apiKey
        self.providerType = .openAI
        super.init()
    }

    /// Initialize with Azure OpenAI configuration
    init(azureApiKey: String, endpoint: String, deployment: String) {
        self.apiKey = azureApiKey
        self.providerType = .azure(endpoint: endpoint, deployment: deployment)
        super.init()
    }

    // MARK: - Connection

    func connect(voice: OpenAIVoice = .sage, systemPrompt: String = "") async throws {
        guard !isConnected else {
            logWarning("OpenAI Realtime: Already connected")
            return
        }

        // Validate API key
        guard !apiKey.isEmpty else {
            logError("OpenAI Realtime: API key is empty")
            throw OpenAIRealtimeError.serverError("API key is not configured. Please add your API key in Settings → Providers.")
        }

        self.currentVoice = voice
        self.systemPrompt = systemPrompt

        // Build URL and request based on provider type
        let request: URLRequest
        switch providerType {
        case .openAI:
            // Validate OpenAI key format
            guard apiKey.hasPrefix("sk-") else {
                logError("OpenAI Realtime: Invalid API key format")
                throw OpenAIRealtimeError.serverError("Invalid OpenAI API key format. Key should start with 'sk-'")
            }

            logInfo("OpenAI Realtime: Connecting to OpenAI...")
            logInfo("OpenAI Realtime: Using voice: \(voice.rawValue)")

            guard var urlComponents = URLComponents(string: openAIRealtimeURL) else {
                throw OpenAIRealtimeError.invalidURL
            }
            urlComponents.queryItems = [URLQueryItem(name: "model", value: model)]

            guard let url = urlComponents.url else {
                throw OpenAIRealtimeError.invalidURL
            }

            var req = URLRequest(url: url)
            req.setValue("Bearer \(apiKey)", forHTTPHeaderField: "Authorization")
            req.setValue("realtime=v1", forHTTPHeaderField: "OpenAI-Beta")
            request = req

        case .azure(let endpoint, let deployment):
            logInfo("OpenAI Realtime: Connecting to Azure OpenAI...")
            logInfo("OpenAI Realtime: Endpoint: \(endpoint), Deployment: \(deployment)")
            logInfo("OpenAI Realtime: Using voice: \(voice.rawValue)")

            // Azure URL format: wss://endpoint/openai/realtime?api-version=X&deployment=Y
            let cleanEndpoint = endpoint.trimmingCharacters(in: CharacterSet(charactersIn: "/"))
            let wsEndpoint = cleanEndpoint.replacingOccurrences(of: "https://", with: "wss://")
            let urlString = "\(wsEndpoint)/openai/realtime?api-version=\(azureAPIVersion)&deployment=\(deployment)"

            guard let url = URL(string: urlString) else {
                logError("OpenAI Realtime: Invalid Azure URL: \(urlString)")
                throw OpenAIRealtimeError.invalidURL
            }

            var req = URLRequest(url: url)
            req.setValue(apiKey, forHTTPHeaderField: "api-key")
            request = req
        }

        // Reset connection state
        connectionError = nil

        // Create URL session and WebSocket task
        let config = URLSessionConfiguration.default
        config.timeoutIntervalForRequest = 30
        config.timeoutIntervalForResource = 60
        urlSession = URLSession(configuration: config, delegate: self, delegateQueue: nil)
        webSocketTask = urlSession?.webSocketTask(with: request)

        webSocketTask?.resume()

        // Wait for connection with proper async handling
        try await waitForConnection()

        // Check for connection errors
        if let error = connectionError {
            throw error
        }

        // Configure session
        try await configureSession()

        isConnected = true
        logInfo("OpenAI Realtime: Connected successfully")

        // Start receiving messages
        Task { await receiveMessages() }

        delegate?.realtimeDidConnect(self)
    }

    func disconnect() async {
        guard isConnected else { return }

        logInfo("OpenAI Realtime: Disconnecting...")

        webSocketTask?.cancel(with: .normalClosure, reason: nil)
        webSocketTask = nil
        urlSession?.invalidateAndCancel()
        urlSession = nil
        isConnected = false

        delegate?.realtimeDidDisconnect(self, error: nil)
        logInfo("OpenAI Realtime: Disconnected")
    }

    // MARK: - Session Configuration

    private func configureSession() async throws {
        let sessionConfig: [String: Any] = [
            "type": "session.update",
            "session": [
                "modalities": ["text", "audio"],
                "instructions": systemPrompt.isEmpty ? getDefaultInstructions() : systemPrompt,
                "voice": currentVoice.rawValue,
                "input_audio_format": "pcm16",
                "output_audio_format": "pcm16",
                "input_audio_transcription": [
                    "model": "whisper-1"
                ],
                "turn_detection": [
                    "type": "server_vad",
                    "threshold": 0.5,
                    "prefix_padding_ms": 300,
                    "silence_duration_ms": 500
                ],
                "temperature": 0.8,
                "max_response_output_tokens": 4096
            ]
        ]

        try await sendJSON(sessionConfig)
        logInfo("OpenAI Realtime: Session configured with voice: \(currentVoice.rawValue)")
    }

    private func getDefaultInstructions() -> String {
        """
        You are a friendly and knowledgeable educational tutor helping Italian students learn.
        Speak naturally and conversationally. Be encouraging and patient.
        When explaining concepts, use simple language and relatable examples.
        If a student seems confused, try a different approach or break down the concept further.
        Always respond in the same language the student uses.
        """
    }

    // MARK: - Audio Handling

    func sendAudio(_ buffer: AVAudioPCMBuffer) async {
        guard isConnected else { return }

        // Convert buffer to PCM16 data
        guard let audioData = convertBufferToData(buffer) else {
            logError("OpenAI Realtime: Failed to convert audio buffer")
            return
        }

        // Encode as base64
        let base64Audio = audioData.base64EncodedString()

        // Send audio append event
        let audioEvent: [String: Any] = [
            "type": "input_audio_buffer.append",
            "audio": base64Audio
        ]

        do {
            try await sendJSON(audioEvent)
        } catch {
            logError("OpenAI Realtime: Failed to send audio: \(error.localizedDescription)")
        }
    }

    func commitAudio() async {
        guard isConnected else { return }

        let commitEvent: [String: Any] = [
            "type": "input_audio_buffer.commit"
        ]

        do {
            try await sendJSON(commitEvent)
            logDebug("OpenAI Realtime: Audio committed")
        } catch {
            logError("OpenAI Realtime: Failed to commit audio: \(error.localizedDescription)")
        }
    }

    func cancelResponse() async {
        guard isConnected else { return }

        let cancelEvent: [String: Any] = [
            "type": "response.cancel"
        ]

        do {
            try await sendJSON(cancelEvent)
            logInfo("OpenAI Realtime: Response cancelled (barge-in)")
        } catch {
            logError("OpenAI Realtime: Failed to cancel response: \(error.localizedDescription)")
        }
    }

    func sendText(_ text: String) async {
        guard isConnected else { return }

        // Create conversation item
        let textEvent: [String: Any] = [
            "type": "conversation.item.create",
            "item": [
                "type": "message",
                "role": "user",
                "content": [
                    [
                        "type": "input_text",
                        "text": text
                    ]
                ]
            ]
        ]

        do {
            try await sendJSON(textEvent)

            // Request response
            let responseEvent: [String: Any] = [
                "type": "response.create"
            ]
            try await sendJSON(responseEvent)
        } catch {
            logError("OpenAI Realtime: Failed to send text: \(error.localizedDescription)")
        }
    }

    // MARK: - Private Methods

    private func waitForConnection() async throws {
        // Wait for WebSocket to connect with timeout
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            self.connectionContinuation = continuation

            // Set timeout for connection
            Task {
                try await Task.sleep(nanoseconds: 10_000_000_000) // 10 seconds timeout
                if let cont = self.connectionContinuation {
                    self.connectionContinuation = nil
                    cont.resume(throwing: OpenAIRealtimeError.serverError("Connection timeout. Please check your internet connection and API key."))
                }
            }
        }
    }

    private func sendJSON(_ json: [String: Any]) async throws {
        guard let webSocketTask = webSocketTask else {
            throw OpenAIRealtimeError.notConnected
        }

        let data = try JSONSerialization.data(withJSONObject: json)
        guard let jsonString = String(data: data, encoding: .utf8) else {
            throw OpenAIRealtimeError.encodingError
        }

        let message = URLSessionWebSocketTask.Message.string(jsonString)
        try await webSocketTask.send(message)
    }

    private func receiveMessages() async {
        guard let webSocketTask = webSocketTask else { return }

        do {
            while isConnected {
                let message = try await webSocketTask.receive()

                switch message {
                case .string(let text):
                    await handleMessage(text)
                case .data(let data):
                    if let text = String(data: data, encoding: .utf8) {
                        await handleMessage(text)
                    }
                @unknown default:
                    break
                }
            }
        } catch {
            if isConnected {
                logError("OpenAI Realtime: Receive error: \(error.localizedDescription)")
                delegate?.realtime(self, didEncounterError: error)
            }
        }
    }

    private func handleMessage(_ text: String) async {
        guard let data = text.data(using: .utf8),
              let json = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let type = json["type"] as? String else {
            return
        }

        switch type {
        case "session.created", "session.updated":
            logDebug("OpenAI Realtime: Session event - \(type)")

        case "input_audio_buffer.speech_started":
            logDebug("OpenAI Realtime: Speech started")

        case "input_audio_buffer.speech_stopped":
            logDebug("OpenAI Realtime: Speech stopped")

        case "conversation.item.input_audio_transcription.completed":
            if let transcript = json["transcript"] as? String {
                logDebug("OpenAI Realtime: Transcript: \(transcript)")
                delegate?.realtime(self, didReceiveTranscript: transcript, isFinal: true)
            }

        case "response.audio_transcript.delta":
            if let delta = json["delta"] as? String {
                delegate?.realtime(self, didReceiveResponse: delta)
            }

        case "response.audio_transcript.done":
            if let transcript = json["transcript"] as? String {
                logDebug("OpenAI Realtime: Response transcript done: \(transcript)")
            }

        case "response.audio.delta":
            if let audioBase64 = json["delta"] as? String,
               let audioData = Data(base64Encoded: audioBase64) {
                await audioBuffer.append(audioData)
                delegate?.realtime(self, didReceiveAudio: audioData)
            }

        case "response.audio.done":
            logDebug("OpenAI Realtime: Audio response complete")
            await audioBuffer.clear()

        case "response.done":
            logDebug("OpenAI Realtime: Response complete")
            delegate?.realtime(self, didCompleteResponse: ())

        case "error":
            if let error = json["error"] as? [String: Any],
               let message = error["message"] as? String {
                logError("OpenAI Realtime: Error - \(message)")
                delegate?.realtime(self, didEncounterError: OpenAIRealtimeError.serverError(message))
            }

        default:
            logDebug("OpenAI Realtime: Unhandled event - \(type)")
        }
    }

    private func convertBufferToData(_ buffer: AVAudioPCMBuffer) -> Data? {
        guard let channelData = buffer.int16ChannelData else { return nil }

        let frameLength = Int(buffer.frameLength)
        let channelCount = Int(buffer.format.channelCount)

        // For mono, just copy the data directly
        if channelCount == 1 {
            return Data(bytes: channelData[0], count: frameLength * MemoryLayout<Int16>.size)
        }

        // For stereo, mix down to mono
        var monoData = [Int16](repeating: 0, count: frameLength)
        for frame in 0..<frameLength {
            var sum: Int32 = 0
            for channel in 0..<channelCount {
                sum += Int32(channelData[channel][frame])
            }
            monoData[frame] = Int16(sum / Int32(channelCount))
        }

        return Data(bytes: monoData, count: frameLength * MemoryLayout<Int16>.size)
    }
}

// MARK: - URLSessionWebSocketDelegate

extension OpenAIRealtimeWebSocket: URLSessionWebSocketDelegate {

    func urlSession(_ session: URLSession, webSocketTask: URLSessionWebSocketTask, didOpenWithProtocol protocol: String?) {
        logInfo("OpenAI Realtime: WebSocket opened successfully")

        // Resume the continuation on successful connection
        if let continuation = connectionContinuation {
            connectionContinuation = nil
            continuation.resume()
        }
    }

    func urlSession(_ session: URLSession, webSocketTask: URLSessionWebSocketTask, didCloseWith closeCode: URLSessionWebSocketTask.CloseCode, reason: Data?) {
        isConnected = false
        let reasonString = reason.flatMap { String(data: $0, encoding: .utf8) } ?? "Unknown"
        logInfo("OpenAI Realtime: WebSocket closed - Code: \(closeCode.rawValue) - Reason: \(reasonString)")

        // If we have a pending continuation, it means connection failed during initial connect
        if let continuation = connectionContinuation {
            connectionContinuation = nil
            let errorMessage: String
            switch closeCode {
            case .normalClosure:
                errorMessage = "Connection closed normally"
            case .goingAway:
                errorMessage = "Server is going away"
            case .protocolError:
                errorMessage = "Protocol error - check API key format"
            case .unsupportedData:
                errorMessage = "Unsupported data format"
            case .noStatusReceived:
                errorMessage = "No status received from server"
            case .abnormalClosure:
                errorMessage = "Connection closed abnormally - possible authentication failure. Check your OpenAI API key."
            case .invalidFramePayloadData:
                errorMessage = "Invalid data received"
            case .policyViolation:
                errorMessage = "Policy violation - API key may be invalid or expired"
            case .messageTooBig:
                errorMessage = "Message too large"
            case .mandatoryExtensionMissing:
                errorMessage = "Required extension missing"
            case .internalServerError:
                errorMessage = "OpenAI server error"
            case .tlsHandshakeFailure:
                errorMessage = "TLS handshake failed - network security issue"
            @unknown default:
                errorMessage = "Unknown connection error (code: \(closeCode.rawValue))"
            }
            continuation.resume(throwing: OpenAIRealtimeError.serverError(errorMessage))
        }
    }

    func urlSession(_ session: URLSession, task: URLSessionTask, didCompleteWithError error: Error?) {
        if let error = error {
            logError("OpenAI Realtime: Task completed with error: \(error.localizedDescription)")

            // Resume continuation with error if still pending
            if let continuation = connectionContinuation {
                connectionContinuation = nil
                let nsError = error as NSError
                let errorMessage: String
                if nsError.domain == NSURLErrorDomain {
                    switch nsError.code {
                    case NSURLErrorNotConnectedToInternet:
                        errorMessage = "No internet connection"
                    case NSURLErrorTimedOut:
                        errorMessage = "Connection timed out"
                    case NSURLErrorCannotConnectToHost:
                        errorMessage = "Cannot connect to OpenAI server"
                    case NSURLErrorNetworkConnectionLost:
                        errorMessage = "Network connection lost"
                    case NSURLErrorSecureConnectionFailed:
                        errorMessage = "Secure connection failed"
                    default:
                        errorMessage = "Network error: \(error.localizedDescription)"
                    }
                } else {
                    errorMessage = error.localizedDescription
                }
                continuation.resume(throwing: OpenAIRealtimeError.serverError(errorMessage))
            }

            connectionError = error
        }
    }
}

// MARK: - Errors

enum OpenAIRealtimeError: LocalizedError {
    case invalidURL
    case notConnected
    case encodingError
    case serverError(String)
    case connectionFailed

    var errorDescription: String? {
        switch self {
        case .invalidURL:
            return "Invalid OpenAI Realtime URL"
        case .notConnected:
            return "Not connected to OpenAI Realtime"
        case .encodingError:
            return "Failed to encode message"
        case .serverError(let message):
            return "Server error: \(message)"
        case .connectionFailed:
            return "Failed to connect to OpenAI Realtime"
        }
    }
}
