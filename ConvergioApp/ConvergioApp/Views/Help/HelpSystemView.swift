/**
 * CONVERGIO NATIVE - In-App Help System
 *
 * Comprehensive help system with contextual tooltips, guided tours,
 * searchable documentation, and quick-access help cards.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

// MARK: - Help Topic Model

struct HelpTopic: Identifiable {
    let id = UUID()
    let title: String
    let icon: String
    let category: HelpCategory
    let content: String
    let relatedTopics: [String]
    let keywords: [String]
}

enum HelpCategory: String, CaseIterable {
    case gettingStarted = "Getting Started"
    case conversations = "Conversations"
    case agents = "Agents"
    case settings = "Settings"
    case shortcuts = "Shortcuts"
    case troubleshooting = "Troubleshooting"

    var icon: String {
        switch self {
        case .gettingStarted: return "star.fill"
        case .conversations: return "bubble.left.and.bubble.right.fill"
        case .agents: return "person.3.fill"
        case .settings: return "gear"
        case .shortcuts: return "keyboard"
        case .troubleshooting: return "wrench.and.screwdriver.fill"
        }
    }

    var color: Color {
        switch self {
        case .gettingStarted: return .yellow
        case .conversations: return .blue
        case .agents: return .purple
        case .settings: return .gray
        case .shortcuts: return .green
        case .troubleshooting: return .orange
        }
    }
}

// MARK: - Help Content Database

struct HelpDatabase {
    static let topics: [HelpTopic] = [
        // Getting Started
        HelpTopic(
            title: "Welcome to Convergio",
            icon: "hand.wave.fill",
            category: .gettingStarted,
            content: """
            # Welcome to Convergio

            Convergio is your AI executive team - a powerful multi-agent system that coordinates specialized AI agents to help you accomplish complex tasks.

            ## Key Features

            - **Multi-Agent Collaboration**: Multiple AI specialists work together
            - **Budget Control**: Set spending limits and track costs
            - **Model Selection**: Choose from multiple AI providers
            - **Customizable Agents**: Edit agent system prompts

            ## Quick Start

            1. Configure at least one API key in Settings
            2. Type your request in the chat
            3. Watch your AI team collaborate!
            """,
            relatedTopics: ["Setting Up API Keys", "Your First Conversation"],
            keywords: ["welcome", "start", "begin", "intro", "introduction"]
        ),
        HelpTopic(
            title: "Setting Up API Keys",
            icon: "key.fill",
            category: .gettingStarted,
            content: """
            # Setting Up API Keys

            Convergio supports multiple AI providers. You need at least one API key to get started.

            ## Supported Providers

            - **Anthropic** (Claude) - Recommended
            - **OpenAI** (GPT-4)
            - **Google** (Gemini)
            - **OpenRouter** (Multiple models)

            ## How to Add a Key

            1. Open **Settings** (Cmd+,)
            2. Go to the **Providers** tab
            3. Select your provider
            4. Paste your API key
            5. Click **Save Key**

            ## Security

            API keys are stored securely in your macOS Keychain and never sent anywhere except to the respective API providers.
            """,
            relatedTopics: ["Budget Management", "Choosing a Model"],
            keywords: ["api", "key", "setup", "anthropic", "openai", "configure"]
        ),
        HelpTopic(
            title: "Your First Conversation",
            icon: "text.bubble.fill",
            category: .gettingStarted,
            content: """
            # Your First Conversation

            Starting a conversation with Convergio is easy!

            ## How It Works

            1. Type your message in the input field at the bottom
            2. Press **Cmd+Return** or click Send
            3. The orchestrator agent analyzes your request
            4. Specialized agents are activated as needed
            5. A coordinated response is generated

            ## Tips

            - Be specific about what you need
            - Complex tasks work best (that's where multiple agents shine!)
            - Watch the Agent Panel to see which agents are working

            ## Example Prompts

            - "Analyze this business problem and suggest solutions"
            - "Write a technical document about X"
            - "Help me plan a project with milestones"
            """,
            relatedTopics: ["Understanding Agents", "Conversation History"],
            keywords: ["first", "conversation", "chat", "message", "start"]
        ),

        // Conversations
        HelpTopic(
            title: "Conversation History",
            icon: "clock.arrow.circlepath",
            category: .conversations,
            content: """
            # Conversation History

            Convergio maintains your conversation history for context continuity.

            ## Managing History

            - **New Conversation**: Cmd+N starts fresh
            - **Clear History**: Cmd+Shift+K clears current conversation
            - **Export Chat**: Save conversations as text files

            ## Context Window

            The AI uses recent messages as context. Very long conversations may need to be trimmed for optimal performance.

            ## Privacy

            All conversations are stored locally on your Mac. Nothing is sent to any server except for API requests to your chosen AI provider.
            """,
            relatedTopics: ["Export Options", "Privacy Settings"],
            keywords: ["history", "context", "conversation", "clear", "export"]
        ),
        HelpTopic(
            title: "Export Options",
            icon: "square.and.arrow.up.fill",
            category: .conversations,
            content: """
            # Export Options

            Save your conversations for future reference.

            ## Export Formats

            - **Plain Text** (.txt) - Simple, readable format
            - More formats coming soon!

            ## How to Export

            1. Right-click in the conversation area
            2. Select "Export Chat"
            3. Choose location and format
            4. Click Save

            Or use the Dock menu (right-click on app icon) for quick access.
            """,
            relatedTopics: ["Conversation History"],
            keywords: ["export", "save", "download", "text", "file"]
        ),

        // Agents
        HelpTopic(
            title: "Understanding Agents",
            icon: "person.3.sequence.fill",
            category: .agents,
            content: """
            # Understanding Agents

            Convergio uses specialized AI agents that collaborate to help you.

            ## Agent Roles

            - **Orchestrator (Ali)**: Coordinates all agents
            - **Analyst**: Research and data analysis
            - **Coder**: Software development
            - **Writer**: Content creation
            - **Critic**: Quality review
            - **Planner**: Task breakdown
            - **Executor**: Tool execution
            - **Memory**: Context retrieval

            ## How They Work

            When you send a message, the Orchestrator analyzes it and decides which specialists to involve. They work together and the Orchestrator synthesizes their outputs.
            """,
            relatedTopics: ["Agent Visualizer", "Customizing Agents"],
            keywords: ["agent", "role", "specialist", "team", "orchestrator"]
        ),
        HelpTopic(
            title: "Agent Visualizer",
            icon: "circle.hexagongrid.fill",
            category: .agents,
            content: """
            # Agent Visualizer

            Watch your AI team collaborate in real-time!

            ## Accessing the Visualizer

            - Menu: View > Agent Interaction Visualizer
            - Shortcut: Cmd+Opt+3

            ## What You'll See

            - **Central Hub**: The orchestrator
            - **Outer Nodes**: Specialized agents
            - **Connection Lines**: Active communication
            - **Pulse Effects**: Data flow

            ## Status Indicators

            - **Green**: Agent is active
            - **Gray**: Agent is idle
            - **Purple**: Converging results
            """,
            relatedTopics: ["Understanding Agents"],
            keywords: ["visualizer", "visual", "animation", "status", "activity"]
        ),
        HelpTopic(
            title: "Customizing Agents",
            icon: "slider.horizontal.3",
            category: .agents,
            content: """
            # Customizing Agents

            Tailor agent behavior to your needs using the Agent Editor.

            ## Accessing the Editor

            - Menu: View > Agent Editor
            - Shortcut: Cmd+Opt+4

            ## What You Can Customize

            - **System Prompt**: Core instructions in Markdown
            - **Custom Instructions**: Additional guidance
            - **Temperature**: Creativity level (0-1)
            - **Max Tokens**: Response length limit
            - **Enable/Disable**: Turn agents on/off

            ## Tips

            - Use Markdown formatting for clarity
            - Be specific about desired behavior
            - Test changes with sample prompts
            """,
            relatedTopics: ["Understanding Agents", "Agent Visualizer"],
            keywords: ["customize", "edit", "prompt", "system", "markdown"]
        ),

        // Settings
        HelpTopic(
            title: "Budget Management",
            icon: "dollarsign.circle.fill",
            category: .settings,
            content: """
            # Budget Management

            Control your AI spending with Convergio's budget features.

            ## Setting Budgets

            1. Open Settings (Cmd+,)
            2. Go to Budget tab
            3. Set your limits

            ## Budget Types

            - **Session Budget**: Per-session limit
            - **Monthly Budget**: Monthly spending cap
            - **Warning Threshold**: Alert percentage

            ## Cost Tracking

            - View real-time costs in the Cost Dashboard
            - See per-request breakdowns
            - Monitor token usage

            ## Pause on Exceed

            Enable this to automatically stop requests when budget is reached.
            """,
            relatedTopics: ["Choosing a Model", "Cost Optimization"],
            keywords: ["budget", "cost", "money", "spending", "limit"]
        ),
        HelpTopic(
            title: "MCP Services",
            icon: "server.rack",
            category: .settings,
            content: """
            # MCP Services

            Connect external tools via Model Context Protocol.

            ## What is MCP?

            Model Context Protocol (MCP) allows AI models to connect to external services and tools, extending their capabilities.

            ## Configuring Servers

            1. Open Settings (Cmd+,)
            2. Go to MCP Services tab
            3. Add or edit server configurations

            ## Available Servers

            - **Filesystem**: File operations
            - **GitHub**: Repository access
            - **Memory**: Persistent storage
            - Custom servers

            ## Security

            MCP servers run locally. Be careful with permissions and only enable trusted servers.
            """,
            relatedTopics: ["Advanced Settings"],
            keywords: ["mcp", "server", "protocol", "tools", "external"]
        ),

        // Shortcuts
        HelpTopic(
            title: "Keyboard Shortcuts",
            icon: "keyboard.fill",
            category: .shortcuts,
            content: """
            # Keyboard Shortcuts

            Master these shortcuts to boost your productivity.

            ## General

            - **Cmd+N**: New Conversation
            - **Cmd+,**: Open Settings
            - **Cmd+Q**: Quit Convergio

            ## Conversation

            - **Cmd+Return**: Send Message
            - **Cmd+.**: Cancel Request
            - **Cmd+Shift+K**: Clear History

            ## View

            - **Cmd+Opt+1**: Toggle Agent Panel
            - **Cmd+Opt+2**: Toggle Cost Dashboard
            - **Cmd+Opt+3**: Agent Visualizer
            - **Cmd+Opt+4**: Agent Editor
            - **Cmd+Opt+L**: Log Viewer

            ## Global

            - **Cmd+Shift+Space**: Activate from anywhere
            """,
            relatedTopics: [],
            keywords: ["shortcut", "keyboard", "hotkey", "command", "quick"]
        ),

        // Troubleshooting
        HelpTopic(
            title: "Common Issues",
            icon: "exclamationmark.triangle.fill",
            category: .troubleshooting,
            content: """
            # Common Issues

            Solutions to frequently encountered problems.

            ## API Key Not Working

            1. Verify the key in your provider's dashboard
            2. Check for extra spaces when pasting
            3. Ensure your account has API access
            4. Try the Test Connection button

            ## Slow Responses

            - Complex tasks take longer (multiple agents working)
            - Check your internet connection
            - Try a smaller model for faster responses

            ## High Costs

            - Set a budget limit in Settings
            - Use smaller models for simple tasks
            - Reduce Max Tokens in agent settings

            ## App Not Responding

            - Check Log Viewer (Cmd+Opt+L) for errors
            - Restart the app
            - Report persistent issues on GitHub
            """,
            relatedTopics: ["Setting Up API Keys", "Budget Management"],
            keywords: ["problem", "issue", "error", "fix", "help", "troubleshoot"]
        ),
    ]

    static func search(_ query: String) -> [HelpTopic] {
        let lowercaseQuery = query.lowercased()
        return topics.filter { topic in
            topic.title.lowercased().contains(lowercaseQuery) ||
            topic.content.lowercased().contains(lowercaseQuery) ||
            topic.keywords.contains { $0.lowercased().contains(lowercaseQuery) }
        }
    }

    static func topics(for category: HelpCategory) -> [HelpTopic] {
        topics.filter { $0.category == category }
    }
}

// MARK: - Main Help View

struct HelpSystemView: View {
    @State private var searchText = ""
    @State private var selectedCategory: HelpCategory?
    @State private var selectedTopic: HelpTopic?

    private var filteredTopics: [HelpTopic] {
        if !searchText.isEmpty {
            return HelpDatabase.search(searchText)
        } else if let category = selectedCategory {
            return HelpDatabase.topics(for: category)
        }
        return HelpDatabase.topics
    }

    var body: some View {
        NavigationSplitView {
            // Sidebar with categories
            List(selection: $selectedCategory) {
                Section("Categories") {
                    ForEach(HelpCategory.allCases, id: \.self) { category in
                        Label(category.rawValue, systemImage: category.icon)
                            .tag(category)
                    }
                }

                Section("Quick Links") {
                    Link(destination: URL(string: "https://github.com/Roberdan/Convergio/wiki")!) {
                        Label("Online Documentation", systemImage: "globe")
                    }
                    Link(destination: URL(string: "https://github.com/Roberdan/Convergio/issues")!) {
                        Label("Report an Issue", systemImage: "exclamationmark.bubble")
                    }
                }
            }
            .listStyle(.sidebar)
            .frame(minWidth: 180)
        } content: {
            // Topic list
            VStack(spacing: 0) {
                // Search bar
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundStyle(.secondary)
                    TextField("Search help topics...", text: $searchText)
                        .textFieldStyle(.plain)
                    if !searchText.isEmpty {
                        Button {
                            searchText = ""
                        } label: {
                            Image(systemName: "xmark.circle.fill")
                                .foregroundStyle(.secondary)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(10)
                .background(Color.primary.opacity(0.05))

                List(selection: $selectedTopic) {
                    ForEach(filteredTopics) { topic in
                        HelpTopicRow(topic: topic)
                            .tag(topic)
                    }
                }
                .listStyle(.plain)
            }
            .frame(minWidth: 250)
        } detail: {
            // Topic detail
            if let topic = selectedTopic {
                HelpTopicDetail(topic: topic, onSelectRelated: { title in
                    if let related = HelpDatabase.topics.first(where: { $0.title == title }) {
                        selectedTopic = related
                    }
                })
            } else {
                HelpWelcomeView()
            }
        }
        .frame(minWidth: 800, minHeight: 500)
    }
}

// MARK: - Help Topic Row

private struct HelpTopicRow: View {
    let topic: HelpTopic

    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: topic.icon)
                .font(.title3)
                .foregroundStyle(topic.category.color)
                .frame(width: 32)

            VStack(alignment: .leading, spacing: 2) {
                Text(topic.title)
                    .font(.headline)
                Text(topic.category.rawValue)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
        .padding(.vertical, 4)
    }
}

// MARK: - Help Topic Detail

private struct HelpTopicDetail: View {
    let topic: HelpTopic
    var onSelectRelated: (String) -> Void

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                // Header
                HStack(spacing: 16) {
                    ZStack {
                        Circle()
                            .fill(topic.category.color.gradient)
                            .frame(width: 60, height: 60)
                        Image(systemName: topic.icon)
                            .font(.title)
                            .foregroundStyle(.white)
                    }

                    VStack(alignment: .leading, spacing: 4) {
                        Text(topic.title)
                            .font(.largeTitle.bold())
                        Label(topic.category.rawValue, systemImage: topic.category.icon)
                            .font(.subheadline)
                            .foregroundStyle(.secondary)
                    }

                    Spacer()
                }

                Divider()

                // Content (rendered as simple markdown)
                MarkdownContentView(content: topic.content)

                // Related topics
                if !topic.relatedTopics.isEmpty {
                    Divider()

                    VStack(alignment: .leading, spacing: 12) {
                        Text("Related Topics")
                            .font(.headline)

                        FlowLayout(spacing: 8) {
                            ForEach(topic.relatedTopics, id: \.self) { related in
                                Button {
                                    onSelectRelated(related)
                                } label: {
                                    Label(related, systemImage: "arrow.right.circle")
                                        .font(.caption)
                                }
                                .buttonStyle(.bordered)
                            }
                        }
                    }
                }
            }
            .padding(24)
        }
    }
}

// MARK: - Markdown Content View

private struct MarkdownContentView: View {
    let content: String

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            ForEach(content.components(separatedBy: "\n\n"), id: \.self) { paragraph in
                renderParagraph(paragraph)
            }
        }
    }

    @ViewBuilder
    private func renderParagraph(_ text: String) -> some View {
        if text.hasPrefix("# ") {
            Text(text.dropFirst(2))
                .font(.title.bold())
        } else if text.hasPrefix("## ") {
            Text(text.dropFirst(3))
                .font(.title2.bold())
                .padding(.top, 8)
        } else if text.hasPrefix("### ") {
            Text(text.dropFirst(4))
                .font(.title3.bold())
        } else if text.contains("\n- ") || text.hasPrefix("- ") {
            VStack(alignment: .leading, spacing: 6) {
                ForEach(text.components(separatedBy: "\n"), id: \.self) { line in
                    if line.hasPrefix("- ") {
                        HStack(alignment: .top, spacing: 8) {
                            Text("•")
                                .foregroundStyle(.secondary)
                            Text(parseInline(String(line.dropFirst(2))))
                        }
                    } else if !line.isEmpty {
                        Text(parseInline(line))
                    }
                }
            }
        } else if text.contains("\n1. ") || text.hasPrefix("1. ") {
            VStack(alignment: .leading, spacing: 6) {
                ForEach(Array(text.components(separatedBy: "\n").enumerated()), id: \.offset) { index, line in
                    if let range = line.range(of: "^\\d+\\. ", options: .regularExpression) {
                        HStack(alignment: .top, spacing: 8) {
                            Text("\(index + 1).")
                                .foregroundStyle(.secondary)
                                .frame(width: 20, alignment: .trailing)
                            Text(parseInline(String(line[range.upperBound...])))
                        }
                    } else if !line.isEmpty {
                        Text(parseInline(line))
                    }
                }
            }
        } else {
            Text(parseInline(text))
        }
    }

    private func parseInline(_ text: String) -> AttributedString {
        var result = text

        // Simple bold handling
        while let startRange = result.range(of: "**") {
            if let endRange = result.range(of: "**", range: startRange.upperBound..<result.endIndex) {
                let boldText = String(result[startRange.upperBound..<endRange.lowerBound])
                result.replaceSubrange(startRange.lowerBound..<endRange.upperBound, with: boldText)
            } else {
                break
            }
        }

        return AttributedString(result)
    }
}

// MARK: - Help Welcome View

private struct HelpWelcomeView: View {
    var body: some View {
        VStack(spacing: 24) {
            Image(systemName: "questionmark.circle.fill")
                .font(.system(size: 64))
                .foregroundStyle(.purple.gradient)

            Text("Convergio Help")
                .font(.largeTitle.bold())

            Text("Select a topic from the list or search for help")
                .font(.headline)
                .foregroundStyle(.secondary)

            Divider()
                .frame(width: 200)

            // Quick help cards
            LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 16) {
                QuickHelpCard(
                    icon: "star.fill",
                    title: "Getting Started",
                    description: "New to Convergio? Start here",
                    color: .yellow
                )
                QuickHelpCard(
                    icon: "keyboard.fill",
                    title: "Shortcuts",
                    description: "Learn keyboard shortcuts",
                    color: .green
                )
                QuickHelpCard(
                    icon: "person.3.fill",
                    title: "Agents",
                    description: "Understand your AI team",
                    color: .purple
                )
                QuickHelpCard(
                    icon: "gear",
                    title: "Settings",
                    description: "Configure Convergio",
                    color: .gray
                )
            }
            .frame(maxWidth: 500)
        }
        .padding(40)
    }
}

private struct QuickHelpCard: View {
    let icon: String
    let title: String
    let description: String
    let color: Color

    var body: some View {
        VStack(spacing: 12) {
            Image(systemName: icon)
                .font(.title)
                .foregroundStyle(color)

            Text(title)
                .font(.headline)

            Text(description)
                .font(.caption)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity)
        .padding()
        .background(color.opacity(0.1))
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

// MARK: - Flow Layout

private struct FlowLayout: Layout {
    var spacing: CGFloat = 8

    func sizeThatFits(proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) -> CGSize {
        let result = FlowResult(in: proposal.width ?? 0, spacing: spacing, subviews: subviews)
        return result.size
    }

    func placeSubviews(in bounds: CGRect, proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) {
        let result = FlowResult(in: bounds.width, spacing: spacing, subviews: subviews)
        for (index, subview) in subviews.enumerated() {
            subview.place(at: CGPoint(x: bounds.minX + result.positions[index].x,
                                      y: bounds.minY + result.positions[index].y),
                         proposal: .unspecified)
        }
    }

    private struct FlowResult {
        var size: CGSize = .zero
        var positions: [CGPoint] = []

        init(in width: CGFloat, spacing: CGFloat, subviews: Subviews) {
            var x: CGFloat = 0
            var y: CGFloat = 0
            var rowHeight: CGFloat = 0

            for subview in subviews {
                let size = subview.sizeThatFits(.unspecified)
                if x + size.width > width && x > 0 {
                    x = 0
                    y += rowHeight + spacing
                    rowHeight = 0
                }
                positions.append(CGPoint(x: x, y: y))
                rowHeight = max(rowHeight, size.height)
                x += size.width + spacing
                self.size.width = max(self.size.width, x)
            }
            self.size.height = y + rowHeight
        }
    }
}

// MARK: - Contextual Help Tooltip

struct ContextualHelpButton: View {
    let topic: String
    @State private var showPopover = false

    private var helpContent: HelpTopic? {
        HelpDatabase.topics.first { $0.title == topic || $0.keywords.contains(topic.lowercased()) }
    }

    var body: some View {
        Button {
            showPopover.toggle()
        } label: {
            Image(systemName: "questionmark.circle")
                .foregroundStyle(.secondary)
        }
        .buttonStyle(.plain)
        .popover(isPresented: $showPopover) {
            if let content = helpContent {
                VStack(alignment: .leading, spacing: 12) {
                    HStack {
                        Image(systemName: content.icon)
                            .foregroundStyle(content.category.color)
                        Text(content.title)
                            .font(.headline)
                    }

                    Text(content.content.prefix(300) + "...")
                        .font(.caption)
                        .foregroundStyle(.secondary)

                    Button("Learn more...") {
                        // Open full help
                    }
                    .font(.caption)
                }
                .padding()
                .frame(width: 300)
            } else {
                Text("No help available for this topic")
                    .padding()
            }
        }
        .help("Get help about \(topic)")
    }
}

// MARK: - Preview

#Preview("Help System") {
    HelpSystemView()
}
