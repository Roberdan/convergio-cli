/**
 * CONVERGIO NATIVE - ViewModel Unit Tests
 *
 * Unit tests for ViewModels using Swift Testing framework.
 * Tests business logic without UI dependencies.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import XCTest
@testable import ConvergioCore

@MainActor
final class OrchestratorViewModelTests: XCTestCase {

    // MARK: - Initialization Tests

    func testInitialState() async throws {
        let vm = OrchestratorViewModel()

        XCTAssertFalse(vm.isReady)
        XCTAssertFalse(vm.isLoading)
        XCTAssertTrue(vm.agents.isEmpty)
        XCTAssertTrue(vm.activeAgents.isEmpty)
    }

    func testPreviewHasMockData() async throws {
        let vm = OrchestratorViewModel.preview

        XCTAssertTrue(vm.isReady)
        XCTAssertFalse(vm.agents.isEmpty)
        XCTAssertGreaterThan(vm.costInfo.sessionCost, 0)
    }

    // MARK: - Cost Tests

    func testCostInfoStructure() async throws {
        let costInfo = CostInfo(
            sessionCost: 5.0,
            totalCost: 100.0,
            budgetLimit: 10.0,
            budgetRemaining: 5.0,
            isOverBudget: false,
            sessionUsage: TokenUsage(),
            totalUsage: TokenUsage()
        )

        XCTAssertEqual(costInfo.sessionCost, 5.0)
        XCTAssertEqual(costInfo.budgetLimit, 10.0)
        XCTAssertEqual(costInfo.budgetUsagePercent, 0.5, accuracy: 0.001)
    }

    func testBudgetUsagePercentZeroBudget() async throws {
        let costInfo = CostInfo(
            sessionCost: 5.0,
            totalCost: 100.0,
            budgetLimit: 0,
            budgetRemaining: 0,
            isOverBudget: true,
            sessionUsage: TokenUsage(),
            totalUsage: TokenUsage()
        )

        XCTAssertEqual(costInfo.budgetUsagePercent, 0)
    }

    // MARK: - Model Selection Tests

    func testAvailableModels() async throws {
        let vm = OrchestratorViewModel.preview

        XCTAssertFalse(vm.availableModels.isEmpty)
        XCTAssertTrue(vm.availableModels.contains("claude-sonnet-4"))
    }

    func testSelectModel() async throws {
        let vm = OrchestratorViewModel.preview

        vm.selectModel("gpt-4o")
        XCTAssertEqual(vm.currentModel, "gpt-4o")
    }

    // MARK: - Agent Tests

    func testAgentByName() async throws {
        let vm = OrchestratorViewModel.preview

        let ali = vm.agent(named: "Ali")
        XCTAssertNotNil(ali)
        XCTAssertEqual(ali?.name, "Ali")
    }

    func testAgentByNameCaseInsensitive() async throws {
        let vm = OrchestratorViewModel.preview

        let ali = vm.agent(named: "ali")
        XCTAssertNotNil(ali)
    }

    func testAgentByNameNotFound() async throws {
        let vm = OrchestratorViewModel.preview

        let unknown = vm.agent(named: "NonExistentAgent")
        XCTAssertNil(unknown)
    }

    // MARK: - UI State Tests

    func testShowAgentPanelDefault() async throws {
        let vm = OrchestratorViewModel()
        XCTAssertTrue(vm.showAgentPanel)
    }

    func testShowCostDashboardDefault() async throws {
        let vm = OrchestratorViewModel()
        XCTAssertFalse(vm.showCostDashboard)
    }
}

@MainActor
final class ConversationViewModelTests: XCTestCase {

    // MARK: - Initialization Tests

    func testInitialState() async throws {
        let vm = ConversationViewModel()

        XCTAssertTrue(vm.messages.isEmpty)
        XCTAssertFalse(vm.isProcessing)
        XCTAssertFalse(vm.isStreaming)
        XCTAssertTrue(vm.streamingText.isEmpty)
        XCTAssertNil(vm.error)
    }

    func testPreviewHasMessages() async throws {
        let vm = ConversationViewModel.preview

        XCTAssertFalse(vm.messages.isEmpty)
    }

    // MARK: - Message Management Tests

    func testNewConversation() async throws {
        let vm = ConversationViewModel.preview

        XCTAssertFalse(vm.messages.isEmpty)

        vm.newConversation()

        XCTAssertTrue(vm.messages.isEmpty)
    }

    func testClearHistory() async throws {
        let vm = ConversationViewModel.preview

        vm.clearHistory()

        XCTAssertTrue(vm.messages.isEmpty)
        XCTAssertTrue(vm.streamingText.isEmpty)
        XCTAssertNil(vm.error)
    }

    func testRemoveMessage() async throws {
        let vm = ConversationViewModel.preview

        let messageCount = vm.messages.count
        guard messageCount > 0 else {
            XCTFail("Preview should have messages")
            return
        }

        let messageToRemove = vm.messages[0]
        vm.removeMessage(messageToRemove)

        XCTAssertEqual(vm.messages.count, messageCount - 1)
        XCTAssertFalse(vm.messages.contains(where: { $0.id == messageToRemove.id }))
    }
}

// MARK: - Agent Type Tests

final class AgentTests: XCTestCase {

    @MainActor
    func testAgentRoleDisplayNames() throws {
        XCTAssertEqual(AgentRole.orchestrator.displayName, "Orchestrator")
        XCTAssertEqual(AgentRole.analyst.displayName, "Analyst")
        XCTAssertEqual(AgentRole.coder.displayName, "Coder")
        XCTAssertEqual(AgentRole.writer.displayName, "Writer")
        XCTAssertEqual(AgentRole.critic.displayName, "Critic")
        XCTAssertEqual(AgentRole.planner.displayName, "Planner")
        XCTAssertEqual(AgentRole.executor.displayName, "Executor")
        XCTAssertEqual(AgentRole.memory.displayName, "Memory")
    }

    @MainActor
    func testAgentRoleIcons() throws {
        for role in AgentRole.allCases {
            XCTAssertFalse(role.iconName.isEmpty, "\(role) should have an icon")
        }
    }

    @MainActor
    func testAgentWorkStateIsActive() throws {
        XCTAssertFalse(AgentWorkState.idle.isActive)
        XCTAssertTrue(AgentWorkState.thinking.isActive)
        XCTAssertTrue(AgentWorkState.executing.isActive)
        XCTAssertTrue(AgentWorkState.waiting.isActive)
        XCTAssertTrue(AgentWorkState.communicating.isActive)
    }

    @MainActor
    func testAgentEquatable() throws {
        let agent1 = Agent(id: 1, name: "Test", description: "Test agent", role: .analyst)
        let agent2 = Agent(id: 1, name: "Test", description: "Test agent", role: .analyst)
        let agent3 = Agent(id: 2, name: "Other", description: "Other agent", role: .analyst)

        XCTAssertEqual(agent1, agent2)
        XCTAssertNotEqual(agent1, agent3)
    }

    @MainActor
    func testAgentHashable() throws {
        let agent = Agent(id: 1, name: "Test", description: "Test agent", role: .analyst)

        var set = Set<Agent>()
        set.insert(agent)

        XCTAssertTrue(set.contains(agent))
    }

    @MainActor
    func testPreviewAgents() throws {
        let agents = Agent.previewAgents

        XCTAssertGreaterThan(agents.count, 0)
        XCTAssertEqual(agents[0].name, "Ali")
    }
}

// MARK: - Message Type Tests

final class MessageTests: XCTestCase {

    func testMessageTypeVisibility() throws {
        XCTAssertTrue(MessageType.userInput.isVisible)
        XCTAssertTrue(MessageType.agentResponse.isVisible)
        XCTAssertTrue(MessageType.convergence.isVisible)
        XCTAssertTrue(MessageType.error.isVisible)

        XCTAssertFalse(MessageType.agentThought.isVisible)
        XCTAssertFalse(MessageType.agentAction.isVisible)
        XCTAssertFalse(MessageType.taskDelegate.isVisible)
        XCTAssertFalse(MessageType.taskReport.isVisible)
    }

    func testMessageTypeIsUserMessage() throws {
        XCTAssertTrue(MessageType.userInput.isUserMessage)

        for type in MessageType.allCases where type != .userInput {
            XCTAssertFalse(type.isUserMessage, "\(type) should not be user message")
        }
    }

    func testMessageTypeIsResponse() throws {
        XCTAssertTrue(MessageType.agentResponse.isResponse)
        XCTAssertTrue(MessageType.convergence.isResponse)

        XCTAssertFalse(MessageType.userInput.isResponse)
        XCTAssertFalse(MessageType.agentThought.isResponse)
    }

    func testMessageTypeIcons() throws {
        for type in MessageType.allCases {
            XCTAssertFalse(type.iconName.isEmpty, "\(type) should have an icon")
        }
    }

    func testMessageIsFromUser() throws {
        let userMessage = Message(id: 1, type: .userInput, senderId: 0, content: "Test")
        let agentMessage = Message(id: 2, type: .agentResponse, senderId: 1, content: "Response")

        XCTAssertTrue(userMessage.isFromUser)
        XCTAssertFalse(agentMessage.isFromUser)
    }

    func testMessageFormattedTime() throws {
        let message = Message(id: 1, type: .userInput, senderId: 0, content: "Test")

        let formattedTime = message.formattedTime
        XCTAssertFalse(formattedTime.isEmpty)
    }

    func testPreviewMessages() throws {
        let messages = Message.previewMessages

        XCTAssertGreaterThan(messages.count, 0)
        XCTAssertTrue(messages[0].type == .userInput)
    }
}

// MARK: - TokenUsage Tests

final class TokenUsageTests: XCTestCase {

    func testTotalTokens() throws {
        let usage = TokenUsage(inputTokens: 100, outputTokens: 50, cachedTokens: 20, estimatedCost: 0.01)

        XCTAssertEqual(usage.totalTokens, 150)
    }

    func testDefaultInitialization() throws {
        let usage = TokenUsage()

        XCTAssertEqual(usage.inputTokens, 0)
        XCTAssertEqual(usage.outputTokens, 0)
        XCTAssertEqual(usage.cachedTokens, 0)
        XCTAssertEqual(usage.estimatedCost, 0)
    }
}

// MARK: - ProviderType Tests

final class ProviderTypeTests: XCTestCase {

    func testDisplayNames() throws {
        XCTAssertEqual(ProviderType.anthropic.displayName, "Anthropic")
        XCTAssertEqual(ProviderType.openai.displayName, "OpenAI")
        XCTAssertEqual(ProviderType.gemini.displayName, "Google Gemini")
        XCTAssertEqual(ProviderType.mlx.displayName, "MLX (Local)")
    }

    func testIsLocal() throws {
        XCTAssertTrue(ProviderType.mlx.isLocal)
        XCTAssertTrue(ProviderType.ollama.isLocal)

        XCTAssertFalse(ProviderType.anthropic.isLocal)
        XCTAssertFalse(ProviderType.openai.isLocal)
        XCTAssertFalse(ProviderType.gemini.isLocal)
        XCTAssertFalse(ProviderType.openRouter.isLocal)
    }

    func testAllCases() throws {
        XCTAssertEqual(ProviderType.allCases.count, 6)
    }
}

// MARK: - Error Tests

final class OrchestratorErrorTests: XCTestCase {

    func testErrorDescriptions() throws {
        let errors: [OrchestratorError] = [
            .notInitialized,
            .initializationFailed,
            .cancelled,
            .budgetExceeded,
            .processingFailed("test error")
        ]

        for error in errors {
            XCTAssertNotNil(error.errorDescription, "\(error) should have a description")
            XCTAssertFalse(error.errorDescription!.isEmpty)
        }
    }

    func testProcessingFailedIncludesMessage() throws {
        let error = OrchestratorError.processingFailed("Custom error message")
        XCTAssertTrue(error.errorDescription!.contains("Custom error message"))
    }
}
