/**
 * CONVERGIO NATIVE - FlashcardDeckView
 *
 * Interactive flashcard review interface with swipe gestures.
 * Uses FSRS algorithm for optimal spaced repetition scheduling.
 *
 * Part of the Scuola 2026 Education Core (Tasks 1.3.2 - 1.3.4)
 *
 * Swipe Gestures:
 * - Left: Again (failed)
 * - Right: Good (recalled)
 * - Up: Easy (effortless recall)
 * - Down/Tap: Flip card
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

// MARK: - FlashcardDeckView

struct FlashcardDeckView: View {
    @StateObject private var fsrs = FSRSManager.shared
    @State private var currentCards: [Flashcard] = []
    @State private var currentIndex: Int = 0
    @State private var showAnswer: Bool = false
    @State private var cardOffset: CGSize = .zero
    @State private var cardRotation: Double = 0
    @State private var reviewStartTime: Date = Date()
    @State private var sessionComplete: Bool = false

    let deck: FlashcardDeck?

    var body: some View {
        VStack(spacing: 0) {
            // Header
            header

            // Card stack
            if sessionComplete {
                sessionCompleteView
            } else if currentCards.isEmpty {
                emptyStateView
            } else {
                cardStack
            }

            // Rating buttons (when answer shown)
            if showAnswer && !sessionComplete {
                ratingButtons
            }

            // Progress bar
            progressBar
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(nsColor: .windowBackgroundColor))
        .onAppear { loadCards() }
    }

    // MARK: - Header

    private var header: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(deck?.name ?? "All Cards")
                    .font(.title2.bold())

                Text("\(currentCards.count) cards remaining")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            Spacer()

            // Stats
            HStack(spacing: 16) {
                StatBadge(label: "Today", value: "\(fsrs.todayReviews)", color: .blue)
                StatBadge(label: "Due", value: "\(fsrs.dueCards.count)", color: .orange)
                StatBadge(label: "Streak", value: "\(fsrs.streak)", color: .green)
            }
        }
        .padding()
    }

    // MARK: - Card Stack

    private var cardStack: some View {
        ZStack {
            // Background cards
            ForEach(Array(currentCards.dropFirst().prefix(2).enumerated()), id: \.element.id) { index, card in
                FlashcardView(card: card, showAnswer: false)
                    .scaleEffect(1 - CGFloat(index + 1) * 0.05)
                    .offset(y: CGFloat(index + 1) * 10)
                    .opacity(0.7 - Double(index) * 0.2)
            }

            // Current card
            if let card = currentCards.first {
                FlashcardView(card: card, showAnswer: showAnswer)
                    .offset(cardOffset)
                    .rotationEffect(.degrees(cardRotation))
                    .gesture(dragGesture)
                    .gesture(tapGesture)
                    .overlay(swipeIndicator)
                    .animation(.spring(response: 0.3), value: cardOffset)
            }
        }
        .padding(.horizontal, 40)
        .padding(.vertical, 20)
    }

    // MARK: - Gestures

    private var dragGesture: some Gesture {
        DragGesture()
            .onChanged { value in
                cardOffset = value.translation
                cardRotation = Double(value.translation.width) / 20
            }
            .onEnded { value in
                handleSwipe(translation: value.translation, velocity: value.predictedEndTranslation)
            }
    }

    private var tapGesture: some Gesture {
        TapGesture()
            .onEnded {
                withAnimation(.spring(response: 0.4)) {
                    showAnswer.toggle()
                    if showAnswer {
                        reviewStartTime = Date()
                    }
                }
            }
    }

    private func handleSwipe(translation: CGSize, velocity: CGSize) {
        let threshold: CGFloat = 100
        let verticalThreshold: CGFloat = 80

        // Determine swipe direction
        if abs(translation.width) > threshold || abs(velocity.width) > 500 {
            // Horizontal swipe
            if translation.width > 0 {
                // Right = Good
                rateCard(.good)
            } else {
                // Left = Again
                rateCard(.again)
            }
        } else if translation.height < -verticalThreshold || velocity.height < -300 {
            // Swipe up = Easy
            rateCard(.easy)
        } else if translation.height > verticalThreshold {
            // Swipe down = Show answer
            withAnimation(.spring(response: 0.4)) {
                showAnswer = true
            }
            cardOffset = .zero
            cardRotation = 0
        } else {
            // Reset
            withAnimation(.spring(response: 0.3)) {
                cardOffset = .zero
                cardRotation = 0
            }
        }
    }

    // MARK: - Swipe Indicator

    @ViewBuilder
    private var swipeIndicator: some View {
        ZStack {
            if cardOffset.width > 50 {
                Text("GOOD")
                    .font(.title.bold())
                    .foregroundColor(.green)
                    .padding()
                    .background(Color.green.opacity(0.2))
                    .cornerRadius(10)
                    .offset(x: -80)
            }

            if cardOffset.width < -50 {
                Text("AGAIN")
                    .font(.title.bold())
                    .foregroundColor(.red)
                    .padding()
                    .background(Color.red.opacity(0.2))
                    .cornerRadius(10)
                    .offset(x: 80)
            }

            if cardOffset.height < -50 {
                Text("EASY")
                    .font(.title.bold())
                    .foregroundColor(.blue)
                    .padding()
                    .background(Color.blue.opacity(0.2))
                    .cornerRadius(10)
                    .offset(y: 80)
            }
        }
        .opacity(min(1, max(abs(cardOffset.width), abs(cardOffset.height)) / 100))
    }

    // MARK: - Rating Buttons

    private var ratingButtons: some View {
        HStack(spacing: 16) {
            ForEach(FSRSRating.allCases, id: \.rawValue) { rating in
                RatingButton(rating: rating) {
                    rateCard(rating)
                }
            }
        }
        .padding()
        .transition(.move(edge: .bottom).combined(with: .opacity))
    }

    private func rateCard(_ rating: FSRSRating) {
        guard let card = currentCards.first else { return }

        let duration = Date().timeIntervalSince(reviewStartTime) * 1000 // ms
        let _ = fsrs.reviewCard(card, rating: rating, duration: duration)

        // Animate card out
        withAnimation(.easeOut(duration: 0.3)) {
            switch rating {
            case .again:
                cardOffset = CGSize(width: -500, height: 0)
            case .hard:
                cardOffset = CGSize(width: -300, height: 100)
            case .good:
                cardOffset = CGSize(width: 500, height: 0)
            case .easy:
                cardOffset = CGSize(width: 0, height: -500)
            }
        }

        // Load next card
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
            currentCards.removeFirst()
            cardOffset = .zero
            cardRotation = 0
            showAnswer = false

            if currentCards.isEmpty {
                sessionComplete = true
            }
        }
    }

    // MARK: - Progress Bar

    private var progressBar: some View {
        GeometryReader { geometry in
            let total = currentCards.count + fsrs.todayReviews
            let progress = total > 0 ? CGFloat(fsrs.todayReviews) / CGFloat(total) : 0

            ZStack(alignment: .leading) {
                Rectangle()
                    .fill(Color.gray.opacity(0.2))

                Rectangle()
                    .fill(Color.green)
                    .frame(width: geometry.size.width * progress)
            }
        }
        .frame(height: 4)
    }

    // MARK: - Empty State

    private var emptyStateView: some View {
        VStack(spacing: 20) {
            Image(systemName: "checkmark.circle.fill")
                .font(.system(size: 80))
                .foregroundColor(.green)

            Text("No cards due!")
                .font(.title2.bold())

            Text("Great job! Come back later for more reviews.")
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .padding()
    }

    // MARK: - Session Complete

    private var sessionCompleteView: some View {
        VStack(spacing: 24) {
            Image(systemName: "party.popper.fill")
                .font(.system(size: 80))
                .foregroundColor(.yellow)

            Text("Session Complete!")
                .font(.title.bold())

            VStack(spacing: 8) {
                Text("Cards reviewed: \(fsrs.todayReviews)")
                Text("Keep up the great work!")
            }
            .foregroundColor(.secondary)

            Button("Start New Session") {
                loadCards()
                sessionComplete = false
            }
            .buttonStyle(.borderedProminent)
        }
        .padding()
    }

    // MARK: - Data Loading

    private func loadCards() {
        currentCards = fsrs.getDueCards(for: deck)
        currentIndex = 0
        showAnswer = false
        sessionComplete = false
    }
}

// MARK: - FlashcardView

struct FlashcardView: View {
    let card: Flashcard
    let showAnswer: Bool

    @State private var flipped = false

    var body: some View {
        ZStack {
            // Front
            CardFace(
                content: card.front,
                subtitle: "Question",
                color: .blue
            )
            .opacity(showAnswer ? 0 : 1)
            .rotation3DEffect(.degrees(showAnswer ? 180 : 0), axis: (x: 0, y: 1, z: 0))

            // Back
            CardFace(
                content: card.back,
                subtitle: "Answer",
                color: .green
            )
            .opacity(showAnswer ? 1 : 0)
            .rotation3DEffect(.degrees(showAnswer ? 0 : -180), axis: (x: 0, y: 1, z: 0))
        }
        .frame(maxWidth: 500, maxHeight: 400)
        .animation(.spring(response: 0.4), value: showAnswer)
    }
}

// MARK: - CardFace

struct CardFace: View {
    let content: String
    let subtitle: String
    let color: Color

    var body: some View {
        VStack(spacing: 16) {
            Text(subtitle.uppercased())
                .font(.caption.bold())
                .foregroundColor(color)
                .padding(.horizontal, 12)
                .padding(.vertical, 4)
                .background(color.opacity(0.1))
                .cornerRadius(4)

            Spacer()

            Text(content)
                .font(.title2)
                .multilineTextAlignment(.center)
                .padding()

            Spacer()

            Text("Tap to flip")
                .font(.caption)
                .foregroundColor(.secondary)
        }
        .padding(24)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(
            RoundedRectangle(cornerRadius: 20)
                .fill(Color(nsColor: .controlBackgroundColor))
                .shadow(color: color.opacity(0.2), radius: 10, y: 5)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 20)
                .stroke(color.opacity(0.3), lineWidth: 2)
        )
    }
}

// MARK: - Rating Button

struct RatingButton: View {
    let rating: FSRSRating
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            VStack(spacing: 4) {
                Text(rating.emoji)
                    .font(.title2)

                Text(rating.displayName)
                    .font(.caption.bold())
            }
            .frame(width: 70, height: 60)
            .background(ratingColor.opacity(0.1))
            .cornerRadius(12)
            .overlay(
                RoundedRectangle(cornerRadius: 12)
                    .stroke(ratingColor, lineWidth: 2)
            )
        }
        .buttonStyle(.plain)
        .keyboardShortcut(keyboardKey, modifiers: [])
    }

    private var ratingColor: Color {
        switch rating {
        case .again: return .red
        case .hard: return .orange
        case .good: return .green
        case .easy: return .blue
        }
    }

    private var keyboardKey: KeyEquivalent {
        switch rating {
        case .again: return "1"
        case .hard: return "2"
        case .good: return "3"
        case .easy: return "4"
        }
    }
}

// MARK: - Stat Badge

struct StatBadge: View {
    let label: String
    let value: String
    let color: Color

    var body: some View {
        VStack(spacing: 2) {
            Text(value)
                .font(.headline.bold())
                .foregroundColor(color)

            Text(label)
                .font(.caption2)
                .foregroundColor(.secondary)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 6)
        .background(color.opacity(0.1))
        .cornerRadius(8)
    }
}

// MARK: - Deck List View

struct FlashcardDecksListView: View {
    @StateObject private var fsrs = FSRSManager.shared
    @State private var showingCreateDeck = false
    @State private var selectedDeck: FlashcardDeck?

    var body: some View {
        NavigationSplitView {
            List(selection: $selectedDeck) {
                Section("Your Decks") {
                    ForEach(fsrs.decks) { deck in
                        NavigationLink(value: deck) {
                            DeckRow(deck: deck)
                        }
                    }
                }

                Section {
                    Button {
                        showingCreateDeck = true
                    } label: {
                        Label("Create New Deck", systemImage: "plus.circle.fill")
                    }
                }
            }
            .navigationTitle("Flashcards")
            .toolbar {
                ToolbarItem {
                    Button {
                        selectedDeck = nil
                    } label: {
                        Label("Study All", systemImage: "rectangle.stack.fill")
                    }
                }
            }
        } detail: {
            FlashcardDeckView(deck: selectedDeck)
        }
        .sheet(isPresented: $showingCreateDeck) {
            CreateDeckSheet { name, subject, maestroId in
                let _ = fsrs.createDeck(name: name, subject: subject, maestroId: maestroId)
            }
        }
    }
}

// MARK: - Deck Row

struct DeckRow: View {
    let deck: FlashcardDeck

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(deck.name)
                    .font(.headline)

                Text(deck.subject)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            Spacer()

            Image(systemName: "chevron.right")
                .foregroundColor(.secondary)
        }
        .padding(.vertical, 4)
    }
}

// MARK: - Create Deck Sheet

struct CreateDeckSheet: View {
    @Environment(\.dismiss) private var dismiss
    @State private var name = ""
    @State private var subject = ""
    @State private var maestroId: String?

    let onCreate: (String, String, String?) -> Void

    var body: some View {
        NavigationStack {
            Form {
                TextField("Deck Name", text: $name)
                TextField("Subject", text: $subject)
            }
            .frame(width: 400, height: 200)
            .navigationTitle("Create Deck")
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") { dismiss() }
                }
                ToolbarItem(placement: .confirmationAction) {
                    Button("Create") {
                        onCreate(name, subject, maestroId)
                        dismiss()
                    }
                    .disabled(name.isEmpty || subject.isEmpty)
                }
            }
        }
    }
}

// MARK: - Previews

#if DEBUG
struct FlashcardDeckView_Previews: PreviewProvider {
    static var previews: some View {
        FlashcardDeckView(deck: nil)
            .frame(width: 800, height: 600)
    }
}
#endif
