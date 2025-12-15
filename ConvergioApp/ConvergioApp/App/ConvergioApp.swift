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
    @StateObject private var orchestratorVM = OrchestratorViewModel()
    @StateObject private var conversationVM = ConversationViewModel()
    @StateObject private var logger = Logger.shared

    @Environment(\.openWindow) private var openWindow

    init() {
        // Set up crash handler
        setupCrashHandler()
        logInfo("Convergio app starting", category: "System")
    }

    var body: some Scene {
        // Main window
        WindowGroup {
            ContentView()
                .environmentObject(orchestratorVM)
                .environmentObject(conversationVM)
                .environmentObject(logger)
                .frame(minWidth: 900, minHeight: 600)
                .task {
                    logInfo("Initializing orchestrator", category: "System")
                    await orchestratorVM.initialize()
                    logInfo("Orchestrator initialized", category: "System")
                }
        }
        .windowStyle(.hiddenTitleBar)
        .defaultSize(width: 1200, height: 800)
        .commands {
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

            CommandGroup(after: .toolbar) {
                Button("Show Agent Panel") {
                    orchestratorVM.showAgentPanel.toggle()
                }
                .keyboardShortcut("1", modifiers: [.command, .option])

                Button("Show Cost Dashboard") {
                    orchestratorVM.showCostDashboard.toggle()
                }
                .keyboardShortcut("2", modifiers: [.command, .option])
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
