/**
 * CONVERGIO NATIVE - Quiz ViewModel
 *
 * Manages quiz state, answer tracking, and maieutic feedback.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import Combine

@MainActor
final class QuizViewModel: ObservableObject {
    // MARK: - Published Properties

    @Published private(set) var quiz: Quiz
    @Published private(set) var currentQuestionIndex: Int = 0
    @Published private(set) var answers: [StudentAnswer] = []
    @Published private(set) var currentAnswer: String = ""
    @Published private(set) var showingFeedback: Bool = false
    @Published private(set) var feedbackMessage: String = ""
    @Published private(set) var isCorrect: Bool = false
    @Published private(set) var showingHint: Bool = false
    @Published private(set) var attemptsForCurrentQuestion: Int = 0
    @Published private(set) var isQuizComplete: Bool = false
    @Published private(set) var quizResult: QuizResult?
    @Published private(set) var timeRemaining: TimeInterval?
    @Published private(set) var startTime: Date?
    @Published private(set) var elapsedTime: TimeInterval = 0

    // MARK: - Private Properties

    private var timer: Timer?
    private var questionAttempts: [UUID: Int] = [:] // Track attempts per question

    // MARK: - Computed Properties

    var currentQuestion: Question? {
        guard currentQuestionIndex < quiz.questions.count else { return nil }
        return quiz.questions[currentQuestionIndex]
    }

    var progress: Double {
        guard quiz.questions.count > 0 else { return 0 }
        return Double(currentQuestionIndex) / Double(quiz.questions.count)
    }

    var score: Int {
        answers.filter { $0.isCorrect }.count
    }

    var canAdvance: Bool {
        !currentAnswer.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
    }

    // MARK: - Initialization

    init(quiz: Quiz) {
        self.quiz = quiz
        self.timeRemaining = quiz.timeLimit
    }

    // MARK: - Quiz Control

    /// Start the quiz
    func startQuiz() {
        startTime = Date()
        currentQuestionIndex = 0
        answers.removeAll()
        questionAttempts.removeAll()
        currentAnswer = ""
        isQuizComplete = false
        quizResult = nil
        elapsedTime = 0

        // Start timer if time limit is set
        if quiz.timeLimit != nil {
            startTimer()
        }
    }

    /// Submit current answer
    func submitAnswer() {
        guard let question = currentQuestion else { return }

        // Increment attempts for this question
        let attempts = (questionAttempts[question.id] ?? 0) + 1
        questionAttempts[question.id] = attempts
        attemptsForCurrentQuestion = attempts

        // Check if answer is correct
        let correct = question.isCorrect(currentAnswer)
        isCorrect = correct

        // Generate maieutic feedback
        feedbackMessage = generateFeedback(
            question: question,
            answer: currentAnswer,
            isCorrect: correct,
            attempts: attempts
        )

        // Show feedback
        showingFeedback = true

        // If correct, save the answer
        if correct {
            let studentAnswer = StudentAnswer(
                questionId: question.id,
                answer: currentAnswer,
                isCorrect: true,
                attemptsCount: attempts
            )
            answers.append(studentAnswer)
        }
    }

    /// Move to next question after feedback
    func nextQuestion() {
        // If answer was incorrect, save it
        if !isCorrect, let question = currentQuestion {
            let studentAnswer = StudentAnswer(
                questionId: question.id,
                answer: currentAnswer,
                isCorrect: false,
                attemptsCount: attemptsForCurrentQuestion
            )
            answers.append(studentAnswer)
        }

        // Reset for next question
        currentAnswer = ""
        showingFeedback = false
        showingHint = false
        attemptsForCurrentQuestion = 0

        // Move to next question or complete quiz
        if currentQuestionIndex < quiz.questions.count - 1 {
            currentQuestionIndex += 1
        } else {
            completeQuiz()
        }
    }

    /// Try again on current question (maieutic approach)
    func tryAgain() {
        currentAnswer = ""
        showingFeedback = false
    }

    /// Show hint for current question
    func showHint() {
        showingHint = true
    }

    /// Complete the quiz and calculate results
    private func completeQuiz() {
        stopTimer()
        isQuizComplete = true

        let finalScore = score
        let totalQuestions = quiz.questions.count

        // Calculate XP based on performance
        let percentage = Double(finalScore) / Double(totalQuestions)
        var xpEarned = Int(Double(quiz.xpReward) * percentage)

        // Bonus XP for mastery (80%+)
        if percentage >= quiz.requiredScore {
            xpEarned += 50
        }

        // Bonus XP for speed (if timed)
        if let timeLimit = quiz.timeLimit, elapsedTime < timeLimit * 0.75 {
            xpEarned += 25
        }

        // Calculate completion time
        let completionTime = startTime.map { Date().timeIntervalSince($0) } ?? elapsedTime

        quizResult = QuizResult(
            quizId: quiz.id,
            score: finalScore,
            totalQuestions: totalQuestions,
            answers: answers,
            xpEarned: xpEarned,
            completionTime: completionTime
        )
    }

    // MARK: - Timer Management

    private func startTimer() {
        timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            Task { @MainActor [weak self] in
                guard let self = self else { return }
                self.elapsedTime += 1

                if let timeLimit = self.quiz.timeLimit {
                    let remaining = timeLimit - self.elapsedTime
                    self.timeRemaining = max(0, remaining)

                    // Auto-complete if time runs out
                    if remaining <= 0 {
                        self.completeQuiz()
                    }
                }
            }
        }
    }

    private func stopTimer() {
        timer?.invalidate()
        timer = nil
    }

    // MARK: - Maieutic Feedback Generation

    /// Generate encouraging, maieutic feedback based on answer
    private func generateFeedback(
        question: Question,
        answer: String,
        isCorrect: Bool,
        attempts: Int
    ) -> String {
        if isCorrect {
            // Positive reinforcement
            let messages = [
                "Ottimo! Hai capito il concetto! 🌟",
                "Esatto! Il tuo ragionamento è corretto! 💡",
                "Perfetto! Continua così! ✨",
                "Molto bene! Hai colto nel segno! 🎯",
                "Eccellente! Dimostri una buona comprensione! 👏"
            ]

            // Extra encouragement for getting it right after attempts
            if attempts > 1 {
                return "Bravissimo! Ci hai riflettuto e hai trovato la risposta giusta! Non arrendersi paga sempre! 🏆"
            }

            return messages.randomElement() ?? messages[0]
        } else {
            // Maieutic guidance - never say "wrong"
            if attempts == 1 {
                // First attempt - gentle guidance
                let messages = [
                    "Mmm, ripensa a quello che sai su questo argomento. Cosa potrebbe aiutarti?",
                    "Interessante prospettiva! Prova a considerare l'argomento da un altro punto di vista.",
                    "Ci sei vicino! Rifletti ancora un momento: cosa ti dice l'intuizione?",
                    "Buon tentativo! Proviamo a ragionare insieme: cosa sai già su questo?",
                    "Quasi! Ripensa ai concetti fondamentali che abbiamo studiato."
                ]
                return messages.randomElement() ?? messages[0]
            } else if attempts == 2 {
                // Second attempt - more specific guidance with hint
                if let hint = question.hint {
                    return "Ecco un suggerimento che potrebbe aiutarti: \(hint)"
                }
                return "Prendiamoci un momento per riflettere. Cosa succederebbe se provassimo da un'altra angolazione?"
            } else {
                // Third+ attempt - show explanation and move forward
                return "Capisco che questo è un concetto complesso. Ecco la spiegazione: \(question.explanation)\n\nNon preoccuparti, l'importante è imparare! Andiamo avanti! 💪"
            }
        }
    }

    // MARK: - Quiz Actions

    /// Reset quiz to initial state
    func resetQuiz() {
        stopTimer()
        currentQuestionIndex = 0
        answers.removeAll()
        questionAttempts.removeAll()
        currentAnswer = ""
        showingFeedback = false
        showingHint = false
        attemptsForCurrentQuestion = 0
        isQuizComplete = false
        quizResult = nil
        timeRemaining = quiz.timeLimit
        startTime = nil
        elapsedTime = 0
    }

    /// Set current answer (for text input)
    func setAnswer(_ answer: String) {
        currentAnswer = answer
    }

    // MARK: - Cleanup

    deinit {
        timer?.invalidate()
        timer = nil
    }
}

// MARK: - Preview Helper

extension QuizViewModel {
    /// Preview instance with mock data
    static var preview: QuizViewModel {
        let vm = QuizViewModel(quiz: .preview)
        vm.startQuiz()
        return vm
    }
}
