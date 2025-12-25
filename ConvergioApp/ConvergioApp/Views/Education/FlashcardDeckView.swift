//
//  FlashcardDeckView.swift
//  ConvergioApp
//
//  Flashcard deck with swipe gestures and FSRS scheduling
//  Swipe directions: Left = Again, Down = Hard, Right = Good, Up = Easy
//
//  Created by Roberto Daniele on 2025-12-24.
//

import SwiftUI

struct FlashcardDeckView: View {
    @StateObject private var viewModel: FlashcardDeckViewModel
    @State private var showSessionSummary = false
    @State private var currentCardOffset: CGSize = .zero
    @State private var currentCardRotation: Double = 0
    @State private var isFlipped = false

    private let cardWidth: CGFloat = 400
    private let cardHeight: CGFloat = 500
    private let swipeThreshold: CGFloat = 100

    init(deck: FlashcardDeck) {
        _viewModel = StateObject(wrappedValue: FlashcardDeckViewModel(deck: deck))
    }

    var body: some View {
        ZStack {
            // Background
            Color.black.opacity(0.02)
                .ignoresSafeArea()

            VStack(spacing: 0) {
                // Header
                HeaderView(
                    deckName: viewModel.deck.name,
                    remaining: viewModel.remainingCards,
                    total: viewModel.totalCards,
                    onClose: {
                        showSessionSummary = true
                    }
                )
                .padding()

                Spacer()

                // Card Stack
                ZStack {
                    // Background cards (for stack effect)
                    ForEach(0..<min(3, viewModel.remainingCards), id: \.self) { index in
                        if index > 0 {
                            RoundedRectangle(cornerRadius: 20)
                                .fill(Color.secondary.opacity(0.1))
                                .frame(width: cardWidth, height: cardHeight)
                                .offset(y: CGFloat(index * 10))
                                .scaleEffect(1 - CGFloat(index) * 0.05)
                        }
                    }

                    // Current card
                    if let currentCard = viewModel.currentCard {
                        SwipeableCardView(
                            flashcard: currentCard,
                            isFlipped: $isFlipped,
                            offset: $currentCardOffset,
                            rotation: $currentCardRotation,
                            onSwipe: handleSwipe
                        )
                        .offset(currentCardOffset)
                        .rotationEffect(.degrees(currentCardRotation))
                    } else {
                        // All cards completed
                        CompletedView(onReview: {
                            viewModel.resetSession()
                            isFlipped = false
                        })
                    }
                }

                Spacer()

                // Swipe indicators
                if viewModel.currentCard != nil {
                    SwipeIndicatorsView()
                        .padding(.bottom, 20)
                }

                // Progress bar
                ProgressBarView(
                    completed: viewModel.totalCards - viewModel.remainingCards,
                    total: viewModel.totalCards
                )
                .padding()
            }

            // Swipe direction indicator
            if currentCardOffset != .zero {
                SwipeDirectionOverlay(offset: currentCardOffset)
            }
        }
        .sheet(isPresented: $showSessionSummary) {
            SessionSummaryView(
                session: viewModel.currentSession,
                onDismiss: { showSessionSummary = false },
                onContinue: {
                    showSessionSummary = false
                }
            )
        }
    }

    private func handleSwipe(direction: SwipeDirection) {
        guard viewModel.currentCard != nil else { return }

        // Determine rating from swipe direction
        let rating: Rating
        switch direction {
        case .left: rating = .again
        case .down: rating = .hard
        case .right: rating = .good
        case .up: rating = .easy
        }

        // Animate card away
        withAnimation(.spring(response: 0.5, dampingFraction: 0.7)) {
            switch direction {
            case .left:
                currentCardOffset = CGSize(width: -500, height: 0)
                currentCardRotation = -15
            case .down:
                currentCardOffset = CGSize(width: 0, height: 500)
                currentCardRotation = 0
            case .right:
                currentCardOffset = CGSize(width: 500, height: 0)
                currentCardRotation = 15
            case .up:
                currentCardOffset = CGSize(width: 0, height: -500)
                currentCardRotation = 0
            }
        }

        // Process rating and move to next card
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
            viewModel.rateCurrentCard(rating: rating)
            isFlipped = false

            // Reset offset and rotation
            currentCardOffset = .zero
            currentCardRotation = 0
        }
    }
}

// MARK: - Swipeable Card View

private struct SwipeableCardView: View {
    let flashcard: Flashcard
    @Binding var isFlipped: Bool
    @Binding var offset: CGSize
    @Binding var rotation: Double

    let onSwipe: (SwipeDirection) -> Void

    @State private var dragOffset: CGSize = .zero
    private let swipeThreshold: CGFloat = 100

    var body: some View {
        FlashcardView(flashcard: flashcard, showRatingButtons: false)
            .gesture(
                DragGesture()
                    .onChanged { value in
                        dragOffset = value.translation
                        offset = value.translation
                        rotation = Double(value.translation.width / 20)
                    }
                    .onEnded { value in
                        let direction = determineSwipeDirection(translation: value.translation)
                        if let direction = direction {
                            onSwipe(direction)
                        } else {
                            // Snap back
                            withAnimation(.spring(response: 0.3, dampingFraction: 0.7)) {
                                offset = .zero
                                rotation = 0
                            }
                        }
                        dragOffset = .zero
                    }
            )
            .onTapGesture {
                withAnimation(.spring(response: 0.6, dampingFraction: 0.8)) {
                    isFlipped.toggle()
                }
            }
    }

    private func determineSwipeDirection(translation: CGSize) -> SwipeDirection? {
        let horizontal = abs(translation.width)
        let vertical = abs(translation.height)

        if horizontal > vertical {
            // Horizontal swipe
            if horizontal > swipeThreshold {
                return translation.width > 0 ? .right : .left
            }
        } else {
            // Vertical swipe
            if vertical > swipeThreshold {
                return translation.height > 0 ? .down : .up
            }
        }

        return nil
    }
}

enum SwipeDirection {
    case left, right, up, down
}

// MARK: - Header View

private struct HeaderView: View {
    let deckName: String
    let remaining: Int
    let total: Int
    let onClose: () -> Void

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(deckName)
                    .font(.title2)
                    .fontWeight(.bold)
                Text("\(remaining) of \(total) cards remaining")
                    .font(.subheadline)
                    .foregroundColor(.secondary)
            }

            Spacer()

            Button(action: onClose) {
                Image(systemName: "xmark.circle.fill")
                    .font(.title2)
                    .foregroundColor(.secondary)
            }
            .buttonStyle(PlainButtonStyle())
        }
    }
}

// MARK: - Swipe Indicators

private struct SwipeIndicatorsView: View {
    var body: some View {
        HStack(spacing: 40) {
            SwipeIndicator(
                direction: "←",
                label: "Again",
                color: .red,
                icon: "xmark.circle.fill"
            )

            SwipeIndicator(
                direction: "↓",
                label: "Hard",
                color: .orange,
                icon: "minus.circle.fill"
            )

            SwipeIndicator(
                direction: "→",
                label: "Good",
                color: .green,
                icon: "checkmark.circle.fill"
            )

            SwipeIndicator(
                direction: "↑",
                label: "Easy",
                color: .blue,
                icon: "star.circle.fill"
            )
        }
        .padding()
        .background(Color.secondary.opacity(0.05))
        .cornerRadius(16)
    }
}

private struct SwipeIndicator: View {
    let direction: String
    let label: String
    let color: Color
    let icon: String

    var body: some View {
        VStack(spacing: 4) {
            Image(systemName: icon)
                .font(.title3)
                .foregroundColor(color)
            Text(direction)
                .font(.title)
                .foregroundColor(color)
            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
        }
        .frame(width: 70)
    }
}

// MARK: - Swipe Direction Overlay

private struct SwipeDirectionOverlay: View {
    let offset: CGSize

    private var activeRating: Rating? {
        let horizontal = abs(offset.width)
        let vertical = abs(offset.height)

        if horizontal > vertical && horizontal > 50 {
            return offset.width > 0 ? .good : .again
        } else if vertical > 50 {
            return offset.height > 0 ? .hard : .easy
        }
        return nil
    }

    var body: some View {
        if let rating = activeRating {
            VStack {
                Image(systemName: rating.icon)
                    .font(.system(size: 60))
                    .foregroundColor(rating.color)
                Text(rating.rawValue)
                    .font(.title)
                    .fontWeight(.bold)
                    .foregroundColor(rating.color)
            }
            .padding(30)
            .background(rating.color.opacity(0.2))
            .cornerRadius(20)
            .shadow(color: rating.color.opacity(0.3), radius: 10)
        }
    }
}

// MARK: - Progress Bar

private struct ProgressBarView: View {
    let completed: Int
    let total: Int

    private var progress: Double {
        guard total > 0 else { return 0 }
        return Double(completed) / Double(total)
    }

    var body: some View {
        VStack(spacing: 8) {
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    RoundedRectangle(cornerRadius: 4)
                        .fill(Color.secondary.opacity(0.2))

                    RoundedRectangle(cornerRadius: 4)
                        .fill(
                            LinearGradient(
                                colors: [Color.blue, Color.purple],
                                startPoint: .leading,
                                endPoint: .trailing
                            )
                        )
                        .frame(width: geometry.size.width * progress)
                }
            }
            .frame(height: 8)

            Text("\(completed) / \(total)")
                .font(.caption)
                .foregroundColor(.secondary)
        }
    }
}

// MARK: - Completed View

private struct CompletedView: View {
    let onReview: () -> Void

    var body: some View {
        VStack(spacing: 30) {
            Image(systemName: "checkmark.circle.fill")
                .font(.system(size: 80))
                .foregroundColor(.green)

            VStack(spacing: 10) {
                Text("Session Complete!")
                    .font(.title)
                    .fontWeight(.bold)
                Text("Great job! All cards reviewed.")
                    .font(.subheadline)
                    .foregroundColor(.secondary)
            }

            Button(action: onReview) {
                Text("Review Again")
                    .font(.headline)
                    .foregroundColor(.white)
                    .padding(.horizontal, 30)
                    .padding(.vertical, 12)
                    .background(
                        LinearGradient(
                            colors: [Color.blue, Color.purple],
                            startPoint: .leading,
                            endPoint: .trailing
                        )
                    )
                    .cornerRadius(12)
            }
            .buttonStyle(PlainButtonStyle())
        }
        .frame(width: 400, height: 500)
    }
}

// MARK: - Session Summary

private struct SessionSummaryView: View {
    let session: ReviewSession
    let onDismiss: () -> Void
    let onContinue: () -> Void

    var body: some View {
        VStack(spacing: 30) {
            // Header
            HStack {
                Text("Session Summary")
                    .font(.title)
                    .fontWeight(.bold)
                Spacer()
                Button(action: onDismiss) {
                    Image(systemName: "xmark.circle.fill")
                        .font(.title2)
                        .foregroundColor(.secondary)
                }
                .buttonStyle(PlainButtonStyle())
            }

            Divider()

            // Statistics
            VStack(spacing: 20) {
                FlashcardStatRow(
                    icon: "clock",
                    label: "Duration",
                    value: formatDuration(session.duration)
                )
                FlashcardStatRow(
                    icon: "rectangle.stack",
                    label: "Cards Reviewed",
                    value: "\(session.cardsReviewed)"
                )
                FlashcardStatRow(
                    icon: "gauge.medium",
                    label: "Success Rate",
                    value: String(format: "%.0f%%", session.successRate * 100)
                )
                FlashcardStatRow(
                    icon: "timer",
                    label: "Avg. Time/Card",
                    value: String(format: "%.1fs", session.averageTimePerCard)
                )
            }

            // Rating breakdown
            VStack(alignment: .leading, spacing: 12) {
                Text("Rating Breakdown")
                    .font(.headline)
                    .padding(.bottom, 8)

                RatingBar(rating: .again, count: session.againCount, total: session.cardsReviewed)
                RatingBar(rating: .hard, count: session.hardCount, total: session.cardsReviewed)
                RatingBar(rating: .good, count: session.goodCount, total: session.cardsReviewed)
                RatingBar(rating: .easy, count: session.easyCount, total: session.cardsReviewed)
            }
            .padding()
            .background(Color.secondary.opacity(0.05))
            .cornerRadius(12)

            Spacer()

            // Actions
            HStack(spacing: 16) {
                Button(action: onDismiss) {
                    Text("Close")
                        .font(.headline)
                        .foregroundColor(.primary)
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 12)
                        .background(Color.secondary.opacity(0.2))
                        .cornerRadius(12)
                }

                Button(action: onContinue) {
                    Text("Continue")
                        .font(.headline)
                        .foregroundColor(.white)
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 12)
                        .background(
                            LinearGradient(
                                colors: [Color.blue, Color.purple],
                                startPoint: .leading,
                                endPoint: .trailing
                            )
                        )
                        .cornerRadius(12)
                }
            }
            .buttonStyle(PlainButtonStyle())
        }
        .padding(30)
        .frame(width: 500, height: 600)
    }

    private func formatDuration(_ duration: TimeInterval) -> String {
        let minutes = Int(duration) / 60
        let seconds = Int(duration) % 60
        return String(format: "%d:%02d", minutes, seconds)
    }
}

private struct FlashcardStatRow: View {
    let icon: String
    let label: String
    let value: String

    var body: some View {
        HStack {
            Image(systemName: icon)
                .foregroundColor(.blue)
                .frame(width: 30)
            Text(label)
                .foregroundColor(.secondary)
            Spacer()
            Text(value)
                .fontWeight(.semibold)
        }
    }
}

private struct RatingBar: View {
    let rating: Rating
    let count: Int
    let total: Int

    private var percentage: Double {
        guard total > 0 else { return 0 }
        return Double(count) / Double(total)
    }

    var body: some View {
        HStack {
            HStack(spacing: 8) {
                Image(systemName: rating.icon)
                    .foregroundColor(rating.color)
                Text(rating.rawValue)
                    .frame(width: 60, alignment: .leading)
            }
            .frame(width: 120)

            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    RoundedRectangle(cornerRadius: 4)
                        .fill(Color.secondary.opacity(0.2))

                    RoundedRectangle(cornerRadius: 4)
                        .fill(rating.color)
                        .frame(width: geometry.size.width * percentage)
                }
            }
            .frame(height: 8)

            Text("\(count)")
                .font(.caption)
                .foregroundColor(.secondary)
                .frame(width: 40, alignment: .trailing)
        }
    }
}

// MARK: - View Model

class FlashcardDeckViewModel: ObservableObject {
    @Published var deck: FlashcardDeck
    @Published var currentSession: ReviewSession
    @Published private(set) var currentCard: Flashcard?
    @Published private(set) var remainingCards: Int = 0

    private let fsrsManager = FSRSManager()
    private var dueCards: [Flashcard] = []
    private var currentIndex = 0

    var totalCards: Int {
        dueCards.count
    }

    init(deck: FlashcardDeck) {
        self.deck = deck
        self.currentSession = ReviewSession(deckId: deck.id)
        loadDueCards()
    }

    private func loadDueCards() {
        dueCards = fsrsManager.getDueCards(deck: deck)
        if dueCards.isEmpty {
            dueCards = fsrsManager.getNewCards(deck: deck, limit: 20)
        }
        remainingCards = dueCards.count
        currentCard = dueCards.first
    }

    func rateCurrentCard(rating: Rating) {
        guard let card = currentCard else { return }

        // Process with FSRS
        let info = fsrsManager.review(card: card, rating: rating)

        // Update deck
        if let index = deck.cards.firstIndex(where: { $0.id == card.id }) {
            deck.cards[index] = info.card
        }

        // Update session stats
        currentSession.cardsReviewed += 1
        switch rating {
        case .again: currentSession.againCount += 1
        case .hard: currentSession.hardCount += 1
        case .good: currentSession.goodCount += 1
        case .easy: currentSession.easyCount += 1
        }

        // Move to next card
        currentIndex += 1
        if currentIndex < dueCards.count {
            currentCard = dueCards[currentIndex]
            remainingCards = dueCards.count - currentIndex
        } else {
            currentCard = nil
            remainingCards = 0
            currentSession.endTime = Date()
        }
    }

    func resetSession() {
        currentIndex = 0
        loadDueCards()
        currentSession = ReviewSession(deckId: deck.id)
    }
}

// MARK: - Preview

#Preview("Active Deck") {
    FlashcardDeckView(
        deck: FlashcardDeck(
            name: "Italian Vocabulary",
            subject: "Italian",
            maestro: "Mrs. Rossi",
            cards: [
                Flashcard(
                    front: "Come stai?",
                    back: "How are you?",
                    subject: "Italian",
                    maestro: "Mrs. Rossi"
                ),
                Flashcard(
                    front: "Buongiorno",
                    back: "Good morning",
                    subject: "Italian",
                    maestro: "Mrs. Rossi"
                ),
                Flashcard(
                    front: "Grazie",
                    back: "Thank you",
                    subject: "Italian",
                    maestro: "Mrs. Rossi"
                )
            ]
        )
    )
    .frame(width: 800, height: 900)
}
