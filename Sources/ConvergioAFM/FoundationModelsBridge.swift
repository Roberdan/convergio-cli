/**
 * FoundationModels Swift Bridge
 *
 * Swift wrapper for Apple's FoundationModels framework that exposes
 * C-compatible functions for use from Objective-C/C code.
 *
 * Requires macOS 26+ (Tahoe) with Apple Intelligence enabled.
 * On earlier systems, all functions return AFM_ERR_NOT_MACOS_26.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation

// MARK: - C-Compatible Error Codes

@frozen
public struct AFMError: RawRepresentable, Equatable {
    public let rawValue: Int32
    public init(rawValue: Int32) { self.rawValue = rawValue }

    public static let available = AFMError(rawValue: 0)
    public static let notMacOS26 = AFMError(rawValue: -1)
    public static let notAppleSilicon = AFMError(rawValue: -2)
    public static let intelligenceDisabled = AFMError(rawValue: -3)
    public static let modelNotReady = AFMError(rawValue: -4)
    public static let sessionFailed = AFMError(rawValue: -5)
    public static let generationFailed = AFMError(rawValue: -6)
    public static let toolCallFailed = AFMError(rawValue: -7)
    public static let guidedGenFailed = AFMError(rawValue: -8)
    public static let unknown = AFMError(rawValue: -100)
}

// MARK: - Availability Check

#if canImport(FoundationModels)
@_exported import FoundationModels

// MARK: - Session Management (macOS 26+)

/// Session wrapper that holds instructions
private class AFMSessionWrapper {
    var session: LanguageModelSession
    var instructions: String?

    init(instructions: String? = nil) {
        if let inst = instructions {
            self.session = LanguageModelSession(instructions: inst)
        } else {
            self.session = LanguageModelSession()
        }
        self.instructions = instructions
    }

    func updateInstructions(_ newInstructions: String) {
        self.instructions = newInstructions
        // Create new session with updated instructions
        self.session = LanguageModelSession(instructions: newInstructions)
    }
}

private var activeSessions: [Int64: AFMSessionWrapper] = [:]
private var nextSessionId: Int64 = 1
private let sessionLock = NSLock()

// MARK: - C-Exported Functions (macOS 26+)

/// Check if FoundationModels is available
@_cdecl("swift_afm_check_availability")
public func swiftAFMCheckAvailability(
    outIsAvailable: UnsafeMutablePointer<Bool>?,
    outIntelligenceEnabled: UnsafeMutablePointer<Bool>?,
    outModelReady: UnsafeMutablePointer<Bool>?
) -> Int32 {
    let model = SystemLanguageModel.default
    let availability = model.availability

    switch availability {
    case .available:
        outIsAvailable?.pointee = true
        outIntelligenceEnabled?.pointee = true
        outModelReady?.pointee = true
        return AFMError.available.rawValue

    case .unavailable(let reason):
        outIsAvailable?.pointee = false
        outIntelligenceEnabled?.pointee = false
        outModelReady?.pointee = false

        switch reason {
        case .deviceNotEligible:
            return AFMError.notAppleSilicon.rawValue
        case .appleIntelligenceNotEnabled:
            return AFMError.intelligenceDisabled.rawValue
        case .modelNotReady:
            outModelReady?.pointee = false
            return AFMError.modelNotReady.rawValue
        @unknown default:
            return AFMError.unknown.rawValue
        }

    @unknown default:
        outIsAvailable?.pointee = false
        outIntelligenceEnabled?.pointee = false
        outModelReady?.pointee = false
        return AFMError.unknown.rawValue
    }
}

/// Create a new language model session
@_cdecl("swift_afm_session_create")
public func swiftAFMSessionCreate(outSessionId: UnsafeMutablePointer<Int64>?) -> Int32 {
    sessionLock.lock()
    defer { sessionLock.unlock() }

    let wrapper = AFMSessionWrapper()
    let sessionId = nextSessionId
    nextSessionId += 1
    activeSessions[sessionId] = wrapper

    outSessionId?.pointee = sessionId
    return AFMError.available.rawValue
}

/// Destroy a session
@_cdecl("swift_afm_session_destroy")
public func swiftAFMSessionDestroy(sessionId: Int64) {
    sessionLock.lock()
    defer { sessionLock.unlock() }
    activeSessions.removeValue(forKey: sessionId)
}

/// Set session instructions (system prompt)
/// Note: This creates a new session with the updated instructions
@_cdecl("swift_afm_session_set_instructions")
public func swiftAFMSessionSetInstructions(
    sessionId: Int64,
    instructions: UnsafePointer<CChar>?
) -> Int32 {
    guard let instructions = instructions else { return AFMError.unknown.rawValue }

    sessionLock.lock()
    guard let wrapper = activeSessions[sessionId] else {
        sessionLock.unlock()
        return AFMError.sessionFailed.rawValue
    }
    sessionLock.unlock()

    wrapper.updateInstructions(String(cString: instructions))
    return AFMError.available.rawValue
}

/// Generate response synchronously
@_cdecl("swift_afm_generate")
public func swiftAFMGenerate(
    sessionId: Int64,
    prompt: UnsafePointer<CChar>?,
    outResponse: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?
) -> Int32 {
    guard let prompt = prompt else { return AFMError.unknown.rawValue }

    sessionLock.lock()
    guard let wrapper = activeSessions[sessionId] else {
        sessionLock.unlock()
        return AFMError.sessionFailed.rawValue
    }
    sessionLock.unlock()

    let promptStr = String(cString: prompt)

    // Use semaphore for synchronous execution
    let semaphore = DispatchSemaphore(value: 0)
    var responseContent: String?
    var errorOccurred = false

    Task {
        do {
            let response = try await wrapper.session.respond(to: promptStr)
            responseContent = response.content
        } catch {
            errorOccurred = true
        }
        semaphore.signal()
    }

    semaphore.wait()

    if errorOccurred {
        return AFMError.generationFailed.rawValue
    }

    if let content = responseContent {
        let cString = strdup(content)
        outResponse?.pointee = cString
    }

    return AFMError.available.rawValue
}

/// Streaming callback type
public typealias AFMStreamCallback = @convention(c) (
    UnsafePointer<CChar>?,  // partial content
    Bool,                    // is final
    UnsafeMutableRawPointer? // user context
) -> Void

/// Generate response with streaming
@_cdecl("swift_afm_generate_stream")
public func swiftAFMGenerateStream(
    sessionId: Int64,
    prompt: UnsafePointer<CChar>?,
    callback: AFMStreamCallback?,
    userCtx: UnsafeMutableRawPointer?
) -> Int32 {
    guard let prompt = prompt, let callback = callback else {
        return AFMError.unknown.rawValue
    }

    sessionLock.lock()
    guard let wrapper = activeSessions[sessionId] else {
        sessionLock.unlock()
        return AFMError.sessionFailed.rawValue
    }
    sessionLock.unlock()

    let promptStr = String(cString: prompt)

    let semaphore = DispatchSemaphore(value: 0)
    var errorOccurred = false

    Task {
        do {
            let stream = wrapper.session.streamResponse(to: promptStr)
            for try await snapshot in stream {
                let content = snapshot.content
                content.withCString { cStr in
                    callback(cStr, false, userCtx)
                }
            }
            // Signal completion
            callback(nil, true, userCtx)
        } catch {
            errorOccurred = true
        }
        semaphore.signal()
    }

    semaphore.wait()

    return errorOccurred ? AFMError.generationFailed.rawValue : AFMError.available.rawValue
}

/// Free a C string allocated by this bridge
@_cdecl("swift_afm_free_string")
public func swiftAFMFreeString(str: UnsafeMutablePointer<CChar>?) {
    free(str)
}

/// Get model information
@_cdecl("swift_afm_get_model_info")
public func swiftAFMGetModelInfo(
    outName: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?,
    outSizeBillions: UnsafeMutablePointer<Float>?
) -> Int32 {
    // FoundationModels uses a ~3B on-device model
    outName?.pointee = strdup("Apple Foundation Model (On-Device)")
    outSizeBillions?.pointee = 3.0
    return AFMError.available.rawValue
}

#else
// MARK: - Stub Implementations (pre-macOS 26)

/// Streaming callback type (stub)
public typealias AFMStreamCallback = @convention(c) (
    UnsafePointer<CChar>?,
    Bool,
    UnsafeMutableRawPointer?
) -> Void

@_cdecl("swift_afm_check_availability")
public func swiftAFMCheckAvailability(
    outIsAvailable: UnsafeMutablePointer<Bool>?,
    outIntelligenceEnabled: UnsafeMutablePointer<Bool>?,
    outModelReady: UnsafeMutablePointer<Bool>?
) -> Int32 {
    outIsAvailable?.pointee = false
    outIntelligenceEnabled?.pointee = false
    outModelReady?.pointee = false
    return AFMError.notMacOS26.rawValue
}

@_cdecl("swift_afm_session_create")
public func swiftAFMSessionCreate(outSessionId: UnsafeMutablePointer<Int64>?) -> Int32 {
    outSessionId?.pointee = 0
    return AFMError.notMacOS26.rawValue
}

@_cdecl("swift_afm_session_destroy")
public func swiftAFMSessionDestroy(sessionId: Int64) {
    // No-op on pre-macOS 26
}

@_cdecl("swift_afm_session_set_instructions")
public func swiftAFMSessionSetInstructions(
    sessionId: Int64,
    instructions: UnsafePointer<CChar>?
) -> Int32 {
    return AFMError.notMacOS26.rawValue
}

@_cdecl("swift_afm_generate")
public func swiftAFMGenerate(
    sessionId: Int64,
    prompt: UnsafePointer<CChar>?,
    outResponse: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?
) -> Int32 {
    outResponse?.pointee = nil
    return AFMError.notMacOS26.rawValue
}

@_cdecl("swift_afm_generate_stream")
public func swiftAFMGenerateStream(
    sessionId: Int64,
    prompt: UnsafePointer<CChar>?,
    callback: AFMStreamCallback?,
    userCtx: UnsafeMutableRawPointer?
) -> Int32 {
    return AFMError.notMacOS26.rawValue
}

@_cdecl("swift_afm_free_string")
public func swiftAFMFreeString(str: UnsafeMutablePointer<CChar>?) {
    free(str)
}

@_cdecl("swift_afm_get_model_info")
public func swiftAFMGetModelInfo(
    outName: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?,
    outSizeBillions: UnsafeMutablePointer<Float>?
) -> Int32 {
    outName?.pointee = nil
    outSizeBillions?.pointee = 0.0
    return AFMError.notMacOS26.rawValue
}

#endif
