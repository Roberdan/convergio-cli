//
//  FlashcardView.swift
//  ConvergioApp
//
//  Individual flashcard display with flip animation
//
//  Created by Roberto Daniele on 2025-12-24.
//

import SwiftUI

struct FlashcardView: View {
    let flashcard: Flashcard
    let onRate: ((Rating) -> Void)?
    let showRatingButtons: Bool

    @State private var isFlipped = false
    @State private var dragOffset: CGSize = .zero
    @State private var rotationAngle: Double = 0

    private let cardWidth: CGFloat = 400
    private let cardHeight: CGFloat = 500

    init(
        flashcard: Flashcard,
        onRate: ((Rating) -> Void)? = nil,
        showRatingButtons: Bool = false
    ) {
        self.flashcard = flashcard
        self.onRate = onRate
        self.showRatingButtons = showRatingButtons
    }

    var body: some View {
        VStack(spacing: 20) {
            // Card with flip animation
            ZStack {
                // Back of card
                CardFace(
                    content: flashcard.back,
                    isBack: true,
                    subject: flashcard.subject,
                    maestro: flashcard.maestro
                )
                .opacity(isFlipped ? 1 : 0)
                .rotation3DEffect(
                    .degrees(isFlipped ? 0 : 180),
                    axis: (x: 0, y: 1, z: 0)
                )

                // Front of card
                CardFace(
                    content: flashcard.front,
                    isBack: false,
                    subject: flashcard.subject,
                    maestro: flashcard.maestro
                )
                .opacity(isFlipped ? 0 : 1)
                .rotation3DEffect(
                    .degrees(isFlipped ? 180 : 0),
                    axis: (x: 0, y: 1, z: 0)
                )
            }
            .frame(width: cardWidth, height: cardHeight)
            .onTapGesture {
                withAnimation(.spring(response: 0.6, dampingFraction: 0.8)) {
                    isFlipped.toggle()
                }
            }

            // Flip instruction (only show when not flipped)
            if !isFlipped {
                Text("Tap to reveal answer")
                    .font(.caption)
                    .foregroundColor(.secondary)
                    .transition(.opacity)
            }

            // Rating buttons (only show when flipped)
            if isFlipped && showRatingButtons {
                RatingButtonsView(onRate: handleRating)
                    .transition(.move(edge: .bottom).combined(with: .opacity))
            }

            // Card statistics
            if flashcard.state != .new {
                CardStatisticsView(flashcard: flashcard)
                    .transition(.opacity)
            }
        }
        .padding()
    }

    private func handleRating(_ rating: Rating) {
        onRate?(rating)
        // Reset flip state for next card
        withAnimation {
            isFlipped = false
        }
    }
}

// MARK: - Card Face

private struct CardFace: View {
    let content: String
    let isBack: Bool
    let subject: String
    let maestro: String

    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 20)
                .fill(
                    LinearGradient(
                        colors: [
                            isBack ? Color.blue.opacity(0.1) : Color.purple.opacity(0.1),
                            isBack ? Color.blue.opacity(0.05) : Color.purple.opacity(0.05)
                        ],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .overlay(
                    RoundedRectangle(cornerRadius: 20)
                        .stroke(
                            isBack ? Color.blue.opacity(0.3) : Color.purple.opacity(0.3),
                            lineWidth: 2
                        )
                )
                .shadow(color: .black.opacity(0.1), radius: 10, x: 0, y: 5)

            VStack(spacing: 20) {
                // Header with badge
                HStack {
                    SubjectBadge(subject: subject, maestro: maestro)
                    Spacer()
                    Text(isBack ? "Answer" : "Question")
                        .font(.caption)
                        .foregroundColor(.secondary)
                        .padding(.horizontal, 12)
                        .padding(.vertical, 6)
                        .background(Color.secondary.opacity(0.1))
                        .cornerRadius(12)
                }
                .padding(.horizontal)
                .padding(.top)

                // Content
                ScrollView {
                    Text(content)
                        .font(.title2)
                        .multilineTextAlignment(.center)
                        .padding()
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
    }
}

// MARK: - Subject Badge

private struct SubjectBadge: View {
    let subject: String
    let maestro: String

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(subject)
                .font(.caption)
                .fontWeight(.semibold)
                .foregroundColor(.white)
            Text(maestro)
                .font(.caption2)
                .foregroundColor(.white.opacity(0.9))
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(
            LinearGradient(
                colors: [Color.purple, Color.blue],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
        )
        .cornerRadius(12)
    }
}

// MARK: - Rating Buttons

private struct RatingButtonsView: View {
    let onRate: (Rating) -> Void

    var body: some View {
        HStack(spacing: 16) {
            ForEach(Rating.allCases, id: \.self) { rating in
                RatingButton(rating: rating) {
                    onRate(rating)
                }
            }
        }
        .padding()
        .background(Color.secondary.opacity(0.05))
        .cornerRadius(16)
    }
}

private struct RatingButton: View {
    let rating: Rating
    let action: () -> Void

    @State private var isHovered = false

    var body: some View {
        Button(action: action) {
            VStack(spacing: 8) {
                Image(systemName: rating.icon)
                    .font(.title2)
                    .foregroundColor(rating.color)
                Text(rating.rawValue)
                    .font(.caption)
                    .foregroundColor(.primary)
            }
            .frame(width: 80, height: 80)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(isHovered ? rating.color.opacity(0.1) : Color.clear)
            )
            .overlay(
                RoundedRectangle(cornerRadius: 12)
                    .stroke(rating.color.opacity(isHovered ? 0.5 : 0.3), lineWidth: 2)
            )
            .scaleEffect(isHovered ? 1.05 : 1.0)
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { hovering in
            withAnimation(.easeInOut(duration: 0.2)) {
                isHovered = hovering
            }
        }
    }
}

// MARK: - Card Statistics

private struct CardStatisticsView: View {
    let flashcard: Flashcard

    var body: some View {
        HStack(spacing: 20) {
            StatItem(
                icon: "repeat.circle",
                label: "Reviews",
                value: "\(flashcard.reps)"
            )
            StatItem(
                icon: "exclamationmark.triangle",
                label: "Lapses",
                value: "\(flashcard.lapses)"
            )
            StatItem(
                icon: "gauge.medium",
                label: "Difficulty",
                value: String(format: "%.1f", flashcard.difficulty)
            )
            if flashcard.state == .review {
                StatItem(
                    icon: "brain.head.profile",
                    label: "Recall",
                    value: String(format: "%.0f%%", flashcard.retrievability() * 100)
                )
            }
        }
        .padding()
        .background(Color.secondary.opacity(0.05))
        .cornerRadius(12)
    }
}

private struct StatItem: View {
    let icon: String
    let label: String
    let value: String

    var body: some View {
        VStack(spacing: 4) {
            Image(systemName: icon)
                .font(.caption)
                .foregroundColor(.secondary)
            Text(value)
                .font(.headline)
                .foregroundColor(.primary)
            Text(label)
                .font(.caption2)
                .foregroundColor(.secondary)
        }
    }
}

// MARK: - Preview

#Preview("New Card") {
    FlashcardView(
        flashcard: Flashcard(
            front: "What is the capital of Italy?",
            back: "Rome (Roma)",
            subject: "Geography",
            maestro: "Mrs. Smith"
        ),
        showRatingButtons: true
    )
    .frame(width: 600, height: 800)
}

#Preview("Reviewed Card") {
    FlashcardView(
        flashcard: Flashcard(
            front: "What is photosynthesis?",
            back: "The process by which plants convert light energy into chemical energy",
            subject: "Biology",
            maestro: "Dr. Johnson",
            stability: 15.5,
            difficulty: 5.2,
            reps: 8,
            lapses: 1,
            state: .review
        ),
        showRatingButtons: true
    )
    .frame(width: 600, height: 800)
}
