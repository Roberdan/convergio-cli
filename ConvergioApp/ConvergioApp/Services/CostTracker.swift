/**
 * CONVERGIO NATIVE - Cost Tracker
 *
 * Tracks token usage and costs across AI providers.
 * Provides session budget management and usage analytics.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation

// MARK: - Model Pricing

/// Pricing information for AI models
public struct ModelPricing {
    public let inputTokensPerMillionCents: Double
    public let outputTokensPerMillionCents: Double

    /// Calculate cost for token usage
    public func calculateCost(inputTokens: Int, outputTokens: Int) -> Double {
        let inputCost = (Double(inputTokens) / 1_000_000.0) * inputTokensPerMillionCents / 100.0
        let outputCost = (Double(outputTokens) / 1_000_000.0) * outputTokensPerMillionCents / 100.0
        return inputCost + outputCost
    }
}

// MARK: - Model Catalog

/// Catalog of model pricing
public enum ModelCatalog {
    // Azure OpenAI / OpenAI GPT-4o models
    public static let gpt4o = ModelPricing(
        inputTokensPerMillionCents: 250,    // $2.50 per 1M input tokens
        outputTokensPerMillionCents: 1000   // $10.00 per 1M output tokens
    )

    public static let gpt4oMini = ModelPricing(
        inputTokensPerMillionCents: 15,     // $0.15 per 1M input tokens
        outputTokensPerMillionCents: 60     // $0.60 per 1M output tokens
    )

    // Anthropic Claude models
    public static let claude35Sonnet = ModelPricing(
        inputTokensPerMillionCents: 300,    // $3.00 per 1M input tokens
        outputTokensPerMillionCents: 1500   // $15.00 per 1M output tokens
    )

    public static let claude35Haiku = ModelPricing(
        inputTokensPerMillionCents: 25,     // $0.25 per 1M input tokens
        outputTokensPerMillionCents: 125    // $1.25 per 1M output tokens
    )

    // Google Gemini models
    public static let gemini15Pro = ModelPricing(
        inputTokensPerMillionCents: 125,    // $1.25 per 1M input tokens
        outputTokensPerMillionCents: 500    // $5.00 per 1M output tokens
    )

    public static let gemini15Flash = ModelPricing(
        inputTokensPerMillionCents: 8,      // $0.075 per 1M input tokens
        outputTokensPerMillionCents: 30     // $0.30 per 1M output tokens
    )

    /// Get pricing for a model by name
    public static func getPricing(for model: String) -> ModelPricing {
        let lowercased = model.lowercased()

        // GPT models
        if lowercased.contains("gpt-4o-mini") {
            return gpt4oMini
        } else if lowercased.contains("gpt-4o") {
            return gpt4o
        }

        // Claude models
        if lowercased.contains("claude") && lowercased.contains("sonnet") {
            return claude35Sonnet
        } else if lowercased.contains("claude") && lowercased.contains("haiku") {
            return claude35Haiku
        }

        // Gemini models
        if lowercased.contains("gemini") && lowercased.contains("flash") {
            return gemini15Flash
        } else if lowercased.contains("gemini") {
            return gemini15Pro
        }

        // Default to GPT-4o mini (most economical)
        return gpt4oMini
    }
}

// MARK: - Token Counter

/// Utility for counting tokens (approximate)
public struct TokenCounter {
    /// Approximate token count (rough estimate: 1 token ≈ 4 characters)
    public static func estimateTokens(_ text: String) -> Int {
        // This is a rough approximation
        // For production, use tiktoken or equivalent
        let characters = text.count
        return max(1, characters / 4)
    }

    /// Count tokens in messages
    public static func estimateTokens(messages: [[String: String]]) -> Int {
        var total = 0

        for message in messages {
            // Add tokens for role
            if let role = message["role"] {
                total += estimateTokens(role)
            }

            // Add tokens for content
            if let content = message["content"] {
                total += estimateTokens(content)
            }

            // Add overhead per message (approximately 4 tokens)
            total += 4
        }

        return total
    }
}

// MARK: - Usage Record

/// Record of API usage
public struct UsageRecord: Codable, Identifiable {
    public let id: UUID
    public let timestamp: Date
    public let provider: String
    public let model: String
    public let inputTokens: Int
    public let outputTokens: Int
    public let cost: Double

    public var totalTokens: Int {
        inputTokens + outputTokens
    }

    public init(
        id: UUID = UUID(),
        timestamp: Date = Date(),
        provider: String,
        model: String,
        inputTokens: Int,
        outputTokens: Int,
        cost: Double
    ) {
        self.id = id
        self.timestamp = timestamp
        self.provider = provider
        self.model = model
        self.inputTokens = inputTokens
        self.outputTokens = outputTokens
        self.cost = cost
    }
}

// MARK: - Cost Tracker

/// Tracks costs and usage across sessions
@MainActor
public final class CostTracker: ObservableObject {
    public static let shared = CostTracker()

    // Current session tracking
    @Published public private(set) var sessionRecords: [UsageRecord] = []
    @Published public private(set) var sessionStartTime: Date = Date()

    // Budget management
    @Published public var dailyBudgetDollars: Double = 5.0
    @Published public var monthlyBudgetDollars: Double = 100.0

    // All-time tracking
    private var allTimeRecords: [UsageRecord] = []
    private let userDefaults = UserDefaults.standard
    private let recordsKey = "convergio.usage_records"

    private init() {
        loadRecords()
    }

    // MARK: - Persistence

    private func loadRecords() {
        guard let data = userDefaults.data(forKey: recordsKey) else {
            logInfo("No usage records found", category: "CostTracker")
            return
        }

        do {
            allTimeRecords = try JSONDecoder().decode([UsageRecord].self, from: data)
            logInfo("Loaded \(allTimeRecords.count) usage records", category: "CostTracker")
        } catch {
            logError("Failed to load usage records: \(error)", category: "CostTracker")
        }
    }

    private func saveRecords() {
        do {
            let data = try JSONEncoder().encode(allTimeRecords)
            userDefaults.set(data, forKey: recordsKey)
        } catch {
            logError("Failed to save usage records: \(error)", category: "CostTracker")
        }
    }

    // MARK: - Recording Usage

    /// Record API usage
    public func recordUsage(
        provider: String,
        model: String,
        inputTokens: Int,
        outputTokens: Int
    ) -> UsageRecord {
        let pricing = ModelCatalog.getPricing(for: model)
        let cost = pricing.calculateCost(inputTokens: inputTokens, outputTokens: outputTokens)

        let record = UsageRecord(
            provider: provider,
            model: model,
            inputTokens: inputTokens,
            outputTokens: outputTokens,
            cost: cost
        )

        sessionRecords.append(record)
        allTimeRecords.append(record)
        saveRecords()

        logInfo("Recorded usage: \(provider)/\(model) - \(inputTokens + outputTokens) tokens, $\(String(format: "%.4f", cost))", category: "CostTracker")

        return record
    }

    // MARK: - Session Stats

    /// Total cost for current session
    public var sessionCost: Double {
        sessionRecords.reduce(0) { $0 + $1.cost }
    }

    /// Total tokens for current session
    public var sessionTokens: Int {
        sessionRecords.reduce(0) { $0 + $1.totalTokens }
    }

    /// Reset current session
    public func resetSession() {
        sessionRecords = []
        sessionStartTime = Date()
        logInfo("Reset session tracking", category: "CostTracker")
    }

    // MARK: - Daily Stats

    /// Get records for today
    private func getTodayRecords() -> [UsageRecord] {
        let calendar = Calendar.current
        let today = calendar.startOfDay(for: Date())

        return allTimeRecords.filter { record in
            calendar.startOfDay(for: record.timestamp) == today
        }
    }

    /// Total cost for today
    public var dailyCost: Double {
        getTodayRecords().reduce(0) { $0 + $1.cost }
    }

    /// Total tokens for today
    public var dailyTokens: Int {
        getTodayRecords().reduce(0) { $0 + $1.totalTokens }
    }

    /// Remaining daily budget
    public var dailyBudgetRemaining: Double {
        max(0, dailyBudgetDollars - dailyCost)
    }

    /// Is daily budget exceeded?
    public var isDailyBudgetExceeded: Bool {
        dailyCost >= dailyBudgetDollars
    }

    // MARK: - Monthly Stats

    /// Get records for this month
    private func getMonthRecords() -> [UsageRecord] {
        let calendar = Calendar.current
        let now = Date()

        return allTimeRecords.filter { record in
            calendar.isDate(record.timestamp, equalTo: now, toGranularity: .month)
        }
    }

    /// Total cost for this month
    public var monthlyCost: Double {
        getMonthRecords().reduce(0) { $0 + $1.cost }
    }

    /// Total tokens for this month
    public var monthlyTokens: Int {
        getMonthRecords().reduce(0) { $0 + $1.totalTokens }
    }

    /// Remaining monthly budget
    public var monthlyBudgetRemaining: Double {
        max(0, monthlyBudgetDollars - monthlyCost)
    }

    /// Is monthly budget exceeded?
    public var isMonthlyBudgetExceeded: Bool {
        monthlyCost >= monthlyBudgetDollars
    }

    // MARK: - Analytics

    /// Get usage breakdown by provider
    public func getProviderBreakdown(records: [UsageRecord]) -> [String: (tokens: Int, cost: Double)] {
        var breakdown: [String: (tokens: Int, cost: Double)] = [:]

        for record in records {
            let current = breakdown[record.provider] ?? (tokens: 0, cost: 0.0)
            breakdown[record.provider] = (
                tokens: current.tokens + record.totalTokens,
                cost: current.cost + record.cost
            )
        }

        return breakdown
    }

    /// Get usage breakdown by model
    public func getModelBreakdown(records: [UsageRecord]) -> [String: (tokens: Int, cost: Double)] {
        var breakdown: [String: (tokens: Int, cost: Double)] = [:]

        for record in records {
            let current = breakdown[record.model] ?? (tokens: 0, cost: 0.0)
            breakdown[record.model] = (
                tokens: current.tokens + record.totalTokens,
                cost: current.cost + record.cost
            )
        }

        return breakdown
    }

    /// Format cost for display
    public static func formatCost(_ cost: Double) -> String {
        if cost < 0.01 {
            return String(format: "$%.4f", cost)
        } else if cost < 1.0 {
            return String(format: "$%.3f", cost)
        } else {
            return String(format: "$%.2f", cost)
        }
    }
}
