/**
 * CONVERGIO NATIVE - Message Swift Type
 *
 * Swift-native wrapper for Message from the C library.
 * Provides a clean, type-safe interface for conversation history.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import CConvergio

// MARK: - Message Type

/// The type of message in a conversation
public enum MessageType: Int, CaseIterable, Sendable {
    case userInput = 0       // From human
    case agentThought = 1    // Internal reasoning
    case agentAction = 2     // Tool/action request
    case agentResponse = 3   // Response to user/other agent
    case taskDelegate = 4    // Delegate to sub-agent
    case taskReport = 5      // Report back to orchestrator
    case convergence = 6     // Final converged answer
    case error = 7           // Error condition

    init(from cType: CMessageType) {
        self = MessageType(rawValue: Int(cType.rawValue)) ?? .userInput
    }

    /// Whether this message is from the user
    public var isUserMessage: Bool {
        self == .userInput
    }

    /// Whether this message is a final response
    public var isResponse: Bool {
        switch self {
        case .agentResponse, .convergence:
            return true
        default:
            return false
        }
    }

    /// Whether this message should be displayed to the user
    public var isVisible: Bool {
        switch self {
        case .userInput, .agentResponse, .convergence, .error:
            return true
        case .agentThought, .agentAction, .taskDelegate, .taskReport:
            return false
        }
    }

    /// SF Symbol icon for the message type
    public var iconName: String {
        switch self {
        case .userInput: return "person.fill"
        case .agentThought: return "brain"
        case .agentAction: return "wrench.and.screwdriver"
        case .agentResponse: return "bubble.left.fill"
        case .taskDelegate: return "arrow.right.circle"
        case .taskReport: return "doc.text"
        case .convergence: return "arrow.triangle.merge"
        case .error: return "exclamationmark.triangle"
        }
    }
}

// MARK: - Message

/// A message in the conversation history
public struct Message: Identifiable, Sendable {
    /// Unique identifier
    public let id: UInt64

    /// Message type
    public let type: MessageType

    /// Sender's semantic ID (0 for user)
    public let senderId: UInt64

    /// Message content
    public let content: String

    /// Timestamp
    public let timestamp: Date

    /// Initialize from C message pointer
    internal init(cMessage: OpaquePointer) {
        // Note: We need to extract data from C - using UInt64 as placeholder
        // The actual implementation depends on the C API
        self.id = UInt64(bitPattern: Int64(truncatingIfNeeded: Int(bitPattern: cMessage)))
        self.type = MessageType(from: convergio_message_get_type(cMessage))
        self.senderId = convergio_message_get_sender(cMessage)
        self.content = String(cString: convergio_message_get_content(cMessage) ?? "")

        let unix = convergio_message_get_timestamp(cMessage)
        self.timestamp = Date(timeIntervalSince1970: TimeInterval(unix))
    }

    /// Create a message for preview/testing
    public init(
        id: UInt64,
        type: MessageType,
        senderId: UInt64,
        content: String,
        timestamp: Date = Date()
    ) {
        self.id = id
        self.type = type
        self.senderId = senderId
        self.content = content
        self.timestamp = timestamp
    }

    /// Whether this is a user message
    public var isFromUser: Bool {
        type.isUserMessage || senderId == 0
    }

    /// Whether this message should be displayed
    public var isVisible: Bool {
        type.isVisible
    }

    /// Formatted timestamp for display
    public var formattedTime: String {
        let formatter = DateFormatter()
        formatter.timeStyle = .short
        return formatter.string(from: timestamp)
    }
}

// MARK: - Message Hashable & Equatable

extension Message: Hashable {
    public static func == (lhs: Message, rhs: Message) -> Bool {
        lhs.id == rhs.id
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(id)
    }
}

// MARK: - Preview Helpers

extension Message {
    /// Create mock messages for SwiftUI previews
    public static let previewMessages: [Message] = [
        Message(
            id: 1,
            type: .userInput,
            senderId: 0,
            content: "How should we approach the EMEA market expansion?",
            timestamp: Date().addingTimeInterval(-300)
        ),
        Message(
            id: 2,
            type: .agentResponse,
            senderId: 1,
            content: """
            Based on my analysis, I recommend a phased approach to EMEA market expansion:

            **Phase 1: Market Research (Q1)**
            - Identify key markets: UK, Germany, France
            - Regulatory compliance assessment
            - Competitive landscape analysis

            **Phase 2: Localization (Q2)**
            - Product localization for top 3 markets
            - Legal entity establishment
            - Local partnership development

            **Phase 3: Launch (Q3)**
            - Soft launch in UK market
            - Performance monitoring and iteration
            - Expand to Germany and France

            Would you like me to involve Amy for financial projections?
            """,
            timestamp: Date().addingTimeInterval(-240)
        ),
        Message(
            id: 3,
            type: .userInput,
            senderId: 0,
            content: "Yes, please get Amy's financial analysis.",
            timestamp: Date().addingTimeInterval(-180)
        ),
        Message(
            id: 4,
            type: .convergence,
            senderId: 1,
            content: """
            ## Converged Analysis: EMEA Market Expansion

            **Contributors:** Ali (Strategy), Angela (Data), Amy (Finance), Matteo (Operations)

            ### Executive Summary
            EMEA expansion is financially viable with an estimated ROI of 180% over 3 years.

            ### Financial Projections (Amy)
            - Initial investment: €2.5M
            - Year 1 revenue: €1.2M
            - Year 2 revenue: €3.8M
            - Year 3 revenue: €7.2M
            - Break-even: Month 18

            ### Market Analysis (Angela)
            - TAM: €450M
            - SAM: €120M
            - SOM: €15M (Year 1)

            ### Operational Plan (Matteo)
            - Team: 12 FTE by EOY
            - Office: Remote-first with London hub
            - Partners: 3 channel partners identified

            **Recommendation:** Proceed with UK pilot in Q2.
            """,
            timestamp: Date().addingTimeInterval(-60)
        ),
    ]

    /// Preview message for SwiftUI
    public static var preview: Message {
        previewMessages[1]
    }
}
