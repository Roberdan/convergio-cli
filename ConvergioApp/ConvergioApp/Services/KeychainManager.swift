/**
 * CONVERGIO NATIVE - Keychain Manager
 *
 * Secure storage for API keys using macOS Keychain.
 * Reads existing environment variables on first launch.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import Security

// MARK: - Provider Keys

/// Supported API providers
public enum APIProvider: String, CaseIterable, Identifiable {
    case anthropic = "ANTHROPIC_API_KEY"
    case openai = "OPENAI_API_KEY"
    case azureOpenAI = "AZURE_OPENAI_API_KEY"
    case gemini = "GEMINI_API_KEY"
    case openrouter = "OPENROUTER_API_KEY"
    case perplexity = "PERPLEXITY_API_KEY"
    case grok = "GROK_API_KEY"
    case azureRealtimeKey = "AZURE_OPENAI_REALTIME_API_KEY"
    case azureRealtimeEndpoint = "AZURE_OPENAI_REALTIME_ENDPOINT"
    case azureRealtimeDeployment = "AZURE_OPENAI_REALTIME_DEPLOYMENT"

    public var id: String { rawValue }

    public var displayName: String {
        switch self {
        case .anthropic: return "Anthropic"
        case .openai: return "OpenAI"
        case .azureOpenAI: return "Azure OpenAI"
        case .gemini: return "Google Gemini"
        case .openrouter: return "OpenRouter"
        case .perplexity: return "Perplexity"
        case .grok: return "Grok"
        case .azureRealtimeKey: return "Azure Realtime API Key"
        case .azureRealtimeEndpoint: return "Azure Realtime Endpoint"
        case .azureRealtimeDeployment: return "Azure Realtime Deployment"
        }
    }

    public var icon: String {
        switch self {
        case .anthropic: return "brain"
        case .openai: return "sparkles"
        case .azureOpenAI: return "cloud"
        case .gemini: return "g.circle"
        case .openrouter: return "arrow.triangle.branch"
        case .perplexity: return "magnifyingglass"
        case .grok: return "bolt"
        case .azureRealtimeKey, .azureRealtimeEndpoint, .azureRealtimeDeployment: return "cloud"
        }
    }

    public var envVariable: String { rawValue }

    /// Whether this provider is for Azure voice services
    public var isAzureVoice: Bool {
        switch self {
        case .azureRealtimeKey, .azureRealtimeEndpoint, .azureRealtimeDeployment: return true
        default: return false
        }
    }
}

// MARK: - Keychain Manager

@MainActor
public final class KeychainManager: ObservableObject {
    public static let shared = KeychainManager()

    private let service = "com.convergio.app"

    /// Published state for UI binding
    @Published public private(set) var hasAnthropicKey = false
    @Published public private(set) var hasOpenAIKey = false
    @Published public private(set) var hasAzureOpenAIKey = false
    @Published public private(set) var hasGeminiKey = false
    @Published public private(set) var hasOpenRouterKey = false

    private init() {
        // Check which keys are available
        refreshKeyStatus()
    }

    // MARK: - Key Status

    /// Refresh the status of all keys
    public func refreshKeyStatus() {
        hasAnthropicKey = getKey(for: .anthropic) != nil
        hasOpenAIKey = getKey(for: .openai) != nil
        hasAzureOpenAIKey = getKey(for: .azureOpenAI) != nil
        hasGeminiKey = getKey(for: .gemini) != nil
        hasOpenRouterKey = getKey(for: .openrouter) != nil
    }

    // MARK: - Import from Environment

    /// Import API keys from environment variables (first launch)
    public func importFromEnvironment() {
        logInfo("Importing API keys from environment variables", category: "Keychain")

        var imported = 0
        for provider in APIProvider.allCases {
            // Only import if not already in Keychain
            if getKey(for: provider) == nil {
                if let envValue = ProcessInfo.processInfo.environment[provider.envVariable],
                   !envValue.isEmpty {
                    if saveKey(envValue, for: provider) {
                        imported += 1
                        logInfo("Imported \(provider.displayName) key from environment", category: "Keychain")
                    }
                }
            }
        }

        // Also try importing Azure Realtime from alternate env var names
        importAzureRealtimeFromAlternateEnvVars()

        logInfo("Imported \(imported) API keys from environment", category: "Keychain")
        refreshKeyStatus()
    }

    /// Import Azure Realtime credentials from alternate environment variable names
    private func importAzureRealtimeFromAlternateEnvVars() {
        // Check for AZURE_OPENAI_REALTIME_* first, then fall back to AZURE_OPENAI_*
        let envMappings: [(APIProvider, [String])] = [
            (.azureRealtimeEndpoint, ["AZURE_OPENAI_REALTIME_ENDPOINT", "AZURE_OPENAI_ENDPOINT"]),
            (.azureRealtimeKey, ["AZURE_OPENAI_REALTIME_API_KEY", "AZURE_OPENAI_API_KEY"]),
            (.azureRealtimeDeployment, ["AZURE_OPENAI_REALTIME_DEPLOYMENT"])
        ]

        for (provider, envVars) in envMappings {
            if getKey(for: provider) == nil {
                for envVar in envVars {
                    if let value = ProcessInfo.processInfo.environment[envVar],
                       !value.isEmpty {
                        if saveKey(value, for: provider) {
                            logInfo("Imported \(provider.displayName) from \(envVar)", category: "Keychain")
                        }
                        break
                    }
                }
            }
        }
    }

    /// Export keys to environment variables for C library
    public func exportToEnvironment() {
        logInfo("Exporting API keys to environment variables", category: "Keychain")

        for provider in APIProvider.allCases {
            if let key = getKey(for: provider) {
                setenv(provider.envVariable, key, 1)
                logDebug("Set \(provider.envVariable) environment variable", category: "Keychain")
            }
        }
    }

    // MARK: - Save Key

    /// Save an API key to Keychain
    @discardableResult
    public func saveKey(_ key: String, for provider: APIProvider) -> Bool {
        let account = provider.rawValue

        // Delete existing key first
        deleteKey(for: provider)

        guard let data = key.data(using: .utf8) else {
            logError("Failed to encode key for \(provider.displayName)", category: "Keychain")
            return false
        }

        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account,
            kSecValueData as String: data,
            kSecAttrAccessible as String: kSecAttrAccessibleWhenUnlocked
        ]

        let status = SecItemAdd(query as CFDictionary, nil)

        if status == errSecSuccess {
            logInfo("Saved \(provider.displayName) key to Keychain", category: "Keychain")
            refreshKeyStatus()

            // Also update environment variable immediately
            setenv(provider.envVariable, key, 1)
            return true
        } else {
            logError("Failed to save \(provider.displayName) key: \(status)", category: "Keychain")
            return false
        }
    }

    // MARK: - Get Key

    /// Get an API key from Keychain
    public func getKey(for provider: APIProvider) -> String? {
        let account = provider.rawValue

        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account,
            kSecReturnData as String: true,
            kSecMatchLimit as String: kSecMatchLimitOne
        ]

        var result: AnyObject?
        let status = SecItemCopyMatching(query as CFDictionary, &result)

        if status == errSecSuccess, let data = result as? Data {
            return String(data: data, encoding: .utf8)
        }

        // Fallback to environment variable
        if let envValue = ProcessInfo.processInfo.environment[provider.envVariable],
           !envValue.isEmpty {
            return envValue
        }

        return nil
    }

    // MARK: - Delete Key

    /// Delete an API key from Keychain
    @discardableResult
    public func deleteKey(for provider: APIProvider) -> Bool {
        let account = provider.rawValue

        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account
        ]

        let status = SecItemDelete(query as CFDictionary)

        if status == errSecSuccess || status == errSecItemNotFound {
            logInfo("Deleted \(provider.displayName) key from Keychain", category: "Keychain")
            refreshKeyStatus()

            // Also clear environment variable
            unsetenv(provider.envVariable)
            return true
        } else {
            logError("Failed to delete \(provider.displayName) key: \(status)", category: "Keychain")
            return false
        }
    }

    // MARK: - Validation

    /// Check if a key looks valid (basic format check)
    public func isValidKeyFormat(_ key: String, for provider: APIProvider) -> Bool {
        guard !key.isEmpty else { return false }

        switch provider {
        case .anthropic:
            // Anthropic keys start with "sk-ant-"
            return key.hasPrefix("sk-ant-") && key.count > 20
        case .openai:
            // OpenAI keys start with "sk-"
            return key.hasPrefix("sk-") && key.count > 20
        case .azureOpenAI:
            // Azure OpenAI keys are 32 hex characters
            return key.count >= 32
        case .gemini:
            // Google API keys are typically 39 characters
            return key.count >= 30
        case .openrouter:
            // OpenRouter keys start with "sk-or-"
            return key.hasPrefix("sk-or-") && key.count > 20
        case .perplexity:
            // Perplexity keys start with "pplx-"
            return key.hasPrefix("pplx-") && key.count > 20
        case .grok:
            // Grok keys format
            return key.count > 20
        case .azureRealtimeKey:
            // Azure API keys are typically 32 characters
            return key.count >= 32
        case .azureRealtimeEndpoint:
            // Azure endpoint should be a valid URL
            return key.hasPrefix("https://") && key.contains(".openai.azure.com")
        case .azureRealtimeDeployment:
            // Deployment name should be alphanumeric with dashes
            return key.count > 0 && key.range(of: "^[a-zA-Z0-9-]+$", options: .regularExpression) != nil
        }
    }

    /// Get masked version of key for display
    public func getMaskedKey(for provider: APIProvider) -> String? {
        guard let key = getKey(for: provider) else { return nil }

        if key.count <= 8 {
            return String(repeating: "•", count: key.count)
        }

        let prefix = String(key.prefix(4))
        let suffix = String(key.suffix(4))
        let middle = String(repeating: "•", count: min(20, key.count - 8))

        return "\(prefix)\(middle)\(suffix)"
    }
}

// MARK: - App Initialization Extension

extension KeychainManager {
    /// Initialize keys on app launch
    public func initializeOnLaunch() {
        logInfo("Initializing KeychainManager on launch", category: "Keychain")

        // First, try to import from environment if Keychain is empty
        if !hasAnthropicKey && !hasOpenAIKey && !hasAzureOpenAIKey && !hasGeminiKey {
            importFromEnvironment()
        }

        // Then export all keys to environment for C library
        exportToEnvironment()

        logInfo("KeychainManager initialized - Anthropic: \(hasAnthropicKey), OpenAI: \(hasOpenAIKey), Azure: \(hasAzureOpenAIKey), Gemini: \(hasGeminiKey)", category: "Keychain")
    }
}
