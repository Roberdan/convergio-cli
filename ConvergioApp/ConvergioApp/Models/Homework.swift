/**
 * CONVERGIO NATIVE - Homework Model
 *
 * Models for the Homework Assistant that guides students using the
 * maieutic/Socratic method without giving direct answers.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import SwiftUI

// MARK: - Homework Type

enum HomeworkType: String, CaseIterable, Identifiable {
    case math = "Matematica"
    case essay = "Tema/Scrittura"
    case science = "Scienze"
    case language = "Lingue"
    case history = "Storia"
    case geography = "Geografia"
    case philosophy = "Filosofia"
    case other = "Altro"

    var id: String { rawValue }

    var color: Color {
        switch self {
        case .math: return .blue
        case .essay: return .purple
        case .science: return .green
        case .language: return .orange
        case .history: return .brown
        case .geography: return .cyan
        case .philosophy: return .gray
        case .other: return .pink
        }
    }

    var icon: String {
        switch self {
        case .math: return "function"
        case .essay: return "pencil.and.outline"
        case .science: return "atom"
        case .language: return "character.book.closed"
        case .history: return "book.closed"
        case .geography: return "globe"
        case .philosophy: return "brain.head.profile"
        case .other: return "questionmark.circle"
        }
    }

    /// Suggested prompts for this homework type
    var maieuticPrompts: [String] {
        switch self {
        case .math:
            return [
                "Cosa succede se provi a semplificare questa parte?",
                "Quali proprietà matematiche conosci che potrebbero aiutarti qui?",
                "Hai provato a disegnare un diagramma del problema?",
                "Cosa sai già su questo tipo di problema?",
                "Quale sarebbe il primo passo logico?"
            ]
        case .essay:
            return [
                "Qual è l'idea principale che vuoi comunicare?",
                "Hai pensato a esempi concreti per supportare questa tesi?",
                "Come potresti strutturare meglio questo ragionamento?",
                "Cosa vuoi che il lettore capisca da questo paragrafo?",
                "Come si collega questa parte al tuo argomento principale?"
            ]
        case .science:
            return [
                "Cosa osservi in questo fenomeno?",
                "Quale legge scientifica potrebbe spiegare questo?",
                "Hai provato a formulare un'ipotesi?",
                "Come potresti verificare questa idea?",
                "Cosa succederebbe se cambiassi questo parametro?"
            ]
        case .language:
            return [
                "Quale tempo verbale è più appropriato qui?",
                "Come suona questa frase se la leggi ad alta voce?",
                "Conosci un sinonimo più preciso per questa parola?",
                "Questa costruzione segue le regole grammaticali?",
                "Come diresti la stessa cosa in modo più naturale?"
            ]
        case .history:
            return [
                "Quali erano le cause di questo evento storico?",
                "Come si collega questo fatto al contesto dell'epoca?",
                "Quali conseguenze ha avuto questo evento?",
                "Cosa stavano pensando le persone in quel periodo?",
                "Come possiamo confrontare questa situazione con altre simili?"
            ]
        case .geography:
            return [
                "Dove si trova esattamente questo luogo?",
                "Quali caratteristiche geografiche noti?",
                "Come il clima influenza questa regione?",
                "Quali sono le risorse naturali disponibili?",
                "Come la geografia ha influenzato la storia di questa area?"
            ]
        case .philosophy:
            return [
                "Qual è la tesi principale di questo filosofo?",
                "Come potresti argomentare contro questa posizione?",
                "Quali presupposti ci sono in questo ragionamento?",
                "Cosa succederebbe se applicassimo questa idea alla vita reale?",
                "Come si collega questo pensiero ad altre filosofie?"
            ]
        case .other:
            return [
                "Proviamo insieme passo per passo",
                "Cosa hai capito finora?",
                "Quale parte ti sembra più difficile?",
                "Hai già affrontato qualcosa di simile?",
                "Come potresti dividere questo problema in parti più piccole?"
            ]
        }
    }
}

// MARK: - Homework Step

struct HomeworkStep: Identifiable, Hashable {
    let id: UUID
    let instruction: String
    let hints: [String]  // Progressive hints: small -> bigger -> example
    var isCompleted: Bool
    var studentNotes: String

    init(
        instruction: String,
        hints: [String] = [],
        isCompleted: Bool = false,
        studentNotes: String = ""
    ) {
        self.id = UUID()
        self.instruction = instruction
        self.hints = hints
        self.isCompleted = isCompleted
        self.studentNotes = studentNotes
    }

    /// Get the next hint (progressive disclosure)
    func nextHint(currentHintIndex: Int) -> String? {
        guard currentHintIndex < hints.count else { return nil }
        return hints[currentHintIndex]
    }

    /// Whether there are more hints available
    func hasMoreHints(currentHintIndex: Int) -> Bool {
        currentHintIndex < hints.count
    }
}

// MARK: - Homework

struct Homework: Identifiable, Hashable {
    let id: UUID
    var subject: HomeworkType
    var problem: String
    var steps: [HomeworkStep]
    var currentStepIndex: Int
    var completed: Bool
    var startedAt: Date
    var completedAt: Date?
    var imageData: Data?  // For photo-based homework

    init(
        subject: HomeworkType,
        problem: String,
        steps: [HomeworkStep] = [],
        currentStepIndex: Int = 0,
        completed: Bool = false,
        startedAt: Date = Date(),
        completedAt: Date? = nil,
        imageData: Data? = nil
    ) {
        self.id = UUID()
        self.subject = subject
        self.problem = problem
        self.steps = steps
        self.currentStepIndex = currentStepIndex
        self.completed = completed
        self.startedAt = startedAt
        self.completedAt = completedAt
        self.imageData = imageData
    }

    /// Current step being worked on
    var currentStep: HomeworkStep? {
        guard currentStepIndex < steps.count else { return nil }
        return steps[currentStepIndex]
    }

    /// Progress percentage
    var progressPercentage: Double {
        guard !steps.isEmpty else { return 0 }
        let completedSteps = steps.filter { $0.isCompleted }.count
        return Double(completedSteps) / Double(steps.count)
    }

    /// Total time spent
    var timeSpent: TimeInterval {
        if let completedAt = completedAt {
            return completedAt.timeIntervalSince(startedAt)
        }
        return Date().timeIntervalSince(startedAt)
    }

    /// Formatted time spent
    var formattedTimeSpent: String {
        let minutes = Int(timeSpent) / 60
        let seconds = Int(timeSpent) % 60
        return String(format: "%d:%02d", minutes, seconds)
    }

    /// Mark current step as completed and move to next
    mutating func completeCurrentStep() {
        guard currentStepIndex < steps.count else { return }
        steps[currentStepIndex].isCompleted = true

        if currentStepIndex < steps.count - 1 {
            currentStepIndex += 1
        } else {
            completed = true
            completedAt = Date()
        }
    }

    /// Add a new step dynamically
    mutating func addStep(_ step: HomeworkStep) {
        steps.append(step)
    }

    /// Update student notes for current step
    mutating func updateCurrentStepNotes(_ notes: String) {
        guard currentStepIndex < steps.count else { return }
        steps[currentStepIndex].studentNotes = notes
    }
}

// MARK: - Preview Helpers

extension Homework {
    static let preview = Homework(
        subject: .math,
        problem: "Risolvi l'equazione: 2x + 5 = 15",
        steps: [
            HomeworkStep(
                instruction: "Prima, identifica cosa stai cercando. Qual è l'incognita in questa equazione?",
                hints: [
                    "Guarda le lettere nell'equazione",
                    "L'incognita è la variabile che non conosci",
                    "È la 'x' - rappresenta il numero che stai cercando"
                ]
            ),
            HomeworkStep(
                instruction: "Ora, pensa a come isolare l'incognita. Cosa c'è insieme alla x?",
                hints: [
                    "Guarda il lato sinistro dell'equazione",
                    "C'è un +5 che 'disturba' la x",
                    "Prova a togliere il +5 da entrambi i lati"
                ]
            ),
            HomeworkStep(
                instruction: "Ottimo! Ora hai 2x = 10. Come puoi ottenere solo x?",
                hints: [
                    "La x è moltiplicata per 2",
                    "L'operazione inversa della moltiplicazione è...",
                    "Dividi entrambi i lati per 2"
                ]
            ),
            HomeworkStep(
                instruction: "Verifica la tua soluzione! Come puoi essere sicuro che x = 5 sia corretto?",
                hints: [
                    "Sostituisci il valore trovato nell'equazione originale",
                    "Calcola: 2(5) + 5 = ?",
                    "Se ottieni 15, la soluzione è corretta!"
                ],
                isCompleted: false
            )
        ],
        currentStepIndex: 0
    )

    static let previewCompleted = Homework(
        subject: .essay,
        problem: "Scrivi un tema sulla libertà",
        steps: [
            HomeworkStep(
                instruction: "Definisci cosa significa 'libertà' per te",
                isCompleted: true,
                studentNotes: "La libertà è poter scegliere..."
            ),
            HomeworkStep(
                instruction: "Trova esempi concreti dalla vita reale",
                isCompleted: true,
                studentNotes: "Libertà di parola, movimento..."
            )
        ],
        currentStepIndex: 1,
        completed: true,
        completedAt: Date()
    )
}
