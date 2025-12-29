/**
 * CONVERGIO NATIVE - Design System
 *
 * Centralized design tokens for consistent UI across the application.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

enum DesignSystem {
    // MARK: - Colors
    
    enum Colors {
        // Primary
        static let primary = Color(red: 0.2, green: 0.4, blue: 0.9)
        static let primaryDark = Color(red: 0.1, green: 0.3, blue: 0.8)
        static let primaryLight = Color(red: 0.3, green: 0.5, blue: 1.0)
        
        // Semantic
        static let success = Color.green
        static let warning = Color.orange
        static let error = Color.red
        static let info = Color.blue
        
        // Background
        static let background = Color(nsColor: .windowBackgroundColor)
        static let surface = Color(nsColor: .textBackgroundColor)
        static let overlay = Color.black.opacity(0.3)
        static let overlayLight = Color.white.opacity(0.1)
        
        // Text
        static let textPrimary = Color.primary
        static let textSecondary = Color.secondary
        static let textTertiary = Color(white: 0.5)
        
        // Voice specific
        static let voiceListening = Color.cyan
        static let voiceSpeaking = Color.purple
        static let voiceProcessing = Color.yellow
    }
    
    // MARK: - Spacing
    
    enum Spacing {
        static let xs: CGFloat = 4
        static let sm: CGFloat = 8
        static let md: CGFloat = 16
        static let lg: CGFloat = 24
        static let xl: CGFloat = 32
        static let xxl: CGFloat = 48
    }
    
    // MARK: - Typography
    
    enum Typography {
        static let largeTitle = Font.system(.largeTitle, weight: .bold)
        static let title = Font.system(.title, weight: .semibold)
        static let title2 = Font.system(.title2, weight: .semibold)
        static let headline = Font.system(.headline, weight: .semibold)
        static let body = Font.system(.body, weight: .regular)
        static let callout = Font.system(.callout, weight: .regular)
        static let subheadline = Font.system(.subheadline, weight: .regular)
        static let footnote = Font.system(.footnote, weight: .regular)
        static let caption = Font.system(.caption, weight: .regular)
        static let caption2 = Font.system(.caption2, weight: .regular)
    }
    
    // MARK: - Corner Radius
    
    enum CornerRadius {
        static let small: CGFloat = 4
        static let medium: CGFloat = 8
        static let large: CGFloat = 12
        static let xlarge: CGFloat = 16
        static let round: CGFloat = 999
    }
    
    // MARK: - Shadows
    
    enum Shadow {
        static let small = ShadowStyle(color: .black.opacity(0.1), radius: 2, x: 0, y: 1)
        static let medium = ShadowStyle(color: .black.opacity(0.15), radius: 4, x: 0, y: 2)
        static let large = ShadowStyle(color: .black.opacity(0.2), radius: 8, x: 0, y: 4)
        
        struct ShadowStyle {
            let color: Color
            let radius: CGFloat
            let x: CGFloat
            let y: CGFloat
        }
    }
    
    // MARK: - Animation
    
    enum Animation {
        static let quick = SwiftUI.Animation.spring(response: 0.2, dampingFraction: 0.8)
        static let smooth = SwiftUI.Animation.spring(response: 0.3, dampingFraction: 0.7)
        static let gentle = SwiftUI.Animation.spring(response: 0.4, dampingFraction: 0.6)
    }
}

// MARK: - Button Style

enum DesignButtonStyle {
    case primary
    case secondary
}

// MARK: - View Modifiers

extension View {
    func designSystemCard() -> some View {
        self
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.surface)
            .cornerRadius(DesignSystem.CornerRadius.medium)
    }

    func designSystemButton(style: DesignButtonStyle = .primary) -> some View {
        self
            .padding(.horizontal, DesignSystem.Spacing.lg)
            .padding(.vertical, DesignSystem.Spacing.md)
            .background(style == .primary ? DesignSystem.Colors.primary : DesignSystem.Colors.surface)
            .foregroundColor(style == .primary ? .white : DesignSystem.Colors.textPrimary)
            .cornerRadius(DesignSystem.CornerRadius.medium)
            .shadow(color: DesignSystem.Shadow.medium.color, radius: DesignSystem.Shadow.medium.radius, x: DesignSystem.Shadow.medium.x, y: DesignSystem.Shadow.medium.y)
    }
}










