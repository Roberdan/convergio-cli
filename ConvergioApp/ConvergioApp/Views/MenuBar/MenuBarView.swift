/**
 * CONVERGIO NATIVE - Menu Bar View
 *
 * Quick access menu bar interface for Convergio.
 * Provides quick prompts, agent status, and cost overview.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

struct MenuBarView: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel
    @EnvironmentObject var conversationVM: ConversationViewModel

    @State private var quickPrompt = ""

    var body: some View {
        VStack(spacing: 16) {
            // Quick prompt field
            HStack {
                TextField("Ask the team...", text: $quickPrompt)
                    .textFieldStyle(.plain)
                    .onSubmit {
                        submitQuickPrompt()
                    }

                if !quickPrompt.isEmpty {
                    Button {
                        submitQuickPrompt()
                    } label: {
                        Image(systemName: "arrow.right.circle.fill")
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(12)
            .background(.ultraThinMaterial)
            .clipShape(RoundedRectangle(cornerRadius: 8))

            // Active agents
            if !orchestratorVM.activeAgents.isEmpty {
                VStack(alignment: .leading, spacing: 8) {
                    Text("Active")
                        .font(.caption)
                        .foregroundStyle(.secondary)

                    ForEach(orchestratorVM.activeAgents.prefix(5)) { agent in
                        HStack {
                            Image(systemName: agent.role.iconName)
                                .frame(width: 20)

                            Text(agent.name)
                                .font(.callout)

                            Spacer()

                            StatusIndicator(state: agent.workState)
                        }
                    }

                    if orchestratorVM.activeAgents.count > 5 {
                        Text("+\(orchestratorVM.activeAgents.count - 5) more")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                }
                .padding(.horizontal, 8)
            }

            Divider()

            // Quick stats
            HStack {
                Label(
                    String(format: "$%.2f", orchestratorVM.costInfo.sessionCost),
                    systemImage: "dollarsign.circle"
                )

                Spacer()

                Label(orchestratorVM.currentModel, systemImage: "cpu")
            }
            .font(.caption)
            .foregroundStyle(.secondary)
            .padding(.horizontal, 8)

            Divider()

            // Actions
            VStack(spacing: 4) {
                Button {
                    NSApp.activate(ignoringOtherApps: true)
                    // Would open main window
                } label: {
                    HStack {
                        Text("Open Convergio")
                        Spacer()
                        Text("⌘O")
                            .foregroundStyle(.secondary)
                    }
                }
                .buttonStyle(.plain)
                .padding(.vertical, 4)
                .padding(.horizontal, 8)

                Button {
                    conversationVM.newConversation()
                } label: {
                    HStack {
                        Text("New Conversation")
                        Spacer()
                        Text("⌘N")
                            .foregroundStyle(.secondary)
                    }
                }
                .buttonStyle(.plain)
                .padding(.vertical, 4)
                .padding(.horizontal, 8)

                Divider()

                Button(role: .destructive) {
                    NSApp.terminate(nil)
                } label: {
                    HStack {
                        Text("Quit")
                        Spacer()
                        Text("⌘Q")
                            .foregroundStyle(.secondary)
                    }
                }
                .buttonStyle(.plain)
                .padding(.vertical, 4)
                .padding(.horizontal, 8)
            }
        }
        .padding()
        .frame(width: 320)
    }

    private func submitQuickPrompt() {
        guard !quickPrompt.isEmpty else { return }
        let prompt = quickPrompt
        quickPrompt = ""

        Task {
            await conversationVM.send(prompt)
        }
    }
}

// MARK: - Status Indicator

struct StatusIndicator: View {
    let state: AgentWorkState

    var body: some View {
        HStack(spacing: 4) {
            Circle()
                .fill(statusColor)
                .frame(width: 6, height: 6)

            Text(state.displayName)
                .font(.caption2)
                .foregroundStyle(.secondary)
        }
    }

    private var statusColor: Color {
        switch state {
        case .idle: return .gray
        case .thinking: return .blue
        case .executing: return .orange
        case .waiting: return .yellow
        case .communicating: return .purple
        }
    }
}

// MARK: - Preview

#Preview {
    MenuBarView()
        .environmentObject(OrchestratorViewModel.preview)
        .environmentObject(ConversationViewModel.preview)
}
