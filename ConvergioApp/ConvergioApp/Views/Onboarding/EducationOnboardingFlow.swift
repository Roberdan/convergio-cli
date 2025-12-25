/**
 * CONVERGIO NATIVE - Education Onboarding Flow
 *
 * Multi-step wizard for student onboarding in the Italian school system.
 * Privacy-focused design with only essential information collection.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

// MARK: - Education Onboarding Flow

struct EducationOnboardingFlow: View {
    @Binding var isComplete: Bool
    @StateObject private var profileManager = StudentProfileManager.shared
    @State private var currentStep = 0
    @State private var profile = StudentProfile()
    @State private var showValidationError = false
    @State private var validationMessage = ""

    private let totalSteps = 5

    var body: some View {
        ZStack {
            // Background gradient
            LinearGradient(
                colors: [
                    Color.purple.opacity(0.1),
                    Color.blue.opacity(0.1),
                    Color.green.opacity(0.05)
                ],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
            .ignoresSafeArea()

            VStack(spacing: 0) {
                // Progress indicator
                OnboardingProgressBar(current: currentStep, total: totalSteps)
                    .padding(.top, 30)
                    .padding(.horizontal, 40)

                // Content area with page transitions
                TabView(selection: $currentStep) {
                    // Step 1: Welcome + Name
                    WelcomeNameStep(firstName: $profile.firstName)
                        .tag(0)

                    // Step 2: Age + School Year
                    AgeGradeStep(age: $profile.age, schoolYear: $profile.schoolYear, curriculum: $profile.curriculum)
                        .tag(1)

                    // Step 3: Curriculum Selection
                    CurriculumSelectionStep(curriculum: $profile.curriculum, schoolYear: $profile.schoolYear)
                        .tag(2)

                    // Step 4: Accessibility Preferences
                    AccessibilityStep(settings: $profile.accessibilitySettings)
                        .tag(3)

                    // Step 5: Meet Ali (handled separately)
                    Color.clear
                        .tag(4)
                }
                .tabViewStyle(.automatic)
                .animation(.easeInOut(duration: 0.3), value: currentStep)

                // Navigation buttons
                if currentStep < 4 {
                    HStack(spacing: 16) {
                        if currentStep > 0 {
                            Button {
                                withAnimation {
                                    currentStep -= 1
                                }
                            } label: {
                                HStack {
                                    Image(systemName: "chevron.left")
                                    Text("Indietro")
                                }
                                .frame(maxWidth: .infinity)
                            }
                            .buttonStyle(.bordered)
                            .controlSize(.large)
                        }

                        Button {
                            handleNext()
                        } label: {
                            HStack {
                                Text(currentStep == 3 ? "Completa" : "Avanti")
                                Image(systemName: "chevron.right")
                            }
                            .frame(maxWidth: .infinity)
                        }
                        .buttonStyle(.borderedProminent)
                        .controlSize(.large)
                        .disabled(!canProceed)
                    }
                    .padding(.horizontal, 40)
                    .padding(.bottom, 30)
                }
            }
        }
        .frame(width: 800, height: 600)
        .alert("Attenzione", isPresented: $showValidationError) {
            Button("OK", role: .cancel) {}
        } message: {
            Text(validationMessage)
        }
        .sheet(isPresented: Binding(
            get: { currentStep == 4 },
            set: { if !$0 { currentStep = 3 } }
        )) {
            AliWelcomeView(
                studentName: profile.firstName,
                isComplete: $isComplete,
                onComplete: completeOnboarding
            )
        }
    }

    private var canProceed: Bool {
        switch currentStep {
        case 0:
            return !profile.firstName.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
        case 1:
            return profile.age >= 6 && profile.age <= 19
        case 2:
            return true
        case 3:
            return true
        default:
            return false
        }
    }

    private func handleNext() {
        // Validate current step
        if currentStep == 0 {
            let trimmedName = profile.firstName.trimmingCharacters(in: .whitespacesAndNewlines)
            if trimmedName.count < 2 {
                validationMessage = "Il nome deve contenere almeno 2 caratteri"
                showValidationError = true
                return
            }
            profile.firstName = trimmedName
        }

        if currentStep == 1 {
            // Auto-suggest curriculum and school year based on age
            profile.curriculum = profileManager.suggestedCurriculum(for: profile.age)
            profile.schoolYear = profileManager.suggestedSchoolYear(for: profile.age, curriculum: profile.curriculum)
        }

        // Move to next step
        withAnimation {
            currentStep += 1
        }
    }

    private func completeOnboarding() {
        // Save profile
        profileManager.saveProfile(profile)
        profileManager.completeOnboarding()

        // Complete onboarding
        isComplete = true
    }
}

// MARK: - Progress Bar

private struct OnboardingProgressBar: View {
    let current: Int
    let total: Int

    var body: some View {
        VStack(spacing: 8) {
            HStack(spacing: 8) {
                ForEach(0..<total, id: \.self) { index in
                    Capsule()
                        .fill(index <= current ? AnyShapeStyle(Color.purple.gradient) : AnyShapeStyle(Color.gray.opacity(0.3)))
                        .frame(height: 6)
                        .animation(.spring(response: 0.4, dampingFraction: 0.7), value: current)
                }
            }

            Text("Passo \(current + 1) di \(total)")
                .font(.caption)
                .foregroundStyle(.secondary)
        }
    }
}

// MARK: - Step 1: Welcome + Name

private struct WelcomeNameStep: View {
    @Binding var firstName: String
    @State private var animateWelcome = false
    @FocusState private var isNameFieldFocused: Bool

    var body: some View {
        VStack(spacing: 32) {
            Spacer()

            // Animated welcome icon
            ZStack {
                Circle()
                    .fill(Color.purple.opacity(0.1))
                    .frame(width: 120, height: 120)
                    .scaleEffect(animateWelcome ? 1.1 : 1.0)

                Image(systemName: "sparkles")
                    .font(.system(size: 50))
                    .foregroundStyle(
                        LinearGradient(
                            colors: [.purple, .blue, .green],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        )
                    )
                    .rotationEffect(.degrees(animateWelcome ? 360 : 0))
            }
            .animation(
                .easeInOut(duration: 3).repeatForever(autoreverses: true),
                value: animateWelcome
            )
            .onAppear { animateWelcome = true }

            VStack(spacing: 12) {
                Text("Benvenuto in Convergio Scuola!")
                    .font(.system(size: 34, weight: .bold, design: .rounded))
                    .multilineTextAlignment(.center)

                Text("Il tuo assistente AI personale per studiare meglio")
                    .font(.title3)
                    .foregroundStyle(.secondary)
                    .multilineTextAlignment(.center)
            }

            // Name input
            VStack(alignment: .leading, spacing: 8) {
                Text("Come ti chiami?")
                    .font(.headline)
                    .foregroundStyle(.primary)

                TextField("Il tuo nome", text: $firstName)
                    .textFieldStyle(.roundedBorder)
                    .font(.title2)
                    .multilineTextAlignment(.center)
                    .focused($isNameFieldFocused)
                    .onAppear {
                        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                            isNameFieldFocused = true
                        }
                    }

                Text("Useremo solo il tuo nome per personalizzare l'esperienza")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            .frame(maxWidth: 400)

            Spacer()
        }
        .padding(40)
    }
}

// MARK: - Step 2: Age + School Year

private struct AgeGradeStep: View {
    @Binding var age: Int
    @Binding var schoolYear: SchoolYear
    @Binding var curriculum: Curriculum
    @State private var animateIcon = false

    var body: some View {
        VStack(spacing: 32) {
            Spacer()

            // Age icon
            ZStack {
                Circle()
                    .fill(Color.blue.opacity(0.1))
                    .frame(width: 100, height: 100)
                    .scaleEffect(animateIcon ? 1.05 : 1.0)

                Image(systemName: "calendar.circle.fill")
                    .font(.system(size: 50))
                    .foregroundStyle(.blue.gradient)
            }
            .animation(
                .easeInOut(duration: 2).repeatForever(autoreverses: true),
                value: animateIcon
            )
            .onAppear { animateIcon = true }

            VStack(spacing: 12) {
                Text("Quanti anni hai?")
                    .font(.system(size: 28, weight: .bold, design: .rounded))

                Text("Questo ci aiuta a personalizzare i contenuti per te")
                    .font(.body)
                    .foregroundStyle(.secondary)
                    .multilineTextAlignment(.center)
            }

            // Age picker
            VStack(spacing: 16) {
                HStack(spacing: 12) {
                    Button {
                        if age > 6 {
                            age -= 1
                        }
                    } label: {
                        Image(systemName: "minus.circle.fill")
                            .font(.system(size: 32))
                    }
                    .buttonStyle(.plain)
                    .disabled(age <= 6)

                    Text("\(age)")
                        .font(.system(size: 56, weight: .bold, design: .rounded))
                        .frame(width: 120)
                        .animation(.spring(response: 0.3), value: age)

                    Button {
                        if age < 19 {
                            age += 1
                        }
                    } label: {
                        Image(systemName: "plus.circle.fill")
                            .font(.system(size: 32))
                    }
                    .buttonStyle(.plain)
                    .disabled(age >= 19)
                }

                Text("anni")
                    .font(.title2)
                    .foregroundStyle(.secondary)
            }

            Spacer()
        }
        .padding(40)
    }
}

// MARK: - Step 3: Curriculum Selection

private struct CurriculumSelectionStep: View {
    @Binding var curriculum: Curriculum
    @Binding var schoolYear: SchoolYear

    var body: some View {
        ScrollView {
            VStack(spacing: 32) {
                VStack(spacing: 12) {
                    Text("Che scuola frequenti?")
                        .font(.system(size: 28, weight: .bold, design: .rounded))

                    Text("Seleziona il tuo percorso scolastico")
                        .font(.body)
                        .foregroundStyle(.secondary)
                }
                .padding(.top, 30)

                // Curriculum grid
                LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible()), GridItem(.flexible())], spacing: 16) {
                    ForEach(Curriculum.allCases, id: \.self) { curr in
                        CurriculumCard(
                            curriculum: curr,
                            isSelected: curriculum == curr
                        ) {
                            withAnimation(.spring(response: 0.3)) {
                                curriculum = curr
                                // Auto-select appropriate school year
                                if let firstYear = curr.availableSchoolYears.first {
                                    schoolYear = firstYear
                                }
                            }
                        }
                    }
                }
                .padding(.horizontal, 20)

                // School year selection
                if !curriculum.availableSchoolYears.isEmpty {
                    VStack(spacing: 16) {
                        Divider()
                            .padding(.horizontal, 40)

                        Text("Che classe frequenti?")
                            .font(.headline)

                        LazyVGrid(columns: [GridItem(.adaptive(minimum: 120))], spacing: 12) {
                            ForEach(curriculum.availableSchoolYears, id: \.self) { year in
                                Button {
                                    withAnimation {
                                        schoolYear = year
                                    }
                                } label: {
                                    Text(year.displayName)
                                        .font(.body)
                                        .padding(.horizontal, 20)
                                        .padding(.vertical, 12)
                                        .frame(maxWidth: .infinity)
                                        .background(schoolYear == year ? Color.purple.opacity(0.2) : Color.gray.opacity(0.1))
                                        .clipShape(RoundedRectangle(cornerRadius: 8))
                                        .overlay(
                                            RoundedRectangle(cornerRadius: 8)
                                                .stroke(schoolYear == year ? Color.purple : Color.clear, lineWidth: 2)
                                        )
                                }
                                .buttonStyle(.plain)
                            }
                        }
                        .padding(.horizontal, 40)
                    }
                }

                Spacer(minLength: 20)
            }
        }
    }
}

private struct CurriculumCard: View {
    let curriculum: Curriculum
    let isSelected: Bool
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            VStack(spacing: 12) {
                ZStack {
                    Circle()
                        .fill(Color(curriculum.color).opacity(0.2))
                        .frame(width: 60, height: 60)

                    Image(systemName: curriculum.icon)
                        .font(.title)
                        .foregroundStyle(Color(curriculum.color))
                }

                VStack(spacing: 4) {
                    Text(curriculum.displayName)
                        .font(.headline)
                        .multilineTextAlignment(.center)
                        .lineLimit(2)

                    Text(curriculum.description)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .multilineTextAlignment(.center)
                        .lineLimit(2)
                }
            }
            .padding(16)
            .frame(maxWidth: .infinity)
            .frame(height: 160)
            .background(isSelected ? Color.purple.opacity(0.1) : Color.gray.opacity(0.05))
            .clipShape(RoundedRectangle(cornerRadius: 16))
            .overlay(
                RoundedRectangle(cornerRadius: 16)
                    .stroke(isSelected ? Color.purple : Color.clear, lineWidth: 3)
            )
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Step 4: Accessibility Preferences

private struct AccessibilityStep: View {
    @Binding var settings: StudentAccessibilitySettings

    var body: some View {
        ScrollView {
            VStack(spacing: 32) {
                VStack(spacing: 12) {
                    Image(systemName: "accessibility")
                        .font(.system(size: 50))
                        .foregroundStyle(.green.gradient)

                    Text("Preferenze di Accessibilità")
                        .font(.system(size: 28, weight: .bold, design: .rounded))

                    Text("Personalizza l'esperienza per le tue esigenze")
                        .font(.body)
                        .foregroundStyle(.secondary)
                        .multilineTextAlignment(.center)
                }
                .padding(.top, 30)

                VStack(spacing: 20) {
                    // Font Size
                    VStack(alignment: .leading, spacing: 12) {
                        Label("Dimensione Testo", systemImage: "textformat.size")
                            .font(.headline)

                        HStack(spacing: 12) {
                            ForEach(StudentAccessibilitySettings.StudentFontSize.allCases, id: \.self) { size in
                                Button {
                                    settings.fontSize = size
                                } label: {
                                    Text(size.displayName)
                                        .font(.body)
                                        .padding(.horizontal, 16)
                                        .padding(.vertical, 10)
                                        .background(settings.fontSize == size ? Color.purple.opacity(0.2) : Color.gray.opacity(0.1))
                                        .clipShape(RoundedRectangle(cornerRadius: 8))
                                }
                                .buttonStyle(.plain)
                            }
                        }
                    }

                    Divider()

                    // Accessibility toggles
                    VStack(spacing: 16) {
                        AccessibilityToggle(
                            title: "Alto Contrasto",
                            description: "Aumenta il contrasto dei colori",
                            icon: "circle.righthalf.filled",
                            isOn: $settings.highContrast
                        )

                        AccessibilityToggle(
                            title: "Lettura Vocale",
                            description: "Ali può leggere le risposte ad alta voce",
                            icon: "speaker.wave.3.fill",
                            isOn: $settings.voiceEnabled
                        )

                        AccessibilityToggle(
                            title: "Linguaggio Semplificato",
                            description: "Usa frasi più brevi e semplici",
                            icon: "text.bubble.fill",
                            isOn: $settings.simplifiedLanguage
                        )

                        AccessibilityToggle(
                            title: "Font per Dislessia",
                            description: "Usa un carattere più leggibile",
                            icon: "character.cursor.ibeam",
                            isOn: $settings.dyslexiaFont
                        )
                    }
                }
                .padding(.horizontal, 40)

                Text("Puoi modificare queste impostazioni in qualsiasi momento")
                    .font(.caption)
                    .foregroundStyle(.tertiary)
                    .padding(.bottom, 20)
            }
        }
    }
}

private struct AccessibilityToggle: View {
    let title: String
    let description: String
    let icon: String
    @Binding var isOn: Bool

    var body: some View {
        HStack(spacing: 16) {
            Image(systemName: icon)
                .font(.title2)
                .foregroundStyle(.purple)
                .frame(width: 40)

            VStack(alignment: .leading, spacing: 4) {
                Text(title)
                    .font(.headline)

                Text(description)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            Toggle("", isOn: $isOn)
                .labelsHidden()
        }
        .padding(16)
        .background(Color.gray.opacity(0.05))
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

// MARK: - Preview

#Preview("Education Onboarding") {
    EducationOnboardingFlow(isComplete: .constant(false))
}
