/**
 * CONVERGIO NATIVE - Logger Service
 *
 * Comprehensive logging system with file output, unified logging,
 * and crash reporting integration.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import os.log

// MARK: - Log Level

public enum LogLevel: Int, Comparable, CustomStringConvertible {
    case trace = 0
    case debug = 1
    case info = 2
    case warning = 3
    case error = 4
    case critical = 5

    public var description: String {
        switch self {
        case .trace: return "TRACE"
        case .debug: return "DEBUG"
        case .info: return "INFO"
        case .warning: return "WARN"
        case .error: return "ERROR"
        case .critical: return "CRITICAL"
        }
    }

    public var emoji: String {
        switch self {
        case .trace: return "🔍"
        case .debug: return "🐛"
        case .info: return "ℹ️"
        case .warning: return "⚠️"
        case .error: return "❌"
        case .critical: return "🔥"
        }
    }

    public var osLogType: OSLogType {
        switch self {
        case .trace, .debug: return .debug
        case .info: return .info
        case .warning: return .default
        case .error: return .error
        case .critical: return .fault
        }
    }

    public static func < (lhs: LogLevel, rhs: LogLevel) -> Bool {
        lhs.rawValue < rhs.rawValue
    }
}

// MARK: - Log Entry

public struct LogEntry: Codable, Identifiable {
    public let id: UUID
    public let timestamp: Date
    public let level: String
    public let category: String
    public let message: String
    public let file: String
    public let function: String
    public let line: Int
    public let metadata: [String: String]?

    public init(
        level: LogLevel,
        category: String,
        message: String,
        file: String,
        function: String,
        line: Int,
        metadata: [String: String]? = nil
    ) {
        self.id = UUID()
        self.timestamp = Date()
        self.level = level.description
        self.category = category
        self.message = message
        self.file = (file as NSString).lastPathComponent
        self.function = function
        self.line = line
        self.metadata = metadata
    }

    public var formatted: String {
        let dateFormatter = ISO8601DateFormatter()
        dateFormatter.formatOptions = [.withInternetDateTime, .withFractionalSeconds]
        let ts = dateFormatter.string(from: timestamp)
        var result = "[\(ts)] [\(level)] [\(category)] \(message)"
        if let meta = metadata, !meta.isEmpty {
            let metaStr = meta.map { "\($0.key)=\($0.value)" }.joined(separator: ", ")
            result += " {\(metaStr)}"
        }
        result += " (\(file):\(line) \(function))"
        return result
    }
}

// MARK: - Logger

@MainActor
public final class Logger: ObservableObject {
    public static let shared = Logger()

    // MARK: - Published Properties

    @Published public private(set) var recentLogs: [LogEntry] = []
    @Published public var minimumLevel: LogLevel = .debug

    // MARK: - Private Properties

    private let osLog: OSLog
    private let fileHandle: FileHandle?
    private let logFileURL: URL?
    private let queue = DispatchQueue(label: "com.convergio.logger", qos: .utility)
    private let maxRecentLogs = 1000
    private let dateFormatter: DateFormatter

    // MARK: - Initialization

    private init() {
        self.osLog = OSLog(subsystem: "com.convergio.app", category: "Convergio")
        self.dateFormatter = DateFormatter()
        self.dateFormatter.dateFormat = "yyyy-MM-dd HH:mm:ss.SSS"

        // Set up log file
        let logsDir = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first?
            .appendingPathComponent("Convergio/Logs", isDirectory: true)

        if let logsDir = logsDir {
            try? FileManager.default.createDirectory(at: logsDir, withIntermediateDirectories: true)

            let logFileName = "convergio_\(Self.currentDateString()).log"
            let logFileURL = logsDir.appendingPathComponent(logFileName)
            self.logFileURL = logFileURL

            // Create or open log file
            if !FileManager.default.fileExists(atPath: logFileURL.path) {
                FileManager.default.createFile(atPath: logFileURL.path, contents: nil)
            }

            self.fileHandle = try? FileHandle(forWritingTo: logFileURL)
            self.fileHandle?.seekToEndOfFile()

            // Write startup header
            let header = "\n\n" + String(repeating: "=", count: 80) + "\n"
                + "CONVERGIO NATIVE - Log Session Started\n"
                + "Date: \(dateFormatter.string(from: Date()))\n"
                + "Version: 1.0.0 (Build 1)\n"
                + "macOS: \(ProcessInfo.processInfo.operatingSystemVersionString)\n"
                + String(repeating: "=", count: 80) + "\n\n"

            if let data = header.data(using: .utf8) {
                self.fileHandle?.write(data)
            }
        } else {
            self.logFileURL = nil
            self.fileHandle = nil
        }

        // Clean up old logs (keep last 7 days)
        cleanupOldLogs()

        info("Logger initialized", category: "System")
    }

    deinit {
        try? fileHandle?.close()
    }

    // MARK: - Static Helpers

    private static func currentDateString() -> String {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd"
        return formatter.string(from: Date())
    }

    // MARK: - Logging Methods

    public func log(
        _ level: LogLevel,
        _ message: String,
        category: String = "App",
        metadata: [String: String]? = nil,
        file: String = #file,
        function: String = #function,
        line: Int = #line
    ) {
        guard level >= minimumLevel else { return }

        let entry = LogEntry(
            level: level,
            category: category,
            message: message,
            file: file,
            function: function,
            line: line,
            metadata: metadata
        )

        // Add to recent logs
        recentLogs.append(entry)
        if recentLogs.count > maxRecentLogs {
            recentLogs.removeFirst(recentLogs.count - maxRecentLogs)
        }

        // Write to OS Log
        os_log("%{public}@", log: osLog, type: level.osLogType, entry.formatted)

        // Write to file
        queue.async { [weak self] in
            guard let self = self else { return }
            let line = entry.formatted + "\n"
            if let data = line.data(using: .utf8) {
                self.fileHandle?.write(data)
            }
        }

        // Print to console in debug builds
        #if DEBUG
        print("\(level.emoji) \(entry.formatted)")
        #endif
    }

    // MARK: - Convenience Methods

    public func trace(_ message: String, category: String = "App", metadata: [String: String]? = nil, file: String = #file, function: String = #function, line: Int = #line) {
        log(.trace, message, category: category, metadata: metadata, file: file, function: function, line: line)
    }

    public func debug(_ message: String, category: String = "App", metadata: [String: String]? = nil, file: String = #file, function: String = #function, line: Int = #line) {
        log(.debug, message, category: category, metadata: metadata, file: file, function: function, line: line)
    }

    public func info(_ message: String, category: String = "App", metadata: [String: String]? = nil, file: String = #file, function: String = #function, line: Int = #line) {
        log(.info, message, category: category, metadata: metadata, file: file, function: function, line: line)
    }

    public func warning(_ message: String, category: String = "App", metadata: [String: String]? = nil, file: String = #file, function: String = #function, line: Int = #line) {
        log(.warning, message, category: category, metadata: metadata, file: file, function: function, line: line)
    }

    public func error(_ message: String, category: String = "App", metadata: [String: String]? = nil, file: String = #file, function: String = #function, line: Int = #line) {
        log(.error, message, category: category, metadata: metadata, file: file, function: function, line: line)
    }

    public func critical(_ message: String, category: String = "App", metadata: [String: String]? = nil, file: String = #file, function: String = #function, line: Int = #line) {
        log(.critical, message, category: category, metadata: metadata, file: file, function: function, line: line)
    }

    // MARK: - Error Logging

    public func error(_ error: Error, message: String? = nil, category: String = "App", file: String = #file, function: String = #function, line: Int = #line) {
        let errorMessage = message ?? "Error occurred"
        let metadata: [String: String] = [
            "error": String(describing: error),
            "errorType": String(describing: type(of: error)),
            "localizedDescription": error.localizedDescription
        ]
        log(.error, errorMessage, category: category, metadata: metadata, file: file, function: function, line: line)
    }

    // MARK: - Performance Logging

    public func measureTime<T>(_ operation: String, category: String = "Performance", block: () throws -> T) rethrows -> T {
        let start = CFAbsoluteTimeGetCurrent()
        let result = try block()
        let elapsed = (CFAbsoluteTimeGetCurrent() - start) * 1000
        debug("\(operation) completed in \(String(format: "%.2f", elapsed))ms", category: category)
        return result
    }

    public func measureTimeAsync<T>(_ operation: String, category: String = "Performance", block: () async throws -> T) async rethrows -> T {
        let start = CFAbsoluteTimeGetCurrent()
        let result = try await block()
        let elapsed = (CFAbsoluteTimeGetCurrent() - start) * 1000
        debug("\(operation) completed in \(String(format: "%.2f", elapsed))ms", category: category)
        return result
    }

    // MARK: - Log Management

    public var logFileURL_: URL? {
        logFileURL
    }

    public func exportLogs() -> String {
        recentLogs.map { $0.formatted }.joined(separator: "\n")
    }

    public func clearRecentLogs() {
        recentLogs.removeAll()
    }

    public func getLogsDirectory() -> URL? {
        logFileURL?.deletingLastPathComponent()
    }

    private func cleanupOldLogs() {
        guard let logsDir = getLogsDirectory() else { return }

        queue.async {
            let fileManager = FileManager.default
            guard let files = try? fileManager.contentsOfDirectory(at: logsDir, includingPropertiesForKeys: [.creationDateKey]) else { return }

            let calendar = Calendar.current
            let cutoffDate = calendar.date(byAdding: .day, value: -7, to: Date())!

            for file in files {
                guard file.pathExtension == "log" else { continue }
                guard let attrs = try? fileManager.attributesOfItem(atPath: file.path),
                      let creationDate = attrs[.creationDate] as? Date else { continue }

                if creationDate < cutoffDate {
                    try? fileManager.removeItem(at: file)
                }
            }
        }
    }
}

// MARK: - Global Logging Functions

public func logTrace(_ message: String, category: String = "App", file: String = #file, function: String = #function, line: Int = #line) {
    Task { @MainActor in
        Logger.shared.trace(message, category: category, file: file, function: function, line: line)
    }
}

public func logDebug(_ message: String, category: String = "App", file: String = #file, function: String = #function, line: Int = #line) {
    Task { @MainActor in
        Logger.shared.debug(message, category: category, file: file, function: function, line: line)
    }
}

public func logInfo(_ message: String, category: String = "App", file: String = #file, function: String = #function, line: Int = #line) {
    Task { @MainActor in
        Logger.shared.info(message, category: category, file: file, function: function, line: line)
    }
}

public func logWarning(_ message: String, category: String = "App", file: String = #file, function: String = #function, line: Int = #line) {
    Task { @MainActor in
        Logger.shared.warning(message, category: category, file: file, function: function, line: line)
    }
}

public func logError(_ message: String, category: String = "App", file: String = #file, function: String = #function, line: Int = #line) {
    Task { @MainActor in
        Logger.shared.error(message, category: category, file: file, function: function, line: line)
    }
}

public func logCritical(_ message: String, category: String = "App", file: String = #file, function: String = #function, line: Int = #line) {
    Task { @MainActor in
        Logger.shared.critical(message, category: category, file: file, function: function, line: line)
    }
}
