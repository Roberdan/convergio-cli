// MLXBridge.swift - Swift bridge for MLX LLM inference callable from Objective-C
// Part of Convergio CLI - Local LLM inference on Apple Silicon

import Foundation
import MLX
import MLXLLM
import MLXLMCommon

// MARK: - C-compatible types for ObjC interop

/// Result structure for generation
@objc public class MLXGenerationResult: NSObject {
    @objc public var text: String = ""
    @objc public var tokenCount: Int = 0
    @objc public var success: Bool = false
    @objc public var errorMessage: String = ""
    @objc public var tokensPerSecond: Double = 0.0
}

/// Callback type for streaming tokens
public typealias MLXTokenCallback = @convention(c) (UnsafePointer<CChar>) -> Void

// MARK: - Global state

private var currentModelContainer: ModelContainer?
private var currentModelId: String?

// MARK: - Bridge class exposed to Objective-C

@objc public class MLXBridge: NSObject {

    // MARK: - Model Loading

    /// Check if MLX is available on this system
    @objc public static func isAvailable() -> Bool {
        #if arch(arm64)
        if #available(macOS 14.0, *) {
            return true
        }
        #endif
        return false
    }

    /// Get the currently loaded model ID (nil if none)
    @objc public static func currentModel() -> String? {
        return currentModelId
    }

    /// Load a model from the mlx-community on HuggingFace
    /// - Parameters:
    ///   - modelId: HuggingFace model ID (e.g., "mlx-community/Llama-3.2-1B-Instruct-4bit")
    ///   - cachePath: Local cache directory for downloaded models
    /// - Returns: true on success, false on failure
    @objc public static func loadModel(_ modelId: String, cachePath: String) -> Bool {
        // Unload previous model if any
        if currentModelContainer != nil {
            unloadModel()
        }

        let semaphore = DispatchSemaphore(value: 0)
        nonisolated(unsafe) var success = false

        // CRITICAL FIX: Use TWO detached tasks pattern from Wade Tregaskis
        // https://wadetregaskis.com/calling-swift-concurrency-async-code-synchronously-in-swift/
        // First task executes async code, second task monitors and signals semaphore
        let coreTask = Task<Void, Never>.detached(priority: .userInitiated) {
            do {
                // Configure model loading
                let configuration = ModelConfiguration(id: modelId)

                // Load the model container
                let container = try await LLMModelFactory.shared.loadContainer(
                    configuration: configuration
                ) { progress in
                    // Progress callback
                    let percent = Int(progress.fractionCompleted * 100)
                    print("[MLX] Loading model: \(percent)%")
                }

                currentModelContainer = container
                currentModelId = modelId
                success = true
                print("[MLX] Model loaded successfully: \(modelId)")

            } catch {
                print("[MLX] Failed to load model: \(error.localizedDescription)")
                success = false
            }
        }

        // Second detached task monitors the first and signals when complete
        Task<Void, Never>.detached(priority: .userInitiated) {
            await coreTask.value
            semaphore.signal()
        }

        semaphore.wait()
        return success
    }

    /// Unload the current model to free memory
    @objc public static func unloadModel() {
        currentModelContainer = nil
        currentModelId = nil
        // Force memory cleanup
        MLX.GPU.clearCache()
        print("[MLX] Model unloaded")
    }

    // MARK: - Text Generation

    /// Generate text from a prompt (blocking, returns full response)
    /// - Parameters:
    ///   - prompt: The input prompt
    ///   - systemPrompt: Optional system prompt
    ///   - maxTokens: Maximum tokens to generate (default 2048)
    ///   - temperature: Sampling temperature (default 0.7)
    /// - Returns: MLXGenerationResult with generated text and stats
    @objc public static func generate(
        prompt: String,
        systemPrompt: String?,
        maxTokens: Int,
        temperature: Float
    ) -> MLXGenerationResult {
        let result = MLXGenerationResult()

        guard let container = currentModelContainer else {
            result.success = false
            result.errorMessage = "No model loaded. Call loadModel() first."
            return result
        }

        let semaphore = DispatchSemaphore(value: 0)

        // CRITICAL FIX: Two detached tasks pattern for deadlock-free sync/async bridging
        // https://wadetregaskis.com/calling-swift-concurrency-async-code-synchronously-in-swift/

        // Build messages array BEFORE the task to avoid captured var mutation warnings (Swift 6)
        var messages: [[String: String]] = []
        if let system = systemPrompt, !system.isEmpty {
            messages.append(["role": "system", "content": system])
        }
        messages.append(["role": "user", "content": prompt])
        let finalMessages = messages  // Immutable copy for sendable closure

        // Capture maxTokens as local let
        let maxTokensLimit = maxTokens

        let coreTask = Task<Void, Never>.detached(priority: .userInitiated) {
            do {
                let startTime = Date()

                // Generate with the model
                let generateParameters = GenerateParameters(
                    temperature: temperature,
                    topP: 0.9,
                    repetitionPenalty: 1.1
                )

                // Use async streaming version - collect results inside perform block
                // to avoid Swift 6 captured var mutation warnings
                let (generatedText, tokenCount): (String, Int) = try await container.perform { context in
                    let input = try await context.processor.prepare(
                        input: .init(messages: finalMessages)
                    )

                    var text = ""
                    var count = 0
                    for try await output in try MLXLMCommon.generate(
                        input: input,
                        parameters: generateParameters,
                        context: context
                    ) {
                        count += 1
                        if let chunk = output.chunk {
                            text += chunk
                        }
                        if count >= maxTokensLimit {
                            break
                        }
                    }
                    return (text, count)
                }

                let elapsed = Date().timeIntervalSince(startTime)

                result.text = generatedText
                result.tokenCount = tokenCount
                result.tokensPerSecond = elapsed > 0 ? Double(tokenCount) / elapsed : 0
                result.success = true

            } catch {
                result.success = false
                result.errorMessage = error.localizedDescription
            }
        }

        // Monitor task signals when complete
        Task<Void, Never>.detached(priority: .userInitiated) {
            await coreTask.value
            semaphore.signal()
        }

        semaphore.wait()
        return result
    }

    /// Generate text with streaming callback for each token
    /// - Parameters:
    ///   - prompt: The input prompt
    ///   - systemPrompt: Optional system prompt
    ///   - maxTokens: Maximum tokens to generate
    ///   - temperature: Sampling temperature
    ///   - callback: C function pointer called for each generated token
    /// - Returns: MLXGenerationResult with final stats
    @objc public static func generateStreaming(
        prompt: String,
        systemPrompt: String?,
        maxTokens: Int,
        temperature: Float,
        callback: @escaping (String) -> Void
    ) -> MLXGenerationResult {
        let result = MLXGenerationResult()

        guard let container = currentModelContainer else {
            result.success = false
            result.errorMessage = "No model loaded. Call loadModel() first."
            return result
        }

        let semaphore = DispatchSemaphore(value: 0)

        // CRITICAL FIX: Two detached tasks pattern for deadlock-free sync/async bridging
        // Build messages array BEFORE the task to avoid captured var mutation warnings (Swift 6)
        var messages: [[String: String]] = []
        if let system = systemPrompt, !system.isEmpty {
            messages.append(["role": "system", "content": system])
        }
        messages.append(["role": "user", "content": prompt])
        let finalMessages = messages  // Immutable copy for sendable closure

        // Capture maxTokens as local let
        let maxTokensLimit = maxTokens

        let coreTask = Task<Void, Never>.detached(priority: .userInitiated) {
            do {
                let startTime = Date()

                let generateParameters = GenerateParameters(
                    temperature: temperature,
                    topP: 0.9,
                    repetitionPenalty: 1.1
                )

                // Use async streaming version - collect results inside perform block
                // to avoid Swift 6 captured var mutation warnings
                let (fullText, tokenCount): (String, Int) = try await container.perform { context in
                    let input = try await context.processor.prepare(
                        input: .init(messages: finalMessages)
                    )

                    var text = ""
                    var count = 0
                    // Stream tokens
                    for try await output in try MLXLMCommon.generate(
                        input: input,
                        parameters: generateParameters,
                        context: context
                    ) {
                        count += 1

                        if let chunk = output.chunk {
                            text += chunk
                            // Call the callback (already on background thread)
                            callback(chunk)
                        }

                        if count >= maxTokensLimit {
                            break
                        }
                    }
                    return (text, count)
                }

                let elapsed = Date().timeIntervalSince(startTime)

                result.text = fullText
                result.tokenCount = tokenCount
                result.tokensPerSecond = elapsed > 0 ? Double(tokenCount) / elapsed : 0
                result.success = true

            } catch {
                result.success = false
                result.errorMessage = error.localizedDescription
            }
        }

        // Monitor task signals when complete
        Task<Void, Never>.detached(priority: .userInitiated) {
            await coreTask.value
            semaphore.signal()
        }

        semaphore.wait()
        return result
    }

    // MARK: - Model Management

    /// List available models in the cache directory
    @objc public static func listCachedModels(cachePath: String) -> [String] {
        let cacheURL = URL(fileURLWithPath: cachePath)
        var models: [String] = []

        guard let contents = try? FileManager.default.contentsOfDirectory(
            at: cacheURL,
            includingPropertiesForKeys: [.isDirectoryKey]
        ) else {
            return models
        }

        for url in contents {
            var isDirectory: ObjCBool = false
            if FileManager.default.fileExists(atPath: url.path, isDirectory: &isDirectory),
               isDirectory.boolValue {
                // Check if it looks like an MLX model directory
                let configPath = url.appendingPathComponent("config.json")
                if FileManager.default.fileExists(atPath: configPath.path) {
                    models.append(url.lastPathComponent)
                }
            }
        }

        return models
    }

    /// Get the size of a cached model in bytes
    @objc public static func modelSize(modelPath: String) -> Int64 {
        let url = URL(fileURLWithPath: modelPath)
        var totalSize: Int64 = 0

        guard let enumerator = FileManager.default.enumerator(
            at: url,
            includingPropertiesForKeys: [.fileSizeKey]
        ) else {
            return 0
        }

        for case let fileURL as URL in enumerator {
            guard let resourceValues = try? fileURL.resourceValues(forKeys: [.fileSizeKey]),
                  let fileSize = resourceValues.fileSize else {
                continue
            }
            totalSize += Int64(fileSize)
        }

        return totalSize
    }

    /// Delete a cached model
    @objc public static func deleteModel(modelPath: String) -> Bool {
        do {
            try FileManager.default.removeItem(atPath: modelPath)
            return true
        } catch {
            print("[MLX] Failed to delete model: \(error.localizedDescription)")
            return false
        }
    }

    // MARK: - Memory Management

    /// Get current GPU memory usage in bytes
    @objc public static func gpuMemoryUsed() -> Int64 {
        return Int64(MLX.GPU.activeMemory)
    }

    /// Get peak GPU memory usage in bytes
    @objc public static func gpuMemoryPeak() -> Int64 {
        return Int64(MLX.GPU.peakMemory)
    }

    /// Clear GPU cache to free memory
    @objc public static func clearGPUCache() {
        MLX.GPU.clearCache()
    }
}

// MARK: - C-compatible wrapper functions for direct linking

/// Check if MLX is available
@_cdecl("mlx_bridge_is_available")
public func mlx_bridge_is_available() -> Bool {
    return MLXBridge.isAvailable()
}

/// Load a model
@_cdecl("mlx_bridge_load_model")
public func mlx_bridge_load_model(
    _ modelId: UnsafePointer<CChar>,
    _ cachePath: UnsafePointer<CChar>
) -> Bool {
    let model = String(cString: modelId)
    let cache = String(cString: cachePath)
    return MLXBridge.loadModel(model, cachePath: cache)
}

/// Unload the current model
@_cdecl("mlx_bridge_unload_model")
public func mlx_bridge_unload_model() {
    MLXBridge.unloadModel()
}

/// Generate text (non-streaming) - returns allocated C string that caller must free
@_cdecl("mlx_bridge_generate")
public func mlx_bridge_generate(
    _ prompt: UnsafePointer<CChar>,
    _ systemPrompt: UnsafePointer<CChar>?,
    _ maxTokens: Int32,
    _ temperature: Float,
    _ outTokenCount: UnsafeMutablePointer<Int32>,
    _ outTokensPerSec: UnsafeMutablePointer<Float>,
    _ outError: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>
) -> UnsafeMutablePointer<CChar>? {
    let promptStr = String(cString: prompt)
    let systemStr = systemPrompt.map { String(cString: $0) }

    let result = MLXBridge.generate(
        prompt: promptStr,
        systemPrompt: systemStr,
        maxTokens: Int(maxTokens),
        temperature: temperature
    )

    outTokenCount.pointee = Int32(result.tokenCount)
    outTokensPerSec.pointee = Float(result.tokensPerSecond)

    if result.success {
        outError.pointee = nil
        return strdup(result.text)
    } else {
        outError.pointee = strdup(result.errorMessage)
        return nil
    }
}

/// Clear GPU cache
@_cdecl("mlx_bridge_clear_cache")
public func mlx_bridge_clear_cache() {
    MLXBridge.clearGPUCache()
}

/// Get GPU memory used
@_cdecl("mlx_bridge_gpu_memory_used")
public func mlx_bridge_gpu_memory_used() -> Int64 {
    return MLXBridge.gpuMemoryUsed()
}

/// Download a model (triggers HuggingFace download via loadModel)
/// Returns error message or NULL on success
@_cdecl("mlx_bridge_download_model")
public func mlx_bridge_download_model(
    _ modelId: UnsafePointer<CChar>,
    _ progressCallback: (@convention(c) (Int32) -> Void)?
) -> UnsafeMutablePointer<CChar>? {
    let model = String(cString: modelId)

    let semaphore = DispatchSemaphore(value: 0)
    nonisolated(unsafe) var errorMessage: String? = nil

    // CRITICAL FIX: Two detached tasks pattern for deadlock-free sync/async bridging
    let coreTask = Task<Void, Never>.detached(priority: .userInitiated) {
        do {
            let configuration = ModelConfiguration(id: model)

            // Load triggers download
            let _ = try await LLMModelFactory.shared.loadContainer(
                configuration: configuration
            ) { progress in
                let percent = Int32(progress.fractionCompleted * 100)
                progressCallback?(percent)
            }

            print("[MLX] Model downloaded: \(model)")
        } catch {
            errorMessage = error.localizedDescription
            print("[MLX] Download failed: \(errorMessage ?? "unknown")")
        }
    }

    // Monitor task signals when complete
    Task<Void, Never>.detached(priority: .userInitiated) {
        await coreTask.value
        semaphore.signal()
    }

    semaphore.wait()

    if let error = errorMessage {
        return strdup(error)
    }
    return nil
}

/// Check if a model exists in the MLX cache
@_cdecl("mlx_bridge_model_exists")
public func mlx_bridge_model_exists(_ modelId: UnsafePointer<CChar>) -> Bool {
    let model = String(cString: modelId)

    // MLX-Swift uses ~/Library/Caches/models/ for caching
    let homeDir = FileManager.default.homeDirectoryForCurrentUser
    let mlxCacheDir = homeDir.appendingPathComponent("Library/Caches/models")
    let mlxModelPath = mlxCacheDir.appendingPathComponent(model)

    // Check if model.safetensors exists (indicates completed download)
    let modelFile = mlxModelPath.appendingPathComponent("model.safetensors")
    if FileManager.default.fileExists(atPath: modelFile.path) {
        return true
    }

    // Also check alternative locations
    let configFile = mlxModelPath.appendingPathComponent("config.json")
    if FileManager.default.fileExists(atPath: configFile.path) {
        return true
    }

    // Fallback: check HuggingFace cache (for models downloaded via huggingface_hub)
    let hfCacheDir = homeDir.appendingPathComponent(".cache/huggingface/hub")
    let modelDirName = "models--\(model.replacingOccurrences(of: "/", with: "--"))"
    let hfModelPath = hfCacheDir.appendingPathComponent(modelDirName)
    let snapshotsPath = hfModelPath.appendingPathComponent("snapshots")

    return FileManager.default.fileExists(atPath: snapshotsPath.path)
}

/// Get the size of a downloaded model in bytes
@_cdecl("mlx_bridge_model_size")
public func mlx_bridge_model_size(_ modelId: UnsafePointer<CChar>) -> Int64 {
    let model = String(cString: modelId)

    let homeDir = FileManager.default.homeDirectoryForCurrentUser
    let cacheDir = homeDir.appendingPathComponent(".cache/huggingface/hub")
    let modelDirName = "models--\(model.replacingOccurrences(of: "/", with: "--"))"
    let modelPath = cacheDir.appendingPathComponent(modelDirName)

    return MLXBridge.modelSize(modelPath: modelPath.path)
}

/// Delete a downloaded model
@_cdecl("mlx_bridge_delete_model")
public func mlx_bridge_delete_model(_ modelId: UnsafePointer<CChar>) -> Bool {
    let model = String(cString: modelId)

    let homeDir = FileManager.default.homeDirectoryForCurrentUser
    let cacheDir = homeDir.appendingPathComponent(".cache/huggingface/hub")
    let modelDirName = "models--\(model.replacingOccurrences(of: "/", with: "--"))"
    let modelPath = cacheDir.appendingPathComponent(modelDirName)

    return MLXBridge.deleteModel(modelPath: modelPath.path)
}

/// List downloaded models - returns newline-separated list, caller must free
@_cdecl("mlx_bridge_list_models")
public func mlx_bridge_list_models() -> UnsafeMutablePointer<CChar>? {
    let homeDir = FileManager.default.homeDirectoryForCurrentUser
    let cacheDir = homeDir.appendingPathComponent(".cache/huggingface/hub")

    var models: [String] = []

    guard let contents = try? FileManager.default.contentsOfDirectory(
        at: cacheDir,
        includingPropertiesForKeys: [.isDirectoryKey]
    ) else {
        return strdup("")
    }

    for url in contents {
        let name = url.lastPathComponent
        // MLX models are in directories starting with "models--mlx-community--"
        if name.hasPrefix("models--mlx-community--") {
            // Check if it has snapshots (complete download)
            let snapshotsPath = url.appendingPathComponent("snapshots")
            if FileManager.default.fileExists(atPath: snapshotsPath.path) {
                // Convert back to HuggingFace ID format
                let modelId = name
                    .replacingOccurrences(of: "models--", with: "")
                    .replacingOccurrences(of: "--", with: "/")
                models.append(modelId)
            }
        }
    }

    return strdup(models.joined(separator: "\n"))
}
