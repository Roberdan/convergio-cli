/**
 * CONVERGIO NATIVE - Log Viewer View
 *
 * Debug view for viewing and exporting application logs.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

struct LogViewerView: View {
    @ObservedObject var logger = Logger.shared
    @State private var searchText = ""
    @State private var selectedLevel: LogLevel? = nil
    @State private var selectedCategory: String? = nil
    @State private var autoScroll = true

    private var filteredLogs: [LogEntry] {
        logger.recentLogs.filter { entry in
            // Level filter
            if let level = selectedLevel {
                guard entry.level == level.description else { return false }
            }

            // Category filter
            if let category = selectedCategory {
                guard entry.category == category else { return false }
            }

            // Search filter
            if !searchText.isEmpty {
                let searchLower = searchText.lowercased()
                return entry.message.lowercased().contains(searchLower) ||
                       entry.category.lowercased().contains(searchLower) ||
                       entry.file.lowercased().contains(searchLower)
            }

            return true
        }
    }

    private var categories: [String] {
        Array(Set(logger.recentLogs.map { $0.category })).sorted()
    }

    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack(spacing: 12) {
                // Search
                TextField("Search logs...", text: $searchText)
                    .textFieldStyle(.roundedBorder)
                    .frame(maxWidth: 200)

                // Level filter
                Picker("Level", selection: $selectedLevel) {
                    Text("All Levels").tag(nil as LogLevel?)
                    Divider()
                    ForEach([LogLevel.trace, .debug, .info, .warning, .error, .critical], id: \.self) { level in
                        Text("\(level.emoji) \(level.description)").tag(level as LogLevel?)
                    }
                }
                .frame(width: 120)

                // Category filter
                Picker("Category", selection: $selectedCategory) {
                    Text("All Categories").tag(nil as String?)
                    Divider()
                    ForEach(categories, id: \.self) { category in
                        Text(category).tag(category as String?)
                    }
                }
                .frame(width: 120)

                Spacer()

                // Auto-scroll toggle
                Toggle("Auto-scroll", isOn: $autoScroll)
                    .toggleStyle(.checkbox)

                // Actions
                Button {
                    logger.clearRecentLogs()
                } label: {
                    Image(systemName: "trash")
                }
                .help("Clear logs")

                Button {
                    exportLogs()
                } label: {
                    Image(systemName: "square.and.arrow.up")
                }
                .help("Export logs")

                Button {
                    openLogsFolder()
                } label: {
                    Image(systemName: "folder")
                }
                .help("Open logs folder")
            }
            .padding()
            .background(.ultraThinMaterial)

            Divider()

            // Log list
            ScrollViewReader { proxy in
                List(filteredLogs) { entry in
                    LogEntryRow(entry: entry)
                        .id(entry.id)
                }
                .listStyle(.plain)
                .onChange(of: logger.recentLogs.count) { _, _ in
                    if autoScroll, let last = filteredLogs.last {
                        proxy.scrollTo(last.id, anchor: .bottom)
                    }
                }
            }

            // Status bar
            HStack {
                Text("\(filteredLogs.count) of \(logger.recentLogs.count) entries")
                    .font(.caption)
                    .foregroundStyle(.secondary)

                Spacer()

                if let url = logger.logFileURL_ {
                    Text(url.lastPathComponent)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }
            .padding(.horizontal)
            .padding(.vertical, 4)
            .background(.ultraThinMaterial)
        }
        .frame(minWidth: 700, minHeight: 400)
    }

    private func exportLogs() {
        let panel = NSSavePanel()
        panel.allowedContentTypes = [.plainText]
        panel.nameFieldStringValue = "convergio_logs_\(Date().ISO8601Format()).txt"

        if panel.runModal() == .OK, let url = panel.url {
            let content = logger.exportLogs()
            try? content.write(to: url, atomically: true, encoding: .utf8)
        }
    }

    private func openLogsFolder() {
        if let url = logger.getLogsDirectory() {
            NSWorkspace.shared.open(url)
        }
    }
}

// MARK: - Log Entry Row

struct LogEntryRow: View {
    let entry: LogEntry

    private var levelColor: Color {
        switch entry.level {
        case "TRACE": return .gray
        case "DEBUG": return .blue
        case "INFO": return .green
        case "WARN", "WARNING": return .orange
        case "ERROR": return .red
        case "CRITICAL": return .purple
        default: return .primary
        }
    }

    private var levelEmoji: String {
        switch entry.level {
        case "TRACE": return "🔍"
        case "DEBUG": return "🐛"
        case "INFO": return "ℹ️"
        case "WARN", "WARNING": return "⚠️"
        case "ERROR": return "❌"
        case "CRITICAL": return "🔥"
        default: return "📝"
        }
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                // Level badge
                Text("\(levelEmoji) \(entry.level)")
                    .font(.caption.monospaced().weight(.semibold))
                    .foregroundStyle(levelColor)
                    .padding(.horizontal, 6)
                    .padding(.vertical, 2)
                    .background(levelColor.opacity(0.1))
                    .clipShape(RoundedRectangle(cornerRadius: 4))

                // Category
                Text(entry.category)
                    .font(.caption.weight(.medium))
                    .foregroundStyle(.secondary)
                    .padding(.horizontal, 6)
                    .padding(.vertical, 2)
                    .background(Color.secondary.opacity(0.1))
                    .clipShape(RoundedRectangle(cornerRadius: 4))

                Spacer()

                // Timestamp
                Text(formatTimestamp(entry.timestamp))
                    .font(.caption.monospaced())
                    .foregroundStyle(.tertiary)
            }

            // Message
            Text(entry.message)
                .font(.body)
                .textSelection(.enabled)

            // Location
            HStack {
                Text("\(entry.file):\(entry.line)")
                    .font(.caption2.monospaced())
                    .foregroundStyle(.tertiary)

                Text(entry.function)
                    .font(.caption2.monospaced())
                    .foregroundStyle(.tertiary)
            }

            // Metadata
            if let metadata = entry.metadata, !metadata.isEmpty {
                HStack {
                    ForEach(Array(metadata.keys.sorted()), id: \.self) { key in
                        HStack(spacing: 2) {
                            Text(key)
                                .font(.caption2.weight(.medium))
                            Text("=")
                                .font(.caption2)
                            Text(metadata[key] ?? "")
                                .font(.caption2)
                        }
                        .padding(.horizontal, 4)
                        .padding(.vertical, 1)
                        .background(Color.secondary.opacity(0.1))
                        .clipShape(RoundedRectangle(cornerRadius: 3))
                    }
                }
            }
        }
        .padding(.vertical, 4)
    }

    private func formatTimestamp(_ date: Date) -> String {
        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm:ss.SSS"
        return formatter.string(from: date)
    }
}

// MARK: - Preview

#Preview {
    LogViewerView()
}
