/**
 * CONVERGIO NATIVE - Edition Manager
 *
 * Manages different product editions (education, business, developer, master)
 * with filtered agent access and edition-specific features.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import SwiftUI

// MARK: - Convergio Edition

/// Product editions with different agent access levels
public enum ConvergioEdition: String, CaseIterable, Codable, Identifiable {
    case education  // 18 Maestri + 3 coordinatori = 21 agents
    case business   // 10 agents sales/marketing/finance
    case developer  // All technical agents
    case master     // All 54+ agents

    public var id: String { rawValue }

    /// Display name for the edition
    public var displayName: String {
        switch self {
        case .education: return "Education"
        case .business: return "Business"
        case .developer: return "Developer"
        case .master: return "Master"
        }
    }

    /// Tagline for the edition
    public var tagline: String {
        switch self {
        case .education: return "Learn with the greatest teachers in history"
        case .business: return "Scale your business with AI executives"
        case .developer: return "Build faster with AI development team"
        case .master: return "Access all agents and features"
        }
    }

    /// Icon for the edition
    public var icon: String {
        switch self {
        case .education: return "graduationcap.fill"
        case .business: return "briefcase.fill"
        case .developer: return "hammer.fill"
        case .master: return "crown.fill"
        }
    }

    /// Color theme for the edition
    public var color: Color {
        switch self {
        case .education: return .blue
        case .business: return .green
        case .developer: return .purple
        case .master: return .orange
        }
    }

    /// Get allowed agents for this edition
    public var allowedAgents: Set<String> {
        switch self {
        case .education:
            return Set(Self.educationAgents)
        case .business:
            return Set(Self.businessAgents)
        case .developer:
            return Set(Self.developerAgents)
        case .master:
            return [] // Empty set means all agents are allowed
        }
    }

    /// Check if an agent is allowed in this edition
    public func isAgentAllowed(_ agentId: String) -> Bool {
        switch self {
        case .master:
            return true // Master edition allows all agents
        default:
            return allowedAgents.contains(agentId)
        }
    }

    // MARK: - Education Edition Agents (20 total)

    /// The 17 Maestri Storici (Historical Teachers)
    public static let maestri: [String] = [
        // Mathematics (2)
        "euclide-matematica",      // Euclid - Geometry, deductive logic
        "pitagora-matematica",     // Pythagoras - Numbers, harmony, algebra

        // Physics (2)
        "feynman-fisica",          // Feynman - Modern physics, science communication
        "galileo-fisica",          // Galileo - Scientific method, astronomy

        // Chemistry & Biology (2)
        "curie-chimica",           // Curie - Radioactivity, perseverance
        "darwin-scienze",          // Darwin - Evolution, observation

        // History & Geography (2)
        "erodoto-storia",          // Herodotus - Historical narrative
        "humboldt-geografia",      // Humboldt - Exploration, ecosystems

        // Literature (2)
        "manzoni-italiano",        // Manzoni - Italian, narrative
        "shakespeare-inglese",     // Shakespeare - English, theater

        // Arts (2)
        "leonardo-arte",           // Leonardo - Renaissance, creativity
        "mozart-musica",           // Mozart - Composition, harmony

        // Civics & Economics (2)
        "cicerone-civica",         // Cicero - Rhetoric, citizenship
        "smith-economia",          // Smith - Economics, markets

        // STEM & Health (3)
        "lovelace-informatica",    // Lovelace - Algorithms, programming
        "ippocrate-salute",        // Hippocrates - Medicine, wellness
        "socrate-filosofia",       // Socrates - Maieutics, critical thinking

        // International Law (1)
        "grozio-diritto"           // Grotius - International law, natural law
    ]

    /// The 3 Coordinatori (Coordinators)
    public static let coordinatori: [String] = [
        "ali-preside",             // Ali - Headmaster, coordinates all teachers
        "anna-assistente",         // Anna - Executive assistant, homework, reminders
        "jenny-accessibilita"      // Jenny - Accessibility champion, content adaptation
    ]

    /// All 21 education agents (18 maestri + 3 coordinatori)
    public static let educationAgents: [String] = maestri + coordinatori

    // MARK: - Business Edition Agents (10 total)

    public static let businessAgents: [String] = [
        "ali",                     // CEO - Strategy & leadership
        "frank-sales",             // Sales executive
        "emma-marketing",          // Marketing director
        "carlos-finance",          // Finance officer
        "sophia-hr",               // HR manager
        "lucas-ops",               // Operations manager
        "olivia-legal",            // Legal counsel
        "noah-product",            // Product manager
        "anna",                    // Executive assistant
        "david-analyst"            // Business analyst
    ]

    // MARK: - Developer Edition Agents

    public static let developerAgents: [String] = [
        "ali",                     // Technical director
        "marcus-swift",            // Swift/iOS expert
        "diana-python",            // Python expert
        "raj-devops",              // DevOps engineer
        "sarah-security",          // Security expert
        "alex-fullstack",          // Full-stack developer
        "nina-data",               // Data scientist
        "tom-qa",                  // QA engineer
        "lisa-ux",                 // UX designer
        "anna"                     // Project coordinator
    ]
}

// MARK: - Edition Manager

/// Manages the current edition and provides edition-specific functionality
@MainActor
public final class EditionManager: ObservableObject {
    public static let shared = EditionManager()

    /// Current active edition
    @Published public private(set) var currentEdition: ConvergioEdition

    /// UserDefaults key for edition persistence
    private let editionKey = "convergio.edition"

    private init() {
        // Load edition from UserDefaults, default to education
        if let savedEdition = UserDefaults.standard.string(forKey: editionKey),
           let edition = ConvergioEdition(rawValue: savedEdition) {
            self.currentEdition = edition
            logInfo("Loaded edition: \(edition.displayName)", category: "Edition")
        } else {
            self.currentEdition = .education
            logInfo("Defaulting to Education edition", category: "Edition")
        }
    }

    // MARK: - Edition Management

    /// Switch to a different edition
    public func setEdition(_ edition: ConvergioEdition) {
        guard edition != currentEdition else { return }

        logInfo("Switching edition from \(currentEdition.displayName) to \(edition.displayName)", category: "Edition")

        currentEdition = edition

        // Persist to UserDefaults
        UserDefaults.standard.set(edition.rawValue, forKey: editionKey)

        logInfo("Edition switched to \(edition.displayName)", category: "Edition")
    }

    // MARK: - Agent Filtering

    /// Get all allowed agent IDs for the current edition
    public var allowedAgents: Set<String> {
        currentEdition.allowedAgents
    }

    /// Check if an agent is allowed in the current edition
    public func isAgentAllowed(_ agentId: String) -> Bool {
        currentEdition.isAgentAllowed(agentId)
    }

    /// Filter a list of agent IDs to only those allowed in the current edition
    public func filterAgents(_ agentIds: [String]) -> [String] {
        switch currentEdition {
        case .master:
            return agentIds // Master edition allows all agents
        default:
            let allowed = currentEdition.allowedAgents
            return agentIds.filter { allowed.contains($0) }
        }
    }

    /// Get count of allowed agents in current edition
    public var allowedAgentCount: Int {
        switch currentEdition {
        case .education:
            return 21
        case .business:
            return 10
        case .developer:
            return 10
        case .master:
            return Int.max // All agents
        }
    }

    // MARK: - Edition Features

    /// Check if education features are enabled
    public var hasEducationFeatures: Bool {
        currentEdition == .education || currentEdition == .master
    }

    /// Check if business features are enabled
    public var hasBusinessFeatures: Bool {
        currentEdition == .business || currentEdition == .master
    }

    /// Check if developer features are enabled
    public var hasDeveloperFeatures: Bool {
        currentEdition == .developer || currentEdition == .master
    }

    /// Check if voice features are enabled (Education exclusive)
    public var hasVoiceFeatures: Bool {
        currentEdition == .education || currentEdition == .master
    }

    /// Check if FSRS flashcards are enabled (Education exclusive)
    public var hasFSRSFeatures: Bool {
        currentEdition == .education || currentEdition == .master
    }

    /// Check if safety guardrails are enabled (Education exclusive)
    public var hasSafetyGuardrails: Bool {
        currentEdition == .education || currentEdition == .master
    }

    /// Check if accessibility features are enabled (Education exclusive)
    public var hasAccessibilityFeatures: Bool {
        currentEdition == .education || currentEdition == .master
    }

    // MARK: - Education Specific

    /// Get list of maestri (teachers) - Education edition only
    public var maestri: [String] {
        guard hasEducationFeatures else { return [] }
        return ConvergioEdition.maestri
    }

    /// Get list of coordinatori (coordinators) - Education edition only
    public var coordinatori: [String] {
        guard hasEducationFeatures else { return [] }
        return ConvergioEdition.coordinatori
    }

    /// Check if an agent is a maestro
    public func isMaestro(_ agentId: String) -> Bool {
        ConvergioEdition.maestri.contains(agentId)
    }

    /// Check if an agent is a coordinator
    public func isCoordinator(_ agentId: String) -> Bool {
        ConvergioEdition.coordinatori.contains(agentId)
    }

    // MARK: - Debug

    /// Get edition info for debugging
    public var debugInfo: String {
        """
        Edition Manager Debug Info:
        - Current Edition: \(currentEdition.displayName)
        - Allowed Agents: \(allowedAgentCount)
        - Education Features: \(hasEducationFeatures)
        - Voice Features: \(hasVoiceFeatures)
        - Safety Guardrails: \(hasSafetyGuardrails)
        """
    }
}

// MARK: - SwiftUI Environment

/// Environment key for EditionManager
private struct EditionManagerKey: EnvironmentKey {
    @MainActor static let defaultValue = EditionManager.shared
}

extension EnvironmentValues {
    public var editionManager: EditionManager {
        get { self[EditionManagerKey.self] }
        set { self[EditionManagerKey.self] = newValue }
    }
}

// MARK: - View Extensions

extension View {
    /// Inject EditionManager into environment
    @MainActor public func withEditionManager(_ manager: EditionManager = .shared) -> some View {
        self.environment(\.editionManager, manager)
    }
}
