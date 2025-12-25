/**
 * CONVERGIO NATIVE - Azure OpenAI Provider
 *
 * GDPR-compliant AI provider using Azure OpenAI for EU student data.
 * Includes fallback chain to OpenAI and local models.
 *
 * Part of the Scuola 2026 Foundation (Tasks 0.4.1 - 0.4.4)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import Combine

// MARK: - Provider Types

enum AIProviderType: String, CaseIterable, Codable {
    case azureOpenAI = "azure"
    case openAI = "openai"
    case local = "local"

    var displayName: String {
        switch self {
        case .azureOpenAI: return "Azure OpenAI (EU)"
        case .openAI: return "OpenAI"
        case .local: return "Local (MLX)"
        }
    }

    var isGDPRCompliant: Bool {
        switch self {
        case .azureOpenAI, .local: return true
        case .openAI: return false
        }
    }
}

// MARK: - Configuration

struct AzureOpenAIConfig: Codable {
    let endpoint: String
    let apiKey: String
    let deploymentName: String
    let apiVersion: String

    static let defaultAPIVersion = "2024-02-15-preview"

    var isValid: Bool {
        !endpoint.isEmpty && !apiKey.isEmpty && !deploymentName.isEmpty
    }

    static var fromEnvironment: AzureOpenAIConfig? {
        guard let endpoint = ProcessInfo.processInfo.environment["AZURE_OPENAI_ENDPOINT"],
              let apiKey = ProcessInfo.processInfo.environment["AZURE_OPENAI_API_KEY"],
              let deployment = ProcessInfo.processInfo.environment["AZURE_OPENAI_DEPLOYMENT"] else {
            return nil
        }

        return AzureOpenAIConfig(
            endpoint: endpoint,
            apiKey: apiKey,
            deploymentName: deployment,
            apiVersion: ProcessInfo.processInfo.environment["AZURE_OPENAI_API_VERSION"] ?? defaultAPIVersion
        )
    }
}

// MARK: - Request/Response Models

struct ChatMessage: Codable {
    let role: String
    let content: String
}

struct ChatCompletionRequest: Codable {
    let messages: [ChatMessage]
    let temperature: Double
    let maxTokens: Int
    let stream: Bool

    enum CodingKeys: String, CodingKey {
        case messages
        case temperature
        case maxTokens = "max_tokens"
        case stream
    }
}

struct ChatCompletionResponse: Codable {
    struct Choice: Codable {
        struct Message: Codable {
            let role: String
            let content: String
        }
        let message: Message
        let finishReason: String?

        enum CodingKeys: String, CodingKey {
            case message
            case finishReason = "finish_reason"
        }
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

    let id: String
    let choices: [Choice]
    let usage: Usage?
}

struct StreamChunk: Codable {
    struct Choice: Codable {
        struct Delta: Codable {
            let content: String?
        }
        let delta: Delta
        let finishReason: String?

        enum CodingKeys: String, CodingKey {
            case delta
            case finishReason = "finish_reason"
        }
    }

    let choices: [Choice]
}

// MARK: - Cost Tracking

struct SessionCost {
    var inputTokens: Int = 0
    var outputTokens: Int = 0
    var cachedTokens: Int = 0
    var requestCount: Int = 0

    // Pricing per 1M tokens (Azure GPT-4o)
    static let inputPrice: Double = 5.0   // $5 per 1M input tokens
    static let outputPrice: Double = 15.0 // $15 per 1M output tokens
    static let cachedPrice: Double = 2.5  // $2.5 per 1M cached tokens

    var estimatedCost: Double {
        let inputCost = Double(inputTokens) / 1_000_000 * Self.inputPrice
        let outputCost = Double(outputTokens) / 1_000_000 * Self.outputPrice
        let cachedCost = Double(cachedTokens) / 1_000_000 * Self.cachedPrice
        return inputCost + outputCost + cachedCost
    }

    var totalTokens: Int {
        inputTokens + outputTokens
    }

    mutating func addUsage(_ usage: ChatCompletionResponse.Usage) {
        inputTokens += usage.promptTokens
        outputTokens += usage.completionTokens
        requestCount += 1
    }

    mutating func reset() {
        inputTokens = 0
        outputTokens = 0
        cachedTokens = 0
        requestCount = 0
    }
}

// MARK: - Azure OpenAI Provider

@MainActor
final class AzureOpenAIProvider: ObservableObject {
    // MARK: - Published State

    @Published private(set) var currentProvider: AIProviderType = .azureOpenAI
    @Published private(set) var isConfigured: Bool = false
    @Published private(set) var isProcessing: Bool = false
    @Published private(set) var sessionCost: SessionCost = SessionCost()
    @Published private(set) var lastError: String?

    // MARK: - Configuration

    private var azureConfig: AzureOpenAIConfig?
    private var openAIKey: String?
    private var session: URLSession

    // MARK: - Fallback Chain

    /// Fallback order depends on edition:
    /// - EDU: Only Azure OpenAI and Local (GDPR compliant)
    /// - Other: Full fallback chain
    private var fallbackOrder: [AIProviderType] {
        if EditionManager.shared.currentEdition == .education {
            // EDU edition: ONLY GDPR-compliant providers
            return [.azureOpenAI, .local]
        }
        return [.azureOpenAI, .openAI, .local]
    }
    private var failedProviders: Set<AIProviderType> = []

    // MARK: - Budget

    private let budgetLimit: Double = 10.0 // $10 per session

    var budgetRemaining: Double {
        max(0, budgetLimit - sessionCost.estimatedCost)
    }

    var isOverBudget: Bool {
        sessionCost.estimatedCost >= budgetLimit
    }

    // MARK: - Singleton

    static let shared = AzureOpenAIProvider()

    // MARK: - Initialization

    private init() {
        let config = URLSessionConfiguration.default
        config.timeoutIntervalForRequest = 60
        config.timeoutIntervalForResource = 300
        self.session = URLSession(configuration: config)

        Task {
            await configure()
        }
    }

    // MARK: - Configuration

    /// Check if a provider is allowed for the current edition
    var isProviderAllowedForEdition: Bool {
        let edition = EditionManager.shared.currentEdition
        if edition == .education {
            // EDU only allows GDPR-compliant providers
            return currentProvider.isGDPRCompliant
        }
        return true
    }

    func configure() async {
        let isEDU = EditionManager.shared.currentEdition == .education

        // Try Azure first (required for EDU)
        if let config = AzureOpenAIConfig.fromEnvironment, config.isValid {
            azureConfig = config
            currentProvider = .azureOpenAI
            isConfigured = true
            logInfo("Configured with Azure OpenAI (GDPR compliant)", category: "AI")
            return
        }

        // For EDU edition, if Azure is not configured, only allow local
        if isEDU {
            currentProvider = .local
            isConfigured = true
            logWarning("EDU edition: Azure OpenAI not configured, using local fallback only", category: "AI")
            return
        }

        // Non-EDU: Try OpenAI
        if let key = ProcessInfo.processInfo.environment["OPENAI_API_KEY"], !key.isEmpty {
            openAIKey = key
            currentProvider = .openAI
            isConfigured = true
            logInfo("Configured with OpenAI", category: "AI")
            return
        }

        // Fallback to local
        currentProvider = .local
        isConfigured = true
        logInfo("Using local AI fallback", category: "AI")
    }

    func validateAPIKey() async -> Bool {
        guard let config = azureConfig, config.isValid else {
            return false
        }

        // Simple validation request
        do {
            let _ = try await sendRequest(
                messages: [ChatMessage(role: "user", content: "test")],
                temperature: 0.1,
                maxTokens: 5,
                stream: false
            )
            return true
        } catch {
            lastError = "API key validation failed: \(error.localizedDescription)"
            return false
        }
    }

    // MARK: - Chat Completion

    func complete(
        messages: [ChatMessage],
        temperature: Double = 0.7,
        maxTokens: Int = 2048
    ) async throws -> String {
        guard !isOverBudget else {
            throw ProviderError.budgetExceeded
        }

        isProcessing = true
        defer { isProcessing = false }

        // Try current provider, then fallbacks
        var lastError: Error?

        for provider in fallbackOrder {
            guard !failedProviders.contains(provider) else { continue }

            do {
                let response = try await sendRequest(
                    to: provider,
                    messages: messages,
                    temperature: temperature,
                    maxTokens: maxTokens,
                    stream: false
                )

                // Update cost tracking
                if let usage = response.usage {
                    sessionCost.addUsage(usage)
                }

                currentProvider = provider
                return response.choices.first?.message.content ?? ""

            } catch {
                lastError = error
                failedProviders.insert(provider)
                continue
            }
        }

        throw lastError ?? ProviderError.allProvidersFailed
    }

    func stream(
        messages: [ChatMessage],
        temperature: Double = 0.7,
        maxTokens: Int = 2048
    ) -> AsyncThrowingStream<String, Error> {
        AsyncThrowingStream { continuation in
            Task {
                guard !isOverBudget else {
                    continuation.finish(throwing: ProviderError.budgetExceeded)
                    return
                }

                await MainActor.run { isProcessing = true }
                defer { Task { @MainActor in isProcessing = false } }

                do {
                    let stream = try await createStream(
                        messages: messages,
                        temperature: temperature,
                        maxTokens: maxTokens
                    )

                    for try await chunk in stream {
                        if let content = chunk.choices.first?.delta.content {
                            continuation.yield(content)
                        }
                    }

                    continuation.finish()
                } catch {
                    continuation.finish(throwing: error)
                }
            }
        }
    }

    // MARK: - Request Handling

    private func sendRequest(
        to provider: AIProviderType = .azureOpenAI,
        messages: [ChatMessage],
        temperature: Double,
        maxTokens: Int,
        stream: Bool
    ) async throws -> ChatCompletionResponse {
        let request = try buildRequest(
            for: provider,
            body: ChatCompletionRequest(
                messages: messages,
                temperature: temperature,
                maxTokens: maxTokens,
                stream: stream
            )
        )

        let (data, response) = try await session.data(for: request)

        guard let httpResponse = response as? HTTPURLResponse else {
            throw ProviderError.invalidResponse
        }

        guard (200...299).contains(httpResponse.statusCode) else {
            let errorMessage = String(data: data, encoding: .utf8) ?? "Unknown error"
            throw ProviderError.httpError(httpResponse.statusCode, errorMessage)
        }

        return try JSONDecoder().decode(ChatCompletionResponse.self, from: data)
    }

    private func createStream(
        messages: [ChatMessage],
        temperature: Double,
        maxTokens: Int
    ) async throws -> AsyncThrowingStream<StreamChunk, Error> {
        let request = try buildRequest(
            for: currentProvider,
            body: ChatCompletionRequest(
                messages: messages,
                temperature: temperature,
                maxTokens: maxTokens,
                stream: true
            )
        )

        let (bytes, response) = try await session.bytes(for: request)

        guard let httpResponse = response as? HTTPURLResponse,
              (200...299).contains(httpResponse.statusCode) else {
            throw ProviderError.invalidResponse
        }

        return AsyncThrowingStream { continuation in
            Task {
                do {
                    for try await line in bytes.lines {
                        guard line.hasPrefix("data: "),
                              line != "data: [DONE]" else { continue }

                        let jsonString = String(line.dropFirst(6))
                        if let data = jsonString.data(using: .utf8),
                           let chunk = try? JSONDecoder().decode(StreamChunk.self, from: data) {
                            continuation.yield(chunk)
                        }
                    }
                    continuation.finish()
                } catch {
                    continuation.finish(throwing: error)
                }
            }
        }
    }

    private func buildRequest(for provider: AIProviderType, body: ChatCompletionRequest) throws -> URLRequest {
        let url: URL
        var request: URLRequest

        switch provider {
        case .azureOpenAI:
            guard let config = azureConfig else {
                throw ProviderError.notConfigured
            }
            url = URL(string: "\(config.endpoint)/openai/deployments/\(config.deploymentName)/chat/completions?api-version=\(config.apiVersion)")!
            request = URLRequest(url: url)
            request.setValue(config.apiKey, forHTTPHeaderField: "api-key")

        case .openAI:
            guard let apiKey = openAIKey else {
                throw ProviderError.notConfigured
            }
            url = URL(string: "https://api.openai.com/v1/chat/completions")!
            request = URLRequest(url: url)
            request.setValue("Bearer \(apiKey)", forHTTPHeaderField: "Authorization")

        case .local:
            throw ProviderError.localNotImplemented
        }

        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpBody = try JSONEncoder().encode(body)

        return request
    }

    // MARK: - Cost Management

    func resetSessionCost() {
        sessionCost.reset()
    }

    func resetFailedProviders() {
        failedProviders.removeAll()
    }
}

// MARK: - Errors

enum ProviderError: LocalizedError {
    case notConfigured
    case invalidResponse
    case httpError(Int, String)
    case allProvidersFailed
    case budgetExceeded
    case localNotImplemented

    var errorDescription: String? {
        switch self {
        case .notConfigured:
            return "AI provider not configured. Check your API keys."
        case .invalidResponse:
            return "Invalid response from AI provider."
        case .httpError(let code, let message):
            return "HTTP error \(code): \(message)"
        case .allProvidersFailed:
            return "All AI providers failed. Check your network connection."
        case .budgetExceeded:
            return "Session budget exceeded. Please start a new session."
        case .localNotImplemented:
            return "Local AI provider not yet implemented."
        }
    }
}

// MARK: - Education-Specific Extensions

extension AzureOpenAIProvider {
    /// Send a message with maestro context
    func askMaestro(
        _ maestro: Maestro,
        question: String,
        conversationHistory: [ChatMessage] = [],
        studentProfile: StudentProfile? = nil
    ) async throws -> String {
        var messages: [ChatMessage] = []

        // System prompt for maestro
        let systemPrompt = buildMaestroSystemPrompt(maestro: maestro, student: studentProfile)
        messages.append(ChatMessage(role: "system", content: systemPrompt))

        // Conversation history
        messages.append(contentsOf: conversationHistory)

        // Current question
        messages.append(ChatMessage(role: "user", content: question))

        return try await complete(messages: messages)
    }

    private func buildMaestroSystemPrompt(maestro: Maestro, student: StudentProfile?) -> String {
        var prompt = """
        Sei \(maestro.name), maestro di \(maestro.subject).
        \(maestro.description)

        REGOLE FONDAMENTALI:
        1. Usa il metodo maieutico: guida lo studente verso la risposta con domande.
        2. NON dare mai risposte dirette ai compiti. Aiuta a ragionare.
        3. Adatta il linguaggio all'eta dello studente.
        4. Sii incoraggiante ma onesto.
        5. Se lo studente mostra frustrazione, semplifica e incoraggia.
        """

        if let student = student {
            prompt += """

        PROFILO STUDENTE:
        - Nome: \(student.firstName)
        - Classe: \(student.schoolYear.displayName)
        - Stile: Adatta alle preferenze di accessibilita
        """
        }

        return prompt
    }
}

// MARK: - Previews

#if DEBUG
extension AzureOpenAIProvider {
    static var preview: AzureOpenAIProvider {
        let provider = AzureOpenAIProvider.shared
        provider.sessionCost = SessionCost(
            inputTokens: 5000,
            outputTokens: 2000,
            cachedTokens: 1000,
            requestCount: 10
        )
        return provider
    }
}
#endif
