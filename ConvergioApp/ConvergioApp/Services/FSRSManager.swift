/**
 * CONVERGIO NATIVE - FSRS Manager
 *
 * Implementation of the Free Spaced Repetition Scheduler (FSRS) algorithm
 * for scientific, evidence-based learning with flashcards.
 *
 * Based on: https://github.com/open-spaced-repetition/fsrs4anki
 *
 * Part of the Scuola 2026 Education Core (Tasks 1.3.1 - 1.3.6)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import Combine
import SQLite3

// MARK: - FSRS Algorithm Parameters

struct FSRSParameters {
    // Default FSRS-4.5 weights
    static let defaultWeights: [Double] = [
        0.4, 0.6, 2.4, 5.8, 4.93, 0.94, 0.86, 0.01,
        1.49, 0.14, 0.94, 2.18, 0.05, 0.34, 1.26,
        0.29, 2.61
    ]

    let w: [Double]
    let requestRetention: Double
    let maximumInterval: Int
    let enableFuzz: Bool

    init(
        w: [Double] = FSRSParameters.defaultWeights,
        requestRetention: Double = 0.9,
        maximumInterval: Int = 36500,
        enableFuzz: Bool = true
    ) {
        self.w = w
        self.requestRetention = requestRetention
        self.maximumInterval = maximumInterval
        self.enableFuzz = enableFuzz
    }

    // Decay constant from FSRS paper
    static let decay: Double = -0.5
    static let factor: Double = 19.0 / 81.0
}

// MARK: - Rating

enum FSRSRating: Int, CaseIterable, Codable {
    case again = 1
    case hard = 2
    case good = 3
    case easy = 4

    var displayName: String {
        switch self {
        case .again: return "Again"
        case .hard: return "Hard"
        case .good: return "Good"
        case .easy: return "Easy"
        }
    }

    var emoji: String {
        switch self {
        case .again: return "🔄"
        case .hard: return "😓"
        case .good: return "👍"
        case .easy: return "🌟"
        }
    }

    var color: String {
        switch self {
        case .again: return "red"
        case .hard: return "orange"
        case .good: return "green"
        case .easy: return "blue"
        }
    }
}

// MARK: - Card State

enum CardState: Int, Codable {
    case new = 0
    case learning = 1
    case review = 2
    case relearning = 3
}

// MARK: - Flashcard

struct Flashcard: Identifiable, Codable, Equatable {
    let id: UUID
    var front: String
    var back: String
    var subject: String
    var maestroId: String?
    var tags: [String]

    // FSRS state
    var state: CardState
    var stability: Double
    var difficulty: Double
    var elapsedDays: Int
    var scheduledDays: Int
    var reps: Int
    var lapses: Int
    var due: Date
    var lastReview: Date?

    // Metadata
    let createdAt: Date
    var updatedAt: Date

    init(
        id: UUID = UUID(),
        front: String,
        back: String,
        subject: String,
        maestroId: String? = nil,
        tags: [String] = []
    ) {
        self.id = id
        self.front = front
        self.back = back
        self.subject = subject
        self.maestroId = maestroId
        self.tags = tags

        // Initial FSRS state
        self.state = .new
        self.stability = 0
        self.difficulty = 0
        self.elapsedDays = 0
        self.scheduledDays = 0
        self.reps = 0
        self.lapses = 0
        self.due = Date()
        self.lastReview = nil

        self.createdAt = Date()
        self.updatedAt = Date()
    }

    var isDue: Bool {
        due <= Date()
    }

    var retrievability: Double {
        guard stability > 0, let lastReview = lastReview else { return 0 }
        let elapsed = Date().timeIntervalSince(lastReview) / 86400 // days
        return pow(1 + FSRSParameters.factor * elapsed / stability, FSRSParameters.decay)
    }
}

// MARK: - Review Log

struct ReviewLog: Identifiable, Codable {
    let id: UUID
    let cardId: UUID
    let rating: FSRSRating
    let state: CardState
    let due: Date
    let stability: Double
    let difficulty: Double
    let elapsedDays: Int
    let scheduledDays: Int
    let review: Date
    let duration: TimeInterval // milliseconds

    init(
        cardId: UUID,
        rating: FSRSRating,
        state: CardState,
        due: Date,
        stability: Double,
        difficulty: Double,
        elapsedDays: Int,
        scheduledDays: Int,
        duration: TimeInterval
    ) {
        self.id = UUID()
        self.cardId = cardId
        self.rating = rating
        self.state = state
        self.due = due
        self.stability = stability
        self.difficulty = difficulty
        self.elapsedDays = elapsedDays
        self.scheduledDays = scheduledDays
        self.review = Date()
        self.duration = duration
    }
}

// MARK: - Scheduling Info

struct SchedulingInfo {
    var again: Flashcard
    var hard: Flashcard
    var good: Flashcard
    var easy: Flashcard

    func card(for rating: FSRSRating) -> Flashcard {
        switch rating {
        case .again: return again
        case .hard: return hard
        case .good: return good
        case .easy: return easy
        }
    }
}

// MARK: - FSRS Algorithm

final class FSRSAlgorithm {
    let params: FSRSParameters

    init(params: FSRSParameters = FSRSParameters()) {
        self.params = params
    }

    // MARK: - Core Scheduling

    func scheduleReview(card: Flashcard, now: Date) -> SchedulingInfo {
        var again = card
        var hard = card
        var good = card
        var easy = card

        // Update elapsed days
        if let lastReview = card.lastReview {
            let elapsed = Int(now.timeIntervalSince(lastReview) / 86400)
            again.elapsedDays = elapsed
            hard.elapsedDays = elapsed
            good.elapsedDays = elapsed
            easy.elapsedDays = elapsed
        }

        // Set review date
        again.lastReview = now
        hard.lastReview = now
        good.lastReview = now
        easy.lastReview = now

        switch card.state {
        case .new:
            scheduleNew(again: &again, hard: &hard, good: &good, easy: &easy, now: now)

        case .learning, .relearning:
            scheduleLearning(
                again: &again, hard: &hard, good: &good, easy: &easy,
                now: now, isRelearning: card.state == .relearning
            )

        case .review:
            scheduleReview(again: &again, hard: &hard, good: &good, easy: &easy, now: now)
        }

        return SchedulingInfo(again: again, hard: hard, good: good, easy: easy)
    }

    private func scheduleNew(again: inout Flashcard, hard: inout Flashcard, good: inout Flashcard, easy: inout Flashcard, now: Date) {
        // Initialize difficulty
        let d0 = initDifficulty(rating: .good)
        again.difficulty = d0
        hard.difficulty = d0
        good.difficulty = d0
        easy.difficulty = d0

        // Initialize stability
        again.stability = initStability(rating: .again)
        hard.stability = initStability(rating: .hard)
        good.stability = initStability(rating: .good)
        easy.stability = initStability(rating: .easy)

        // Schedule
        again.state = .learning
        again.scheduledDays = 0
        again.due = now.addingTimeInterval(60) // 1 minute

        hard.state = .learning
        hard.scheduledDays = 0
        hard.due = now.addingTimeInterval(5 * 60) // 5 minutes

        good.state = .learning
        good.scheduledDays = 0
        good.due = now.addingTimeInterval(10 * 60) // 10 minutes

        let easyInterval = nextInterval(stability: easy.stability)
        easy.state = .review
        easy.scheduledDays = easyInterval
        easy.due = now.addingTimeInterval(Double(easyInterval) * 86400)

        // Update reps
        again.reps += 1
        hard.reps += 1
        good.reps += 1
        easy.reps += 1
    }

    private func scheduleLearning(again: inout Flashcard, hard: inout Flashcard, good: inout Flashcard, easy: inout Flashcard, now: Date, isRelearning: Bool) {
        // Again
        again.state = isRelearning ? .relearning : .learning
        again.scheduledDays = 0
        again.due = now.addingTimeInterval(5 * 60)
        if isRelearning {
            again.lapses += 1
        }

        // Hard
        hard.state = isRelearning ? .relearning : .learning
        hard.scheduledDays = 0
        hard.due = now.addingTimeInterval(10 * 60)

        // Good -> Graduate to review
        let goodInterval = nextInterval(stability: good.stability)
        good.state = .review
        good.scheduledDays = goodInterval
        good.due = now.addingTimeInterval(Double(goodInterval) * 86400)

        // Easy -> Graduate with bonus
        let easyInterval = max(goodInterval + 1, nextInterval(stability: easy.stability * 1.3))
        easy.state = .review
        easy.scheduledDays = easyInterval
        easy.due = now.addingTimeInterval(Double(easyInterval) * 86400)

        // Update reps
        again.reps += 1
        hard.reps += 1
        good.reps += 1
        easy.reps += 1
    }

    private func scheduleReview(again: inout Flashcard, hard: inout Flashcard, good: inout Flashcard, easy: inout Flashcard, now: Date) {
        let elapsedDays = again.elapsedDays
        let lastD = again.difficulty
        let lastS = again.stability
        let retrievability = pow(1 + FSRSParameters.factor * Double(elapsedDays) / lastS, FSRSParameters.decay)

        // Update difficulty
        again.difficulty = nextDifficulty(d: lastD, rating: .again)
        hard.difficulty = nextDifficulty(d: lastD, rating: .hard)
        good.difficulty = nextDifficulty(d: lastD, rating: .good)
        easy.difficulty = nextDifficulty(d: lastD, rating: .easy)

        // Update stability
        again.stability = nextForgetStability(d: again.difficulty, s: lastS, r: retrievability)
        hard.stability = nextRecallStability(d: hard.difficulty, s: lastS, r: retrievability, rating: .hard)
        good.stability = nextRecallStability(d: good.difficulty, s: lastS, r: retrievability, rating: .good)
        easy.stability = nextRecallStability(d: easy.difficulty, s: lastS, r: retrievability, rating: .easy)

        // Schedule
        again.state = .relearning
        again.scheduledDays = 0
        again.due = now.addingTimeInterval(5 * 60)
        again.lapses += 1

        let hardInterval = nextInterval(stability: hard.stability)
        hard.state = .review
        hard.scheduledDays = hardInterval
        hard.due = now.addingTimeInterval(Double(hardInterval) * 86400)

        let goodInterval = nextInterval(stability: good.stability)
        good.state = .review
        good.scheduledDays = goodInterval
        good.due = now.addingTimeInterval(Double(goodInterval) * 86400)

        let easyInterval = max(goodInterval + 1, nextInterval(stability: easy.stability))
        easy.state = .review
        easy.scheduledDays = easyInterval
        easy.due = now.addingTimeInterval(Double(easyInterval) * 86400)

        // Update reps
        again.reps += 1
        hard.reps += 1
        good.reps += 1
        easy.reps += 1
    }

    // MARK: - FSRS Formulas

    private func initStability(rating: FSRSRating) -> Double {
        params.w[rating.rawValue - 1]
    }

    private func initDifficulty(rating: FSRSRating) -> Double {
        let d = params.w[4] - (Double(rating.rawValue) - 3) * params.w[5]
        return min(max(d, 1), 10)
    }

    private func nextDifficulty(d: Double, rating: FSRSRating) -> Double {
        let delta = -(params.w[6] * (Double(rating.rawValue) - 3))
        let newD = d + delta * ((10 - d) / 9)
        return min(max(meanReversion(init: initDifficulty(rating: .easy), current: newD), 1), 10)
    }

    private func nextRecallStability(d: Double, s: Double, r: Double, rating: FSRSRating) -> Double {
        let hardPenalty = rating == .hard ? params.w[15] : 1
        let easyBonus = rating == .easy ? params.w[16] : 1
        return s * (1 + exp(params.w[8]) * (11 - d) * pow(s, -params.w[9]) * (exp((1 - r) * params.w[10]) - 1) * hardPenalty * easyBonus)
    }

    private func nextForgetStability(d: Double, s: Double, r: Double) -> Double {
        return params.w[11] * pow(d, -params.w[12]) * (pow(s + 1, params.w[13]) - 1) * exp((1 - r) * params.w[14])
    }

    private func nextInterval(stability: Double) -> Int {
        let interval = stability / FSRSParameters.factor * (pow(params.requestRetention, 1 / FSRSParameters.decay) - 1)
        var result = Int(round(interval))

        if params.enableFuzz && result >= 3 {
            let fuzz = Double.random(in: -0.05...0.05) * Double(result)
            result += Int(fuzz)
        }

        return min(max(result, 1), params.maximumInterval)
    }

    private func meanReversion(init: Double, current: Double) -> Double {
        return params.w[7] * `init` + (1 - params.w[7]) * current
    }
}

// MARK: - FSRS Manager

@MainActor
final class FSRSManager: ObservableObject {
    // MARK: - Published State

    @Published private(set) var decks: [FlashcardDeck] = []
    @Published private(set) var dueCards: [Flashcard] = []
    @Published private(set) var todayReviews: Int = 0
    @Published private(set) var streak: Int = 0

    // MARK: - Properties

    private let algorithm: FSRSAlgorithm
    private var db: OpaquePointer?
    private let dbPath: URL

    // MARK: - Singleton

    static let shared = FSRSManager()

    // MARK: - Initialization

    private init() {
        self.algorithm = FSRSAlgorithm()

        // Database path
        let appSupport = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first!
        let convergio = appSupport.appendingPathComponent("Convergio", isDirectory: true)
        try? FileManager.default.createDirectory(at: convergio, withIntermediateDirectories: true)
        self.dbPath = convergio.appendingPathComponent("flashcards.db")

        setupDatabase()
        loadDecks()
        updateDueCards()
    }

    deinit {
        sqlite3_close(db)
    }

    // MARK: - Database Setup

    private func setupDatabase() {
        guard sqlite3_open(dbPath.path, &db) == SQLITE_OK else {
            print("FSRSManager: Failed to open database")
            return
        }

        let createTables = """
        CREATE TABLE IF NOT EXISTS decks (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            subject TEXT NOT NULL,
            maestro_id TEXT,
            created_at REAL NOT NULL,
            updated_at REAL NOT NULL
        );

        CREATE TABLE IF NOT EXISTS cards (
            id TEXT PRIMARY KEY,
            deck_id TEXT NOT NULL,
            front TEXT NOT NULL,
            back TEXT NOT NULL,
            subject TEXT NOT NULL,
            maestro_id TEXT,
            tags TEXT,
            state INTEGER NOT NULL DEFAULT 0,
            stability REAL NOT NULL DEFAULT 0,
            difficulty REAL NOT NULL DEFAULT 0,
            elapsed_days INTEGER NOT NULL DEFAULT 0,
            scheduled_days INTEGER NOT NULL DEFAULT 0,
            reps INTEGER NOT NULL DEFAULT 0,
            lapses INTEGER NOT NULL DEFAULT 0,
            due REAL NOT NULL,
            last_review REAL,
            created_at REAL NOT NULL,
            updated_at REAL NOT NULL,
            FOREIGN KEY (deck_id) REFERENCES decks(id)
        );

        CREATE TABLE IF NOT EXISTS review_logs (
            id TEXT PRIMARY KEY,
            card_id TEXT NOT NULL,
            rating INTEGER NOT NULL,
            state INTEGER NOT NULL,
            due REAL NOT NULL,
            stability REAL NOT NULL,
            difficulty REAL NOT NULL,
            elapsed_days INTEGER NOT NULL,
            scheduled_days INTEGER NOT NULL,
            review REAL NOT NULL,
            duration REAL NOT NULL,
            FOREIGN KEY (card_id) REFERENCES cards(id)
        );

        CREATE INDEX IF NOT EXISTS idx_cards_due ON cards(due);
        CREATE INDEX IF NOT EXISTS idx_cards_deck ON cards(deck_id);
        """

        var errorMessage: UnsafeMutablePointer<CChar>?
        if sqlite3_exec(db, createTables, nil, nil, &errorMessage) != SQLITE_OK {
            print("FSRSManager: Failed to create tables: \(String(cString: errorMessage!))")
            sqlite3_free(errorMessage)
        }
    }

    // MARK: - Public API

    func createDeck(name: String, subject: String, maestroId: String? = nil) -> FlashcardDeck {
        let deck = FlashcardDeck(name: name, subject: subject, maestroId: maestroId)
        saveDeck(deck)
        decks.append(deck)
        return deck
    }

    func addCard(to deck: FlashcardDeck, front: String, back: String, tags: [String] = []) -> Flashcard {
        let card = Flashcard(
            front: front,
            back: back,
            subject: deck.subject,
            maestroId: deck.maestroId,
            tags: tags
        )
        saveCard(card, deckId: deck.id)
        updateDueCards()
        return card
    }

    func reviewCard(_ card: Flashcard, rating: FSRSRating, duration: TimeInterval) -> Flashcard {
        let scheduling = algorithm.scheduleReview(card: card, now: Date())
        let updatedCard = scheduling.card(for: rating)

        // Save review log
        let log = ReviewLog(
            cardId: card.id,
            rating: rating,
            state: card.state,
            due: card.due,
            stability: card.stability,
            difficulty: card.difficulty,
            elapsedDays: card.elapsedDays,
            scheduledDays: card.scheduledDays,
            duration: duration
        )
        saveReviewLog(log)

        // Update card
        updateCard(updatedCard)

        // Update stats
        todayReviews += 1
        updateDueCards()

        return updatedCard
    }

    func getSchedulingPreview(for card: Flashcard) -> SchedulingInfo {
        algorithm.scheduleReview(card: card, now: Date())
    }

    func getDueCards(for deck: FlashcardDeck? = nil, limit: Int = 20) -> [Flashcard] {
        // Return cards that are due for review
        var cards = dueCards
        if let deck = deck {
            cards = cards.filter { $0.subject == deck.subject }
        }
        return Array(cards.prefix(limit))
    }

    // MARK: - Private Database Operations

    private func loadDecks() {
        guard let db = db else { return }

        let query = "SELECT id, name, subject, maestro_id, created_at, updated_at FROM decks ORDER BY name"
        var stmt: OpaquePointer?

        guard sqlite3_prepare_v2(db, query, -1, &stmt, nil) == SQLITE_OK else { return }
        defer { sqlite3_finalize(stmt) }

        var loadedDecks: [FlashcardDeck] = []
        while sqlite3_step(stmt) == SQLITE_ROW {
            let id = UUID(uuidString: String(cString: sqlite3_column_text(stmt, 0))) ?? UUID()
            let name = String(cString: sqlite3_column_text(stmt, 1))
            let subject = String(cString: sqlite3_column_text(stmt, 2))
            let maestroId = sqlite3_column_text(stmt, 3).map { String(cString: $0) }

            loadedDecks.append(FlashcardDeck(id: id, name: name, subject: subject, maestroId: maestroId))
        }

        decks = loadedDecks
    }

    private func saveDeck(_ deck: FlashcardDeck) {
        guard let db = db else { return }

        let query = """
        INSERT OR REPLACE INTO decks (id, name, subject, maestro_id, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?)
        """
        var stmt: OpaquePointer?

        guard sqlite3_prepare_v2(db, query, -1, &stmt, nil) == SQLITE_OK else { return }
        defer { sqlite3_finalize(stmt) }

        sqlite3_bind_text(stmt, 1, deck.id.uuidString, -1, nil)
        sqlite3_bind_text(stmt, 2, deck.name, -1, nil)
        sqlite3_bind_text(stmt, 3, deck.subject, -1, nil)
        if let maestroId = deck.maestroId {
            sqlite3_bind_text(stmt, 4, maestroId, -1, nil)
        } else {
            sqlite3_bind_null(stmt, 4)
        }
        sqlite3_bind_double(stmt, 5, deck.createdAt.timeIntervalSince1970)
        sqlite3_bind_double(stmt, 6, Date().timeIntervalSince1970)

        sqlite3_step(stmt)
    }

    private func saveCard(_ card: Flashcard, deckId: UUID) {
        guard let db = db else { return }

        let query = """
        INSERT OR REPLACE INTO cards
        (id, deck_id, front, back, subject, maestro_id, tags, state, stability, difficulty,
         elapsed_days, scheduled_days, reps, lapses, due, last_review, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        var stmt: OpaquePointer?

        guard sqlite3_prepare_v2(db, query, -1, &stmt, nil) == SQLITE_OK else { return }
        defer { sqlite3_finalize(stmt) }

        sqlite3_bind_text(stmt, 1, card.id.uuidString, -1, nil)
        sqlite3_bind_text(stmt, 2, deckId.uuidString, -1, nil)
        sqlite3_bind_text(stmt, 3, card.front, -1, nil)
        sqlite3_bind_text(stmt, 4, card.back, -1, nil)
        sqlite3_bind_text(stmt, 5, card.subject, -1, nil)
        if let maestroId = card.maestroId {
            sqlite3_bind_text(stmt, 6, maestroId, -1, nil)
        } else {
            sqlite3_bind_null(stmt, 6)
        }
        sqlite3_bind_text(stmt, 7, card.tags.joined(separator: ","), -1, nil)
        sqlite3_bind_int(stmt, 8, Int32(card.state.rawValue))
        sqlite3_bind_double(stmt, 9, card.stability)
        sqlite3_bind_double(stmt, 10, card.difficulty)
        sqlite3_bind_int(stmt, 11, Int32(card.elapsedDays))
        sqlite3_bind_int(stmt, 12, Int32(card.scheduledDays))
        sqlite3_bind_int(stmt, 13, Int32(card.reps))
        sqlite3_bind_int(stmt, 14, Int32(card.lapses))
        sqlite3_bind_double(stmt, 15, card.due.timeIntervalSince1970)
        if let lastReview = card.lastReview {
            sqlite3_bind_double(stmt, 16, lastReview.timeIntervalSince1970)
        } else {
            sqlite3_bind_null(stmt, 16)
        }
        sqlite3_bind_double(stmt, 17, card.createdAt.timeIntervalSince1970)
        sqlite3_bind_double(stmt, 18, Date().timeIntervalSince1970)

        sqlite3_step(stmt)
    }

    private func updateCard(_ card: Flashcard) {
        guard let db = db else { return }

        let query = """
        UPDATE cards SET
            state = ?, stability = ?, difficulty = ?, elapsed_days = ?,
            scheduled_days = ?, reps = ?, lapses = ?, due = ?, last_review = ?, updated_at = ?
        WHERE id = ?
        """
        var stmt: OpaquePointer?

        guard sqlite3_prepare_v2(db, query, -1, &stmt, nil) == SQLITE_OK else { return }
        defer { sqlite3_finalize(stmt) }

        sqlite3_bind_int(stmt, 1, Int32(card.state.rawValue))
        sqlite3_bind_double(stmt, 2, card.stability)
        sqlite3_bind_double(stmt, 3, card.difficulty)
        sqlite3_bind_int(stmt, 4, Int32(card.elapsedDays))
        sqlite3_bind_int(stmt, 5, Int32(card.scheduledDays))
        sqlite3_bind_int(stmt, 6, Int32(card.reps))
        sqlite3_bind_int(stmt, 7, Int32(card.lapses))
        sqlite3_bind_double(stmt, 8, card.due.timeIntervalSince1970)
        if let lastReview = card.lastReview {
            sqlite3_bind_double(stmt, 9, lastReview.timeIntervalSince1970)
        } else {
            sqlite3_bind_null(stmt, 9)
        }
        sqlite3_bind_double(stmt, 10, Date().timeIntervalSince1970)
        sqlite3_bind_text(stmt, 11, card.id.uuidString, -1, nil)

        sqlite3_step(stmt)
    }

    private func saveReviewLog(_ log: ReviewLog) {
        guard let db = db else { return }

        let query = """
        INSERT INTO review_logs
        (id, card_id, rating, state, due, stability, difficulty, elapsed_days, scheduled_days, review, duration)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        var stmt: OpaquePointer?

        guard sqlite3_prepare_v2(db, query, -1, &stmt, nil) == SQLITE_OK else { return }
        defer { sqlite3_finalize(stmt) }

        sqlite3_bind_text(stmt, 1, log.id.uuidString, -1, nil)
        sqlite3_bind_text(stmt, 2, log.cardId.uuidString, -1, nil)
        sqlite3_bind_int(stmt, 3, Int32(log.rating.rawValue))
        sqlite3_bind_int(stmt, 4, Int32(log.state.rawValue))
        sqlite3_bind_double(stmt, 5, log.due.timeIntervalSince1970)
        sqlite3_bind_double(stmt, 6, log.stability)
        sqlite3_bind_double(stmt, 7, log.difficulty)
        sqlite3_bind_int(stmt, 8, Int32(log.elapsedDays))
        sqlite3_bind_int(stmt, 9, Int32(log.scheduledDays))
        sqlite3_bind_double(stmt, 10, log.review.timeIntervalSince1970)
        sqlite3_bind_double(stmt, 11, log.duration)

        sqlite3_step(stmt)
    }

    private func updateDueCards() {
        guard let db = db else { return }

        let query = "SELECT * FROM cards WHERE due <= ? ORDER BY due LIMIT 100"
        var stmt: OpaquePointer?

        guard sqlite3_prepare_v2(db, query, -1, &stmt, nil) == SQLITE_OK else { return }
        defer { sqlite3_finalize(stmt) }

        sqlite3_bind_double(stmt, 1, Date().timeIntervalSince1970)

        var cards: [Flashcard] = []
        while sqlite3_step(stmt) == SQLITE_ROW {
            if let card = parseCard(from: stmt) {
                cards.append(card)
            }
        }

        dueCards = cards
    }

    private func parseCard(from stmt: OpaquePointer?) -> Flashcard? {
        guard let stmt = stmt else { return nil }

        guard let idString = sqlite3_column_text(stmt, 0),
              let id = UUID(uuidString: String(cString: idString)) else { return nil }

        let front = String(cString: sqlite3_column_text(stmt, 2))
        let back = String(cString: sqlite3_column_text(stmt, 3))
        let subject = String(cString: sqlite3_column_text(stmt, 4))
        let maestroId = sqlite3_column_text(stmt, 5).map { String(cString: $0) }
        let tagsString = String(cString: sqlite3_column_text(stmt, 6))
        let tags = tagsString.isEmpty ? [] : tagsString.components(separatedBy: ",")

        var card = Flashcard(id: id, front: front, back: back, subject: subject, maestroId: maestroId, tags: tags)
        card.state = CardState(rawValue: Int(sqlite3_column_int(stmt, 7))) ?? .new
        card.stability = sqlite3_column_double(stmt, 8)
        card.difficulty = sqlite3_column_double(stmt, 9)
        card.elapsedDays = Int(sqlite3_column_int(stmt, 10))
        card.scheduledDays = Int(sqlite3_column_int(stmt, 11))
        card.reps = Int(sqlite3_column_int(stmt, 12))
        card.lapses = Int(sqlite3_column_int(stmt, 13))
        card.due = Date(timeIntervalSince1970: sqlite3_column_double(stmt, 14))

        if sqlite3_column_type(stmt, 15) != SQLITE_NULL {
            card.lastReview = Date(timeIntervalSince1970: sqlite3_column_double(stmt, 15))
        }

        return card
    }
}

// MARK: - Flashcard Deck

struct FlashcardDeck: Identifiable, Codable, Hashable {
    let id: UUID
    var name: String
    var subject: String
    var maestroId: String?
    let createdAt: Date

    init(id: UUID = UUID(), name: String, subject: String, maestroId: String? = nil) {
        self.id = id
        self.name = name
        self.subject = subject
        self.maestroId = maestroId
        self.createdAt = Date()
    }
}

// MARK: - Previews

#if DEBUG
extension FSRSManager {
    static var preview: FSRSManager {
        let manager = FSRSManager.shared
        return manager
    }
}
#endif
