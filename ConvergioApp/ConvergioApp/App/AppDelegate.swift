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

    func applicationDidFinishLaunching(_ notification: Notification) {
        logInfo("AppDelegate: applicationDidFinishLaunching", category: "System")
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
