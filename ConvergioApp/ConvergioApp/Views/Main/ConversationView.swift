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
import UniformTypeIdentifiers
import AppKit

struct ConversationView: View {
    @EnvironmentObject var conversationVM: ConversationViewModel
    @EnvironmentObject var orchestratorVM: OrchestratorViewModel

    @State private var inputText = ""
    @State private var scrollProxy: ScrollViewProxy?
    @FocusState private var isInputFocused: Bool

    var body: some View {
        VStack(spacing: 0) {
            // Message list or Empty State
            if conversationVM.messages.isEmpty && !conversationVM.isStreaming {
                EmptyConversationView()
            } else {
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
                    .onChange(of: conversationVM.isStreaming) { wasStreaming, isStreaming in
                        if isStreaming {
                            DispatchQueue.main.async {
                                proxy.scrollTo("streaming", anchor: .bottom)
                            }
                        } else if wasStreaming && !isStreaming {
                            // Response completed - play sound
                            AppDelegate.playCompletionSound()
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
                    .font(DesignSystem.Typography.body)
                    .lineLimit(1...4)
                    .padding(.horizontal, DesignSystem.Spacing.md)
                    .padding(.vertical, DesignSystem.Spacing.sm + 2)
                    .background(DesignSystem.Colors.surface)
                    .clipShape(RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.medium))
                    .overlay(
                        RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.medium)
                            .stroke(isInputFocused ? DesignSystem.Colors.primary.opacity(0.5) : DesignSystem.Colors.textSecondary.opacity(0.2), lineWidth: isInputFocused ? 2 : 1)
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
                    .animation(DesignSystem.Animation.quick, value: isInputFocused)

                // Send/Stop button
                Button {
                    if conversationVM.isProcessing {
                        conversationVM.cancel()
                    } else {
                        sendMessage()
                    }
                } label: {
                    Image(systemName: conversationVM.isProcessing ? "stop.circle.fill" : "arrow.up.circle.fill")
                        .font(.system(size: 26, weight: .medium))
                        .foregroundStyle(conversationVM.isProcessing ? DesignSystem.Colors.error : (inputText.isEmpty ? DesignSystem.Colors.textSecondary : DesignSystem.Colors.primary))
                        .contentTransition(.symbolEffect(.replace))
                        .symbolEffect(.bounce, value: inputText.isEmpty == false && !conversationVM.isProcessing)
                }
                .buttonStyle(.plain)
                .disabled(inputText.isEmpty && !conversationVM.isProcessing)
                .keyboardShortcut(.return, modifiers: [.command])
                .help(conversationVM.isProcessing ? "Stop (Cmd+.)" : "Send (Cmd+Return)")
            }
            .padding(.horizontal, DesignSystem.Spacing.md)
            .padding(.vertical, DesignSystem.Spacing.sm)
            .background(.ultraThinMaterial)

            // Status bar
            StatusBarView()
        }
        .navigationTitle(dynamicTitle)
        .toolbar {
            ToolbarItemGroup(placement: .automatic) {
                Button {
                    conversationVM.newConversation()
                } label: {
                    Image(systemName: "plus.circle")
                }
                .help("New Conversation")
                .keyboardShortcut("n", modifiers: [.command])

                // Export chat
                Button {
                    exportChat()
                } label: {
                    Image(systemName: "square.and.arrow.up")
                }
                .help("Export Chat")
                .disabled(conversationVM.messages.isEmpty)

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
        .onReceive(NotificationCenter.default.publisher(for: .newConversation)) { _ in
            conversationVM.newConversation()
        }
        .onReceive(NotificationCenter.default.publisher(for: .clearHistory)) { _ in
            conversationVM.clearHistory()
        }
    }

    // MARK: - Dynamic Title

    private var dynamicTitle: String {
        if conversationVM.messages.isEmpty {
            return "Convergio"
        } else {
            return "Convergio - \(conversationVM.messages.count) messages"
        }
    }

    // MARK: - Export Chat

    private func exportChat() {
        let panel = NSSavePanel()
        panel.allowedContentTypes = [.plainText, .text]
        panel.nameFieldStringValue = "conversation_\(Date().ISO8601Format()).txt"
        panel.title = "Export Conversation"

        panel.begin { response in
            if response == .OK, let url = panel.url {
                let content = conversationVM.messages.map { message in
                    let sender = message.isFromUser ? "You" : "AI Team"
                    return "[\(message.formattedTime)] \(sender):\n\(message.content)\n"
                }.joined(separator: "\n---\n\n")

                do {
                    try content.write(to: url, atomically: true, encoding: .utf8)
                    logInfo("Exported conversation to \(url.path)", category: "Export")
                } catch {
                    logError("Failed to export: \(error)", category: "Export")
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

// MARK: - Empty Conversation View

struct EmptyConversationView: View {
    @State private var animateLogo = false

    var body: some View {
        VStack(spacing: 24) {
            Spacer()

            // Animated logo
            ZStack {
                Circle()
                    .fill(
                        LinearGradient(
                            colors: [DesignSystem.Colors.primary.opacity(0.2), DesignSystem.Colors.info.opacity(0.1)],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        )
                    )
                    .frame(width: 120, height: 120)
                    .scaleEffect(animateLogo ? 1.05 : 0.95)
                    .shadow(
                        color: DesignSystem.Colors.primary.opacity(0.3),
                        radius: 20,
                        x: 0,
                        y: 10
                    )

                Image(systemName: "brain.head.profile")
                    .font(.system(size: 50, weight: .medium))
                    .foregroundStyle(
                        LinearGradient(
                            colors: [DesignSystem.Colors.primary, DesignSystem.Colors.info],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        )
                    )
            }
            .animation(DesignSystem.Animation.gentle.repeatForever(autoreverses: true), value: animateLogo)
            .onAppear { animateLogo = true }

            Text("Welcome to Convergio")
                .font(DesignSystem.Typography.title.bold())
                .foregroundStyle(
                    LinearGradient(
                        colors: [DesignSystem.Colors.primary, DesignSystem.Colors.info],
                        startPoint: .leading,
                        endPoint: .trailing
                    )
                )

            Text("Your AI Executive Team is ready to help")
                .font(DesignSystem.Typography.body)
                .foregroundStyle(DesignSystem.Colors.textSecondary)
                .multilineTextAlignment(.center)

            // Quick suggestions
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                Text("Try asking:")
                    .font(DesignSystem.Typography.caption)
                    .foregroundStyle(DesignSystem.Colors.textTertiary)
                    .padding(.horizontal, DesignSystem.Spacing.xs)

                ForEach([
                    "Help me plan a project",
                    "Analyze this problem for me",
                    "Write a compelling email"
                ], id: \.self) { suggestion in
                    Button {
                        // Auto-fill suggestion
                    } label: {
                        HStack(spacing: DesignSystem.Spacing.sm) {
                            Image(systemName: "lightbulb.fill")
                                .foregroundStyle(DesignSystem.Colors.warning)
                                .font(DesignSystem.Typography.caption)
                            Text(suggestion)
                                .font(DesignSystem.Typography.callout)
                                .foregroundStyle(DesignSystem.Colors.textPrimary)
                        }
                        .padding(.horizontal, DesignSystem.Spacing.md)
                        .padding(.vertical, DesignSystem.Spacing.sm + 2)
                        .background(DesignSystem.Colors.textSecondary.opacity(0.1))
                        .clipShape(RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.medium))
                        .overlay(
                            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.medium)
                                .stroke(DesignSystem.Colors.textSecondary.opacity(0.2), lineWidth: 1)
                        )
                    }
                    .buttonStyle(.plain)
                }
            }

            Spacer()
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(nsColor: .windowBackgroundColor))
    }
}

// MARK: - Message Bubble

struct MessageBubble: View {
    let message: Message
    @State private var isHovering = false
    @State private var showCopied = false

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
                            .font(DesignSystem.Typography.caption)
                            .fontWeight(.semibold)
                    }

                    Text(message.formattedTime)
                        .font(DesignSystem.Typography.caption2)
                        .foregroundStyle(DesignSystem.Colors.textSecondary)

                    // Copy button (appears on hover)
                    if isHovering && !message.isFromUser {
                        Button {
                            copyToClipboard()
                        } label: {
                            HStack(spacing: 2) {
                                Image(systemName: showCopied ? "checkmark" : "doc.on.doc")
                                if showCopied {
                                    Text("Copied!")
                                        .font(.caption2)
                                }
                            }
                            .font(.caption)
                            .foregroundStyle(showCopied ? .green : .secondary)
                        }
                        .buttonStyle(.plain)
                        .transition(.opacity)
                    }
                }

                // Message content
                Text(message.content)
                    .textSelection(.enabled)
                    .padding(.horizontal, DesignSystem.Spacing.md)
                    .padding(.vertical, DesignSystem.Spacing.sm)
                    .background(bubbleBackground)
                    .clipShape(RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.large))
                    .shadow(
                        color: DesignSystem.Shadow.medium.color,
                        radius: DesignSystem.Shadow.medium.radius,
                        x: DesignSystem.Shadow.medium.x,
                        y: DesignSystem.Shadow.medium.y
                    )
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
        .onHover { hovering in
            withAnimation(.easeInOut(duration: 0.15)) {
                isHovering = hovering
            }
        }
        .contextMenu {
            Button("Copy") {
                copyToClipboard()
            }
        }
    }

    private func copyToClipboard() {
        NSPasteboard.general.clearContents()
        NSPasteboard.general.setString(message.content, forType: .string)

        withAnimation {
            showCopied = true
        }
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.5) {
            withAnimation {
                showCopied = false
            }
        }
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
            LinearGradient(
                colors: [DesignSystem.Colors.primary.opacity(0.2), DesignSystem.Colors.primary.opacity(0.15)],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
        } else if message.type == .convergence {
            LinearGradient(
                colors: [DesignSystem.Colors.primary.opacity(0.15), DesignSystem.Colors.info.opacity(0.15)],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
        } else if message.type == .error {
            DesignSystem.Colors.error.opacity(0.15)
        } else {
            DesignSystem.Colors.textSecondary.opacity(0.15)
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

            VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                HStack {
                    Text("Thinking")
                        .font(DesignSystem.Typography.caption)
                        .fontWeight(.semibold)

                    Text(String(repeating: ".", count: dotCount + 1))
                        .font(DesignSystem.Typography.caption)
                        .foregroundStyle(DesignSystem.Colors.textSecondary)
                }

                if !text.isEmpty {
                    Text(text)
                        .textSelection(.enabled)
                        .padding(.horizontal, DesignSystem.Spacing.md)
                        .padding(.vertical, DesignSystem.Spacing.sm)
                        .background(DesignSystem.Colors.textSecondary.opacity(0.1))
                        .clipShape(RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.large))
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
        HStack(spacing: DesignSystem.Spacing.xs) {
            ForEach(0..<3, id: \.self) { index in
                Circle()
                    .fill(DesignSystem.Colors.textSecondary)
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
        .padding(.horizontal, DesignSystem.Spacing.md)
        .padding(.vertical, DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.textSecondary.opacity(0.1))
        .clipShape(RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.large))
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
        HStack(spacing: DesignSystem.Spacing.md) {
            // Model indicator
            HStack(spacing: DesignSystem.Spacing.xs) {
                Image(systemName: "cpu")
                    .font(DesignSystem.Typography.caption2)
                Text(orchestratorVM.currentModel)
                    .font(DesignSystem.Typography.caption2)
            }
            .foregroundStyle(DesignSystem.Colors.textSecondary)

            Divider()
                .frame(height: 12)

            // Token count
            HStack(spacing: DesignSystem.Spacing.xs) {
                Image(systemName: "number")
                    .font(DesignSystem.Typography.caption2)
                Text("\(orchestratorVM.costInfo.sessionUsage.totalTokens) tokens")
                    .font(DesignSystem.Typography.caption2)
            }
            .foregroundStyle(DesignSystem.Colors.textSecondary)

            Divider()
                .frame(height: 12)

            // Cost indicator
            HStack(spacing: DesignSystem.Spacing.xs) {
                Image(systemName: "dollarsign.circle")
                    .font(DesignSystem.Typography.caption2)
                Text(String(format: "$%.2f / $%.0f", orchestratorVM.costInfo.sessionCost, orchestratorVM.costInfo.budgetLimit))
                    .font(DesignSystem.Typography.caption2)
            }
            .foregroundStyle(orchestratorVM.costInfo.isOverBudget ? DesignSystem.Colors.error : DesignSystem.Colors.textSecondary)

            Spacer()

            // Message count
            HStack(spacing: DesignSystem.Spacing.xs) {
                Image(systemName: "bubble.left.and.bubble.right")
                    .font(DesignSystem.Typography.caption2)
                Text("\(conversationVM.messages.count) messages")
                    .font(DesignSystem.Typography.caption2)
            }
            .foregroundStyle(DesignSystem.Colors.textSecondary)
        }
        .padding(.horizontal, DesignSystem.Spacing.md)
        .padding(.vertical, DesignSystem.Spacing.xs)
        .background(DesignSystem.Colors.background)
    }
}

// MARK: - Previews

#Preview {
    ConversationView()
        .environmentObject(ConversationViewModel.preview)
        .environmentObject(OrchestratorViewModel.preview)
}
