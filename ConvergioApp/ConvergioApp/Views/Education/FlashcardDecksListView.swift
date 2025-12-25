//
//  FlashcardDecksListView.swift
//  ConvergioApp
//
//  List view for flashcard decks with creation and management
//
//  Created by Roberto Daniele on 2025-12-25.
//

import SwiftUI

struct FlashcardDecksListView: View {
    @State private var decks: [FlashcardDeck] = []
    @State private var showCreateDeck = false
    @State private var selectedDeck: FlashcardDeck?

    var body: some View {
        NavigationStack {
            Group {
                if decks.isEmpty {
                    emptyStateView
                } else {
                    deckListView
                }
            }
            .navigationTitle("Flashcards")
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button(action: { showCreateDeck = true }) {
                        Label("Create Deck", systemImage: "plus")
                    }
                }
            }
            .sheet(isPresented: $showCreateDeck) {
                CreateDeckSheet(onSave: { deck in
                    decks.append(deck)
                    showCreateDeck = false
                })
            }
            .sheet(item: $selectedDeck) { deck in
                FlashcardDeckView(deck: deck)
            }
        }
        .onAppear {
            loadDecks()
        }
    }

    private var emptyStateView: some View {
        VStack(spacing: 20) {
            Image(systemName: "rectangle.stack")
                .font(.system(size: 60))
                .foregroundColor(.secondary)

            Text("No Flashcard Decks")
                .font(.title2)
                .fontWeight(.semibold)

            Text("Create your first deck to start studying")
                .foregroundColor(.secondary)

            Button(action: { showCreateDeck = true }) {
                Label("Create Deck", systemImage: "plus.circle.fill")
                    .font(.headline)
            }
            .buttonStyle(.borderedProminent)
        }
        .padding()
    }

    private var deckListView: some View {
        List(decks) { deck in
            DeckRowView(deck: deck)
                .contentShape(Rectangle())
                .onTapGesture {
                    selectedDeck = deck
                }
        }
    }

    private func loadDecks() {
        // Load decks from UserDefaults or storage
        if let data = UserDefaults.standard.data(forKey: "convergio.flashcard.decks"),
           let savedDecks = try? JSONDecoder().decode([FlashcardDeck].self, from: data) {
            decks = savedDecks
        }
    }

    private func saveDecks() {
        if let data = try? JSONEncoder().encode(decks) {
            UserDefaults.standard.set(data, forKey: "convergio.flashcard.decks")
        }
    }
}

// MARK: - Deck Row View

private struct DeckRowView: View {
    let deck: FlashcardDeck

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(deck.name)
                    .font(.headline)

                Text("\(deck.totalCards) cards • \(deck.dueCards.count) due")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            Spacer()

            if deck.dueCards.count > 0 {
                Text("\(deck.dueCards.count)")
                    .font(.caption)
                    .fontWeight(.bold)
                    .foregroundColor(.white)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(Color.blue)
                    .clipShape(Capsule())
            }

            Image(systemName: "chevron.right")
                .foregroundColor(.secondary)
        }
        .padding(.vertical, 8)
    }
}

// MARK: - Create Deck Sheet

private struct CreateDeckSheet: View {
    @Environment(\.dismiss) var dismiss
    @State private var name = ""
    @State private var subject = ""
    let onSave: (FlashcardDeck) -> Void

    var body: some View {
        NavigationStack {
            Form {
                Section {
                    TextField("Deck Name", text: $name)
                    TextField("Subject", text: $subject)
                }
            }
            .navigationTitle("New Deck")
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") { dismiss() }
                }
                ToolbarItem(placement: .confirmationAction) {
                    Button("Create") {
                        let deck = FlashcardDeck(
                            name: name,
                            subject: subject,
                            maestro: "Ali"
                        )
                        onSave(deck)
                    }
                    .disabled(name.isEmpty)
                }
            }
        }
        .frame(minWidth: 400, minHeight: 200)
    }
}

#Preview {
    FlashcardDecksListView()
}
