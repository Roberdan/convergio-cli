/**
 * CONVERGIO NATIVE - Homework ViewModel
 *
 * Manages homework assistance using the Socratic/maieutic method.
 * CRITICAL: Never gives final answers, only guides through steps.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

@MainActor
final class HomeworkViewModel: ObservableObject {
    // MARK: - Published Properties

    @Published var currentHomework: Homework?
    @Published var homeworkHistory: [Homework] = []
    @Published var isProcessing = false
    @Published var error: Error?
    @Published var currentHintIndex = 0
    @Published var showCelebration = false
    @Published var currentQuestion = ""

    // MARK: - Private Properties

    private let orchestrator: Orchestrator

    // MARK: - Anti-Cheating Rules

    /// System prompt for the AI that enforces maieutic method
    private let maieuticSystemPrompt = """
    You are a Socratic tutor helping a student with homework. Your role is to GUIDE, not to SOLVE.

    CRITICAL RULES (NEVER BREAK THESE):
    1. NEVER give the final answer or solution
    2. ALWAYS ask guiding questions
    3. Use the maieutic method - help the student discover the answer themselves
    4. If student asks "what's the answer?", redirect to methodology
    5. Break problems into small, manageable steps
    6. Celebrate the thinking process, not just results
    7. Be patient, encouraging, and positive

    RESPONSE FORMAT:
    - For each step, provide an instruction (what to think about)
    - Provide 3 progressive hints:
      1. Small hint (gentle nudge)
      2. Bigger hint (more specific guidance)
      3. Example hint (show similar case, but NOT the answer)

    ITALIAN-FRIENDLY PHRASES:
    - "Proviamo insieme passo per passo"
    - "Cosa succede se...?"
    - "Hai provato a...?"
    - "Ottimo ragionamento!"
    - "Ci stai arrivando!"
    - "Che bella intuizione!"

    TONE: Warm, encouraging, patient, never condescending.
    """

    // MARK: - Initialization

    init(orchestrator: Orchestrator = .shared) {
        self.orchestrator = orchestrator
    }

    // MARK: - Start New Homework

    /// Analyze a homework problem and create guided steps
    func startHomework(problem: String, subject: HomeworkType, imageData: Data? = nil) async {
        guard !isProcessing else { return }
        guard orchestrator.isReady else {
            error = OrchestratorError.notInitialized
            return
        }

        isProcessing = true
        error = nil
        currentHintIndex = 0

        do {
            // Ask AI to break down the problem into Socratic steps
            let prompt = """
            \(maieuticSystemPrompt)

            HOMEWORK PROBLEM:
            Subject: \(subject.rawValue)
            Problem: \(problem)

            Generate 3-5 guided steps to help the student solve this problem using the Socratic method.
            For each step, provide:
            1. An instruction (question or guidance)
            2. Three progressive hints

            Format as JSON:
            {
                "steps": [
                    {
                        "instruction": "...",
                        "hints": ["small hint", "bigger hint", "example hint"]
                    }
                ]
            }

            Remember: NEVER give the final answer. Guide them to discover it.
            """

            let response = try await orchestrator.send(prompt)

            // Parse AI response and create steps
            let steps = parseStepsFromResponse(response, subject: subject)

            // Create new homework
            var homework = Homework(
                subject: subject,
                problem: problem,
                steps: steps,
                imageData: imageData
            )

            currentHomework = homework
            logInfo("Started homework assistance for \(subject.rawValue)", category: "Homework")

        } catch {
            self.error = error
            logError("Failed to start homework: \(error)", category: "Homework")
        }

        isProcessing = false
    }

    // MARK: - Step Management

    /// Complete the current step and move to next
    public func completeCurrentStep() {
        guard var homework = currentHomework else { return }

        homework.completeCurrentStep()
        currentHomework = homework
        currentHintIndex = 0  // Reset hints for next step

        // Show celebration if all done
        if homework.completed {
            showCelebration = true
            homeworkHistory.append(homework)

            // Play celebration sound after delay
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
                AppDelegate.playCompletionSound()
            }

            logInfo("Homework completed! Time: \(homework.formattedTimeSpent)", category: "Homework")
        }
    }

    /// Request the next hint (progressive disclosure)
    public func getNextHint() -> String? {
        guard let homework = currentHomework,
              let currentStep = homework.currentStep else { return nil }

        let hint = currentStep.nextHint(currentHintIndex: currentHintIndex)
        if hint != nil {
            currentHintIndex += 1
        }
        return hint
    }

    /// Check if more hints are available
    public func hasMoreHints() -> Bool {
        guard let homework = currentHomework,
              let currentStep = homework.currentStep else { return false }

        return currentStep.hasMoreHints(currentHintIndex: currentHintIndex)
    }

    /// Update student notes for current step
    public func updateStepNotes(_ notes: String) {
        guard var homework = currentHomework else { return }
        homework.updateCurrentStepNotes(notes)
        currentHomework = homework
    }

    // MARK: - AI-Assisted Help

    /// Ask a question about the current step (with anti-cheating)
    public func askQuestion(_ question: String) async {
        guard let homework = currentHomework else { return }
        guard !isProcessing else { return }
        guard orchestrator.isReady else { return }

        isProcessing = true
        currentQuestion = question

        do {
            let prompt = """
            \(maieuticSystemPrompt)

            CONTEXT:
            Subject: \(homework.subject.rawValue)
            Problem: \(homework.problem)
            Current Step: \(homework.currentStep?.instruction ?? "")

            STUDENT QUESTION:
            \(question)

            Respond with a Socratic counter-question or guiding thought.
            NEVER give the answer directly. Help them think through it.

            Use Italian-friendly phrases when appropriate.
            """

            let response = try await orchestrator.send(prompt)

            // In a real implementation, you would display this response
            // For now, we'll log it
            logInfo("AI Guidance: \(response)", category: "Homework")

        } catch {
            self.error = error
            logError("Failed to get AI guidance: \(error)", category: "Homework")
        }

        isProcessing = false
        currentQuestion = ""
    }

    /// Auto-detect subject from problem text
    func detectSubject(from problem: String) async -> HomeworkType {
        guard orchestrator.isReady else { return .other }

        do {
            let prompt = """
            Analyze this homework problem and identify the subject.
            Return ONLY one word: math, essay, science, language, history, geography, philosophy, or other.

            Problem: \(problem)

            Subject:
            """

            let response = try await orchestrator.send(prompt)
            let cleaned = response.trimmingCharacters(in: .whitespacesAndNewlines).lowercased()

            switch cleaned {
            case "math", "matematica": return .math
            case "essay", "tema", "scrittura": return .essay
            case "science", "scienze": return .science
            case "language", "lingue", "inglese", "italiano": return .language
            case "history", "storia": return .history
            case "geography", "geografia": return .geography
            case "philosophy", "filosofia": return .philosophy
            default: return .other
            }

        } catch {
            logError("Failed to detect subject: \(error)", category: "Homework")
            return .other
        }
    }

    // MARK: - History Management

    /// Clear current homework
    func clearCurrentHomework() {
        if let homework = currentHomework, !homework.completed {
            homeworkHistory.append(homework)
        }
        currentHomework = nil
        currentHintIndex = 0
        showCelebration = false
    }

    /// Delete homework from history
    func deleteHomework(_ homework: Homework) {
        homeworkHistory.removeAll { $0.id == homework.id }
    }

    // MARK: - Helper Methods

    /// Parse AI response to extract steps
    private func parseStepsFromResponse(_ response: String, subject: HomeworkType) -> [HomeworkStep] {
        // In a real implementation, parse the JSON response
        // For now, return default steps based on subject

        let defaultPrompts = subject.maieuticPrompts

        // Create 3-4 generic steps
        var steps: [HomeworkStep] = []

        steps.append(HomeworkStep(
            instruction: "Prima di tutto, analizza il problema. Cosa ti viene chiesto esattamente?",
            hints: [
                "Leggi attentamente il testo",
                "Sottolinea le parole chiave",
                "Identifica cosa devi trovare o dimostrare"
            ]
        ))

        steps.append(HomeworkStep(
            instruction: "Quali informazioni hai già? Cosa conosci su questo argomento?",
            hints: [
                "Pensa alle lezioni precedenti",
                "Hai fatto esercizi simili?",
                "Quali formule o regole potrebbero aiutarti?"
            ]
        ))

        steps.append(HomeworkStep(
            instruction: "Ora, quale strategia potresti usare per risolvere questo problema?",
            hints: defaultPrompts.dropLast().map { String($0) }
        ))

        steps.append(HomeworkStep(
            instruction: "Verifica il tuo ragionamento. Ha senso? Come puoi esserne sicuro?",
            hints: [
                "Rileggi la tua soluzione",
                "Prova a spiegarla a voce alta",
                "Controlla ogni passaggio"
            ]
        ))

        return steps
    }

    // MARK: - Encouragement Messages

    /// Get random encouraging message
    public func getEncouragementMessage() -> String {
        let messages = [
            "Ottimo lavoro! Continua così! 🌟",
            "Ci stai arrivando! Hai fatto un grande passo avanti! 💪",
            "Che bella intuizione! Sei sulla strada giusta! 🎯",
            "Bravissimo! Il tuo ragionamento è corretto! ✨",
            "Fantastico! Hai capito il concetto! 🚀",
            "Eccellente! Stai pensando nel modo giusto! 🧠",
            "Complimenti! Hai fatto progressi importanti! 🌈"
        ]

        return messages.randomElement() ?? messages[0]
    }

    /// Get struggle support message
    public func getStruggleSupportMessage() -> String {
        let messages = [
            "Va benissimo fare fatica. Proviamo un altro approccio! 🤔",
            "Nessun problema! Dividiamo in passi più piccoli. 📝",
            "È normale trovare difficoltà. Sei qui per imparare! 💡",
            "Non ti preoccupare, ci arriviamo insieme passo dopo passo. 🤝",
            "La difficoltà è parte dell'apprendimento. Andiamo avanti! 🌱"
        ]

        return messages.randomElement() ?? messages[0]
    }
}

// MARK: - Preview Helper

extension HomeworkViewModel {
    /// Preview instance with mock data
    public static var preview: HomeworkViewModel {
        let vm = HomeworkViewModel()
        vm.currentHomework = Homework.preview
        return vm
    }
}
