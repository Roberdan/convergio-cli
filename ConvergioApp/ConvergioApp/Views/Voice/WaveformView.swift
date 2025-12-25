/**
 * CONVERGIO NATIVE - WaveformView
 *
 * Real-time audio waveform visualization using Metal for high performance.
 * Displays voice input/output levels with smooth animations.
 *
 * Part of the Scuola 2026 Voice System (Task 0.3.5)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import MetalKit
import Combine

// MARK: - Waveform Data Model

@MainActor
final class WaveformData: ObservableObject {
    @Published private(set) var samples: [Float] = []
    @Published private(set) var peakLevel: Float = 0
    @Published private(set) var rmsLevel: Float = 0

    private let maxSamples: Int
    private var smoothingFactor: Float = 0.3

    init(maxSamples: Int = 128) {
        self.maxSamples = maxSamples
        self.samples = Array(repeating: 0, count: maxSamples)
    }

    func addSamples(_ newSamples: [Float]) {
        // Calculate RMS and peak
        var peak: Float = 0
        var sumSquares: Float = 0

        for sample in newSamples {
            let abs = Swift.abs(sample)
            if abs > peak { peak = abs }
            sumSquares += sample * sample
        }

        let rms = sqrt(sumSquares / Float(max(newSamples.count, 1)))

        // Smooth values
        peakLevel = peakLevel * (1 - smoothingFactor) + peak * smoothingFactor
        rmsLevel = rmsLevel * (1 - smoothingFactor) + rms * smoothingFactor

        // Update sample buffer (downsampled)
        let step = max(1, newSamples.count / 8)
        var newBuffer: [Float] = []

        for i in stride(from: 0, to: newSamples.count, by: step) {
            let end = min(i + step, newSamples.count)
            let slice = newSamples[i..<end]
            let avg = slice.reduce(0, +) / Float(slice.count)
            newBuffer.append(avg)
        }

        samples = Array(samples.dropFirst(newBuffer.count)) + newBuffer
        if samples.count > maxSamples {
            samples = Array(samples.suffix(maxSamples))
        }
        while samples.count < maxSamples {
            samples.insert(0, at: 0)
        }
    }

    func reset() {
        samples = Array(repeating: 0, count: maxSamples)
        peakLevel = 0
        rmsLevel = 0
    }
}

// MARK: - Metal Waveform Renderer

final class WaveformRenderer: NSObject, MTKViewDelegate {
    private let device: MTLDevice
    private let commandQueue: MTLCommandQueue
    private var pipelineState: MTLRenderPipelineState?
    private var vertexBuffer: MTLBuffer?

    private var samples: [Float] = []
    private var accentColor: SIMD4<Float> = SIMD4<Float>(0.3, 0.7, 1.0, 1.0)
    private var isListening: Bool = false

    struct Vertex {
        var position: SIMD2<Float>
        var color: SIMD4<Float>
    }

    init?(device: MTLDevice) {
        self.device = device
        guard let queue = device.makeCommandQueue() else { return nil }
        self.commandQueue = queue
        super.init()

        setupPipeline()
    }

    private func setupPipeline() {
        let library = device.makeDefaultLibrary()

        // If no shader library, use simple line rendering
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
        pipelineDescriptor.colorAttachments[0].isBlendingEnabled = true
        pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha

        // Use function constants for simple vertex/fragment if available
        if let vertexFunction = library?.makeFunction(name: "waveformVertex"),
           let fragmentFunction = library?.makeFunction(name: "waveformFragment") {
            pipelineDescriptor.vertexFunction = vertexFunction
            pipelineDescriptor.fragmentFunction = fragmentFunction
            pipelineState = try? device.makeRenderPipelineState(descriptor: pipelineDescriptor)
        }
    }

    func updateSamples(_ newSamples: [Float], color: NSColor, listening: Bool) {
        samples = newSamples
        accentColor = SIMD4<Float>(
            Float(color.redComponent),
            Float(color.greenComponent),
            Float(color.blueComponent),
            Float(color.alphaComponent)
        )
        isListening = listening
    }

    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        // Handle resize if needed
    }

    func draw(in view: MTKView) {
        guard let drawable = view.currentDrawable,
              let renderPassDescriptor = view.currentRenderPassDescriptor,
              let commandBuffer = commandQueue.makeCommandBuffer() else { return }

        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 0)
        renderPassDescriptor.colorAttachments[0].loadAction = .clear

        guard let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else { return }

        // Draw waveform using primitive lines if no custom shader
        drawWaveformLines(encoder: encoder, in: view)

        encoder.endEncoding()
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }

    private func drawWaveformLines(encoder: MTLRenderCommandEncoder, in view: MTKView) {
        guard samples.count >= 2 else { return }

        let height = Float(view.drawableSize.height)
        let centerY = height / 2
        let amplitude = height * 0.4

        var vertices: [Vertex] = []

        for i in 0..<samples.count {
            let x = (Float(i) / Float(samples.count - 1)) * 2.0 - 1.0
            let y = samples[i] * amplitude / centerY

            let color = isListening ? accentColor : SIMD4<Float>(0.5, 0.5, 0.5, 0.6)
            vertices.append(Vertex(position: SIMD2<Float>(x, y), color: color))
        }

        // Create vertex buffer
        let dataSize = vertices.count * MemoryLayout<Vertex>.stride
        guard let buffer = device.makeBuffer(bytes: vertices, length: dataSize, options: []) else { return }

        encoder.setVertexBuffer(buffer, offset: 0, index: 0)

        if let pipeline = pipelineState {
            encoder.setRenderPipelineState(pipeline)
        }

        encoder.drawPrimitives(type: .lineStrip, vertexStart: 0, vertexCount: vertices.count)
    }
}

// MARK: - SwiftUI Metal View Wrapper

struct MetalWaveformView: NSViewRepresentable {
    @ObservedObject var data: WaveformData
    var accentColor: Color
    var isListening: Bool

    func makeNSView(context: Context) -> MTKView {
        guard let device = MTLCreateSystemDefaultDevice() else {
            // Fallback: return empty view
            let view = MTKView()
            view.clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 0)
            return view
        }

        let view = MTKView(frame: .zero, device: device)
        view.clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 0)
        view.layer?.isOpaque = false
        view.enableSetNeedsDisplay = false
        view.isPaused = false
        view.preferredFramesPerSecond = 60

        if let renderer = WaveformRenderer(device: device) {
            context.coordinator.renderer = renderer
            view.delegate = renderer
        }

        return view
    }

    func updateNSView(_ nsView: MTKView, context: Context) {
        context.coordinator.renderer?.updateSamples(
            data.samples,
            color: NSColor(accentColor),
            listening: isListening
        )
    }

    func makeCoordinator() -> Coordinator {
        Coordinator()
    }

    class Coordinator {
        var renderer: WaveformRenderer?
    }
}

// MARK: - SwiftUI Fallback Waveform (Canvas-based)

struct CanvasWaveformView: View {
    @ObservedObject var data: WaveformData
    var accentColor: Color
    var isListening: Bool

    var body: some View {
        Canvas { context, size in
            let midY = size.height / 2
            let amplitude = size.height * 0.4
            let stepX = size.width / CGFloat(max(data.samples.count - 1, 1))

            var path = Path()

            for (index, sample) in data.samples.enumerated() {
                let x = CGFloat(index) * stepX
                let y = midY - CGFloat(sample) * amplitude

                if index == 0 {
                    path.move(to: CGPoint(x: x, y: y))
                } else {
                    path.addLine(to: CGPoint(x: x, y: y))
                }
            }

            let color = isListening ? accentColor : Color.gray.opacity(0.6)
            context.stroke(path, with: .color(color), lineWidth: 2)

            // Draw glow effect when listening
            if isListening {
                context.stroke(path, with: .color(accentColor.opacity(0.3)), lineWidth: 6)
            }
        }
    }
}

// MARK: - WaveformView (Public API)

struct WaveformView: View {
    @StateObject private var data: WaveformData
    let isListening: Bool
    let useMetal: Bool
    let height: CGFloat
    var accentColor: Color = .blue

    private var useMetalActual: Bool {
        useMetal && MTLCreateSystemDefaultDevice() != nil
    }

    init(samples: [Float] = [], isListening: Bool = false, useMetal: Bool = true, height: CGFloat = 60) {
        _data = StateObject(wrappedValue: WaveformData())
        self.isListening = isListening
        self.useMetal = useMetal
        self.height = height
    }

    var body: some View {
        ZStack {
            // Background
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.black.opacity(0.1))

            // Waveform
            if useMetalActual {
                MetalWaveformView(data: data, accentColor: accentColor, isListening: isListening)
            } else {
                CanvasWaveformView(data: data, accentColor: accentColor, isListening: isListening)
            }

            // Level indicators
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text("RMS")
                        .font(.caption2)
                        .foregroundColor(.secondary)
                    ProgressView(value: Double(data.rmsLevel), total: 1.0)
                        .progressViewStyle(.linear)
                        .frame(width: 40)
                }
                .opacity(isListening ? 1 : 0.3)

                Spacer()

                VStack(alignment: .trailing, spacing: 4) {
                    Text("Peak")
                        .font(.caption2)
                        .foregroundColor(.secondary)
                    ProgressView(value: Double(data.peakLevel), total: 1.0)
                        .progressViewStyle(.linear)
                        .frame(width: 40)
                }
                .opacity(isListening ? 1 : 0.3)
            }
            .padding(.horizontal, 8)
        }
        .frame(height: height)
        .clipShape(RoundedRectangle(cornerRadius: 8))
    }

    func addSamples(_ samples: [Float]) {
        data.addSamples(samples)
    }
}

// MARK: - Compact Waveform for Voice Session

struct CompactWaveformView: View {
    @ObservedObject var data: WaveformData
    var isListening: Bool
    var maestroColor: Color = .blue

    var body: some View {
        HStack(spacing: 2) {
            ForEach(0..<min(32, data.samples.count), id: \.self) { index in
                let sampleIndex = index * (data.samples.count / 32)
                let sample = sampleIndex < data.samples.count ? abs(data.samples[sampleIndex]) : 0

                RoundedRectangle(cornerRadius: 1)
                    .fill(isListening ? maestroColor : Color.gray.opacity(0.4))
                    .frame(width: 3, height: max(4, CGFloat(sample) * 40))
                    .animation(.easeOut(duration: 0.1), value: sample)
            }
        }
        .frame(height: 44)
        .padding(.horizontal, 8)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.black.opacity(0.05))
        )
    }
}

// MARK: - Previews

#if DEBUG
struct WaveformView_Previews: PreviewProvider {
    static var previews: some View {
        VStack(spacing: 20) {
            Text("Metal Waveform")
                .font(.headline)
            WaveformView(isListening: true, useMetal: true)
                .frame(height: 80)

            Text("Canvas Waveform")
                .font(.headline)
            WaveformView(isListening: true, useMetal: false)
                .frame(height: 80)

            Text("Compact Waveform")
                .font(.headline)
            CompactWaveformView(data: WaveformData(), isListening: true, maestroColor: .purple)
        }
        .padding()
        .frame(width: 400)
    }
}
#endif
