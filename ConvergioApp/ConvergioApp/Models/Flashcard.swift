//
//  Flashcard.swift
//  ConvergioApp
//
//  Created by Roberto Daniele on 2025-12-24.
//

import Foundation
import SwiftUI

/// Rating for flashcard review based on recall difficulty
enum Rating: String, Codable, CaseIterable {
    case again = "Again"
    case hard = "Hard"
    case good = "Good"
    case easy = "Easy"

    var color: Color {
        switch self {
        case .again: return .red
        case .hard: return .orange
        case .good: return .green
        case .easy: return .blue
        }
    }

    var icon: String {
        switch self {
        case .again: return "xmark.circle.fill"
        case .hard: return "minus.circle.fill"
        case .good: return "checkmark.circle.fill"
        case .easy: return "star.circle.fill"
        }
    }
}

/// Individual flashcard with FSRS-5 scheduling data
struct Flashcard: Identifiable, Codable, Equatable {
    let id: UUID
    var front: String
    var back: String
    var subject: String
    var maestro: String

    // FSRS-5 scheduling parameters
    var due: Date
    var stability: Double
    var difficulty: Double
    var elapsedDays: Int
    var scheduledDays: Int
    var reps: Int
    var lapses: Int
    var state: CardState
    var lastReview: Date?

    enum CardState: String, Codable {
        case new
        case learning
        case review
        case relearning
    }

    init(
        id: UUID = UUID(),
        front: String,
        back: String,
        subject: String,
        maestro: String,
        due: Date = Date(),
        stability: Double = 0,
        difficulty: Double = 0,
        elapsedDays: Int = 0,
        scheduledDays: Int = 0,
        reps: Int = 0,
        lapses: Int = 0,
        state: CardState = .new,
        lastReview: Date? = nil
    ) {
        self.id = id
        self.front = front
        self.back = back
        self.subject = subject
        self.maestro = maestro
        self.due = due
        self.stability = stability
        self.difficulty = difficulty
        self.elapsedDays = elapsedDays
        self.scheduledDays = scheduledDays
        self.reps = reps
        self.lapses = lapses
        self.state = state
        self.lastReview = lastReview
    }

    /// Check if card is due for review
    var isDue: Bool {
        return due <= Date()
    }

    /// Get card's retrievability (probability of recall)
    func retrievability(now: Date = Date()) -> Double {
        if stability <= 0 { return 0 }
        let elapsed = now.timeIntervalSince(due) / 86400.0 // days
        return pow(1 + elapsed / (9 * stability), -1)
    }
}

/// Collection of flashcards for a specific topic
struct FlashcardDeck: Identifiable, Codable {
    let id: UUID
    var name: String
    var subject: String
    var maestro: String
    var cards: [Flashcard]
    var createdAt: Date
    var lastReviewed: Date?

    init(
        id: UUID = UUID(),
        name: String,
        subject: String,
        maestro: String,
        cards: [Flashcard] = [],
        createdAt: Date = Date(),
        lastReviewed: Date? = nil
    ) {
        self.id = id
        self.name = name
        self.subject = subject
        self.maestro = maestro
        self.cards = cards
        self.createdAt = createdAt
        self.lastReviewed = lastReviewed
    }

    /// Get cards that are due for review
    var dueCards: [Flashcard] {
        cards.filter { $0.isDue }
    }

    /// Get cards that are new (never reviewed)
    var newCards: [Flashcard] {
        cards.filter { $0.state == .new }
    }

    /// Get cards currently in learning phase
    var learningCards: [Flashcard] {
        cards.filter { $0.state == .learning || $0.state == .relearning }
    }

    /// Get cards in review phase
    var reviewCards: [Flashcard] {
        cards.filter { $0.state == .review }
    }

    /// Total number of cards
    var totalCards: Int {
        cards.count
    }

    /// Progress percentage (0-100)
    var progress: Double {
        guard totalCards > 0 else { return 0 }
        let reviewedCards = cards.filter { $0.reps > 0 }.count
        return Double(reviewedCards) / Double(totalCards) * 100
    }
}

/// Session statistics for tracking review performance
struct ReviewSession: Identifiable {
    let id: UUID
    var deckId: UUID
    var startTime: Date
    var endTime: Date?
    var cardsReviewed: Int
    var againCount: Int
    var hardCount: Int
    var goodCount: Int
    var easyCount: Int

    init(
        id: UUID = UUID(),
        deckId: UUID,
        startTime: Date = Date(),
        endTime: Date? = nil,
        cardsReviewed: Int = 0,
        againCount: Int = 0,
        hardCount: Int = 0,
        goodCount: Int = 0,
        easyCount: Int = 0
    ) {
        self.id = id
        self.deckId = deckId
        self.startTime = startTime
        self.endTime = endTime
        self.cardsReviewed = cardsReviewed
        self.againCount = againCount
        self.hardCount = hardCount
        self.goodCount = goodCount
        self.easyCount = easyCount
    }

    /// Duration of the session in seconds
    var duration: TimeInterval {
        guard let end = endTime else {
            return Date().timeIntervalSince(startTime)
        }
        return end.timeIntervalSince(startTime)
    }

    /// Average time per card in seconds
    var averageTimePerCard: TimeInterval {
        guard cardsReviewed > 0 else { return 0 }
        return duration / Double(cardsReviewed)
    }

    /// Success rate (good + easy) / total
    var successRate: Double {
        guard cardsReviewed > 0 else { return 0 }
        return Double(goodCount + easyCount) / Double(cardsReviewed)
    }
}
