/**
 * CONVERGIO NATIVE - Chat Message Models
 *
 * Data models for chat messages and conversation history.
 * Supports multiple AI providers with consistent message format.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation

// MARK: - Chat Role

/// Role of a message in a conversation
public enum ChatRole: String, Codable, CaseIterable {
    case system
    case user
    case assistant

    public var displayName: String {
        switch self {
        case .system: return "System"
        case .user: return "User"
        case .assistant: return "Assistant"
        }
    }

    public var icon: String {
        switch self {
        case .system: return "gear"
        case .user: return "person.fill"
        case .assistant: return "brain"
        }
    }
}

// MARK: - Chat Message

/// A single message in a conversation
public struct ChatMessage: Identifiable, Codable, Equatable {
    public let id: UUID
    public let role: ChatRole
    public let content: String
    public let timestamp: Date

    /// Optional metadata
    public var provider: String?
    public var model: String?
    public var tokens: Int?
    public var cost: Double?

    public init(
        id: UUID = UUID(),
        role: ChatRole,
        content: String,
        timestamp: Date = Date(),
        provider: String? = nil,
        model: String? = nil,
        tokens: Int? = nil,
        cost: Double? = nil
    ) {
        self.id = id
        self.role = role
        self.content = content
        self.timestamp = timestamp
        self.provider = provider
        self.model = model
        self.tokens = tokens
        self.cost = cost
    }

    /// Create a system message
    public static func system(_ content: String) -> ChatMessage {
        ChatMessage(role: .system, content: content)
    }

    /// Create a user message
    public static func user(_ content: String) -> ChatMessage {
        ChatMessage(role: .user, content: content)
    }

    /// Create an assistant message
    public static func assistant(_ content: String, provider: String? = nil, model: String? = nil) -> ChatMessage {
        ChatMessage(role: .assistant, content: content, provider: provider, model: model)
    }

    /// Get a summary of the message for display
    public var summary: String {
        let maxLength = 50
        if content.count > maxLength {
            return String(content.prefix(maxLength)) + "..."
        }
        return content
    }
}

// MARK: - Conversation

/// A conversation with message history management
public struct Conversation: Identifiable, Codable {
    public let id: UUID
    public var title: String
    public var messages: [ChatMessage]
    public let createdAt: Date
    public var updatedAt: Date

    /// System prompt (always the first message if present)
    public var systemPrompt: String? {
        messages.first(where: { $0.role == .system })?.content
    }

    /// Total number of tokens used in this conversation
    public var totalTokens: Int {
        messages.compactMap(\.tokens).reduce(0, +)
    }

    /// Total cost of this conversation
    public var totalCost: Double {
        messages.compactMap(\.cost).reduce(0, +)
    }

    /// Last message timestamp
    public var lastMessageTime: Date? {
        messages.last?.timestamp
    }

    public init(
        id: UUID = UUID(),
        title: String = "New Conversation",
        messages: [ChatMessage] = [],
        createdAt: Date = Date(),
        updatedAt: Date = Date()
    ) {
        self.id = id
        self.title = title
        self.messages = messages
        self.createdAt = createdAt
        self.updatedAt = updatedAt
    }

    /// Add a message to the conversation
    public mutating func addMessage(_ message: ChatMessage) {
        messages.append(message)
        updatedAt = Date()
    }

    /// Set or update the system prompt
    public mutating func setSystemPrompt(_ prompt: String) {
        // Remove existing system message
        messages.removeAll(where: { $0.role == .system })

        // Add new system message at the beginning
        messages.insert(ChatMessage.system(prompt), at: 0)
        updatedAt = Date()
    }

    /// Get messages formatted for API request
    public func getAPIMessages(includeSystem: Bool = true) -> [[String: String]] {
        let messagesToInclude = includeSystem ? messages : messages.filter { $0.role != .system }

        return messagesToInclude.map { message in
            [
                "role": message.role.rawValue,
                "content": message.content
            ]
        }
    }

    /// Clear all messages except system prompt
    public mutating func clearMessages(keepSystemPrompt: Bool = true) {
        if keepSystemPrompt, let systemMsg = messages.first(where: { $0.role == .system }) {
            messages = [systemMsg]
        } else {
            messages = []
        }
        updatedAt = Date()
    }
}

// MARK: - Conversation Manager

/// Manages multiple conversations
@MainActor
public final class ConversationManager: ObservableObject {
    @Published public private(set) var conversations: [Conversation] = []
    @Published public var currentConversation: Conversation?

    private let userDefaults = UserDefaults.standard
    private let conversationsKey = "convergio.conversations"

    public static let shared = ConversationManager()

    private init() {
        loadConversations()
    }

    // MARK: - Persistence

    private func loadConversations() {
        guard let data = userDefaults.data(forKey: conversationsKey) else {
            logInfo("No saved conversations found", category: "ConversationManager")
            return
        }

        do {
            conversations = try JSONDecoder().decode([Conversation].self, from: data)
            logInfo("Loaded \(conversations.count) conversations", category: "ConversationManager")

            // Restore last conversation
            if let last = conversations.first {
                currentConversation = last
            }
        } catch {
            logError("Failed to load conversations: \(error)", category: "ConversationManager")
        }
    }

    private func saveConversations() {
        do {
            let data = try JSONEncoder().encode(conversations)
            userDefaults.set(data, forKey: conversationsKey)
            logDebug("Saved \(conversations.count) conversations", category: "ConversationManager")
        } catch {
            logError("Failed to save conversations: \(error)", category: "ConversationManager")
        }
    }

    // MARK: - Operations

    public func createConversation(title: String = "New Conversation", systemPrompt: String? = nil) -> Conversation {
        var conversation = Conversation(title: title)

        if let systemPrompt = systemPrompt {
            conversation.setSystemPrompt(systemPrompt)
        }

        conversations.insert(conversation, at: 0)
        currentConversation = conversation
        saveConversations()

        logInfo("Created new conversation: \(title)", category: "ConversationManager")
        return conversation
    }

    public func updateConversation(_ conversation: Conversation) {
        if let index = conversations.firstIndex(where: { $0.id == conversation.id }) {
            conversations[index] = conversation
            if currentConversation?.id == conversation.id {
                currentConversation = conversation
            }
            saveConversations()
        }
    }

    public func deleteConversation(_ conversation: Conversation) {
        conversations.removeAll(where: { $0.id == conversation.id })
        if currentConversation?.id == conversation.id {
            currentConversation = conversations.first
        }
        saveConversations()
        logInfo("Deleted conversation: \(conversation.title)", category: "ConversationManager")
    }

    public func addMessage(_ message: ChatMessage, to conversationId: UUID? = nil) {
        let targetId = conversationId ?? currentConversation?.id

        guard let targetId = targetId,
              let index = conversations.firstIndex(where: { $0.id == targetId }) else {
            logError("No conversation found to add message", category: "ConversationManager")
            return
        }

        conversations[index].addMessage(message)
        if currentConversation?.id == targetId {
            currentConversation = conversations[index]
        }
        saveConversations()
    }
}
