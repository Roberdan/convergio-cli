/**
 * CONVERGIO NATIVE - Orchestrator ViewModel
 *
 * Main view model that wraps the Orchestrator for SwiftUI binding.
 * Handles agent management, cost tracking, and model selection.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

@MainActor
public final class OrchestratorViewModel: ObservableObject {
    // MARK: - Published Properties

    @Published public private(set) var isReady = false
    @Published public private(set) var isLoading = false
    @Published public private(set) var agents: [Agent] = []
    @Published public private(set) var activeAgents: [Agent] = []
    @Published public private(set) var costInfo: CostInfo
    @Published public private(set) var currentModel = "claude-sonnet-4"
    @Published public var showAgentPanel = true
    @Published public var showCostDashboard = false

    // Available models (would be loaded from providers)
    public let availableModels = [
        "claude-sonnet-4",
        "claude-opus-4.5",
        "gpt-4o",
        "gpt-4o-mini",
        "gemini-2.0-flash",
        "llama-3.3-70b"
    ]

    // MARK: - Private Properties

    private let orchestrator: Orchestrator
    private var refreshTask: Task<Void, Never>?

    // MARK: - Initialization

    public init(orchestrator: Orchestrator = .shared) {
        self.orchestrator = orchestrator
        self.costInfo = CostInfo(
            sessionCost: 0,
            totalCost: 0,
            budgetLimit: 10.0,
            budgetRemaining: 10.0,
            isOverBudget: false,
            sessionUsage: TokenUsage(),
            totalUsage: TokenUsage()
        )
    }

    // MARK: - Lifecycle

    /// Initialize the orchestrator
    public func initialize(budgetLimit: Double = 10.0) async {
        guard !isReady else { return }

        isLoading = true
        defer { isLoading = false }

        do {
            try await orchestrator.initialize(budgetLimit: budgetLimit)
            isReady = true
            await refresh()
            startAutoRefresh()
        } catch {
            print("Failed to initialize orchestrator: \(error)")
        }
    }

    /// Shutdown the orchestrator
    public func shutdown() {
        refreshTask?.cancel()
        orchestrator.shutdown()
        isReady = false
    }

    // MARK: - Refresh

    /// Refresh all data from orchestrator
    public func refresh() async {
        guard isReady else { return }

        await orchestrator.refresh()
        agents = orchestrator.agents
        activeAgents = orchestrator.activeAgents
        costInfo = orchestrator.costInfo
        currentModel = orchestrator.modelName
    }

    /// Start auto-refresh timer
    private func startAutoRefresh() {
        refreshTask = Task { [weak self] in
            while !Task.isCancelled {
                try? await Task.sleep(nanoseconds: 2_000_000_000) // 2 seconds
                await self?.refresh()
            }
        }
    }

    // MARK: - Agent Management

    /// Get agent by name
    public func agent(named name: String) -> Agent? {
        orchestrator.agent(named: name)
    }

    /// Refresh active agents
    public func refreshActiveAgents() {
        orchestrator.refreshActiveAgents()
        activeAgents = orchestrator.activeAgents
    }

    // MARK: - Cost Management

    /// Set budget limit
    public func setBudget(_ limit: Double) {
        orchestrator.setBudget(limit)
        costInfo = orchestrator.costInfo
    }

    /// Get formatted cost report
    public func getCostReport() -> String {
        orchestrator.getCostReport()
    }

    // MARK: - Model Selection

    /// Select a different model
    public func selectModel(_ model: String) {
        currentModel = model
        // In a real implementation, this would update the orchestrator
    }

    // MARK: - Status

    /// Get orchestrator status
    public func getStatus() -> String {
        orchestrator.getStatus()
    }
}

// MARK: - Preview Helper

extension OrchestratorViewModel {
    /// Preview instance with mock data
    public static var preview: OrchestratorViewModel {
        let vm = OrchestratorViewModel()
        vm.isReady = true
        vm.agents = Agent.previewAgents
        vm.activeAgents = [Agent.previewAgents[0]]
        vm.costInfo = CostInfo(
            sessionCost: 2.34,
            totalCost: 45.67,
            budgetLimit: 10.0,
            budgetRemaining: 7.66,
            isOverBudget: false,
            sessionUsage: TokenUsage(inputTokens: 15000, outputTokens: 8000, cachedTokens: 5000, estimatedCost: 2.34),
            totalUsage: TokenUsage(inputTokens: 500000, outputTokens: 200000, cachedTokens: 100000, estimatedCost: 45.67)
        )
        return vm
    }
}
