/**
 * CONVERGIO NATIVE - Content View
 *
 * Main content view with sidebar navigation and conversation area.
 * Uses NavigationSplitView for three-column layout.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

struct ContentView: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel
    @EnvironmentObject var conversationVM: ConversationViewModel
    @StateObject private var editionManager = EditionManager.shared

    @State private var columnVisibility: NavigationSplitViewVisibility = .all
    @State private var selectedAgent: Agent?
    @State private var selectedMaestro: Maestro?
    @State private var educationSection: EducationSection = .maestri

    var body: some View {
        Group {
            if editionManager.currentEdition == .education {
                educationLayout
            } else {
                businessLayout
            }
        }
        .toolbar {
            ToolbarItemGroup(placement: .primaryAction) {
                // Edition indicator
                Menu {
                    ForEach(ConvergioEdition.allCases, id: \.self) { edition in
                        Button {
                            editionManager.setEdition(edition)
                        } label: {
                            HStack {
                                Text(edition.displayName)
                                if edition == editionManager.currentEdition {
                                    Image(systemName: "checkmark")
                                }
                            }
                        }
                    }
                } label: {
                    Label(editionManager.currentEdition.displayName, systemImage: editionManager.currentEdition == .education ? "graduationcap.fill" : "briefcase.fill")
                }

                // Model selector
                Menu {
                    ForEach(orchestratorVM.availableModels, id: \.self) { model in
                        Button(model) {
                            orchestratorVM.selectModel(model)
                        }
                    }
                } label: {
                    Label(orchestratorVM.currentModel, systemImage: "cpu")
                }

                // Cost indicator
                CostBadge(cost: orchestratorVM.costInfo)

                // Toggle sidebar
                Button {
                    withAnimation {
                        columnVisibility = columnVisibility == .all ? .detailOnly : .all
                    }
                } label: {
                    Image(systemName: "sidebar.left")
                }
            }
        }
    }

    // MARK: - Education Layout

    private var educationLayout: some View {
        NavigationSplitView(columnVisibility: $columnVisibility) {
            EducationSidebarView(selectedSection: $educationSection, selectedMaestro: $selectedMaestro)
                .navigationSplitViewColumnWidth(min: 180, ideal: 220, max: 280)
        } detail: {
            educationMainContent
                .frame(minWidth: 600)
        }
        .navigationSplitViewStyle(.balanced)
    }

    @ViewBuilder
    private var educationMainContent: some View {
        switch educationSection {
        case .maestri:
            if let maestro = selectedMaestro {
                VStack(spacing: 0) {
                    // Back button bar
                    HStack {
                        Button {
                            withAnimation(.spring(duration: 0.3)) {
                                selectedMaestro = nil
                            }
                        } label: {
                            HStack(spacing: 6) {
                                Image(systemName: "chevron.left")
                                    .font(.body.weight(.medium))
                                Text("Tutti i Maestri")
                                    .font(.body)
                            }
                            .foregroundStyle(.secondary)
                        }
                        .buttonStyle(.plain)
                        .padding(.horizontal, 20)
                        .padding(.vertical, 12)

                        Spacer()
                    }
                    .background(.ultraThinMaterial)

                    MaestroDetailView(maestro: maestro)
                }
            } else {
                MaestriGridView(selectedMaestro: $selectedMaestro)
            }
        case .flashcards:
            FlashcardDecksListView()
        case .quiz:
            QuizSelectionView()
        case .libretto:
            LibrettoView()
        case .progress:
            ProgressDashboard()
        case .voice:
            VoiceSessionView(maestro: selectedMaestro)
        case .homework:
            HomeworkAssistantView()
        case .mindmap:
            MindmapView()
        }
    }

    // MARK: - Business Layout (Original)

    private var businessLayout: some View {
        NavigationSplitView(columnVisibility: $columnVisibility) {
            SidebarView(selectedAgent: $selectedAgent)
                .navigationSplitViewColumnWidth(min: 200, ideal: 250, max: 300)
        } content: {
            if let agent = selectedAgent {
                AgentDetailView(agent: agent)
            } else {
                AgentGridView()
            }
        } detail: {
            ConversationView()
                .navigationSplitViewColumnWidth(min: 400, ideal: 600)
        }
        .navigationSplitViewStyle(.balanced)
    }
}

// MARK: - Education Section Enum

enum EducationSection: String, CaseIterable, Identifiable {
    case maestri = "Maestri"
    case voice = "Voce"
    case flashcards = "Flashcard"
    case quiz = "Quiz"
    case homework = "Compiti"
    case mindmap = "Mappe"
    case libretto = "Libretto"
    case progress = "Progressi"

    var id: String { rawValue }

    var icon: String {
        switch self {
        case .maestri: return "person.3.fill"
        case .voice: return "waveform.circle.fill"
        case .flashcards: return "rectangle.stack.fill"
        case .quiz: return "questionmark.circle.fill"
        case .homework: return "book.fill"
        case .mindmap: return "arrow.triangle.branch"
        case .libretto: return "text.book.closed.fill"
        case .progress: return "chart.bar.fill"
        }
    }
}

// MARK: - Education Sidebar

struct EducationSidebarView: View {
    @Binding var selectedSection: EducationSection
    @Binding var selectedMaestro: Maestro?

    var body: some View {
        List(selection: $selectedSection) {
            Section("Scuola 2026") {
                ForEach(EducationSection.allCases) { section in
                    Button {
                        selectedSection = section
                        // Clear maestro selection when clicking Maestri to go back to grid
                        if section == .maestri {
                            selectedMaestro = nil
                        }
                    } label: {
                        Label(section.rawValue, systemImage: section.icon)
                    }
                    .buttonStyle(.plain)
                    .tag(section)
                    .listRowBackground(
                        selectedSection == section
                            ? Color.accentColor.opacity(0.15)
                            : Color.clear
                    )
                }
            }

            Section("Maestri Recenti") {
                ForEach(Maestro.previewMaestri.prefix(5)) { maestro in
                    Button {
                        selectedMaestro = maestro
                        selectedSection = .maestri
                    } label: {
                        HStack(spacing: 10) {
                            ZStack {
                                Circle()
                                    .fill(maestro.color.opacity(0.2))
                                    .frame(width: 28, height: 28)
                                Image(systemName: maestro.icon)
                                    .font(.system(size: 12))
                                    .foregroundStyle(maestro.color)
                            }
                            Text(maestro.name)
                                .font(.subheadline)
                        }
                    }
                    .buttonStyle(.plain)
                }
            }
        }
        .listStyle(.sidebar)
        .navigationTitle("Education")
    }
}

// MARK: - Agent Grid View

struct AgentGridView: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel

    let columns = [
        GridItem(.adaptive(minimum: 150, maximum: 200), spacing: 16)
    ]

    var body: some View {
        ScrollView {
            LazyVGrid(columns: columns, spacing: 16) {
                ForEach(orchestratorVM.agents) { agent in
                    AgentCardView(agent: agent)
                }
            }
            .padding()
        }
        .navigationTitle("AI Executive Team")
    }
}

// MARK: - Cost Badge

struct CostBadge: View {
    let cost: CostInfo

    var body: some View {
        HStack(spacing: 4) {
            Image(systemName: "dollarsign.circle")
            Text(String(format: "$%.2f", cost.sessionCost))
                .monospacedDigit()

            if cost.budgetLimit > 0 {
                Text("/")
                    .foregroundStyle(.secondary)
                Text(String(format: "$%.0f", cost.budgetLimit))
                    .monospacedDigit()
                    .foregroundStyle(.secondary)
            }
        }
        .font(.caption)
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(.ultraThinMaterial)
        .clipShape(Capsule())
        .foregroundStyle(cost.isOverBudget ? .red : .primary)
    }
}

// MARK: - Quick Settings View (Placeholder for ContentView)

struct QuickSettingsView: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel

    var body: some View {
        TabView {
            GeneralSettingsView()
                .tabItem {
                    Label("General", systemImage: "gear")
                }

            ProvidersSettingsView()
                .tabItem {
                    Label("Providers", systemImage: "server.rack")
                }

            BudgetSettingsView()
                .tabItem {
                    Label("Budget", systemImage: "dollarsign.circle")
                }
        }
        .frame(width: 500, height: 400)
    }
}

struct GeneralSettingsView: View {
    var body: some View {
        Form {
            Text("General settings will go here")
        }
        .padding()
    }
}

struct ProvidersSettingsView: View {
    var body: some View {
        Form {
            Text("Provider settings will go here")
        }
        .padding()
    }
}

struct BudgetSettingsView: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel
    @State private var budgetLimit: Double = 10.0

    var body: some View {
        Form {
            Section("Session Budget") {
                Slider(value: $budgetLimit, in: 1...100, step: 1) {
                    Text("Budget Limit")
                } minimumValueLabel: {
                    Text("$1")
                } maximumValueLabel: {
                    Text("$100")
                }

                Text("Current limit: $\(Int(budgetLimit))")
                    .font(.caption)
                    .foregroundStyle(.secondary)

                Button("Apply") {
                    orchestratorVM.setBudget(budgetLimit)
                }
            }
        }
        .padding()
        .onAppear {
            budgetLimit = orchestratorVM.costInfo.budgetLimit
        }
    }
}

// MARK: - Previews

#Preview {
    ContentView()
        .environmentObject(OrchestratorViewModel.preview)
        .environmentObject(ConversationViewModel.preview)
}
