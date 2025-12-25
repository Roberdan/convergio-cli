/**
 * CONVERGIO NATIVE - AFM Bridge Stubs
 *
 * Provides stub implementations for Apple Foundation Models bridge functions.
 * These allow the app to link and run on macOS versions where AFM is not available.
 * On macOS 26+, these could be replaced with real implementations.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation

// MARK: - AFM Bridge Stub Implementations

/// These functions are declared as weak imports in apple_foundation.m
/// We provide stub implementations that return "not available"
/// The C code checks swift_afm_check_availability() first, so these stubs
/// should rarely be called - they're here to satisfy the linker.

/// Check if AFM is available (stub: always returns not available)
@_cdecl("swift_afm_check_availability")
public func swift_afm_check_availability(
    _ outIsAvailable: UnsafeMutablePointer<Bool>?,
    _ outIntelligenceEnabled: UnsafeMutablePointer<Bool>?,
    _ outModelReady: UnsafeMutablePointer<Bool>?
) -> Int32 {
    outIsAvailable?.pointee = false
    outIntelligenceEnabled?.pointee = false
    outModelReady?.pointee = false
    // Return -1 (AFM_ERR_NOT_MACOS_26) to indicate not available
    return -1
}

/// Create session (stub: always fails)
@_cdecl("swift_afm_session_create")
public func swift_afm_session_create(
    _ outSessionId: UnsafeMutablePointer<Int64>?
) -> Int32 {
    outSessionId?.pointee = 0
    return -1  // Error
}

/// Destroy session (stub: no-op)
@_cdecl("swift_afm_session_destroy")
public func swift_afm_session_destroy(
    _ sessionId: Int64
) {
    // No-op for stub
}

/// Set session instructions (stub: no-op, returns success)
@_cdecl("swift_afm_session_set_instructions")
public func swift_afm_session_set_instructions(
    _ sessionId: Int64,
    _ instructions: UnsafePointer<CChar>?
) -> Int32 {
    return 0  // Success (no-op)
}

/// Generate response (stub: always fails)
@_cdecl("swift_afm_generate")
public func swift_afm_generate(
    _ sessionId: Int64,
    _ prompt: UnsafePointer<CChar>?,
    _ outResponse: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?
) -> Int32 {
    outResponse?.pointee = nil
    return -1  // Error
}

/// Stream callback type matching C declaration
public typealias AFMSwiftStreamCallback = @convention(c) (
    UnsafePointer<CChar>?,  // content
    Bool,                    // isFinal
    UnsafeMutableRawPointer? // userCtx
) -> Void

/// Generate with streaming (stub: always fails)
@_cdecl("swift_afm_generate_stream")
public func swift_afm_generate_stream(
    _ sessionId: Int64,
    _ prompt: UnsafePointer<CChar>?,
    _ callback: AFMSwiftStreamCallback?,
    _ userCtx: UnsafeMutableRawPointer?
) -> Int32 {
    return -1  // Error
}

/// Free string allocated by Swift (stub: no-op since we never allocate)
@_cdecl("swift_afm_free_string")
public func swift_afm_free_string(
    _ str: UnsafeMutablePointer<CChar>?
) {
    // In a real implementation, this would free a Swift-allocated string
    // Since our stubs never allocate, this is a no-op
}

/// Get model info (stub: returns unavailable info)
@_cdecl("swift_afm_get_model_info")
public func swift_afm_get_model_info(
    _ outName: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?,
    _ outSizeBillions: UnsafeMutablePointer<Float>?
) -> Int32 {
    outName?.pointee = nil
    outSizeBillions?.pointee = 0
    return -1  // Not available
}
