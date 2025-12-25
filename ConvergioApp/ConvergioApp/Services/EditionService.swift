/**
 * CONVERGIO NATIVE - Edition Service
 *
 * Provides edition-specific services including agent filtering,
 * feature flags, and edition-aware configurations.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import SwiftUI

// MARK: - Edition Service

/// Service layer for edition-specific functionality
@MainActor
public final class EditionService: ObservableObject {
    public static let shared = EditionService()

    /// Reference to the edition manager
    private let editionManager = EditionManager.shared

    /// Feature flags based on current edition
    @Published public private(set) var features: EditionFeatures

    private init() {
        self.features = EditionFeatures(edition: editionManager.currentEdition)

        // Observe edition changes
        Task { @MainActor in
            for await _ in NotificationCenter.default.notifications(named: .editionDidChange) {
                self.updateFeatures()
            }
        }
    }

    // MARK: - Edition Access

    /// Get the current edition
    public var currentEdition: ConvergioEdition {
        editionManager.currentEdition
    }

    /// Set the current edition
    public func setEdition(_ edition: ConvergioEdition) {
        editionManager.setEdition(edition)
        updateFeatures()

        // Post notification
        NotificationCenter.default.post(name: .editionDidChange, object: edition)
    }

    // MARK: - Feature Flags

    /// Update feature flags based on current edition
    private func updateFeatures() {
        features = EditionFeatures(edition: editionManager.currentEdition)
        logInfo("Updated features for \(currentEdition.displayName) edition", category: "Edition")
    }

    // MARK: - Agent Filtering

    /// Filter agents by current edition
    public func filterAgents(_ agents: [AgentInfo]) -> [AgentInfo] {
        switch currentEdition {
        case .master:
            return agents // Master edition shows all agents
        default:
            let allowedIds = editionManager.allowedAgents
            return agents.filter { allowedIds.contains($0.id) }
        }
    }

    /// Filter agent IDs by current edition
    public func filterAgentIds(_ agentIds: [String]) -> [String] {
        editionManager.filterAgents(agentIds)
    }

    /// Check if an agent is allowed
    public func isAgentAllowed(_ agentId: String) -> Bool {
        editionManager.isAgentAllowed(agentId)
    }

    // MARK: - Education Features

    /// Get maestri for education edition
    public var maestri: [String] {
        editionManager.maestri
    }

    /// Get coordinatori for education edition
    public var coordinatori: [String] {
        editionManager.coordinatori
    }

    /// Check if agent is a maestro
    public func isMaestro(_ agentId: String) -> Bool {
        editionManager.isMaestro(agentId)
    }

    /// Check if agent is a coordinator
    public func isCoordinator(_ agentId: String) -> Bool {
        editionManager.isCoordinator(agentId)
    }

    /// Get subject for a maestro
    public func getMaestroSubject(_ agentId: String) -> String? {
        guard isMaestro(agentId) else { return nil }

        // Extract subject from agent ID (e.g., "euclide-matematica" -> "Matematica")
        let parts = agentId.split(separator: "-")
        guard parts.count >= 2 else { return nil }

        let subject = String(parts[1])
        return subject.capitalized
    }

    // MARK: - Edition Requirements

    /// Check if current edition requires API key validation
    public var requiresApiKey: Bool {
        // All editions require API keys
        return true
    }

    /// Check if current edition requires age verification (Education only)
    public var requiresAgeVerification: Bool {
        return currentEdition == .education
    }

    /// Check if current edition requires safety guardrails
    public var requiresSafetyGuardrails: Bool {
        return editionManager.hasSafetyGuardrails
    }

    /// Check if current edition supports voice features
    public var supportsVoice: Bool {
        return editionManager.hasVoiceFeatures
    }

    /// Check if current edition supports FSRS
    public var supportsFSRS: Bool {
        return editionManager.hasFSRSFeatures
    }

    /// Check if current edition supports accessibility features
    public var supportsAccessibility: Bool {
        return editionManager.hasAccessibilityFeatures
    }

    // MARK: - Edition Metadata

    /// Get edition display name
    public var editionDisplayName: String {
        currentEdition.displayName
    }

    /// Get edition tagline
    public var editionTagline: String {
        currentEdition.tagline
    }

    /// Get edition icon
    public var editionIcon: String {
        currentEdition.icon
    }

    /// Get edition color
    public var editionColor: Color {
        currentEdition.color
    }

    /// Get allowed agent count
    public var allowedAgentCount: Int {
        editionManager.allowedAgentCount
    }
}

// MARK: - Edition Features

/// Feature flags for different editions
public struct EditionFeatures: Equatable {
    public let edition: ConvergioEdition

    // Core Features
    public let hasChat: Bool
    public let hasAgentOrchestration: Bool
    public let hasCostTracking: Bool

    // Education Features
    public let hasMaestri: Bool
    public let hasVoice: Bool
    public let hasFSRS: Bool
    public let hasQuizzes: Bool
    public let hasLibretto: Bool
    public let hasHomeworkAssistant: Bool
    public let hasMindmaps: Bool
    public let hasSafetyGuardrails: Bool
    public let hasAccessibility: Bool
    public let hasStudentProfile: Bool

    // Business Features
    public let hasSalesTools: Bool
    public let hasMarketingTools: Bool
    public let hasFinanceTools: Bool
    public let hasHRTools: Bool

    // Developer Features
    public let hasCodeGeneration: Bool
    public let hasDevOpsTools: Bool
    public let hasSecurityAudit: Bool
    public let hasDataAnalysis: Bool

    // Advanced Features (Master only)
    public let hasAdvancedOrchestration: Bool
    public let hasCustomAgents: Bool
    public let hasAPIAccess: Bool

    public init(edition: ConvergioEdition) {
        self.edition = edition

        // Core features (all editions)
        self.hasChat = true
        self.hasAgentOrchestration = true
        self.hasCostTracking = true

        // Education features
        self.hasMaestri = edition == .education || edition == .master
        self.hasVoice = edition == .education || edition == .master
        self.hasFSRS = edition == .education || edition == .master
        self.hasQuizzes = edition == .education || edition == .master
        self.hasLibretto = edition == .education || edition == .master
        self.hasHomeworkAssistant = edition == .education || edition == .master
        self.hasMindmaps = edition == .education || edition == .master
        self.hasSafetyGuardrails = edition == .education || edition == .master
        self.hasAccessibility = edition == .education || edition == .master
        self.hasStudentProfile = edition == .education || edition == .master

        // Business features
        self.hasSalesTools = edition == .business || edition == .master
        self.hasMarketingTools = edition == .business || edition == .master
        self.hasFinanceTools = edition == .business || edition == .master
        self.hasHRTools = edition == .business || edition == .master

        // Developer features
        self.hasCodeGeneration = edition == .developer || edition == .master
        self.hasDevOpsTools = edition == .developer || edition == .master
        self.hasSecurityAudit = edition == .developer || edition == .master
        self.hasDataAnalysis = edition == .developer || edition == .master

        // Master-only features
        self.hasAdvancedOrchestration = edition == .master
        self.hasCustomAgents = edition == .master
        self.hasAPIAccess = edition == .master
    }
}

// MARK: - Agent Info

/// Lightweight agent information for filtering
public struct AgentInfo: Identifiable, Equatable {
    public let id: String
    public let name: String
    public let role: String

    public init(id: String, name: String, role: String) {
        self.id = id
        self.name = name
        self.role = role
    }
}

// MARK: - Notification Names

extension Notification.Name {
    /// Posted when edition changes
    public static let editionDidChange = Notification.Name("editionDidChange")
}

// MARK: - Edition Validator

/// Validates edition requirements and constraints
public struct EditionValidator {
    private let edition: ConvergioEdition

    public init(edition: ConvergioEdition) {
        self.edition = edition
    }

    /// Validate if feature is available in current edition
    public func canUseFeature(_ feature: String) -> Bool {
        let features = EditionFeatures(edition: edition)

        switch feature.lowercased() {
        case "voice": return features.hasVoice
        case "fsrs": return features.hasFSRS
        case "quiz": return features.hasQuizzes
        case "libretto": return features.hasLibretto
        case "homework": return features.hasHomeworkAssistant
        case "mindmap": return features.hasMindmaps
        case "safety": return features.hasSafetyGuardrails
        case "accessibility": return features.hasAccessibility
        default: return true // Unknown features default to allowed
        }
    }

    /// Get upgrade message for locked feature
    public func getUpgradeMessage(for feature: String) -> String {
        switch edition {
        case .education:
            return "This feature is exclusive to Education edition."
        case .business:
            return "Upgrade to Master edition to access \(feature)."
        case .developer:
            return "Upgrade to Master edition to access \(feature)."
        case .master:
            return "" // Master has everything
        }
    }

    /// Check if agent count is within edition limits
    public func validateAgentCount(_ count: Int) -> Bool {
        switch edition {
        case .education:
            return count <= 20
        case .business, .developer:
            return count <= 10
        case .master:
            return true // No limit
        }
    }
}

// MARK: - SwiftUI Environment

/// Environment key for EditionService
private struct EditionServiceKey: EnvironmentKey {
    static let defaultValue = EditionService.shared
}

extension EnvironmentValues {
    public var editionService: EditionService {
        get { self[EditionServiceKey.self] }
        set { self[EditionServiceKey.self] = newValue }
    }
}

// MARK: - View Extensions

extension View {
    /// Inject EditionService into environment
    public func withEditionService(_ service: EditionService = .shared) -> some View {
        self.environment(\.editionService, service)
    }

    /// Conditionally show view based on edition feature
    public func editionFeature(_ feature: KeyPath<EditionFeatures, Bool>) -> some View {
        self.modifier(EditionFeatureModifier(feature: feature))
    }
}

// MARK: - View Modifiers

/// View modifier that shows/hides content based on edition features
private struct EditionFeatureModifier: ViewModifier {
    @Environment(\.editionService) private var editionService
    let feature: KeyPath<EditionFeatures, Bool>

    func body(content: Content) -> some View {
        if editionService.features[keyPath: feature] {
            content
        }
    }
}
