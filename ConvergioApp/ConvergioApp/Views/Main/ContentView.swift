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

    @State private var columnVisibility: NavigationSplitViewVisibility = .all
    @State private var selectedAgent: Agent?

    var body: some View {
        NavigationSplitView(columnVisibility: $columnVisibility) {
            // Sidebar - Agent list
            SidebarView(selectedAgent: $selectedAgent)
                .navigationSplitViewColumnWidth(min: 200, ideal: 250, max: 300)
        } content: {
            // Content - Agent detail or empty state
            if let agent = selectedAgent {
                AgentDetailView(agent: agent)
            } else {
                AgentGridView()
            }
        } detail: {
            // Detail - Conversation
            ConversationView()
                .navigationSplitViewColumnWidth(min: 400, ideal: 600)
        }
        .navigationSplitViewStyle(.balanced)
        .toolbar {
            ToolbarItemGroup(placement: .primaryAction) {
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

// MARK: - Agent Detail View

struct AgentDetailView: View {
    let agent: Agent

    var body: some View {
        VStack(alignment: .leading, spacing: 20) {
            // Header
            HStack(spacing: 16) {
                Image(systemName: agent.role.iconName)
                    .font(.system(size: 48))
                    .foregroundStyle(.primary)
                    .frame(width: 80, height: 80)
                    .background(.ultraThinMaterial)
                    .clipShape(Circle())

                VStack(alignment: .leading, spacing: 4) {
                    Text(agent.name)
                        .font(.largeTitle)
                        .fontWeight(.bold)

                    Text(agent.role.displayName)
                        .font(.title3)
                        .foregroundStyle(.secondary)

                    HStack {
                        Circle()
                            .fill(agent.workState.isActive ? Color.green : Color.gray)
                            .frame(width: 8, height: 8)
                        Text(agent.workState.displayName)
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                }
            }
            .padding()

            Divider()

            // Description
            Text(agent.description)
                .font(.body)
                .padding(.horizontal)

            // Current task
            if let task = agent.currentTask {
                GroupBox("Current Task") {
                    Text(task)
                        .font(.body)
                }
                .padding(.horizontal)
            }

            Spacer()
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .navigationTitle(agent.name)
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

// MARK: - Settings View (Placeholder)

struct SettingsView: View {
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
