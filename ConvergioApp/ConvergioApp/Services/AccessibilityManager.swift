//
//  AccessibilityManager.swift
//  ConvergioApp
//
//  Created on 2025-12-24.
//  Accessibility settings manager for inclusive education
//

import Foundation
import SwiftUI
import Combine

/// Accessibility settings structure
struct AccessibilitySettings: Codable, Equatable {
    // Dyslexia support
    var dyslexiaFont: Bool = false
    var extraLetterSpacing: Bool = false
    var increasedLineHeight: Bool = false

    // Visual support
    var highContrast: Bool = false
    var largeText: Bool = false
    var reducedMotion: Bool = false

    // Text-to-Speech
    var ttsEnabled: Bool = false
    var ttsSpeed: Double = 1.0  // 0.5 to 2.0
    var ttsAutoRead: Bool = false

    // ADHD support
    var adhdMode: Bool = false
    var distractionFreeMode: Bool = false
    var breakReminders: Bool = false

    // General accessibility
    var lineSpacing: Double = 1.0  // 1.0 to 2.0
    var fontSize: Double = 1.0     // 0.8 to 1.5
    var colorBlindMode: Bool = false
    var keyboardNavigation: Bool = true

    // Custom colors for high contrast
    var customBackgroundColor: String = "#FFFFFF"
    var customTextColor: String = "#000000"
}

/// Accessibility Manager - Observable object for managing accessibility settings
@MainActor
class AccessibilityManager: ObservableObject {
    static let shared = AccessibilityManager()

    @Published var settings: AccessibilitySettings {
        didSet {
            saveSettings()
            applySystemAccessibilitySettings()
        }
    }

    private let userDefaultsKey = "com.convergio.accessibility.settings"
    private var cancellables = Set<AnyCancellable>()

    private init() {
        self.settings = AccessibilityManager.loadSettings()
        applySystemAccessibilitySettings()
        observeSystemAccessibilityChanges()
    }

    // MARK: - Settings Persistence

    private static func loadSettings() -> AccessibilitySettings {
        guard let data = UserDefaults.standard.data(forKey: "com.convergio.accessibility.settings"),
              let settings = try? JSONDecoder().decode(AccessibilitySettings.self, from: data) else {
            // Return default settings with system preferences
            var defaultSettings = AccessibilitySettings()
            defaultSettings.reducedMotion = NSWorkspace.shared.accessibilityDisplayShouldReduceMotion
            defaultSettings.highContrast = NSWorkspace.shared.accessibilityDisplayShouldIncreaseContrast
            return defaultSettings
        }
        return settings
    }

    private func saveSettings() {
        if let data = try? JSONEncoder().encode(settings) {
            UserDefaults.standard.set(data, forKey: userDefaultsKey)
            Logger.shared.info("Accessibility settings saved")
        }
    }

    // MARK: - System Integration

    private func applySystemAccessibilitySettings() {
        // Sync with system accessibility preferences
        if settings.reducedMotion {
            // Apps should check this setting before animations
            Logger.shared.debug("Reduced motion enabled")
        }

        // Update VoiceOver settings if needed
        if settings.ttsEnabled {
            Logger.shared.debug("TTS enabled")
        }

        // Notify observers
        NotificationCenter.default.post(
            name: NSNotification.Name("AccessibilitySettingsChanged"),
            object: settings
        )
    }

    private func observeSystemAccessibilityChanges() {
        // Observe system accessibility changes
        NotificationCenter.default.publisher(for: NSWorkspace.accessibilityDisplayOptionsDidChangeNotification)
            .sink { [weak self] _ in
                Task { @MainActor [weak self] in
                    guard let self = self else { return }

                    // Update settings from system
                    let systemReducedMotion = NSWorkspace.shared.accessibilityDisplayShouldReduceMotion
                    let systemHighContrast = NSWorkspace.shared.accessibilityDisplayShouldIncreaseContrast

                    if self.settings.reducedMotion != systemReducedMotion {
                        self.settings.reducedMotion = systemReducedMotion
                        Logger.shared.info("System reduced motion changed to: \(systemReducedMotion)")
                    }

                    if self.settings.highContrast != systemHighContrast {
                        self.settings.highContrast = systemHighContrast
                        Logger.shared.info("System high contrast changed to: \(systemHighContrast)")
                    }
                }
            }
            .store(in: &cancellables)
    }

    // MARK: - Helper Methods

    /// Get the appropriate line spacing based on settings
    func getLineSpacing() -> CGFloat {
        var spacing = settings.lineSpacing

        if settings.dyslexiaFont && settings.increasedLineHeight {
            spacing = max(spacing, 1.5)
        }

        return CGFloat(spacing)
    }

    /// Get the appropriate font size multiplier
    func getFontSizeMultiplier() -> CGFloat {
        var multiplier = settings.fontSize

        if settings.largeText {
            multiplier *= 1.2
        }

        return CGFloat(multiplier)
    }

    /// Get letter spacing for dyslexia support
    func getLetterSpacing() -> CGFloat {
        if settings.dyslexiaFont && settings.extraLetterSpacing {
            return 0.05  // 0.05em extra spacing
        }
        return 0
    }

    /// Check if animations should be enabled
    func shouldAnimate() -> Bool {
        return !settings.reducedMotion
    }

    /// Get animation duration with accessibility consideration
    func getAnimationDuration(_ baseDuration: Double = 0.3) -> Double {
        if settings.reducedMotion {
            return 0
        }
        return baseDuration
    }

    /// Reset to default settings
    func resetToDefaults() {
        settings = AccessibilitySettings()
        Logger.shared.info("Accessibility settings reset to defaults")
    }

    /// Export settings as JSON
    func exportSettings() -> String? {
        if let data = try? JSONEncoder().encode(settings),
           let json = String(data: data, encoding: .utf8) {
            return json
        }
        return nil
    }

    /// Import settings from JSON
    func importSettings(from json: String) -> Bool {
        guard let data = json.data(using: .utf8),
              let imported = try? JSONDecoder().decode(AccessibilitySettings.self, from: data) else {
            Logger.shared.error("Failed to import accessibility settings")
            return false
        }

        settings = imported
        Logger.shared.info("Accessibility settings imported successfully")
        return true
    }

    // MARK: - Preset Profiles

    /// Apply dyslexia-optimized profile
    func applyDyslexiaProfile() {
        settings.dyslexiaFont = true
        settings.extraLetterSpacing = true
        settings.increasedLineHeight = true
        settings.lineSpacing = 1.5
        settings.fontSize = 1.1
        Logger.shared.info("Dyslexia profile applied")
    }

    /// Apply ADHD-optimized profile
    func applyADHDProfile() {
        settings.adhdMode = true
        settings.distractionFreeMode = true
        settings.breakReminders = true
        settings.reducedMotion = true
        Logger.shared.info("ADHD profile applied")
    }

    /// Apply visual impairment profile
    func applyVisualImpairmentProfile() {
        settings.highContrast = true
        settings.largeText = true
        settings.fontSize = 1.3
        settings.ttsEnabled = true
        Logger.shared.info("Visual impairment profile applied")
    }

    /// Apply motor impairment profile
    func applyMotorImpairmentProfile() {
        settings.keyboardNavigation = true
        settings.reducedMotion = true
        Logger.shared.info("Motor impairment profile applied")
    }
}

// MARK: - SwiftUI Environment Key

private struct AccessibilityManagerKey: EnvironmentKey {
    @MainActor static let defaultValue = AccessibilityManager.shared
}

extension EnvironmentValues {
    var accessibilityManager: AccessibilityManager {
        get { self[AccessibilityManagerKey.self] }
        set { self[AccessibilityManagerKey.self] = newValue }
    }
}
