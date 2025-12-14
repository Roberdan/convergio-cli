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

    @Environment(\.openWindow) private var openWindow

    var body: some Scene {
        // Main window
        WindowGroup {
            ContentView()
                .environmentObject(orchestratorVM)
                .environmentObject(conversationVM)
                .frame(minWidth: 900, minHeight: 600)
                .task {
                    await orchestratorVM.initialize()
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
    }
}
