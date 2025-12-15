/**
 * CONVERGIO NATIVE - Main Application
 *
 * The main entry point for Convergio Native macOS application.
 * Features a main window, menu bar presence, and global hotkey support.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

@main
struct ConvergioApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

    @StateObject private var orchestratorVM = OrchestratorViewModel()
    @StateObject private var conversationVM = ConversationViewModel()
    @StateObject private var keychainManager = KeychainManager.shared
    @StateObject private var logger = Logger.shared

    @Environment(\.openWindow) private var openWindow

    /// Check if any API key is configured
    private var hasAnyApiKey: Bool {
        keychainManager.hasAnthropicKey ||
        keychainManager.hasOpenAIKey ||
        keychainManager.hasGeminiKey ||
        keychainManager.hasOpenRouterKey
    }

    /// Show onboarding if no API keys are configured
    @State private var showOnboarding = false
    @State private var showAbout = false

    init() {
        // Set up crash handler
        setupCrashHandler()
        logInfo("Convergio app starting", category: "System")

        // Initialize Keychain and load API keys
        Task { @MainActor in
            KeychainManager.shared.initializeOnLaunch()
        }
    }

    var body: some Scene {
        // Main window
        WindowGroup {
            ZStack {
                ContentView()
                    .environmentObject(orchestratorVM)
                    .environmentObject(conversationVM)
                    .environmentObject(keychainManager)
                    .environmentObject(logger)
                    .frame(minWidth: 900, minHeight: 600)
                    .task {
                        logInfo("Initializing orchestrator", category: "System")
                        await orchestratorVM.initialize()
                        logInfo("Orchestrator initialized", category: "System")

                        // Check if we need to show onboarding
                        if !hasAnyApiKey && !UserDefaults.standard.bool(forKey: "onboardingComplete") {
                            showOnboarding = true
                        }
                    }
            }
            .sheet(isPresented: $showOnboarding) {
                OnboardingView(isComplete: $showOnboarding)
                    .environmentObject(keychainManager)
            }
            .sheet(isPresented: $showAbout) {
                AboutView()
            }
        }
        .windowStyle(.hiddenTitleBar)
        .defaultSize(width: 1200, height: 800)
        .commands {
            // App menu - About
            CommandGroup(replacing: .appInfo) {
                Button("About Convergio") {
                    showAbout = true
                }
            }

            // File menu
            CommandGroup(replacing: .newItem) {
                Button("New Conversation") {
                    conversationVM.newConversation()
                }
                .keyboardShortcut("n", modifiers: .command)

                Divider()

                Button("Clear History") {
                    conversationVM.clearHistory()
                }
                .keyboardShortcut("k", modifiers: [.command, .shift])
            }

            // View menu
            CommandGroup(after: .toolbar) {
                Button("Show Agent Panel") {
                    orchestratorVM.showAgentPanel.toggle()
                }
                .keyboardShortcut("1", modifiers: [.command, .option])

                Button("Show Cost Dashboard") {
                    orchestratorVM.showCostDashboard.toggle()
                }
                .keyboardShortcut("2", modifiers: [.command, .option])

                Divider()

                Button("Agent Interaction Visualizer") {
                    openWindow(id: "agent-visualizer")
                }
                .keyboardShortcut("3", modifiers: [.command, .option])
            }

            // Help menu
            CommandGroup(replacing: .help) {
                Button("Convergio Help") {
                    if let url = URL(string: "https://github.com/Roberdan/Convergio/wiki") {
                        NSWorkspace.shared.open(url)
                    }
                }
                .keyboardShortcut("?", modifiers: [.command, .shift])

                Divider()

                Button("Keyboard Shortcuts") {
                    openWindow(id: "help-shortcuts")
                }

                Button("Report an Issue") {
                    if let url = URL(string: "https://github.com/Roberdan/Convergio/issues") {
                        NSWorkspace.shared.open(url)
                    }
                }
            }
        }

        // Menu bar presence
        MenuBarExtra("Convergio", systemImage: "brain.head.profile") {
            MenuBarView()
                .environmentObject(orchestratorVM)
                .environmentObject(conversationVM)
        }
        .menuBarExtraStyle(.window)

        // Settings window
        Settings {
            SettingsView()
                .environmentObject(orchestratorVM)
        }

        // Debug log viewer window
        Window("Log Viewer", id: "log-viewer") {
            LogViewerView()
        }
        .defaultSize(width: 800, height: 500)
        .keyboardShortcut("l", modifiers: [.command, .option])

        // Keyboard shortcuts help window
        Window("Keyboard Shortcuts", id: "help-shortcuts") {
            KeyboardShortcutsView()
        }
        .defaultSize(width: 500, height: 400)

        // Agent Interaction Visualizer
        Window("Agent Interaction Visualizer", id: "agent-visualizer") {
            AgentInteractionVisualizer()
                .environmentObject(orchestratorVM)
        }
        .defaultSize(width: 900, height: 700)
        .windowStyle(.hiddenTitleBar)
    }
}

// MARK: - About View

struct AboutView: View {
    @Environment(\.dismiss) private var dismiss

    private var appVersion: String {
        Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String ?? "1.0"
    }

    private var buildNumber: String {
        Bundle.main.infoDictionary?["CFBundleVersion"] as? String ?? "1"
    }

    var body: some View {
        VStack(spacing: 20) {
            // App Icon
            if let appIcon = NSImage(named: "AppIcon") {
                Image(nsImage: appIcon)
                    .resizable()
                    .frame(width: 128, height: 128)
            } else {
                Image(systemName: "brain.head.profile")
                    .font(.system(size: 80))
                    .foregroundStyle(.purple.gradient)
            }

            // App Name
            Text("Convergio")
                .font(.largeTitle.bold())

            Text("Your AI Executive Team")
                .font(.headline)
                .foregroundStyle(.secondary)

            // Version
            Text("Version \(appVersion) (\(buildNumber))")
                .font(.caption)
                .foregroundStyle(.tertiary)

            Divider()
                .frame(width: 200)

            // Credits
            VStack(spacing: 4) {
                Text("Created by Roberto D'Angelo")
                    .font(.caption)
                Text("with the help of AI agents team")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Text("Copyright 2025. All rights reserved.")
                .font(.caption2)
                .foregroundStyle(.tertiary)

            Spacer()

            Button("OK") {
                dismiss()
            }
            .keyboardShortcut(.defaultAction)
        }
        .padding(30)
        .frame(width: 350, height: 400)
    }
}

// MARK: - Keyboard Shortcuts View

struct KeyboardShortcutsView: View {
    private let shortcuts: [(category: String, items: [(key: String, description: String)])] = [
        ("General", [
            ("Cmd+N", "New Conversation"),
            ("Cmd+,", "Open Settings"),
            ("Cmd+Q", "Quit Convergio"),
        ]),
        ("Conversation", [
            ("Cmd+Return", "Send Message"),
            ("Cmd+.", "Cancel Request"),
            ("Cmd+Shift+K", "Clear History"),
        ]),
        ("View", [
            ("Cmd+Opt+1", "Toggle Agent Panel"),
            ("Cmd+Opt+2", "Toggle Cost Dashboard"),
            ("Cmd+Opt+L", "Open Log Viewer"),
        ]),
        ("Global", [
            ("Cmd+Shift+Space", "Activate from anywhere"),
        ]),
    ]

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Keyboard Shortcuts")
                .font(.title2.bold())
                .padding(.bottom, 8)

            ForEach(shortcuts, id: \.category) { section in
                VStack(alignment: .leading, spacing: 8) {
                    Text(section.category)
                        .font(.headline)
                        .foregroundStyle(.secondary)

                    ForEach(section.items, id: \.key) { item in
                        HStack {
                            Text(item.key)
                                .font(.system(.body, design: .monospaced))
                                .padding(.horizontal, 8)
                                .padding(.vertical, 2)
                                .background(Color.secondary.opacity(0.1))
                                .clipShape(RoundedRectangle(cornerRadius: 4))

                            Spacer()

                            Text(item.description)
                                .foregroundStyle(.primary)
                        }
                    }
                }
            }

            Spacer()
        }
        .padding(24)
    }
}

// MARK: - Crash Handler

private func setupCrashHandler() {
    NSSetUncaughtExceptionHandler { exception in
        let message = """
        Uncaught Exception:
        Name: \(exception.name.rawValue)
        Reason: \(exception.reason ?? "Unknown")
        Call Stack:
        \(exception.callStackSymbols.joined(separator: "\n"))
        """
        logCritical(message, category: "Crash")

        // Write to file immediately
        if let logsDir = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first?
            .appendingPathComponent("Convergio/Logs", isDirectory: true) {
            try? FileManager.default.createDirectory(at: logsDir, withIntermediateDirectories: true)
            let crashFile = logsDir.appendingPathComponent("crash_\(Date().ISO8601Format()).log")
            try? message.write(to: crashFile, atomically: true, encoding: .utf8)
        }
    }

    // Signal handlers for segfaults etc.
    signal(SIGABRT) { _ in logCritical("SIGABRT received", category: "Crash") }
    signal(SIGSEGV) { _ in logCritical("SIGSEGV received", category: "Crash") }
    signal(SIGBUS) { _ in logCritical("SIGBUS received", category: "Crash") }
}
