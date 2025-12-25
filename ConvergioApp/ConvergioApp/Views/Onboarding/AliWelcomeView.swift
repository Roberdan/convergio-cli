/**
 * CONVERGIO NATIVE - Ali Welcome View
 *
 * Personalized welcome from Ali, the school principal.
 * Introduces the 17 maestri concept and completes onboarding.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

// MARK: - Ali Welcome View

struct AliWelcomeView: View {
    let studentName: String
    @Binding var isComplete: Bool
    let onComplete: () -> Void

    @State private var animateAli = false
    @State private var showMaestri = false
    @State private var currentMaestro = 0

    private let maestri = [
        ("Matematica", "function", "blue"),
        ("Italiano", "book.fill", "red"),
        ("Storia", "building.columns.fill", "brown"),
        ("Scienze", "atom", "green"),
        ("Inglese", "globe.europe.africa.fill", "indigo"),
        ("Arte", "paintbrush.fill", "orange"),
        ("Musica", "music.note", "pink"),
        ("Educazione Fisica", "figure.run", "cyan"),
        ("Geografia", "map.fill", "teal"),
        ("Filosofia", "brain.head.profile", "purple"),
        ("Fisica", "bolt.fill", "yellow"),
        ("Chimica", "flask.fill", "mint"),
        ("Latino", "scroll.fill", "brown"),
        ("Greco", "scroll.fill", "indigo"),
        ("Informatica", "laptopcomputer", "blue"),
        ("Diritto", "books.vertical.fill", "gray"),
        ("Economia", "chart.line.uptrend.xyaxis", "green")
    ]

    var body: some View {
        ZStack {
            // Animated background
            AnimatedGradientBackground()

            ScrollView {
                VStack(spacing: 32) {
                    Spacer(minLength: 40)

                    // Ali's Avatar
                    AliAvatar(isAnimating: $animateAli)
                        .onAppear {
                            withAnimation(.spring(response: 0.6, dampingFraction: 0.7)) {
                                animateAli = true
                            }

                            // Show maestri after a delay
                            DispatchQueue.main.asyncAfter(deadline: .now() + 1.5) {
                                withAnimation {
                                    showMaestri = true
                                }
                            }
                        }

                    // Welcome message
                    VStack(spacing: 16) {
                        Text("Ciao \(studentName)! 👋")
                            .font(.system(size: 36, weight: .bold, design: .rounded))
                            .multilineTextAlignment(.center)

                        Text("Sono Ali, il tuo assistente personale")
                            .font(.title2)
                            .foregroundStyle(.secondary)
                            .multilineTextAlignment(.center)

                        Text("Mi occupo di coordinare un team di 17 maestri specializzati, pronti ad aiutarti in ogni materia!")
                            .font(.body)
                            .foregroundStyle(.secondary)
                            .multilineTextAlignment(.center)
                            .frame(maxWidth: 500)
                            .padding(.top, 8)
                    }
                    .opacity(animateAli ? 1 : 0)
                    .offset(y: animateAli ? 0 : 20)
                    .animation(.easeOut(duration: 0.8).delay(0.3), value: animateAli)

                    // 17 Maestri visualization
                    if showMaestri {
                        VStack(spacing: 24) {
                            Text("I Tuoi 17 Maestri")
                                .font(.title.weight(.bold))
                                .transition(.scale.combined(with: .opacity))

                            LazyVGrid(
                                columns: [
                                    GridItem(.adaptive(minimum: 80), spacing: 12)
                                ],
                                spacing: 12
                            ) {
                                ForEach(Array(maestri.enumerated()), id: \.offset) { index, maestro in
                                    MaestroIcon(
                                        name: maestro.0,
                                        icon: maestro.1,
                                        color: maestro.2,
                                        delay: Double(index) * 0.05
                                    )
                                }
                            }
                            .padding(.horizontal, 40)
                            .transition(.scale.combined(with: .opacity))
                        }
                    }

                    // Call to action
                    VStack(spacing: 16) {
                        Text("Cosa vuoi studiare oggi?")
                            .font(.title3.weight(.semibold))
                            .foregroundStyle(.secondary)

                        Button {
                            completeOnboarding()
                        } label: {
                            HStack(spacing: 12) {
                                Text("Inizia a Studiare")
                                    .font(.title3.weight(.semibold))
                                Image(systemName: "arrow.right.circle.fill")
                                    .font(.title2)
                            }
                            .foregroundColor(.white)
                            .padding(.horizontal, 40)
                            .padding(.vertical, 18)
                            .background(
                                LinearGradient(
                                    colors: [.purple, .blue],
                                    startPoint: .leading,
                                    endPoint: .trailing
                                )
                            )
                            .clipShape(Capsule())
                            .shadow(color: .purple.opacity(0.3), radius: 10, x: 0, y: 5)
                        }
                        .buttonStyle(.plain)
                        .scaleEffect(showMaestri ? 1 : 0.8)
                        .opacity(showMaestri ? 1 : 0)
                        .animation(.spring(response: 0.6, dampingFraction: 0.7).delay(1.5), value: showMaestri)
                    }
                    .padding(.top, 20)

                    Spacer(minLength: 40)
                }
            }
        }
        .frame(width: 800, height: 700)
    }

    private func completeOnboarding() {
        onComplete()

        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            isComplete = true
        }
    }
}

// MARK: - Ali Avatar

private struct AliAvatar: View {
    @Binding var isAnimating: Bool
    @State private var rotationAngle: Double = 0
    @State private var pulseScale: CGFloat = 1

    var body: some View {
        ZStack {
            // Outer glow rings
            ForEach(0..<3) { index in
                Circle()
                    .stroke(
                        LinearGradient(
                            colors: [.purple.opacity(0.3), .blue.opacity(0.3)],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        ),
                        lineWidth: 2
                    )
                    .frame(width: 140 + CGFloat(index * 20), height: 140 + CGFloat(index * 20))
                    .opacity(isAnimating ? 0.3 : 0)
                    .scaleEffect(isAnimating ? 1 : 0.8)
                    .animation(
                        .easeOut(duration: 1.5)
                        .delay(Double(index) * 0.2)
                        .repeatForever(autoreverses: true),
                        value: isAnimating
                    )
            }

            // Main avatar circle
            Circle()
                .fill(
                    LinearGradient(
                        colors: [.purple, .blue, .cyan],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .frame(width: 140, height: 140)
                .scaleEffect(pulseScale)
                .animation(
                    .easeInOut(duration: 2)
                    .repeatForever(autoreverses: true),
                    value: pulseScale
                )
                .onAppear {
                    pulseScale = 1.05
                }

            // Ali icon
            VStack(spacing: 8) {
                Image(systemName: "brain.head.profile")
                    .font(.system(size: 50))
                    .foregroundStyle(.white)

                Text("Ali")
                    .font(.headline)
                    .foregroundStyle(.white)
            }
            .rotationEffect(.degrees(rotationAngle))
            .animation(
                .linear(duration: 20)
                .repeatForever(autoreverses: false),
                value: rotationAngle
            )
            .onAppear {
                rotationAngle = 360
            }
        }
        .scaleEffect(isAnimating ? 1 : 0.5)
        .opacity(isAnimating ? 1 : 0)
    }
}

// MARK: - Maestro Icon

private struct MaestroIcon: View {
    let name: String
    let icon: String
    let color: String
    let delay: Double

    @State private var isVisible = false

    var body: some View {
        VStack(spacing: 8) {
            ZStack {
                Circle()
                    .fill(Color(color).opacity(0.2))
                    .frame(width: 60, height: 60)

                Image(systemName: icon)
                    .font(.title2)
                    .foregroundStyle(Color(color))
            }

            Text(name)
                .font(.caption2)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
                .lineLimit(2)
                .frame(height: 30)
        }
        .scaleEffect(isVisible ? 1 : 0.5)
        .opacity(isVisible ? 1 : 0)
        .onAppear {
            withAnimation(.spring(response: 0.5, dampingFraction: 0.7).delay(delay)) {
                isVisible = true
            }
        }
    }
}

// MARK: - Animated Gradient Background

private struct AnimatedGradientBackground: View {
    @State private var animateGradient = false

    var body: some View {
        LinearGradient(
            colors: [
                Color.purple.opacity(0.15),
                Color.blue.opacity(0.15),
                Color.cyan.opacity(0.15),
                Color.green.opacity(0.1)
            ],
            startPoint: animateGradient ? .topLeading : .bottomLeading,
            endPoint: animateGradient ? .bottomTrailing : .topTrailing
        )
        .ignoresSafeArea()
        .onAppear {
            withAnimation(.easeInOut(duration: 5).repeatForever(autoreverses: true)) {
                animateGradient = true
            }
        }
    }
}

// MARK: - Preview

#Preview("Ali Welcome") {
    AliWelcomeView(
        studentName: "Marco",
        isComplete: .constant(false),
        onComplete: {}
    )
}
