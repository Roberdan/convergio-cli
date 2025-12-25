//
//  AccessibilitySettingsView.swift
//  ConvergioApp
//
//  Created on 2025-12-24.
//  Comprehensive accessibility settings UI
//

import SwiftUI

struct AccessibilitySettingsView: View {
    @StateObject private var accessibilityManager = AccessibilityManager.shared
    @StateObject private var adhdManager = ADHDModeManager.shared
    @Environment(\.colorScheme) private var colorScheme

    @State private var selectedTab: AccessibilityCategory = .dyslexia
    @State private var showResetConfirmation = false

    enum AccessibilityCategory: String, CaseIterable {
        case dyslexia = "Dyslexia"
        case adhd = "ADHD"
        case visual = "Visual"
        case motor = "Motor"
        case presets = "Quick Presets"

        var icon: String {
            switch self {
            case .dyslexia: return "textformat.size"
            case .adhd: return "brain.head.profile"
            case .visual: return "eye.fill"
            case .motor: return "hand.point.up.left.fill"
            case .presets: return "star.fill"
            }
        }
    }

    var body: some View {
        NavigationView {
            // Sidebar with categories
            List(AccessibilityCategory.allCases, id: \.self) { category in
                Button(action: { selectedTab = category }) {
                    HStack {
                        Image(systemName: category.icon)
                            .frame(width: 24)

                        Text(category.rawValue)
                            .dyslexiaFont(size: 14)

                        Spacer()

                        if selectedTab == category {
                            Image(systemName: "checkmark")
                                .foregroundColor(.accentColor)
                        }
                    }
                }
                .buttonStyle(PlainButtonStyle())
                .padding(.vertical, 4)
            }
            .listStyle(SidebarListStyle())
            .frame(minWidth: 200, idealWidth: 220)

            // Main content area
            ScrollView {
                VStack(alignment: .leading, spacing: 24) {
                    headerSection

                    switch selectedTab {
                    case .dyslexia:
                        dyslexiaSettings
                    case .adhd:
                        adhdSettings
                    case .visual:
                        visualSettings
                    case .motor:
                        motorSettings
                    case .presets:
                        presetsSettings
                    }

                    Divider()

                    actionButtons
                }
                .padding(24)
                .frame(maxWidth: 800)
            }
            .frame(minWidth: 600, idealWidth: 800)
        }
        .navigationTitle("Accessibility Settings")
        .environmentObject(accessibilityManager)
        .alert("Reset Settings", isPresented: $showResetConfirmation) {
            Button("Cancel", role: .cancel) { }
            Button("Reset", role: .destructive) {
                accessibilityManager.resetToDefaults()
            }
        } message: {
            Text("Are you sure you want to reset all accessibility settings to defaults?")
        }
    }

    // MARK: - Header Section

    private var headerSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Image(systemName: selectedTab.icon)
                    .font(.system(size: 32))
                    .foregroundColor(.accentColor)

                Text(selectedTab.rawValue)
                    .font(.system(size: 28, weight: .bold))
            }

            Text(categoryDescription)
                .dyslexiaFont(size: 14)
                .foregroundColor(.secondary)
        }
    }

    private var categoryDescription: String {
        switch selectedTab {
        case .dyslexia:
            return "Optimize text readability with special fonts, spacing, and formatting"
        case .adhd:
            return "Focus tools including session timers, break reminders, and distraction-free mode"
        case .visual:
            return "High contrast, large text, and screen reader support"
        case .motor:
            return "Keyboard navigation and large touch targets"
        case .presets:
            return "Quickly apply pre-configured accessibility profiles"
        }
    }

    // MARK: - Dyslexia Settings

    private var dyslexiaSettings: some View {
        VStack(alignment: .leading, spacing: 20) {
            SettingsGroup(title: "Font & Spacing") {
                SettingsToggle(
                    title: "Dyslexia-Friendly Font",
                    description: "Use OpenDyslexic or similar fonts designed for dyslexia",
                    isOn: $accessibilityManager.settings.dyslexiaFont,
                    icon: "textformat"
                )

                if accessibilityManager.settings.dyslexiaFont {
                    SettingsToggle(
                        title: "Extra Letter Spacing",
                        description: "Increase spacing between letters (0.05em)",
                        isOn: $accessibilityManager.settings.extraLetterSpacing,
                        icon: "arrow.left.and.right"
                    )

                    SettingsToggle(
                        title: "Increased Line Height",
                        description: "Use 1.5x line height for better readability",
                        isOn: $accessibilityManager.settings.increasedLineHeight,
                        icon: "arrow.up.and.down"
                    )
                }

                SettingsSlider(
                    title: "Line Spacing",
                    value: $accessibilityManager.settings.lineSpacing,
                    range: 1.0...2.0,
                    step: 0.1,
                    unit: "x"
                )

                SettingsSlider(
                    title: "Font Size",
                    value: $accessibilityManager.settings.fontSize,
                    range: 0.8...1.5,
                    step: 0.1,
                    unit: "x"
                )
            }

            SettingsGroup(title: "Text-to-Speech") {
                SettingsToggle(
                    title: "Enable Text-to-Speech",
                    description: "Read text aloud with system voice",
                    isOn: $accessibilityManager.settings.ttsEnabled,
                    icon: "speaker.wave.3.fill"
                )

                if accessibilityManager.settings.ttsEnabled {
                    SettingsSlider(
                        title: "Speech Speed",
                        value: $accessibilityManager.settings.ttsSpeed,
                        range: 0.5...2.0,
                        step: 0.1,
                        unit: "x"
                    )

                    SettingsToggle(
                        title: "Auto-Read New Content",
                        description: "Automatically read new text as it appears",
                        isOn: $accessibilityManager.settings.ttsAutoRead,
                        icon: "play.circle.fill"
                    )
                }
            }

            previewSection
        }
    }

    // MARK: - ADHD Settings

    private var adhdSettings: some View {
        VStack(alignment: .leading, spacing: 20) {
            SettingsGroup(title: "Focus Mode") {
                SettingsToggle(
                    title: "Enable ADHD Mode",
                    description: "Activate focus-enhancing features",
                    isOn: $accessibilityManager.settings.adhdMode,
                    icon: "brain.head.profile"
                )

                SettingsToggle(
                    title: "Distraction-Free Mode",
                    description: "Hide non-essential UI elements during sessions",
                    isOn: $accessibilityManager.settings.distractionFreeMode,
                    icon: "eye.slash.fill"
                )

                SettingsToggle(
                    title: "Break Reminders",
                    description: "Get notifications for breaks",
                    isOn: $accessibilityManager.settings.breakReminders,
                    icon: "bell.fill"
                )
            }

            SettingsGroup(title: "Session Configuration") {
                HStack {
                    Text("Work Duration:")
                        .dyslexiaFont(size: 14)
                    Spacer()
                    Stepper(
                        "\(Int(adhdManager.config.workDuration / 60)) min",
                        value: Binding(
                            get: { adhdManager.config.workDuration / 60 },
                            set: { adhdManager.setWorkDuration(minutes: Int($0)) }
                        ),
                        in: 5...60,
                        step: 5
                    )
                }

                HStack {
                    Text("Break Duration:")
                        .dyslexiaFont(size: 14)
                    Spacer()
                    Stepper(
                        "\(Int(adhdManager.config.breakDuration / 60)) min",
                        value: Binding(
                            get: { adhdManager.config.breakDuration / 60 },
                            set: { adhdManager.setBreakDuration(minutes: Int($0)) }
                        ),
                        in: 3...30,
                        step: 1
                    )
                }

                SettingsToggle(
                    title: "Enable Gamification",
                    description: "Earn XP and track streaks",
                    isOn: $adhdManager.config.enableGamification,
                    icon: "star.fill"
                )
            }

            adhdStatsSection
        }
    }

    // MARK: - Visual Settings

    private var visualSettings: some View {
        VStack(alignment: .leading, spacing: 20) {
            SettingsGroup(title: "Contrast & Colors") {
                SettingsToggle(
                    title: "High Contrast Mode",
                    description: "Increase contrast for better visibility",
                    isOn: $accessibilityManager.settings.highContrast,
                    icon: "circle.lefthalf.filled"
                )

                SettingsToggle(
                    title: "Color Blind Mode",
                    description: "Use color blind-safe palette",
                    isOn: $accessibilityManager.settings.colorBlindMode,
                    icon: "eyedropper.halffull"
                )
            }

            SettingsGroup(title: "Text Size") {
                SettingsToggle(
                    title: "Large Text",
                    description: "Increase all text sizes by 20%",
                    isOn: $accessibilityManager.settings.largeText,
                    icon: "textformat.size.larger"
                )

                SettingsSlider(
                    title: "Font Size Multiplier",
                    value: $accessibilityManager.settings.fontSize,
                    range: 0.8...1.5,
                    step: 0.1,
                    unit: "x"
                )
            }

            SettingsGroup(title: "Motion") {
                SettingsToggle(
                    title: "Reduce Motion",
                    description: "Minimize animations and transitions",
                    isOn: $accessibilityManager.settings.reducedMotion,
                    icon: "move.3d"
                )
            }

            contrastPreview
        }
    }

    // MARK: - Motor Settings

    private var motorSettings: some View {
        VStack(alignment: .leading, spacing: 20) {
            SettingsGroup(title: "Navigation") {
                SettingsToggle(
                    title: "Full Keyboard Navigation",
                    description: "Navigate using keyboard only",
                    isOn: $accessibilityManager.settings.keyboardNavigation,
                    icon: "keyboard.fill"
                )

                HStack {
                    Image(systemName: "info.circle.fill")
                        .foregroundColor(.accentColor)

                    Text("All interactive elements have minimum 44x44 point touch targets per WCAG 2.1 AA guidelines.")
                        .dyslexiaFont(size: 12)
                        .foregroundColor(.secondary)
                }
                .padding(.vertical, 8)
            }

            SettingsGroup(title: "Motor Accessibility Tips") {
                VStack(alignment: .leading, spacing: 12) {
                    TipRow(icon: "keyboard", text: "Use Tab to navigate between elements")
                    TipRow(icon: "arrow.up.arrow.down", text: "Use arrow keys to select options")
                    TipRow(icon: "return", text: "Press Enter/Space to activate buttons")
                    TipRow(icon: "escape", text: "Press Escape to close dialogs")
                }
            }
        }
    }

    // MARK: - Presets Settings

    private var presetsSettings: some View {
        VStack(alignment: .leading, spacing: 20) {
            SettingsGroup(title: "Quick Apply Profiles") {
                PresetButton(
                    title: "Dyslexia Profile",
                    description: "Optimized font, spacing, and TTS",
                    icon: "textformat.size",
                    color: .blue
                ) {
                    accessibilityManager.applyDyslexiaProfile()
                }

                PresetButton(
                    title: "ADHD Profile",
                    description: "Focus mode, timers, and minimal distractions",
                    icon: "brain.head.profile",
                    color: .purple
                ) {
                    accessibilityManager.applyADHDProfile()
                }

                PresetButton(
                    title: "Visual Impairment Profile",
                    description: "High contrast, large text, and TTS",
                    icon: "eye.fill",
                    color: .orange
                ) {
                    accessibilityManager.applyVisualImpairmentProfile()
                }

                PresetButton(
                    title: "Motor Impairment Profile",
                    description: "Keyboard navigation and reduced motion",
                    icon: "hand.point.up.left.fill",
                    color: .green
                ) {
                    accessibilityManager.applyMotorImpairmentProfile()
                }
            }

            SettingsGroup(title: "Import/Export") {
                HStack(spacing: 12) {
                    Button("Export Settings") {
                        exportSettings()
                    }
                    .buttonStyle(.bordered)

                    Button("Import Settings") {
                        importSettings()
                    }
                    .buttonStyle(.bordered)
                }
            }
        }
    }

    // MARK: - Preview Sections

    private var previewSection: some View {
        SettingsGroup(title: "Preview") {
            VStack(alignment: .leading, spacing: 12) {
                Text("Sample Text Preview")
                    .font(.headline)

                DyslexiaFriendlyText(
                    "The quick brown fox jumps over the lazy dog. This text demonstrates how your content will appear with the current accessibility settings.",
                    size: 16
                )
                .padding()
                .background(Color.gray.opacity(0.1))
                .cornerRadius(8)
            }
        }
    }

    private var contrastPreview: some View {
        SettingsGroup(title: "Contrast Preview") {
            HStack(spacing: 16) {
                VStack(spacing: 8) {
                    Text("Normal")
                        .font(.caption)

                    Rectangle()
                        .fill(Color.primary)
                        .frame(width: 60, height: 60)
                        .cornerRadius(8)
                }

                VStack(spacing: 8) {
                    Text("High Contrast")
                        .font(.caption)

                    Rectangle()
                        .fill(
                            HighContrastTheme.text(
                                for: colorScheme,
                                highContrast: true
                            )
                        )
                        .frame(width: 60, height: 60)
                        .cornerRadius(8)
                        .overlay(
                            RoundedRectangle(cornerRadius: 8)
                                .stroke(
                                    HighContrastTheme.border(
                                        for: colorScheme,
                                        highContrast: true
                                    ),
                                    lineWidth: 2
                                )
                        )
                }
            }
        }
    }

    private var adhdStatsSection: some View {
        SettingsGroup(title: "Your Statistics") {
            VStack(alignment: .leading, spacing: 12) {
                StatRow(
                    label: "Total Sessions",
                    value: "\(adhdManager.stats.totalSessions)"
                )

                StatRow(
                    label: "Completed Sessions",
                    value: "\(adhdManager.stats.completedSessions)"
                )

                StatRow(
                    label: "Current Streak",
                    value: "\(adhdManager.stats.currentStreak) days"
                )

                StatRow(
                    label: "Total XP Earned",
                    value: "\(adhdManager.stats.totalXPEarned)"
                )

                if adhdManager.stats.totalSessions > 0 {
                    StatRow(
                        label: "Completion Rate",
                        value: String(format: "%.1f%%", adhdManager.getCompletionRate() * 100)
                    )
                }
            }
        }
    }

    // MARK: - Action Buttons

    private var actionButtons: some View {
        HStack {
            Button("Reset to Defaults") {
                showResetConfirmation = true
            }
            .buttonStyle(.bordered)
            .foregroundColor(.red)

            Spacer()

            Button("Done") {
                // Close window or navigate back
            }
            .buttonStyle(.borderedProminent)
        }
    }

    // MARK: - Helper Methods

    private func exportSettings() {
        if let json = accessibilityManager.exportSettings() {
            let panel = NSSavePanel()
            panel.nameFieldStringValue = "accessibility-settings.json"
            panel.allowedContentTypes = [.json]

            if panel.runModal() == .OK, let url = panel.url {
                try? json.write(to: url, atomically: true, encoding: .utf8)
            }
        }
    }

    private func importSettings() {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [.json]
        panel.allowsMultipleSelection = false

        if panel.runModal() == .OK, let url = panel.url {
            if let json = try? String(contentsOf: url) {
                _ = accessibilityManager.importSettings(from: json)
            }
        }
    }
}

// MARK: - Supporting Views

struct SettingsGroup<Content: View>: View {
    let title: String
    let content: Content

    init(title: String, @ViewBuilder content: () -> Content) {
        self.title = title
        self.content = content()
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text(title)
                .font(.system(size: 16, weight: .semibold))
                .foregroundColor(.primary)

            VStack(alignment: .leading, spacing: 16) {
                content
            }
            .padding()
            .background(Color.gray.opacity(0.05))
            .cornerRadius(8)
        }
    }
}

struct SettingsToggle: View {
    let title: String
    let description: String
    @Binding var isOn: Bool
    let icon: String

    var body: some View {
        Toggle(isOn: $isOn) {
            HStack(spacing: 12) {
                Image(systemName: icon)
                    .frame(width: 20)
                    .foregroundColor(.accentColor)

                VStack(alignment: .leading, spacing: 2) {
                    Text(title)
                        .dyslexiaFont(size: 14, weight: .medium)

                    Text(description)
                        .dyslexiaFont(size: 12)
                        .foregroundColor(.secondary)
                }
            }
        }
        .toggleStyle(SwitchToggleStyle())
    }
}

struct SettingsSlider: View {
    let title: String
    @Binding var value: Double
    let range: ClosedRange<Double>
    let step: Double
    let unit: String

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text(title)
                    .dyslexiaFont(size: 14)

                Spacer()

                Text(String(format: "%.1f%@", value, unit))
                    .dyslexiaFont(size: 14, weight: .medium)
                    .foregroundColor(.secondary)
            }

            Slider(value: $value, in: range, step: step)
        }
    }
}

struct PresetButton: View {
    let title: String
    let description: String
    let icon: String
    let color: Color
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            HStack(spacing: 16) {
                Image(systemName: icon)
                    .font(.system(size: 24))
                    .foregroundColor(color)
                    .frame(width: 40, height: 40)
                    .background(color.opacity(0.1))
                    .cornerRadius(8)

                VStack(alignment: .leading, spacing: 4) {
                    Text(title)
                        .dyslexiaFont(size: 14, weight: .semibold)
                        .foregroundColor(.primary)

                    Text(description)
                        .dyslexiaFont(size: 12)
                        .foregroundColor(.secondary)
                }

                Spacer()

                Image(systemName: "chevron.right")
                    .foregroundColor(.secondary)
            }
            .padding()
            .background(Color.gray.opacity(0.05))
            .cornerRadius(8)
        }
        .buttonStyle(PlainButtonStyle())
    }
}

struct StatRow: View {
    let label: String
    let value: String

    var body: some View {
        HStack {
            Text(label)
                .dyslexiaFont(size: 14)
                .foregroundColor(.secondary)

            Spacer()

            Text(value)
                .dyslexiaFont(size: 14, weight: .semibold)
                .foregroundColor(.primary)
        }
    }
}

struct TipRow: View {
    let icon: String
    let text: String

    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: icon)
                .frame(width: 20)
                .foregroundColor(.accentColor)

            Text(text)
                .dyslexiaFont(size: 13)
                .foregroundColor(.secondary)
        }
    }
}

// MARK: - Preview Provider

#Preview {
    AccessibilitySettingsView()
        .frame(width: 1000, height: 700)
}
