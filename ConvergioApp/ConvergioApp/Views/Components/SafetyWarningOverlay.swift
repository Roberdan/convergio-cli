/**
 * CONVERGIO NATIVE - Safety Warning Overlay
 *
 * P0 CRITICAL: Child-friendly UI for safety warnings
 * Gentle, non-scary overlay for ages 6-19
 * Redirects to trusted adults when needed
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

// MARK: - Safety Warning Overlay

/// Overlay displayed when content is blocked or flagged by SafetyFilter
struct SafetyWarningOverlay: View {
    let response: SafetyResponse
    let onDismiss: () -> Void

    @State private var scale: CGFloat = 0.8
    @State private var opacity: Double = 0

    var body: some View {
        ZStack {
            // Semi-transparent backdrop
            Color.black.opacity(0.4)
                .ignoresSafeArea()
                .onTapGesture {
                    dismiss()
                }

            // Warning card
            VStack(spacing: 0) {
                // Header with icon
                headerSection

                // Message content
                messageSection

                // Action buttons
                actionSection
            }
            .frame(maxWidth: 500)
            .background(glassBackground)
            .clipShape(RoundedRectangle(cornerRadius: 24))
            .overlay(
                RoundedRectangle(cornerRadius: 24)
                    .stroke(Color.white.opacity(0.2), lineWidth: 1)
            )
            .shadow(color: .black.opacity(0.3), radius: 30, x: 0, y: 10)
            .scaleEffect(scale)
            .opacity(opacity)
        }
        .onAppear {
            withAnimation(.spring(response: 0.4, dampingFraction: 0.7)) {
                scale = 1.0
                opacity = 1.0
            }

            // Log the safety event
            if response.shouldLog {
                logWarning(
                    "Safety warning displayed: \(response.category?.description ?? "unknown")",
                    category: "Safety"
                )
            }
        }
    }

    // MARK: - Header Section

    private var headerSection: some View {
        VStack(spacing: 12) {
            // Icon based on category
            iconForCategory
                .font(.system(size: 56))
                .padding(.top, 32)

            // Title
            Text(titleForCategory)
                .font(.title2.weight(.semibold))
                .multilineTextAlignment(.center)
        }
        .padding(.horizontal, 32)
        .padding(.bottom, 24)
    }

    // MARK: - Message Section

    private var messageSection: some View {
        VStack(spacing: 16) {
            if let message = response.redirectMessage {
                Text(message)
                    .font(.body)
                    .foregroundStyle(.primary)
                    .multilineTextAlignment(.center)
                    .lineSpacing(4)
            }
        }
        .padding(.horizontal, 32)
        .padding(.bottom, 32)
    }

    // MARK: - Action Section

    private var actionSection: some View {
        VStack(spacing: 12) {
            // Primary action - Talk to adult (for redirect cases)
            if shouldShowTalkToAdultButton {
                GlassButton(
                    title: "Parla con un adulto / Talk to an adult",
                    icon: "person.2.fill",
                    action: {
                        // Open parent notification or help resources
                        openHelpResources()
                        dismiss()
                    },
                    tint: .blue
                )
            }

            // Dismiss button
            GlassButton(
                title: shouldShowTalkToAdultButton ? "Ho capito / I understand" : "OK",
                icon: "checkmark.circle.fill",
                action: dismiss,
                tint: .secondary
            )
        }
        .padding(.horizontal, 32)
        .padding(.bottom, 32)
    }

    // MARK: - Glass Background

    @ViewBuilder
    private var glassBackground: some View {
        ZStack {
            // Base blur
            VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)

            // Gradient tint based on severity
            LinearGradient(
                colors: gradientColors,
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
            .opacity(0.15)

            // Subtle overlay gradient
            LinearGradient(
                colors: [
                    .white.opacity(0.2),
                    .clear,
                    .black.opacity(0.1)
                ],
                startPoint: .top,
                endPoint: .bottom
            )
        }
    }

    // MARK: - Computed Properties

    private var iconForCategory: some View {
        Group {
            switch response.category {
            case .selfHarm, .isolation:
                Image(systemName: "heart.circle.fill")
                    .foregroundStyle(.blue)
            case .violence:
                Image(systemName: "exclamationmark.triangle.fill")
                    .foregroundStyle(.orange)
            case .drugs:
                Image(systemName: "cross.circle.fill")
                    .foregroundStyle(.red)
            case .adult:
                Image(systemName: "hand.raised.fill")
                    .foregroundStyle(.purple)
            case .bullying:
                Image(systemName: "heart.circle.fill")
                    .foregroundStyle(.pink)
            case .privacy:
                Image(systemName: "lock.shield.fill")
                    .foregroundStyle(.green)
            case .jailbreak, .cheating, .none:
                Image(systemName: "info.circle.fill")
                    .foregroundStyle(.blue)
            }
        }
    }

    private var titleForCategory: String {
        switch response.category {
        case .selfHarm, .isolation:
            return "Ti vogliamo bene 💙 / We care about you 💙"
        case .violence:
            return "Non posso aiutarti con questo / I can't help with this"
        case .drugs:
            return "Argomento non adatto / Inappropriate topic"
        case .adult:
            return "Contenuto non adatto / Inappropriate content"
        case .bullying:
            return "Possiamo aiutarti / We can help you"
        case .privacy:
            return "Proteggi la tua privacy / Protect your privacy"
        case .jailbreak, .cheating, .none:
            return "Informazione importante / Important information"
        }
    }

    private var shouldShowTalkToAdultButton: Bool {
        switch response.category {
        case .selfHarm, .isolation, .bullying:
            return true
        default:
            return false
        }
    }

    private var gradientColors: [Color] {
        switch response.category {
        case .selfHarm, .isolation:
            return [.blue, .cyan]
        case .violence:
            return [.orange, .red]
        case .drugs:
            return [.red, .pink]
        case .adult:
            return [.purple, .pink]
        case .bullying:
            return [.pink, .blue]
        case .privacy:
            return [.green, .teal]
        case .jailbreak, .cheating, .none:
            return [.blue, .indigo]
        }
    }

    // MARK: - Actions

    private func dismiss() {
        withAnimation(.spring(response: 0.3, dampingFraction: 0.8)) {
            scale = 0.8
            opacity = 0
        }

        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
            onDismiss()
        }
    }

    private func openHelpResources() {
        // Open help resources or notification to parents
        logInfo("Opening help resources for safety concern", category: "Safety")

        // In production, this could:
        // 1. Send notification to parent's device
        // 2. Open emergency contacts
        // 3. Display school counselor info
        // 4. Show crisis hotline numbers

        // For now, log the event
        if let category = response.category {
            logWarning(
                "User requested help for: \(category.description) - action: talk_to_adult",
                category: "Safety"
            )
        }
    }
}

// MARK: - Compact Warning Badge

/// Small, non-intrusive badge for minor warnings
struct SafetyWarningBadge: View {
    let category: SafetyCategory
    let message: String
    let onDismiss: () -> Void

    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: iconName)
                .font(.body.weight(.semibold))
                .foregroundStyle(color)

            Text(message)
                .font(.callout)
                .foregroundStyle(.primary)
                .lineLimit(2)

            Spacer()

            Button(action: onDismiss) {
                Image(systemName: "xmark.circle.fill")
                    .font(.title3)
                    .foregroundStyle(.secondary)
            }
            .buttonStyle(.plain)
        }
        .padding(16)
        .background(
            ZStack {
                VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)
                color.opacity(0.1)
            }
        )
        .clipShape(RoundedRectangle(cornerRadius: 12))
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(color.opacity(0.3), lineWidth: 1)
        )
        .shadow(color: .black.opacity(0.15), radius: 10, x: 0, y: 4)
    }

    private var iconName: String {
        switch category {
        case .privacy: return "lock.shield.fill"
        case .jailbreak, .cheating: return "info.circle.fill"
        default: return "exclamationmark.triangle.fill"
        }
    }

    private var color: Color {
        switch category {
        case .privacy: return .green
        case .jailbreak, .cheating: return .blue
        default: return .orange
        }
    }
}

// MARK: - Preview

#Preview("Safety Warning - Self Harm") {
    ZStack {
        // Background
        LinearGradient(
            colors: [.purple.opacity(0.3), .blue.opacity(0.3)],
            startPoint: .topLeading,
            endPoint: .bottomTrailing
        )
        .ignoresSafeArea()

        SafetyWarningOverlay(
            response: SafetyResponse(
                blocked: true,
                category: .selfHarm,
                redirectMessage: "Mi sembra che tu stia affrontando un momento difficile. È molto importante che tu parla con un adulto di fiducia.\n\nI notice you might be going through a difficult time. It's very important that you talk to a trusted adult."
            ),
            onDismiss: {}
        )
    }
}

#Preview("Safety Warning - Violence") {
    ZStack {
        LinearGradient(
            colors: [.purple.opacity(0.3), .blue.opacity(0.3)],
            startPoint: .topLeading,
            endPoint: .bottomTrailing
        )
        .ignoresSafeArea()

        SafetyWarningOverlay(
            response: SafetyResponse(
                blocked: true,
                category: .violence,
                redirectMessage: "Non posso aiutarti con questo argomento. Se stai vivendo una situazione di pericolo, per favore parla con un adulto di fiducia.\n\nI can't help with this topic. If you're in a dangerous situation, please talk to a trusted adult."
            ),
            onDismiss: {}
        )
    }
}

#Preview("Safety Warning - Privacy") {
    ZStack {
        LinearGradient(
            colors: [.purple.opacity(0.3), .blue.opacity(0.3)],
            startPoint: .topLeading,
            endPoint: .bottomTrailing
        )
        .ignoresSafeArea()

        SafetyWarningOverlay(
            response: SafetyResponse(
                blocked: true,
                category: .privacy,
                redirectMessage: "⚠️ ATTENZIONE: Non condividere mai informazioni personali online!\n\n⚠️ WARNING: Never share personal information online!"
            ),
            onDismiss: {}
        )
    }
}

#Preview("Safety Badge - Privacy") {
    VStack {
        Spacer()

        SafetyWarningBadge(
            category: .privacy,
            message: "Ricorda: non condividere dati personali / Remember: don't share personal data",
            onDismiss: {}
        )
        .padding()
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .background(
        LinearGradient(
            colors: [.purple.opacity(0.3), .blue.opacity(0.3)],
            startPoint: .topLeading,
            endPoint: .bottomTrailing
        )
    )
}
