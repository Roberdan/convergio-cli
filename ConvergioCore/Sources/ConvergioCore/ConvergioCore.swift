/**
 * CONVERGIO CORE
 *
 * Swift wrapper for the Convergio C library.
 * Provides a native Swift interface for the AI executive team orchestrator.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
@_exported import CConvergio

// MARK: - Module Version

/// ConvergioCore module version
public let convergioVersion = "5.0.0"

// MARK: - Provider Type

/// Available AI providers
public enum ProviderType: Int, CaseIterable, Sendable {
    case anthropic = 0
    case openai = 1
    case gemini = 2
    case openRouter = 3
    case ollama = 4
    case mlx = 5

    /// Human-readable name
    public var displayName: String {
        switch self {
        case .anthropic: return "Anthropic"
        case .openai: return "OpenAI"
        case .gemini: return "Google Gemini"
        case .openRouter: return "OpenRouter"
        case .ollama: return "Ollama"
        case .mlx: return "MLX (Local)"
        }
    }

    /// Whether this provider is available (API key configured)
    public var isAvailable: Bool {
        convergio_is_provider_available(CProviderType(rawValue: CProviderType.RawValue(rawValue)))
    }

    /// Whether this is a local provider
    public var isLocal: Bool {
        switch self {
        case .ollama, .mlx:
            return true
        default:
            return false
        }
    }
}

// MARK: - Convenience Functions

/// Initialize the Convergio system
/// - Parameter budgetLimit: Maximum spend in USD (default: $10)
/// - Throws: OrchestratorError if initialization fails
@MainActor
public func initializeConvergio(budgetLimit: Double = 10.0) async throws {
    try await Orchestrator.shared.initialize(budgetLimit: budgetLimit)
}

/// Shutdown the Convergio system
@MainActor
public func shutdownConvergio() {
    Orchestrator.shared.shutdown()
}
