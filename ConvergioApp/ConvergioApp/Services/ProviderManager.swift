/**
 * CONVERGIO NATIVE - Provider Manager
 *
 * Manages AI provider selection and fallback chain.
 * Fallback order: Azure OpenAI -> OpenAI -> Local (say command)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation

// MARK: - Provider Protocol

/// Protocol for AI providers
@MainActor
public protocol AIProvider {
    var providerName: String { get }
    var isAvailable: Bool { get }

    func sendPrompt(
        _ prompt: String,
        systemPrompt: String?,
        temperature: Double
    ) async throws -> String

    func streamPrompt(
        _ prompt: String,
        systemPrompt: String?,
        temperature: Double,
        onChunk: @escaping (String) -> Void
    ) async throws -> Void
}

// MARK: - Provider Type

/// Available AI provider types
public enum ProviderType: String, CaseIterable, Identifiable {
    case azure = "Azure OpenAI"
    case openai = "OpenAI"
    case local = "Local (say)"

    public var id: String { rawValue }

    public var displayName: String { rawValue }

    public var icon: String {
        switch self {
        case .azure: return "cloud"
        case .openai: return "sparkles"
        case .local: return "speaker.wave.2"
        }
    }

    public var color: String {
        switch self {
        case .azure: return "blue"
        case .openai: return "green"
        case .local: return "gray"
        }
    }
}

// MARK: - Azure OpenAI Provider Adapter

/// Adapter for Azure OpenAI provider
@MainActor
private struct AzureProvider: AIProvider {
    let providerName = "Azure OpenAI"

    var isAvailable: Bool {
        AzureOpenAIProvider.shared.isConfigured
    }

    func sendPrompt(_ prompt: String, systemPrompt: String?, temperature: Double) async throws -> String {
        try await AzureOpenAIProvider.shared.sendPrompt(prompt, systemPrompt: systemPrompt)
    }

    func streamPrompt(_ prompt: String, systemPrompt: String?, temperature: Double, onChunk: @escaping (String) -> Void) async throws {
        var messages: [[String: String]] = []
        if let systemPrompt = systemPrompt {
            messages.append(["role": "system", "content": systemPrompt])
        }
        messages.append(["role": "user", "content": prompt])

        _ = try await AzureOpenAIProvider.shared.streamChatCompletion(
            messages: messages,
            temperature: temperature,
            onChunk: onChunk
        )
    }
}

// MARK: - OpenAI Provider Adapter

/// Adapter for OpenAI provider (placeholder - to be implemented)
@MainActor
private struct OpenAIProvider: AIProvider {
    let providerName = "OpenAI"

    var isAvailable: Bool {
        // Check if OpenAI key is available
        KeychainManager.shared.getKey(for: .openai) != nil
    }

    func sendPrompt(_ prompt: String, systemPrompt: String?, temperature: Double) async throws -> String {
        // TODO: Implement OpenAI API client
        throw ProviderError.notImplemented(provider: providerName)
    }

    func streamPrompt(_ prompt: String, systemPrompt: String?, temperature: Double, onChunk: @escaping (String) -> Void) async throws {
        // TODO: Implement OpenAI streaming
        throw ProviderError.notImplemented(provider: providerName)
    }
}

// MARK: - Local Provider (say command)

/// Local macOS say command as fallback
@MainActor
private struct LocalProvider: AIProvider {
    let providerName = "Local (say)"

    var isAvailable: Bool {
        true // Always available on macOS
    }

    func sendPrompt(_ prompt: String, systemPrompt: String?, temperature: Double) async throws -> String {
        // For local provider, we just speak the prompt
        // This is a fallback when no AI provider is available
        let message = "Local provider cannot generate text. Available providers: Azure OpenAI, OpenAI."

        // Speak the message
        try await speak(message)

        return message
    }

    func streamPrompt(_ prompt: String, systemPrompt: String?, temperature: Double, onChunk: @escaping (String) -> Void) async throws {
        let message = "Local provider cannot generate text. Available providers: Azure OpenAI, OpenAI."
        onChunk(message)
        try await speak(message)
    }

    private func speak(_ text: String) async throws {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/say")
        process.arguments = [text]

        try process.run()
        process.waitUntilExit()
    }
}

// MARK: - Provider Manager

/// Manages AI providers and fallback chain
@MainActor
public final class ProviderManager: ObservableObject {
    public static let shared = ProviderManager()

    @Published public var preferredProvider: ProviderType = .azure
    @Published public private(set) var currentProvider: ProviderType?
    @Published public private(set) var lastError: String?
    @Published public private(set) var fallbackAttempts: Int = 0

    private let providers: [ProviderType: AIProvider] = [
        .azure: AzureProvider(),
        .openai: OpenAIProvider(),
        .local: LocalProvider()
    ]

    /// Default fallback chain
    private let defaultFallbackChain: [ProviderType] = [.azure, .openai, .local]

    private init() {
        logInfo("ProviderManager initialized", category: "ProviderManager")
    }

    // MARK: - Provider Selection

    /// Get fallback chain based on preference
    private func getFallbackChain() -> [ProviderType] {
        // Start with preferred provider
        var chain = [preferredProvider]

        // Add other providers in default order
        for provider in defaultFallbackChain {
            if provider != preferredProvider {
                chain.append(provider)
            }
        }

        return chain
    }

    /// Check if a provider is available
    public func isProviderAvailable(_ type: ProviderType) -> Bool {
        providers[type]?.isAvailable ?? false
    }

    /// Get available providers
    public var availableProviders: [ProviderType] {
        ProviderType.allCases.filter { isProviderAvailable($0) }
    }

    // MARK: - Send Prompt (with fallback)

    /// Send a prompt with automatic fallback
    public func sendPrompt(
        _ prompt: String,
        systemPrompt: String? = nil,
        temperature: Double = 0.7
    ) async throws -> (response: String, provider: ProviderType) {
        fallbackAttempts = 0
        lastError = nil

        let chain = getFallbackChain()

        for providerType in chain {
            guard let provider = providers[providerType] else { continue }

            // Skip unavailable providers
            guard provider.isAvailable else {
                logInfo("Skipping unavailable provider: \(providerType.displayName)", category: "ProviderManager")
                continue
            }

            fallbackAttempts += 1

            do {
                logInfo("Attempting provider: \(providerType.displayName) (attempt \(fallbackAttempts))", category: "ProviderManager")

                let response = try await provider.sendPrompt(
                    prompt,
                    systemPrompt: systemPrompt,
                    temperature: temperature
                )

                currentProvider = providerType
                logInfo("Success with provider: \(providerType.displayName)", category: "ProviderManager")

                return (response, providerType)
            } catch {
                let errorMessage = error.localizedDescription
                lastError = errorMessage
                logWarning("Provider \(providerType.displayName) failed: \(errorMessage)", category: "ProviderManager")

                // Continue to next provider in chain
                continue
            }
        }

        // All providers failed
        throw ProviderError.allProvidersFailed(lastError: lastError)
    }

    // MARK: - Stream Prompt (with fallback)

    /// Stream a prompt with automatic fallback
    public func streamPrompt(
        _ prompt: String,
        systemPrompt: String? = nil,
        temperature: Double = 0.7,
        onChunk: @escaping (String) -> Void
    ) async throws -> ProviderType {
        fallbackAttempts = 0
        lastError = nil

        let chain = getFallbackChain()

        for providerType in chain {
            guard let provider = providers[providerType] else { continue }

            // Skip unavailable providers
            guard provider.isAvailable else {
                logInfo("Skipping unavailable provider: \(providerType.displayName)", category: "ProviderManager")
                continue
            }

            fallbackAttempts += 1

            do {
                logInfo("Attempting streaming with provider: \(providerType.displayName)", category: "ProviderManager")

                try await provider.streamPrompt(
                    prompt,
                    systemPrompt: systemPrompt,
                    temperature: temperature,
                    onChunk: onChunk
                )

                currentProvider = providerType
                logInfo("Streaming success with provider: \(providerType.displayName)", category: "ProviderManager")

                return providerType
            } catch {
                let errorMessage = error.localizedDescription
                lastError = errorMessage
                logWarning("Streaming with provider \(providerType.displayName) failed: \(errorMessage)", category: "ProviderManager")

                // Continue to next provider in chain
                continue
            }
        }

        // All providers failed
        throw ProviderError.allProvidersFailed(lastError: lastError)
    }

    // MARK: - Provider Status

    /// Get status for all providers
    public func getProviderStatus() -> [(type: ProviderType, available: Bool, name: String)] {
        ProviderType.allCases.map { type in
            (
                type: type,
                available: isProviderAvailable(type),
                name: type.displayName
            )
        }
    }

    /// Get detailed status message
    public func getStatusMessage() -> String {
        let status = getProviderStatus()
        let available = status.filter { $0.available }

        if available.isEmpty {
            return "No AI providers available. Please configure Azure OpenAI or OpenAI."
        } else if available.count == 1 {
            return "1 provider available: \(available[0].name)"
        } else {
            let names = available.map { $0.name }.joined(separator: ", ")
            return "\(available.count) providers available: \(names)"
        }
    }
}

// MARK: - Provider Errors

public enum ProviderError: LocalizedError {
    case notImplemented(provider: String)
    case allProvidersFailed(lastError: String?)

    public var errorDescription: String? {
        switch self {
        case .notImplemented(let provider):
            return "\(provider) is not implemented yet."
        case .allProvidersFailed(let lastError):
            if let lastError = lastError {
                return "All AI providers failed. Last error: \(lastError)"
            } else {
                return "All AI providers failed. No providers available."
            }
        }
    }
}

// MARK: - Convenience Extensions

extension ProviderManager {
    /// Quick chat completion (simple question-answer)
    public func chat(_ message: String) async throws -> String {
        let (response, _) = try await sendPrompt(message)
        return response
    }

    /// Quick chat with system prompt
    public func chat(_ message: String, system: String) async throws -> String {
        let (response, _) = try await sendPrompt(message, systemPrompt: system)
        return response
    }

    /// Streaming chat
    public func streamChat(_ message: String, onChunk: @escaping (String) -> Void) async throws {
        _ = try await streamPrompt(message, onChunk: onChunk)
    }

    /// Streaming chat with system prompt
    public func streamChat(_ message: String, system: String, onChunk: @escaping (String) -> Void) async throws {
        _ = try await streamPrompt(message, systemPrompt: system, onChunk: onChunk)
    }
}
