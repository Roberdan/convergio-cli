/**
 * CONVERGIO CORE - Smoke Tests
 *
 * Basic tests to verify the C-Swift bridge works correctly.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import XCTest
@testable import ConvergioCore

final class ConvergioCoreTests: XCTestCase {

    // MARK: - Agent Tests

    func testAgentRoleDisplayNames() {
        XCTAssertEqual(AgentRole.orchestrator.displayName, "Orchestrator")
        XCTAssertEqual(AgentRole.analyst.displayName, "Analyst")
        XCTAssertEqual(AgentRole.coder.displayName, "Coder")
        XCTAssertEqual(AgentRole.writer.displayName, "Writer")
    }

    func testAgentRoleIcons() {
        XCTAssertFalse(AgentRole.orchestrator.iconName.isEmpty)
        XCTAssertFalse(AgentRole.analyst.iconName.isEmpty)
    }

    func testAgentWorkStateIsActive() {
        XCTAssertFalse(AgentWorkState.idle.isActive)
        XCTAssertTrue(AgentWorkState.thinking.isActive)
        XCTAssertTrue(AgentWorkState.executing.isActive)
    }

    @MainActor
    func testPreviewAgents() {
        XCTAssertFalse(Agent.previewAgents.isEmpty)
        XCTAssertEqual(Agent.preview.name, "Ali")
    }

    // MARK: - Message Tests

    func testMessageTypeVisibility() {
        XCTAssertTrue(MessageType.userInput.isVisible)
        XCTAssertTrue(MessageType.agentResponse.isVisible)
        XCTAssertTrue(MessageType.convergence.isVisible)
        XCTAssertTrue(MessageType.error.isVisible)
        XCTAssertFalse(MessageType.agentThought.isVisible)
        XCTAssertFalse(MessageType.agentAction.isVisible)
    }

    func testMessageTypeIsUserMessage() {
        XCTAssertTrue(MessageType.userInput.isUserMessage)
        XCTAssertFalse(MessageType.agentResponse.isUserMessage)
    }

    func testMessageTypeIsResponse() {
        XCTAssertTrue(MessageType.agentResponse.isResponse)
        XCTAssertTrue(MessageType.convergence.isResponse)
        XCTAssertFalse(MessageType.userInput.isResponse)
    }

    func testPreviewMessages() {
        XCTAssertFalse(Message.previewMessages.isEmpty)
        XCTAssertTrue(Message.preview.isVisible)
    }

    func testMessageFormattedTime() {
        let message = Message(id: 1, type: .userInput, senderId: 0, content: "Test")
        XCTAssertFalse(message.formattedTime.isEmpty)
    }

    // MARK: - TokenUsage Tests

    func testTokenUsageTotal() {
        let usage = TokenUsage(inputTokens: 100, outputTokens: 50, cachedTokens: 20, estimatedCost: 0.01)
        XCTAssertEqual(usage.totalTokens, 150)
    }

    // MARK: - CostInfo Tests

    func testCostInfoBudgetUsagePercent() {
        let info = CostInfo(
            sessionCost: 5.0,
            totalCost: 10.0,
            budgetLimit: 10.0,
            budgetRemaining: 5.0,
            isOverBudget: false,
            sessionUsage: TokenUsage(),
            totalUsage: TokenUsage()
        )
        XCTAssertEqual(info.budgetUsagePercent, 0.5, accuracy: 0.001)
    }

    func testCostInfoZeroBudget() {
        let info = CostInfo(
            sessionCost: 5.0,
            totalCost: 10.0,
            budgetLimit: 0.0,
            budgetRemaining: 0.0,
            isOverBudget: true,
            sessionUsage: TokenUsage(),
            totalUsage: TokenUsage()
        )
        XCTAssertEqual(info.budgetUsagePercent, 0.0)
    }

    // MARK: - ProviderType Tests

    func testProviderTypeDisplayNames() {
        XCTAssertEqual(ProviderType.anthropic.displayName, "Anthropic")
        XCTAssertEqual(ProviderType.openai.displayName, "OpenAI")
        XCTAssertEqual(ProviderType.mlx.displayName, "MLX (Local)")
    }

    func testProviderTypeIsLocal() {
        XCTAssertTrue(ProviderType.mlx.isLocal)
        XCTAssertTrue(ProviderType.ollama.isLocal)
        XCTAssertFalse(ProviderType.anthropic.isLocal)
        XCTAssertFalse(ProviderType.openai.isLocal)
    }

    // MARK: - Orchestrator Preview Tests

    @MainActor
    func testOrchestratorPreview() {
        let preview = Orchestrator.preview
        XCTAssertTrue(preview.isReady)
        XCTAssertFalse(preview.agents.isEmpty)
        XCTAssertFalse(preview.messages.isEmpty)
        XCTAssertGreaterThan(preview.costInfo.sessionCost, 0)
    }

    // MARK: - Error Tests

    func testOrchestratorErrorDescriptions() {
        XCTAssertNotNil(OrchestratorError.notInitialized.errorDescription)
        XCTAssertNotNil(OrchestratorError.initializationFailed.errorDescription)
        XCTAssertNotNil(OrchestratorError.cancelled.errorDescription)
        XCTAssertNotNil(OrchestratorError.budgetExceeded.errorDescription)
        XCTAssertNotNil(OrchestratorError.processingFailed("test").errorDescription)
    }
}
