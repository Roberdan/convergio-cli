/**
 * CONVERGIO NATIVE - Sidebar View
 *
 * Agent list sidebar with grouping by role.
 * Shows agent status and enables quick selection.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

struct SidebarView: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel
    @Binding var selectedAgent: Agent?

    @State private var searchText = ""
    @State private var expandedSections: Set<AgentRole> = Set(AgentRole.allCases)

    var body: some View {
        List(selection: $selectedAgent) {
            // Active agents section
            if !orchestratorVM.activeAgents.isEmpty {
                Section("Active") {
                    ForEach(orchestratorVM.activeAgents) { agent in
                        AgentRowView(agent: agent)
                            .tag(agent)
                    }
                }
            }

            // All agents by role
            ForEach(AgentRole.allCases, id: \.self) { role in
                let agentsForRole = filteredAgents.filter { $0.role == role }
                if !agentsForRole.isEmpty {
                    Section(isExpanded: Binding(
                        get: { expandedSections.contains(role) },
                        set: { isExpanded in
                            if isExpanded {
                                expandedSections.insert(role)
                            } else {
                                expandedSections.remove(role)
                            }
                        }
                    )) {
                        ForEach(agentsForRole) { agent in
                            AgentRowView(agent: agent)
                                .tag(agent)
                        }
                    } header: {
                        HStack {
                            Image(systemName: role.iconName)
                            Text(role.displayName)
                        }
                    }
                }
            }
        }
        .listStyle(.sidebar)
        .searchable(text: $searchText, prompt: "Search agents")
        .navigationTitle("Agents")
        .toolbar {
            ToolbarItem {
                Menu {
                    Button("Expand All") {
                        expandedSections = Set(AgentRole.allCases)
                    }
                    Button("Collapse All") {
                        expandedSections.removeAll()
                    }
                    Divider()
                    Button("Show Active Only") {
                        // Filter implementation
                    }
                } label: {
                    Image(systemName: "line.3.horizontal.decrease.circle")
                }
            }
        }
    }

    private var filteredAgents: [Agent] {
        if searchText.isEmpty {
            return orchestratorVM.agents
        }
        return orchestratorVM.agents.filter { agent in
            agent.name.localizedCaseInsensitiveContains(searchText) ||
            agent.description.localizedCaseInsensitiveContains(searchText) ||
            agent.role.displayName.localizedCaseInsensitiveContains(searchText)
        }
    }
}

// MARK: - Agent Row View

struct AgentRowView: View {
    @ObservedObject var agent: Agent

    var body: some View {
        HStack(spacing: 12) {
            // Icon with status indicator
            ZStack(alignment: .bottomTrailing) {
                Image(systemName: agent.role.iconName)
                    .font(.title2)
                    .foregroundStyle(.primary)
                    .frame(width: 32, height: 32)

                // Status dot
                Circle()
                    .fill(statusColor)
                    .frame(width: 10, height: 10)
                    .overlay(
                        Circle()
                            .stroke(.background, lineWidth: 2)
                    )
                    .offset(x: 4, y: 4)
            }

            VStack(alignment: .leading, spacing: 2) {
                Text(agent.name)
                    .font(.headline)

                if agent.workState.isActive {
                    Text(agent.workState.displayName)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                } else {
                    Text(agent.role.displayName)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }

            Spacer()

            // Activity indicator
            if agent.workState == .thinking {
                ProgressView()
                    .scaleEffect(0.7)
            }
        }
        .padding(.vertical, 4)
    }

    private var statusColor: Color {
        switch agent.workState {
        case .idle:
            return .gray
        case .thinking:
            return .blue
        case .executing:
            return .orange
        case .waiting:
            return .yellow
        case .communicating:
            return .purple
        }
    }
}

// MARK: - Previews

#Preview {
    NavigationSplitView {
        SidebarView(selectedAgent: .constant(nil))
            .environmentObject(OrchestratorViewModel.preview)
    } detail: {
        Text("Detail")
    }
}
