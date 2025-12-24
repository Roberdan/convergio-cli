/**
 * CONVERGIO NATIVE - Maestro Detail View
 *
 * Detailed profile view for a selected AI maestro.
 * Shows full information, teaching style, and action buttons.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

struct MaestroDetailView: View {
    let maestro: Maestro
    @State private var showingStudySession = false
    @State private var showingVoiceConversation = false
    @State private var isAvatarHovered = false

    var body: some View {
        ScrollView {
            VStack(spacing: 32) {
                // Header with avatar and basic info
                headerSection

                // Description section
                descriptionSection

                // Teaching style section
                teachingStyleSection

                // Action buttons
                actionButtonsSection

                Spacer(minLength: 40)
            }
            .padding(32)
        }
        .navigationTitle(maestro.name)
        .sheet(isPresented: $showingStudySession) {
            StudySessionView(maestro: maestro)
        }
        .sheet(isPresented: $showingVoiceConversation) {
            VoiceSessionView(maestro: maestro)
                .frame(width: 600, height: 700)
        }
    }

    // MARK: - Header Section

    private var headerSection: some View {
        VStack(spacing: 20) {
            // Large avatar with animated glow
            ZStack {
                // Animated glow
                Circle()
                    .fill(
                        RadialGradient(
                            colors: [
                                maestro.color.opacity(0.4),
                                maestro.color.opacity(0.2),
                                maestro.color.opacity(0.0)
                            ],
                            center: .center,
                            startRadius: 50,
                            endRadius: 100
                        )
                    )
                    .frame(width: 200, height: 200)
                    .blur(radius: 20)
                    .scaleEffect(isAvatarHovered ? 1.1 : 1.0)

                // Avatar ring
                Circle()
                    .stroke(
                        LinearGradient(
                            colors: [
                                maestro.color,
                                maestro.color.opacity(0.6),
                                maestro.color.opacity(0.8),
                                maestro.color
                            ],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        ),
                        lineWidth: 4
                    )
                    .frame(width: 140, height: 140)

                // Icon
                Image(systemName: maestro.icon)
                    .font(.system(size: 60))
                    .foregroundStyle(maestro.color)
            }
            .frame(width: 200, height: 200)
            .animation(.spring(duration: 0.5), value: isAvatarHovered)
            .onHover { hovering in
                isAvatarHovered = hovering
            }

            // Name and subject
            VStack(spacing: 8) {
                Text(maestro.name)
                    .font(.largeTitle.weight(.bold))

                HStack(spacing: 12) {
                    // Subject badge
                    HStack(spacing: 6) {
                        Image(systemName: maestro.subject.icon)
                            .font(.subheadline)
                        Text(maestro.subject.rawValue)
                            .font(.subheadline.weight(.medium))
                    }
                    .padding(.horizontal, 12)
                    .padding(.vertical, 6)
                    .background(maestro.color.opacity(0.2))
                    .foregroundStyle(maestro.color)
                    .clipShape(Capsule())

                    // Specialization
                    Text(maestro.specialization)
                        .font(.subheadline)
                        .foregroundStyle(.secondary)
                }
            }
        }
    }

    // MARK: - Description Section

    private var descriptionSection: some View {
        GlassCard(tint: maestro.color) {
            VStack(alignment: .leading, spacing: 12) {
                Label("Chi sono", systemImage: "person.text.rectangle")
                    .font(.headline)
                    .foregroundStyle(maestro.color)

                Text(maestro.description)
                    .font(.body)
                    .foregroundStyle(.primary)
                    .lineSpacing(4)
            }
            .padding(20)
        }
    }

    // MARK: - Teaching Style Section

    private var teachingStyleSection: some View {
        GlassCard(tint: maestro.color) {
            VStack(alignment: .leading, spacing: 12) {
                Label("Stile di insegnamento", systemImage: "graduationcap")
                    .font(.headline)
                    .foregroundStyle(maestro.color)

                Text(maestro.teachingStyle)
                    .font(.body)
                    .foregroundStyle(.primary)
                    .lineSpacing(4)

                // Key features
                VStack(alignment: .leading, spacing: 8) {
                    featureRow(icon: "brain.head.profile", text: "Approccio personalizzato")
                    featureRow(icon: "clock", text: "Sessioni ADHD-friendly (15 min)")
                    featureRow(icon: "message", text: "Conversazioni interattive")
                    featureRow(icon: "speaker.wave.2", text: "Supporto vocale disponibile")
                }
                .padding(.top, 8)
            }
            .padding(20)
        }
    }

    private func featureRow(icon: String, text: String) -> some View {
        HStack(spacing: 8) {
            Image(systemName: icon)
                .font(.caption)
                .foregroundStyle(maestro.color)
                .frame(width: 20)

            Text(text)
                .font(.caption)
                .foregroundStyle(.secondary)
        }
    }

    // MARK: - Action Buttons Section

    private var actionButtonsSection: some View {
        VStack(spacing: 12) {
            // Primary action: Start Study Session
            Button {
                showingStudySession = true
            } label: {
                HStack(spacing: 8) {
                    Image(systemName: "book.pages")
                        .font(.body.weight(.semibold))
                    Text("Inizia Sessione di Studio")
                        .font(.body.weight(.semibold))
                }
                .frame(maxWidth: .infinity)
                .padding(.vertical, 14)
                .background(
                    ZStack {
                        maestro.color
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
                )
                .foregroundStyle(.white)
                .clipShape(RoundedRectangle(cornerRadius: 12))
                .shadow(color: maestro.color.opacity(0.4), radius: 8, x: 0, y: 4)
            }
            .buttonStyle(.plain)

            // Secondary action: Voice Conversation
            Button {
                showingVoiceConversation = true
            } label: {
                HStack(spacing: 8) {
                    Image(systemName: "waveform")
                        .font(.body.weight(.medium))
                    Text("Conversazione Vocale")
                        .font(.body.weight(.medium))
                }
                .frame(maxWidth: .infinity)
                .padding(.vertical, 14)
                .background(
                    ZStack {
                        VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)
                        maestro.color.opacity(0.15)
                    }
                )
                .foregroundStyle(maestro.color)
                .clipShape(RoundedRectangle(cornerRadius: 12))
                .overlay(
                    RoundedRectangle(cornerRadius: 12)
                        .stroke(maestro.color.opacity(0.3), lineWidth: 1)
                )
            }
            .buttonStyle(.plain)
        }
        .frame(maxWidth: 400)
    }
}

// MARK: - Preview

#Preview {
    NavigationStack {
        MaestroDetailView(maestro: Maestro.preview)
    }
    .frame(width: 800, height: 900)
}
