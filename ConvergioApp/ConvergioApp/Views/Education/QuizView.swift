/**
 * CONVERGIO NATIVE - Quiz View
 *
 * Interactive quiz interface with maieutic feedback.
 * Supports multiple question types and real-time progress tracking.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

struct QuizView: View {
    @StateObject private var viewModel: QuizViewModel
    @Environment(\.dismiss) private var dismiss
    @State private var showingExitAlert = false

    init(quiz: Quiz) {
        _viewModel = StateObject(wrappedValue: QuizViewModel(quiz: quiz))
    }

    var body: some View {
        ZStack {
            if viewModel.isQuizComplete, let result = viewModel.quizResult {
                QuizResultView(result: result, quiz: viewModel.quiz, onRetry: {
                    viewModel.resetQuiz()
                    viewModel.startQuiz()
                }, onDismiss: {
                    dismiss()
                })
            } else {
                quizContent
            }
        }
        .onAppear {
            if viewModel.startTime == nil {
                viewModel.startQuiz()
            }
        }
    }

    // MARK: - Quiz Content

    private var quizContent: some View {
        VStack(spacing: 0) {
            // Header with progress and timer
            quizHeader

            Divider()

            // Question area
            ScrollView {
                VStack(spacing: 24) {
                    if let question = viewModel.currentQuestion {
                        questionCard(question)
                    }
                }
                .padding()
            }

            Divider()

            // Action buttons
            actionButtons
        }
        .background(Color(NSColor.windowBackgroundColor))
        .alert("Vuoi uscire dal quiz?", isPresented: $showingExitAlert) {
            Button("Annulla", role: .cancel) { }
            Button("Esci", role: .destructive) {
                dismiss()
            }
        } message: {
            Text("Il tuo progresso andrà perso.")
        }
    }

    // MARK: - Header

    private var quizHeader: some View {
        VStack(spacing: 12) {
            // Title and close button
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text(viewModel.quiz.title)
                        .font(.title2)
                        .fontWeight(.bold)

                    HStack(spacing: 8) {
                        Image(systemName: viewModel.quiz.subject.icon)
                        Text(viewModel.quiz.maestro)
                        Text("•")
                        Text("\(viewModel.quiz.difficulty) / 5")
                    }
                    .font(.caption)
                    .foregroundColor(.secondary)
                }

                Spacer()

                Button(action: { showingExitAlert = true }) {
                    Image(systemName: "xmark.circle.fill")
                        .font(.title2)
                        .foregroundColor(.secondary)
                }
                .buttonStyle(.plain)
            }

            // Progress bar
            ProgressView(value: viewModel.progress)
                .tint(viewModel.quiz.subject.color)

            // Question counter and timer
            HStack {
                Text("Domanda \(viewModel.currentQuestionIndex + 1) di \(viewModel.quiz.questions.count)")
                    .font(.subheadline)
                    .foregroundColor(.secondary)

                Spacer()

                if let timeRemaining = viewModel.timeRemaining {
                    HStack(spacing: 4) {
                        Image(systemName: "timer")
                        Text(formatTime(timeRemaining))
                    }
                    .font(.subheadline)
                    .foregroundColor(timeRemaining < 60 ? .red : .secondary)
                } else {
                    HStack(spacing: 4) {
                        Image(systemName: "clock")
                        Text(formatTime(viewModel.elapsedTime))
                    }
                    .font(.subheadline)
                    .foregroundColor(.secondary)
                }
            }
        }
        .padding()
    }

    // MARK: - Question Card

    private func questionCard(_ question: Question) -> some View {
        VStack(alignment: .leading, spacing: 20) {
            // Question text
            VStack(alignment: .leading, spacing: 12) {
                HStack {
                    Image(systemName: question.type.icon)
                        .foregroundColor(viewModel.quiz.subject.color)
                    Text(question.type.displayName)
                        .font(.caption)
                        .foregroundColor(.secondary)
                }

                Text(question.text)
                    .font(.title3)
                    .fontWeight(.medium)
            }

            // Difficulty indicator
            HStack(spacing: 4) {
                ForEach(0..<5) { index in
                    Image(systemName: index < question.difficulty ? "star.fill" : "star")
                        .font(.caption)
                        .foregroundColor(index < question.difficulty ? .orange : .gray.opacity(0.3))
                }
                Text("Difficoltà")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            Divider()

            // Answer input based on question type
            answerInput(for: question)

            // Hint button (if available and not showing feedback)
            if question.hint != nil && !viewModel.showingFeedback {
                Button(action: { viewModel.showHint() }) {
                    HStack {
                        Image(systemName: "lightbulb")
                        Text(viewModel.showingHint ? "Suggerimento" : "Mostra suggerimento")
                    }
                    .font(.subheadline)
                }
                .buttonStyle(.plain)
                .foregroundColor(viewModel.quiz.subject.color)

                if viewModel.showingHint, let hint = question.hint {
                    Text(hint)
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                        .padding()
                        .background(Color.yellow.opacity(0.1))
                        .cornerRadius(8)
                }
            }

            // Feedback area
            if viewModel.showingFeedback {
                feedbackCard
            }
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    // MARK: - Answer Input

    private func answerInput(for question: Question) -> some View {
        Group {
            switch question.type {
            case .multipleChoice, .trueFalse:
                multipleChoiceInput(question)
            case .openEnded:
                openEndedInput(question)
            }
        }
    }

    private func multipleChoiceInput(_ question: Question) -> some View {
        VStack(spacing: 12) {
            ForEach(question.options ?? [], id: \.self) { option in
                Button(action: {
                    if !viewModel.showingFeedback {
                        viewModel.setAnswer(option)
                    }
                }) {
                    HStack {
                        Text(option)
                            .font(.body)
                            .foregroundColor(.primary)
                            .multilineTextAlignment(.leading)

                        Spacer()

                        if viewModel.currentAnswer == option {
                            Image(systemName: "checkmark.circle.fill")
                                .foregroundColor(viewModel.quiz.subject.color)
                        } else {
                            Image(systemName: "circle")
                                .foregroundColor(.secondary)
                        }
                    }
                    .padding()
                    .background(
                        viewModel.currentAnswer == option ?
                        viewModel.quiz.subject.color.opacity(0.1) :
                        Color(NSColor.controlBackgroundColor)
                    )
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(
                                viewModel.currentAnswer == option ?
                                viewModel.quiz.subject.color :
                                Color.gray.opacity(0.3),
                                lineWidth: viewModel.currentAnswer == option ? 2 : 1
                            )
                    )
                    .cornerRadius(8)
                }
                .buttonStyle(.plain)
                .disabled(viewModel.showingFeedback)
            }
        }
    }

    private func openEndedInput(_ question: Question) -> some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("La tua risposta:")
                .font(.subheadline)
                .foregroundColor(.secondary)

            TextEditor(text: Binding(
                get: { viewModel.currentAnswer },
                set: { viewModel.setAnswer($0) }
            ))
            .font(.body)
            .frame(minHeight: 100)
            .padding(8)
            .background(Color(NSColor.textBackgroundColor))
            .cornerRadius(8)
            .overlay(
                RoundedRectangle(cornerRadius: 8)
                    .stroke(Color.gray.opacity(0.3), lineWidth: 1)
            )
            .disabled(viewModel.showingFeedback)
        }
    }

    // MARK: - Feedback Card

    private var feedbackCard: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Image(systemName: viewModel.isCorrect ? "checkmark.circle.fill" : "lightbulb.fill")
                    .font(.title2)
                    .foregroundColor(viewModel.isCorrect ? .green : .orange)

                Text(viewModel.isCorrect ? "Risposta Corretta!" : "Proviamo a riflettere...")
                    .font(.headline)
                    .foregroundColor(viewModel.isCorrect ? .green : .orange)
            }

            Text(viewModel.feedbackMessage)
                .font(.body)
                .foregroundColor(.primary)

            // Show explanation if correct or after multiple attempts
            if viewModel.isCorrect || viewModel.attemptsForCurrentQuestion >= 3,
               let question = viewModel.currentQuestion {
                Divider()

                VStack(alignment: .leading, spacing: 8) {
                    Text("Spiegazione:")
                        .font(.subheadline)
                        .fontWeight(.semibold)

                    Text(question.explanation)
                        .font(.body)
                        .foregroundColor(.secondary)
                }
            }
        }
        .padding()
        .background(viewModel.isCorrect ? Color.green.opacity(0.1) : Color.orange.opacity(0.1))
        .cornerRadius(8)
    }

    // MARK: - Action Buttons

    private var actionButtons: some View {
        HStack(spacing: 12) {
            if viewModel.showingFeedback {
                if !viewModel.isCorrect && viewModel.attemptsForCurrentQuestion < 3 {
                    Button("Riprova") {
                        viewModel.tryAgain()
                    }
                    .buttonStyle(.bordered)
                }

                Button(viewModel.currentQuestionIndex < viewModel.quiz.questions.count - 1 ? "Prossima Domanda" : "Completa Quiz") {
                    withAnimation(.easeInOut(duration: 0.3)) {
                        viewModel.nextQuestion()
                    }
                }
                .buttonStyle(.borderedProminent)
                .tint(viewModel.quiz.subject.color)
            } else {
                Button("Invia Risposta") {
                    viewModel.submitAnswer()
                }
                .buttonStyle(.borderedProminent)
                .tint(viewModel.quiz.subject.color)
                .disabled(!viewModel.canAdvance)
            }
        }
        .padding()
    }

    // MARK: - Helpers

    private func formatTime(_ seconds: TimeInterval) -> String {
        let minutes = Int(seconds) / 60
        let remainingSeconds = Int(seconds) % 60
        return String(format: "%d:%02d", minutes, remainingSeconds)
    }
}

// MARK: - Preview

#Preview {
    QuizView(quiz: .preview)
        .frame(width: 800, height: 600)
}
