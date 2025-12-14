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
    @FocusState private var isInputFocused: Bool

    var body: some View {
        VStack(spacing: 0) {
            // Message list
            ScrollViewReader { proxy in
                ScrollView {
                    LazyVStack(alignment: .leading, spacing: 16) {
                        ForEach(conversationVM.messages) { message in
                            MessageBubble(message: message)
                                .id(message.id)
                        }

                        // Streaming indicator
                        if conversationVM.isStreaming {
                            StreamingIndicator(text: conversationVM.streamingText)
                                .id("streaming")
                        }
                    }
                    .padding()
                }
                .onChange(of: conversationVM.messages.count) { _, _ in
                    withAnimation(.easeOut(duration: 0.2)) {
                        proxy.scrollTo(conversationVM.messages.last?.id, anchor: .bottom)
                    }
                }
                .onChange(of: conversationVM.streamingText) { _, _ in
                    withAnimation(.easeOut(duration: 0.1)) {
                        proxy.scrollTo("streaming", anchor: .bottom)
                    }
                }
            }

            Divider()

            // Input area
            HStack(alignment: .bottom, spacing: 12) {
                // Text input
                TextField("Ask the team...", text: $inputText, axis: .vertical)
                    .textFieldStyle(.plain)
                    .lineLimit(1...10)
                    .focused($isInputFocused)
                    .onSubmit {
                        if !inputText.isEmpty {
                            sendMessage()
                        }
                    }
                    .disabled(conversationVM.isProcessing)

                // Send button
                Button {
                    sendMessage()
                } label: {
                    Image(systemName: conversationVM.isProcessing ? "stop.fill" : "arrow.up.circle.fill")
                        .font(.title2)
                        .foregroundStyle(inputText.isEmpty && !conversationVM.isProcessing ? .secondary : .primary)
                }
                .buttonStyle(.plain)
                .disabled(inputText.isEmpty && !conversationVM.isProcessing)
                .keyboardShortcut(.return, modifiers: [.command])
            }
            .padding()
            .background(.ultraThinMaterial)
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

                if conversationVM.isProcessing {
                    Button {
                        conversationVM.cancel()
                    } label: {
                        Image(systemName: "stop.circle")
                    }
                    .help("Cancel")
                }
            }
        }
    }

    private func sendMessage() {
        guard !inputText.isEmpty else { return }

        if conversationVM.isProcessing {
            conversationVM.cancel()
        } else {
            let message = inputText
            inputText = ""
            Task {
                await conversationVM.send(message)
            }
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

// MARK: - Previews

#Preview {
    ConversationView()
        .environmentObject(ConversationViewModel.preview)
        .environmentObject(OrchestratorViewModel.preview)
}
