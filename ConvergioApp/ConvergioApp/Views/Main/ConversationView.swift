/**
 * CONVERGIO NATIVE - Conversation View
 *
 * Main chat interface with message history and input field.
 * Supports streaming responses and rich markdown content.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

struct ConversationView: View {
    @EnvironmentObject var conversationVM: ConversationViewModel
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel

    @State private var inputText = ""
    @State private var scrollProxy: ScrollViewProxy?
    @FocusState private var isInputFocused: Bool

    var body: some View {
        VStack(spacing: 0) {
            // Message list
            ScrollViewReader { proxy in
                ScrollView {
                    VStack(alignment: .leading, spacing: 16) {
                        ForEach(conversationVM.messages) { message in
                            MessageBubble(message: message)
                                .id(message.id)
                                .transition(.asymmetric(
                                    insertion: .scale(scale: 0.95).combined(with: .opacity),
                                    removal: .opacity
                                ))
                        }

                        // Streaming indicator
                        if conversationVM.isStreaming {
                            StreamingIndicator(text: conversationVM.streamingText)
                                .id("streaming")
                                .transition(.opacity)
                        }

                        // Invisible spacer to ensure scroll works
                        Color.clear
                            .frame(height: 1)
                            .id("bottom")
                    }
                    .padding()
                    .animation(.easeInOut(duration: 0.2), value: conversationVM.messages.count)
                }
                .onAppear {
                    scrollProxy = proxy
                }
                .onChange(of: conversationVM.messages.count) { oldCount, newCount in
                    // Scroll immediately when new message added
                    DispatchQueue.main.async {
                        withAnimation(.easeOut(duration: 0.15)) {
                            proxy.scrollTo("bottom", anchor: .bottom)
                        }
                    }
                }
                .onChange(of: conversationVM.streamingText) { _, newText in
                    // Scroll during streaming (throttled)
                    if newText.count % 20 == 0 || newText.hasSuffix("\n") {
                        DispatchQueue.main.async {
                            proxy.scrollTo("streaming", anchor: .bottom)
                        }
                    }
                }
                .onChange(of: conversationVM.isStreaming) { _, isStreaming in
                    if isStreaming {
                        DispatchQueue.main.async {
                            proxy.scrollTo("streaming", anchor: .bottom)
                        }
                    }
                }
            }

            Divider()

            // Input area - compact design
            HStack(alignment: .center, spacing: 10) {
                // Text input - single line by default, expands as needed
                TextField("Ask the team...", text: $inputText, axis: .vertical)
                    .textFieldStyle(.plain)
                    .font(.body)
                    .lineLimit(1...4)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .background(Color(nsColor: .textBackgroundColor))
                    .clipShape(RoundedRectangle(cornerRadius: 8))
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(Color.secondary.opacity(0.2), lineWidth: 1)
                    )
                    .focused($isInputFocused)
                    .disabled(conversationVM.isProcessing)
                    .onAppear {
                        isInputFocused = true
                    }
                    .onSubmit {
                        if !inputText.isEmpty && !conversationVM.isProcessing {
                            sendMessage()
                        }
                    }

                // Send/Stop button
                Button {
                    if conversationVM.isProcessing {
                        conversationVM.cancel()
                    } else {
                        sendMessage()
                    }
                } label: {
                    Image(systemName: conversationVM.isProcessing ? "stop.circle.fill" : "arrow.up.circle.fill")
                        .font(.system(size: 24))
                        .foregroundStyle(conversationVM.isProcessing ? .red : (inputText.isEmpty ? .secondary : .accentColor))
                        .contentTransition(.symbolEffect(.replace))
                }
                .buttonStyle(.plain)
                .disabled(inputText.isEmpty && !conversationVM.isProcessing)
                .keyboardShortcut(.return, modifiers: [.command])
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(.ultraThinMaterial)

            // Status bar
            StatusBarView()
        }
        .navigationTitle("Conversation")
        .toolbar {
            ToolbarItemGroup(placement: .automatic) {
                Button {
                    conversationVM.newConversation()
                } label: {
                    Image(systemName: "plus.circle")
                }
                .help("New Conversation")
                .keyboardShortcut("n", modifiers: [.command])

                if conversationVM.isProcessing {
                    ProgressView()
                        .scaleEffect(0.7)
                        .frame(width: 20, height: 20)

                    Button {
                        conversationVM.cancel()
                    } label: {
                        Image(systemName: "xmark.circle")
                            .foregroundStyle(.red)
                    }
                    .help("Cancel (Cmd+.)")
                    .keyboardShortcut(".", modifiers: [.command])
                }
            }
        }
    }

    private func sendMessage() {
        guard !inputText.isEmpty else { return }
        guard !conversationVM.isProcessing else { return }

        let message = inputText
        inputText = ""

        // Keep focus on input
        isInputFocused = true

        Task {
            await conversationVM.send(message)
        }
    }
}

// MARK: - Message Bubble

struct MessageBubble: View {
    let message: Message

    var body: some View {
        HStack(alignment: .top, spacing: 12) {
            // Avatar
            if !message.isFromUser {
                Image(systemName: avatarIcon)
                    .font(.title2)
                    .foregroundStyle(.white)
                    .frame(width: 36, height: 36)
                    .background(avatarColor)
                    .clipShape(Circle())
            }

            // Content
            VStack(alignment: message.isFromUser ? .trailing : .leading, spacing: 4) {
                // Header
                HStack {
                    if !message.isFromUser {
                        Text(senderName)
                            .font(.caption)
                            .fontWeight(.semibold)
                    }

                    Text(message.formattedTime)
                        .font(.caption2)
                        .foregroundStyle(.secondary)
                }

                // Message content
                Text(message.content)
                    .textSelection(.enabled)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
                    .background(bubbleBackground)
                    .clipShape(RoundedRectangle(cornerRadius: 12))
            }

            // User avatar on right
            if message.isFromUser {
                Image(systemName: "person.circle.fill")
                    .font(.title2)
                    .foregroundStyle(.secondary)
                    .frame(width: 36, height: 36)
            }
        }
        .frame(maxWidth: .infinity, alignment: message.isFromUser ? .trailing : .leading)
    }

    private var senderName: String {
        switch message.type {
        case .convergence:
            return "Team Convergence"
        case .agentResponse:
            return "AI Team"
        default:
            return "Agent"
        }
    }

    private var avatarIcon: String {
        switch message.type {
        case .convergence:
            return "arrow.triangle.merge"
        case .error:
            return "exclamationmark.triangle"
        default:
            return "brain.head.profile"
        }
    }

    private var avatarColor: Color {
        switch message.type {
        case .convergence:
            return .purple
        case .error:
            return .red
        default:
            return .blue
        }
    }

    @ViewBuilder
    private var bubbleBackground: some View {
        if message.isFromUser {
            Color.accentColor.opacity(0.15)
        } else if message.type == .convergence {
            LinearGradient(
                colors: [.purple.opacity(0.1), .blue.opacity(0.1)],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
        } else if message.type == .error {
            Color.red.opacity(0.1)
        } else {
            Color.secondary.opacity(0.1)
        }
    }
}

// MARK: - Streaming Indicator

struct StreamingIndicator: View {
    let text: String

    @State private var dotCount = 0
    private let timer = Timer.publish(every: 0.4, on: .main, in: .common).autoconnect()

    var body: some View {
        HStack(alignment: .top, spacing: 12) {
            // Avatar
            Image(systemName: "brain.head.profile")
                .font(.title2)
                .foregroundStyle(.white)
                .frame(width: 36, height: 36)
                .background(.blue)
                .clipShape(Circle())

            VStack(alignment: .leading, spacing: 4) {
                HStack {
                    Text("Thinking")
                        .font(.caption)
                        .fontWeight(.semibold)

                    Text(String(repeating: ".", count: dotCount + 1))
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }

                if !text.isEmpty {
                    Text(text)
                        .textSelection(.enabled)
                        .padding(.horizontal, 12)
                        .padding(.vertical, 8)
                        .background(Color.secondary.opacity(0.1))
                        .clipShape(RoundedRectangle(cornerRadius: 12))
                } else {
                    TypingIndicator()
                }
            }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .onReceive(timer) { _ in
            dotCount = (dotCount + 1) % 3
        }
    }
}

// MARK: - Typing Indicator

struct TypingIndicator: View {
    @State private var animating = false

    var body: some View {
        HStack(spacing: 4) {
            ForEach(0..<3, id: \.self) { index in
                Circle()
                    .fill(Color.secondary)
                    .frame(width: 8, height: 8)
                    .scaleEffect(animating ? 1.0 : 0.5)
                    .animation(
                        .easeInOut(duration: 0.6)
                        .repeatForever()
                        .delay(Double(index) * 0.2),
                        value: animating
                    )
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(Color.secondary.opacity(0.1))
        .clipShape(RoundedRectangle(cornerRadius: 12))
        .onAppear {
            animating = true
        }
    }
}

// MARK: - Status Bar View

struct StatusBarView: View {
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel
    @EnvironmentObject var conversationVM: ConversationViewModel

    var body: some View {
        HStack(spacing: 16) {
            // Model indicator
            HStack(spacing: 4) {
                Image(systemName: "cpu")
                    .font(.caption2)
                Text(orchestratorVM.currentModel)
                    .font(.caption2)
            }
            .foregroundStyle(.secondary)

            Divider()
                .frame(height: 12)

            // Token count
            HStack(spacing: 4) {
                Image(systemName: "number")
                    .font(.caption2)
                Text("\(orchestratorVM.costInfo.sessionUsage.totalTokens) tokens")
                    .font(.caption2)
            }
            .foregroundStyle(.secondary)

            Divider()
                .frame(height: 12)

            // Cost indicator
            HStack(spacing: 4) {
                Image(systemName: "dollarsign.circle")
                    .font(.caption2)
                Text(String(format: "$%.2f / $%.0f", orchestratorVM.costInfo.sessionCost, orchestratorVM.costInfo.budgetLimit))
                    .font(.caption2)
            }
            .foregroundStyle(orchestratorVM.costInfo.isOverBudget ? .red : .secondary)

            Spacer()

            // Message count
            HStack(spacing: 4) {
                Image(systemName: "bubble.left.and.bubble.right")
                    .font(.caption2)
                Text("\(conversationVM.messages.count) messages")
                    .font(.caption2)
            }
            .foregroundStyle(.secondary)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 4)
        .background(Color(nsColor: .windowBackgroundColor))
    }
}

// MARK: - Previews

#Preview {
    ConversationView()
        .environmentObject(ConversationViewModel.preview)
        .environmentObject(OrchestratorViewModel.preview)
}
