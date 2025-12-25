/**
 * CONVERGIO NATIVE - Conversation ViewModel
 *
 * Manages conversation state, message history, and streaming.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

@MainActor
public final class ConversationViewModel: ObservableObject {
    // MARK: - Published Properties

    @Published public private(set) var messages: [Message] = []
    @Published public private(set) var isProcessing = false
    @Published public private(set) var isStreaming = false
    @Published public private(set) var streamingText = ""
    @Published public private(set) var error: Error?

    // MARK: - Private Properties

    private let orchestrator: Orchestrator
    private var nextMessageId: UInt64 = 1

    // MARK: - Initialization

    public init(orchestrator: Orchestrator = .shared) {
        self.orchestrator = orchestrator
    }

    // MARK: - Message Sending

    /// Send a message and get a streamed response
    public func send(_ input: String) async {
        guard !isProcessing else { return }
        guard orchestrator.isReady else {
            error = OrchestratorError.notInitialized
            return
        }

        // Add user message
        let userMessage = Message(
            id: nextMessageId,
            type: .userInput,
            senderId: 0,
            content: input,
            timestamp: Date()
        )
        nextMessageId += 1
        messages.append(userMessage)

        // Start processing
        isProcessing = true
        isStreaming = true
        streamingText = ""
        error = nil

        do {
            let response = try await orchestrator.sendStreaming(input) { [weak self] chunk in
                // Use DispatchQueue for immediate main thread execution
                DispatchQueue.main.async {
                    self?.streamingText += chunk
                }
            }

            // Add response message
            let responseMessage = Message(
                id: nextMessageId,
                type: .agentResponse,
                senderId: 1,
                content: response,
                timestamp: Date()
            )
            nextMessageId += 1
            messages.append(responseMessage)

        } catch {
            self.error = error

            // Add error message
            let errorMessage = Message(
                id: nextMessageId,
                type: .error,
                senderId: 0,
                content: "Error: \(error.localizedDescription)",
                timestamp: Date()
            )
            nextMessageId += 1
            messages.append(errorMessage)
        }

        isProcessing = false
        isStreaming = false
        streamingText = ""
    }

    /// Send without streaming (for simple requests)
    public func sendSimple(_ input: String) async {
        guard !isProcessing else { return }
        guard orchestrator.isReady else {
            error = OrchestratorError.notInitialized
            return
        }

        // Add user message
        let userMessage = Message(
            id: nextMessageId,
            type: .userInput,
            senderId: 0,
            content: input,
            timestamp: Date()
        )
        nextMessageId += 1
        messages.append(userMessage)

        isProcessing = true
        error = nil

        do {
            let response = try await orchestrator.send(input)

            let responseMessage = Message(
                id: nextMessageId,
                type: .agentResponse,
                senderId: 1,
                content: response,
                timestamp: Date()
            )
            nextMessageId += 1
            messages.append(responseMessage)

        } catch {
            self.error = error
        }

        isProcessing = false
    }

    // MARK: - Cancellation

    /// Cancel the current request
    public func cancel() {
        orchestrator.cancel()
        isProcessing = false
        isStreaming = false
        streamingText = ""
    }

    // MARK: - History Management

    /// Start a new conversation
    public func newConversation() {
        messages.removeAll()
        streamingText = ""
        error = nil
    }

    /// Clear all history
    public func clearHistory() {
        messages.removeAll()
        streamingText = ""
        error = nil
        nextMessageId = 1
    }

    /// Remove a specific message
    public func removeMessage(_ message: Message) {
        messages.removeAll { $0.id == message.id }
    }

    // MARK: - Convergence

    /// Request parallel analysis from multiple agents
    public func requestConvergence(_ input: String, agents: [String]) async {
        guard !isProcessing else { return }
        guard orchestrator.isReady else {
            error = OrchestratorError.notInitialized
            return
        }

        // Add user message
        let userMessage = Message(
            id: nextMessageId,
            type: .userInput,
            senderId: 0,
            content: input,
            timestamp: Date()
        )
        nextMessageId += 1
        messages.append(userMessage)

        isProcessing = true
        error = nil

        do {
            let response = try await orchestrator.parallelAnalyze(input, agents: agents)

            let responseMessage = Message(
                id: nextMessageId,
                type: .convergence,
                senderId: 1,
                content: response,
                timestamp: Date()
            )
            nextMessageId += 1
            messages.append(responseMessage)

        } catch {
            self.error = error

            let errorMessage = Message(
                id: nextMessageId,
                type: .error,
                senderId: 0,
                content: "Convergence error: \(error.localizedDescription)",
                timestamp: Date()
            )
            nextMessageId += 1
            messages.append(errorMessage)
        }

        isProcessing = false
    }
}

// MARK: - Preview Helper

extension ConversationViewModel {
    /// Preview instance with mock data
    public static var preview: ConversationViewModel {
        let vm = ConversationViewModel()
        vm.messages = Message.previewMessages
        return vm
    }
}
