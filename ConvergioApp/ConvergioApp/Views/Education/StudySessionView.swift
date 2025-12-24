/**
 * CONVERGIO NATIVE - Study Session View
 *
 * Active study session interface with AI maestro.
 * Features topic input, conversation area, progress tracking, and ADHD-friendly timer.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

struct StudySessionView: View {
    let maestro: Maestro
    @Environment(\.dismiss) var dismiss

    @State private var topic = ""
    @State private var messages: [StudyMessage] = []
    @State private var currentMessage = ""
    @State private var sessionStartTime: Date?
    @State private var elapsedTime: TimeInterval = 0
    @State private var sessionProgress: Double = 0.0
    @State private var isSessionActive = false
    @State private var timer: Timer?
    @State private var isLoading = false
    @State private var showVoiceSession = false

    // AI Provider for real responses
    private let aiProvider = AzureOpenAIProvider.shared

    // ADHD mode settings
    private let sessionDuration: TimeInterval = 15 * 60 // 15 minutes
    private let adhdMode = true

    var body: some View {
        VStack(spacing: 0) {
            // Header with maestro info and timer
            headerSection
                .padding()
                .background(Color(nsColor: .windowBackgroundColor))

            Divider()

            // Main content area
            HStack(spacing: 0) {
                // Conversation area
                conversationSection
                    .frame(maxWidth: .infinity)

                Divider()

                // Sidebar with topic and progress
                sidebarSection
                    .frame(width: 280)
            }
        }
        .frame(width: 1000, height: 700)
        .onAppear {
            setupWelcomeMessage()
        }
        .onDisappear {
            stopSession()
        }
        .sheet(isPresented: $showVoiceSession) {
            VoiceSessionView(maestro: maestro)
        }
    }

    // MARK: - Header Section

    private var headerSection: some View {
        HStack {
            // Maestro info
            HStack(spacing: 12) {
                ZStack {
                    Circle()
                        .fill(maestro.color.opacity(0.2))
                        .frame(width: 50, height: 50)

                    Image(systemName: maestro.icon)
                        .font(.title3)
                        .foregroundStyle(maestro.color)
                }

                VStack(alignment: .leading, spacing: 2) {
                    Text(maestro.name)
                        .font(.headline)
                    Text(maestro.subject.rawValue)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }

            Spacer()

            // Timer and progress (ADHD mode)
            if adhdMode && isSessionActive {
                timerSection
            }

            // Close button
            Button {
                dismiss()
            } label: {
                Image(systemName: "xmark.circle.fill")
                    .font(.title2)
                    .foregroundStyle(.secondary)
            }
            .buttonStyle(.plain)
        }
    }

    private var timerSection: some View {
        HStack(spacing: 16) {
            // Circular progress
            ZStack {
                Circle()
                    .stroke(Color.primary.opacity(0.1), lineWidth: 3)
                    .frame(width: 50, height: 50)

                Circle()
                    .trim(from: 0, to: sessionProgress)
                    .stroke(
                        AngularGradient(
                            colors: [maestro.color, maestro.color.opacity(0.6)],
                            center: .center,
                            startAngle: .degrees(0),
                            endAngle: .degrees(360)
                        ),
                        style: StrokeStyle(lineWidth: 3, lineCap: .round)
                    )
                    .frame(width: 50, height: 50)
                    .rotationEffect(.degrees(-90))

                VStack(spacing: 0) {
                    Text(timeRemaining)
                        .font(.caption.weight(.semibold))
                        .monospacedDigit()
                    Text("min")
                        .font(.caption2)
                        .foregroundStyle(.secondary)
                }
            }
            .animation(.linear(duration: 1), value: sessionProgress)

            VStack(alignment: .leading, spacing: 2) {
                Text("Sessione ADHD")
                    .font(.caption.weight(.medium))
                Text("15 minuti")
                    .font(.caption2)
                    .foregroundStyle(.secondary)
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(
            ZStack {
                VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)
                maestro.color.opacity(0.1)
            }
        )
        .clipShape(RoundedRectangle(cornerRadius: 12))
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(maestro.color.opacity(0.2), lineWidth: 1)
        )
    }

    private var timeRemaining: String {
        let remaining = max(0, sessionDuration - elapsedTime)
        let minutes = Int(remaining) / 60
        return "\(minutes)"
    }

    // MARK: - Conversation Section

    private var conversationSection: some View {
        VStack(spacing: 0) {
            // Messages area
            ScrollViewReader { proxy in
                ScrollView {
                    LazyVStack(spacing: 16) {
                        ForEach(messages) { message in
                            StudyMessageBubble(message: message, maestroColor: maestro.color)
                                .id(message.id)
                        }
                    }
                    .padding()
                }
                .onChange(of: messages.count) { _, _ in
                    if let lastMessage = messages.last {
                        withAnimation {
                            proxy.scrollTo(lastMessage.id, anchor: .bottom)
                        }
                    }
                }
            }

            Divider()

            // Input area
            messageInputSection
        }
    }

    private var messageInputSection: some View {
        HStack(spacing: 12) {
            // Voice button - prominent microphone
            Button {
                showVoiceSession = true
            } label: {
                Image(systemName: "mic.circle.fill")
                    .font(.system(size: 38))
                    .foregroundStyle(maestro.color)
                    .symbolRenderingMode(.hierarchical)
            }
            .buttonStyle(.plain)
            .help("Parla con \(maestro.name)")

            // Text input
            TextField("Scrivi la tua domanda o risposta...", text: $currentMessage)
                .textFieldStyle(.plain)
                .padding(.horizontal, 12)
                .padding(.vertical, 10)
                .background(Color.primary.opacity(0.05))
                .clipShape(RoundedRectangle(cornerRadius: 10))
                .onSubmit {
                    sendMessage()
                }
                .disabled(isLoading)

            // Loading indicator or send button
            if isLoading {
                ProgressView()
                    .scaleEffect(0.8)
                    .frame(width: 32, height: 32)
            } else {
                // Send button
                Button {
                    sendMessage()
                } label: {
                    Image(systemName: "arrow.up.circle.fill")
                        .font(.title2)
                        .foregroundStyle(currentMessage.isEmpty ? .secondary : maestro.color)
                }
                .buttonStyle(.plain)
                .disabled(currentMessage.isEmpty)
            }
        }
        .padding()
        .background(Color(nsColor: .windowBackgroundColor))
    }

    // MARK: - Sidebar Section

    private var sidebarSection: some View {
        ScrollView {
            VStack(spacing: 20) {
                // Topic section
                VStack(alignment: .leading, spacing: 12) {
                    Label("Argomento", systemImage: "book")
                        .font(.headline)
                        .foregroundStyle(maestro.color)

                    if !isSessionActive {
                        TextField("Es: Teorema di Pitagora", text: $topic)
                            .textFieldStyle(.plain)
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
                                    .stroke(Color.primary.opacity(0.1), lineWidth: 1)
                            )

                        Button {
                            startSession()
                        } label: {
                            HStack {
                                Image(systemName: "play.fill")
                                Text("Inizia Studio")
                            }
                            .frame(maxWidth: .infinity)
                            .padding(.vertical, 10)
                            .background(maestro.color)
                            .foregroundStyle(.white)
                            .clipShape(RoundedRectangle(cornerRadius: 10))
                        }
                        .buttonStyle(.plain)
                        .disabled(topic.isEmpty)
                    } else {
                        Text(topic)
                            .font(.body.weight(.medium))
                            .padding(12)
                            .frame(maxWidth: .infinity, alignment: .leading)
                            .background(maestro.color.opacity(0.1))
                            .clipShape(RoundedRectangle(cornerRadius: 10))
                    }
                }

                if isSessionActive {
                    Divider()

                    // Session stats
                    sessionStatsSection

                    Divider()

                    // Quick actions
                    quickActionsSection

                    Spacer()

                    // End session button
                    Button {
                        endSession()
                    } label: {
                        HStack {
                            Image(systemName: "stop.fill")
                            Text("Termina Sessione")
                        }
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 10)
                        .background(Color.red.opacity(0.1))
                        .foregroundStyle(.red)
                        .clipShape(RoundedRectangle(cornerRadius: 10))
                        .overlay(
                            RoundedRectangle(cornerRadius: 10)
                                .stroke(Color.red.opacity(0.3), lineWidth: 1)
                        )
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding()
        }
        .background(Color(nsColor: .windowBackgroundColor).opacity(0.5))
    }

    private var sessionStatsSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Label("Progresso", systemImage: "chart.line.uptrend.xyaxis")
                .font(.headline)
                .foregroundStyle(maestro.color)

            // Progress bar
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("Completamento")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    Spacer()
                    Text("\(Int(sessionProgress * 100))%")
                        .font(.caption.weight(.semibold))
                        .foregroundStyle(maestro.color)
                }

                GeometryReader { geometry in
                    ZStack(alignment: .leading) {
                        Capsule()
                            .fill(Color.primary.opacity(0.1))

                        Capsule()
                            .fill(
                                LinearGradient(
                                    colors: [maestro.color, maestro.color.opacity(0.7)],
                                    startPoint: .leading,
                                    endPoint: .trailing
                                )
                            )
                            .frame(width: geometry.size.width * sessionProgress)
                    }
                }
                .frame(height: 8)
            }

            // Stats
            VStack(spacing: 8) {
                statRow(icon: "message", label: "Messaggi", value: "\(messages.count)")
                statRow(icon: "clock", label: "Tempo", value: formattedElapsedTime)
            }
        }
    }

    private func statRow(icon: String, label: String, value: String) -> some View {
        HStack {
            Image(systemName: icon)
                .font(.caption)
                .foregroundStyle(maestro.color)
                .frame(width: 20)

            Text(label)
                .font(.caption)
                .foregroundStyle(.secondary)

            Spacer()

            Text(value)
                .font(.caption.weight(.medium))
        }
    }

    private var formattedElapsedTime: String {
        let minutes = Int(elapsedTime) / 60
        let seconds = Int(elapsedTime) % 60
        return String(format: "%d:%02d", minutes, seconds)
    }

    private var quickActionsSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Label("Azioni Rapide", systemImage: "bolt")
                .font(.headline)
                .foregroundStyle(maestro.color)

            VStack(spacing: 8) {
                quickActionButton(icon: "lightbulb", text: "Spiegazione", action: requestExplanation)
                quickActionButton(icon: "questionmark.circle", text: "Esempio", action: requestExample)
                quickActionButton(icon: "list.bullet", text: "Riassunto", action: requestSummary)
            }
        }
    }

    private func quickActionButton(icon: String, text: String, action: @escaping () -> Void) -> some View {
        Button {
            action()
        } label: {
            HStack(spacing: 8) {
                Image(systemName: icon)
                    .font(.caption)
                    .frame(width: 20)
                Text(text)
                    .font(.caption.weight(.medium))
                Spacer()
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(
                ZStack {
                    VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)
                    maestro.color.opacity(0.1)
                }
            )
            .foregroundStyle(maestro.color)
            .clipShape(RoundedRectangle(cornerRadius: 8))
        }
        .buttonStyle(.plain)
    }

    // MARK: - Session Management

    private func setupWelcomeMessage() {
        let welcomeMessage = StudyMessage(
            content: "Ciao! Sono \(maestro.name), il tuo maestro di \(maestro.subject.rawValue). \(maestro.specialization). Sono qui per aiutarti a studiare. Su quale argomento vuoi lavorare oggi?",
            isFromMaestro: true,
            timestamp: Date()
        )
        messages.append(welcomeMessage)
    }

    private func startSession() {
        guard !topic.isEmpty else { return }

        isSessionActive = true
        sessionStartTime = Date()

        // Start timer for ADHD mode
        if adhdMode {
            timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { _ in
                updateSessionProgress()
            }
        }

        // Maestro response
        let response = StudyMessage(
            content: "Perfetto! Iniziamo con \"\(topic)\". \(maestro.teachingStyle.split(separator: ".").first ?? "Sono qui per guidarti.") Fammi una domanda o dimmi cosa vorresti capire meglio.",
            isFromMaestro: true,
            timestamp: Date()
        )
        messages.append(response)
    }

    private func updateSessionProgress() {
        guard let startTime = sessionStartTime else { return }
        elapsedTime = Date().timeIntervalSince(startTime)
        sessionProgress = min(1.0, elapsedTime / sessionDuration)

        // Check if session should end
        if sessionProgress >= 1.0 {
            notifySessionEnd()
        }
    }

    private func notifySessionEnd() {
        let notification = StudyMessage(
            content: "⏰ La sessione di 15 minuti è terminata! Ottimo lavoro! Ricorda di fare una pausa prima di continuare. Vuoi terminare qui o continuare ancora un po'?",
            isFromMaestro: true,
            timestamp: Date()
        )
        messages.append(notification)
        timer?.invalidate()
    }

    private func endSession() {
        let farewell = StudyMessage(
            content: "È stato un piacere studiare con te! Ricorda: \(["la pratica costante è la chiave", "ogni piccolo progresso conta", "non smettere mai di fare domande", "la curiosità è il motore dell'apprendimento"].randomElement()!). A presto! 👋",
            isFromMaestro: true,
            timestamp: Date()
        )
        messages.append(farewell)

        DispatchQueue.main.asyncAfter(deadline: .now() + 1.5) {
            dismiss()
        }
    }

    private func stopSession() {
        timer?.invalidate()
        timer = nil
    }

    // MARK: - Message Handling

    private func sendMessage() {
        guard !currentMessage.isEmpty else { return }

        // Add user message
        let userMessage = StudyMessage(
            content: currentMessage,
            isFromMaestro: false,
            timestamp: Date()
        )
        messages.append(userMessage)

        // Clear input and start loading
        let sentMessage = currentMessage
        currentMessage = ""
        isLoading = true

        // Call real AI provider
        Task {
            await getMaestroResponse(to: sentMessage)
        }
    }

    private func getMaestroResponse(to userMessage: String) async {
        // Build conversation context
        let systemPrompt = """
        Sei \(maestro.name), un maestro esperto di \(maestro.subject.rawValue).
        \(maestro.description)

        Stile di insegnamento: \(maestro.teachingStyle)

        Argomento della sessione: \(topic.isEmpty ? "generale" : topic)

        Rispondi in italiano. Sii paziente, chiaro e incoraggiante.
        Usa il metodo maieutico quando appropriato.
        """

        // Build conversation history for context
        var conversationMessages = [ChatMessage(role: "system", content: systemPrompt)]

        // Add recent conversation history
        for msg in messages.suffix(10) {
            conversationMessages.append(ChatMessage(
                role: msg.isFromMaestro ? "assistant" : "user",
                content: msg.content
            ))
        }

        // Add current user message
        conversationMessages.append(ChatMessage(role: "user", content: userMessage))

        do {
            let response = try await aiProvider.complete(
                messages: conversationMessages,
                temperature: 0.8,
                maxTokens: 1024
            )

            await MainActor.run {
                isLoading = false
                let maestroMessage = StudyMessage(
                    content: response,
                    isFromMaestro: true,
                    timestamp: Date()
                )
                messages.append(maestroMessage)
            }
        } catch {
            await MainActor.run {
                isLoading = false
                let errorMessage = StudyMessage(
                    content: "Mi scuso, ho avuto un problema tecnico. Potresti ripetere la tua domanda?",
                    isFromMaestro: true,
                    timestamp: Date()
                )
                messages.append(errorMessage)
                logError("AI response error: \(error.localizedDescription)")
            }
        }
    }

    private func requestExplanation() {
        currentMessage = "Puoi spiegarmi meglio questo concetto?"
        sendMessage()
    }

    private func requestExample() {
        currentMessage = "Potresti farmi un esempio pratico?"
        sendMessage()
    }

    private func requestSummary() {
        currentMessage = "Puoi fare un riassunto di quello che abbiamo visto?"
        sendMessage()
    }
}

// MARK: - Study Message Model

struct StudyMessage: Identifiable {
    let id = UUID()
    let content: String
    let isFromMaestro: Bool
    let timestamp: Date
}

// MARK: - Study Message Bubble

private struct StudyMessageBubble: View {
    let message: StudyMessage
    let maestroColor: Color

    var body: some View {
        HStack(alignment: .top, spacing: 12) {
            if message.isFromMaestro {
                // Maestro avatar
                Image(systemName: "person.circle.fill")
                    .font(.title2)
                    .foregroundStyle(maestroColor)

                // Message content
                VStack(alignment: .leading, spacing: 4) {
                    Text(message.content)
                        .font(.body)
                        .padding(12)
                        .background(
                            ZStack {
                                VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)
                                maestroColor.opacity(0.15)
                            }
                        )
                        .clipShape(RoundedRectangle(cornerRadius: 12))
                        .overlay(
                            RoundedRectangle(cornerRadius: 12)
                                .stroke(maestroColor.opacity(0.2), lineWidth: 0.5)
                        )

                    Text(message.timestamp, style: .time)
                        .font(.caption2)
                        .foregroundStyle(.secondary)
                        .padding(.leading, 4)
                }

                Spacer()
            } else {
                Spacer()

                // User message
                VStack(alignment: .trailing, spacing: 4) {
                    Text(message.content)
                        .font(.body)
                        .padding(12)
                        .background(Color.accentColor)
                        .foregroundStyle(.white)
                        .clipShape(RoundedRectangle(cornerRadius: 12))

                    Text(message.timestamp, style: .time)
                        .font(.caption2)
                        .foregroundStyle(.secondary)
                        .padding(.trailing, 4)
                }
            }
        }
    }
}

// MARK: - Preview

#Preview {
    StudySessionView(maestro: Maestro.preview)
}
