/**
 * CONVERGIO NATIVE - Quiz Model
 *
 * Models for quiz system with maieutic feedback.
 * Supports multiple question types and mastery-based progression.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import SwiftUI

// MARK: - Question Type

enum QuestionType: String, Codable, CaseIterable {
    case multipleChoice = "multiple_choice"
    case openEnded = "open_ended"
    case trueFalse = "true_false"

    var displayName: String {
        switch self {
        case .multipleChoice: return "Scelta multipla"
        case .openEnded: return "Risposta aperta"
        case .trueFalse: return "Vero/Falso"
        }
    }

    var icon: String {
        switch self {
        case .multipleChoice: return "list.bullet.circle"
        case .openEnded: return "text.bubble"
        case .trueFalse: return "checkmark.circle.badge.xmark"
        }
    }
}

// MARK: - Question Model

struct Question: Identifiable, Codable, Hashable {
    let id: UUID
    let text: String
    let type: QuestionType
    let options: [String]? // For multiple choice and true/false
    let correctAnswer: String
    let explanation: String // Explanation shown after answering
    let hint: String? // Maieutic hint to guide thinking
    let difficulty: Int // 1-5 scale

    init(
        text: String,
        type: QuestionType,
        options: [String]? = nil,
        correctAnswer: String,
        explanation: String,
        hint: String? = nil,
        difficulty: Int = 3
    ) {
        self.id = UUID()
        self.text = text
        self.type = type
        self.options = options
        self.correctAnswer = correctAnswer
        self.explanation = explanation
        self.hint = hint
        self.difficulty = difficulty
    }

    // Check if answer is correct (case-insensitive for open-ended)
    func isCorrect(_ answer: String) -> Bool {
        switch type {
        case .openEnded:
            // For open-ended, accept variations (this can be enhanced with AI)
            return answer.lowercased().trimmingCharacters(in: .whitespacesAndNewlines)
                .contains(correctAnswer.lowercased())
        case .multipleChoice, .trueFalse:
            return answer.trimmingCharacters(in: .whitespacesAndNewlines)
                .lowercased() == correctAnswer.lowercased()
        }
    }
}

// MARK: - Student Answer

struct StudentAnswer: Identifiable, Codable, Hashable {
    let id: UUID
    let questionId: UUID
    let answer: String
    let isCorrect: Bool
    let timestamp: Date
    let attemptsCount: Int

    init(questionId: UUID, answer: String, isCorrect: Bool, attemptsCount: Int = 1) {
        self.id = UUID()
        self.questionId = questionId
        self.answer = answer
        self.isCorrect = isCorrect
        self.timestamp = Date()
        self.attemptsCount = attemptsCount
    }
}

// MARK: - Quiz Result

struct QuizResult: Identifiable, Codable {
    let id: UUID
    let quizId: UUID
    let score: Int // Number of correct answers
    let totalQuestions: Int
    let answers: [StudentAnswer]
    let xpEarned: Int
    let masteryAchieved: Bool // True if >= 80%
    let completionTime: TimeInterval // Seconds taken
    let timestamp: Date

    init(
        quizId: UUID,
        score: Int,
        totalQuestions: Int,
        answers: [StudentAnswer],
        xpEarned: Int,
        completionTime: TimeInterval
    ) {
        self.id = UUID()
        self.quizId = quizId
        self.score = score
        self.totalQuestions = totalQuestions
        self.answers = answers
        self.xpEarned = xpEarned
        self.masteryAchieved = (Double(score) / Double(totalQuestions)) >= 0.8
        self.completionTime = completionTime
        self.timestamp = Date()
    }

    var percentage: Double {
        guard totalQuestions > 0 else { return 0 }
        return (Double(score) / Double(totalQuestions)) * 100
    }

    var grade: String {
        switch percentage {
        case 90...100: return "Eccellente!"
        case 80..<90: return "Molto Bene!"
        case 70..<80: return "Buon lavoro!"
        case 60..<70: return "Discreto"
        default: return "Continua a studiare!"
        }
    }

    var gradeColor: Color {
        switch percentage {
        case 90...100: return .green
        case 80..<90: return .blue
        case 70..<80: return .cyan
        case 60..<70: return .orange
        default: return .red
        }
    }
}

// MARK: - Quiz Model

struct Quiz: Identifiable, Codable, Hashable {
    let id: UUID
    let title: String
    let subject: Subject
    let maestro: String // Maestro name
    let description: String
    let questions: [Question]
    let timeLimit: TimeInterval? // Optional time limit in seconds
    let requiredScore: Double // 0.0-1.0, default 0.8 for mastery
    let xpReward: Int // XP earned for completion
    let difficulty: Int // 1-5 overall difficulty
    let estimatedMinutes: Int

    init(
        title: String,
        subject: Subject,
        maestro: String,
        description: String,
        questions: [Question],
        timeLimit: TimeInterval? = nil,
        requiredScore: Double = 0.8,
        xpReward: Int = 100,
        difficulty: Int = 3,
        estimatedMinutes: Int = 10
    ) {
        self.id = UUID()
        self.title = title
        self.subject = subject
        self.maestro = maestro
        self.description = description
        self.questions = questions
        self.timeLimit = timeLimit
        self.requiredScore = requiredScore
        self.xpReward = xpReward
        self.difficulty = difficulty
        self.estimatedMinutes = estimatedMinutes
    }

    var difficultyStars: String {
        String(repeating: "★", count: difficulty) + String(repeating: "☆", count: 5 - difficulty)
    }
}

// MARK: - Preview Data

extension Quiz {
    static let preview = Quiz(
        title: "Il Teorema di Pitagora",
        subject: .matematica,
        maestro: "Pitagora",
        description: "Verifica la tua comprensione del celebre teorema sui triangoli rettangoli.",
        questions: [
            Question(
                text: "In un triangolo rettangolo, come si relazionano l'ipotenusa e i cateti?",
                type: .multipleChoice,
                options: [
                    "L'ipotenusa al quadrato è uguale alla somma dei quadrati dei cateti",
                    "L'ipotenusa è uguale alla somma dei cateti",
                    "L'ipotenusa è sempre più corta dei cateti",
                    "Non c'è relazione"
                ],
                correctAnswer: "L'ipotenusa al quadrato è uguale alla somma dei quadrati dei cateti",
                explanation: "Il Teorema di Pitagora afferma che a² + b² = c², dove c è l'ipotenusa e a, b sono i cateti.",
                hint: "Pensa alla relazione matematica tra i lati, non alla loro semplice somma.",
                difficulty: 2
            ),
            Question(
                text: "Se un triangolo ha cateti di 3 e 4 unità, quanto misura l'ipotenusa?",
                type: .openEnded,
                correctAnswer: "5",
                explanation: "Usando il teorema: 3² + 4² = 9 + 16 = 25, quindi c = √25 = 5",
                hint: "Applica la formula a² + b² = c². Ricorda di calcolare la radice quadrata!",
                difficulty: 3
            ),
            Question(
                text: "Il Teorema di Pitagora vale solo per i triangoli rettangoli?",
                type: .trueFalse,
                options: ["Vero", "Falso"],
                correctAnswer: "Vero",
                explanation: "Il Teorema di Pitagora vale esclusivamente per i triangoli rettangoli, cioè quelli con un angolo di 90°.",
                difficulty: 1
            )
        ],
        xpReward: 150,
        difficulty: 2,
        estimatedMinutes: 8
    )

    static let previewQuizzes: [Quiz] = [
        preview,
        Quiz(
            title: "La Fisica di Galileo",
            subject: .fisica,
            maestro: "Galileo",
            description: "Esplora i fondamenti del metodo scientifico e del moto.",
            questions: [
                Question(
                    text: "Qual è la caratteristica principale del metodo scientifico galileiano?",
                    type: .multipleChoice,
                    options: [
                        "L'osservazione e l'esperimento",
                        "Solo la teoria matematica",
                        "La fede religiosa",
                        "L'intuizione personale"
                    ],
                    correctAnswer: "L'osservazione e l'esperimento",
                    explanation: "Galileo ha rivoluzionato la scienza enfatizzando l'importanza dell'osservazione empirica e degli esperimenti verificabili.",
                    difficulty: 2
                )
            ],
            xpReward: 120,
            difficulty: 3,
            estimatedMinutes: 12
        )
    ]
}

extension Question {
    static let preview = Quiz.preview.questions[0]
}

extension QuizResult {
    static let preview = QuizResult(
        quizId: Quiz.preview.id,
        score: 2,
        totalQuestions: 3,
        answers: [
            StudentAnswer(questionId: UUID(), answer: "L'ipotenusa al quadrato è uguale alla somma dei quadrati dei cateti", isCorrect: true),
            StudentAnswer(questionId: UUID(), answer: "5", isCorrect: true),
            StudentAnswer(questionId: UUID(), answer: "Falso", isCorrect: false, attemptsCount: 2)
        ],
        xpEarned: 100,
        completionTime: 245
    )
}
