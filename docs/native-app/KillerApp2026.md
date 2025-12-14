# Convergio Native: Killer App 2026

> **Vision:** Transform Convergio from a powerful CLI into a stunning native macOS application that leverages Apple's new Liquid Glass design language, becoming the definitive AI executive team interface for professionals.

**Created:** 2025-12-14 18:49:29
**Author:** Roberto + AI Team
**Status:** PLANNING PHASE

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Brutally Honest Assessment](#brutally-honest-assessment)
3. [Architecture Diagrams (Mermaid)](#architecture-diagrams-mermaid)
4. [macOS Tahoe 26 & Liquid Glass Deep Dive](#macos-tahoe-26--liquid-glass-deep-dive)
5. [Architecture: Unified Repository Strategy](#architecture-unified-repository-strategy)
6. [Advanced Features: Anna, Notifications, Browser](#advanced-features-anna-notifications-browser)
7. [Vertical Market Solutions](#vertical-market-solutions)
8. [Accessibility & Inclusive Design](#accessibility--inclusive-design)
9. [UI/UX Design Proposals](#uiux-design-proposals)
10. [Technical Implementation Plan](#technical-implementation-plan)
11. [Phase Breakdown & Milestones](#phase-breakdown--milestones)
12. [Risk Matrix & Mitigations](#risk-matrix--mitigations)
13. [Competitive Analysis](#competitive-analysis)
14. [Success Metrics](#success-metrics)

---

## Executive Summary

### The Opportunity

Convergio CLI is **architecturally perfect** for native app conversion:
- Clean separation: 80%+ code reusable as-is
- Already Objective-C integrated (7 .m files)
- Provider abstraction is UI-agnostic
- SQLite persistence works natively

### The Vision

**Convergio Native** will be the first AI executive team application to fully embrace macOS Tahoe 26's Liquid Glass design language, creating an interface where:

- 54+ AI agents appear as **living glass entities** that morph, collaborate, and converge
- The user experience shifts from "typing commands" to "directing a team"
- Professional workflows become **visual, intuitive, and delightful**

### Why Now?

1. **Liquid Glass just launched** (September 2025) - First-mover advantage
2. **No competitor** has a multi-agent orchestration UI on Mac
3. **Apple Intelligence integration** opportunities in macOS 26
4. **CLI market saturated** - GUI is the next frontier

---

## Brutally Honest Assessment

### What Will Work

| Aspect | Confidence | Reason |
|--------|------------|--------|
| Core C library integration | 95% | Swift-C interop is mature; we have working .m bridges |
| Provider abstraction | 100% | Already UI-agnostic by design |
| SQLite persistence | 100% | Native macOS support |
| Multi-agent orchestration | 90% | Message bus architecture translates directly |
| Cost tracking | 100% | Pure business logic, no UI coupling |

### What Will Be Hard

| Challenge | Severity | Why |
|-----------|----------|-----|
| **54 agents visualization** | HIGH | How do you show 54 agents without overwhelming? |
| **Convergence UX** | HIGH | CLI shows linear text; GUI needs spatial representation |
| **Readline replacement** | MEDIUM | Auto-completion system needs SwiftUI reimplementation |
| **Real-time streaming** | MEDIUM | ANSI markdown → native text rendering is non-trivial |
| **Testing** | HIGH | E2E tests are CLI-focused; need complete rewrite |
| **Menu bar + main app** | MEDIUM | Two UI paradigms in one app |

### What Could Kill This Project

1. **Scope creep** - Trying to do everything at once
2. **UX paralysis** - 54 agents is a UX nightmare if not handled carefully
3. **Performance** - SwiftUI + C interop + real-time streaming = potential bottlenecks
4. **Xcode 26 bugs** - Liquid Glass is new; expect framework issues

### Honest Timeline

| Scenario | Duration | Notes |
|----------|----------|-------|
| **MVP (functional)** | 8-10 weeks | Core features, basic UI |
| **Beta (polished)** | 14-16 weeks | Full Liquid Glass, testing |
| **App Store ready** | 20-24 weeks | Sandboxing, notarization, polish |

---

## Architecture Diagrams (Mermaid)

### System Architecture Overview

```mermaid
graph TB
    subgraph "Convergio Native App"
        subgraph "SwiftUI Layer"
            UI[SwiftUI Views]
            VM[ViewModels]
            SVC[Services]
        end

        subgraph "ConvergioCore Swift Package"
            BRIDGE[Swift-C Bridge]
            TYPES[Swift Types]
            ASYNC[Async Handlers]
        end
    end

    subgraph "C Core Library (libConvergio.a)"
        subgraph "Orchestration"
            ORCH[Orchestrator]
            MSGBUS[Message Bus]
            COST[Cost Controller]
        end

        subgraph "Agents"
            ALI[Ali - Chief of Staff]
            AGENTS[54+ Specialized Agents]
        end

        subgraph "Providers"
            ANTHROPIC[Anthropic]
            OPENAI[OpenAI]
            GEMINI[Gemini]
            MLX[MLX Local]
            OLLAMA[Ollama]
        end

        subgraph "Persistence"
            SQLITE[(SQLite DB)]
            SEMANTIC[Semantic Memory]
        end
    end

    subgraph "macOS System"
        NOTIF[Notification Center]
        KEYCHAIN[Keychain]
        METAL[Metal GPU]
        WEBKIT[WebKit]
    end

    UI --> VM
    VM --> SVC
    SVC --> BRIDGE
    BRIDGE --> ORCH
    ORCH --> MSGBUS
    ORCH --> ALI
    ALI --> AGENTS
    ORCH --> COST
    AGENTS --> ANTHROPIC
    AGENTS --> OPENAI
    AGENTS --> GEMINI
    AGENTS --> MLX
    AGENTS --> OLLAMA
    ORCH --> SQLITE
    ORCH --> SEMANTIC

    SVC --> NOTIF
    SVC --> KEYCHAIN
    MLX --> METAL
    SVC --> WEBKIT
```

### Agent Convergence Flow

```mermaid
sequenceDiagram
    participant U as User
    participant UI as SwiftUI
    participant Ali as Ali (Chief of Staff)
    participant A1 as Angela (Data)
    participant A2 as Amy (CFO)
    participant A3 as Matteo (Strategy)
    participant Conv as Convergence Engine

    U->>UI: "How should we enter EMEA market?"
    UI->>Ali: Route question

    Ali->>Ali: Analyze intent
    Ali->>A1: Request data analysis
    Ali->>A2: Request financial assessment
    Ali->>A3: Request strategy input

    par Parallel Agent Work
        A1->>A1: Analyze market data
        A2->>A2: Calculate budget options
        A3->>A3: Draft strategic framework
    end

    A1-->>Conv: Data insights ready
    A2-->>Conv: Financial analysis ready
    A3-->>Conv: Strategy draft ready

    Conv->>Conv: Synthesize perspectives
    Conv->>Ali: Converged analysis

    Ali->>UI: Stream final response
    UI->>U: Display with agent attribution
```

### Data Flow Architecture

```mermaid
flowchart LR
    subgraph Input
        USER[User Input]
        VOICE[Voice Input]
        PASTE[Clipboard]
        FILE[File Drop]
    end

    subgraph Processing
        INTENT[Intent Parser]
        ROUTER[Agent Router]
        QUEUE[Message Queue]
    end

    subgraph Agents
        direction TB
        ALI[Ali]
        TEAM[Agent Team]
        CONV[Convergence]
    end

    subgraph Output
        STREAM[Streaming Response]
        NOTIF[Notifications]
        EXPORT[Export/Share]
    end

    subgraph Storage
        DB[(SQLite)]
        MEM[Semantic Memory]
        CACHE[Response Cache]
    end

    USER --> INTENT
    VOICE --> INTENT
    PASTE --> INTENT
    FILE --> INTENT

    INTENT --> ROUTER
    ROUTER --> QUEUE
    QUEUE --> ALI
    ALI --> TEAM
    TEAM --> CONV
    CONV --> STREAM

    STREAM --> NOTIF
    STREAM --> EXPORT

    CONV --> DB
    CONV --> MEM
    ROUTER --> CACHE
```

### Feature Map

```mermaid
mindmap
  root((Convergio Native))
    Core Features
      Multi-Agent Chat
        54+ Specialized Agents
        Agent Selection
        Team Presets
      Convergence Engine
        Visual Convergence
        Progress Tracking
        Attribution
      Cost Management
        Real-time Tracking
        Budget Alerts
        Usage History
    Anna Services
      Calendar Integration
        Meeting Scheduling
        Reminder Management
        Event Creation
      Task Management
        Todo Lists
        Priority Sorting
        Deadline Tracking
      Communication
        Email Drafting
        Message Templates
        Follow-up Reminders
    Native Integration
      Notifications
        macOS Notification Center
        Custom Alert Types
        Action Buttons
      Menu Bar
        Quick Access
        Status Display
        Mini Chat
      Global Hotkey
        Toggle Window
        Quick Prompt
        Voice Activation
    Browser & Research
      In-App Browser
        Web Research
        Link Preview
        Page Capture
      Content Integration
        URL Analysis
        Document Import
        Image Analysis
    Offline Mode
      MLX Local Inference
        Privacy Mode
        No Internet Required
        Fast Responses
      Local Storage
        Encrypted DB
        Semantic Cache
        History Export
```

### Development Timeline (Gantt)

```mermaid
gantt
    title Convergio Native Development Plan
    dateFormat  YYYY-MM-DD

    section Phase 0: Foundation
    C-Swift Bridge Setup           :p0a, 2026-01-06, 1w
    Core Library Integration       :p0b, after p0a, 1w
    Smoke Testing                  :p0c, after p0b, 3d

    section Phase 1: Skeleton App
    Xcode Project Setup            :p1a, after p0c, 3d
    Basic SwiftUI Structure        :p1b, after p1a, 1w
    Menu Bar Integration           :p1c, after p1b, 4d

    section Phase 2: Liquid Glass
    Agent Cards UI                 :p2a, after p1c, 1w
    Conversation View              :p2b, after p2a, 1w
    Convergence Animation          :p2c, after p2b, 1w
    Accessibility Testing          :p2d, after p2c, 4d

    section Phase 3: Advanced
    Anna Services                  :p3a, after p2d, 2w
    Notification System            :p3b, after p2d, 1w
    In-App Browser                 :p3c, after p3b, 1w
    Global Hotkey                  :p3d, after p3c, 4d

    section Phase 4: Polish
    Performance Optimization       :p4a, after p3d, 1w
    UI Polish                      :p4b, after p4a, 1w
    Testing & Bug Fixes            :p4c, after p4b, 2w

    section Phase 5: Release
    Sandboxing                     :p5a, after p4c, 1w
    App Store Prep                 :p5b, after p5a, 1w
    Beta Testing                   :p5c, after p5b, 2w
    Launch                         :milestone, after p5c, 0d
```

### Component Dependency Graph

```mermaid
graph BT
    subgraph "UI Components"
        ContentView
        SidebarView
        ConversationView
        AgentCardView
        ConvergenceView
        MenuBarView
        BrowserView
        NotificationView
    end

    subgraph "ViewModels"
        OrchestratorVM
        AgentVM
        ConversationVM
        AnnaVM
        NotificationVM
        BrowserVM
    end

    subgraph "Services"
        NotificationService
        KeyboardService
        BrowserService
        CalendarService
        AnnaService
    end

    subgraph "Core"
        Orchestrator
        AgentManager
        MessageBus
        CostController
    end

    ContentView --> OrchestratorVM
    SidebarView --> AgentVM
    ConversationView --> ConversationVM
    AgentCardView --> AgentVM
    ConvergenceView --> OrchestratorVM
    MenuBarView --> OrchestratorVM
    BrowserView --> BrowserVM
    NotificationView --> NotificationVM

    OrchestratorVM --> Orchestrator
    AgentVM --> AgentManager
    ConversationVM --> MessageBus
    AnnaVM --> AnnaService
    NotificationVM --> NotificationService
    BrowserVM --> BrowserService

    AnnaService --> CalendarService
    NotificationService --> Orchestrator
    BrowserService --> ConversationVM
```

### State Machine: Agent Lifecycle

```mermaid
stateDiagram-v2
    [*] --> Idle: App Launch

    Idle --> Selected: User selects agent
    Selected --> Idle: User deselects

    Selected --> Thinking: Query received
    Thinking --> Responding: Analysis complete
    Thinking --> Collaborating: Needs other agents

    Collaborating --> Thinking: Input received
    Collaborating --> Converging: All inputs ready

    Converging --> Responding: Synthesis complete

    Responding --> Idle: Response delivered
    Responding --> Thinking: Follow-up question

    Idle --> [*]: App Quit
```

**Visual Indicators per State:**
| State | Visual Effect | Animation |
|-------|---------------|-----------|
| **Idle** | Dim glass, gray ring | None |
| **Thinking** | Bright glass, blue ring | Soft pulse (1.5s) |
| **Collaborating** | Connected lines to other agents | Line drawing |
| **Converging** | Moving toward center | Position interpolation |
| **Responding** | Bright glass, streaming glow | Shimmer effect |

### Notification Flow

```mermaid
flowchart TD
    subgraph Triggers
        REMINDER[Reminder Due]
        BUDGET[Budget Alert]
        COMPLETE[Task Complete]
        CONV[Convergence Done]
        ERROR[Error Occurred]
    end

    subgraph NotificationService
        QUEUE[Notification Queue]
        PRIORITY[Priority Sorter]
        DEDUP[Deduplication]
        FORMAT[Format & Localize]
    end

    subgraph Delivery
        NC[macOS Notification Center]
        INAPP[In-App Toast]
        MENUBAR[Menu Bar Badge]
        SOUND[Sound Alert]
    end

    subgraph Actions
        DISMISS[Dismiss]
        OPEN[Open App]
        REPLY[Quick Reply]
        SNOOZE[Snooze]
    end

    REMINDER --> QUEUE
    BUDGET --> QUEUE
    COMPLETE --> QUEUE
    CONV --> QUEUE
    ERROR --> QUEUE

    QUEUE --> PRIORITY
    PRIORITY --> DEDUP
    DEDUP --> FORMAT

    FORMAT --> NC
    FORMAT --> INAPP
    FORMAT --> MENUBAR
    FORMAT --> SOUND

    NC --> DISMISS
    NC --> OPEN
    NC --> REPLY
    NC --> SNOOZE
```

---

## macOS Tahoe 26 & Liquid Glass Deep Dive

### What is Liquid Glass?

Apple's most significant design evolution since iOS 7. Key characteristics:

- **Translucent material** with real-time light refraction
- **Dynamic adaptation** to background content
- **Morphing transitions** between UI states
- **Depth hierarchy** - navigation floats above content

### Liquid Glass API Essentials

```swift
// Basic glass effect
Text("Hello, Glass!")
    .padding()
    .glassEffect()

// With customization
Button("Action") { }
    .glassEffect(.regular.tint(.blue).interactive())

// Morphing containers
GlassEffectContainer(spacing: 40) {
    ForEach(agents) { agent in
        AgentBadge(agent)
            .glassEffect()
            .glassEffectID(agent.id, in: namespace)
    }
}
```

### Material Variants

| Variant | Use Case | For Convergio |
|---------|----------|---------------|
| `.regular` | Default UI elements | Agent cards, toolbars, sidebars |
| `.clear` | Media-rich backgrounds | Conversation view, markdown content |
| `.identity` | Conditional disable | Accessibility fallback |

### Design Principles for Convergio

1. **Glass is for navigation, not content** - Agent responses stay solid
2. **Use morphing for state changes** - Agents thinking → converging
3. **Leverage interactive()** - All buttons must respond to touch
4. **Container-based layouts** - GlassEffectContainer for agent groups

---

## Advanced Features: Anna, Notifications, Browser

### Anna - Executive Assistant Services

Anna is the specialized agent that handles executive assistant tasks. In the native app, her capabilities expand significantly with native macOS integration.

#### Calendar Integration

```swift
// Services/CalendarService.swift
import EventKit

@MainActor
class CalendarService: ObservableObject {
    private let eventStore = EKEventStore()
    @Published var hasAccess = false

    func requestAccess() async throws {
        if #available(macOS 14.0, *) {
            hasAccess = try await eventStore.requestFullAccessToEvents()
        } else {
            hasAccess = try await eventStore.requestAccess(to: .event)
        }
    }

    func createMeeting(
        title: String,
        startDate: Date,
        duration: TimeInterval,
        attendees: [String],
        notes: String?
    ) async throws -> EKEvent {
        guard hasAccess else { throw CalendarError.noAccess }

        let event = EKEvent(eventStore: eventStore)
        event.title = title
        event.startDate = startDate
        event.endDate = startDate.addingTimeInterval(duration)
        event.notes = notes
        event.calendar = eventStore.defaultCalendarForNewEvents

        // Add attendees
        event.attendees = attendees.map { email in
            EKParticipant() // Note: Attendees require more complex handling
        }

        try eventStore.save(event, span: .thisEvent)
        return event
    }

    func getUpcomingEvents(days: Int = 7) async throws -> [EKEvent] {
        let startDate = Date()
        let endDate = Calendar.current.date(byAdding: .day, value: days, to: startDate)!

        let predicate = eventStore.predicateForEvents(
            withStart: startDate,
            end: endDate,
            calendars: nil
        )

        return eventStore.events(matching: predicate)
    }
}
```

#### Anna Service Architecture

```swift
// Services/AnnaService.swift
@MainActor
class AnnaService: ObservableObject {
    private let calendarService: CalendarService
    private let reminderService: ReminderService
    private let todoService: TodoService
    private let orchestrator: Orchestrator

    @Published var pendingReminders: [Reminder] = []
    @Published var todaysMeetings: [EKEvent] = []
    @Published var activeTodos: [Todo] = []

    // MARK: - Meeting Management

    func scheduleMeeting(from naturalLanguage: String) async throws -> EKEvent {
        // Use orchestrator to parse natural language
        let parsed = try await orchestrator.parseIntent(naturalLanguage)

        guard case .scheduleMeeting(let details) = parsed else {
            throw AnnaError.invalidIntent
        }

        return try await calendarService.createMeeting(
            title: details.title,
            startDate: details.date,
            duration: details.duration,
            attendees: details.attendees,
            notes: details.notes
        )
    }

    // MARK: - Reminder Management

    func setReminder(
        message: String,
        triggerDate: Date,
        repeating: RepeatInterval? = nil
    ) async throws -> Reminder {
        let reminder = Reminder(
            id: UUID(),
            message: message,
            triggerDate: triggerDate,
            repeating: repeating,
            createdAt: Date()
        )

        try await reminderService.schedule(reminder)
        pendingReminders.append(reminder)

        // Schedule notification
        await NotificationService.shared.scheduleReminder(reminder)

        return reminder
    }

    // MARK: - Todo Management

    func createTodo(
        title: String,
        priority: Priority,
        dueDate: Date?,
        project: String?
    ) async throws -> Todo {
        let todo = Todo(
            id: UUID(),
            title: title,
            priority: priority,
            dueDate: dueDate,
            project: project,
            status: .pending,
            createdAt: Date()
        )

        try await todoService.save(todo)
        activeTodos.append(todo)

        return todo
    }

    // MARK: - Daily Briefing

    func generateDailyBriefing() async throws -> DailyBriefing {
        async let meetings = calendarService.getUpcomingEvents(days: 1)
        async let todos = todoService.getDueTodos(by: Date())
        async let reminders = reminderService.getUpcoming(hours: 24)

        return DailyBriefing(
            date: Date(),
            meetings: try await meetings,
            todos: try await todos,
            reminders: try await reminders
        )
    }
}
```

#### Anna UI Components

```swift
// Views/Anna/AnnaCommandPalette.swift
struct AnnaCommandPalette: View {
    @EnvironmentObject var anna: AnnaService
    @State private var command = ""
    @State private var suggestions: [AnnaSuggestion] = []

    var body: some View {
        VStack(spacing: 0) {
            // Command input
            HStack {
                Image(systemName: "sparkles")
                    .foregroundStyle(.purple)

                TextField("Ask Anna...", text: $command)
                    .textFieldStyle(.plain)
                    .onSubmit { executeCommand() }

                if !command.isEmpty {
                    Button(action: executeCommand) {
                        Image(systemName: "arrow.right.circle.fill")
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding()
            .glassEffect(.regular.tint(.purple.opacity(0.2)))

            // Quick suggestions
            if command.isEmpty {
                VStack(alignment: .leading, spacing: 8) {
                    Text("Quick Actions")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .padding(.horizontal)

                    ForEach(AnnaSuggestion.quickActions) { suggestion in
                        Button {
                            command = suggestion.command
                            executeCommand()
                        } label: {
                            HStack {
                                Image(systemName: suggestion.icon)
                                Text(suggestion.title)
                                Spacer()
                            }
                            .padding(.horizontal)
                            .padding(.vertical, 8)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(.vertical)
            }

            // Parsed intent preview
            if let intent = anna.parsedIntent {
                IntentPreviewView(intent: intent)
                    .padding()
                    .glassEffect(.clear)
            }
        }
        .frame(width: 400)
    }

    private func executeCommand() {
        Task {
            try await anna.executeCommand(command)
            command = ""
        }
    }
}

// Quick action suggestions for Anna
struct AnnaSuggestion: Identifiable {
    let id = UUID()
    let icon: String
    let title: String
    let command: String

    static let quickActions: [AnnaSuggestion] = [
        AnnaSuggestion(icon: "calendar.badge.plus", title: "Schedule a meeting", command: "Schedule meeting with "),
        AnnaSuggestion(icon: "bell.badge", title: "Set a reminder", command: "Remind me to "),
        AnnaSuggestion(icon: "checklist", title: "Create a todo", command: "Add todo: "),
        AnnaSuggestion(icon: "sun.horizon", title: "Daily briefing", command: "What's on my agenda today?"),
        AnnaSuggestion(icon: "envelope", title: "Draft an email", command: "Draft email to "),
    ]
}
```

### Notification System

Complete notification management with macOS Notification Center integration.

#### Notification Service

```swift
// Services/NotificationService.swift
import UserNotifications

@MainActor
class NotificationService: ObservableObject {
    static let shared = NotificationService()

    @Published var pendingNotifications: [PendingNotification] = []
    @Published var notificationHistory: [DeliveredNotification] = []

    private let center = UNUserNotificationCenter.current()

    // MARK: - Setup

    func requestAuthorization() async throws -> Bool {
        try await center.requestAuthorization(options: [.alert, .sound, .badge])
    }

    // MARK: - Notification Types

    enum NotificationType: String {
        case reminder = "reminder"
        case budgetAlert = "budget"
        case convergenceComplete = "convergence"
        case agentMessage = "agent"
        case taskComplete = "task"
        case error = "error"

        var categoryIdentifier: String { rawValue }
    }

    // MARK: - Schedule Notifications

    func scheduleReminder(_ reminder: Reminder) async {
        let content = UNMutableNotificationContent()
        content.title = "Reminder from Anna"
        content.body = reminder.message
        content.sound = .default
        content.categoryIdentifier = NotificationType.reminder.categoryIdentifier

        // Add actions
        content.categoryIdentifier = "REMINDER_CATEGORY"

        let trigger = UNCalendarNotificationTrigger(
            dateMatching: Calendar.current.dateComponents(
                [.year, .month, .day, .hour, .minute],
                from: reminder.triggerDate
            ),
            repeats: reminder.repeating != nil
        )

        let request = UNNotificationRequest(
            identifier: reminder.id.uuidString,
            content: content,
            trigger: trigger
        )

        try? await center.add(request)
    }

    func sendBudgetAlert(current: Double, budget: Double) async {
        let percentage = (current / budget) * 100
        let content = UNMutableNotificationContent()
        content.title = "Budget Alert"
        content.body = String(format: "You've used %.0f%% of your budget ($%.2f / $%.2f)", percentage, current, budget)
        content.sound = .default
        content.categoryIdentifier = NotificationType.budgetAlert.categoryIdentifier

        // Immediate delivery
        let request = UNNotificationRequest(
            identifier: UUID().uuidString,
            content: content,
            trigger: nil
        )

        try? await center.add(request)
    }

    func sendConvergenceComplete(agentCount: Int, topic: String) async {
        let content = UNMutableNotificationContent()
        content.title = "Convergence Complete"
        content.body = "\(agentCount) agents have converged on: \(topic)"
        content.sound = .default
        content.categoryIdentifier = NotificationType.convergenceComplete.categoryIdentifier

        // Add "View" action
        content.categoryIdentifier = "CONVERGENCE_CATEGORY"

        let request = UNNotificationRequest(
            identifier: UUID().uuidString,
            content: content,
            trigger: nil
        )

        try? await center.add(request)
    }

    // MARK: - Register Categories with Actions

    func registerCategories() {
        // Reminder actions
        let snoozeAction = UNNotificationAction(
            identifier: "SNOOZE_ACTION",
            title: "Snooze 15 min",
            options: []
        )
        let doneAction = UNNotificationAction(
            identifier: "DONE_ACTION",
            title: "Mark Done",
            options: [.destructive]
        )
        let reminderCategory = UNNotificationCategory(
            identifier: "REMINDER_CATEGORY",
            actions: [snoozeAction, doneAction],
            intentIdentifiers: []
        )

        // Convergence actions
        let viewAction = UNNotificationAction(
            identifier: "VIEW_ACTION",
            title: "View Result",
            options: [.foreground]
        )
        let convergenceCategory = UNNotificationCategory(
            identifier: "CONVERGENCE_CATEGORY",
            actions: [viewAction],
            intentIdentifiers: []
        )

        center.setNotificationCategories([reminderCategory, convergenceCategory])
    }
}
```

#### In-App Toast Notifications

```swift
// Views/Components/ToastView.swift
struct ToastView: View {
    let notification: InAppNotification
    let onDismiss: () -> Void
    let onAction: ((String) -> Void)?

    @State private var isShowing = false

    var body: some View {
        HStack(spacing: 12) {
            // Icon
            Image(systemName: notification.icon)
                .font(.title2)
                .foregroundStyle(notification.iconColor)
                .frame(width: 32)

            // Content
            VStack(alignment: .leading, spacing: 2) {
                Text(notification.title)
                    .font(.headline)

                if let body = notification.body {
                    Text(body)
                        .font(.subheadline)
                        .foregroundStyle(.secondary)
                        .lineLimit(2)
                }
            }

            Spacer()

            // Actions
            if let actions = notification.actions {
                ForEach(actions, id: \.title) { action in
                    Button(action.title) {
                        onAction?(action.identifier)
                    }
                    .buttonStyle(.glass)
                }
            }

            // Dismiss
            Button {
                withAnimation(.easeOut(duration: 0.2)) {
                    isShowing = false
                }
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
                    onDismiss()
                }
            } label: {
                Image(systemName: "xmark")
                    .foregroundStyle(.secondary)
            }
            .buttonStyle(.plain)
        }
        .padding()
        .glassEffect(.regular.tint(notification.tintColor.opacity(0.2)))
        .frame(maxWidth: 400)
        .opacity(isShowing ? 1 : 0)
        .offset(y: isShowing ? 0 : -20)
        .onAppear {
            withAnimation(.spring(duration: 0.3)) {
                isShowing = true
            }

            // Auto-dismiss after delay
            if notification.autoDismiss {
                DispatchQueue.main.asyncAfter(deadline: .now() + notification.duration) {
                    withAnimation(.easeOut(duration: 0.2)) {
                        isShowing = false
                    }
                    DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
                        onDismiss()
                    }
                }
            }
        }
    }
}

// Toast container overlay
struct ToastContainer: View {
    @ObservedObject var notificationManager: InAppNotificationManager

    var body: some View {
        VStack(spacing: 8) {
            ForEach(notificationManager.activeToasts) { toast in
                ToastView(
                    notification: toast,
                    onDismiss: {
                        notificationManager.dismiss(toast)
                    },
                    onAction: { action in
                        notificationManager.handleAction(action, for: toast)
                    }
                )
                .transition(.asymmetric(
                    insertion: .move(edge: .top).combined(with: .opacity),
                    removal: .move(edge: .trailing).combined(with: .opacity)
                ))
            }
        }
        .padding()
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topTrailing)
    }
}
```

### In-App Browser

WebKit-based browser for research and content integration.

#### Browser Service

```swift
// Services/BrowserService.swift
import WebKit

@MainActor
class BrowserService: NSObject, ObservableObject {
    @Published var currentURL: URL?
    @Published var pageTitle: String = ""
    @Published var isLoading: Bool = false
    @Published var canGoBack: Bool = false
    @Published var canGoForward: Bool = false
    @Published var pageContent: String = ""

    private var webView: WKWebView?

    // MARK: - Configuration

    func createWebView() -> WKWebView {
        let config = WKWebViewConfiguration()

        // Enable JavaScript
        config.defaultWebpagePreferences.allowsContentJavaScript = true

        // User agent
        config.applicationNameForUserAgent = "Convergio/1.0"

        let webView = WKWebView(frame: .zero, configuration: config)
        webView.navigationDelegate = self
        webView.allowsBackForwardNavigationGestures = true

        self.webView = webView
        return webView
    }

    // MARK: - Navigation

    func load(url: URL) {
        webView?.load(URLRequest(url: url))
    }

    func load(urlString: String) {
        guard let url = URL(string: urlString) else { return }

        // Add https if missing
        if url.scheme == nil {
            load(url: URL(string: "https://\(urlString)")!)
        } else {
            load(url: url)
        }
    }

    func goBack() {
        webView?.goBack()
    }

    func goForward() {
        webView?.goForward()
    }

    func reload() {
        webView?.reload()
    }

    func stopLoading() {
        webView?.stopLoading()
    }

    // MARK: - Content Extraction

    func extractPageContent() async throws -> WebPageContent {
        guard let webView = webView else {
            throw BrowserError.noWebView
        }

        // Extract text content via JavaScript
        let textScript = """
            document.body.innerText
        """

        let titleScript = """
            document.title
        """

        let metaScript = """
            JSON.stringify({
                description: document.querySelector('meta[name="description"]')?.content || '',
                keywords: document.querySelector('meta[name="keywords"]')?.content || '',
                author: document.querySelector('meta[name="author"]')?.content || ''
            })
        """

        async let text = webView.evaluateJavaScript(textScript) as? String
        async let title = webView.evaluateJavaScript(titleScript) as? String
        async let metaJSON = webView.evaluateJavaScript(metaScript) as? String

        let meta = try? JSONDecoder().decode(
            PageMeta.self,
            from: (try await metaJSON ?? "{}").data(using: .utf8)!
        )

        return WebPageContent(
            url: currentURL!,
            title: try await title ?? "",
            text: try await text ?? "",
            meta: meta
        )
    }

    // MARK: - Screenshot

    func captureScreenshot() async throws -> NSImage {
        guard let webView = webView else {
            throw BrowserError.noWebView
        }

        let config = WKSnapshotConfiguration()
        return try await webView.takeSnapshot(configuration: config)
    }

    // MARK: - Send to Conversation

    func analyzeCurrentPage(with orchestrator: Orchestrator) async throws {
        let content = try await extractPageContent()

        let prompt = """
        Analyze this web page and provide insights:

        URL: \(content.url)
        Title: \(content.title)

        Content:
        \(content.text.prefix(10000))
        """

        try await orchestrator.send(prompt)
    }
}

extension BrowserService: WKNavigationDelegate {
    func webView(_ webView: WKWebView, didStartProvisionalNavigation navigation: WKNavigation!) {
        isLoading = true
    }

    func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
        isLoading = false
        currentURL = webView.url
        pageTitle = webView.title ?? ""
        canGoBack = webView.canGoBack
        canGoForward = webView.canGoForward
    }

    func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
        isLoading = false
    }
}
```

#### Browser View

```swift
// Views/Browser/BrowserView.swift
import SwiftUI
import WebKit

struct BrowserView: View {
    @StateObject private var browser = BrowserService()
    @EnvironmentObject var orchestrator: Orchestrator
    @State private var urlInput = ""
    @State private var showAnalysis = false

    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack(spacing: 12) {
                // Navigation buttons
                HStack(spacing: 4) {
                    Button {
                        browser.goBack()
                    } label: {
                        Image(systemName: "chevron.left")
                    }
                    .disabled(!browser.canGoBack)

                    Button {
                        browser.goForward()
                    } label: {
                        Image(systemName: "chevron.right")
                    }
                    .disabled(!browser.canGoForward)

                    Button {
                        browser.reload()
                    } label: {
                        Image(systemName: browser.isLoading ? "xmark" : "arrow.clockwise")
                    }
                }
                .buttonStyle(.plain)

                // URL bar
                HStack {
                    Image(systemName: "globe")
                        .foregroundStyle(.secondary)

                    TextField("Enter URL or search...", text: $urlInput)
                        .textFieldStyle(.plain)
                        .onSubmit {
                            browser.load(urlString: urlInput)
                        }

                    if browser.isLoading {
                        ProgressView()
                            .scaleEffect(0.7)
                    }
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 8)
                .glassEffect(.regular)

                // Actions
                HStack(spacing: 8) {
                    Button {
                        showAnalysis = true
                        Task {
                            try await browser.analyzeCurrentPage(with: orchestrator)
                        }
                    } label: {
                        Image(systemName: "sparkles")
                    }
                    .help("Analyze with AI")

                    Button {
                        Task {
                            if let image = try? await browser.captureScreenshot() {
                                // Copy to clipboard or save
                            }
                        }
                    } label: {
                        Image(systemName: "camera")
                    }
                    .help("Capture screenshot")

                    Button {
                        if let url = browser.currentURL {
                            NSPasteboard.general.clearContents()
                            NSPasteboard.general.setString(url.absoluteString, forType: .string)
                        }
                    } label: {
                        Image(systemName: "doc.on.doc")
                    }
                    .help("Copy URL")
                }
                .buttonStyle(.glass)
            }
            .padding()
            .glassEffect(.regular)

            // WebView
            WebViewRepresentable(browser: browser)
        }
        .onChange(of: browser.currentURL) { _, newURL in
            urlInput = newURL?.absoluteString ?? ""
        }
        .sheet(isPresented: $showAnalysis) {
            AnalysisResultView()
                .environmentObject(orchestrator)
        }
    }
}

// SwiftUI wrapper for WKWebView
struct WebViewRepresentable: NSViewRepresentable {
    @ObservedObject var browser: BrowserService

    func makeNSView(context: Context) -> WKWebView {
        browser.createWebView()
    }

    func updateNSView(_ nsView: WKWebView, context: Context) {
        // Updates handled by browser service
    }
}
```

#### Browser Integration with Conversation

```swift
// Views/Browser/BrowserSplitView.swift
struct BrowserSplitView: View {
    @State private var showBrowser = false
    @StateObject private var browser = BrowserService()

    var body: some View {
        HSplitView {
            // Main conversation
            ConversationView()
                .frame(minWidth: 400)

            // Browser panel (collapsible)
            if showBrowser {
                BrowserView()
                    .frame(minWidth: 400, idealWidth: 500)
                    .transition(.move(edge: .trailing))
            }
        }
        .toolbar {
            ToolbarItem(placement: .automatic) {
                Button {
                    withAnimation(.spring(duration: 0.3)) {
                        showBrowser.toggle()
                    }
                } label: {
                    Image(systemName: showBrowser ? "sidebar.trailing" : "globe")
                }
                .help(showBrowser ? "Hide browser" : "Show browser")
            }
        }
    }
}
```

### Notification Center View

```swift
// Views/Notifications/NotificationCenterView.swift
struct NotificationCenterView: View {
    @ObservedObject var notificationService: NotificationService
    @State private var filter: NotificationFilter = .all

    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Notifications")
                    .font(.headline)

                Spacer()

                Picker("Filter", selection: $filter) {
                    Text("All").tag(NotificationFilter.all)
                    Text("Reminders").tag(NotificationFilter.reminders)
                    Text("Alerts").tag(NotificationFilter.alerts)
                    Text("Tasks").tag(NotificationFilter.tasks)
                }
                .pickerStyle(.segmented)
                .frame(width: 200)

                Button("Clear All") {
                    notificationService.clearAll()
                }
                .buttonStyle(.plain)
                .foregroundStyle(.secondary)
            }
            .padding()

            Divider()

            // Notification list
            if filteredNotifications.isEmpty {
                ContentUnavailableView(
                    "No Notifications",
                    systemImage: "bell.slash",
                    description: Text("You're all caught up!")
                )
            } else {
                List(filteredNotifications) { notification in
                    NotificationRow(notification: notification)
                        .swipeActions(edge: .trailing) {
                            Button("Delete", role: .destructive) {
                                notificationService.delete(notification)
                            }
                        }
                        .swipeActions(edge: .leading) {
                            if notification.type == .reminder {
                                Button("Snooze") {
                                    notificationService.snooze(notification, minutes: 15)
                                }
                                .tint(.orange)
                            }
                        }
                }
                .listStyle(.inset)
            }
        }
    }

    private var filteredNotifications: [DeliveredNotification] {
        switch filter {
        case .all:
            return notificationService.notificationHistory
        case .reminders:
            return notificationService.notificationHistory.filter { $0.type == .reminder }
        case .alerts:
            return notificationService.notificationHistory.filter { $0.type == .budgetAlert }
        case .tasks:
            return notificationService.notificationHistory.filter { $0.type == .taskComplete }
        }
    }
}

struct NotificationRow: View {
    let notification: DeliveredNotification

    var body: some View {
        HStack(spacing: 12) {
            // Icon
            Image(systemName: notification.icon)
                .font(.title2)
                .foregroundStyle(notification.iconColor)
                .frame(width: 40, height: 40)
                .glassEffect(.regular.tint(notification.iconColor.opacity(0.2)), in: .circle)

            // Content
            VStack(alignment: .leading, spacing: 4) {
                HStack {
                    Text(notification.title)
                        .font(.headline)

                    Spacer()

                    Text(notification.timestamp, style: .relative)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }

                if let body = notification.body {
                    Text(body)
                        .font(.subheadline)
                        .foregroundStyle(.secondary)
                        .lineLimit(2)
                }
            }
        }
        .padding(.vertical, 4)
    }
}
```

---

## Architecture: Unified Repository Strategy

### Repository Structure (Proposed)

```
ConvergioCLI/
├── src/                          # Existing C source (UNCHANGED)
│   ├── orchestrator/
│   ├── providers/
│   ├── agents/
│   ├── memory/
│   └── ...
├── include/                      # C headers (UNCHANGED)
├── ConvergioCore/                # NEW: Static library wrapper
│   ├── Package.swift
│   ├── Sources/
│   │   ├── ConvergioCore/        # Swift wrapper for C library
│   │   │   ├── Core.swift        # Main orchestrator interface
│   │   │   ├── Agent.swift       # Agent protocol & types
│   │   │   ├── Provider.swift    # Provider abstraction
│   │   │   └── Message.swift     # Message types
│   │   └── CConvergio/           # C bridging target
│   │       ├── module.modulemap
│   │       └── shim.h
│   └── Tests/
├── ConvergioApp/                 # NEW: SwiftUI application
│   ├── ConvergioApp.xcodeproj
│   ├── ConvergioApp/
│   │   ├── App/
│   │   │   ├── ConvergioApp.swift
│   │   │   └── AppDelegate.swift
│   │   ├── Views/
│   │   │   ├── Main/
│   │   │   │   ├── ContentView.swift
│   │   │   │   ├── SidebarView.swift
│   │   │   │   └── ConversationView.swift
│   │   │   ├── Agents/
│   │   │   │   ├── AgentGridView.swift
│   │   │   │   ├── AgentCardView.swift
│   │   │   │   └── ConvergenceView.swift
│   │   │   ├── MenuBar/
│   │   │   │   ├── MenuBarView.swift
│   │   │   │   └── QuickAccessView.swift
│   │   │   └── Components/
│   │   │       ├── GlassButton.swift
│   │   │       ├── StreamingTextView.swift
│   │   │       └── CostBadge.swift
│   │   ├── ViewModels/
│   │   │   ├── OrchestratorViewModel.swift
│   │   │   ├── AgentViewModel.swift
│   │   │   └── ConversationViewModel.swift
│   │   ├── Services/
│   │   │   ├── NotificationService.swift
│   │   │   └── KeyboardShortcutService.swift
│   │   └── Resources/
│   │       ├── Assets.xcassets
│   │       └── Localizable.strings
│   └── ConvergioAppTests/
├── Makefile                      # Updated: build cli OR app
├── CMakeLists.txt                # Existing (for CLI)
└── README.md
```

### Build Strategy

```makefile
# Makefile additions
.PHONY: cli app core

cli:                              # Existing CLI build
	@mkdir -p build && cd build && cmake .. && make

core:                             # Build libConvergioCore.a
	@cd ConvergioCore && swift build -c release

app:                              # Build native app
	@xcodebuild -project ConvergioApp/ConvergioApp.xcodeproj \
	            -scheme ConvergioApp \
	            -configuration Release \
	            -destination 'platform=macOS'

all: cli core app
```

### C-to-Swift Bridge Design

```swift
// ConvergioCore/Sources/CConvergio/shim.h
#ifndef CONVERGIO_SHIM_H
#define CONVERGIO_SHIM_H

#include "orchestrator.h"
#include "nous.h"
#include "provider.h"
#include "persistence.h"

// Opaque type wrappers for Swift
typedef struct OrchestratorHandle OrchestratorHandle;
OrchestratorHandle* convergio_create(void);
void convergio_destroy(OrchestratorHandle* handle);
int convergio_send_message(OrchestratorHandle* handle, const char* message);
const char* convergio_get_response(OrchestratorHandle* handle);

// Agent enumeration
int convergio_get_agent_count(OrchestratorHandle* handle);
const char* convergio_get_agent_name(OrchestratorHandle* handle, int index);
int convergio_get_agent_state(OrchestratorHandle* handle, int index);

// Cost tracking
double convergio_get_session_cost(OrchestratorHandle* handle);
double convergio_get_budget_remaining(OrchestratorHandle* handle);

#endif
```

```swift
// ConvergioCore/Sources/ConvergioCore/Core.swift
import CConvergio

@MainActor
public final class Orchestrator: ObservableObject {
    private var handle: OpaquePointer?

    @Published public var agents: [Agent] = []
    @Published public var messages: [Message] = []
    @Published public var sessionCost: Double = 0
    @Published public var isProcessing: Bool = false

    public init() {
        handle = convergio_create()
        loadAgents()
    }

    deinit {
        if let h = handle {
            convergio_destroy(h)
        }
    }

    public func send(_ message: String) async throws -> AsyncStream<String> {
        isProcessing = true
        defer { isProcessing = false }

        return AsyncStream { continuation in
            // Callback-based streaming from C
            convergio_send_message_streamed(handle, message) { chunk in
                continuation.yield(String(cString: chunk))
            }
            continuation.finish()
        }
    }
}
```

---

## Vertical Market Solutions

### Vision: One Platform, Infinite Possibilities

Convergio's architecture enables **vertical market customization** through agent persona replacement. The core orchestration engine remains unchanged - only the agent definitions and UI themes adapt to each industry.

### Vertical Architecture

```mermaid
graph TB
    subgraph "Core Platform (Unchanged)"
        ENGINE[Orchestration Engine]
        PROVIDERS[AI Providers]
        PERSISTENCE[Persistence Layer]
        UI_FRAMEWORK[UI Framework]
    end

    subgraph "Vertical Layer (Customizable)"
        AGENTS[Agent Personas]
        THEMES[Visual Themes]
        TERMINOLOGY[Domain Terminology]
        WORKFLOWS[Industry Workflows]
    end

    subgraph "Vertical Products"
        EDU[Convergio EDU]
        HEALTH[Convergio Health]
        MARKETING[Convergio Marketing]
        LOGISTICS[Convergio Logistics]
        RESEARCH[Convergio Research]
        LEGAL[Convergio Legal]
        FINANCE[Convergio Finance]
    end

    ENGINE --> AGENTS
    PROVIDERS --> AGENTS
    PERSISTENCE --> WORKFLOWS
    UI_FRAMEWORK --> THEMES

    AGENTS --> EDU
    AGENTS --> HEALTH
    AGENTS --> MARKETING
    AGENTS --> LOGISTICS
    AGENTS --> RESEARCH
    AGENTS --> LEGAL
    AGENTS --> FINANCE
```

### Vertical Definitions

#### 1. Convergio EDU (Education)

**Target:** Schools, universities, tutoring centers, homeschooling families

**Agent Personas:**
| Original Agent | EDU Persona | Role |
|----------------|-------------|------|
| Ali (Chief of Staff) | **Prof. Sofia** (Dean) | Coordinates learning team |
| Angela (Data Analyst) | **Dr. Marco** (Math Teacher) | Mathematics, statistics, logic |
| Amy (CFO) | **Prof. Elena** (Science Teacher) | Physics, chemistry, biology |
| Matteo (Strategy) | **Dr. Luca** (History Teacher) | History, geography, civics |
| Sofia (Marketing) | **Prof. Anna** (Language Arts) | Writing, literature, communication |
| Jony (Creative Director) | **Maestro Giorgio** (Art Teacher) | Visual arts, music, creativity |
| Oliver (PM) | **Coach Roberto** (PE Teacher) | Physical education, health, wellness |

**Special Features:**
- **Adaptive Learning Mode** - Adjusts explanation complexity based on student level
- **Progress Tracking** - Visual learning progress with achievements
- **Parent Dashboard** - Summary reports for parents/guardians
- **Homework Helper** - Step-by-step problem solving (shows work, not just answers)
- **Study Planner** - Integrated with Anna for study schedules
- **Dyslexia Mode** - OpenDyslexic font, colored overlays, text-to-speech
- **Dyscalculia Support** - Visual math representations, step highlighting

**UI Theme:**
- Warm, encouraging colors (soft blues, greens)
- Achievement badges and progress bars
- Simplified navigation for younger users
- Parental controls integration

```swift
// Example: EDU vertical configuration
struct ConvergioEDUConfig: VerticalConfig {
    let name = "Convergio EDU"
    let icon = "graduationcap.fill"
    let primaryColor = Color(hex: "4A90D9")  // Calming blue

    let agentMapping: [String: AgentPersona] = [
        "ali": AgentPersona(
            name: "Prof. Sofia",
            title: "Dean of Learning",
            icon: "person.bust",
            systemPrompt: """
                You are Prof. Sofia, the Dean of Learning at Convergio Academy.
                You coordinate a team of expert teachers to help students learn.
                Always be encouraging, patient, and adapt to the student's level.
                Never give direct answers - guide students to discover solutions.
                Celebrate small victories and normalize making mistakes as learning.
                """
        ),
        // ... more mappings
    ]

    let features: [VerticalFeature] = [
        .progressTracking,
        .parentDashboard,
        .adaptiveLearning,
        .homeworkHelper,
        .studyPlanner
    ]
}
```

#### 2. Convergio Health (Healthcare)

**Target:** Hospitals, clinics, healthcare administrators, medical researchers

**Agent Personas:**
| Original Agent | Health Persona | Role |
|----------------|----------------|------|
| Ali | **Dr. Chief** (CMO) | Coordinates medical team |
| Angela | **Dr. Data** (Clinical Analyst) | Medical data analysis, statistics |
| Amy | **Dr. Finance** (Healthcare CFO) | Medical billing, budgets |
| Elena (Legal) | **Dr. Compliance** | HIPAA, medical regulations |
| Guardian | **Dr. Security** | PHI protection, audit trails |

**Special Features:**
- **HIPAA Compliance Mode** - All data handling follows HIPAA guidelines
- **Medical Terminology** - Understands and uses proper medical terms
- **Clinical Decision Support** - Evidence-based recommendations with citations
- **Patient Communication** - Helps draft clear patient communications
- **Research Assistant** - Literature review, study design support
- **Audit Trail** - Complete logging for compliance

**Critical Requirements:**
- No diagnosis or treatment recommendations without "educational purposes only" disclaimer
- All outputs include "consult healthcare provider" notice
- PHI handling strictly controlled
- Integration with medical knowledge bases (with proper licensing)

#### 3. Convergio Marketing

**Target:** Marketing agencies, brand managers, content creators

**Agent Personas:**
| Original Agent | Marketing Persona | Role |
|----------------|-------------------|------|
| Ali | **Chief Brand Officer** | Brand strategy coordination |
| Sofia | **Creative Director** | Campaign concepts, messaging |
| Angela | **Analytics Lead** | Performance metrics, ROI |
| Riccardo | **Content Strategist** | Storytelling, content calendar |
| Sara | **UX Researcher** | Audience insights, personas |

**Special Features:**
- **Campaign Builder** - End-to-end campaign planning
- **Content Calendar** - Integrated scheduling with Anna
- **A/B Test Designer** - Experiment design and analysis
- **Brand Voice Keeper** - Maintains consistent brand tone
- **Competitor Tracker** - Market intelligence summaries
- **Social Listening** - Trend analysis and insights

#### 4. Convergio Logistics

**Target:** Supply chain managers, warehouse operators, transportation companies

**Agent Personas:**
| Original Agent | Logistics Persona | Role |
|----------------|-------------------|------|
| Ali | **Operations Director** | End-to-end coordination |
| Angela | **Demand Planner** | Forecasting, inventory optimization |
| Enrico | **Process Engineer** | Workflow optimization |
| Wanda | **Route Optimizer** | Transportation planning |
| Amy | **Cost Controller** | Logistics budget management |

**Special Features:**
- **Route Optimization** - Multi-stop planning with constraints
- **Inventory Forecasting** - Demand prediction models
- **Supplier Scorecards** - Vendor performance tracking
- **Capacity Planning** - Resource allocation optimization
- **Risk Assessment** - Supply chain vulnerability analysis

#### 5. Convergio Research

**Target:** Academic researchers, R&D teams, think tanks

**Agent Personas:**
| Original Agent | Research Persona | Role |
|----------------|------------------|------|
| Ali | **Principal Investigator** | Research coordination |
| Angela | **Statistician** | Research methodology, analysis |
| Socrates | **Critical Reviewer** | Hypothesis testing, peer review |
| Marcus | **Literature Expert** | Systematic reviews, citations |
| Elena | **Ethics Advisor** | IRB compliance, research ethics |

**Special Features:**
- **Literature Review Assistant** - Systematic search and synthesis
- **Methodology Advisor** - Study design recommendations
- **Statistical Consultant** - Analysis approach selection
- **Citation Manager** - Reference organization
- **Grant Writing Support** - Proposal structure and language
- **Peer Review Prep** - Anticipate reviewer questions

#### 6. Convergio Legal

**Target:** Law firms, corporate legal departments, compliance teams

**Agent Personas:**
| Original Agent | Legal Persona | Role |
|----------------|---------------|------|
| Ali | **Managing Partner** | Case coordination |
| Elena | **Senior Counsel** | Legal analysis, compliance |
| Angela | **Paralegal Analyst** | Document review, research |
| Matteo | **Strategy Counsel** | Case strategy, negotiations |
| Guardian | **Compliance Officer** | Regulatory monitoring |

#### 7. Convergio Finance

**Target:** Investment firms, corporate finance, financial advisors

**Agent Personas:**
| Original Agent | Finance Persona | Role |
|----------------|-----------------|------|
| Ali | **Chief Investment Officer** | Portfolio coordination |
| Amy | **CFO** | Financial planning, analysis |
| Angela | **Quantitative Analyst** | Financial modeling, risk |
| Michael | **Investment Analyst** | Due diligence, valuations |
| Elena | **Compliance Officer** | Regulatory, fiduciary duties |

### Vertical Implementation Strategy

```mermaid
flowchart LR
    subgraph "Phase 1: Core Platform"
        A[Base Convergio] --> B[Vertical Framework]
        B --> C[Agent Persona System]
        C --> D[Theme Engine]
    end

    subgraph "Phase 2: First Verticals"
        D --> E[Convergio EDU]
        D --> F[Convergio Marketing]
    end

    subgraph "Phase 3: Expansion"
        E --> G[Convergio Health]
        F --> H[Convergio Research]
        G --> I[Convergio Legal]
        H --> J[Convergio Finance]
        I --> K[Convergio Logistics]
    end

    subgraph "Phase 4: Marketplace"
        K --> L[Custom Verticals]
        L --> M[Partner Ecosystem]
    end
```

### Technical Implementation

```swift
// Vertical configuration protocol
protocol VerticalConfig {
    var name: String { get }
    var identifier: String { get }
    var icon: String { get }
    var primaryColor: Color { get }
    var agentMapping: [String: AgentPersona] { get }
    var features: [VerticalFeature] { get }
    var terminology: [String: String] { get }
    var disclaimers: [String] { get }
    var accessibilityOverrides: AccessibilityConfig? { get }
}

// Agent persona definition
struct AgentPersona {
    let name: String
    let title: String
    let icon: String
    let systemPrompt: String
    let voiceProfile: VoiceProfile?  // For TTS
    let specializations: [String]
}

// Vertical manager
@MainActor
class VerticalManager: ObservableObject {
    @Published var activeVertical: VerticalConfig = DefaultConvergioConfig()
    @Published var availableVerticals: [VerticalConfig] = []

    func switchVertical(to vertical: VerticalConfig) {
        // Update agent personas
        orchestrator.reloadAgents(with: vertical.agentMapping)

        // Update UI theme
        ThemeManager.shared.apply(vertical.primaryColor)

        // Update terminology
        TerminologyManager.shared.load(vertical.terminology)

        // Apply accessibility overrides
        if let a11y = vertical.accessibilityOverrides {
            AccessibilityManager.shared.apply(a11y)
        }

        activeVertical = vertical
    }
}
```

### Business Model for Verticals

| Tier | Verticals Included | Price Point |
|------|-------------------|-------------|
| **Convergio Core** | Base (54 agents) | Free / $9.99/mo |
| **Convergio Pro** | Core + 2 verticals | $29.99/mo |
| **Convergio Enterprise** | All verticals + custom | Contact sales |
| **Vertical Add-ons** | Individual verticals | $14.99/mo each |

---

## Accessibility & Inclusive Design

> **Principle:** Convergio must be usable by EVERYONE, regardless of ability. Accessibility is not a feature - it's a fundamental requirement.

### Accessibility Standards Compliance

| Standard | Compliance Level | Status |
|----------|------------------|--------|
| **WCAG 2.2** | AAA | Target |
| **Section 508** | Full | Required |
| **EN 301 549** | Full | Required (EU) |
| **Apple Accessibility Guidelines** | Full | Required |
| **ADA Compliance** | Full | Required |

### Accessibility Architecture

```mermaid
graph TB
    subgraph "Input Methods"
        KEYBOARD[Full Keyboard Navigation]
        VOICE[Voice Control]
        SWITCH[Switch Control]
        EYETRACK[Eye Tracking]
        SIRI[Siri Shortcuts]
    end

    subgraph "Output Methods"
        VOICEOVER[VoiceOver Screen Reader]
        BRAILLE[Braille Display Support]
        TTS[Text-to-Speech]
        HAPTIC[Haptic Feedback]
        VISUAL[Visual Indicators]
    end

    subgraph "Cognitive Accessibility"
        DYSLEXIA[Dyslexia Support]
        DYSCALCULIA[Dyscalculia Support]
        ADHD[Focus Mode]
        AUTISM[Sensory Settings]
        SIMPLIFY[Simplified Mode]
    end

    subgraph "Motor Accessibility"
        LARGETARGETS[Large Touch Targets]
        DWELLCLICK[Dwell Click]
        GESTURES[Custom Gestures]
        TIMING[Extended Timing]
    end

    subgraph "Convergio Core"
        ENGINE[Accessibility Engine]
    end

    KEYBOARD --> ENGINE
    VOICE --> ENGINE
    SWITCH --> ENGINE
    EYETRACK --> ENGINE
    SIRI --> ENGINE

    ENGINE --> VOICEOVER
    ENGINE --> BRAILLE
    ENGINE --> TTS
    ENGINE --> HAPTIC
    ENGINE --> VISUAL

    ENGINE --> DYSLEXIA
    ENGINE --> DYSCALCULIA
    ENGINE --> ADHD
    ENGINE --> AUTISM
    ENGINE --> SIMPLIFY

    ENGINE --> LARGETARGETS
    ENGINE --> DWELLCLICK
    ENGINE --> GESTURES
    ENGINE --> TIMING
```

### Visual Accessibility

#### Color & Contrast

```swift
// Accessibility color system
struct AccessibleColors {
    // Minimum contrast ratios (WCAG AAA)
    static let minContrastNormal: CGFloat = 7.0  // Normal text
    static let minContrastLarge: CGFloat = 4.5   // Large text (18pt+)

    // High contrast theme
    static let highContrastBackground = Color.black
    static let highContrastForeground = Color.white
    static let highContrastAccent = Color.yellow

    // Color blind safe palette
    static let colorBlindSafe: [Color] = [
        Color(hex: "0077BB"),  // Blue
        Color(hex: "EE7733"),  // Orange
        Color(hex: "009988"),  // Teal
        Color(hex: "CC3311"),  // Red
        Color(hex: "33BBEE"),  // Cyan
        Color(hex: "EE3377"),  // Magenta
        Color(hex: "BBBBBB"),  // Gray
    ]

    // Never rely on color alone
    static func statusIndicator(_ status: AgentStatus) -> some View {
        HStack(spacing: 4) {
            // Shape distinguishes status (not just color)
            switch status {
            case .idle:
                Circle()
                    .stroke(Color.gray, lineWidth: 2)
            case .thinking:
                Circle()
                    .fill(Color.blue)
                    .overlay(PulseAnimation())
            case .active:
                Circle()
                    .fill(Color.green)
            case .error:
                Image(systemName: "exclamationmark.triangle.fill")
                    .foregroundColor(.red)
            }

            // Text label always present
            Text(status.localizedDescription)
                .accessibilityLabel(status.accessibilityDescription)
        }
    }
}
```

#### Typography & Readability

```swift
// Dyslexia-friendly typography
struct DyslexiaFriendlyText: ViewModifier {
    @Environment(\.accessibilityDyslexiaFriendly) var dyslexiaMode

    func body(content: Content) -> some View {
        content
            .font(dyslexiaMode ? .custom("OpenDyslexic", size: 16) : .body)
            .lineSpacing(dyslexiaMode ? 8 : 4)
            .tracking(dyslexiaMode ? 1.2 : 0)
            .foregroundColor(dyslexiaMode ? Color(hex: "1A1A2E") : .primary)
    }
}

// Reading ruler for focus
struct ReadingRuler: View {
    @Binding var position: CGFloat
    let lineHeight: CGFloat = 24

    var body: some View {
        GeometryReader { geo in
            VStack(spacing: 0) {
                // Dimmed area above
                Color.black.opacity(0.5)
                    .frame(height: position)

                // Clear reading line
                Color.clear
                    .frame(height: lineHeight * 2)
                    .border(Color.yellow, width: 2)

                // Dimmed area below
                Color.black.opacity(0.5)
            }
            .gesture(
                DragGesture()
                    .onChanged { value in
                        position = value.location.y
                    }
            )
        }
        .allowsHitTesting(true)
        .accessibilityLabel("Reading guide. Drag to move.")
    }
}

// Color overlay for visual stress reduction
struct ColorOverlay: View {
    @AppStorage("overlayColor") var overlayColor: String = "none"
    @AppStorage("overlayOpacity") var overlayOpacity: Double = 0.2

    var body: some View {
        if overlayColor != "none" {
            Rectangle()
                .fill(colorForName(overlayColor))
                .opacity(overlayOpacity)
                .allowsHitTesting(false)
        }
    }

    private func colorForName(_ name: String) -> Color {
        switch name {
        case "yellow": return .yellow
        case "blue": return .blue
        case "pink": return .pink
        case "green": return .green
        case "peach": return Color(hex: "FFDAB9")
        default: return .clear
        }
    }
}
```

### Cognitive Accessibility

#### Dyslexia Support

```swift
struct DyslexiaSettings: View {
    @AppStorage("dyslexiaFontEnabled") var dyslexiaFont = false
    @AppStorage("lineSpacingMultiplier") var lineSpacing = 1.5
    @AppStorage("letterSpacingMultiplier") var letterSpacing = 1.0
    @AppStorage("syllableHighlighting") var syllableHighlight = false
    @AppStorage("colorOverlay") var colorOverlay = "none"
    @AppStorage("readingRuler") var readingRuler = false

    var body: some View {
        Form {
            Section("Typography") {
                Toggle("OpenDyslexic Font", isOn: $dyslexiaFont)
                    .accessibilityHint("Uses a font designed for readers with dyslexia")

                Slider(value: $lineSpacing, in: 1.0...3.0, step: 0.25) {
                    Text("Line Spacing: \(lineSpacing, specifier: "%.2f")x")
                }

                Slider(value: $letterSpacing, in: 0.5...2.0, step: 0.1) {
                    Text("Letter Spacing: \(letterSpacing, specifier: "%.1f")x")
                }
            }

            Section("Reading Aids") {
                Toggle("Syllable Highlighting", isOn: $syllableHighlight)
                    .accessibilityHint("Highlights syllables in different colors to aid reading")

                Toggle("Reading Ruler", isOn: $readingRuler)
                    .accessibilityHint("Shows a movable guide to focus on one line at a time")

                Picker("Color Overlay", selection: $colorOverlay) {
                    Text("None").tag("none")
                    Text("Yellow").tag("yellow")
                    Text("Blue").tag("blue")
                    Text("Pink").tag("pink")
                    Text("Green").tag("green")
                    Text("Peach").tag("peach")
                }
                .accessibilityHint("Applies a colored tint to reduce visual stress")
            }

            Section("Text-to-Speech") {
                NavigationLink("Configure Voice") {
                    VoiceSettingsView()
                }

                Toggle("Auto-read Responses", isOn: .constant(false))
                    .accessibilityHint("Automatically reads agent responses aloud")
            }
        }
    }
}
```

#### Dyscalculia Support

```swift
struct DyscalculiaSettings: View {
    @AppStorage("visualNumbers") var visualNumbers = false
    @AppStorage("numberLine") var numberLine = false
    @AppStorage("stepByStepMath") var stepByStep = true
    @AppStorage("mathAnxietyMode") var mathAnxietyMode = false

    var body: some View {
        Form {
            Section("Number Representation") {
                Toggle("Visual Number Representations", isOn: $visualNumbers)
                    .accessibilityHint("Shows numbers as visual blocks or dots")

                Toggle("Number Line Helper", isOn: $numberLine)
                    .accessibilityHint("Displays a number line for context")
            }

            Section("Calculation Support") {
                Toggle("Step-by-Step Solutions", isOn: $stepByStep)
                    .accessibilityHint("Shows each step of calculations clearly")

                Toggle("Math Anxiety Mode", isOn: $mathAnxietyMode)
                    .accessibilityHint("Uses encouraging language and removes time pressure")
            }
        }
    }
}

// Visual number representation
struct VisualNumber: View {
    let value: Int
    let maxDots = 10

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            // Numeric representation
            Text("\(value)")
                .font(.title)
                .accessibilityLabel("The number \(value)")

            // Visual dot representation (for small numbers)
            if value <= maxDots * maxDots {
                LazyVGrid(columns: Array(repeating: GridItem(.fixed(12)), count: min(value, maxDots))) {
                    ForEach(0..<value, id: \.self) { _ in
                        Circle()
                            .fill(Color.blue)
                            .frame(width: 10, height: 10)
                    }
                }
                .accessibilityHidden(true)
            }

            // Number line context
            NumberLineView(value: value, range: 0...max(value + 5, 20))
        }
    }
}
```

#### ADHD & Focus Support

```swift
struct FocusModeSettings: View {
    @AppStorage("focusModeEnabled") var focusMode = false
    @AppStorage("reducedAnimations") var reducedAnimations = false
    @AppStorage("minimalistUI") var minimalistUI = false
    @AppStorage("taskTimer") var taskTimer = false
    @AppStorage("breakReminders") var breakReminders = false

    var body: some View {
        Form {
            Section("Focus Aids") {
                Toggle("Focus Mode", isOn: $focusMode)
                    .accessibilityHint("Hides non-essential UI elements")

                Toggle("Minimalist Interface", isOn: $minimalistUI)
                    .accessibilityHint("Shows only the most essential controls")

                Toggle("Reduce Animations", isOn: $reducedAnimations)
                    .accessibilityHint("Minimizes motion and transitions")
            }

            Section("Time Management") {
                Toggle("Task Timer", isOn: $taskTimer)
                    .accessibilityHint("Shows a timer for the current task")

                Toggle("Break Reminders", isOn: $breakReminders)
                    .accessibilityHint("Reminds you to take breaks")

                if breakReminders {
                    Stepper("Break every 25 minutes", value: .constant(25), in: 10...60, step: 5)
                }
            }
        }
    }
}

// Focus mode modifier
struct FocusModeModifier: ViewModifier {
    @Environment(\.accessibilityFocusMode) var focusMode

    func body(content: Content) -> some View {
        content
            .overlay {
                if focusMode {
                    // Dim everything except focused element
                    FocusDimOverlay()
                }
            }
    }
}
```

#### Autism Spectrum & Sensory Settings

```swift
struct SensorySettings: View {
    @AppStorage("reduceVisualClutter") var reduceClutter = false
    @AppStorage("predictableTransitions") var predictableTransitions = true
    @AppStorage("soundsEnabled") var soundsEnabled = true
    @AppStorage("hapticFeedback") var hapticFeedback = true
    @AppStorage("warningBeforeChanges") var warningBeforeChanges = true

    var body: some View {
        Form {
            Section("Visual") {
                Toggle("Reduce Visual Clutter", isOn: $reduceClutter)
                    .accessibilityHint("Simplifies the interface by hiding decorative elements")

                Toggle("Predictable Transitions", isOn: $predictableTransitions)
                    .accessibilityHint("Uses consistent, predictable animations")
            }

            Section("Audio & Haptic") {
                Toggle("Sound Effects", isOn: $soundsEnabled)

                Toggle("Haptic Feedback", isOn: $hapticFeedback)
            }

            Section("Predictability") {
                Toggle("Warn Before Major Changes", isOn: $warningBeforeChanges)
                    .accessibilityHint("Shows a confirmation before changing screens or modes")
            }
        }
    }
}
```

### Motor Accessibility

#### Large Touch Targets

```swift
// Minimum touch target: 44x44 points (Apple guideline)
// Accessible target: 48x48 points minimum
struct AccessibleButton<Content: View>: View {
    let action: () -> Void
    @ViewBuilder let content: () -> Content

    @Environment(\.accessibilityLargeTouchTargets) var largeTouchTargets

    var body: some View {
        Button(action: action) {
            content()
                .frame(
                    minWidth: largeTouchTargets ? 56 : 44,
                    minHeight: largeTouchTargets ? 56 : 44
                )
                .contentShape(Rectangle())
        }
    }
}

// Spacing between interactive elements
struct AccessibleSpacing {
    static func minimum(_ base: CGFloat = 8) -> CGFloat {
        let largeTouchTargets = UserDefaults.standard.bool(forKey: "accessibilityLargeTouchTargets")
        return largeTouchTargets ? base * 1.5 : base
    }
}
```

#### Switch Control & Dwell Click

```swift
// Full keyboard navigation
struct KeyboardNavigable: ViewModifier {
    @FocusState private var isFocused: Bool

    func body(content: Content) -> some View {
        content
            .focusable()
            .focused($isFocused)
            .overlay {
                if isFocused {
                    RoundedRectangle(cornerRadius: 8)
                        .stroke(Color.accentColor, lineWidth: 3)
                }
            }
            .accessibilityAddTraits(.isButton)
    }
}

// Dwell click support
struct DwellClickModifier: ViewModifier {
    @AppStorage("dwellClickEnabled") var dwellEnabled = false
    @AppStorage("dwellClickDuration") var dwellDuration = 1.0

    @State private var hoverTimer: Timer?
    @State private var hoverProgress: Double = 0

    let action: () -> Void

    func body(content: Content) -> some View {
        content
            .overlay {
                if dwellEnabled && hoverProgress > 0 {
                    CircularProgressView(progress: hoverProgress)
                        .frame(width: 30, height: 30)
                }
            }
            .onHover { hovering in
                if dwellEnabled {
                    if hovering {
                        startDwellTimer()
                    } else {
                        cancelDwellTimer()
                    }
                }
            }
    }

    private func startDwellTimer() {
        hoverProgress = 0
        hoverTimer = Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { _ in
            hoverProgress += 0.05 / dwellDuration
            if hoverProgress >= 1.0 {
                action()
                cancelDwellTimer()
            }
        }
    }

    private func cancelDwellTimer() {
        hoverTimer?.invalidate()
        hoverTimer = nil
        hoverProgress = 0
    }
}
```

### Voice Control & Screen Reader

#### VoiceOver Optimization

```swift
// Comprehensive VoiceOver support
struct AgentCardAccessible: View {
    let agent: Agent

    var body: some View {
        AgentCardView(agent: agent)
            .accessibilityElement(children: .combine)
            .accessibilityLabel(agentAccessibilityLabel)
            .accessibilityValue(agentAccessibilityValue)
            .accessibilityHint(agentAccessibilityHint)
            .accessibilityActions {
                Button("Select \(agent.name)") {
                    // Select agent
                }
                Button("View \(agent.name)'s details") {
                    // Show details
                }
                Button("Ask \(agent.name) directly") {
                    // Direct message
                }
            }
    }

    private var agentAccessibilityLabel: String {
        "\(agent.name), \(agent.role)"
    }

    private var agentAccessibilityValue: String {
        switch agent.state {
        case .idle: return "Available"
        case .thinking: return "Currently thinking"
        case .responding: return "Responding"
        case .collaborating: return "Collaborating with other agents"
        }
    }

    private var agentAccessibilityHint: String {
        "Double tap to select. Swipe up or down for more actions."
    }
}

// Announce important changes
func announceToVoiceOver(_ message: String, priority: UIAccessibilityNotification = .announcement) {
    UIAccessibility.post(notification: priority, argument: message)
}

// Example usage
func onConvergenceComplete() {
    announceToVoiceOver("Convergence complete. \(activeAgents.count) agents have synthesized a response.", priority: .announcement)
}
```

#### Voice Control Commands

```swift
// Custom voice control commands
struct VoiceCommands {
    static func register() {
        // Navigation
        UIApplication.shared.accessibilityCustomActions = [
            UIAccessibilityCustomAction(name: "Go to conversation") { _ in
                navigateTo(.conversation)
                return true
            },
            UIAccessibilityCustomAction(name: "Go to agents") { _ in
                navigateTo(.agents)
                return true
            },
            UIAccessibilityCustomAction(name: "Open settings") { _ in
                navigateTo(.settings)
                return true
            },

            // Actions
            UIAccessibilityCustomAction(name: "Send message") { _ in
                sendCurrentMessage()
                return true
            },
            UIAccessibilityCustomAction(name: "Clear conversation") { _ in
                clearConversation()
                return true
            },
            UIAccessibilityCustomAction(name: "Read last response") { _ in
                readLastResponse()
                return true
            },
        ]
    }
}
```

### Accessibility Settings Hub

```swift
struct AccessibilitySettingsView: View {
    var body: some View {
        NavigationStack {
            List {
                Section {
                    NavigationLink {
                        VisionSettingsView()
                    } label: {
                        Label("Vision", systemImage: "eye")
                    }

                    NavigationLink {
                        DyslexiaSettings()
                    } label: {
                        Label("Dyslexia Support", systemImage: "textformat")
                    }

                    NavigationLink {
                        DyscalculiaSettings()
                    } label: {
                        Label("Dyscalculia Support", systemImage: "number")
                    }
                }

                Section {
                    NavigationLink {
                        MotorSettingsView()
                    } label: {
                        Label("Motor & Mobility", systemImage: "hand.raised")
                    }

                    NavigationLink {
                        VoiceControlSettingsView()
                    } label: {
                        Label("Voice Control", systemImage: "mic")
                    }
                }

                Section {
                    NavigationLink {
                        FocusModeSettings()
                    } label: {
                        Label("Focus & Attention", systemImage: "eye.trianglebadge.exclamationmark")
                    }

                    NavigationLink {
                        SensorySettings()
                    } label: {
                        Label("Sensory Preferences", systemImage: "waveform")
                    }
                }

                Section {
                    NavigationLink {
                        TextToSpeechSettingsView()
                    } label: {
                        Label("Text-to-Speech", systemImage: "speaker.wave.3")
                    }

                    NavigationLink {
                        LanguageSettingsView()
                    } label: {
                        Label("Language & Reading", systemImage: "globe")
                    }
                }

                Section {
                    Button("Reset to Defaults") {
                        resetAccessibilitySettings()
                    }
                    .foregroundColor(.red)
                }
            }
            .navigationTitle("Accessibility")
        }
    }
}
```

### Testing & Validation

```swift
// Automated accessibility testing
struct AccessibilityTests {
    static func runAudit() -> [AccessibilityIssue] {
        var issues: [AccessibilityIssue] = []

        // Check contrast ratios
        issues += checkContrastRatios()

        // Check touch target sizes
        issues += checkTouchTargets()

        // Check accessibility labels
        issues += checkAccessibilityLabels()

        // Check keyboard navigation
        issues += checkKeyboardNavigation()

        // Check VoiceOver compatibility
        issues += checkVoiceOverCompatibility()

        return issues
    }

    static func checkContrastRatios() -> [AccessibilityIssue] {
        // Implementation using ColorContrast library
        []
    }

    static func checkTouchTargets() -> [AccessibilityIssue] {
        // Check all interactive elements are at least 44x44
        []
    }
}

// Accessibility audit view (dev tool)
struct AccessibilityAuditView: View {
    @State private var issues: [AccessibilityIssue] = []

    var body: some View {
        List(issues) { issue in
            VStack(alignment: .leading) {
                Text(issue.title)
                    .font(.headline)
                Text(issue.description)
                    .font(.caption)
                    .foregroundColor(.secondary)
                Text("Severity: \(issue.severity.rawValue)")
                    .font(.caption)
                    .foregroundColor(issue.severity.color)
            }
        }
        .onAppear {
            issues = AccessibilityTests.runAudit()
        }
    }
}
```

### Accessibility Commitment

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│                    CONVERGIO ACCESSIBILITY COMMITMENT                       │
│                                                                             │
│  We believe that AI-powered productivity tools should be accessible        │
│  to everyone, regardless of ability. Convergio is committed to:            │
│                                                                             │
│  ✓ WCAG 2.2 AAA compliance as our minimum standard                         │
│  ✓ Regular accessibility audits by disabled users                          │
│  ✓ Dedicated accessibility team for ongoing improvements                   │
│  ✓ Free accessibility features (never paywalled)                           │
│  ✓ Open dialogue with the disability community                             │
│  ✓ Transparent accessibility roadmap                                       │
│                                                                             │
│  If you encounter any accessibility barriers, please contact us:           │
│  accessibility@convergio.app                                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## UI/UX Design Proposals

### Design Philosophy

> "Not a chatbot. A boardroom."

The UI must communicate that Convergio is a **team of specialists**, not a single AI assistant.

### Main Window Layout

```
┌─────────────────────────────────────────────────────────────────────────┐
│ ◉ ◉ ◉                    Convergio                          🔍  👤  ⚙️ │
├──────────────────┬──────────────────────────────────────────────────────┤
│                  │                                                      │
│  🧠 Ali          │  ┌─────────────────────────────────────────────────┐ │
│  Chief of Staff  │  │                                                 │ │
│  ● Active        │  │  "I'll coordinate the team on this analysis.   │ │
│                  │  │   Let me bring in the relevant experts..."     │ │
│  ─────────────── │  │                                                 │ │
│                  │  │  ┌─────────────────────────────────────────┐   │ │
│  📊 Angela       │  │  │        CONVERGENCE IN PROGRESS          │   │ │
│  Data Analyst    │  │  │                                         │   │ │
│  ◐ Thinking...   │  │  │   Angela ──┐                            │   │ │
│                  │  │  │   Matteo ──┼──► Ali ──► Final Response  │   │ │
│  💼 Matteo       │  │  │   Amy ─────┘                            │   │ │
│  Business Arch   │  │  │                                         │   │ │
│  ◐ Thinking...   │  │  │   [Progress: 67%] [Cost: $0.12]         │   │ │
│                  │  │  └─────────────────────────────────────────┘   │ │
│  💰 Amy          │  │                                                 │ │
│  CFO             │  │  ─────────────────────────────────────────────  │ │
│  ◐ Thinking...   │  │                                                 │ │
│                  │  │  [Previous conversation history...]             │ │
│  ─────────────── │  │                                                 │ │
│                  │  └─────────────────────────────────────────────────┘ │
│  + 50 more       │                                                      │
│    agents        │  ┌─────────────────────────────────────────────────┐ │
│                  │  │ Ask the team...                              ⌘↵ │ │
├──────────────────┴──┴─────────────────────────────────────────────────┴─┤
│  Session: 45 min │ Cost: $2.34 / $10.00 │ Model: Claude 3.5 │ MLX: On  │
└─────────────────────────────────────────────────────────────────────────┘
```

### Liquid Glass Components

#### 1. Agent Cards (Sidebar)

```swift
struct AgentCardView: View {
    let agent: Agent
    @Namespace private var namespace

    var body: some View {
        HStack(spacing: 12) {
            // Agent avatar with state indicator
            ZStack {
                Image(systemName: agent.icon)
                    .font(.title2)
                    .foregroundStyle(.white)

                // State ring
                Circle()
                    .stroke(agent.stateColor, lineWidth: 2)
                    .frame(width: 44, height: 44)
            }
            .glassEffect(.regular.interactive(), in: .circle)
            .glassEffectID("avatar-\(agent.id)", in: namespace)

            VStack(alignment: .leading, spacing: 2) {
                Text(agent.name)
                    .font(.headline)
                    .foregroundStyle(.primary)

                Text(agent.role)
                    .font(.caption)
                    .foregroundStyle(.secondary)

                if agent.state == .thinking {
                    ThinkingIndicator()
                }
            }

            Spacer()
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .glassEffect(.regular.tint(agent.accentColor.opacity(0.3)))
    }
}
```

#### 2. Convergence Visualization

```swift
struct ConvergenceView: View {
    let activeAgents: [Agent]
    let convergenceProgress: Double
    @Namespace private var convergenceNamespace

    var body: some View {
        GlassEffectContainer(spacing: 20) {
            // Agent bubbles that morph toward center
            ForEach(activeAgents) { agent in
                AgentBubble(agent: agent, progress: convergenceProgress)
                    .glassEffect(.regular.interactive())
                    .glassEffectID("converge-\(agent.id)", in: convergenceNamespace)
                    .offset(bubbleOffset(for: agent, progress: convergenceProgress))
            }

            // Central convergence point
            if convergenceProgress > 0.5 {
                Circle()
                    .fill(.ultraThinMaterial)
                    .frame(width: 80, height: 80)
                    .overlay {
                        Image(systemName: "brain.head.profile")
                            .font(.title)
                            .foregroundStyle(.white)
                    }
                    .glassEffect(.regular.tint(.purple))
                    .glassEffectID("convergence-center", in: convergenceNamespace)
                    .transition(.scale.combined(with: .opacity))
            }
        }
        .animation(.spring(duration: 0.6), value: convergenceProgress)
    }

    private func bubbleOffset(for agent: Agent, progress: Double) -> CGSize {
        // Agents start at edges, converge toward center
        let angle = agent.assignedAngle
        let distance = 150 * (1 - progress) // 150pt at start, 0 at convergence
        return CGSize(
            width: cos(angle) * distance,
            height: sin(angle) * distance
        )
    }
}
```

#### 3. Streaming Response View

```swift
struct StreamingResponseView: View {
    @ObservedObject var viewModel: ConversationViewModel

    var body: some View {
        ScrollViewReader { proxy in
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 16) {
                    ForEach(viewModel.messages) { message in
                        MessageBubble(message: message)
                            .id(message.id)
                    }

                    // Live streaming indicator
                    if viewModel.isStreaming {
                        HStack {
                            TypingIndicator()
                            Text(viewModel.streamingText)
                                .font(.body)
                                .foregroundStyle(.primary)
                        }
                        .padding()
                        .glassEffect(.clear)
                        .id("streaming")
                    }
                }
                .padding()
            }
            .onChange(of: viewModel.messages.count) { _, _ in
                withAnimation {
                    proxy.scrollTo(viewModel.messages.last?.id, anchor: .bottom)
                }
            }
        }
    }
}
```

#### 4. Menu Bar Quick Access

```swift
@main
struct ConvergioApp: App {
    @StateObject private var orchestrator = Orchestrator()

    var body: some Scene {
        // Main window
        WindowGroup {
            ContentView()
                .environmentObject(orchestrator)
        }
        .windowStyle(.hiddenTitleBar)

        // Menu bar presence
        MenuBarExtra("Convergio", systemImage: "brain.head.profile") {
            MenuBarView()
                .environmentObject(orchestrator)
        }
        .menuBarExtraStyle(.window)

        // Settings
        Settings {
            SettingsView()
                .environmentObject(orchestrator)
        }
    }
}

struct MenuBarView: View {
    @EnvironmentObject var orchestrator: Orchestrator
    @State private var quickPrompt: String = ""

    var body: some View {
        VStack(spacing: 16) {
            // Quick prompt field
            TextField("Ask the team...", text: $quickPrompt)
                .textFieldStyle(.plain)
                .padding(12)
                .glassEffect(.regular)
                .onSubmit {
                    Task {
                        await orchestrator.send(quickPrompt)
                        quickPrompt = ""
                    }
                }

            // Active agents
            if !orchestrator.activeAgents.isEmpty {
                VStack(alignment: .leading, spacing: 8) {
                    Text("Active")
                        .font(.caption)
                        .foregroundStyle(.secondary)

                    ForEach(orchestrator.activeAgents.prefix(5)) { agent in
                        HStack {
                            Image(systemName: agent.icon)
                            Text(agent.name)
                            Spacer()
                            StateIndicator(state: agent.state)
                        }
                        .font(.callout)
                    }
                }
                .padding(.horizontal)
            }

            Divider()

            // Quick stats
            HStack {
                Label("$\(orchestrator.sessionCost, specifier: "%.2f")", systemImage: "dollarsign.circle")
                Spacer()
                Label(orchestrator.modelName, systemImage: "cpu")
            }
            .font(.caption)
            .foregroundStyle(.secondary)
            .padding(.horizontal)

            Divider()

            // Actions
            Button("Open Convergio") {
                NSApp.activate(ignoringOtherApps: true)
            }
            .keyboardShortcut("o")

            Button("Quit") {
                NSApp.terminate(nil)
            }
            .keyboardShortcut("q")
        }
        .padding()
        .frame(width: 320)
    }
}
```

#### 5. Global Hotkey Integration

```swift
// Using sindresorhus/KeyboardShortcuts
import KeyboardShortcuts

extension KeyboardShortcuts.Name {
    static let toggleConvergio = Self("toggleConvergio", default: .init(.space, modifiers: [.command, .option]))
    static let newConversation = Self("newConversation", default: .init(.n, modifiers: [.command]))
}

struct HotkeySetup {
    static func register() {
        KeyboardShortcuts.onKeyUp(for: .toggleConvergio) {
            if let window = NSApp.windows.first(where: { $0.title == "Convergio" }) {
                if window.isVisible {
                    window.orderOut(nil)
                } else {
                    window.makeKeyAndOrderFront(nil)
                    NSApp.activate(ignoringOtherApps: true)
                }
            }
        }
    }
}
```

### Agent State Visualizations

| State | Visual | Animation |
|-------|--------|-----------|
| **Idle** | Dim glass, gray ring | None |
| **Thinking** | Bright glass, pulsing ring | Soft pulse (1.5s) |
| **Responding** | Bright glass, streaming glow | Shimmer effect |
| **Collaborating** | Connected lines to other agents | Line drawing animation |
| **Converging** | Moving toward center | Position interpolation |

### Color System (Liquid Glass Compatible)

```swift
extension Color {
    // Agent role colors (translucent-friendly)
    static let agentTechnical = Color(red: 0.2, green: 0.6, blue: 1.0)     // Blue
    static let agentBusiness = Color(red: 1.0, green: 0.6, blue: 0.2)       // Orange
    static let agentCreative = Color(red: 0.8, green: 0.3, blue: 0.8)       // Purple
    static let agentStrategy = Color(red: 0.2, green: 0.8, blue: 0.4)       // Green
    static let agentLeadership = Color(red: 1.0, green: 0.8, blue: 0.2)     // Gold

    // State colors
    static let stateIdle = Color.gray.opacity(0.5)
    static let stateThinking = Color.blue.opacity(0.8)
    static let stateActive = Color.green.opacity(0.8)
    static let stateConverging = Color.purple.opacity(0.8)
}
```

---

## Technical Implementation Plan

### Phase 0: Foundation (Week 1-2)

**Goal:** Establish C-to-Swift bridge and verify core functionality.

#### Tasks:

1. **Create ConvergioCore Swift Package**
   - Set up module.modulemap for C headers
   - Create Swift wrapper types
   - Verify build with `swift build`

2. **Build libConvergioCore.a**
   - Modify CMakeLists.txt to produce static library
   - Test linking with simple Swift test

3. **Smoke Test**
   - Call `convergio_create()` from Swift
   - Send a message, receive response
   - Verify no memory leaks

#### Deliverable:
```swift
// This must work:
let orchestrator = Orchestrator()
let response = await orchestrator.send("Hello")
print(response)
```

### Phase 1: Skeleton App (Week 3-4)

**Goal:** Basic SwiftUI app structure with C library integrated.

#### Tasks:

1. **Create Xcode Project**
   - macOS app target
   - Link ConvergioCore
   - Set up entitlements (network, file access)

2. **Implement Core Views**
   - `ContentView` with NavigationSplitView
   - `SidebarView` with agent list (static mock)
   - `ConversationView` with basic text I/O

3. **Basic State Management**
   - `OrchestratorViewModel` wrapping C library
   - `@Published` properties for agents, messages

4. **Menu Bar Integration**
   - MenuBarExtra with simple view
   - Quit/open commands

#### Deliverable:
- App launches, shows sidebar
- Can send message, receive text response
- Menu bar icon appears

### Phase 2: Liquid Glass UI (Week 5-7)

**Goal:** Full Liquid Glass implementation per Apple guidelines.

#### Tasks:

1. **Agent Cards with Glass**
   - Implement `AgentCardView` with `.glassEffect()`
   - State-based color tinting
   - Interactive responses

2. **Conversation Glass Elements**
   - Glass input field
   - Semi-transparent message bubbles (user)
   - Solid response text (content, not navigation)

3. **GlassEffectContainer Layout**
   - Wrap agent grid in container
   - Implement morphing transitions

4. **Convergence Visualization**
   - Basic version with agent bubbles
   - Animation toward center point

5. **Accessibility Testing**
   - Verify reduced transparency mode
   - Test increased contrast
   - Validate with VoiceOver

#### Deliverable:
- Full Liquid Glass aesthetic
- Morphing agent cards
- Convergence animation (basic)

### Phase 3: Advanced Features (Week 8-10)

**Goal:** Feature parity with CLI + native enhancements.

#### Tasks:

1. **Streaming Responses**
   - Async stream from C callbacks
   - Real-time text rendering
   - Markdown to AttributedString

2. **Agent Detail View**
   - Expandable agent profiles
   - Capability list
   - Recent activity

3. **Cost Dashboard**
   - Real-time cost display
   - Budget progress bar
   - Historical cost chart

4. **Global Hotkey**
   - Integrate KeyboardShortcuts
   - Toggle window visibility
   - Quick prompt from anywhere

5. **Project Support**
   - Project selector
   - Context switching
   - History per project

6. **Native Notifications**
   - Reminder integration
   - Agent completion alerts
   - Budget warnings

#### Deliverable:
- Full feature parity with CLI
- Native macOS integration
- Global hotkey functional

### Phase 4: Polish & Testing (Week 11-14)

**Goal:** Production-ready quality.

#### Tasks:

1. **Performance Optimization**
   - Profile with Instruments
   - Optimize list rendering
   - Memory leak detection

2. **UI Polish**
   - Animation timing refinement
   - Edge case handling
   - Loading states

3. **Testing**
   - Unit tests for ViewModels
   - UI tests for critical flows
   - Integration tests with C library

4. **Error Handling**
   - Network failure states
   - Provider error display
   - Graceful degradation

5. **Documentation**
   - In-app onboarding
   - Keyboard shortcuts help
   - Agent capability reference

#### Deliverable:
- No crashes in 48-hour soak test
- 80%+ test coverage on ViewModels
- Smooth 60fps animations

### Phase 5: App Store Preparation (Week 15-18)

**Goal:** Ready for public release.

#### Tasks:

1. **Sandboxing**
   - Review entitlements
   - Test all features in sandbox
   - Handle file access gracefully

2. **Notarization**
   - Code signing setup
   - Hardened runtime
   - Notarization workflow

3. **App Store Assets**
   - Screenshots (all sizes)
   - App preview video
   - Description copy

4. **Privacy**
   - Privacy policy update
   - App tracking transparency
   - Data usage disclosure

5. **Beta Testing**
   - TestFlight distribution
   - Feedback collection
   - Critical bug fixes

#### Deliverable:
- Notarized DMG for direct distribution
- App Store submission package
- TestFlight beta live

---

## Phase Breakdown & Milestones

```
Week  1-2   [████░░░░░░░░░░░░░░]  Phase 0: Foundation
Week  3-4   [████████░░░░░░░░░░]  Phase 1: Skeleton App
Week  5-7   [████████████░░░░░░]  Phase 2: Liquid Glass
Week  8-10  [████████████████░░]  Phase 3: Advanced Features
Week 11-14  [██████████████████]  Phase 4: Polish
Week 15-18  [██████████████████]  Phase 5: App Store

Milestone 1 (Week 2):  C-Swift bridge working
Milestone 2 (Week 4):  Basic app sending/receiving messages
Milestone 3 (Week 7):  Full Liquid Glass UI
Milestone 4 (Week 10): Feature parity with CLI
Milestone 5 (Week 14): Beta-ready build
Milestone 6 (Week 18): App Store submission
```

---

## Risk Matrix & Mitigations

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| **Liquid Glass bugs in Xcode 26** | HIGH | MEDIUM | Have fallback to standard materials |
| **C-Swift interop complexity** | MEDIUM | HIGH | Start with minimal bridge, expand incrementally |
| **54 agents UX overwhelm** | HIGH | HIGH | Smart grouping, search, "featured" agents |
| **Performance issues** | MEDIUM | HIGH | Profile early, optimize lists first |
| **Scope creep** | HIGH | HIGH | Strict MVP definition, feature backlog |
| **Sandboxing breaks features** | MEDIUM | MEDIUM | Test sandbox early, not at the end |
| **App Store rejection** | LOW | HIGH | Follow guidelines strictly, no private APIs |

### Contingency Plans

1. **If Liquid Glass is too buggy:**
   - Fall back to `.ultraThinMaterial`
   - Still looks modern, just less cutting-edge

2. **If C interop is painful:**
   - Create intermediate Objective-C++ layer
   - Already have .m file experience

3. **If 54 agents is overwhelming:**
   - "Team" groupings (Technical, Business, Creative)
   - AI-suggested agent selection
   - "Quick team" presets

---

## Competitive Analysis

### Current Mac AI Apps

| App | Strengths | Weaknesses | Convergio Advantage |
|-----|-----------|------------|---------------------|
| **ChatGPT Mac** | Simple, reliable | Single agent, no customization | Multi-agent orchestration |
| **Claude Mac** | Clean UI, long context | Single agent | 54 specialized experts |
| **Raycast AI** | Fast, integrated | Limited depth | Deep domain expertise |
| **MacGPT** | Menu bar focus | Basic UI | Native Liquid Glass |

### Convergio's Unique Position

1. **Not a chatbot** - A virtual executive team
2. **Convergence** - Multiple experts synthesize answers
3. **Cost control** - Built-in budget management
4. **Offline capable** - MLX local inference
5. **Professional focus** - Built for serious work

### Target Users

- **Startup founders** - Need CFO, legal, strategy advice
- **Solo consultants** - Augment with virtual team
- **Product managers** - Technical + business perspective
- **Executives** - High-level strategic thinking

---

## Success Metrics

### Launch Targets (Month 1)

| Metric | Target |
|--------|--------|
| Daily Active Users | 500 |
| App Store Rating | 4.5+ |
| Crash-free sessions | 99.5% |
| Average session length | 15+ min |

### Growth Targets (Month 6)

| Metric | Target |
|--------|--------|
| Monthly Active Users | 10,000 |
| Paid conversions (if applicable) | 5% |
| User retention (30-day) | 40% |
| NPS Score | 50+ |

### Quality Gates

- [ ] Zero P0 bugs in production
- [ ] App launch < 2 seconds
- [ ] Memory footprint < 500MB
- [ ] Battery impact: "Low" in Activity Monitor

---

## Appendix A: Liquid Glass Quick Reference

```swift
// Basic usage
view.glassEffect()                              // Default
view.glassEffect(.regular)                      // Explicit regular
view.glassEffect(.clear)                        // More transparent
view.glassEffect(.identity)                     // Disabled

// Customization
view.glassEffect(.regular.tint(.blue))          // Color tint
view.glassEffect(.regular.interactive())        // Responds to touch
view.glassEffect(.regular.tint(.red).interactive())  // Both

// Shapes
view.glassEffect(.regular, in: .capsule)        // Capsule (default)
view.glassEffect(.regular, in: .circle)         // Circle
view.glassEffect(.regular, in: .rect(cornerRadius: 16))  // Rounded rect

// Morphing
view.glassEffectID("myID", in: namespace)       // Enable morphing

// Containers
GlassEffectContainer(spacing: 20) {             // Required for morphing
    // Glass children here
}
```

## Appendix B: Key Dependencies

| Dependency | Purpose | Source |
|------------|---------|--------|
| KeyboardShortcuts | Global hotkeys | github.com/sindresorhus/KeyboardShortcuts |
| MarkdownUI | Markdown rendering | github.com/gonzalezreal/MarkdownUI |
| SQLite.swift | Database access (optional) | github.com/stephencelis/SQLite.swift |

## Appendix C: References

- [macOS Tahoe 26 Features](https://www.macrumors.com/roundup/macos-26/)
- [Liquid Glass Implementation Guide](https://github.com/conorluddy/LiquidGlassReference)
- [Apple HIG](https://developer.apple.com/design/human-interface-guidelines/)
- [WWDC25: Build a SwiftUI app with the new design](https://developer.apple.com/videos/play/wwdc2025/323/)
- [SwiftUI C Library Integration](https://www.swift.org/documentation/articles/wrapping-c-cpp-library-in-swift.html)

---

## Next Steps

1. **Immediate:** Set up ConvergioCore package structure
2. **This week:** Create basic C-Swift bridge and verify
3. **Review:** Schedule design review after Phase 1 completion

---

*"The best interface is the one that disappears, letting you focus on your work, not the tool."*

**Let's build the future of AI-assisted decision making.**
