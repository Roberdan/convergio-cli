/**
 * CONVERGIO NATIVE - Main Application
 *
 * The main entry point for Convergio Native macOS application.
 * Features a main window, menu bar presence, and global hotkey support.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore
import AVFoundation

@main
struct ConvergioApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate

    @StateObject private var orchestratorVM = OrchestratorViewModel()
    @StateObject private var conversationVM = ConversationViewModel()
    @StateObject private var keychainManager = KeychainManager.shared
    @StateObject private var logger = Logger.shared

    @Environment(\.openWindow) private var openWindow

    /// Theme setting from AppStorage
    @AppStorage("appearanceMode") private var appearanceMode = "system"

    /// Convert stored appearance mode to ColorScheme
    private var preferredColorScheme: ColorScheme? {
        switch appearanceMode {
        case "light": return .light
        case "dark": return .dark
        default: return nil  // "system" uses nil for automatic
        }
    }

    /// Check if any API key is configured or local models are selected
    private var hasAnyApiKey: Bool {
        keychainManager.hasAnthropicKey ||
        keychainManager.hasOpenAIKey ||
        keychainManager.hasGeminiKey ||
        keychainManager.hasOpenRouterKey ||
        UserDefaults.standard.string(forKey: "selectedProvider") == APIProvider.ollama.rawValue
    }

    /// Show onboarding if no API keys are configured
    @State private var showOnboarding = false
    @State private var showAbout = false

    init() {
        // Set up crash handler
        setupCrashHandler()
        logInfo("Convergio app starting", category: "System")

        // Initialize Keychain and load API keys
        Task { @MainActor in
            KeychainManager.shared.initializeOnLaunch()
        }
    }

    var body: some Scene {
        // Main window
        WindowGroup {
            ZStack {
                ContentView()
                    .environmentObject(orchestratorVM)
                    .environmentObject(conversationVM)
                    .environmentObject(keychainManager)
                    .environmentObject(logger)
                    .preferredColorScheme(preferredColorScheme)
                    .frame(minWidth: 900, minHeight: 600)
                    .task {
                        logInfo("Initializing orchestrator", category: "System")
                        await orchestratorVM.initialize()
                        logInfo("Orchestrator initialized", category: "System")

                        // Check if we need to show onboarding
                        if !hasAnyApiKey && !UserDefaults.standard.bool(forKey: "onboardingComplete") {
                            showOnboarding = true
                        }
                    }
            }
            .sheet(isPresented: $showOnboarding) {
                OnboardingView(isComplete: $showOnboarding)
                    .environmentObject(keychainManager)
            }
            .sheet(isPresented: $showAbout) {
                AboutView()
            }
        }
        .windowStyle(.hiddenTitleBar)
        .defaultSize(width: 1200, height: 800)
        .commands {
            // App menu - About
            CommandGroup(replacing: .appInfo) {
                Button("About Convergio") {
                    showAbout = true
                }
            }

            // File menu
            CommandGroup(replacing: .newItem) {
                Button("New Conversation") {
                    conversationVM.newConversation()
                }
                .keyboardShortcut("n", modifiers: .command)

                Divider()

                Button("Clear History") {
                    conversationVM.clearHistory()
                }
                .keyboardShortcut("k", modifiers: [.command, .shift])
            }

            // View menu
            CommandGroup(after: .toolbar) {
                Button("Show Agent Panel") {
                    orchestratorVM.showAgentPanel.toggle()
                }
                .keyboardShortcut("1", modifiers: [.command, .option])

                Button("Show Cost Dashboard") {
                    orchestratorVM.showCostDashboard.toggle()
                }
                .keyboardShortcut("2", modifiers: [.command, .option])

                Divider()

                Button("Agent Interaction Visualizer") {
                    openWindow(id: "agent-visualizer")
                }
                .keyboardShortcut("3", modifiers: [.command, .option])

                Button("Agent Editor") {
                    openWindow(id: "agent-editor")
                }
                .keyboardShortcut("4", modifiers: [.command, .option])

                Divider()

                Button("Microphone Test") {
                    openWindow(id: "microphone-test")
                }
                .keyboardShortcut("m", modifiers: [.command, .option])
            }

            // Help menu
            CommandGroup(replacing: .help) {
                Button("Convergio Help") {
                    openWindow(id: "help-system")
                }
                .keyboardShortcut("?", modifiers: .command)

                Divider()

                Button("Keyboard Shortcuts") {
                    openWindow(id: "help-shortcuts")
                }

                Button("Online Documentation") {
                    if let url = URL(string: "https://github.com/Roberdan/Convergio/wiki") {
                        NSWorkspace.shared.open(url)
                    }
                }

                Divider()

                Button("Report an Issue") {
                    if let url = URL(string: "https://github.com/Roberdan/Convergio/issues") {
                        NSWorkspace.shared.open(url)
                    }
                }
            }
        }

        // Menu bar presence
        MenuBarExtra("Convergio", systemImage: "brain.head.profile") {
            MenuBarView()
                .environmentObject(orchestratorVM)
                .environmentObject(conversationVM)
        }
        .menuBarExtraStyle(.window)

        // Settings window
        Settings {
            SettingsView()
                .environmentObject(orchestratorVM)
                .preferredColorScheme(preferredColorScheme)
        }

        // Debug log viewer window
        Window("Log Viewer", id: "log-viewer") {
            LogViewerView()
        }
        .defaultSize(width: 800, height: 500)
        .keyboardShortcut("l", modifiers: [.command, .option])

        // Keyboard shortcuts help window
        Window("Keyboard Shortcuts", id: "help-shortcuts") {
            KeyboardShortcutsView()
        }
        .defaultSize(width: 500, height: 400)

        // Agent Interaction Visualizer
        Window("Agent Interaction Visualizer", id: "agent-visualizer") {
            AgentInteractionVisualizer()
                .environmentObject(orchestratorVM)
        }
        .defaultSize(width: 900, height: 700)
        .windowStyle(.hiddenTitleBar)

        // Agent Markdown Editor
        Window("Agent Editor", id: "agent-editor") {
            AgentMarkdownEditor()
                .environmentObject(orchestratorVM)
        }
        .defaultSize(width: 1100, height: 750)

        // Help System
        Window("Convergio Help", id: "help-system") {
            HelpSystemView()
        }
        .defaultSize(width: 900, height: 600)

        // Microphone Test (Debug)
        Window("Microphone Test", id: "microphone-test") {
            MicrophoneTestView()
        }
        .defaultSize(width: 550, height: 600)
    }
}

// MARK: - About View

struct AboutView: View {
    @Environment(\.dismiss) private var dismiss

    private var appVersion: String {
        Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String ?? "1.0"
    }

    private var buildNumber: String {
        Bundle.main.infoDictionary?["CFBundleVersion"] as? String ?? "1"
    }

    var body: some View {
        VStack(spacing: 20) {
            // App Icon
            if let appIcon = NSImage(named: "AppIcon") {
                Image(nsImage: appIcon)
                    .resizable()
                    .frame(width: 128, height: 128)
            } else {
                Image(systemName: "brain.head.profile")
                    .font(.system(size: 80))
                    .foregroundStyle(.purple.gradient)
            }

            // App Name
            Text("Convergio")
                .font(.largeTitle.bold())

            Text("Your AI Executive Team")
                .font(.headline)
                .foregroundStyle(.secondary)

            // Version
            Text("Version \(appVersion) (\(buildNumber))")
                .font(.caption)
                .foregroundStyle(.tertiary)

            Divider()
                .frame(width: 200)

            // Credits
            VStack(spacing: 4) {
                Text("Created by Roberto D'Angelo")
                    .font(.caption)
                Text("with the help of AI agents team")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Text("Copyright 2025. All rights reserved.")
                .font(.caption2)
                .foregroundStyle(.tertiary)

            Spacer()

            Button("OK") {
                dismiss()
            }
            .keyboardShortcut(.defaultAction)
        }
        .padding(30)
        .frame(width: 350, height: 400)
    }
}

// MARK: - Keyboard Shortcuts View

struct KeyboardShortcutsView: View {
    private let shortcuts: [(category: String, items: [(key: String, description: String)])] = [
        ("General", [
            ("Cmd+N", "New Conversation"),
            ("Cmd+,", "Open Settings"),
            ("Cmd+Q", "Quit Convergio"),
        ]),
        ("Conversation", [
            ("Cmd+Return", "Send Message"),
            ("Cmd+.", "Cancel Request"),
            ("Cmd+Shift+K", "Clear History"),
        ]),
        ("View", [
            ("Cmd+Opt+1", "Toggle Agent Panel"),
            ("Cmd+Opt+2", "Toggle Cost Dashboard"),
            ("Cmd+Opt+3", "Agent Visualizer"),
            ("Cmd+Opt+4", "Agent Editor"),
            ("Cmd+Opt+L", "Open Log Viewer"),
        ]),
        ("Global", [
            ("Cmd+Shift+Space", "Activate from anywhere"),
        ]),
    ]

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Keyboard Shortcuts")
                .font(.title2.bold())
                .padding(.bottom, 8)

            ForEach(shortcuts, id: \.category) { section in
                VStack(alignment: .leading, spacing: 8) {
                    Text(section.category)
                        .font(.headline)
                        .foregroundStyle(.secondary)

                    ForEach(section.items, id: \.key) { item in
                        HStack {
                            Text(item.key)
                                .font(.system(.body, design: .monospaced))
                                .padding(.horizontal, 8)
                                .padding(.vertical, 2)
                                .background(Color.secondary.opacity(0.1))
                                .clipShape(RoundedRectangle(cornerRadius: 4))

                            Spacer()

                            Text(item.description)
                                .foregroundStyle(.primary)
                        }
                    }
                }
            }

            Spacer()
        }
        .padding(24)
    }
}

// MARK: - Crash Handler

private func setupCrashHandler() {
    NSSetUncaughtExceptionHandler { exception in
        let message = """
        Uncaught Exception:
        Name: \(exception.name.rawValue)
        Reason: \(exception.reason ?? "Unknown")
        Call Stack:
        \(exception.callStackSymbols.joined(separator: "\n"))
        """
        logCritical(message, category: "Crash")

        // Write to file immediately
        if let logsDir = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first?
            .appendingPathComponent("Convergio/Logs", isDirectory: true) {
            try? FileManager.default.createDirectory(at: logsDir, withIntermediateDirectories: true)
            let crashFile = logsDir.appendingPathComponent("crash_\(Date().ISO8601Format()).log")
            try? message.write(to: crashFile, atomically: true, encoding: .utf8)
        }
    }

    // Signal handlers for segfaults etc.
    signal(SIGABRT) { _ in logCritical("SIGABRT received", category: "Crash") }
    signal(SIGSEGV) { _ in logCritical("SIGSEGV received", category: "Crash") }
    signal(SIGBUS) { _ in logCritical("SIGBUS received", category: "Crash") }
}

// MARK: - Microphone Test View

/// Simple microphone test - minimal code to verify audio capture
struct MicrophoneTestView: View {
    @StateObject private var tester = MicrophoneTester()

    var body: some View {
        VStack(spacing: 20) {
            Text("Microphone Test")
                .font(.largeTitle)
                .fontWeight(.bold)

            HStack {
                Circle()
                    .fill(tester.isCapturing ? Color.green : Color.red)
                    .frame(width: 20, height: 20)
                Text(tester.isCapturing ? "Capturing" : "Stopped")
                    .font(.headline)
            }

            VStack(alignment: .leading, spacing: 8) {
                Text("Audio Level: \(String(format: "%.4f", tester.audioLevel))")
                    .font(.system(.body, design: .monospaced))

                GeometryReader { geo in
                    ZStack(alignment: .leading) {
                        RoundedRectangle(cornerRadius: 4)
                            .fill(Color.gray.opacity(0.3))
                        RoundedRectangle(cornerRadius: 4)
                            .fill(tester.audioLevel > 0.001 ? Color.green : Color.orange)
                            .frame(width: geo.size.width * CGFloat(min(1.0, tester.audioLevel * 10)))
                            .animation(.linear(duration: 0.05), value: tester.audioLevel)
                    }
                }
                .frame(height: 30)
            }
            .padding()
            .background(Color.black.opacity(0.1))
            .cornerRadius(8)

            Text("Buffers received: \(tester.bufferCount)")
                .font(.system(.body, design: .monospaced))

            ScrollView {
                Text(tester.log)
                    .font(.system(.caption, design: .monospaced))
                    .frame(maxWidth: .infinity, alignment: .leading)
            }
            .frame(height: 200)
            .padding()
            .background(Color.black)
            .foregroundColor(.green)
            .cornerRadius(8)

            HStack(spacing: 20) {
                Button(tester.isCapturing ? "Stop" : "Start") {
                    if tester.isCapturing {
                        tester.stop()
                    } else {
                        tester.start()
                    }
                }
                .buttonStyle(.borderedProminent)

                Button("Clear Log") {
                    tester.clearLog()
                }
                .buttonStyle(.bordered)
            }
        }
        .padding(30)
        .frame(minWidth: 500, minHeight: 500)
    }
}

@MainActor
class MicrophoneTester: ObservableObject {
    @Published var isCapturing = false
    @Published var audioLevel: Float = 0.0
    @Published var bufferCount: Int = 0
    @Published var log: String = ""

    private var audioEngine: AVAudioEngine?

    func start() {
        addLog("Starting microphone test...")

        let status = AVCaptureDevice.authorizationStatus(for: .audio)
        addLog("Permission status: \(status.rawValue) (3=authorized)")

        if status != .authorized {
            addLog("ERROR: Microphone not authorized!")
            if status == .notDetermined {
                AVCaptureDevice.requestAccess(for: .audio) { granted in
                    Task { @MainActor in
                        self.addLog(granted ? "Permission granted" : "Permission denied")
                    }
                }
            }
            return
        }

        do {
            let engine = AVAudioEngine()
            self.audioEngine = engine

            let inputNode = engine.inputNode
            addLog("Got inputNode")

            let hwFormat = inputNode.inputFormat(forBus: 0)
            addLog("HW input: \(Int(hwFormat.sampleRate))Hz, \(hwFormat.channelCount)ch")

            let outFormat = inputNode.outputFormat(forBus: 0)
            addLog("Output: \(Int(outFormat.sampleRate))Hz, \(outFormat.channelCount)ch")

            addLog("Installing tap (format: nil)...")

            inputNode.installTap(onBus: 0, bufferSize: 1024, format: nil) { [weak self] buffer, _ in
                guard let channelData = buffer.floatChannelData else { return }
                let frameLength = Int(buffer.frameLength)
                var sum: Float = 0
                for i in 0..<frameLength {
                    let sample = channelData[0][i]
                    sum += sample * sample
                }
                let rms = sqrt(sum / Float(frameLength))

                Task { @MainActor [weak self] in
                    self?.audioLevel = rms
                    self?.bufferCount += 1
                    if self?.bufferCount == 1 {
                        self?.addLog("First buffer! \(Int(buffer.format.sampleRate))Hz")
                    }
                }
            }

            addLog("Tap installed, preparing...")
            engine.prepare()

            addLog("Starting engine...")
            try engine.start()

            isCapturing = true
            addLog("SUCCESS! Speak into microphone...")

        } catch {
            addLog("ERROR: \(error.localizedDescription)")
            addLog("Code: \((error as NSError).code)")
        }
    }

    func stop() {
        addLog("Stopping...")
        if let engine = audioEngine {
            engine.inputNode.removeTap(onBus: 0)
            engine.stop()
        }
        audioEngine = nil
        isCapturing = false
        audioLevel = 0
        bufferCount = 0
        addLog("Stopped")
    }

    func clearLog() {
        log = ""
        bufferCount = 0
    }

    private func addLog(_ message: String) {
        let timestamp = DateFormatter.localizedString(from: Date(), dateStyle: .none, timeStyle: .medium)
        log += "[\(timestamp)] \(message)\n"
        print("[MIC TEST] \(message)")
    }
}
