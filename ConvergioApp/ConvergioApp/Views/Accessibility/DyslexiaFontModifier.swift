//
//  DyslexiaFontModifier.swift
//  ConvergioApp
//
//  Created on 2025-12-24.
//  ViewModifier for dyslexia-friendly text formatting
//

import SwiftUI

/// ViewModifier that applies dyslexia-friendly text formatting
struct DyslexiaFontModifier: ViewModifier {
    @EnvironmentObject private var accessibilityManager: AccessibilityManager

    let baseSize: CGFloat
    let weight: Font.Weight
    let relativeTo: Font.TextStyle

    init(
        size: CGFloat = 16,
        weight: Font.Weight = .regular,
        relativeTo: Font.TextStyle = .body
    ) {
        self.baseSize = size
        self.weight = weight
        self.relativeTo = relativeTo
    }

    func body(content: Content) -> some View {
        content
            .font(dyslexiaFont)
            .tracking(letterSpacing)
            .lineSpacing(lineSpacing)
    }

    private var dyslexiaFont: Font {
        let settings = accessibilityManager.settings

        // Calculate final size with multiplier
        let finalSize = baseSize * accessibilityManager.getFontSizeMultiplier()

        if settings.dyslexiaFont {
            // Try to use OpenDyslexic if available, otherwise use system font
            // OpenDyslexic is a font designed specifically for dyslexia
            // Note: Custom fonts need to be added to the project
            if let customFont = loadDyslexicFont(size: finalSize) {
                return customFont
            }

            // Fallback to system font with dyslexia-friendly characteristics
            // - Sans-serif (easier to read)
            // - Regular weight (not too thin, not too bold)
            // - Monospaced numbers (for consistency)
            return .system(size: finalSize, weight: weight, design: .rounded)
        }

        // Default system font
        return .system(size: finalSize, weight: weight)
    }

    private var letterSpacing: CGFloat {
        let settings = accessibilityManager.settings

        if settings.dyslexiaFont && settings.extraLetterSpacing {
            // 0.05em extra spacing recommended for dyslexia
            return baseSize * 0.05
        }

        return 0
    }

    private var lineSpacing: CGFloat {
        let settings = accessibilityManager.settings

        if settings.dyslexiaFont && settings.increasedLineHeight {
            // 1.5x line height recommended for dyslexia
            return baseSize * 0.5
        }

        // Use the general line spacing setting
        return baseSize * (accessibilityManager.getLineSpacing() - 1.0)
    }

    /// Attempt to load OpenDyslexic or similar dyslexia-friendly font
    private func loadDyslexicFont(size: CGFloat) -> Font? {
        // List of dyslexia-friendly fonts to try
        let dyslexicFonts = [
            "OpenDyslexic",
            "OpenDyslexic-Regular",
            "Dyslexie",
            "Comic Sans MS",  // Surprisingly good for dyslexia
            "Verdana"         // Clear, sans-serif fallback
        ]

        for fontName in dyslexicFonts {
            if NSFont(name: fontName, size: size) != nil {
                return Font.custom(fontName, size: size)
            }
        }

        return nil
    }
}

/// Additional modifier for dyslexia-friendly text containers
struct DyslexiaTextContainerModifier: ViewModifier {
    @EnvironmentObject private var accessibilityManager: AccessibilityManager

    func body(content: Content) -> some View {
        content
            .padding(extraPadding)
            .background(backgroundColor)
            .cornerRadius(cornerRadius)
    }

    private var extraPadding: CGFloat {
        let settings = accessibilityManager.settings

        if settings.dyslexiaFont {
            // Extra padding helps with visual crowding
            return 8
        }

        return 4
    }

    private var backgroundColor: Color {
        let settings = accessibilityManager.settings

        if settings.dyslexiaFont {
            // Slight off-white background reduces glare
            return Color(white: 0.98)
        }

        return Color.clear
    }

    private var cornerRadius: CGFloat {
        let settings = accessibilityManager.settings

        if settings.dyslexiaFont {
            return 6
        }

        return 0
    }
}

/// Modifier for making text more readable for dyslexia
struct ReadableTextModifier: ViewModifier {
    @EnvironmentObject private var accessibilityManager: AccessibilityManager

    let alignment: TextAlignment

    func body(content: Content) -> some View {
        content
            .multilineTextAlignment(alignment)
            .lineLimit(nil)
            .fixedSize(horizontal: false, vertical: true)
            .frame(maxWidth: maxWidth)
    }

    private var maxWidth: CGFloat? {
        let settings = accessibilityManager.settings

        if settings.dyslexiaFont {
            // Optimal line length for readability: 50-75 characters
            // At ~10px per character average, that's 500-750px
            return 650
        }

        return nil
    }
}

// MARK: - View Extensions

extension View {
    /// Apply dyslexia-friendly font styling
    func dyslexiaFont(
        size: CGFloat = 16,
        weight: Font.Weight = .regular,
        relativeTo: Font.TextStyle = .body
    ) -> some View {
        modifier(DyslexiaFontModifier(size: size, weight: weight, relativeTo: relativeTo))
    }

    /// Apply dyslexia-friendly text container styling
    func dyslexiaTextContainer() -> some View {
        modifier(DyslexiaTextContainerModifier())
    }

    /// Apply readable text formatting for dyslexia
    func readableText(alignment: TextAlignment = .leading) -> some View {
        modifier(ReadableTextModifier(alignment: alignment))
    }

    /// Apply all dyslexia-friendly modifiers at once
    func dyslexiaOptimized(
        fontSize: CGFloat = 16,
        weight: Font.Weight = .regular,
        alignment: TextAlignment = .leading
    ) -> some View {
        self
            .dyslexiaFont(size: fontSize, weight: weight)
            .readableText(alignment: alignment)
            .dyslexiaTextContainer()
    }
}

// MARK: - Dyslexia-Friendly Components

/// A text view optimized for dyslexia
struct DyslexiaFriendlyText: View {
    @EnvironmentObject private var accessibilityManager: AccessibilityManager

    let text: String
    let size: CGFloat
    let weight: Font.Weight
    let color: Color

    init(
        _ text: String,
        size: CGFloat = 16,
        weight: Font.Weight = .regular,
        color: Color = .primary
    ) {
        self.text = text
        self.size = size
        self.weight = weight
        self.color = color
    }

    var body: some View {
        Text(text)
            .foregroundColor(textColor)
            .dyslexiaOptimized(fontSize: size, weight: weight)
    }

    private var textColor: Color {
        let settings = accessibilityManager.settings

        if settings.highContrast {
            // High contrast mode: pure black text
            return .black
        }

        return color
    }
}

/// A button optimized for dyslexia and motor accessibility
struct DyslexiaFriendlyButton: View {
    @EnvironmentObject private var accessibilityManager: AccessibilityManager

    let title: String
    let icon: String?
    let action: () -> Void

    init(_ title: String, icon: String? = nil, action: @escaping () -> Void) {
        self.title = title
        self.icon = icon
        self.action = action
    }

    var body: some View {
        Button(action: action) {
            HStack(spacing: 12) {
                if let icon = icon {
                    Image(systemName: icon)
                        .font(.system(size: 18, weight: .medium))
                }

                Text(title)
                    .dyslexiaFont(size: 16, weight: .medium)
            }
            .padding(.horizontal, 20)
            .padding(.vertical, 12)
            .frame(minWidth: minButtonWidth, minHeight: minButtonHeight)
            .background(buttonBackground)
            .foregroundColor(.white)
            .cornerRadius(8)
        }
        .buttonStyle(PlainButtonStyle())
        .contentShape(Rectangle())
    }

    private var minButtonWidth: CGFloat {
        // Motor accessibility: larger targets
        return 100
    }

    private var minButtonHeight: CGFloat {
        // WCAG 2.1 AA: minimum 44x44 points
        return 44
    }

    private var buttonBackground: Color {
        let settings = accessibilityManager.settings

        if settings.highContrast {
            return .black
        }

        return .accentColor
    }
}

// MARK: - Preview Provider

#Preview("Dyslexia Font Comparison") {
    VStack(spacing: 30) {
        VStack(alignment: .leading, spacing: 10) {
            Text("Regular Text")
                .font(.headline)

            Text("The quick brown fox jumps over the lazy dog. This text uses the standard system font without any dyslexia optimizations.")
                .font(.system(size: 16))
        }
        .padding()
        .background(Color.gray.opacity(0.1))
        .cornerRadius(8)

        VStack(alignment: .leading, spacing: 10) {
            Text("Dyslexia-Optimized Text")
                .font(.headline)

            DyslexiaFriendlyText(
                "The quick brown fox jumps over the lazy dog. This text uses dyslexia-friendly formatting with extra spacing and optimized font.",
                size: 16
            )
        }
        .padding()
        .background(Color.gray.opacity(0.1))
        .cornerRadius(8)

        DyslexiaFriendlyButton("Click Me", icon: "hand.tap") {
            print("Button tapped")
        }
    }
    .padding()
    .environmentObject(AccessibilityManager.shared)
}
