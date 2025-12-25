/**
 * CONVERGIO NATIVE - Quiz Selection View
 *
 * Displays available quizzes grouped by subject.
 * Users can select a quiz to start the maieutic quiz experience.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

struct QuizSelectionView: View {
    @State private var selectedSubject: Subject?
    @State private var selectedQuiz: Quiz?
    @State private var searchText = ""

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(spacing: 24) {
                    // Header
                    headerSection

                    // Subject filter
                    subjectFilterSection

                    // Quiz list
                    quizListSection
                }
                .padding()
            }
            .navigationTitle("Quiz")
            .sheet(item: $selectedQuiz) { quiz in
                QuizView(quiz: quiz)
            }
        }
    }

    // MARK: - Header

    private var headerSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Metti alla prova le tue conoscenze")
                .font(.title2.weight(.semibold))

            Text("Scegli un quiz per verificare la tua preparazione con feedback maieutico")
                .font(.subheadline)
                .foregroundStyle(.secondary)

            // Search field
            GlassTextField(
                placeholder: "Cerca quiz...",
                text: $searchText,
                icon: "magnifyingglass"
            )
        }
    }

    // MARK: - Subject Filter

    private var subjectFilterSection: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 8) {
                // All subjects
                filterPill(subject: nil, label: "Tutti")

                // Individual subjects
                ForEach(Subject.allCases) { subject in
                    filterPill(subject: subject, label: subject.rawValue)
                }
            }
        }
    }

    private func filterPill(subject: Subject?, label: String) -> some View {
        Button {
            withAnimation(.spring(duration: 0.3)) {
                selectedSubject = subject
            }
        } label: {
            HStack(spacing: 6) {
                if let subject = subject {
                    Image(systemName: subject.icon)
                        .font(.caption)
                }
                Text(label)
                    .font(.subheadline.weight(.medium))
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(
                ZStack {
                    if selectedSubject == subject {
                        subject?.color ?? Color.accentColor
                    } else {
                        VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)
                        Color.primary.opacity(0.05)
                    }
                }
            )
            .foregroundStyle(selectedSubject == subject ? .white : .primary)
            .clipShape(Capsule())
            .overlay(
                Capsule()
                    .stroke(
                        selectedSubject == subject
                            ? (subject?.color ?? Color.accentColor).opacity(0.5)
                            : Color.primary.opacity(0.1),
                        lineWidth: 1
                    )
            )
        }
        .buttonStyle(.plain)
    }

    // MARK: - Quiz List

    private var quizListSection: some View {
        let columns = [
            GridItem(.adaptive(minimum: 280, maximum: 400), spacing: 16)
        ]

        return LazyVGrid(columns: columns, spacing: 16) {
            ForEach(filteredQuizzes) { quiz in
                QuizCardView(quiz: quiz)
                    .onTapGesture {
                        selectedQuiz = quiz
                    }
            }
        }
    }

    // MARK: - Filtered Quizzes

    private var filteredQuizzes: [Quiz] {
        var quizzes = Quiz.previewQuizzes

        // Filter by subject
        if let subject = selectedSubject {
            quizzes = quizzes.filter { $0.subject == subject }
        }

        // Filter by search text
        if !searchText.isEmpty {
            quizzes = quizzes.filter { quiz in
                quiz.title.localizedCaseInsensitiveContains(searchText) ||
                quiz.description.localizedCaseInsensitiveContains(searchText) ||
                quiz.maestro.localizedCaseInsensitiveContains(searchText)
            }
        }

        return quizzes
    }
}

// MARK: - Quiz Card View

struct QuizCardView: View {
    let quiz: Quiz
    @State private var isHovered = false

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            // Header with subject and difficulty
            HStack {
                // Subject badge
                HStack(spacing: 4) {
                    Image(systemName: quiz.subject.icon)
                        .font(.caption)
                    Text(quiz.subject.rawValue)
                        .font(.caption.weight(.medium))
                }
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(quiz.subject.color.opacity(0.15))
                .foregroundStyle(quiz.subject.color)
                .clipShape(Capsule())

                Spacer()

                // Difficulty stars
                HStack(spacing: 2) {
                    ForEach(0..<5) { index in
                        Image(systemName: index < quiz.difficulty ? "star.fill" : "star")
                            .font(.caption2)
                            .foregroundColor(index < quiz.difficulty ? .orange : .gray.opacity(0.3))
                    }
                }
            }

            // Title
            Text(quiz.title)
                .font(.headline)
                .lineLimit(2)

            // Description
            Text(quiz.description)
                .font(.subheadline)
                .foregroundStyle(.secondary)
                .lineLimit(2)

            Spacer()

            // Footer
            HStack {
                // Maestro
                HStack(spacing: 4) {
                    Image(systemName: "person.fill")
                        .font(.caption)
                    Text(quiz.maestro)
                        .font(.caption)
                }
                .foregroundStyle(.secondary)

                Spacer()

                // Time and questions
                HStack(spacing: 12) {
                    HStack(spacing: 4) {
                        Image(systemName: "clock")
                            .font(.caption)
                        Text("\(quiz.estimatedMinutes) min")
                            .font(.caption)
                    }

                    HStack(spacing: 4) {
                        Image(systemName: "questionmark.circle")
                            .font(.caption)
                        Text("\(quiz.questions.count)")
                            .font(.caption)
                    }
                }
                .foregroundStyle(.secondary)
            }

            // XP reward
            HStack {
                Spacer()
                HStack(spacing: 4) {
                    Image(systemName: "star.circle.fill")
                        .foregroundStyle(.yellow)
                    Text("+\(quiz.xpReward) XP")
                        .font(.caption.weight(.semibold))
                        .foregroundStyle(.orange)
                }
            }
        }
        .padding()
        .frame(height: 200)
        .background(cardBackground)
        .clipShape(RoundedRectangle(cornerRadius: 16))
        .overlay(
            RoundedRectangle(cornerRadius: 16)
                .stroke(
                    isHovered ? quiz.subject.color.opacity(0.5) : Color.white.opacity(0.1),
                    lineWidth: isHovered ? 2 : 0.5
                )
        )
        .shadow(
            color: isHovered ? quiz.subject.color.opacity(0.3) : .black.opacity(0.1),
            radius: isHovered ? 12 : 8,
            x: 0,
            y: 4
        )
        .scaleEffect(isHovered ? 1.02 : 1.0)
        .animation(.spring(duration: 0.3), value: isHovered)
        .onHover { hovering in
            isHovered = hovering
        }
    }

    @ViewBuilder
    private var cardBackground: some View {
        ZStack {
            VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)

            LinearGradient(
                colors: [
                    quiz.subject.color.opacity(isHovered ? 0.12 : 0.06),
                    quiz.subject.color.opacity(isHovered ? 0.06 : 0.02)
                ],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )

            // Subtle glass shine
            LinearGradient(
                colors: [
                    .white.opacity(0.1),
                    .clear,
                    .black.opacity(0.03)
                ],
                startPoint: .top,
                endPoint: .bottom
            )
        }
    }
}

// MARK: - Preview

#Preview {
    QuizSelectionView()
        .frame(width: 800, height: 600)
}
