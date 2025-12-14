/**
 * CONVERGIO NATIVE - Global Hotkey Manager
 *
 * Manages global keyboard shortcuts for quick access to Convergio
 * from anywhere in the system.
 *
 * Default hotkey: Cmd+Shift+Space
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import AppKit
import Carbon
import Combine

/// Manages global keyboard shortcuts
final class HotkeyManager: ObservableObject {
    static let shared = HotkeyManager()

    /// Whether the global hotkey is currently enabled
    @Published var isEnabled: Bool = true {
        didSet {
            if isEnabled {
                registerHotkey()
            } else {
                unregisterHotkey()
            }
        }
    }

    /// The action to perform when hotkey is triggered
    var onHotkeyPressed: (() -> Void)?

    private var eventHandler: EventHandlerRef?
    private var hotkeyRef: EventHotKeyRef?

    private init() {
        registerHotkey()
    }

    deinit {
        unregisterHotkey()
    }

    // MARK: - Registration

    private func registerHotkey() {
        guard eventHandler == nil else { return }

        // Define the hotkey: Cmd+Shift+Space
        var hotKeyID = EventHotKeyID()
        hotKeyID.signature = OSType(0x434F4E56) // "CONV" in hex
        hotKeyID.id = 1

        // Install event handler
        var eventType = EventTypeSpec(eventClass: OSType(kEventClassKeyboard), eventKind: UInt32(kEventHotKeyPressed))

        let handlerBlock: EventHandlerUPP = { _, event, userData -> OSStatus in
            guard let userData = userData else { return OSStatus(eventNotHandledErr) }
            let manager = Unmanaged<HotkeyManager>.fromOpaque(userData).takeUnretainedValue()
            manager.handleHotkey()
            return noErr
        }

        let status = InstallEventHandler(
            GetApplicationEventTarget(),
            handlerBlock,
            1,
            &eventType,
            Unmanaged.passUnretained(self).toOpaque(),
            &eventHandler
        )

        guard status == noErr else {
            print("Failed to install event handler: \(status)")
            return
        }

        // Register the hotkey (Cmd+Shift+Space)
        let modifiers = UInt32(cmdKey | shiftKey)
        let keyCode = UInt32(kVK_Space)

        let registerStatus = RegisterEventHotKey(
            keyCode,
            modifiers,
            hotKeyID,
            GetApplicationEventTarget(),
            0,
            &hotkeyRef
        )

        if registerStatus != noErr {
            print("Failed to register hotkey: \(registerStatus)")
        }
    }

    private func unregisterHotkey() {
        if let hotkeyRef = hotkeyRef {
            UnregisterEventHotKey(hotkeyRef)
            self.hotkeyRef = nil
        }

        if let eventHandler = eventHandler {
            RemoveEventHandler(eventHandler)
            self.eventHandler = nil
        }
    }

    private func handleHotkey() {
        DispatchQueue.main.async {
            self.onHotkeyPressed?()
        }
    }
}

// MARK: - App Integration

extension HotkeyManager {
    /// Toggle the main window visibility
    func toggleMainWindow() {
        guard let app = NSApp else { return }

        if app.isActive {
            // Hide the app
            app.hide(nil)
        } else {
            // Show and activate the app
            app.activate(ignoringOtherApps: true)

            // Bring main window to front
            if let window = app.mainWindow ?? app.windows.first {
                window.makeKeyAndOrderFront(nil)
            }
        }
    }

    /// Show the quick prompt panel (for menubar-style quick access)
    func showQuickPrompt() {
        guard let app = NSApp else { return }

        app.activate(ignoringOtherApps: true)

        // Post notification for UI to show quick prompt panel
        NotificationCenter.default.post(
            name: .showQuickPrompt,
            object: nil
        )
    }
}

// MARK: - Notification Names

extension Notification.Name {
    static let showQuickPrompt = Notification.Name("com.convergio.showQuickPrompt")
    static let hotkeyTriggered = Notification.Name("com.convergio.hotkeyTriggered")
}
