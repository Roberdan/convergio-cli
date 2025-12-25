/**
 * CONVERGIO NATIVE - Orchestrator Swift Type
 *
 * Main coordinator for the AI executive team.
 * Wraps the C orchestrator with a Swift-native async/await interface.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import CConvergio

// MARK: - Token Usage

/// Token usage statistics
public struct TokenUsage: Sendable {
    public let inputTokens: Int
    public let outputTokens: Int
    public let cachedTokens: Int
    public let estimatedCost: Double

    internal init(from cUsage: CTokenUsage) {
        self.inputTokens = Int(cUsage.input_tokens)
        self.outputTokens = Int(cUsage.output_tokens)
        self.cachedTokens = Int(cUsage.cached_tokens)
        self.estimatedCost = cUsage.estimated_cost
    }

    public init(inputTokens: Int = 0, outputTokens: Int = 0, cachedTokens: Int = 0, estimatedCost: Double = 0) {
        self.inputTokens = inputTokens
        self.outputTokens = outputTokens
        self.cachedTokens = cachedTokens
        self.estimatedCost = estimatedCost
    }

    /// Total tokens used
    public var totalTokens: Int {
        inputTokens + outputTokens
    }
}

// MARK: - Cost Info

/// Current cost tracking information
public struct CostInfo: Sendable {
    public let sessionCost: Double
    public let totalCost: Double
    public let budgetLimit: Double
    public let budgetRemaining: Double
    public let isOverBudget: Bool
    public let sessionUsage: TokenUsage
    public let totalUsage: TokenUsage

    /// Public initializer
    public init(
        sessionCost: Double,
        totalCost: Double,
        budgetLimit: Double,
        budgetRemaining: Double,
        isOverBudget: Bool,
        sessionUsage: TokenUsage,
        totalUsage: TokenUsage
    ) {
        self.sessionCost = sessionCost
        self.totalCost = totalCost
        self.budgetLimit = budgetLimit
        self.budgetRemaining = budgetRemaining
        self.isOverBudget = isOverBudget
        self.sessionUsage = sessionUsage
        self.totalUsage = totalUsage
    }

    /// Budget usage as a percentage (0-1)
    public var budgetUsagePercent: Double {
        guard budgetLimit > 0 else { return 0 }
        return sessionCost / budgetLimit
    }
}

// MARK: - Orchestrator Error

/// Errors that can occur during orchestrator operations
public enum OrchestratorError: Error, LocalizedError {
    case notInitialized
    case initializationFailed
    case processingFailed(String)
    case cancelled
    case budgetExceeded

    public var errorDescription: String? {
        switch self {
        case .notInitialized:
            return "Orchestrator is not initialized"
        case .initializationFailed:
            return "Failed to initialize orchestrator"
        case .processingFailed(let message):
            return "Processing failed: \(message)"
        case .cancelled:
            return "Request was cancelled"
        case .budgetExceeded:
            return "Budget limit exceeded"
        }
    }
}

// MARK: - Orchestrator

/// The main AI orchestrator that coordinates all agents
@MainActor
public final class Orchestrator: ObservableObject {
    /// Shared instance
    public static let shared = Orchestrator()

    /// Whether the orchestrator is initialized and ready
    @Published public private(set) var isReady: Bool = false

    /// Currently available agents
    @Published public private(set) var agents: [Agent] = []

    /// Agents currently working
    @Published public private(set) var activeAgents: [Agent] = []

    /// Message history
    @Published public private(set) var messages: [Message] = []

    /// Whether a request is currently being processed
    @Published public private(set) var isProcessing: Bool = false

    /// Current streaming text (during processing)
    @Published public private(set) var streamingText: String = ""

    /// Current cost information
    @Published public private(set) var costInfo: CostInfo = CostInfo(
        sessionCost: 0,
        totalCost: 0,
        budgetLimit: 10.0,
        budgetRemaining: 10.0,
        isOverBudget: false,
        sessionUsage: TokenUsage(),
        totalUsage: TokenUsage()
    )

    /// Current session ID
    @Published public private(set) var sessionId: String?

    /// Current model name
    @Published public private(set) var modelName: String = "claude-sonnet-4"

    // MARK: - Initialization

    private init() {}

    /// Initialize the orchestrator with a budget limit
    /// - Parameter budgetLimit: Maximum spend allowed in USD
    public func initialize(budgetLimit: Double = 10.0) async throws {
        guard !isReady else { return }

        let result = convergio_init(budgetLimit)
        guard result == 0 else {
            throw OrchestratorError.initializationFailed
        }

        isReady = true
        await refresh()
    }

    /// Shutdown the orchestrator and free resources
    public func shutdown() {
        guard isReady else { return }
        convergio_shutdown()
        isReady = false
        agents = []
        activeAgents = []
        messages = []
    }

    deinit {
        // Note: This won't run on MainActor, so we need to be careful
        // The user should call shutdown() explicitly
    }

    // MARK: - Message Processing

    /// Send a message and get a response
    /// - Parameter input: The user's message
    /// - Returns: The response from the AI team
    public func send(_ input: String) async throws -> String {
        guard isReady else {
            throw OrchestratorError.notInitialized
        }

        guard !costInfo.isOverBudget else {
            throw OrchestratorError.budgetExceeded
        }

        isProcessing = true
        streamingText = ""
        defer {
            isProcessing = false
            streamingText = ""
        }

        // Reset any previous cancellation
        convergio_reset_cancel()

        // Process the message
        guard let responsePtr = convergio_process(input) else {
            if convergio_is_cancelled() {
                throw OrchestratorError.cancelled
            }
            throw OrchestratorError.processingFailed("No response received")
        }

        let response = String(cString: responsePtr)
        convergio_free_string(responsePtr)

        // Refresh state after processing
        await refresh()

        return response
    }

    /// Send a message with streaming response
    /// - Parameters:
    ///   - input: The user's message
    ///   - onChunk: Callback for each chunk of the response
    /// - Returns: The complete response
    public func sendStreaming(
        _ input: String,
        onChunk: @escaping @Sendable (String) -> Void
    ) async throws -> String {
        guard isReady else {
            throw OrchestratorError.notInitialized
        }

        guard !costInfo.isOverBudget else {
            throw OrchestratorError.budgetExceeded
        }

        isProcessing = true
        streamingText = ""
        defer {
            isProcessing = false
        }

        // Reset any previous cancellation
        convergio_reset_cancel()

        // Create streaming callback context
        let streamingActor = StreamingActor(onChunk: onChunk)
        let context = Unmanaged.passRetained(streamingActor).toOpaque()

        let callback: ConvergioStreamCallback = { chunk, ctx in
            guard let chunk, let ctx else { return }
            let actor = Unmanaged<StreamingActor>.fromOpaque(ctx).takeUnretainedValue()
            let chunkStr = String(cString: chunk)
            actor.handleChunk(chunkStr)
        }

        // Process with streaming
        let responsePtr = convergio_process_stream(input, callback, context)

        // Release the context
        Unmanaged<StreamingActor>.fromOpaque(context).release()

        guard let responsePtr else {
            if convergio_is_cancelled() {
                throw OrchestratorError.cancelled
            }
            throw OrchestratorError.processingFailed("No response received")
        }

        let response = String(cString: responsePtr)
        convergio_free_string(responsePtr)

        // Refresh state after processing
        await refresh()

        return response
    }

    /// Cancel the current request
    public func cancel() {
        convergio_cancel_request()
    }

    // MARK: - Agent Management

    /// Get all available agents
    public func loadAgents() {
        let count = convergio_get_agent_count()
        var newAgents: [Agent] = []

        for i in 0..<count {
            if let cAgent = convergio_get_agent_at(i) {
                let agent = Agent(cAgent: cAgent)
                newAgents.append(agent)
            }
        }

        agents = newAgents
    }

    /// Get an agent by name
    public func agent(named name: String) -> Agent? {
        agents.first { $0.name.lowercased() == name.lowercased() }
    }

    /// Refresh active agents
    public func refreshActiveAgents() {
        var workingAgents = [OpaquePointer?](repeating: nil, count: 64)
        let count = convergio_get_working_agents(&workingAgents, 64)

        activeAgents = agents.filter { agent in
            agent.workState.isActive
        }

        // Refresh all agent states
        for agent in agents {
            agent.refresh()
        }
    }

    // MARK: - Cost Management

    /// Refresh cost information
    public func refreshCost() {
        var sessionUsage = CTokenUsage()
        var totalUsage = CTokenUsage()

        convergio_get_session_usage(&sessionUsage)
        convergio_get_total_usage(&totalUsage)

        costInfo = CostInfo(
            sessionCost: convergio_get_session_cost(),
            totalCost: convergio_get_total_cost(),
            budgetLimit: convergio_get_budget_limit(),
            budgetRemaining: convergio_get_budget_remaining(),
            isOverBudget: convergio_is_budget_exceeded(),
            sessionUsage: TokenUsage(from: sessionUsage),
            totalUsage: TokenUsage(from: totalUsage)
        )
    }

    /// Set a new budget limit
    public func setBudget(_ limit: Double) {
        convergio_set_budget(limit)
        refreshCost()
    }

    /// Get formatted cost report
    public func getCostReport() -> String {
        guard let reportPtr = convergio_get_cost_report() else {
            return "Cost report unavailable"
        }
        let report = String(cString: reportPtr)
        convergio_free_string(reportPtr)
        return report
    }

    // MARK: - Session Management

    /// Get orchestrator status
    public func getStatus() -> String {
        guard let statusPtr = convergio_get_status() else {
            return "Status unavailable"
        }
        let status = String(cString: statusPtr)
        convergio_free_string(statusPtr)
        return status
    }

    // MARK: - Convergence

    /// Request parallel analysis from multiple agents
    /// - Parameters:
    ///   - input: The question or task
    ///   - agentNames: Names of agents to involve
    /// - Returns: Converged response from all agents
    public func parallelAnalyze(
        _ input: String,
        agents agentNames: [String]
    ) async throws -> String {
        guard isReady else {
            throw OrchestratorError.notInitialized
        }

        isProcessing = true
        defer { isProcessing = false }

        // Convert to C string array
        var cStrings = agentNames.map { strdup($0) }
        defer { cStrings.forEach { free($0) } }

        var cStringPtrs = cStrings.map { UnsafePointer($0) }

        guard let responsePtr = convergio_parallel_analyze(
            input,
            &cStringPtrs,
            agentNames.count
        ) else {
            throw OrchestratorError.processingFailed("Parallel analysis failed")
        }

        let response = String(cString: responsePtr)
        convergio_free_string(responsePtr)

        await refresh()

        return response
    }

    // MARK: - Refresh

    /// Refresh all state from C library
    public func refresh() async {
        loadAgents()
        refreshActiveAgents()
        refreshCost()

        if let sessionIdPtr = convergio_get_session_id() {
            sessionId = String(cString: sessionIdPtr)
        }

        if let modelPtr = convergio_get_current_model() {
            modelName = String(cString: modelPtr)
        }
    }
}

// MARK: - Streaming Actor

/// Actor to handle streaming callbacks safely
private final class StreamingActor: @unchecked Sendable {
    private let onChunk: (String) -> Void
    private let lock = NSLock()

    init(onChunk: @escaping (String) -> Void) {
        self.onChunk = onChunk
    }

    func handleChunk(_ chunk: String) {
        // Call callback immediately without holding the lock during execution
        // The lock only protects against concurrent handleChunk calls
        lock.lock()
        let callback = onChunk
        lock.unlock()

        // Execute callback - it will handle its own thread safety
        callback(chunk)
    }
}

// MARK: - Preview Helpers

extension Orchestrator {
    /// Create a preview orchestrator with mock data
    public static var preview: Orchestrator {
        let orch = Orchestrator()
        orch.isReady = true
        orch.agents = Agent.previewAgents
        orch.messages = Message.previewMessages
        orch.costInfo = CostInfo(
            sessionCost: 2.34,
            totalCost: 45.67,
            budgetLimit: 10.0,
            budgetRemaining: 7.66,
            isOverBudget: false,
            sessionUsage: TokenUsage(inputTokens: 15000, outputTokens: 8000, cachedTokens: 5000, estimatedCost: 2.34),
            totalUsage: TokenUsage(inputTokens: 500000, outputTokens: 200000, cachedTokens: 100000, estimatedCost: 45.67)
        )
        orch.modelName = "claude-sonnet-4"
        return orch
    }
}
