/**
 * CONVERGIO NATIVE - Settings View
 *
 * Comprehensive settings interface with tabs for general, providers, budget, and appearance.
 * Uses Liquid Glass design language with glass-styled components.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

// MARK: - Settings Tab Definition

enum SettingsTab: String, CaseIterable, Identifiable {
    case general = "General"
    case providers = "Providers"
    case budget = "Budget"
    case appearance = "Appearance"
    case shortcuts = "Shortcuts"
    case student = "Student"
    case advanced = "Advanced"
    case mcp = "MCP Services"

    var id: String { rawValue }

    var icon: String {
        switch self {
        case .general: return "gear"
        case .providers: return "key.fill"
        case .budget: return "dollarsign.circle.fill"
        case .appearance: return "paintbrush.fill"
        case .shortcuts: return "keyboard"
        case .student: return "graduationcap.fill"
        case .advanced: return "gearshape.2.fill"
        case .mcp: return "server.rack"
        }
    }

    var color: Color {
        switch self {
        case .general: return .gray
        case .providers: return .blue
        case .budget: return .green
        case .appearance: return .purple
        case .shortcuts: return .orange
        case .student: return .indigo
        case .advanced: return .red
        case .mcp: return .teal
        }
    }
}

// MARK: - Main Settings View

struct MainSettingsView: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel
    @State private var selectedTab: SettingsTab = .general

    var body: some View {
        NavigationSplitView {
            // Sidebar
            List(SettingsTab.allCases, selection: $selectedTab) { tab in
                Label {
                    Text(tab.rawValue)
                        .font(.body)
                } icon: {
                    Image(systemName: tab.icon)
                        .foregroundStyle(tab.color)
                }
                .tag(tab)
                .padding(.vertical, 4)
            }
            .listStyle(.sidebar)
            .navigationSplitViewColumnWidth(min: 180, ideal: 200, max: 220)
        } detail: {
            // Content area with glass effect header
            VStack(spacing: 0) {
                // Header bar
                HStack {
                    Image(systemName: selectedTab.icon)
                        .font(.title2)
                        .foregroundStyle(selectedTab.color)
                    Text(selectedTab.rawValue)
                        .font(.title2.weight(.semibold))
                    Spacer()
                }
                .padding(.horizontal, 24)
                .padding(.vertical, 16)
                .background(.regularMaterial)

                Divider()

                // Tab content
                Group {
                    switch selectedTab {
                    case .general:
                        GeneralSettingsTab()
                    case .providers:
                        ProvidersSettingsTab()
                    case .budget:
                        BudgetSettingsTab()
                    case .appearance:
                        AppearanceSettingsTab()
                    case .shortcuts:
                        ShortcutsSettingsTab()
                    case .student:
                        StudentProfileSettingsTab()
                    case .advanced:
                        AdvancedSettingsTab()
                    case .mcp:
                        MCPServicesSettingsTab()
                    }
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        .frame(width: 800, height: 600)
        .navigationSplitViewStyle(.balanced)
    }
}

// MARK: - General Settings Tab

struct GeneralSettingsTab: View {
    @AppStorage("launchAtLogin") private var launchAtLogin = false
    @AppStorage("showInMenuBar") private var showInMenuBar = true
    @AppStorage("notificationsEnabled") private var notificationsEnabled = true
    @AppStorage("soundEffectsEnabled") private var soundEffectsEnabled = true
    @AppStorage("notifyTaskCompletion") private var notifyTaskCompletion = true
    @AppStorage("notifyBudgetWarnings") private var notifyBudgetWarnings = true
    @AppStorage("notifyAgentCollaboration") private var notifyAgentCollaboration = false

    var body: some View {
        ScrollView {
            VStack(spacing: 24) {
                // Startup Section
                SettingsSection(title: "Startup", icon: "power") {
                    SettingsRow {
                        Toggle("Launch at Login", isOn: $launchAtLogin)
                    }
                    SettingsRow {
                        Toggle("Show in Menu Bar", isOn: $showInMenuBar)
                    }
                }

                // Notifications Section
                SettingsSection(title: "Notifications", icon: "bell.fill") {
                    SettingsRow {
                        Toggle("Enable Notifications", isOn: $notificationsEnabled)
                    }
                    SettingsRow {
                        Toggle("Sound Effects", isOn: $soundEffectsEnabled)
                    }
                    .disabled(!notificationsEnabled)

                    if notificationsEnabled {
                        Divider().padding(.vertical, 4)

                        SettingsRow {
                            Toggle("Task Completion", isOn: $notifyTaskCompletion)
                        }
                        SettingsRow {
                            Toggle("Budget Warnings", isOn: $notifyBudgetWarnings)
                        }
                        SettingsRow {
                            Toggle("Agent Collaboration", isOn: $notifyAgentCollaboration)
                        }
                    }
                }

                // Privacy Section
                SettingsSection(title: "Privacy & Data", icon: "lock.shield.fill") {
                    SettingsRow {
                        HStack {
                            VStack(alignment: .leading, spacing: 2) {
                                Text("Conversation History")
                                Text("Clear all past conversations")
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                            }
                            Spacer()
                            Button("Clear History") {
                                // TODO: Implement
                            }
                            .controlSize(.regular)
                        }
                    }

                    SettingsRow {
                        HStack {
                            VStack(alignment: .leading, spacing: 2) {
                                Text("Cached Data")
                                Text("Clear temporary files and cache")
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                            }
                            Spacer()
                            Button("Clear Cache") {
                                // TODO: Implement
                            }
                            .controlSize(.regular)
                        }
                    }
                }

                Spacer()
            }
            .padding(24)
        }
    }
}

// MARK: - Settings UI Components

private struct SettingsSection<Content: View>: View {
    let title: String
    let icon: String
    @ViewBuilder let content: Content

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Label(title, systemImage: icon)
                .font(.headline)
                .foregroundStyle(.primary)

            VStack(spacing: 0) {
                content
            }
            .background(Color(nsColor: .controlBackgroundColor))
            .clipShape(RoundedRectangle(cornerRadius: 8))
        }
    }
}

private struct SettingsRow<Content: View>: View {
    @ViewBuilder let content: Content

    var body: some View {
        content
            .padding(.horizontal, 12)
            .padding(.vertical, 10)
    }
}

// MARK: - Providers Settings Tab

struct ProvidersSettingsTab: View {
    @ObservedObject var keychainManager = KeychainManager.shared
    @State private var selectedProvider: APIProvider?
    @State private var apiKeyInput = ""
    @State private var showApiKey = false
    @State private var showSaveConfirmation = false

    /// Filter providers based on edition
    private var allowedProviders: [APIProvider] {
        let isEDU = EditionManager.shared.currentEdition == .education
        if isEDU {
            return APIProvider.allCases.filter { $0 != .anthropic && !$0.isAzureVoice }
        }
        return APIProvider.allCases.filter { !$0.isAzureVoice }
    }

    var body: some View {
        ScrollView {
            VStack(spacing: 24) {
                // Provider Overview Section
                SettingsSection(title: "AI Providers", icon: "brain.head.profile") {
                    ForEach(allowedProviders) { provider in
                        SettingsRow {
                            HStack(spacing: 12) {
                                Image(systemName: provider.icon)
                                    .font(.title3)
                                    .foregroundStyle(keychainManager.getKey(for: provider) != nil ? .green : .secondary)
                                    .frame(width: 28)

                                VStack(alignment: .leading, spacing: 2) {
                                    Text(provider.displayName)
                                        .font(.body.weight(.medium))
                                    if keychainManager.getKey(for: provider) != nil {
                                        Text("Configured")
                                            .font(.caption)
                                            .foregroundStyle(.green)
                                    } else if !provider.requiresAPIKey {
                                        Text("No key required")
                                            .font(.caption)
                                            .foregroundStyle(.blue)
                                    } else {
                                        Text("Not configured")
                                            .font(.caption)
                                            .foregroundStyle(.secondary)
                                    }
                                }

                                Spacer()

                                if selectedProvider == provider {
                                    Image(systemName: "chevron.down")
                                        .foregroundStyle(.secondary)
                                } else {
                                    Image(systemName: "chevron.right")
                                        .foregroundStyle(.tertiary)
                                }
                            }
                            .contentShape(Rectangle())
                            .onTapGesture {
                                withAnimation(.easeInOut(duration: 0.2)) {
                                    if selectedProvider == provider {
                                        selectedProvider = nil
                                    } else {
                                        selectedProvider = provider
                                        apiKeyInput = ""
                                        showApiKey = false
                                        showSaveConfirmation = false
                                    }
                                }
                            }
                        }

                        // Expanded configuration panel
                        if selectedProvider == provider {
                            ProviderConfigPanel(
                                provider: provider,
                                keychainManager: keychainManager,
                                apiKeyInput: $apiKeyInput,
                                showApiKey: $showApiKey,
                                showSaveConfirmation: $showSaveConfirmation
                            )
                            .padding(.horizontal, 12)
                            .padding(.bottom, 12)
                        }

                        if provider != allowedProviders.last {
                            Divider().padding(.horizontal, 12)
                        }
                    }
                }

                // Azure Voice Services (collapsed by default)
                let azureVoiceProviders = APIProvider.allCases.filter { $0.isAzureVoice }
                if !azureVoiceProviders.isEmpty {
                    SettingsSection(title: "Azure Voice Services", icon: "waveform") {
                        ForEach(azureVoiceProviders) { provider in
                            SettingsRow {
                                HStack(spacing: 12) {
                                    Image(systemName: provider.icon)
                                        .font(.title3)
                                        .foregroundStyle(keychainManager.getKey(for: provider) != nil ? .green : .secondary)
                                        .frame(width: 28)

                                    VStack(alignment: .leading, spacing: 2) {
                                        Text(provider.displayName)
                                            .font(.body.weight(.medium))
                                        if keychainManager.getKey(for: provider) != nil {
                                            Text("Configured")
                                                .font(.caption)
                                                .foregroundStyle(.green)
                                        } else {
                                            Text("Not configured")
                                                .font(.caption)
                                                .foregroundStyle(.secondary)
                                        }
                                    }

                                    Spacer()

                                    if selectedProvider == provider {
                                        Image(systemName: "chevron.down")
                                            .foregroundStyle(.secondary)
                                    } else {
                                        Image(systemName: "chevron.right")
                                            .foregroundStyle(.tertiary)
                                    }
                                }
                                .contentShape(Rectangle())
                                .onTapGesture {
                                    withAnimation(.easeInOut(duration: 0.2)) {
                                        if selectedProvider == provider {
                                            selectedProvider = nil
                                        } else {
                                            selectedProvider = provider
                                            apiKeyInput = ""
                                            showApiKey = false
                                            showSaveConfirmation = false
                                        }
                                    }
                                }
                            }

                            if selectedProvider == provider {
                                ProviderConfigPanel(
                                    provider: provider,
                                    keychainManager: keychainManager,
                                    apiKeyInput: $apiKeyInput,
                                    showApiKey: $showApiKey,
                                    showSaveConfirmation: $showSaveConfirmation
                                )
                                .padding(.horizontal, 12)
                                .padding(.bottom, 12)
                            }

                            if provider != azureVoiceProviders.last {
                                Divider().padding(.horizontal, 12)
                            }
                        }
                    }
                }

                // Info Section
                SettingsSection(title: "Information", icon: "info.circle") {
                    SettingsRow {
                        HStack {
                            Text("API keys are stored securely in macOS Keychain")
                                .font(.caption)
                                .foregroundStyle(.secondary)
                            Spacer()
                        }
                    }
                }

                Spacer()
            }
            .padding(24)
        }
    }
}

// MARK: - Provider Configuration Panel

private struct ProviderConfigPanel: View {
    let provider: APIProvider
    @ObservedObject var keychainManager: KeychainManager
    @Binding var apiKeyInput: String
    @Binding var showApiKey: Bool
    @Binding var showSaveConfirmation: Bool

    // Azure-specific fields
    @State private var azureEndpoint: String = ""
    @State private var azureDeployment: String = ""

    @State private var isTestingConnection = false
    @State private var connectionStatus: ConnectionStatus = .unknown

    enum ConnectionStatus {
        case unknown, testing, success, failure
    }

    private var hasExistingKey: Bool {
        keychainManager.getKey(for: provider) != nil
    }

    private var maskedKey: String {
        keychainManager.getMaskedKey(for: provider) ?? ""
    }

    /// Check if this is the Azure OpenAI provider (which needs endpoint + key + deployment)
    private var isAzureOpenAI: Bool {
        provider == .azureOpenAI
    }

    /// Check if Azure is fully configured (all 3 fields)
    private var isAzureFullyConfigured: Bool {
        keychainManager.getKey(for: .azureOpenAI) != nil &&
        keychainManager.getKey(for: .azureRealtimeEndpoint) != nil &&
        keychainManager.getKey(for: .azureRealtimeDeployment) != nil
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            // Azure OpenAI requires special handling with 3 fields
            if isAzureOpenAI {
                azureOpenAIConfiguration
            } else {
                standardProviderConfiguration
            }

            // Available models
            VStack(alignment: .leading, spacing: 6) {
                Text("Available Models")
                    .font(.caption.weight(.medium))
                    .foregroundStyle(.secondary)

                FlowLayout(spacing: 6) {
                    ForEach(modelsForProvider, id: \.self) { model in
                        Text(model)
                            .font(.caption.monospaced())
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(Color.secondary.opacity(0.1))
                            .clipShape(RoundedRectangle(cornerRadius: 4))
                    }
                }
            }
        }
        .padding(12)
        .background(Color(nsColor: .windowBackgroundColor).opacity(0.5))
        .clipShape(RoundedRectangle(cornerRadius: 8))
        .onAppear {
            // Load existing Azure values
            if isAzureOpenAI {
                azureEndpoint = keychainManager.getKey(for: .azureRealtimeEndpoint) ?? ""
                azureDeployment = keychainManager.getKey(for: .azureRealtimeDeployment) ?? ""
            }
        }
    }

    // MARK: - Azure OpenAI Configuration (3 fields)

    @ViewBuilder
    private var azureOpenAIConfiguration: some View {
        // Status indicator
        if isAzureFullyConfigured {
            HStack {
                Label("Azure OpenAI Configured", systemImage: "checkmark.circle.fill")
                    .font(.caption.weight(.medium))
                    .foregroundStyle(.green)
                Spacer()
                Button("Clear All") {
                    keychainManager.deleteKey(for: .azureOpenAI)
                    keychainManager.deleteKey(for: .azureRealtimeEndpoint)
                    keychainManager.deleteKey(for: .azureRealtimeDeployment)
                    keychainManager.deleteKey(for: .azureRealtimeKey)
                    azureEndpoint = ""
                    azureDeployment = ""
                    apiKeyInput = ""
                    connectionStatus = .unknown
                }
                .font(.caption)
                .foregroundStyle(.red)
                .buttonStyle(.plain)
            }
            .padding(10)
            .background(Color.green.opacity(0.1))
            .clipShape(RoundedRectangle(cornerRadius: 6))
        }

        // Endpoint field
        VStack(alignment: .leading, spacing: 4) {
            Text("Endpoint")
                .font(.caption.weight(.medium))
                .foregroundStyle(.secondary)
            TextField("https://your-resource.openai.azure.com", text: $azureEndpoint)
                .textFieldStyle(.roundedBorder)
                .font(.body.monospaced())
            Text("Your Azure OpenAI resource endpoint")
                .font(.caption2)
                .foregroundStyle(.tertiary)
        }

        // API Key field
        VStack(alignment: .leading, spacing: 4) {
            Text("API Key")
                .font(.caption.weight(.medium))
                .foregroundStyle(.secondary)
            HStack(spacing: 8) {
                if showApiKey {
                    TextField("API Key", text: $apiKeyInput)
                        .textFieldStyle(.roundedBorder)
                        .font(.body.monospaced())
                } else {
                    SecureField("API Key", text: $apiKeyInput)
                        .textFieldStyle(.roundedBorder)
                }
                Button {
                    showApiKey.toggle()
                } label: {
                    Image(systemName: showApiKey ? "eye.slash" : "eye")
                }
                .buttonStyle(.borderless)
            }
        }

        // Deployment Name field
        VStack(alignment: .leading, spacing: 4) {
            Text("Deployment Name")
                .font(.caption.weight(.medium))
                .foregroundStyle(.secondary)
            TextField("gpt-4o or gpt-4o-realtime", text: $azureDeployment)
                .textFieldStyle(.roundedBorder)
                .font(.body.monospaced())
            Text("The name of your deployed model")
                .font(.caption2)
                .foregroundStyle(.tertiary)
        }

        // Save button
        HStack(spacing: 12) {
            Button("Save Azure Configuration") {
                saveAzureConfiguration()
            }
            .buttonStyle(.borderedProminent)
            .controlSize(.small)
            .disabled(azureEndpoint.isEmpty || apiKeyInput.isEmpty || azureDeployment.isEmpty)

            if showSaveConfirmation {
                Label("Saved!", systemImage: "checkmark.circle.fill")
                    .font(.caption)
                    .foregroundStyle(.green)
            }

            Spacer()

            if isAzureFullyConfigured {
                Button {
                    testConnection()
                } label: {
                    HStack(spacing: 4) {
                        if isTestingConnection {
                            ProgressView()
                                .scaleEffect(0.6)
                        }
                        Text("Test")
                    }
                }
                .buttonStyle(.bordered)
                .controlSize(.small)
                .disabled(isTestingConnection)

                connectionStatusView
            }
        }
    }

    // MARK: - Standard Provider Configuration (single API key)

    @ViewBuilder
    private var standardProviderConfiguration: some View {
        // Current key status
        if hasExistingKey {
            HStack {
                Label("Current Key", systemImage: "key.fill")
                    .font(.caption.weight(.medium))
                    .foregroundStyle(.secondary)
                Spacer()
                Text(maskedKey)
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
                Button("Remove") {
                    keychainManager.deleteKey(for: provider)
                    apiKeyInput = ""
                    connectionStatus = .unknown
                }
                .font(.caption)
                .foregroundStyle(.red)
                .buttonStyle(.plain)
            }
            .padding(10)
            .background(Color.green.opacity(0.1))
            .clipShape(RoundedRectangle(cornerRadius: 6))
        }

        // API Key input (if required)
        if provider.requiresAPIKey {
            VStack(alignment: .leading, spacing: 8) {
                Text(hasExistingKey ? "Replace API Key" : "Enter API Key")
                    .font(.caption.weight(.medium))
                    .foregroundStyle(.secondary)

                HStack(spacing: 8) {
                    if showApiKey {
                        TextField("API Key", text: $apiKeyInput)
                            .textFieldStyle(.roundedBorder)
                            .font(.body.monospaced())
                    } else {
                        SecureField("API Key", text: $apiKeyInput)
                            .textFieldStyle(.roundedBorder)
                    }

                    Button {
                        showApiKey.toggle()
                    } label: {
                        Image(systemName: showApiKey ? "eye.slash" : "eye")
                    }
                    .buttonStyle(.borderless)
                }

                // Validation warning
                if !apiKeyInput.isEmpty && !keychainManager.isValidKeyFormat(apiKeyInput, for: provider) {
                    Label("Key format may be invalid for \(provider.displayName)", systemImage: "exclamationmark.triangle")
                        .font(.caption)
                        .foregroundStyle(.orange)
                }
            }

            // Action buttons
            HStack(spacing: 12) {
                Button("Save Key") {
                    saveKey()
                }
                .buttonStyle(.borderedProminent)
                .controlSize(.small)
                .disabled(apiKeyInput.isEmpty)

                if showSaveConfirmation {
                    Label("Saved!", systemImage: "checkmark.circle.fill")
                        .font(.caption)
                        .foregroundStyle(.green)
                }

                Spacer()

                if hasExistingKey {
                    Button {
                        testConnection()
                    } label: {
                        HStack(spacing: 4) {
                            if isTestingConnection {
                                ProgressView()
                                    .scaleEffect(0.6)
                            }
                            Text("Test")
                        }
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.small)
                    .disabled(isTestingConnection)

                    connectionStatusView
                }
            }
        } else {
            // For providers like Ollama that don't need API key
            HStack {
                Label("No API key required", systemImage: "checkmark.circle")
                    .font(.caption)
                    .foregroundStyle(.blue)
                Spacer()
            }
        }
    }

    private var modelsForProvider: [String] {
        switch provider {
        case .anthropic:
            return ["claude-sonnet-4-20250514", "claude-3-5-sonnet", "claude-3-opus"]
        case .openai:
            return ["gpt-4o", "gpt-4o-mini", "gpt-4-turbo"]
        case .azureOpenAI:
            return ["gpt-4o", "gpt-4o-mini", "gpt-4-turbo"]
        case .gemini:
            return ["gemini-2.0-flash", "gemini-1.5-pro", "gemini-1.5-flash"]
        case .openrouter:
            return ["claude-3.5-sonnet", "gpt-4o", "gemini-pro"]
        case .perplexity:
            return ["sonar-large", "sonar-small"]
        case .grok:
            return ["grok-2", "grok-2-mini"]
        case .ollama:
            return ["llama3.3", "mistral", "codellama", "phi3"]
        case .azureRealtimeKey, .azureRealtimeEndpoint, .azureRealtimeDeployment:
            return ["gpt-4o-realtime"]
        }
    }

    @ViewBuilder
    private var connectionStatusView: some View {
        switch connectionStatus {
        case .unknown:
            EmptyView()
        case .testing:
            Text("Testing...")
                .font(.caption)
                .foregroundStyle(.secondary)
        case .success:
            Label("OK", systemImage: "checkmark.circle.fill")
                .font(.caption)
                .foregroundStyle(.green)
        case .failure:
            Label("Failed", systemImage: "xmark.circle.fill")
                .font(.caption)
                .foregroundStyle(.red)
        }
    }

    private func saveKey() {
        guard !apiKeyInput.isEmpty else { return }

        if keychainManager.saveKey(apiKeyInput, for: provider) {
            apiKeyInput = ""
            showSaveConfirmation = true
            logInfo("Saved \(provider.displayName) API key", category: "Settings")

            DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
                withAnimation {
                    showSaveConfirmation = false
                }
            }
        }
    }

    private func saveAzureConfiguration() {
        guard !azureEndpoint.isEmpty, !apiKeyInput.isEmpty, !azureDeployment.isEmpty else { return }

        // Save all 3 Azure fields
        let endpointSaved = keychainManager.saveKey(azureEndpoint, for: .azureRealtimeEndpoint)
        let keySaved = keychainManager.saveKey(apiKeyInput, for: .azureOpenAI)
        let deploymentSaved = keychainManager.saveKey(azureDeployment, for: .azureRealtimeDeployment)

        // Also save to azureRealtimeKey for voice functionality
        _ = keychainManager.saveKey(apiKeyInput, for: .azureRealtimeKey)

        if endpointSaved && keySaved && deploymentSaved {
            apiKeyInput = ""
            showSaveConfirmation = true
            logInfo("Saved Azure OpenAI configuration (endpoint, key, deployment)", category: "Settings")

            DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
                withAnimation {
                    showSaveConfirmation = false
                }
            }
        } else {
            logError("Failed to save Azure OpenAI configuration", category: "Settings")
        }
    }

    private func testConnection() {
        guard hasExistingKey else { return }

        isTestingConnection = true
        connectionStatus = .testing

        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            isTestingConnection = false
            if let key = keychainManager.getKey(for: provider),
               keychainManager.isValidKeyFormat(key, for: provider) {
                connectionStatus = .success
            } else {
                connectionStatus = .failure
            }
        }
    }
}

// MARK: - Flow Layout for Tags

private struct FlowLayout: Layout {
    var spacing: CGFloat = 8

    func sizeThatFits(proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) -> CGSize {
        let result = FlowResult(in: proposal.width ?? 0, spacing: spacing, subviews: subviews)
        return result.size
    }

    func placeSubviews(in bounds: CGRect, proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) {
        let result = FlowResult(in: bounds.width, spacing: spacing, subviews: subviews)
        for (index, subview) in subviews.enumerated() {
            subview.place(at: CGPoint(x: bounds.minX + result.positions[index].x,
                                      y: bounds.minY + result.positions[index].y),
                         proposal: .unspecified)
        }
    }

    struct FlowResult {
        var size: CGSize = .zero
        var positions: [CGPoint] = []

        init(in maxWidth: CGFloat, spacing: CGFloat, subviews: Subviews) {
            var x: CGFloat = 0
            var y: CGFloat = 0
            var rowHeight: CGFloat = 0

            for subview in subviews {
                let size = subview.sizeThatFits(.unspecified)

                if x + size.width > maxWidth && x > 0 {
                    x = 0
                    y += rowHeight + spacing
                    rowHeight = 0
                }

                positions.append(CGPoint(x: x, y: y))
                rowHeight = max(rowHeight, size.height)
                x += size.width + spacing
            }

            self.size = CGSize(width: maxWidth, height: y + rowHeight)
        }
    }
}

// MARK: - Budget Settings Tab

struct BudgetSettingsTab: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel
    @State private var sessionBudget: Double = 10.0
    @State private var monthlyBudget: Double = 100.0
    @State private var warningThreshold: Double = 80.0
    @State private var pauseOnExceed = true

    var body: some View {
        Form {
            Section("Session Budget") {
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        Text("Limit:")
                        Spacer()
                        Text(String(format: "$%.0f", sessionBudget))
                            .font(.headline.monospacedDigit())
                    }

                    Slider(value: $sessionBudget, in: 1...100, step: 1)

                    HStack {
                        Text("$1")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                        Spacer()
                        Text("$100")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                }

                Button("Apply Session Budget") {
                    orchestratorVM.setBudget(sessionBudget)
                }
                .buttonStyle(.borderedProminent)
            }

            Section("Monthly Budget") {
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        Text("Monthly Limit:")
                        Spacer()
                        Text(String(format: "$%.0f", monthlyBudget))
                            .font(.headline.monospacedDigit())
                    }

                    Slider(value: $monthlyBudget, in: 10...500, step: 10)
                }
            }

            Section("Budget Alerts") {
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        Text("Warning at:")
                        Spacer()
                        Text(String(format: "%.0f%%", warningThreshold))
                            .font(.headline.monospacedDigit())
                    }

                    Slider(value: $warningThreshold, in: 50...95, step: 5)
                }

                Toggle("Pause requests when budget exceeded", isOn: $pauseOnExceed)
            }

            Section("Current Usage") {
                HStack {
                    Text("Session Cost:")
                    Spacer()
                    Text(String(format: "$%.4f", orchestratorVM.costInfo.sessionCost))
                        .font(.body.monospacedDigit())
                }

                HStack {
                    Text("Total Cost:")
                    Spacer()
                    Text(String(format: "$%.2f", orchestratorVM.costInfo.totalCost))
                        .font(.body.monospacedDigit())
                }

                BudgetProgressIndicator(
                    spent: orchestratorVM.costInfo.sessionCost,
                    limit: orchestratorVM.costInfo.budgetLimit
                )
            }
        }
        .padding()
        .onAppear {
            sessionBudget = orchestratorVM.costInfo.budgetLimit
        }
    }
}

private struct BudgetProgressIndicator: View {
    let spent: Double
    let limit: Double

    private var progress: Double {
        guard limit > 0 else { return 0 }
        return min(1.0, spent / limit)
    }

    private var color: Color {
        if progress > 0.9 { return .red }
        if progress > 0.7 { return .orange }
        return .green
    }

    var body: some View {
        VStack(spacing: 4) {
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    RoundedRectangle(cornerRadius: 4)
                        .fill(Color.gray.opacity(0.2))

                    RoundedRectangle(cornerRadius: 4)
                        .fill(color)
                        .frame(width: geometry.size.width * progress)
                }
            }
            .frame(height: 8)

            HStack {
                Text(String(format: "$%.2f spent", spent))
                    .font(.caption)
                    .foregroundStyle(.secondary)
                Spacer()
                Text(String(format: "$%.0f limit", limit))
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
    }
}

// MARK: - Appearance Settings Tab

struct AppearanceSettingsTab: View {
    @AppStorage("appearanceMode") private var appearanceMode = "system"
    @AppStorage("accentColor") private var accentColor = "purple"
    @AppStorage("compactMode") private var compactMode = false
    @AppStorage("showAgentAvatars") private var showAgentAvatars = true
    @AppStorage("animationsEnabled") private var animationsEnabled = true

    private let accentColors: [(name: String, color: Color)] = [
        ("purple", .purple),
        ("blue", .blue),
        ("green", .green),
        ("orange", .orange),
        ("pink", .pink),
        ("teal", .teal)
    ]

    var body: some View {
        ScrollView {
            VStack(spacing: 24) {
                // Theme Section
                SettingsSection(title: "Theme", icon: "moon.fill") {
                    SettingsRow {
                        HStack {
                            Text("Appearance")
                                .font(.body)
                            Spacer()
                            Picker("", selection: $appearanceMode) {
                                Label("System", systemImage: "gear").tag("system")
                                Label("Light", systemImage: "sun.max").tag("light")
                                Label("Dark", systemImage: "moon").tag("dark")
                            }
                            .pickerStyle(.segmented)
                            .frame(width: 240)
                        }
                    }
                }

                // Accent Color Section
                SettingsSection(title: "Accent Color", icon: "paintpalette.fill") {
                    SettingsRow {
                        VStack(alignment: .leading, spacing: 12) {
                            Text("Choose your accent color")
                                .font(.caption)
                                .foregroundStyle(.secondary)

                            HStack(spacing: 16) {
                                ForEach(accentColors, id: \.name) { item in
                                    VStack(spacing: 6) {
                                        Circle()
                                            .fill(item.color.gradient)
                                            .frame(width: 36, height: 36)
                                            .overlay(
                                                Circle()
                                                    .stroke(Color.primary, lineWidth: accentColor == item.name ? 3 : 0)
                                                    .padding(2)
                                            )
                                            .shadow(color: item.color.opacity(0.3), radius: accentColor == item.name ? 4 : 0)
                                            .onTapGesture {
                                                withAnimation(.easeInOut(duration: 0.2)) {
                                                    accentColor = item.name
                                                }
                                            }

                                        Text(item.name.capitalized)
                                            .font(.caption2)
                                            .foregroundStyle(accentColor == item.name ? .primary : .secondary)
                                    }
                                }
                            }
                        }
                    }
                }

                // Layout Section
                SettingsSection(title: "Layout", icon: "rectangle.3.group") {
                    SettingsRow {
                        Toggle(isOn: $compactMode) {
                            VStack(alignment: .leading, spacing: 2) {
                                Text("Compact Mode")
                                Text("Reduces spacing and padding throughout the app")
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }

                    Divider().padding(.horizontal, 12)

                    SettingsRow {
                        Toggle(isOn: $showAgentAvatars) {
                            VStack(alignment: .leading, spacing: 2) {
                                Text("Show Agent Avatars")
                                Text("Display agent profile images in conversations")
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }
                }

                // Effects Section
                SettingsSection(title: "Effects", icon: "sparkles") {
                    SettingsRow {
                        Toggle(isOn: $animationsEnabled) {
                            VStack(alignment: .leading, spacing: 2) {
                                Text("Enable Animations")
                                Text("Typing indicators, streaming effects, and transitions")
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }
                }

                Spacer()
            }
            .padding(24)
        }
    }
}

// MARK: - Shortcuts Settings Tab

struct ShortcutsSettingsTab: View {
    @State private var globalActivate = "Cmd + Shift + Space"
    @State private var newConversation = "Cmd + N"
    @State private var sendMessage = "Cmd + Return"
    @State private var cancelRequest = "Cmd + ."

    var body: some View {
        Form {
            Section("Global Shortcuts") {
                ShortcutRow(label: "Activate Convergio", shortcut: $globalActivate)
            }

            Section("Conversation") {
                ShortcutRow(label: "New Conversation", shortcut: $newConversation)
                ShortcutRow(label: "Send Message", shortcut: $sendMessage)
                ShortcutRow(label: "Cancel Request", shortcut: $cancelRequest)
            }

            Section {
                Text("Global shortcuts work even when Convergio is not in focus. Make sure the shortcut doesn't conflict with other applications.")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
        .padding()
    }
}

private struct ShortcutRow: View {
    let label: String
    @Binding var shortcut: String
    @State private var isRecording = false

    var body: some View {
        HStack {
            Text(label)
            Spacer()
            Button(isRecording ? "Press keys..." : shortcut) {
                isRecording.toggle()
            }
            .buttonStyle(.bordered)
            .frame(width: 150)
        }
    }
}

// MARK: - Advanced Settings Tab

struct AdvancedSettingsTab: View {
    @AppStorage("debugMode") private var debugMode = false
    @AppStorage("logLevel") private var logLevel = "info"
    @AppStorage("contextWindow") private var contextWindow = 100000
    @AppStorage("maxTokensPerRequest") private var maxTokensPerRequest = 4096

    var body: some View {
        Form {
            Section("Developer Options") {
                Toggle("Debug Mode", isOn: $debugMode)

                if debugMode {
                    Picker("Log Level", selection: $logLevel) {
                        Text("Error").tag("error")
                        Text("Warning").tag("warning")
                        Text("Info").tag("info")
                        Text("Debug").tag("debug")
                        Text("Trace").tag("trace")
                    }
                }
            }

            Section("Model Parameters") {
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        Text("Context Window:")
                        Spacer()
                        Text("\(contextWindow / 1000)K tokens")
                            .font(.body.monospacedDigit())
                    }

                    Slider(
                        value: Binding(
                            get: { Double(contextWindow) },
                            set: { contextWindow = Int($0) }
                        ),
                        in: 8000...200000,
                        step: 8000
                    )
                }

                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        Text("Max Output Tokens:")
                        Spacer()
                        Text("\(maxTokensPerRequest)")
                            .font(.body.monospacedDigit())
                    }

                    Slider(
                        value: Binding(
                            get: { Double(maxTokensPerRequest) },
                            set: { maxTokensPerRequest = Int($0) }
                        ),
                        in: 256...16384,
                        step: 256
                    )
                }
            }

            Section("Data") {
                HStack {
                    Text("Export Data")
                    Spacer()
                    Button("Export...") {
                        // TODO: Implement data export
                    }
                }

                HStack {
                    Text("Reset All Settings")
                    Spacer()
                    Button("Reset", role: .destructive) {
                        // TODO: Implement settings reset
                    }
                }
            }

            Section {
                HStack {
                    Text("Version")
                    Spacer()
                    Text("1.0.0 (Build 1)")
                        .foregroundStyle(.secondary)
                }

                HStack {
                    Text("Core Library")
                    Spacer()
                    Text("ConvergioCore 5.0.0")
                        .foregroundStyle(.secondary)
                }
            }
        }
        .padding()
    }
}

// MARK: - MCP Services Settings Tab

/// Model Context Protocol server configuration
struct MCPServerConfig: Identifiable, Codable, Equatable, Hashable {
    let id: UUID
    var name: String
    var command: String  // Command to launch the server (e.g., "npx -y @modelcontextprotocol/server-filesystem")
    var args: [String]   // Command arguments
    var env: [String: String]  // Environment variables
    var isEnabled: Bool
    var transportType: MCPTransportType

    init(id: UUID = UUID(), name: String = "", command: String = "", args: [String] = [], env: [String: String] = [:], isEnabled: Bool = true, transportType: MCPTransportType = .stdio) {
        self.id = id
        self.name = name
        self.command = command
        self.args = args
        self.env = env
        self.isEnabled = isEnabled
        self.transportType = transportType
    }

    func hash(into hasher: inout Hasher) {
        hasher.combine(id)
    }
}

enum MCPTransportType: String, Codable, CaseIterable {
    case stdio = "stdio"
    case sse = "sse"
    case http = "http"

    var displayName: String {
        switch self {
        case .stdio: return "Standard I/O"
        case .sse: return "Server-Sent Events"
        case .http: return "HTTP"
        }
    }

    var icon: String {
        switch self {
        case .stdio: return "terminal"
        case .sse: return "antenna.radiowaves.left.and.right"
        case .http: return "globe"
        }
    }
}

/// Manager for MCP server configurations
class MCPConfigManager: ObservableObject {
    static let shared = MCPConfigManager()

    @Published var servers: [MCPServerConfig] = []

    private let configKey = "mcp_servers_config"

    init() {
        loadServers()
    }

    func loadServers() {
        if let data = UserDefaults.standard.data(forKey: configKey),
           let decoded = try? JSONDecoder().decode([MCPServerConfig].self, from: data) {
            servers = decoded
        } else {
            // Add default servers as examples
            servers = [
                MCPServerConfig(
                    name: "Filesystem",
                    command: "npx",
                    args: ["-y", "@modelcontextprotocol/server-filesystem", "/tmp"],
                    isEnabled: false
                ),
                MCPServerConfig(
                    name: "GitHub",
                    command: "npx",
                    args: ["-y", "@modelcontextprotocol/server-github"],
                    env: ["GITHUB_TOKEN": ""],
                    isEnabled: false
                ),
                MCPServerConfig(
                    name: "Memory",
                    command: "npx",
                    args: ["-y", "@modelcontextprotocol/server-memory"],
                    isEnabled: false
                )
            ]
        }
    }

    func saveServers() {
        if let encoded = try? JSONEncoder().encode(servers) {
            UserDefaults.standard.set(encoded, forKey: configKey)
        }
    }

    func addServer(_ server: MCPServerConfig) {
        servers.append(server)
        saveServers()
    }

    func removeServer(_ server: MCPServerConfig) {
        servers.removeAll { $0.id == server.id }
        saveServers()
    }

    func updateServer(_ server: MCPServerConfig) {
        if let index = servers.firstIndex(where: { $0.id == server.id }) {
            servers[index] = server
            saveServers()
        }
    }

    func toggleServer(_ server: MCPServerConfig) {
        if let index = servers.firstIndex(where: { $0.id == server.id }) {
            servers[index].isEnabled.toggle()
            saveServers()
        }
    }
}

struct MCPServicesSettingsTab: View {
    @StateObject private var configManager = MCPConfigManager.shared
    @State private var selectedServer: MCPServerConfig?
    @State private var showAddSheet = false
    @State private var editingServer: MCPServerConfig?

    var body: some View {
        HSplitView {
            // Server list
            VStack(spacing: 0) {
                List(selection: $selectedServer) {
                    ForEach(configManager.servers) { server in
                        MCPServerRow(server: server, configManager: configManager)
                            .tag(server)
                    }
                }
                .listStyle(.sidebar)

                Divider()

                // Add/Remove buttons
                HStack(spacing: 8) {
                    Button {
                        showAddSheet = true
                    } label: {
                        Image(systemName: "plus")
                    }
                    .buttonStyle(.borderless)
                    .help("Add new MCP server")

                    Button {
                        if let server = selectedServer {
                            configManager.removeServer(server)
                            selectedServer = nil
                        }
                    } label: {
                        Image(systemName: "minus")
                    }
                    .buttonStyle(.borderless)
                    .disabled(selectedServer == nil)
                    .help("Remove selected server")

                    Spacer()
                }
                .padding(8)
            }
            .frame(minWidth: 200, maxWidth: 250)

            // Detail view
            if let server = selectedServer {
                MCPServerDetailView(
                    server: server,
                    configManager: configManager,
                    onEdit: { editingServer = $0 }
                )
            } else {
                VStack(spacing: 16) {
                    Image(systemName: "server.rack")
                        .font(.system(size: 48))
                        .foregroundStyle(.secondary)
                    Text("Select a server to view details")
                        .foregroundStyle(.secondary)
                    Text("MCP (Model Context Protocol) servers provide tools and resources to AI models.")
                        .font(.caption)
                        .foregroundStyle(.tertiary)
                        .multilineTextAlignment(.center)
                        .frame(maxWidth: 200)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        .sheet(isPresented: $showAddSheet) {
            MCPServerEditorSheet(
                server: MCPServerConfig(),
                isNew: true,
                onSave: { newServer in
                    configManager.addServer(newServer)
                    selectedServer = newServer
                }
            )
        }
        .sheet(item: $editingServer) { server in
            MCPServerEditorSheet(
                server: server,
                isNew: false,
                onSave: { updatedServer in
                    configManager.updateServer(updatedServer)
                    selectedServer = updatedServer
                }
            )
        }
    }
}

private struct MCPServerRow: View {
    let server: MCPServerConfig
    @ObservedObject var configManager: MCPConfigManager

    var body: some View {
        HStack(spacing: 10) {
            Image(systemName: server.transportType.icon)
                .foregroundStyle(server.isEnabled ? .green : .secondary)
                .frame(width: 20)

            VStack(alignment: .leading, spacing: 2) {
                Text(server.name.isEmpty ? "Unnamed Server" : server.name)
                    .font(.headline)
                Text(server.command)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .lineLimit(1)
            }

            Spacer()

            Toggle("", isOn: Binding(
                get: { server.isEnabled },
                set: { _ in configManager.toggleServer(server) }
            ))
            .toggleStyle(.switch)
            .controlSize(.small)
        }
        .padding(.vertical, 4)
    }
}

private struct MCPServerDetailView: View {
    let server: MCPServerConfig
    @ObservedObject var configManager: MCPConfigManager
    var onEdit: (MCPServerConfig) -> Void

    @State private var testStatus: TestStatus = .idle
    @State private var testMessage = ""

    enum TestStatus {
        case idle, testing, success, failure
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                // Header
                HStack {
                    VStack(alignment: .leading, spacing: 4) {
                        Text(server.name.isEmpty ? "Unnamed Server" : server.name)
                            .font(.title2.bold())
                        HStack(spacing: 8) {
                            Label(server.transportType.displayName, systemImage: server.transportType.icon)
                                .font(.caption)
                                .padding(.horizontal, 8)
                                .padding(.vertical, 4)
                                .background(Color.secondary.opacity(0.1))
                                .clipShape(Capsule())

                            if server.isEnabled {
                                Label("Enabled", systemImage: "checkmark.circle.fill")
                                    .font(.caption)
                                    .foregroundStyle(.green)
                            } else {
                                Label("Disabled", systemImage: "xmark.circle")
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }

                    Spacer()

                    Button("Edit") {
                        onEdit(server)
                    }
                    .buttonStyle(.bordered)
                }

                Divider()

                // Command section
                GroupBox("Command") {
                    VStack(alignment: .leading, spacing: 8) {
                        HStack {
                            Text("Executable:")
                                .foregroundStyle(.secondary)
                            Text(server.command)
                                .font(.body.monospaced())
                        }

                        if !server.args.isEmpty {
                            HStack(alignment: .top) {
                                Text("Arguments:")
                                    .foregroundStyle(.secondary)
                                VStack(alignment: .leading, spacing: 2) {
                                    ForEach(server.args, id: \.self) { arg in
                                        Text(arg)
                                            .font(.body.monospaced())
                                    }
                                }
                            }
                        }
                    }
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(4)
                }

                // Environment variables
                if !server.env.isEmpty {
                    GroupBox("Environment Variables") {
                        VStack(alignment: .leading, spacing: 4) {
                            ForEach(Array(server.env.keys.sorted()), id: \.self) { key in
                                HStack {
                                    Text(key)
                                        .font(.body.monospaced())
                                        .foregroundStyle(.secondary)
                                    Text("=")
                                        .foregroundStyle(.tertiary)
                                    if server.env[key]?.isEmpty ?? true {
                                        Text("(not set)")
                                            .font(.caption)
                                            .foregroundStyle(.orange)
                                    } else {
                                        Text(maskValue(server.env[key] ?? ""))
                                            .font(.body.monospaced())
                                    }
                                }
                            }
                        }
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .padding(4)
                    }
                }

                // Test connection
                GroupBox("Connection Test") {
                    HStack {
                        Button {
                            testConnection()
                        } label: {
                            HStack {
                                if testStatus == .testing {
                                    ProgressView()
                                        .scaleEffect(0.7)
                                }
                                Text("Test Connection")
                            }
                        }
                        .disabled(testStatus == .testing)

                        Spacer()

                        switch testStatus {
                        case .idle:
                            EmptyView()
                        case .testing:
                            Text("Testing...")
                                .foregroundStyle(.secondary)
                        case .success:
                            Label("Connected", systemImage: "checkmark.circle.fill")
                                .foregroundStyle(.green)
                        case .failure:
                            Label(testMessage, systemImage: "xmark.circle.fill")
                                .foregroundStyle(.red)
                        }
                    }
                    .padding(4)
                }

                Spacer()
            }
            .padding()
        }
    }

    private func maskValue(_ value: String) -> String {
        guard value.count > 4 else { return "****" }
        return String(value.prefix(4)) + String(repeating: "*", count: min(20, value.count - 4))
    }

    private func testConnection() {
        testStatus = .testing
        testMessage = ""

        // Simulate connection test
        // In real implementation, this would attempt to launch the MCP server
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.5) {
            // Check if command exists
            let process = Process()
            process.executableURL = URL(fileURLWithPath: "/usr/bin/which")
            process.arguments = [server.command]

            let pipe = Pipe()
            process.standardOutput = pipe
            process.standardError = pipe

            do {
                try process.run()
                process.waitUntilExit()

                if process.terminationStatus == 0 {
                    testStatus = .success
                } else {
                    testStatus = .failure
                    testMessage = "Command not found"
                }
            } catch {
                testStatus = .failure
                testMessage = "Error: \(error.localizedDescription)"
            }
        }
    }
}

private struct MCPServerEditorSheet: View {
    @Environment(\.dismiss) private var dismiss

    @State var server: MCPServerConfig
    let isNew: Bool
    var onSave: (MCPServerConfig) -> Void

    @State private var argsText = ""
    @State private var envKeyInput = ""
    @State private var envValueInput = ""

    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text(isNew ? "Add MCP Server" : "Edit MCP Server")
                    .font(.headline)
                Spacer()
                Button("Cancel") {
                    dismiss()
                }
                .keyboardShortcut(.cancelAction)
            }
            .padding()

            Divider()

            // Form
            Form {
                Section("Server Information") {
                    TextField("Name", text: $server.name, prompt: Text("e.g., Filesystem, GitHub"))

                    Picker("Transport", selection: $server.transportType) {
                        ForEach(MCPTransportType.allCases, id: \.self) { type in
                            Label(type.displayName, systemImage: type.icon).tag(type)
                        }
                    }
                }

                Section("Command") {
                    TextField("Command", text: $server.command, prompt: Text("e.g., npx, node, python"))

                    VStack(alignment: .leading, spacing: 8) {
                        Text("Arguments (one per line)")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                        TextEditor(text: $argsText)
                            .font(.body.monospaced())
                            .frame(height: 80)
                            .border(Color.secondary.opacity(0.3))
                    }
                }

                Section("Environment Variables") {
                    ForEach(Array(server.env.keys.sorted()), id: \.self) { key in
                        HStack {
                            Text(key)
                                .font(.body.monospaced())
                            Spacer()
                            SecureField("Value", text: Binding(
                                get: { server.env[key] ?? "" },
                                set: { server.env[key] = $0 }
                            ))
                            .frame(width: 200)
                            Button {
                                server.env.removeValue(forKey: key)
                            } label: {
                                Image(systemName: "minus.circle.fill")
                                    .foregroundStyle(.red)
                            }
                            .buttonStyle(.plain)
                        }
                    }

                    HStack {
                        TextField("Key", text: $envKeyInput)
                            .frame(width: 120)
                        SecureField("Value", text: $envValueInput)
                        Button {
                            if !envKeyInput.isEmpty {
                                server.env[envKeyInput] = envValueInput
                                envKeyInput = ""
                                envValueInput = ""
                            }
                        } label: {
                            Image(systemName: "plus.circle.fill")
                                .foregroundStyle(.green)
                        }
                        .buttonStyle(.plain)
                        .disabled(envKeyInput.isEmpty)
                    }
                }

                Section {
                    Toggle("Enable this server", isOn: $server.isEnabled)
                }
            }
            .formStyle(.grouped)
            .scrollContentBackground(.hidden)

            Divider()

            // Footer
            HStack {
                Spacer()

                Button("Save") {
                    // Parse args from text
                    server.args = argsText.components(separatedBy: .newlines).filter { !$0.isEmpty }
                    onSave(server)
                    dismiss()
                }
                .keyboardShortcut(.defaultAction)
                .disabled(server.name.isEmpty || server.command.isEmpty)
            }
            .padding()
        }
        .frame(width: 500, height: 550)
        .onAppear {
            argsText = server.args.joined(separator: "\n")
        }
    }
}

// MARK: - Student Profile Settings Tab

struct StudentProfileSettingsTab: View {
    @StateObject private var profileManager = StudentProfileManager.shared

    var body: some View {
        Form {
            if let profile = profileManager.currentProfile {
                Section("Student Information") {
                    HStack {
                        Text("Name:")
                        Spacer()
                        Text(profile.firstName)
                            .foregroundStyle(.secondary)
                    }

                    HStack {
                        Text("Age:")
                        Spacer()
                        Text("\(profile.age) years")
                            .foregroundStyle(.secondary)
                    }

                    HStack {
                        Text("School Year:")
                        Spacer()
                        Text(profile.schoolYear.displayName)
                            .foregroundStyle(.secondary)
                    }

                    HStack {
                        Text("Curriculum:")
                        Spacer()
                        HStack(spacing: 6) {
                            Image(systemName: profile.curriculum.icon)
                                .foregroundStyle(curriculumColor(profile.curriculum))
                            Text(profile.curriculum.displayName)
                        }
                        .foregroundStyle(.secondary)
                    }
                }

                Section("Accessibility Preferences") {
                    HStack {
                        Text("Font Size:")
                        Spacer()
                        Text(profile.accessibilitySettings.fontSize.displayName)
                            .foregroundStyle(.secondary)
                    }

                    HStack {
                        Image(systemName: profile.accessibilitySettings.dyslexiaFont ? "checkmark.circle.fill" : "circle")
                            .foregroundStyle(profile.accessibilitySettings.dyslexiaFont ? .green : .secondary)
                        Text("Dyslexia-Friendly Font")
                    }

                    HStack {
                        Image(systemName: profile.accessibilitySettings.highContrast ? "checkmark.circle.fill" : "circle")
                            .foregroundStyle(profile.accessibilitySettings.highContrast ? .green : .secondary)
                        Text("High Contrast Mode")
                    }

                    HStack {
                        Image(systemName: profile.accessibilitySettings.voiceEnabled ? "checkmark.circle.fill" : "circle")
                            .foregroundStyle(profile.accessibilitySettings.voiceEnabled ? .green : .secondary)
                        Text("Voice Interaction")
                    }

                    HStack {
                        Image(systemName: profile.accessibilitySettings.simplifiedLanguage ? "checkmark.circle.fill" : "circle")
                            .foregroundStyle(profile.accessibilitySettings.simplifiedLanguage ? .green : .secondary)
                        Text("Simplified Language")
                    }
                }

                Section {
                    Button("Edit Profile") {
                        // TODO: Open profile editor sheet
                    }
                    .buttonStyle(.borderedProminent)

                    Button("Reset Profile", role: .destructive) {
                        profileManager.deleteProfile()
                    }
                    .buttonStyle(.bordered)
                }
            } else {
                VStack(spacing: 20) {
                    Image(systemName: "person.crop.circle.badge.questionmark")
                        .font(.system(size: 48))
                        .foregroundStyle(.secondary)

                    Text("No Student Profile")
                        .font(.headline)

                    Text("Complete the onboarding flow to create your student profile.")
                        .font(.subheadline)
                        .foregroundStyle(.secondary)
                        .multilineTextAlignment(.center)

                    Button("Start Onboarding") {
                        // Post notification to show onboarding
                        NotificationCenter.default.post(name: .showOnboarding, object: nil)
                    }
                    .buttonStyle(.borderedProminent)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        .padding()
    }

    private func curriculumColor(_ curriculum: Curriculum) -> Color {
        switch curriculum.color {
        case "purple": return .purple
        case "blue": return .blue
        case "green": return .green
        case "orange": return .orange
        case "red": return .red
        case "indigo": return .indigo
        case "brown": return .brown
        case "cyan": return .cyan
        case "pink": return .pink
        default: return .secondary
        }
    }
}

extension Notification.Name {
    static let showOnboarding = Notification.Name("com.convergio.showOnboarding")
}

// MARK: - SettingsView Wrapper (for Scene)

struct SettingsView: View {
    var body: some View {
        MainSettingsView()
    }
}

// MARK: - Preview

#Preview("Settings") {
    MainSettingsView()
        .environmentObject(OrchestratorViewModel.preview)
}
