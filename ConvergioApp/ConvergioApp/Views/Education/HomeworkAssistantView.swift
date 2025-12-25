/**
 * CONVERGIO NATIVE - Homework Assistant View
 *
 * Main interface for homework assistance using the Socratic/maieutic method.
 * Guides students through problems WITHOUT giving direct answers.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

struct HomeworkAssistantView: View {
    @StateObject private var viewModel = HomeworkViewModel()
    @State private var showingNewHomework = false
    @State private var showingPhotoInput = false
    @State private var studentNotes = ""
    @State private var questionText = ""

    var body: some View {
        NavigationSplitView {
            // Sidebar with history
            HomeworkHistorySidebar(
                history: viewModel.homeworkHistory,
                currentHomework: viewModel.currentHomework,
                onSelect: { homework in
                    viewModel.currentHomework = homework
                },
                onDelete: { homework in
                    viewModel.deleteHomework(homework)
                }
            )
        } detail: {
            // Main content
            if let homework = viewModel.currentHomework {
                HomeworkWorkspaceView(
                    homework: homework,
                    viewModel: viewModel,
                    studentNotes: $studentNotes,
                    questionText: $questionText
                )
            } else {
                HomeworkEmptyStateView(
                    onNewHomework: { showingNewHomework = true },
                    onPhotoHomework: { showingPhotoInput = true }
                )
            }
        }
        .navigationTitle("Homework Assistant")
        .toolbar {
            ToolbarItemGroup(placement: .automatic) {
                Button {
                    showingNewHomework = true
                } label: {
                    Label("New Homework", systemImage: "plus.circle")
                }
                .help("Start new homework")

                Button {
                    showingPhotoInput = true
                } label: {
                    Label("From Photo", systemImage: "camera")
                }
                .help("Import homework from photo")
            }
        }
        .sheet(isPresented: $showingNewHomework) {
            NewHomeworkSheet(viewModel: viewModel, isPresented: $showingNewHomework)
        }
        .sheet(isPresented: $showingPhotoInput) {
            PhotoHomeworkView(viewModel: viewModel, isPresented: $showingPhotoInput)
        }
        .alert("Celebration!", isPresented: $viewModel.showCelebration) {
            Button("Continua", role: .cancel) {
                viewModel.clearCurrentHomework()
            }
        } message: {
            Text("Complimenti! Hai completato tutti i passi! 🎉\n\nTempo impiegato: \(viewModel.currentHomework?.formattedTimeSpent ?? "0:00")")
        }
    }
}

// MARK: - History Sidebar

struct HomeworkHistorySidebar: View {
    let history: [Homework]
    let currentHomework: Homework?
    let onSelect: (Homework) -> Void
    let onDelete: (Homework) -> Void

    var body: some View {
        List {
            Section("Attuale") {
                if let current = currentHomework {
                    HomeworkHistoryRow(homework: current, isCurrent: true)
                        .onTapGesture {
                            onSelect(current)
                        }
                } else {
                    Text("Nessun compito attivo")
                        .foregroundStyle(.secondary)
                        .font(.caption)
                }
            }

            if !history.isEmpty {
                Section("Cronologia") {
                    ForEach(history) { homework in
                        HomeworkHistoryRow(homework: homework, isCurrent: false)
                            .onTapGesture {
                                onSelect(homework)
                            }
                            .swipeActions(edge: .trailing, allowsFullSwipe: true) {
                                Button(role: .destructive) {
                                    onDelete(homework)
                                } label: {
                                    Label("Delete", systemImage: "trash")
                                }
                            }
                    }
                }
            }
        }
        .listStyle(.sidebar)
        .frame(minWidth: 200)
    }
}

struct HomeworkHistoryRow: View {
    let homework: Homework
    let isCurrent: Bool

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Image(systemName: homework.subject.icon)
                    .foregroundStyle(homework.subject.color)
                Text(homework.subject.rawValue)
                    .font(.caption.bold())
                Spacer()
                if homework.completed {
                    Image(systemName: "checkmark.circle.fill")
                        .foregroundStyle(.green)
                        .font(.caption)
                }
            }

            Text(homework.problem)
                .font(.caption)
                .lineLimit(2)
                .foregroundStyle(.secondary)

            // Progress bar
            ProgressView(value: homework.progressPercentage)
                .progressViewStyle(.linear)
                .tint(homework.subject.color)
        }
        .padding(.vertical, 4)
        .background(isCurrent ? homework.subject.color.opacity(0.1) : Color.clear)
        .clipShape(RoundedRectangle(cornerRadius: 8))
    }
}

// MARK: - Workspace View

struct HomeworkWorkspaceView: View {
    let homework: Homework
    @ObservedObject var viewModel: HomeworkViewModel
    @Binding var studentNotes: String
    @Binding var questionText: String

    var body: some View {
        VStack(spacing: 0) {
            // Header with problem and progress
            HomeworkHeader(homework: homework, viewModel: viewModel)

            Divider()

            // Main content area
            ScrollView {
                VStack(alignment: .leading, spacing: 20) {
                    // Current step
                    if let currentStep = homework.currentStep {
                        CurrentStepCard(
                            step: currentStep,
                            stepNumber: homework.currentStepIndex + 1,
                            totalSteps: homework.steps.count,
                            viewModel: viewModel,
                            studentNotes: $studentNotes
                        )
                    }

                    // Completed steps
                    if homework.currentStepIndex > 0 {
                        CompletedStepsSection(homework: homework)
                    }

                    // Help section
                    HomeworkHelpSection(
                        homework: homework,
                        viewModel: viewModel,
                        questionText: $questionText
                    )
                }
                .padding()
            }

            Divider()

            // Bottom action bar
            HomeworkActionBar(
                homework: homework,
                viewModel: viewModel,
                studentNotes: studentNotes
            )
        }
    }
}

// MARK: - Header

struct HomeworkHeader: View {
    let homework: Homework
    @ObservedObject var viewModel: HomeworkViewModel

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            // Subject badge
            HStack {
                Label(homework.subject.rawValue, systemImage: homework.subject.icon)
                    .font(.headline)
                    .foregroundStyle(homework.subject.color)

                Spacer()

                // Timer
                HStack(spacing: 4) {
                    Image(systemName: "clock")
                    Text(homework.formattedTimeSpent)
                }
                .font(.caption)
                .foregroundStyle(.secondary)
            }

            // Problem text
            Text(homework.problem)
                .font(.title3)
                .fontWeight(.semibold)

            // Progress
            VStack(alignment: .leading, spacing: 4) {
                HStack {
                    Text("Progresso")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    Spacer()
                    Text("\(Int(homework.progressPercentage * 100))%")
                        .font(.caption.bold())
                        .foregroundStyle(homework.subject.color)
                }

                ProgressView(value: homework.progressPercentage)
                    .progressViewStyle(.linear)
                    .tint(homework.subject.color)
            }
        }
        .padding()
        .background(.ultraThinMaterial)
    }
}

// MARK: - Current Step Card

struct CurrentStepCard: View {
    let step: HomeworkStep
    let stepNumber: Int
    let totalSteps: Int
    @ObservedObject var viewModel: HomeworkViewModel
    @Binding var studentNotes: String

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            // Step header
            HStack {
                Label("Passo \(stepNumber) di \(totalSteps)", systemImage: "flag.fill")
                    .font(.headline)
                    .foregroundStyle(.purple)

                Spacer()

                // Hint button
                if viewModel.hasMoreHints() {
                    Button {
                        if let hint = viewModel.getNextHint() {
                            // Show hint (could be an alert or inline)
                        }
                    } label: {
                        Label("Aiutino", systemImage: "lightbulb")
                            .font(.caption)
                    }
                    .buttonStyle(.bordered)
                    .tint(.yellow)
                }
            }

            // Instruction
            Text(step.instruction)
                .font(.title3)
                .padding()
                .frame(maxWidth: .infinity, alignment: .leading)
                .background(Color.purple.opacity(0.1))
                .clipShape(RoundedRectangle(cornerRadius: 12))

            // Hints revealed so far
            if viewModel.currentHintIndex > 0 {
                VStack(alignment: .leading, spacing: 8) {
                    ForEach(0..<viewModel.currentHintIndex, id: \.self) { index in
                        if index < step.hints.count {
                            HintBubble(
                                hint: step.hints[index],
                                level: index + 1
                            )
                        }
                    }
                }
            }

            // Student notes area
            VStack(alignment: .leading, spacing: 8) {
                Text("I tuoi appunti:")
                    .font(.caption.bold())
                    .foregroundStyle(.secondary)

                TextEditor(text: $studentNotes)
                    .font(.body)
                    .frame(minHeight: 100)
                    .padding(8)
                    .background(Color(nsColor: .textBackgroundColor))
                    .clipShape(RoundedRectangle(cornerRadius: 8))
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(Color.secondary.opacity(0.2), lineWidth: 1)
                    )
                    .onChange(of: studentNotes) { oldValue, newValue in
                        viewModel.updateStepNotes(newValue)
                    }
            }
        }
        .padding()
        .background(Color(nsColor: .windowBackgroundColor))
        .clipShape(RoundedRectangle(cornerRadius: 16))
        .shadow(color: .black.opacity(0.1), radius: 10, x: 0, y: 4)
    }
}

// MARK: - Hint Bubble

struct HintBubble: View {
    let hint: String
    let level: Int

    private var hintIcon: String {
        switch level {
        case 1: return "1.circle.fill"
        case 2: return "2.circle.fill"
        case 3: return "3.circle.fill"
        default: return "lightbulb.fill"
        }
    }

    private var hintColor: Color {
        switch level {
        case 1: return .green
        case 2: return .orange
        case 3: return .red
        default: return .yellow
        }
    }

    var body: some View {
        HStack(alignment: .top, spacing: 12) {
            Image(systemName: hintIcon)
                .foregroundStyle(hintColor)
                .font(.title3)

            Text(hint)
                .font(.callout)
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(hintColor.opacity(0.1))
        .clipShape(RoundedRectangle(cornerRadius: 8))
    }
}

// MARK: - Completed Steps

struct CompletedStepsSection: View {
    let homework: Homework

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Passi completati ✓")
                .font(.headline)
                .foregroundStyle(.secondary)

            ForEach(Array(homework.steps.enumerated()), id: \.element.id) { index, step in
                if step.isCompleted {
                    CompletedStepRow(
                        step: step,
                        stepNumber: index + 1
                    )
                }
            }
        }
    }
}

struct CompletedStepRow: View {
    let step: HomeworkStep
    let stepNumber: Int

    var body: some View {
        HStack(alignment: .top, spacing: 12) {
            Image(systemName: "checkmark.circle.fill")
                .foregroundStyle(.green)
                .font(.title3)

            VStack(alignment: .leading, spacing: 4) {
                Text("Passo \(stepNumber)")
                    .font(.caption.bold())
                    .foregroundStyle(.secondary)

                Text(step.instruction)
                    .font(.callout)
                    .foregroundStyle(.secondary)

                if !step.studentNotes.isEmpty {
                    Text(step.studentNotes)
                        .font(.caption)
                        .foregroundStyle(.tertiary)
                        .padding(.top, 4)
                }
            }
        }
        .padding()
        .background(Color.green.opacity(0.05))
        .clipShape(RoundedRectangle(cornerRadius: 8))
    }
}

// MARK: - Homework Help Section

private struct HomeworkHelpSection: View {
    let homework: Homework
    @ObservedObject var viewModel: HomeworkViewModel
    @Binding var questionText: String

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Hai bisogno di aiuto?")
                .font(.headline)

            Text("Ricorda: non ti darò la risposta, ma ti guiderò a trovarla da solo!")
                .font(.caption)
                .foregroundStyle(.secondary)
                .padding(.horizontal, 12)
                .padding(.vertical, 6)
                .background(Color.orange.opacity(0.1))
                .clipShape(RoundedRectangle(cornerRadius: 8))

            // Quick help buttons
            VStack(spacing: 8) {
                ForEach(homework.subject.maieuticPrompts.prefix(3), id: \.self) { prompt in
                    Button {
                        Task {
                            await viewModel.askQuestion(prompt)
                        }
                    } label: {
                        HStack {
                            Image(systemName: "questionmark.circle")
                            Text(prompt)
                                .font(.callout)
                            Spacer()
                        }
                        .padding(.horizontal, 12)
                        .padding(.vertical, 8)
                    }
                    .buttonStyle(.bordered)
                    .tint(.purple)
                }
            }

            // Custom question
            HStack(spacing: 8) {
                TextField("Scrivi la tua domanda...", text: $questionText)
                    .textFieldStyle(.plain)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .background(Color(nsColor: .textBackgroundColor))
                    .clipShape(RoundedRectangle(cornerRadius: 8))
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(Color.secondary.opacity(0.2), lineWidth: 1)
                    )

                Button {
                    guard !questionText.isEmpty else { return }
                    Task {
                        await viewModel.askQuestion(questionText)
                        questionText = ""
                    }
                } label: {
                    Image(systemName: "paperplane.fill")
                }
                .buttonStyle(.borderedProminent)
                .disabled(questionText.isEmpty || viewModel.isProcessing)
            }
        }
        .padding()
        .background(Color.purple.opacity(0.05))
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

// MARK: - Action Bar

struct HomeworkActionBar: View {
    let homework: Homework
    @ObservedObject var viewModel: HomeworkViewModel
    let studentNotes: String

    var body: some View {
        HStack(spacing: 16) {
            // Encouragement
            if !studentNotes.isEmpty {
                Text(viewModel.getEncouragementMessage())
                    .font(.caption)
                    .foregroundStyle(.green)
            } else {
                Text("Scrivi i tuoi pensieri nel riquadro sopra")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            // I'm stuck button
            if viewModel.hasMoreHints() {
                Button {
                    if let hint = viewModel.getNextHint() {
                        // Hint is now shown automatically
                    }
                } label: {
                    Label("Sono bloccato", systemImage: "questionmark.circle")
                }
                .buttonStyle(.bordered)
                .tint(.yellow)
            }

            // Complete step button
            Button {
                viewModel.completeCurrentStep()
            } label: {
                if homework.currentStepIndex == homework.steps.count - 1 {
                    Label("Finisci!", systemImage: "checkmark.circle.fill")
                } else {
                    Label("Passo successivo", systemImage: "arrow.right.circle.fill")
                }
            }
            .buttonStyle(.borderedProminent)
            .tint(.green)
            .disabled(studentNotes.isEmpty)
        }
        .padding()
        .background(.ultraThinMaterial)
    }
}

// MARK: - Empty State

struct HomeworkEmptyStateView: View {
    let onNewHomework: () -> Void
    let onPhotoHomework: () -> Void

    var body: some View {
        VStack(spacing: 24) {
            Spacer()

            Image(systemName: "book.and.wrench")
                .font(.system(size: 64))
                .foregroundStyle(.purple.gradient)

            Text("Homework Assistant")
                .font(.largeTitle.bold())

            Text("Ti guido attraverso i compiti senza darti la risposta!")
                .font(.headline)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)

            // Action buttons
            HStack(spacing: 16) {
                Button {
                    onNewHomework()
                } label: {
                    VStack(spacing: 8) {
                        Image(systemName: "pencil.and.outline")
                            .font(.title)
                        Text("Scrivi problema")
                            .font(.caption)
                    }
                    .frame(width: 140, height: 100)
                }
                .buttonStyle(.bordered)

                Button {
                    onPhotoHomework()
                } label: {
                    VStack(spacing: 8) {
                        Image(systemName: "camera")
                            .font(.title)
                        Text("Foto problema")
                            .font(.caption)
                    }
                    .frame(width: 140, height: 100)
                }
                .buttonStyle(.bordered)
            }

            Divider()
                .frame(width: 200)

            // Info cards
            VStack(alignment: .leading, spacing: 12) {
                Text("Come funziona:")
                    .font(.headline)

                InfoRow(icon: "brain.head.profile", text: "Ti faccio domande per guidarti", color: .blue)
                InfoRow(icon: "lightbulb", text: "Aiutini progressivi se sei bloccato", color: .yellow)
                InfoRow(icon: "xmark.circle", text: "MAI la risposta diretta", color: .red)
                InfoRow(icon: "star.fill", text: "Celebriamo il processo, non solo il risultato", color: .green)
            }
            .frame(maxWidth: 400)

            Spacer()
        }
        .padding(40)
    }
}

struct InfoRow: View {
    let icon: String
    let text: String
    let color: Color

    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: icon)
                .foregroundStyle(color)
                .frame(width: 24)
            Text(text)
                .font(.callout)
        }
    }
}

// MARK: - New Homework Sheet

struct NewHomeworkSheet: View {
    @ObservedObject var viewModel: HomeworkViewModel
    @Binding var isPresented: Bool

    @State private var problemText = ""
    @State private var selectedSubject: HomeworkType = .math

    var body: some View {
        VStack(spacing: 20) {
            Text("Nuovo Compito")
                .font(.title2.bold())

            // Subject picker
            Picker("Materia", selection: $selectedSubject) {
                ForEach(HomeworkType.allCases) { subject in
                    Label(subject.rawValue, systemImage: subject.icon)
                        .tag(subject)
                }
            }
            .pickerStyle(.segmented)

            // Problem input
            VStack(alignment: .leading, spacing: 8) {
                Text("Descrivi il problema:")
                    .font(.headline)

                TextEditor(text: $problemText)
                    .font(.body)
                    .frame(minHeight: 150)
                    .padding(8)
                    .background(Color(nsColor: .textBackgroundColor))
                    .clipShape(RoundedRectangle(cornerRadius: 8))
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(Color.secondary.opacity(0.2), lineWidth: 1)
                    )
            }

            // Actions
            HStack(spacing: 12) {
                Button("Annulla") {
                    isPresented = false
                }
                .buttonStyle(.bordered)

                Button("Inizia") {
                    Task {
                        await viewModel.startHomework(
                            problem: problemText,
                            subject: selectedSubject
                        )
                        isPresented = false
                    }
                }
                .buttonStyle(.borderedProminent)
                .disabled(problemText.isEmpty || viewModel.isProcessing)
            }

            if viewModel.isProcessing {
                ProgressView("Analyzing problem...")
                    .padding()
            }
        }
        .padding(24)
        .frame(width: 500)
    }
}

// MARK: - Preview

#Preview("Homework Assistant") {
    HomeworkAssistantView()
}

#Preview("With Homework") {
    let vm = HomeworkViewModel.preview
    return HomeworkWorkspaceView(
        homework: vm.currentHomework!,
        viewModel: vm,
        studentNotes: .constant(""),
        questionText: .constant("")
    )
}
