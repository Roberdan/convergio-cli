/**
 * CONVERGIO NATIVE - Azure OpenAI Provider
 *
 * GDPR-compliant Azure OpenAI API client for EU data handling.
 * Supports chat completions with streaming responses.
 *
 * Endpoint: https://{resource}.openai.azure.com/
 * API Version: 2024-02-15-preview
 * Deployments: gpt-4o-mini (test), gpt-4o (production)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation

// MARK: - Azure OpenAI Configuration

/// Azure OpenAI configuration
public struct AzureOpenAIConfig {
    public let resourceName: String
    public let apiKey: String
    public let deployment: String
    public let apiVersion: String

    /// Default configuration for production
    public static func production(resourceName: String, apiKey: String) -> AzureOpenAIConfig {
        AzureOpenAIConfig(
            resourceName: resourceName,
            apiKey: apiKey,
            deployment: "gpt-4o",
            apiVersion: "2024-02-15-preview"
        )
    }

    /// Default configuration for testing
    public static func test(resourceName: String, apiKey: String) -> AzureOpenAIConfig {
        AzureOpenAIConfig(
            resourceName: resourceName,
            apiKey: apiKey,
            deployment: "gpt-4o-mini",
            apiVersion: "2024-02-15-preview"
        )
    }

    /// Endpoint URL
    public var endpoint: URL? {
        URL(string: "https://\(resourceName).openai.azure.com/openai/deployments/\(deployment)/chat/completions?api-version=\(apiVersion)")
    }
}

// MARK: - Request/Response Models

/// Chat completion request
private struct ChatCompletionRequest: Codable {
    let messages: [[String: String]]
    let temperature: Double
    let maxTokens: Int?
    let stream: Bool

    enum CodingKeys: String, CodingKey {
        case messages
        case temperature
        case maxTokens = "max_tokens"
        case stream
    }
}

/// Chat completion response (non-streaming)
private struct ChatCompletionResponse: Codable {
    let id: String
    let created: Int
    let model: String
    let choices: [Choice]
    let usage: Usage?

    struct Choice: Codable {
        let index: Int
        let message: Message
        let finishReason: String?

        enum CodingKeys: String, CodingKey {
            case index
            case message
            case finishReason = "finish_reason"
        }
    }

    struct Message: Codable {
        let role: String
        let content: String
    }

    struct Usage: Codable {
        let promptTokens: Int
        let completionTokens: Int
        let totalTokens: Int

        enum CodingKeys: String, CodingKey {
            case promptTokens = "prompt_tokens"
            case completionTokens = "completion_tokens"
            case totalTokens = "total_tokens"
        }
    }
}

/// Streaming chunk response
private struct StreamChunk: Codable {
    let id: String
    let created: Int
    let model: String
    let choices: [Choice]

    struct Choice: Codable {
        let index: Int
        let delta: Delta
        let finishReason: String?

        enum CodingKeys: String, CodingKey {
            case index
            case delta
            case finishReason = "finish_reason"
        }
    }

    struct Delta: Codable {
        let role: String?
        let content: String?
    }
}

// MARK: - Azure OpenAI Provider

/// Azure OpenAI API provider
@MainActor
public final class AzureOpenAIProvider: ObservableObject {
    public static let shared = AzureOpenAIProvider()

    @Published public private(set) var isConfigured: Bool = false
    @Published public private(set) var lastError: String?

    private var config: AzureOpenAIConfig?
    private let session: URLSession

    /// Environment variable keys
    private let resourceNameKey = "AZURE_OPENAI_RESOURCE_NAME"
    private let deploymentKey = "AZURE_OPENAI_DEPLOYMENT"

    private init() {
        // Configure URLSession for streaming
        let configuration = URLSessionConfiguration.default
        configuration.timeoutIntervalForRequest = 120
        configuration.timeoutIntervalForResource = 300
        self.session = URLSession(configuration: configuration)

        // Try to auto-configure from environment
        configureFromEnvironment()
    }

    // MARK: - Configuration

    /// Configure from environment variables and Keychain
    public func configureFromEnvironment() {
        guard let apiKey = KeychainManager.shared.getKey(for: .azureOpenAI) else {
            logInfo("No Azure OpenAI API key found", category: "AzureOpenAI")
            isConfigured = false
            return
        }

        // Get resource name from environment
        guard let resourceName = ProcessInfo.processInfo.environment[resourceNameKey],
              !resourceName.isEmpty else {
            logWarning("AZURE_OPENAI_RESOURCE_NAME not set", category: "AzureOpenAI")
            isConfigured = false
            return
        }

        // Get deployment (default to gpt-4o-mini)
        let deployment = ProcessInfo.processInfo.environment[deploymentKey] ?? "gpt-4o-mini"

        configure(resourceName: resourceName, apiKey: apiKey, deployment: deployment)
    }

    /// Configure manually
    public func configure(resourceName: String, apiKey: String, deployment: String = "gpt-4o-mini") {
        config = AzureOpenAIConfig(
            resourceName: resourceName,
            apiKey: apiKey,
            deployment: deployment,
            apiVersion: "2024-02-15-preview"
        )

        isConfigured = true
        logInfo("Configured Azure OpenAI: \(resourceName)/\(deployment)", category: "AzureOpenAI")
    }

    // MARK: - Chat Completion (Non-Streaming)

    /// Send a chat completion request
    public func sendChatCompletion(
        messages: [[String: String]],
        temperature: Double = 0.7,
        maxTokens: Int? = nil
    ) async throws -> (response: String, inputTokens: Int, outputTokens: Int) {
        guard let config = config else {
            throw AzureOpenAIError.notConfigured
        }

        guard let endpoint = config.endpoint else {
            throw AzureOpenAIError.invalidEndpoint
        }

        // Create request
        var request = URLRequest(url: endpoint)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.setValue(config.apiKey, forHTTPHeaderField: "api-key")

        let requestBody = ChatCompletionRequest(
            messages: messages,
            temperature: temperature,
            maxTokens: maxTokens,
            stream: false
        )

        request.httpBody = try JSONEncoder().encode(requestBody)

        logInfo("Sending chat completion to Azure OpenAI (\(config.deployment))", category: "AzureOpenAI")

        // Send request
        let (data, response) = try await session.data(for: request)

        // Check response
        guard let httpResponse = response as? HTTPURLResponse else {
            throw AzureOpenAIError.invalidResponse
        }

        guard httpResponse.statusCode == 200 else {
            let errorMessage = String(data: data, encoding: .utf8) ?? "Unknown error"
            logError("Azure OpenAI error (\(httpResponse.statusCode)): \(errorMessage)", category: "AzureOpenAI")
            throw AzureOpenAIError.apiError(statusCode: httpResponse.statusCode, message: errorMessage)
        }

        // Parse response
        let completionResponse = try JSONDecoder().decode(ChatCompletionResponse.self, from: data)

        guard let choice = completionResponse.choices.first else {
            throw AzureOpenAIError.noResponse
        }

        let content = choice.message.content
        let inputTokens = completionResponse.usage?.promptTokens ?? TokenCounter.estimateTokens(messages: messages)
        let outputTokens = completionResponse.usage?.completionTokens ?? TokenCounter.estimateTokens(content)

        logInfo("Azure OpenAI response: \(outputTokens) tokens", category: "AzureOpenAI")

        // Track usage
        _ = CostTracker.shared.recordUsage(
            provider: "Azure OpenAI",
            model: config.deployment,
            inputTokens: inputTokens,
            outputTokens: outputTokens
        )

        return (content, inputTokens, outputTokens)
    }

    // MARK: - Chat Completion (Streaming)

    /// Send a streaming chat completion request
    public func streamChatCompletion(
        messages: [[String: String]],
        temperature: Double = 0.7,
        maxTokens: Int? = nil,
        onChunk: @escaping (String) -> Void
    ) async throws -> (inputTokens: Int, outputTokens: Int) {
        guard let config = config else {
            throw AzureOpenAIError.notConfigured
        }

        guard let endpoint = config.endpoint else {
            throw AzureOpenAIError.invalidEndpoint
        }

        // Create request
        var request = URLRequest(url: endpoint)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.setValue(config.apiKey, forHTTPHeaderField: "api-key")

        let requestBody = ChatCompletionRequest(
            messages: messages,
            temperature: temperature,
            maxTokens: maxTokens,
            stream: true
        )

        request.httpBody = try JSONEncoder().encode(requestBody)

        logInfo("Streaming chat completion from Azure OpenAI (\(config.deployment))", category: "AzureOpenAI")

        // Send request and get stream
        let (bytes, response) = try await session.bytes(for: request)

        // Check response
        guard let httpResponse = response as? HTTPURLResponse else {
            throw AzureOpenAIError.invalidResponse
        }

        guard httpResponse.statusCode == 200 else {
            throw AzureOpenAIError.apiError(statusCode: httpResponse.statusCode, message: "Streaming request failed")
        }

        // Process stream
        var fullContent = ""

        for try await line in bytes.lines {
            // Skip empty lines
            guard !line.isEmpty else { continue }

            // Skip "data: " prefix
            guard line.hasPrefix("data: ") else { continue }

            let jsonString = String(line.dropFirst(6))

            // Check for [DONE] marker
            guard jsonString != "[DONE]" else { break }

            // Parse chunk
            guard let data = jsonString.data(using: .utf8),
                  let chunk = try? JSONDecoder().decode(StreamChunk.self, from: data),
                  let choice = chunk.choices.first,
                  let content = choice.delta.content else {
                continue
            }

            fullContent += content
            onChunk(content)
        }

        // Estimate tokens
        let inputTokens = TokenCounter.estimateTokens(messages: messages)
        let outputTokens = TokenCounter.estimateTokens(fullContent)

        logInfo("Azure OpenAI stream complete: \(outputTokens) tokens", category: "AzureOpenAI")

        // Track usage
        _ = CostTracker.shared.recordUsage(
            provider: "Azure OpenAI",
            model: config.deployment,
            inputTokens: inputTokens,
            outputTokens: outputTokens
        )

        return (inputTokens, outputTokens)
    }

    // MARK: - Convenience Methods

    /// Send a simple prompt and get a response
    public func sendPrompt(_ prompt: String, systemPrompt: String? = nil) async throws -> String {
        var messages: [[String: String]] = []

        if let systemPrompt = systemPrompt {
            messages.append(["role": "system", "content": systemPrompt])
        }

        messages.append(["role": "user", "content": prompt])

        let (response, _, _) = try await sendChatCompletion(messages: messages)
        return response
    }

    /// Check if the provider is available
    public func checkAvailability() async -> Bool {
        guard isConfigured else { return false }

        do {
            _ = try await sendPrompt("Hello", systemPrompt: "Reply with 'Hi'")
            lastError = nil
            return true
        } catch {
            lastError = error.localizedDescription
            logError("Azure OpenAI availability check failed: \(error)", category: "AzureOpenAI")
            return false
        }
    }
}

// MARK: - Errors

public enum AzureOpenAIError: LocalizedError {
    case notConfigured
    case invalidEndpoint
    case invalidResponse
    case apiError(statusCode: Int, message: String)
    case noResponse

    public var errorDescription: String? {
        switch self {
        case .notConfigured:
            return "Azure OpenAI is not configured. Please set AZURE_OPENAI_API_KEY and AZURE_OPENAI_RESOURCE_NAME."
        case .invalidEndpoint:
            return "Invalid Azure OpenAI endpoint URL."
        case .invalidResponse:
            return "Invalid response from Azure OpenAI."
        case .apiError(let statusCode, let message):
            return "Azure OpenAI API error (\(statusCode)): \(message)"
        case .noResponse:
            return "No response from Azure OpenAI."
        }
    }
}
