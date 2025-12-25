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

    var body: some View {
        Form {
            Section("Startup") {
                Toggle("Launch at Login", isOn: $launchAtLogin)
                Toggle("Show in Menu Bar", isOn: $showInMenuBar)
            }

            Section("Notifications") {
                Toggle("Enable Notifications", isOn: $notificationsEnabled)
                Toggle("Sound Effects", isOn: $soundEffectsEnabled)

                if notificationsEnabled {
                    HStack {
                        Text("Notify on:")
                        Spacer()
                        VStack(alignment: .leading, spacing: 4) {
                            Text("• Task completion")
                            Text("• Budget warnings")
                            Text("• Agent collaboration")
                        }
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    }
                }
            }

            Section("Privacy") {
                HStack {
                    Text("Conversation History")
                    Spacer()
                    Button("Clear History") {
                        // TODO: Implement history clearing
                    }
                    .buttonStyle(.bordered)
                }

                HStack {
                    Text("Cached Data")
                    Spacer()
                    Button("Clear Cache") {
                        // TODO: Implement cache clearing
                    }
                    .buttonStyle(.bordered)
                }
            }
        }
        .padding()
    }
}

// MARK: - Providers Settings Tab

struct ProvidersSettingsTab: View {
    @ObservedObject var keychainManager = KeychainManager.shared
    @State private var selectedProvider: APIProvider = .openai

    /// Filter providers based on edition
    /// EDU edition only allows GDPR-compliant providers (Azure OpenAI via OpenAI key, no Anthropic)
    private var allowedProviders: [APIProvider] {
        let isEDU = EditionManager.shared.currentEdition == .education
        if isEDU {
            // EDU: Only OpenAI (Azure) and Gemini (Google EU), no Anthropic
            return APIProvider.allCases.filter { $0 != .anthropic }
        }
        return APIProvider.allCases
    }

    var body: some View {
        HSplitView {
            // Provider list - filtered by edition
            List(allowedProviders, selection: $selectedProvider) { provider in
                HStack(spacing: 12) {
                    Image(systemName: provider.icon)
                        .frame(width: 24)
                        .foregroundStyle(keychainManager.getKey(for: provider) != nil ? .green : .secondary)
                    VStack(alignment: .leading) {
                        Text(provider.displayName)
                            .font(.headline)
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
                }
                .padding(.vertical, 4)
            }
            .frame(minWidth: 150, maxWidth: 200)

            // Provider detail
            ProviderDetailView(provider: selectedProvider)
        }
        .padding()
    }
}

private struct ProviderDetailView: View {
    let provider: APIProvider
    @ObservedObject var keychainManager = KeychainManager.shared
    @State private var apiKeyInput = ""
    @State private var showApiKey = false
    @State private var isTestingConnection = false
    @State private var connectionStatus: ConnectionStatus = .unknown
    @State private var showSaveConfirmation = false

    enum ConnectionStatus {
        case unknown, testing, success, failure
    }

    private var hasExistingKey: Bool {
        keychainManager.getKey(for: provider) != nil
    }

    private var maskedKey: String {
        keychainManager.getMaskedKey(for: provider) ?? ""
    }

    var body: some View {
        Form {
            Section("API Configuration") {
                if hasExistingKey {
                    // Show existing key (masked)
                    HStack {
                        Text("Current Key:")
                            .foregroundStyle(.secondary)
                        Text(maskedKey)
                            .font(.body.monospaced())
                        Spacer()
                        Button("Remove") {
                            keychainManager.deleteKey(for: provider)
                            apiKeyInput = ""
                            connectionStatus = .unknown
                        }
                        .foregroundStyle(.red)
                    }

                    Divider()

                    Text("Enter new key to replace:")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }

                HStack {
                    if showApiKey {
                        TextField("API Key", text: $apiKeyInput)
                            .textFieldStyle(.roundedBorder)
                    } else {
                        SecureField("API Key", text: $apiKeyInput)
                            .textFieldStyle(.roundedBorder)
                    }
                    Button {
                        showApiKey.toggle()
                    } label: {
                        Image(systemName: showApiKey ? "eye.slash" : "eye")
                    }
                    .buttonStyle(.plain)
                }

                HStack {
                    Button("Save Key") {
                        saveKey()
                    }
                    .disabled(apiKeyInput.isEmpty)
                    .buttonStyle(.borderedProminent)

                    if showSaveConfirmation {
                        Label("Saved!", systemImage: "checkmark.circle.fill")
                            .foregroundStyle(.green)
                            .transition(.opacity)
                    }

                    Spacer()

                    Button("Test Connection") {
                        testConnection()
                    }
                    .disabled(!hasExistingKey || isTestingConnection)

                    if isTestingConnection {
                        ProgressView()
                            .scaleEffect(0.7)
                    }

                    connectionStatusView
                }
            }

            Section("Provider Info") {
                HStack {
                    Text("Environment Variable")
                    Spacer()
                    Text(provider.envVariable)
                        .font(.body.monospaced())
                        .foregroundStyle(.secondary)
                }

                if !keychainManager.isValidKeyFormat(apiKeyInput, for: provider) && !apiKeyInput.isEmpty {
                    Label("Key format may be invalid", systemImage: "exclamationmark.triangle")
                        .foregroundStyle(.orange)
                        .font(.caption)
                }
            }

            Section("Models") {
                ForEach(modelsForProvider, id: \.self) { model in
                    HStack {
                        Text(model)
                            .font(.body.monospaced())
                        Spacer()
                    }
                }
            }
        }
        .padding()
        .onChange(of: provider) { _, _ in
            apiKeyInput = ""
            connectionStatus = .unknown
            showSaveConfirmation = false
        }
    }

    private var modelsForProvider: [String] {
        switch provider {
        case .anthropic:
            return ["claude-sonnet-4-20250514", "claude-3-5-sonnet-20241022", "claude-3-opus-20240229"]
        case .openai:
            return ["gpt-4o", "gpt-4o-mini", "gpt-4-turbo"]
        case .azureOpenAI:
            return ["gpt-4o", "gpt-4o-mini", "gpt-4-turbo"]
        case .gemini:
            return ["gemini-2.0-flash", "gemini-1.5-pro", "gemini-1.5-flash"]
        case .openrouter:
            return ["anthropic/claude-3.5-sonnet", "openai/gpt-4o", "google/gemini-pro"]
        case .perplexity:
            return ["llama-3.1-sonar-large-128k-online", "llama-3.1-sonar-small-128k-online"]
        case .grok:
            return ["grok-2", "grok-2-mini"]
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
                .foregroundStyle(.secondary)
        case .success:
            Label("Connected", systemImage: "checkmark.circle.fill")
                .foregroundStyle(.green)
        case .failure:
            Label("Failed", systemImage: "xmark.circle.fill")
                .foregroundStyle(.red)
        }
    }

    private func saveKey() {
        guard !apiKeyInput.isEmpty else { return }

        if keychainManager.saveKey(apiKeyInput, for: provider) {
            apiKeyInput = ""
            showSaveConfirmation = true
            logInfo("Saved \(provider.displayName) API key", category: "Settings")

            // Hide confirmation after 2 seconds
            DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
                withAnimation {
                    showSaveConfirmation = false
                }
            }
        }
    }

    private func testConnection() {
        guard hasExistingKey else { return }

        isTestingConnection = true
        connectionStatus = .testing

        // TODO: Implement actual API test
        // For now, just validate the key exists and has valid format
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

    private let accentColors = ["purple", "blue", "green", "orange", "pink", "teal"]

    var body: some View {
        Form {
            Section("Theme") {
                Picker("Appearance", selection: $appearanceMode) {
                    Text("System").tag("system")
                    Text("Light").tag("light")
                    Text("Dark").tag("dark")
                }
                .pickerStyle(.segmented)
            }

            Section("Accent Color") {
                HStack(spacing: 12) {
                    ForEach(accentColors, id: \.self) { color in
                        Circle()
                            .fill(Color(color))
                            .frame(width: 32, height: 32)
                            .overlay(
                                Circle()
                                    .stroke(Color.primary, lineWidth: accentColor == color ? 3 : 0)
                                    .padding(2)
                            )
                            .onTapGesture {
                                accentColor = color
                            }
                    }
                }
            }

            Section("Layout") {
                Toggle("Compact Mode", isOn: $compactMode)
                Toggle("Show Agent Avatars", isOn: $showAgentAvatars)
            }

            Section("Effects") {
                Toggle("Enable Animations", isOn: $animationsEnabled)

                if animationsEnabled {
                    Text("Animations include typing indicators, streaming effects, and transitions.")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }
        }
        .padding()
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
