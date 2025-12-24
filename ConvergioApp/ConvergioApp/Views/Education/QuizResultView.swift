/**
 * CONVERGIO NATIVE - Quiz Result View
 *
 * Displays quiz results with encouraging feedback and detailed review.
 * Shows XP earned, mastery achievement, and question-by-question analysis.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

struct QuizResultView: View {
    let result: QuizResult
    let quiz: Quiz
    let onRetry: () -> Void
    let onDismiss: () -> Void

    @State private var showingDetailedReview = false
    @State private var animateScore = false

    var body: some View {
        ScrollView {
            VStack(spacing: 24) {
                // Celebration header
                celebrationHeader

                // Score card
                scoreCard

                // XP earned
                xpCard

                // Mastery status
                if result.masteryAchieved {
                    masteryCard
                }

                // Stats
                statsCard

                // Encouraging message from maestro
                maestroMessage

                // Detailed review toggle
                detailedReviewSection

                // Action buttons
                actionButtons
            }
            .padding()
        }
        .background(Color(NSColor.windowBackgroundColor))
        .onAppear {
            withAnimation(.spring(response: 0.6, dampingFraction: 0.7).delay(0.3)) {
                animateScore = true
            }
        }
    }

    // MARK: - Celebration Header

    private var celebrationHeader: some View {
        VStack(spacing: 16) {
            // Animated icon
            ZStack {
                Circle()
                    .fill(result.gradeColor.opacity(0.2))
                    .frame(width: 120, height: 120)

                Image(systemName: result.masteryAchieved ? "trophy.fill" : "checkmark.seal.fill")
                    .font(.system(size: 60))
                    .foregroundColor(result.gradeColor)
                    .scaleEffect(animateScore ? 1.0 : 0.5)
                    .opacity(animateScore ? 1.0 : 0.0)
            }

            Text(result.grade)
                .font(.largeTitle)
                .fontWeight(.bold)
                .foregroundColor(result.gradeColor)

            Text(quiz.title)
                .font(.title2)
                .foregroundColor(.secondary)
        }
        .padding()
    }

    // MARK: - Score Card

    private var scoreCard: some View {
        VStack(spacing: 16) {
            // Large percentage display
            HStack(alignment: .firstTextBaseline, spacing: 4) {
                Text(String(format: "%.0f", result.percentage))
                    .font(.system(size: 72, weight: .bold, design: .rounded))
                    .foregroundColor(result.gradeColor)
                Text("%")
                    .font(.system(size: 36, weight: .bold, design: .rounded))
                    .foregroundColor(result.gradeColor)
            }

            // Fraction display
            Text("\(result.score) corrette su \(result.totalQuestions)")
                .font(.title3)
                .foregroundColor(.secondary)

            // Visual progress bar
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    Rectangle()
                        .fill(Color.gray.opacity(0.2))
                        .frame(height: 20)
                        .cornerRadius(10)

                    Rectangle()
                        .fill(result.gradeColor)
                        .frame(
                            width: animateScore ? geometry.size.width * (result.percentage / 100) : 0,
                            height: 20
                        )
                        .cornerRadius(10)
                }
            }
            .frame(height: 20)
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    // MARK: - XP Card

    private var xpCard: some View {
        HStack(spacing: 16) {
            Image(systemName: "star.fill")
                .font(.title)
                .foregroundColor(.yellow)

            VStack(alignment: .leading, spacing: 4) {
                Text("XP Guadagnati")
                    .font(.subheadline)
                    .foregroundColor(.secondary)

                Text("+\(result.xpEarned) XP")
                    .font(.title2)
                    .fontWeight(.bold)
                    .foregroundColor(.primary)
            }

            Spacer()

            if result.masteryAchieved {
                VStack(alignment: .trailing, spacing: 4) {
                    Text("Bonus Maestria")
                        .font(.caption)
                        .foregroundColor(.green)
                    Text("+50 XP")
                        .font(.subheadline)
                        .fontWeight(.semibold)
                        .foregroundColor(.green)
                }
            }
        }
        .padding()
        .background(Color.yellow.opacity(0.1))
        .cornerRadius(12)
    }

    // MARK: - Mastery Card

    private var masteryCard: some View {
        HStack(spacing: 12) {
            Image(systemName: "checkmark.seal.fill")
                .font(.title)
                .foregroundColor(.green)

            VStack(alignment: .leading, spacing: 4) {
                Text("Maestria Raggiunta!")
                    .font(.headline)
                    .foregroundColor(.green)

                Text("Hai superato la soglia dell'80% e puoi avanzare al prossimo argomento!")
                    .font(.subheadline)
                    .foregroundColor(.secondary)
            }

            Spacer()
        }
        .padding()
        .background(Color.green.opacity(0.1))
        .cornerRadius(12)
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(Color.green, lineWidth: 2)
        )
    }

    // MARK: - Stats Card

    private var statsCard: some View {
        VStack(spacing: 16) {
            Text("Statistiche")
                .font(.headline)
                .frame(maxWidth: .infinity, alignment: .leading)

            LazyVGrid(columns: [
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible())
            ], spacing: 16) {
                statItem(
                    icon: "clock",
                    label: "Tempo Impiegato",
                    value: formatTime(result.completionTime),
                    color: .blue
                )

                statItem(
                    icon: "checkmark.circle",
                    label: "Risposte Corrette",
                    value: "\(result.score)",
                    color: .green
                )

                statItem(
                    icon: "xmark.circle",
                    label: "Risposte Errate",
                    value: "\(result.totalQuestions - result.score)",
                    color: .red
                )
            }
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    private func statItem(icon: String, label: String, value: String, color: Color) -> some View {
        VStack(spacing: 8) {
            Image(systemName: icon)
                .font(.title2)
                .foregroundColor(color)

            Text(value)
                .font(.title3)
                .fontWeight(.bold)

            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity)
        .padding()
        .background(color.opacity(0.1))
        .cornerRadius(8)
    }

    // MARK: - Maestro Message

    private var maestroMessage: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Image(systemName: quiz.subject.icon)
                    .font(.title2)
                    .foregroundColor(quiz.subject.color)

                Text("Messaggio da \(quiz.maestro)")
                    .font(.headline)
            }

            Text(generateMaestroMessage())
                .font(.body)
                .foregroundColor(.secondary)
                .italic()
        }
        .padding()
        .background(quiz.subject.color.opacity(0.1))
        .cornerRadius(12)
    }

    private func generateMaestroMessage() -> String {
        if result.percentage >= 95 {
            return "Straordinario! La tua comprensione dell'argomento è eccezionale. Sei pronto per sfide ancora più grandi!"
        } else if result.masteryAchieved {
            return "Ottimo lavoro! Hai dimostrato una solida comprensione. Continua con questa dedizione e non ci sarà limite a quello che potrai imparare!"
        } else if result.percentage >= 70 {
            return "Buon lavoro! Hai una buona base, ma c'è ancora spazio per migliorare. Riprova il quiz e consolida le tue conoscenze!"
        } else if result.percentage >= 60 {
            return "Stai facendo progressi! Non scoraggiarti, l'apprendimento è un percorso. Rivedi i concetti e riprova con fiducia!"
        } else {
            return "Non preoccuparti, imparare richiede tempo e pratica. Rivedi i materiali, rifletti sui concetti chiave e riprova. Ogni tentativo ti avvicina alla comprensione!"
        }
    }

    // MARK: - Detailed Review Section

    private var detailedReviewSection: some View {
        VStack(spacing: 16) {
            Button(action: { showingDetailedReview.toggle() }) {
                HStack {
                    Text("Revisione Dettagliata")
                        .font(.headline)

                    Spacer()

                    Image(systemName: showingDetailedReview ? "chevron.up" : "chevron.down")
                }
            }
            .buttonStyle(.plain)
            .padding()
            .background(Color(NSColor.controlBackgroundColor))
            .cornerRadius(12)

            if showingDetailedReview {
                questionReview
            }
        }
    }

    private var questionReview: some View {
        VStack(spacing: 16) {
            ForEach(Array(zip(quiz.questions, result.answers).enumerated()), id: \.offset) { index, pair in
                let (question, answer) = pair
                questionReviewCard(question: question, answer: answer, number: index + 1)
            }
        }
    }

    private func questionReviewCard(question: Question, answer: StudentAnswer, number: Int) -> some View {
        VStack(alignment: .leading, spacing: 12) {
            // Question number and status
            HStack {
                Text("Domanda \(number)")
                    .font(.headline)

                Spacer()

                HStack(spacing: 4) {
                    Image(systemName: answer.isCorrect ? "checkmark.circle.fill" : "xmark.circle.fill")
                    Text(answer.isCorrect ? "Corretta" : "Errata")
                }
                .font(.subheadline)
                .foregroundColor(answer.isCorrect ? .green : .red)
            }

            // Question text
            Text(question.text)
                .font(.body)
                .foregroundColor(.primary)

            Divider()

            // Student's answer
            VStack(alignment: .leading, spacing: 4) {
                Text("La tua risposta:")
                    .font(.caption)
                    .foregroundColor(.secondary)

                Text(answer.answer)
                    .font(.body)
                    .padding(8)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .background(answer.isCorrect ? Color.green.opacity(0.1) : Color.red.opacity(0.1))
                    .cornerRadius(6)
            }

            // Correct answer (if wrong)
            if !answer.isCorrect {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Risposta corretta:")
                        .font(.caption)
                        .foregroundColor(.secondary)

                    Text(question.correctAnswer)
                        .font(.body)
                        .padding(8)
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .background(Color.green.opacity(0.1))
                        .cornerRadius(6)
                }
            }

            // Explanation
            VStack(alignment: .leading, spacing: 4) {
                Text("Spiegazione:")
                    .font(.caption)
                    .fontWeight(.semibold)
                    .foregroundColor(.secondary)

                Text(question.explanation)
                    .font(.subheadline)
                    .foregroundColor(.secondary)
            }

            // Attempts indicator
            if answer.attemptsCount > 1 {
                HStack(spacing: 4) {
                    Image(systemName: "arrow.triangle.2.circlepath")
                        .font(.caption)
                    Text("\(answer.attemptsCount) tentativi")
                        .font(.caption)
                }
                .foregroundColor(.orange)
            }
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    // MARK: - Action Buttons

    private var actionButtons: some View {
        HStack(spacing: 12) {
            Button("Chiudi") {
                onDismiss()
            }
            .buttonStyle(.bordered)

            if !result.masteryAchieved {
                Button("Riprova Quiz") {
                    onRetry()
                }
                .buttonStyle(.borderedProminent)
                .tint(.orange)
            }

            if result.masteryAchieved {
                Button("Prossimo Argomento") {
                    // TODO: Navigate to next topic
                    onDismiss()
                }
                .buttonStyle(.borderedProminent)
                .tint(.green)
            }
        }
        .padding()
    }

    // MARK: - Helpers

    private func formatTime(_ seconds: TimeInterval) -> String {
        let minutes = Int(seconds) / 60
        let remainingSeconds = Int(seconds) % 60

        if minutes > 0 {
            return "\(minutes)m \(remainingSeconds)s"
        } else {
            return "\(remainingSeconds)s"
        }
    }
}

// MARK: - Preview

#Preview {
    QuizResultView(
        result: .preview,
        quiz: .preview,
        onRetry: { print("Retry") },
        onDismiss: { print("Dismiss") }
    )
    .frame(width: 800, height: 900)
}
