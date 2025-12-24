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

final class OpenAIRealtimeWebSocket: NSObject {

    // MARK: - Properties

    weak var delegate: OpenAIRealtimeDelegate?

    private let apiKey: String
    private var webSocketTask: URLSessionWebSocketTask?
    private var urlSession: URLSession?
    private var isConnected = false

    private let model = "gpt-4o-realtime-preview-2024-12-17"
    private let realtimeURL = "wss://api.openai.com/v1/realtime"

    // Voice configuration
    private(set) var currentVoice: OpenAIVoice = .sage
    private(set) var systemPrompt: String = ""

    // Audio buffer for collecting audio chunks (actor-based for thread safety)
    private let audioBuffer = AudioBufferActor()

    // MARK: - Initialization

    init(apiKey: String) {
        self.apiKey = apiKey
        super.init()
    }

    // MARK: - Connection

    func connect(voice: OpenAIVoice = .sage, systemPrompt: String = "") async throws {
        guard !isConnected else {
            logWarning("OpenAI Realtime: Already connected")
            return
        }

        self.currentVoice = voice
        self.systemPrompt = systemPrompt

        logInfo("OpenAI Realtime: Connecting to \(realtimeURL)...")

        // Build URL with model parameter
        guard var urlComponents = URLComponents(string: realtimeURL) else {
            throw OpenAIRealtimeError.invalidURL
        }
        urlComponents.queryItems = [URLQueryItem(name: "model", value: model)]

        guard let url = urlComponents.url else {
            throw OpenAIRealtimeError.invalidURL
        }

        // Create request with authorization
        var request = URLRequest(url: url)
        request.setValue("Bearer \(apiKey)", forHTTPHeaderField: "Authorization")
        request.setValue("realtime=v1", forHTTPHeaderField: "OpenAI-Beta")

        // Create URL session and WebSocket task
        let config = URLSessionConfiguration.default
        urlSession = URLSession(configuration: config, delegate: self, delegateQueue: nil)
        webSocketTask = urlSession?.webSocketTask(with: request)

        webSocketTask?.resume()

        // Wait for connection
        try await waitForConnection()

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
        // Simple ping to verify connection
        try await Task.sleep(nanoseconds: 500_000_000) // 0.5 seconds
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
        logInfo("OpenAI Realtime: WebSocket opened")
    }

    func urlSession(_ session: URLSession, webSocketTask: URLSessionWebSocketTask, didCloseWith closeCode: URLSessionWebSocketTask.CloseCode, reason: Data?) {
        isConnected = false
        let reasonString = reason.flatMap { String(data: $0, encoding: .utf8) } ?? "Unknown"
        logInfo("OpenAI Realtime: WebSocket closed - \(closeCode) - \(reasonString)")
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
