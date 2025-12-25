//
//  FSRSManager.swift
//  ConvergioApp
//
//  FSRS-5 (Free Spaced Repetition Scheduler) Algorithm Implementation
//  Based on: https://github.com/open-spaced-repetition/fsrs4anki/wiki/The-Algorithm
//
//  Created by Roberto Daniele on 2025-12-24.
//

import Foundation

/// FSRS-5 algorithm parameters
struct FSRSParameters {
    /// 17 weights used by the FSRS algorithm
    let w: [Double]

    /// Desired retention rate (0-1), default 0.9 means 90% recall probability
    let requestRetention: Double

    /// Maximum interval in days between reviews
    let maximumInterval: Int

    /// Default FSRS-5 parameters optimized for general use
    static let `default` = FSRSParameters(
        w: [0.4, 0.6, 2.4, 5.8, 4.93, 0.94, 0.86, 0.01, 1.49, 0.14, 0.94, 2.18, 0.05, 0.34, 1.26, 0.29, 2.61],
        requestRetention: 0.9,
        maximumInterval: 36500 // ~100 years
    )

    init(w: [Double], requestRetention: Double, maximumInterval: Int) {
        assert(w.count == 17, "FSRS requires exactly 17 weights")
        assert(requestRetention > 0 && requestRetention <= 1, "Request retention must be between 0 and 1")
        self.w = w
        self.requestRetention = requestRetention
        self.maximumInterval = maximumInterval
    }
}

/// Scheduling information returned by FSRS algorithm
struct SchedulingInfo {
    let card: Flashcard
    let reviewLog: ReviewLog
}

/// Log entry for a card review
struct ReviewLog: Codable {
    let rating: Rating
    let scheduledDays: Int
    let elapsedDays: Int
    let reviewDate: Date
    let state: Flashcard.CardState
}

/// FSRS-5 Spaced Repetition Algorithm Manager
class FSRSManager {
    private let parameters: FSRSParameters
    private let decay: Double = -0.5
    private let factor: Double = 0.9 / Darwin.log(0.9) // ~19/81

    init(parameters: FSRSParameters = .default) {
        self.parameters = parameters
    }

    // MARK: - Public API

    /// Schedule a card review and return updated card with new scheduling
    func review(card: Flashcard, rating: Rating, reviewTime: Date = Date()) -> SchedulingInfo {
        let elapsedDays = calculateElapsedDays(card: card, reviewTime: reviewTime)
        var updatedCard = card

        // Update elapsed days
        updatedCard.elapsedDays = elapsedDays

        // Calculate new state based on rating
        let newState = nextState(currentState: card.state, rating: rating)
        updatedCard.state = newState

        // Update difficulty
        updatedCard.difficulty = nextDifficulty(card: card, rating: rating)

        // Update stability
        updatedCard.stability = nextStability(card: card, rating: rating, elapsedDays: elapsedDays)

        // Calculate new interval
        let interval = nextInterval(stability: updatedCard.stability)
        updatedCard.scheduledDays = interval

        // Update due date
        updatedCard.due = Calendar.current.date(byAdding: .day, value: interval, to: reviewTime) ?? reviewTime

        // Update counters
        updatedCard.reps += 1
        if rating == .again {
            updatedCard.lapses += 1
        }
        updatedCard.lastReview = reviewTime

        let reviewLog = ReviewLog(
            rating: rating,
            scheduledDays: interval,
            elapsedDays: elapsedDays,
            reviewDate: reviewTime,
            state: newState
        )

        return SchedulingInfo(card: updatedCard, reviewLog: reviewLog)
    }

    /// Get all possible scheduling outcomes for a card (for preview)
    func getSchedulingCards(card: Flashcard) -> [Rating: Flashcard] {
        var result: [Rating: Flashcard] = [:]
        for rating in Rating.allCases {
            let info = review(card: card, rating: rating)
            result[rating] = info.card
        }
        return result
    }

    /// Calculate retrievability (probability of recall) for a card
    func retrievability(card: Flashcard, now: Date = Date()) -> Double {
        guard card.state == .review else { return 0 }
        return card.retrievability(now: now)
    }

    // MARK: - Algorithm Core Functions

    /// Calculate initial difficulty for a new card based on first rating
    private func initDifficulty(rating: Rating) -> Double {
        let w = parameters.w
        let difficulty = w[4] - (Double(ratingValue(rating)) - 3.0) * w[5]
        return min(max(difficulty, 1), 10)
    }

    /// Calculate initial stability for a new card based on first rating
    private func initStability(rating: Rating) -> Double {
        let w = parameters.w
        return max(w[ratingValue(rating) - 1], 0.1)
    }

    /// Calculate next difficulty after a review
    private func nextDifficulty(card: Flashcard, rating: Rating) -> Double {
        let w = parameters.w
        let d0 = card.difficulty
        let d = d0 - w[6] * (Double(ratingValue(rating)) - 3.0)
        return constrainDifficulty(meanReversion(w[4], d))
    }

    /// Mean reversion for difficulty
    private func meanReversion(_ initial: Double, _ current: Double) -> Double {
        let w = parameters.w
        return w[7] * initial + (1 - w[7]) * current
    }

    /// Constrain difficulty to valid range [1, 10]
    private func constrainDifficulty(_ difficulty: Double) -> Double {
        return min(max(difficulty, 1), 10)
    }

    /// Calculate next stability after a review
    private func nextStability(card: Flashcard, rating: Rating, elapsedDays: Int) -> Double {
        let w = parameters.w
        let s0 = card.stability
        let d = card.difficulty
        let r = ratingValue(rating)

        if card.state == .new {
            return initStability(rating: rating)
        }

        let retrievability = card.retrievability()
        var newStability: Double

        if rating == .again {
            // Forgetting: use stability decline formula
            newStability = w[11] * pow(d, -w[12]) * (pow(s0 + 1, w[13]) - 1) * exp(w[14] * (1 - retrievability))
        } else {
            // Successful recall: use stability increase formula
            let hardPenalty = rating == .hard ? w[15] : 1.0
            let easyBonus = rating == .easy ? w[16] : 1.0
            newStability = s0 * (1 + exp(w[8]) *
                (11 - d) *
                pow(s0, -w[9]) *
                (exp(w[10] * (1 - retrievability)) - 1) *
                hardPenalty *
                easyBonus)
        }

        return max(newStability, 0.1)
    }

    /// Calculate next interval based on stability
    private func nextInterval(stability: Double) -> Int {
        let interval = stability / self.factor * (pow(parameters.requestRetention, 1.0 / self.decay) - 1)
        return min(max(Int(round(interval)), 1), parameters.maximumInterval)
    }

    /// Determine next card state based on current state and rating
    private func nextState(currentState: Flashcard.CardState, rating: Rating) -> Flashcard.CardState {
        switch currentState {
        case .new:
            return rating == .again ? .learning : .review
        case .learning, .relearning:
            return rating == .again ? .learning : .review
        case .review:
            return rating == .again ? .relearning : .review
        }
    }

    /// Calculate elapsed days since last review
    private func calculateElapsedDays(card: Flashcard, reviewTime: Date) -> Int {
        guard let lastReview = card.lastReview else {
            return 0
        }
        let elapsed = reviewTime.timeIntervalSince(lastReview) / 86400.0
        return max(Int(round(elapsed)), 0)
    }

    /// Convert Rating enum to numeric value (1-4)
    private func ratingValue(_ rating: Rating) -> Int {
        switch rating {
        case .again: return 1
        case .hard: return 2
        case .good: return 3
        case .easy: return 4
        }
    }

    // MARK: - Helper Functions

    /// Natural logarithm helper
    private func ln(_ x: Double) -> Double {
        return log(x)
    }
}

// MARK: - Deck Management Extensions

extension FSRSManager {
    /// Get all cards due for review in a deck
    func getDueCards(deck: FlashcardDeck, limit: Int? = nil) -> [Flashcard] {
        let dueCards = deck.dueCards.sorted { $0.due < $1.due }
        if let limit = limit {
            return Array(dueCards.prefix(limit))
        }
        return dueCards
    }

    /// Get new cards to introduce (never reviewed)
    func getNewCards(deck: FlashcardDeck, limit: Int = 20) -> [Flashcard] {
        return Array(deck.newCards.prefix(limit))
    }

    /// Update deck after a review session
    func updateDeck(deck: inout FlashcardDeck, updatedCard: Flashcard) {
        if let index = deck.cards.firstIndex(where: { $0.id == updatedCard.id }) {
            deck.cards[index] = updatedCard
            deck.lastReviewed = Date()
        }
    }

    /// Calculate optimal daily review limit based on retention goal
    func calculateDailyLimit(deck: FlashcardDeck, targetMinutes: Int = 20, averageSecondsPerCard: Int = 10) -> Int {
        let availableSeconds = targetMinutes * 60
        let limit = availableSeconds / averageSecondsPerCard
        return max(limit, 10) // Minimum 10 cards per day
    }
}
