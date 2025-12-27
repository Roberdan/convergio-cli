/**
 * CONVERGIO NATIVE - Microphone Test View
 *
 * Simple test view to verify microphone capture works.
 * Based on Apple Developer Forums recommendations.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import AVFoundation

/// Simple microphone test - minimal code to verify audio capture
struct MicrophoneTestView: View {
    @StateObject private var tester = MicrophoneTester()

    var body: some View {
        VStack(spacing: 20) {
            Text("Microphone Test")
                .font(.largeTitle)
                .fontWeight(.bold)

            // Status
            HStack {
                Circle()
                    .fill(tester.isCapturing ? Color.green : Color.red)
                    .frame(width: 20, height: 20)
                Text(tester.isCapturing ? "Capturing" : "Stopped")
                    .font(.headline)
            }

            // Audio level meter
            VStack(alignment: .leading, spacing: 8) {
                Text("Audio Level: \(String(format: "%.2f", tester.audioLevel))")
                    .font(.system(.body, design: .monospaced))

                GeometryReader { geo in
                    ZStack(alignment: .leading) {
                        RoundedRectangle(cornerRadius: 4)
                            .fill(Color.gray.opacity(0.3))
                        RoundedRectangle(cornerRadius: 4)
                            .fill(tester.audioLevel > 0.01 ? Color.green : Color.orange)
                            .frame(width: geo.size.width * CGFloat(min(1.0, tester.audioLevel * 5)))
                            .animation(.linear(duration: 0.05), value: tester.audioLevel)
                    }
                }
                .frame(height: 30)
            }
            .padding()
            .background(Color.black.opacity(0.1))
            .cornerRadius(8)

            // Buffer count
            Text("Buffers received: \(tester.bufferCount)")
                .font(.system(.body, design: .monospaced))

            // Log
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

            // Controls
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

/// Minimal microphone tester class
@MainActor
class MicrophoneTester: ObservableObject {
    @Published var isCapturing = false
    @Published var audioLevel: Float = 0.0
    @Published var bufferCount: Int = 0
    @Published var log: String = ""

    private var audioEngine: AVAudioEngine?

    func start() {
        addLog("Starting microphone test...")

        // Check permission first
        let status = AVCaptureDevice.authorizationStatus(for: .audio)
        addLog("Permission status: \(status.rawValue) (3=authorized)")

        if status != .authorized {
            addLog("ERROR: Microphone not authorized!")
            if status == .notDetermined {
                AVCaptureDevice.requestAccess(for: .audio) { granted in
                    Task { @MainActor in
                        if granted {
                            self.addLog("Permission granted, try again")
                        } else {
                            self.addLog("Permission denied")
                        }
                    }
                }
            }
            return
        }

        do {
            // Create fresh engine
            let engine = AVAudioEngine()
            self.audioEngine = engine

            // Get input node
            let inputNode = engine.inputNode
            addLog("Got inputNode")

            // Check hardware format
            let hwFormat = inputNode.inputFormat(forBus: 0)
            addLog("HW format: \(Int(hwFormat.sampleRate))Hz, \(hwFormat.channelCount)ch")

            // Check output format
            let outFormat = inputNode.outputFormat(forBus: 0)
            addLog("Output format: \(Int(outFormat.sampleRate))Hz, \(outFormat.channelCount)ch")

            // Install tap with nil format (let system choose)
            addLog("Installing tap with format: nil")

            inputNode.installTap(onBus: 0, bufferSize: 1024, format: nil) { [weak self] buffer, time in
                // Calculate RMS level
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
                        self?.addLog("First buffer! Format: \(Int(buffer.format.sampleRate))Hz")
                    }
                }
            }

            addLog("Tap installed, preparing engine...")
            engine.prepare()

            addLog("Starting engine...")
            try engine.start()

            isCapturing = true
            addLog("✅ Engine started! Speak into microphone...")

        } catch {
            addLog("❌ ERROR: \(error.localizedDescription)")
            addLog("Error code: \((error as NSError).code)")
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

#Preview {
    MicrophoneTestView()
}
