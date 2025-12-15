/**
 * CONVERGIO NATIVE - Onboarding View
 *
 * First-time user experience with setup wizard and feature tour.
 * Uses Liquid Glass design language with smooth animations.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

// MARK: - Onboarding View

struct OnboardingView: View {
    @Binding var isComplete: Bool
    @EnvironmentObject var keychainManager: KeychainManager
    @State private var currentStep = 0
    @State private var apiKey = ""
    @State private var selectedProvider: APIProvider = .anthropic

    private let totalSteps = 5

    var body: some View {
        VStack(spacing: 0) {
            // Progress indicator
            OnboardingProgressBar(current: currentStep, total: totalSteps)
                .padding(.top, 20)

            // Content area
            TabView(selection: $currentStep) {
                WelcomeStep()
                    .tag(0)

                MeetTheTeamStep()
                    .tag(1)

                ProviderSetupStep(
                    selectedProvider: $selectedProvider,
                    apiKey: $apiKey
                )
                    .tag(2)

                BudgetSetupStep()
                    .tag(3)

                ReadyStep()
                    .tag(4)
            }
            .tabViewStyle(.automatic)
            .animation(.easeInOut, value: currentStep)

            // Navigation buttons
            HStack {
                if currentStep > 0 {
                    Button("Back") {
                        withAnimation {
                            currentStep -= 1
                        }
                    }
                    .buttonStyle(.plain)
                    .foregroundStyle(.secondary)
                }

                Spacer()

                if currentStep < totalSteps - 1 {
                    Button("Continue") {
                        withAnimation {
                            currentStep += 1
                        }
                    }
                    .buttonStyle(.borderedProminent)
                    .disabled(currentStep == 2 && apiKey.isEmpty)
                } else {
                    Button("Get Started") {
                        completeOnboarding()
                    }
                    .buttonStyle(.borderedProminent)
                }
            }
            .padding(30)
        }
        .frame(width: 700, height: 550)
        .background(
            LinearGradient(
                colors: [
                    Color.purple.opacity(0.05),
                    Color.blue.opacity(0.05)
                ],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
        )
    }

    private func completeOnboarding() {
        // Save API key to Keychain
        if !apiKey.isEmpty {
            keychainManager.saveKey(apiKey, for: selectedProvider)
        }

        UserDefaults.standard.set(true, forKey: "onboardingComplete")
        UserDefaults.standard.set(selectedProvider.rawValue, forKey: "selectedProvider")
        isComplete = false // This dismisses the sheet
    }
}

// MARK: - Progress Bar

private struct OnboardingProgressBar: View {
    let current: Int
    let total: Int

    var body: some View {
        HStack(spacing: 8) {
            ForEach(0..<total, id: \.self) { index in
                Capsule()
                    .fill(index <= current ? Color.purple : Color.gray.opacity(0.3))
                    .frame(height: 4)
                    .animation(.easeInOut, value: current)
            }
        }
        .padding(.horizontal, 40)
    }
}

// MARK: - Welcome Step

private struct WelcomeStep: View {
    @State private var animateIcon = false

    var body: some View {
        VStack(spacing: 24) {
            Spacer()

            // Animated logo
            ZStack {
                Circle()
                    .fill(Color.purple.opacity(0.1))
                    .frame(width: 140, height: 140)
                    .scaleEffect(animateIcon ? 1.1 : 1.0)

                Image(systemName: "brain.head.profile")
                    .font(.system(size: 64))
                    .foregroundStyle(.purple.gradient)
                    .rotationEffect(.degrees(animateIcon ? 5 : -5))
            }
            .animation(
                .easeInOut(duration: 2).repeatForever(autoreverses: true),
                value: animateIcon
            )
            .onAppear { animateIcon = true }

            Text("Welcome to Convergio")
                .font(.largeTitle.weight(.bold))

            Text("Your AI Executive Team")
                .font(.title2)
                .foregroundStyle(.secondary)

            Text("Experience the power of collaborative AI agents working together to solve complex problems, generate content, and execute tasks.")
                .font(.body)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
                .frame(maxWidth: 450)
                .padding(.top, 8)

            Spacer()
        }
        .padding(40)
    }
}

// MARK: - Meet The Team Step

private struct MeetTheTeamStep: View {
    private let agents = [
        AgentPreview(name: "Alex", role: "Orchestrator", icon: "arrow.triangle.branch", color: .purple, description: "Coordinates the team and synthesizes responses"),
        AgentPreview(name: "Angela", role: "Analyst", icon: "chart.bar.fill", color: .blue, description: "Researches and analyzes information"),
        AgentPreview(name: "Amy", role: "Coder", icon: "chevron.left.forwardslash.chevron.right", color: .green, description: "Writes and reviews code"),
        AgentPreview(name: "Alan", role: "Writer", icon: "doc.text.fill", color: .orange, description: "Creates compelling content"),
        AgentPreview(name: "Arnold", role: "Critic", icon: "checkmark.circle.fill", color: .red, description: "Reviews and validates quality"),
        AgentPreview(name: "Ali", role: "Planner", icon: "list.bullet.clipboard", color: .indigo, description: "Plans and prioritizes tasks"),
    ]

    var body: some View {
        VStack(spacing: 24) {
            Text("Meet Your AI Team")
                .font(.title.weight(.bold))

            Text("Each agent specializes in different tasks and collaborates to deliver comprehensive solutions.")
                .font(.body)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
                .frame(maxWidth: 450)

            LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible()), GridItem(.flexible())], spacing: 16) {
                ForEach(agents, id: \.name) { agent in
                    AgentPreviewCard(agent: agent)
                }
            }
            .padding(.horizontal, 20)
        }
        .padding(40)
    }
}

private struct AgentPreview {
    let name: String
    let role: String
    let icon: String
    let color: Color
    let description: String
}

private struct AgentPreviewCard: View {
    let agent: AgentPreview

    var body: some View {
        VStack(spacing: 8) {
            ZStack {
                Circle()
                    .fill(agent.color.gradient)
                    .frame(width: 50, height: 50)

                Image(systemName: agent.icon)
                    .font(.title2)
                    .foregroundStyle(.white)
            }

            Text(agent.name)
                .font(.headline)

            Text(agent.role)
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .padding(12)
        .frame(maxWidth: .infinity)
        .background(Color.primary.opacity(0.03))
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

// MARK: - Provider Setup Step

private struct ProviderSetupStep: View {
    @Binding var selectedProvider: APIProvider
    @Binding var apiKey: String
    @State private var showApiKey = false

    private let providers: [APIProvider] = [.anthropic, .openai, .gemini]

    var body: some View {
        VStack(spacing: 24) {
            Text("Connect Your AI Provider")
                .font(.title.weight(.bold))

            Text("Choose your preferred AI provider and enter your API key to get started.")
                .font(.body)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
                .frame(maxWidth: 450)

            // Provider selection
            HStack(spacing: 16) {
                ForEach(providers) { provider in
                    ProviderOptionCard(
                        name: provider.displayName,
                        description: providerDescription(for: provider),
                        icon: provider.icon,
                        isSelected: selectedProvider == provider
                    ) {
                        selectedProvider = provider
                    }
                }
            }

            Divider()
                .padding(.horizontal, 40)

            // API Key input
            VStack(alignment: .leading, spacing: 8) {
                Text("API Key")
                    .font(.subheadline.weight(.medium))

                HStack {
                    if showApiKey {
                        TextField("Enter your API key", text: $apiKey)
                            .textFieldStyle(.roundedBorder)
                    } else {
                        SecureField("Enter your API key", text: $apiKey)
                            .textFieldStyle(.roundedBorder)
                    }

                    Button {
                        showApiKey.toggle()
                    } label: {
                        Image(systemName: showApiKey ? "eye.slash" : "eye")
                    }
                    .buttonStyle(.plain)
                }

                Text("Your API key is stored securely in the macOS Keychain.")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            .frame(maxWidth: 400)
        }
        .padding(40)
    }

    private func providerDescription(for provider: APIProvider) -> String {
        switch provider {
        case .anthropic: return "Claude models"
        case .openai: return "GPT models"
        case .gemini: return "Gemini models"
        case .openrouter: return "Multi-provider"
        case .perplexity: return "Search AI"
        case .grok: return "Grok models"
        }
    }
}

private struct ProviderOptionCard: View {
    let name: String
    let description: String
    let icon: String
    let isSelected: Bool
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            VStack(spacing: 12) {
                Image(systemName: icon)
                    .font(.largeTitle)
                    .foregroundStyle(isSelected ? .purple : .secondary)

                Text(name)
                    .font(.headline)

                Text(description)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            .padding(20)
            .frame(width: 150, height: 140)
            .background(isSelected ? Color.purple.opacity(0.1) : Color.primary.opacity(0.03))
            .clipShape(RoundedRectangle(cornerRadius: 16))
            .overlay(
                RoundedRectangle(cornerRadius: 16)
                    .stroke(isSelected ? Color.purple : Color.clear, lineWidth: 2)
            )
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Budget Setup Step

private struct BudgetSetupStep: View {
    @State private var budgetAmount: Double = 10.0

    var body: some View {
        VStack(spacing: 24) {
            Text("Set Your Budget")
                .font(.title.weight(.bold))

            Text("Control your AI spending with a session budget. You'll be notified when approaching the limit.")
                .font(.body)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
                .frame(maxWidth: 450)

            // Budget visualization
            ZStack {
                Circle()
                    .stroke(Color.purple.opacity(0.2), lineWidth: 20)
                    .frame(width: 180, height: 180)

                Circle()
                    .trim(from: 0, to: budgetAmount / 100)
                    .stroke(Color.purple, style: StrokeStyle(lineWidth: 20, lineCap: .round))
                    .frame(width: 180, height: 180)
                    .rotationEffect(.degrees(-90))
                    .animation(.easeInOut, value: budgetAmount)

                VStack {
                    Text(String(format: "$%.0f", budgetAmount))
                        .font(.system(size: 36, weight: .bold, design: .rounded))

                    Text("per session")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }
            .padding(.vertical, 20)

            // Budget slider
            VStack(spacing: 8) {
                Slider(value: $budgetAmount, in: 1...100, step: 1)
                    .frame(width: 300)

                HStack {
                    Text("$1")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    Spacer()
                    Text("$100")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
                .frame(width: 300)
            }

            Text("You can change this anytime in Settings.")
                .font(.caption)
                .foregroundStyle(.tertiary)
        }
        .padding(40)
    }
}

// MARK: - Ready Step

private struct ReadyStep: View {
    @State private var animateCheckmark = false

    var body: some View {
        VStack(spacing: 24) {
            Spacer()

            // Success animation
            ZStack {
                Circle()
                    .fill(Color.green.opacity(0.1))
                    .frame(width: 140, height: 140)
                    .scaleEffect(animateCheckmark ? 1.0 : 0.8)

                Image(systemName: "checkmark.circle.fill")
                    .font(.system(size: 80))
                    .foregroundStyle(.green)
                    .scaleEffect(animateCheckmark ? 1.0 : 0.5)
            }
            .animation(.spring(response: 0.5, dampingFraction: 0.6), value: animateCheckmark)
            .onAppear {
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
                    animateCheckmark = true
                }
            }

            Text("You're All Set!")
                .font(.largeTitle.weight(.bold))

            Text("Your AI Executive Team is ready to assist you.")
                .font(.title3)
                .foregroundStyle(.secondary)

            VStack(alignment: .leading, spacing: 12) {
                FeatureCheckItem(text: "Multi-agent collaboration")
                FeatureCheckItem(text: "Real-time cost tracking")
                FeatureCheckItem(text: "Global keyboard shortcut")
                FeatureCheckItem(text: "Native macOS experience")
            }
            .padding(.top, 16)

            Spacer()
        }
        .padding(40)
    }
}

private struct FeatureCheckItem: View {
    let text: String

    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: "checkmark.circle.fill")
                .foregroundStyle(.green)
            Text(text)
                .font(.body)
        }
    }
}

// MARK: - Help View

struct HelpView: View {
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(alignment: .leading, spacing: 24) {
                    HelpSection(
                        title: "Getting Started",
                        icon: "play.circle.fill",
                        items: [
                            "Type your question in the chat input",
                            "Press Cmd+Return to send",
                            "Watch agents collaborate in real-time",
                        ]
                    )

                    HelpSection(
                        title: "Keyboard Shortcuts",
                        icon: "keyboard.fill",
                        items: [
                            "Cmd+Shift+Space: Activate from anywhere",
                            "Cmd+N: New conversation",
                            "Cmd+Return: Send message",
                            "Cmd+.: Cancel current request",
                        ]
                    )

                    HelpSection(
                        title: "Cost Management",
                        icon: "dollarsign.circle.fill",
                        items: [
                            "Track spending in real-time",
                            "Set session and monthly budgets",
                            "Receive warnings before limits",
                        ]
                    )

                    HelpSection(
                        title: "Agent Roles",
                        icon: "person.3.fill",
                        items: [
                            "Orchestrator: Coordinates the team",
                            "Analyst: Research and analysis",
                            "Coder: Programming tasks",
                            "Writer: Content creation",
                            "Critic: Quality review",
                        ]
                    )
                }
                .padding(24)
            }
            .navigationTitle("Help & Tips")
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("Done") {
                        dismiss()
                    }
                }
            }
        }
        .frame(width: 500, height: 500)
    }
}

private struct HelpSection: View {
    let title: String
    let icon: String
    let items: [String]

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Image(systemName: icon)
                    .foregroundStyle(.purple)
                Text(title)
                    .font(.headline)
            }

            VStack(alignment: .leading, spacing: 8) {
                ForEach(items, id: \.self) { item in
                    HStack(alignment: .top, spacing: 8) {
                        Text("•")
                            .foregroundStyle(.secondary)
                        Text(item)
                            .font(.body)
                    }
                }
            }
            .padding(.leading, 28)
        }
        .padding(16)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Color.primary.opacity(0.03))
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

// MARK: - Preview

#Preview("Onboarding") {
    OnboardingView(isComplete: .constant(false))
}

#Preview("Help") {
    HelpView()
}
