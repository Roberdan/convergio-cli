/**
 * CONVERGIO NATIVE - Glass Effect Components
 *
 * Reusable glass-styled components following Apple's Liquid Glass design language.
 * Includes fallback implementations for macOS versions before Tahoe (26).
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

// MARK: - Glass Card

/// A card container with glass-like appearance
struct GlassCard<Content: View>: View {
    let content: Content
    var cornerRadius: CGFloat = 16
    var tint: Color = .white

    init(
        cornerRadius: CGFloat = 16,
        tint: Color = .white,
        @ViewBuilder content: () -> Content
    ) {
        self.cornerRadius = cornerRadius
        self.tint = tint
        self.content = content()
    }

    var body: some View {
        content
            .background(glassBackground)
            .clipShape(RoundedRectangle(cornerRadius: cornerRadius))
            .overlay(
                RoundedRectangle(cornerRadius: cornerRadius)
                    .stroke(tint.opacity(0.2), lineWidth: 0.5)
            )
            .shadow(color: .black.opacity(0.1), radius: 10, x: 0, y: 4)
    }

    @ViewBuilder
    private var glassBackground: some View {
        ZStack {
            // Base blur effect
            VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)

            // Tint overlay
            tint.opacity(0.05)

            // Subtle gradient for depth
            LinearGradient(
                colors: [
                    .white.opacity(0.15),
                    .clear,
                    .black.opacity(0.05)
                ],
                startPoint: .top,
                endPoint: .bottom
            )
        }
    }
}

// MARK: - Visual Effect Blur

/// NSVisualEffectView wrapper for SwiftUI
struct VisualEffectBlur: NSViewRepresentable {
    var material: NSVisualEffectView.Material
    var blendingMode: NSVisualEffectView.BlendingMode

    func makeNSView(context: Context) -> NSVisualEffectView {
        let view = NSVisualEffectView()
        view.material = material
        view.blendingMode = blendingMode
        view.state = .active
        return view
    }

    func updateNSView(_ nsView: NSVisualEffectView, context: Context) {
        nsView.material = material
        nsView.blendingMode = blendingMode
    }
}

// MARK: - Glass Button

/// A button with glass-like appearance
struct GlassButton: View {
    let title: String
    let icon: String?
    let action: () -> Void
    var tint: Color = .accentColor

    @State private var isPressed = false

    var body: some View {
        Button(action: action) {
            HStack(spacing: 6) {
                if let icon {
                    Image(systemName: icon)
                        .font(.body.weight(.medium))
                }
                Text(title)
                    .font(.body.weight(.medium))
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 8)
            .background(
                ZStack {
                    VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)
                    tint.opacity(isPressed ? 0.2 : 0.1)
                }
            )
            .foregroundStyle(tint)
            .clipShape(Capsule())
            .overlay(
                Capsule()
                    .stroke(tint.opacity(0.3), lineWidth: 0.5)
            )
        }
        .buttonStyle(.plain)
        .scaleEffect(isPressed ? 0.95 : 1.0)
        .onLongPressGesture(minimumDuration: 0, pressing: { pressing in
            withAnimation(.easeInOut(duration: 0.1)) {
                isPressed = pressing
            }
        }, perform: {})
    }
}

// MARK: - Glass Input Field

/// A text field with glass-like appearance
struct GlassTextField: View {
    let placeholder: String
    @Binding var text: String
    var icon: String? = nil

    @FocusState private var isFocused: Bool

    var body: some View {
        HStack(spacing: 8) {
            if let icon {
                Image(systemName: icon)
                    .foregroundStyle(.secondary)
            }

            TextField(placeholder, text: $text)
                .textFieldStyle(.plain)
                .focused($isFocused)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 10)
        .background(
            ZStack {
                VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)
                Color.primary.opacity(0.05)
            }
        )
        .clipShape(RoundedRectangle(cornerRadius: 10))
        .overlay(
            RoundedRectangle(cornerRadius: 10)
                .stroke(isFocused ? Color.accentColor.opacity(0.5) : Color.primary.opacity(0.1), lineWidth: 1)
        )
        .animation(.easeInOut(duration: 0.2), value: isFocused)
    }
}

// MARK: - Glass Progress Bar

/// A progress bar with glass-like appearance
struct GlassProgressBar: View {
    let progress: Double
    var tint: Color = .accentColor
    var height: CGFloat = 8

    var body: some View {
        GeometryReader { geometry in
            ZStack(alignment: .leading) {
                // Background
                Capsule()
                    .fill(Color.primary.opacity(0.1))

                // Progress fill
                Capsule()
                    .fill(
                        LinearGradient(
                            colors: [tint, tint.opacity(0.7)],
                            startPoint: .leading,
                            endPoint: .trailing
                        )
                    )
                    .frame(width: geometry.size.width * CGFloat(min(1, max(0, progress))))

                // Glass shine effect
                Capsule()
                    .fill(
                        LinearGradient(
                            colors: [.white.opacity(0.3), .clear],
                            startPoint: .top,
                            endPoint: .bottom
                        )
                    )
                    .frame(width: geometry.size.width * CGFloat(min(1, max(0, progress))))
            }
        }
        .frame(height: height)
        .animation(.spring(duration: 0.3), value: progress)
    }
}

// MARK: - Glass Badge

/// A small badge with glass-like appearance
struct GlassBadge: View {
    let text: String
    var icon: String? = nil
    var tint: Color = .secondary

    var body: some View {
        HStack(spacing: 4) {
            if let icon {
                Image(systemName: icon)
                    .font(.caption2)
            }
            Text(text)
                .font(.caption)
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(
            ZStack {
                VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)
                tint.opacity(0.1)
            }
        )
        .foregroundStyle(tint)
        .clipShape(Capsule())
        .overlay(
            Capsule()
                .stroke(tint.opacity(0.2), lineWidth: 0.5)
        )
    }
}

// MARK: - Glass Container

/// A container that groups content with glass morphing support
struct GlassContainer<Content: View>: View {
    let content: Content
    var padding: CGFloat = 16
    var cornerRadius: CGFloat = 20

    init(
        padding: CGFloat = 16,
        cornerRadius: CGFloat = 20,
        @ViewBuilder content: () -> Content
    ) {
        self.padding = padding
        self.cornerRadius = cornerRadius
        self.content = content()
    }

    var body: some View {
        content
            .padding(padding)
            .background(
                ZStack {
                    VisualEffectBlur(material: .sidebar, blendingMode: .behindWindow)
                    Color.white.opacity(0.05)
                }
            )
            .clipShape(RoundedRectangle(cornerRadius: cornerRadius))
            .overlay(
                RoundedRectangle(cornerRadius: cornerRadius)
                    .stroke(Color.white.opacity(0.1), lineWidth: 0.5)
            )
    }
}

// MARK: - Preview

#Preview("Glass Components") {
    ZStack {
        // Background gradient
        LinearGradient(
            colors: [.purple.opacity(0.3), .blue.opacity(0.3)],
            startPoint: .topLeading,
            endPoint: .bottomTrailing
        )
        .ignoresSafeArea()

        VStack(spacing: 24) {
            GlassCard {
                VStack(alignment: .leading, spacing: 8) {
                    Text("Glass Card")
                        .font(.headline)
                    Text("A beautiful glass-like container")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
                .padding()
            }
            .frame(width: 250)

            HStack(spacing: 12) {
                GlassButton(title: "Primary", icon: "star.fill", action: {})
                GlassButton(title: "Secondary", icon: nil, action: {}, tint: .secondary)
            }

            GlassTextField(placeholder: "Enter text...", text: .constant(""), icon: "magnifyingglass")
                .frame(width: 250)

            GlassProgressBar(progress: 0.65, tint: .green)
                .frame(width: 250)

            HStack(spacing: 8) {
                GlassBadge(text: "Active", icon: "checkmark.circle.fill", tint: .green)
                GlassBadge(text: "Pending", icon: "clock", tint: .orange)
                GlassBadge(text: "Error", icon: "xmark.circle", tint: .red)
            }
        }
        .padding(40)
    }
}
