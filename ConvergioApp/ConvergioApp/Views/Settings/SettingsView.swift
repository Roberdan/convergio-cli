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

// MARK: - Main Settings View

struct MainSettingsView: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel

    var body: some View {
        TabView {
            GeneralSettingsTab()
                .tabItem {
                    Label("General", systemImage: "gear")
                }

            ProvidersSettingsTab()
                .tabItem {
                    Label("Providers", systemImage: "server.rack")
                }

            BudgetSettingsTab()
                .tabItem {
                    Label("Budget", systemImage: "dollarsign.circle")
                }

            AppearanceSettingsTab()
                .tabItem {
                    Label("Appearance", systemImage: "paintbrush")
                }

            ShortcutsSettingsTab()
                .tabItem {
                    Label("Shortcuts", systemImage: "keyboard")
                }

            AdvancedSettingsTab()
                .tabItem {
                    Label("Advanced", systemImage: "gearshape.2")
                }
        }
        .frame(width: 600, height: 450)
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
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel
    @State private var selectedProvider = "Anthropic"
    @State private var apiKeys: [String: String] = [:]

    private let providers = [
        ProviderInfo(name: "Anthropic", icon: "brain", models: ["claude-sonnet-4-20250514", "claude-3-5-sonnet-20241022"]),
        ProviderInfo(name: "OpenAI", icon: "sparkles", models: ["gpt-4o", "gpt-4o-mini"]),
        ProviderInfo(name: "Google", icon: "g.circle", models: ["gemini-2.0-flash", "gemini-1.5-pro"]),
        ProviderInfo(name: "Local (MLX)", icon: "desktopcomputer", models: ["llama-3.2", "mistral-7b"])
    ]

    var body: some View {
        HSplitView {
            // Provider list
            List(providers, id: \.name, selection: $selectedProvider) { provider in
                HStack(spacing: 12) {
                    Image(systemName: provider.icon)
                        .frame(width: 24)
                    VStack(alignment: .leading) {
                        Text(provider.name)
                            .font(.headline)
                        Text("\(provider.models.count) models")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                }
                .padding(.vertical, 4)
            }
            .frame(minWidth: 150, maxWidth: 200)

            // Provider detail
            if let provider = providers.first(where: { $0.name == selectedProvider }) {
                ProviderDetailView(provider: provider, apiKey: binding(for: provider.name))
            }
        }
        .padding()
    }

    private func binding(for provider: String) -> Binding<String> {
        Binding(
            get: { apiKeys[provider] ?? "" },
            set: { apiKeys[provider] = $0 }
        )
    }
}

private struct ProviderInfo {
    let name: String
    let icon: String
    let models: [String]
}

private struct ProviderDetailView: View {
    let provider: ProviderInfo
    @Binding var apiKey: String
    @State private var showApiKey = false
    @State private var isTestingConnection = false
    @State private var connectionStatus: ConnectionStatus = .unknown

    enum ConnectionStatus {
        case unknown, testing, success, failure
    }

    var body: some View {
        Form {
            Section("API Configuration") {
                HStack {
                    if showApiKey {
                        TextField("API Key", text: $apiKey)
                            .textFieldStyle(.roundedBorder)
                    } else {
                        SecureField("API Key", text: $apiKey)
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
                    Button("Test Connection") {
                        testConnection()
                    }
                    .disabled(apiKey.isEmpty || isTestingConnection)

                    if isTestingConnection {
                        ProgressView()
                            .scaleEffect(0.7)
                    }

                    Spacer()

                    connectionStatusView
                }
            }

            Section("Available Models") {
                ForEach(provider.models, id: \.self) { model in
                    HStack {
                        Text(model)
                            .font(.body.monospaced())
                        Spacer()
                        Button("Set Default") {
                            // TODO: Set default model
                        }
                        .buttonStyle(.bordered)
                        .controlSize(.small)
                    }
                }
            }

            if provider.name == "Local (MLX)" {
                Section("Local Model Settings") {
                    Text("Local models run on Apple Silicon using MLX framework.")
                        .font(.caption)
                        .foregroundStyle(.secondary)

                    HStack {
                        Text("GPU Memory Limit")
                        Spacer()
                        Picker("", selection: .constant("Auto")) {
                            Text("Auto").tag("Auto")
                            Text("8 GB").tag("8")
                            Text("16 GB").tag("16")
                            Text("32 GB").tag("32")
                        }
                        .frame(width: 100)
                    }
                }
            }
        }
        .padding()
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

    private func testConnection() {
        isTestingConnection = true
        connectionStatus = .testing

        // Simulate connection test
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.5) {
            isTestingConnection = false
            connectionStatus = apiKey.count > 10 ? .success : .failure
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

// MARK: - Preview

#Preview("Settings") {
    MainSettingsView()
        .environmentObject(OrchestratorViewModel.preview)
}
