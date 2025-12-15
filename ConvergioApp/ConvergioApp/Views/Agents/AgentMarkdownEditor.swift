/**
 * CONVERGIO NATIVE - Agent Markdown Editor
 *
 * A professional markdown editor for customizing agent system prompts
 * and instructions with live preview.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

// MARK: - Agent Configuration Model

/// Editable agent configuration stored locally
struct AgentConfiguration: Identifiable, Codable, Equatable {
    let id: UUID
    var agentId: UInt64
    var name: String
    var role: String
    var systemPrompt: String
    var customInstructions: String
    var isEnabled: Bool
    var temperature: Double
    var maxTokens: Int

    init(
        id: UUID = UUID(),
        agentId: UInt64 = 0,
        name: String = "",
        role: String = "orchestrator",
        systemPrompt: String = "",
        customInstructions: String = "",
        isEnabled: Bool = true,
        temperature: Double = 0.7,
        maxTokens: Int = 4096
    ) {
        self.id = id
        self.agentId = agentId
        self.name = name
        self.role = role
        self.systemPrompt = systemPrompt
        self.customInstructions = customInstructions
        self.isEnabled = isEnabled
        self.temperature = temperature
        self.maxTokens = maxTokens
    }

    /// Create from existing agent
    init(from agent: Agent) {
        self.id = UUID()
        self.agentId = agent.id
        self.name = agent.name
        self.role = agent.role.displayName.lowercased()
        self.systemPrompt = Self.defaultSystemPrompt(for: agent.role)
        self.customInstructions = ""
        self.isEnabled = true
        self.temperature = 0.7
        self.maxTokens = 4096
    }

    static func defaultSystemPrompt(for role: AgentRole) -> String {
        switch role {
        case .orchestrator:
            return """
            # Orchestrator Agent

            You are the **Chief of Staff** for the AI executive team. Your role is to:

            1. Coordinate tasks across specialized agents
            2. Delegate work based on expertise
            3. Synthesize outputs into coherent responses
            4. Ensure quality and consistency

            ## Guidelines

            - Always consider which agent is best suited for each subtask
            - Maintain context across the conversation
            - Provide clear, concise summaries
            """
        case .analyst:
            return """
            # Analyst Agent

            You are a **Data Analyst** specialist. Your role is to:

            1. Conduct deep research and analysis
            2. Identify patterns and trends
            3. Provide data-driven insights
            4. Create clear visualizations and reports

            ## Guidelines

            - Always cite sources when possible
            - Use quantitative data to support conclusions
            - Present findings objectively
            """
        case .coder:
            return """
            # Coder Agent

            You are a **Senior Software Engineer**. Your role is to:

            1. Write clean, production-ready code
            2. Review and improve existing code
            3. Debug and fix issues
            4. Design system architecture

            ## Guidelines

            - Follow best practices and design patterns
            - Write comprehensive tests
            - Document your code clearly
            - Consider security implications
            """
        case .writer:
            return """
            # Writer Agent

            You are a **Content Creator** specialist. Your role is to:

            1. Create compelling, engaging content
            2. Edit and refine text
            3. Craft clear narratives
            4. Adapt tone for different audiences

            ## Guidelines

            - Write with clarity and purpose
            - Use active voice
            - Maintain consistent style
            - Engage the reader
            """
        case .critic:
            return """
            # Critic Agent

            You are a **Quality Reviewer**. Your role is to:

            1. Review work for quality and accuracy
            2. Fact-check claims and statements
            3. Provide constructive feedback
            4. Identify areas for improvement

            ## Guidelines

            - Be thorough but fair
            - Provide specific, actionable feedback
            - Acknowledge strengths as well as weaknesses
            - Maintain objectivity
            """
        case .planner:
            return """
            # Planner Agent

            You are a **Strategic Planner**. Your role is to:

            1. Break complex tasks into manageable steps
            2. Prioritize tasks effectively
            3. Create realistic action plans
            4. Identify dependencies and risks

            ## Guidelines

            - Consider all stakeholders
            - Build in contingencies
            - Set clear milestones
            - Monitor progress
            """
        case .executor:
            return """
            # Executor Agent

            You are a **Task Executor**. Your role is to:

            1. Execute tools and actions
            2. Integrate with external APIs
            3. Automate repetitive tasks
            4. Report on execution status

            ## Guidelines

            - Verify inputs before executing
            - Handle errors gracefully
            - Log all actions
            - Report results clearly
            """
        case .memory:
            return """
            # Memory Agent

            You are a **Knowledge Manager**. Your role is to:

            1. Retrieve relevant context
            2. Store important information
            3. Perform semantic search
            4. Maintain knowledge consistency

            ## Guidelines

            - Organize information logically
            - Update outdated information
            - Maintain context relevance
            - Enable efficient retrieval
            """
        }
    }
}

// MARK: - Agent Configuration Manager

class AgentConfigManager: ObservableObject {
    static let shared = AgentConfigManager()

    @Published var configurations: [AgentConfiguration] = []

    private let configKey = "agent_configurations"

    init() {
        loadConfigurations()
    }

    func loadConfigurations() {
        if let data = UserDefaults.standard.data(forKey: configKey),
           let decoded = try? JSONDecoder().decode([AgentConfiguration].self, from: data) {
            configurations = decoded
        }
    }

    func saveConfigurations() {
        if let encoded = try? JSONEncoder().encode(configurations) {
            UserDefaults.standard.set(encoded, forKey: configKey)
        }
    }

    func addConfiguration(_ config: AgentConfiguration) {
        configurations.append(config)
        saveConfigurations()
    }

    func updateConfiguration(_ config: AgentConfiguration) {
        if let index = configurations.firstIndex(where: { $0.id == config.id }) {
            configurations[index] = config
            saveConfigurations()
        }
    }

    func deleteConfiguration(_ config: AgentConfiguration) {
        configurations.removeAll { $0.id == config.id }
        saveConfigurations()
    }

    func getConfiguration(for agentId: UInt64) -> AgentConfiguration? {
        configurations.first { $0.agentId == agentId }
    }

    /// Initialize configurations from agents
    func initializeFromAgents(_ agents: [Agent]) {
        for agent in agents {
            if getConfiguration(for: agent.id) == nil {
                addConfiguration(AgentConfiguration(from: agent))
            }
        }
    }
}

// MARK: - Agent Markdown Editor View

struct AgentMarkdownEditor: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel
    @StateObject private var configManager = AgentConfigManager.shared
    @State private var selectedConfig: AgentConfiguration?
    @State private var editingConfig: AgentConfiguration?
    @State private var showPreview = true
    @State private var editorText = ""

    var body: some View {
        HSplitView {
            // Agent list
            agentList
                .frame(minWidth: 200, maxWidth: 250)

            // Editor area
            if let config = editingConfig {
                editorView(for: config)
            } else {
                emptyStateView
            }
        }
        .onAppear {
            // Initialize configurations from orchestrator agents
            configManager.initializeFromAgents(orchestratorVM.agents)

            // Select first config if available
            if selectedConfig == nil, let first = configManager.configurations.first {
                selectedConfig = first
                editingConfig = first
                editorText = first.systemPrompt + "\n\n---\n\n" + first.customInstructions
            }
        }
    }

    // MARK: - Agent List

    private var agentList: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Agents")
                    .font(.headline)
                Spacer()
            }
            .padding()
            .background(Color.primary.opacity(0.03))

            Divider()

            // List
            List(selection: $selectedConfig) {
                ForEach(configManager.configurations) { config in
                    AgentListRow(config: config)
                        .tag(config)
                }
            }
            .listStyle(.sidebar)
            .onChange(of: selectedConfig) { _, newValue in
                if let config = newValue {
                    editingConfig = config
                    editorText = config.systemPrompt + "\n\n---\n\n" + config.customInstructions
                }
            }
        }
    }

    // MARK: - Editor View

    private func editorView(for config: AgentConfiguration) -> some View {
        VStack(spacing: 0) {
            // Toolbar
            editorToolbar(for: config)

            Divider()

            // Content
            HSplitView {
                // Editor
                VStack(alignment: .leading, spacing: 0) {
                    HStack {
                        Text("System Prompt")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                        Spacer()
                        Text("\(editorText.count) characters")
                            .font(.caption2)
                            .foregroundStyle(.tertiary)
                    }
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .background(Color.primary.opacity(0.03))

                    MarkdownEditorTextView(text: $editorText)
                }
                .frame(minWidth: 300)

                if showPreview {
                    Divider()

                    // Preview
                    VStack(alignment: .leading, spacing: 0) {
                        HStack {
                            Text("Preview")
                                .font(.caption)
                                .foregroundStyle(.secondary)
                            Spacer()
                        }
                        .padding(.horizontal, 12)
                        .padding(.vertical, 8)
                        .background(Color.primary.opacity(0.03))

                        MarkdownPreviewView(markdown: editorText)
                    }
                    .frame(minWidth: 300)
                }
            }

            Divider()

            // Parameters
            agentParameters(for: config)
        }
    }

    // MARK: - Editor Toolbar

    private func editorToolbar(for config: AgentConfiguration) -> some View {
        HStack(spacing: 16) {
            // Agent info
            HStack(spacing: 8) {
                Image(systemName: iconForRole(config.role))
                    .foregroundStyle(colorForRole(config.role))
                Text(config.name)
                    .font(.headline)
            }

            Spacer()

            // Format buttons
            HStack(spacing: 4) {
                FormatButton(icon: "bold", action: { insertFormatting("**", "**") })
                FormatButton(icon: "italic", action: { insertFormatting("_", "_") })
                FormatButton(icon: "list.bullet", action: { insertFormatting("\n- ", "") })
                FormatButton(icon: "number", action: { insertFormatting("\n1. ", "") })
                FormatButton(icon: "chevron.left.forwardslash.chevron.right", action: { insertFormatting("`", "`") })
            }

            Divider()
                .frame(height: 20)

            // Preview toggle
            Toggle("Preview", isOn: $showPreview)
                .toggleStyle(.switch)
                .controlSize(.small)

            Divider()
                .frame(height: 20)

            // Save button
            Button {
                saveChanges(for: config)
            } label: {
                Label("Save", systemImage: "checkmark.circle.fill")
            }
            .buttonStyle(.borderedProminent)

            // Reset button
            Button {
                resetToDefault(for: config)
            } label: {
                Label("Reset", systemImage: "arrow.counterclockwise")
            }
            .buttonStyle(.bordered)
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 10)
    }

    // MARK: - Agent Parameters

    private func agentParameters(for config: AgentConfiguration) -> some View {
        HStack(spacing: 24) {
            // Temperature
            VStack(alignment: .leading, spacing: 4) {
                Text("Temperature")
                    .font(.caption)
                    .foregroundStyle(.secondary)
                HStack {
                    Slider(
                        value: Binding(
                            get: { editingConfig?.temperature ?? 0.7 },
                            set: { editingConfig?.temperature = $0 }
                        ),
                        in: 0...1,
                        step: 0.1
                    )
                    .frame(width: 120)
                    Text(String(format: "%.1f", editingConfig?.temperature ?? 0.7))
                        .font(.caption.monospacedDigit())
                        .frame(width: 30)
                }
            }

            // Max Tokens
            VStack(alignment: .leading, spacing: 4) {
                Text("Max Tokens")
                    .font(.caption)
                    .foregroundStyle(.secondary)
                HStack {
                    Slider(
                        value: Binding(
                            get: { Double(editingConfig?.maxTokens ?? 4096) },
                            set: { editingConfig?.maxTokens = Int($0) }
                        ),
                        in: 256...16384,
                        step: 256
                    )
                    .frame(width: 120)
                    Text("\(editingConfig?.maxTokens ?? 4096)")
                        .font(.caption.monospacedDigit())
                        .frame(width: 50)
                }
            }

            // Enabled toggle
            Toggle("Enabled", isOn: Binding(
                get: { editingConfig?.isEnabled ?? true },
                set: { editingConfig?.isEnabled = $0 }
            ))
            .toggleStyle(.switch)
            .controlSize(.small)

            Spacer()
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 12)
        .background(Color.primary.opacity(0.03))
    }

    // MARK: - Empty State

    private var emptyStateView: some View {
        VStack(spacing: 16) {
            Image(systemName: "doc.text.magnifyingglass")
                .font(.system(size: 48))
                .foregroundStyle(.secondary)
            Text("Select an agent to edit")
                .font(.headline)
                .foregroundStyle(.secondary)
            Text("Customize system prompts and instructions using Markdown")
                .font(.caption)
                .foregroundStyle(.tertiary)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }

    // MARK: - Helper Functions

    private func insertFormatting(_ prefix: String, _ suffix: String) {
        editorText += prefix + suffix
    }

    private func saveChanges(for config: AgentConfiguration) {
        guard var updatedConfig = editingConfig else { return }

        // Parse system prompt and custom instructions from editor text
        let parts = editorText.components(separatedBy: "\n---\n")
        updatedConfig.systemPrompt = parts.first?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
        updatedConfig.customInstructions = parts.count > 1 ? parts.dropFirst().joined(separator: "\n---\n").trimmingCharacters(in: .whitespacesAndNewlines) : ""

        configManager.updateConfiguration(updatedConfig)
        editingConfig = updatedConfig

        logInfo("Saved agent configuration for \(config.name)", category: "AgentEditor")
    }

    private func resetToDefault(for config: AgentConfiguration) {
        guard let role = AgentRole.allCases.first(where: { $0.displayName.lowercased() == config.role }) else { return }
        let defaultPrompt = AgentConfiguration.defaultSystemPrompt(for: role)
        editorText = defaultPrompt
    }

    private func iconForRole(_ role: String) -> String {
        switch role.lowercased() {
        case "orchestrator": return "brain.head.profile"
        case "analyst": return "chart.bar.xaxis"
        case "coder": return "chevron.left.forwardslash.chevron.right"
        case "writer": return "pencil.and.outline"
        case "critic": return "checkmark.seal"
        case "planner": return "list.bullet.clipboard"
        case "executor": return "gearshape.2"
        case "memory": return "memorychip"
        default: return "person.fill"
        }
    }

    private func colorForRole(_ role: String) -> Color {
        switch role.lowercased() {
        case "orchestrator": return .purple
        case "analyst": return .blue
        case "coder": return .green
        case "writer": return .orange
        case "critic": return .red
        case "planner": return .indigo
        case "executor": return .cyan
        case "memory": return .mint
        default: return .gray
        }
    }
}

// MARK: - Agent List Row

private struct AgentListRow: View {
    let config: AgentConfiguration

    var body: some View {
        HStack(spacing: 10) {
            Image(systemName: iconForRole(config.role))
                .foregroundStyle(colorForRole(config.role))
                .frame(width: 24)

            VStack(alignment: .leading, spacing: 2) {
                Text(config.name)
                    .font(.headline)
                Text(config.role.capitalized)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            if !config.isEnabled {
                Image(systemName: "moon.fill")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
        .padding(.vertical, 4)
    }

    private func iconForRole(_ role: String) -> String {
        switch role.lowercased() {
        case "orchestrator": return "brain.head.profile"
        case "analyst": return "chart.bar.xaxis"
        case "coder": return "chevron.left.forwardslash.chevron.right"
        case "writer": return "pencil.and.outline"
        case "critic": return "checkmark.seal"
        case "planner": return "list.bullet.clipboard"
        case "executor": return "gearshape.2"
        case "memory": return "memorychip"
        default: return "person.fill"
        }
    }

    private func colorForRole(_ role: String) -> Color {
        switch role.lowercased() {
        case "orchestrator": return .purple
        case "analyst": return .blue
        case "coder": return .green
        case "writer": return .orange
        case "critic": return .red
        case "planner": return .indigo
        case "executor": return .cyan
        case "memory": return .mint
        default: return .gray
        }
    }
}

// MARK: - Format Button

private struct FormatButton: View {
    let icon: String
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            Image(systemName: icon)
                .frame(width: 28, height: 28)
        }
        .buttonStyle(.borderless)
        .help(icon.capitalized)
    }
}

// MARK: - Markdown Editor Text View

private struct MarkdownEditorTextView: View {
    @Binding var text: String

    var body: some View {
        TextEditor(text: $text)
            .font(.system(.body, design: .monospaced))
            .scrollContentBackground(.hidden)
            .background(Color(nsColor: .textBackgroundColor))
    }
}

// MARK: - Markdown Preview View

private struct MarkdownPreviewView: View {
    let markdown: String

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 8) {
                // Simple markdown rendering
                ForEach(markdown.components(separatedBy: "\n\n"), id: \.self) { paragraph in
                    renderParagraph(paragraph)
                }
            }
            .padding()
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .background(Color(nsColor: .textBackgroundColor))
    }

    @ViewBuilder
    private func renderParagraph(_ text: String) -> some View {
        if text.hasPrefix("# ") {
            Text(text.dropFirst(2))
                .font(.title.bold())
        } else if text.hasPrefix("## ") {
            Text(text.dropFirst(3))
                .font(.title2.bold())
        } else if text.hasPrefix("### ") {
            Text(text.dropFirst(4))
                .font(.title3.bold())
        } else if text.hasPrefix("- ") || text.hasPrefix("* ") {
            HStack(alignment: .top, spacing: 8) {
                Text("•")
                    .foregroundStyle(.secondary)
                Text(renderInlineFormatting(String(text.dropFirst(2))))
            }
        } else if text.hasPrefix("1. ") || text.hasPrefix("2. ") || text.hasPrefix("3. ") {
            HStack(alignment: .top, spacing: 8) {
                Text(String(text.prefix(2)))
                    .foregroundStyle(.secondary)
                Text(renderInlineFormatting(String(text.dropFirst(3))))
            }
        } else if text.hasPrefix("---") {
            Divider()
        } else if text.hasPrefix("```") {
            Text(text.replacingOccurrences(of: "```", with: ""))
                .font(.system(.body, design: .monospaced))
                .padding(8)
                .background(Color.primary.opacity(0.05))
                .clipShape(RoundedRectangle(cornerRadius: 6))
        } else {
            Text(renderInlineFormatting(text))
        }
    }

    private func renderInlineFormatting(_ text: String) -> AttributedString {
        var result = AttributedString(text)

        // Simple bold handling
        if let boldRange = result.range(of: "**") {
            // Basic handling - in production use proper regex
            result.replaceSubrange(boldRange, with: AttributedString(""))
        }

        return result
    }
}

// MARK: - Preview

#Preview("Agent Editor") {
    AgentMarkdownEditor()
        .environmentObject(OrchestratorViewModel.preview)
        .frame(width: 1000, height: 700)
}
