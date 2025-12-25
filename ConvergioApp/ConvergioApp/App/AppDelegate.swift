/**
 * CONVERGIO NATIVE - App Delegate
 *
 * Handles Dock menu, notifications, and other AppKit-level functionality.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import AppKit
import SwiftUI
import AVFoundation

// MARK: - App Delegate

class AppDelegate: NSObject, NSApplicationDelegate {

    private var audioPlayer: AVAudioPlayer?
    private var hotkeyManager: HotkeyManager?

    func applicationDidFinishLaunching(_ notification: Notification) {
        logInfo("AppDelegate: applicationDidFinishLaunching", category: "System")

        // Initialize global hotkey (Cmd+Shift+Space)
        setupGlobalHotkey()
    }

    // MARK: - Global Hotkey

    private func setupGlobalHotkey() {
        hotkeyManager = HotkeyManager.shared
        hotkeyManager?.onHotkeyPressed = { [weak self] in
            self?.handleGlobalHotkey()
        }
        logInfo("Global hotkey registered: Cmd+Shift+Space", category: "System")
    }

    private func handleGlobalHotkey() {
        // Activate and show main window
        NSApp.activate(ignoringOtherApps: true)

        if let window = NSApp.mainWindow ?? NSApp.windows.first(where: { $0.isVisible }) {
            window.makeKeyAndOrderFront(nil)
        }

        // Post notification for voice session quick start
        NotificationCenter.default.post(name: .hotkeyTriggered, object: nil)
        logInfo("Global hotkey triggered", category: "System")
    }

    func applicationWillTerminate(_ notification: Notification) {
        logInfo("AppDelegate: applicationWillTerminate", category: "System")
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return false // Keep running in menu bar
    }

    // MARK: - Dock Menu

    func applicationDockMenu(_ sender: NSApplication) -> NSMenu? {
        let menu = NSMenu()

        // New Conversation
        let newItem = NSMenuItem(
            title: "New Conversation",
            action: #selector(newConversation),
            keyEquivalent: "n"
        )
        newItem.target = self
        menu.addItem(newItem)

        menu.addItem(.separator())

        // Quick Actions
        let clearItem = NSMenuItem(
            title: "Clear History",
            action: #selector(clearHistory),
            keyEquivalent: ""
        )
        clearItem.target = self
        menu.addItem(clearItem)

        menu.addItem(.separator())

        // Open Settings
        let settingsItem = NSMenuItem(
            title: "Settings...",
            action: #selector(openSettings),
            keyEquivalent: ","
        )
        settingsItem.target = self
        menu.addItem(settingsItem)

        return menu
    }

    // MARK: - Dock Menu Actions

    @objc private func newConversation() {
        NotificationCenter.default.post(name: .newConversation, object: nil)
        NSApp.activate(ignoringOtherApps: true)
    }

    @objc private func clearHistory() {
        NotificationCenter.default.post(name: .clearHistory, object: nil)
        NSApp.activate(ignoringOtherApps: true)
    }

    @objc private func openSettings() {
        NSApp.sendAction(Selector(("showSettingsWindow:")), to: nil, from: nil)
        NSApp.activate(ignoringOtherApps: true)
    }

    // MARK: - Notification Sound

    static func playCompletionSound() {
        // Use system sound for completion
        NSSound(named: "Glass")?.play()
    }

    static func playErrorSound() {
        NSSound(named: "Basso")?.play()
    }
}

// MARK: - Notification Names

extension Notification.Name {
    static let newConversation = Notification.Name("com.convergio.newConversation")
    static let clearHistory = Notification.Name("com.convergio.clearHistory")
    static let responseCompleted = Notification.Name("com.convergio.responseCompleted")
}
