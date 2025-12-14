/**
 * CONVERGIO NATIVE - Agent Swift Type
 *
 * Swift-native wrapper for ManagedAgent from the C library.
 * Provides a clean, type-safe interface for SwiftUI.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import CConvergio

// MARK: - Agent Role

/// The specialized role of an agent
public enum AgentRole: Int, CaseIterable, Sendable {
    case orchestrator = 0  // Ali - coordinates everything
    case analyst = 1       // Deep analysis, research
    case coder = 2         // Code generation/review
    case writer = 3        // Content creation
    case critic = 4        // Review and validate
    case planner = 5       // Break down tasks
    case executor = 6      // Execute tools/actions
    case memory = 7        // RAG and context retrieval

    init(from cRole: CAgentRole) {
        self = AgentRole(rawValue: Int(cRole.rawValue)) ?? .orchestrator
    }

    /// Human-readable name for the role
    public var displayName: String {
        switch self {
        case .orchestrator: return "Orchestrator"
        case .analyst: return "Analyst"
        case .coder: return "Coder"
        case .writer: return "Writer"
        case .critic: return "Critic"
        case .planner: return "Planner"
        case .executor: return "Executor"
        case .memory: return "Memory"
        }
    }

    /// SF Symbol icon for the role
    public var iconName: String {
        switch self {
        case .orchestrator: return "brain.head.profile"
        case .analyst: return "chart.bar.xaxis"
        case .coder: return "chevron.left.forwardslash.chevron.right"
        case .writer: return "pencil.and.outline"
        case .critic: return "checkmark.seal"
        case .planner: return "list.bullet.clipboard"
        case .executor: return "gearshape.2"
        case .memory: return "memorychip"
        }
    }
}

// MARK: - Agent Work State

/// The current working state of an agent
public enum AgentWorkState: Int, CaseIterable, Sendable {
    case idle = 0           // Not currently working
    case thinking = 1       // Processing a request
    case executing = 2      // Executing tools
    case waiting = 3        // Waiting for another agent
    case communicating = 4  // Talking to another agent

    init(from cState: CAgentWorkState) {
        self = AgentWorkState(rawValue: Int(cState.rawValue)) ?? .idle
    }

    /// Human-readable description
    public var displayName: String {
        switch self {
        case .idle: return "Idle"
        case .thinking: return "Thinking..."
        case .executing: return "Executing"
        case .waiting: return "Waiting"
        case .communicating: return "Communicating"
        }
    }

    /// Whether the agent is actively working
    public var isActive: Bool {
        self != .idle
    }
}

// MARK: - Agent

/// A managed AI agent with specialized capabilities
@MainActor
public final class Agent: Identifiable, ObservableObject, Sendable {
    /// Unique identifier
    public let id: UInt64

    /// Agent's name
    public let name: String

    /// Description of the agent's capabilities
    public let description: String

    /// Specialized role
    public let role: AgentRole

    /// Current work state
    @Published public private(set) var workState: AgentWorkState = .idle

    /// Current task (if working)
    @Published public private(set) var currentTask: String?

    /// Whether the agent is currently active
    @Published public private(set) var isActive: Bool = false

    /// Reference to the underlying C agent (for internal use)
    internal let cAgent: OpaquePointer?

    /// Initialize from C agent pointer
    internal init(cAgent: OpaquePointer) {
        self.cAgent = cAgent

        // Extract data from C
        self.id = convergio_agent_get_id(cAgent)
        self.name = String(cString: convergio_agent_get_name(cAgent) ?? "Unknown")
        self.description = String(cString: convergio_agent_get_description(cAgent) ?? "")
        self.role = AgentRole(from: convergio_agent_get_role(cAgent))

        // Initial state
        refresh()
    }

    /// Create a preview/mock agent
    public init(id: UInt64, name: String, description: String, role: AgentRole) {
        self.cAgent = nil
        self.id = id
        self.name = name
        self.description = description
        self.role = role
    }

    /// Refresh state from C library
    public func refresh() {
        guard let cAgent else { return }

        let cState = convergio_agent_get_work_state(cAgent)
        workState = AgentWorkState(from: cState)

        if let taskPtr = convergio_agent_get_current_task(cAgent) {
            currentTask = String(cString: taskPtr)
        } else {
            currentTask = nil
        }

        isActive = convergio_agent_is_active(cAgent)
    }
}

// MARK: - Agent Hashable & Equatable

extension Agent: Hashable {
    public static func == (lhs: Agent, rhs: Agent) -> Bool {
        lhs.id == rhs.id
    }

    public nonisolated func hash(into hasher: inout Hasher) {
        hasher.combine(id)
    }
}

// MARK: - Preview Helpers

extension Agent {
    /// Create mock agents for SwiftUI previews
    public static let previewAgents: [Agent] = [
        Agent(id: 1, name: "Ali", description: "Chief of Staff - Coordinates all agents", role: .orchestrator),
        Agent(id: 2, name: "Angela", description: "Data Analyst - Deep research and analysis", role: .analyst),
        Agent(id: 3, name: "Amy", description: "CFO - Financial analysis and budgeting", role: .analyst),
        Agent(id: 4, name: "Matteo", description: "Strategy Director - Business architecture", role: .planner),
        Agent(id: 5, name: "Riccardo", description: "Tech Writer - Documentation and content", role: .writer),
        Agent(id: 6, name: "Jony", description: "Creative Director - Design and UX", role: .writer),
    ]

    /// Preview agent for SwiftUI
    public static var preview: Agent {
        previewAgents[0]
    }
}
