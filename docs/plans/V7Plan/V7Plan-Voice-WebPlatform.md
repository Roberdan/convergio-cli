# Convergio V7: Voice I/O Priority & Web Platform Stack

## Executive Summary

**Priority 1:** Voice conversation must be flawless - this is the killer feature for Education.

**Priority 2:** Web platform stack - React is not preferred, need alternatives.

---

## Part 1: Voice I/O Deep Dive - Current State & Improvements

### Current Implementation Analysis

**What exists:**
- ✅ Swift VoiceManager with AVAudioEngine
- ✅ OpenAI Realtime Audio WebSocket integration
- ✅ Audio format conversion (48kHz → 24kHz)
- ✅ Waveform visualization
- ✅ Azure OpenAI support (GDPR compliant)

**Issues identified:**
1. **Microphone capture returns zeros** (TCC permission cache issue)
2. **No voice activity detection (VAD)** - always listening
3. **No echo cancellation** - feedback issues
4. **No noise suppression** - background noise interference
5. **Limited error recovery** - WebSocket reconnection only
6. **No offline fallback** - requires constant connection
7. **No voice quality metrics** - can't detect poor audio

### Critical Improvements Needed

#### 1. Voice Activity Detection (VAD)

**Problem:** Currently always listening, wastes bandwidth and API calls.

**Solution:** Implement VAD to only send audio when user is speaking.

```swift
// VoiceActivityDetector.swift
class VoiceActivityDetector {
    private let threshold: Float = 0.02  // RMS threshold
    private let minDuration: TimeInterval = 0.3  // Min speech duration
    private var isSpeaking = false
    private var speechStartTime: Date?
    
    func processAudioBuffer(_ buffer: AVAudioPCMBuffer) -> Bool {
        guard let floatData = buffer.floatChannelData else { return false }
        
        // Calculate RMS
        var sumSquares: Float = 0
        let frameCount = Int(buffer.frameLength)
        
        for i in 0..<frameCount {
            let sample = floatData[0][i]
            sumSquares += sample * sample
        }
        
        let rms = sqrt(sumSquares / Float(frameCount))
        let isAboveThreshold = rms > threshold
        
        if isAboveThreshold && !isSpeaking {
            // Speech started
            isSpeaking = true
            speechStartTime = Date()
            return false  // Don't send yet, wait for min duration
        } else if isAboveThreshold && isSpeaking {
            // Continue speaking
            if let startTime = speechStartTime,
               Date().timeIntervalSince(startTime) >= minDuration {
                return true  // Send audio
            }
            return false
        } else {
            // Speech ended
            isSpeaking = false
            speechStartTime = nil
            return false
        }
    }
}
```

**Integration:**
```swift
// In VoiceManager.installInputTap()
inputNode.installTap(onBus: 0, bufferSize: 4800, format: inputFormat) { [weak self] buffer, _ in
    guard let self = self, !self.isMuted, self.isConnected else { return }
    
    // VAD check
    if !self.vad.processAudioBuffer(buffer) {
        return  // Not speaking, skip
    }
    
    // Convert and send
    if let pcmData = self.convertToPCM16(buffer: buffer) {
        Task { @MainActor in
            await self.sendAudioData(pcmData)
        }
    }
}
```

#### 2. Echo Cancellation

**Problem:** Speaker output feeds back into microphone, causing echo/feedback.

**Solution:** Use AVAudioEngine's built-in echo cancellation.

```swift
// In setupAudioSession()
private func setupAudioSession() {
    #if os(macOS)
    // Enable echo cancellation via AVAudioEngine
    audioEngine.inputNode.removeTap(onBus: 0)  // Remove existing tap
    
    // Install tap with echo cancellation
    let inputFormat = audioEngine.inputNode.outputFormat(forBus: 0)
    audioEngine.inputNode.installTap(
        onBus: 0,
        bufferSize: 4800,
        format: inputFormat
    ) { [weak self] buffer, time in
        // Process with echo cancellation
        // AVAudioEngine handles this automatically if configured correctly
        self?.processInputBuffer(buffer, at: time)
    }
    #endif
}
```

**Alternative:** Use CoreAudio's AEC (Acoustic Echo Cancellation) directly.

```swift
// Enable AEC via AudioUnit
private func enableEchoCancellation() {
    var audioUnit: AudioUnit?
    var status: OSStatus
    
    // Get input audio unit
    status = AudioUnitInitialize(audioUnit!)
    guard status == noErr else { return }
    
    // Enable AEC
    var enableAEC: UInt32 = 1
    status = AudioUnitSetProperty(
        audioUnit!,
        kAUVoiceIOProperty_BypassVoiceProcessing,
        kAudioUnitScope_Global,
        0,
        &enableAEC,
        UInt32(MemoryLayout<UInt32>.size)
    )
}
```

#### 3. Noise Suppression

**Problem:** Background noise interferes with speech recognition.

**Solution:** Use AVAudioEngine's noise suppression or ML-based approach.

```swift
// NoiseSuppressor.swift
class NoiseSuppressor {
    private let noiseGate: Float = 0.01  // Below this = noise
    private var noiseProfile: [Float] = []  // Learned noise profile
    
    func processBuffer(_ buffer: AVAudioPCMBuffer) -> AVAudioPCMBuffer? {
        guard let inputData = buffer.floatChannelData else { return nil }
        guard let outputBuffer = AVAudioPCMBuffer(
            pcmFormat: buffer.format,
            frameCapacity: buffer.frameLength
        ) else { return nil }
        
        outputBuffer.frameLength = buffer.frameLength
        guard let outputData = outputBuffer.floatChannelData else { return nil }
        
        let frameCount = Int(buffer.frameLength)
        
        // Simple spectral subtraction
        for i in 0..<frameCount {
            let sample = inputData[0][i]
            let magnitude = abs(sample)
            
            if magnitude < noiseGate {
                // Likely noise, suppress
                outputData[0][i] = sample * 0.1  // Reduce by 90%
            } else {
                // Likely speech, pass through
                outputData[0][i] = sample
            }
        }
        
        return outputBuffer
    }
}
```

#### 4. Audio Quality Metrics

**Problem:** Can't detect when audio quality is poor (low volume, distortion).

**Solution:** Monitor audio quality and warn user.

```swift
// AudioQualityMonitor.swift
class AudioQualityMonitor {
    struct QualityMetrics {
        var rmsLevel: Float = 0
        var peakLevel: Float = 0
        var clippingCount: Int = 0
        var signalToNoiseRatio: Float = 0
        var qualityScore: Float = 0  // 0-1, 1 = perfect
    }
    
    func analyzeBuffer(_ buffer: AVAudioPCMBuffer) -> QualityMetrics {
        guard let floatData = buffer.floatChannelData else {
            return QualityMetrics()
        }
        
        var metrics = QualityMetrics()
        let frameCount = Int(buffer.frameLength)
        var sumSquares: Float = 0
        var peak: Float = 0
        
        for i in 0..<frameCount {
            let sample = floatData[0][i]
            let absSample = abs(sample)
            
            sumSquares += sample * sample
            peak = max(peak, absSample)
            
            if absSample > 0.95 {
                metrics.clippingCount += 1
            }
        }
        
        metrics.rmsLevel = sqrt(sumSquares / Float(frameCount))
        metrics.peakLevel = peak
        
        // Calculate quality score
        if metrics.clippingCount > frameCount / 10 {
            metrics.qualityScore = 0.3  // Too much clipping
        } else if metrics.rmsLevel < 0.01 {
            metrics.qualityScore = 0.2  // Too quiet
        } else if metrics.rmsLevel > 0.5 {
            metrics.qualityScore = 0.7  // Too loud
        } else {
            metrics.qualityScore = 1.0  // Good
        }
        
        return metrics
    }
}
```

**UI Feedback:**
```swift
// Show quality indicator
if metrics.qualityScore < 0.5 {
    showWarning("Audio quality is poor. Please check your microphone.")
}
```

#### 5. Offline Voice Processing

**Problem:** Requires constant internet connection.

**Solution:** Local speech-to-text fallback using Apple's Speech framework.

```swift
// OfflineVoiceProcessor.swift
import Speech

class OfflineVoiceProcessor {
    private let speechRecognizer = SFSpeechRecognizer(locale: Locale(identifier: "it-IT"))
    private let audioEngine = AVAudioEngine()
    
    func startOfflineRecognition() throws {
        let request = SFSpeechAudioBufferRecognitionRequest()
        
        let inputNode = audioEngine.inputNode
        let recordingFormat = inputNode.outputFormat(forBus: 0)
        
        inputNode.installTap(onBus: 0, bufferSize: 1024, format: recordingFormat) { buffer, _ in
            request.append(buffer)
        }
        
        audioEngine.prepare()
        try audioEngine.start()
        
        speechRecognizer?.recognitionTask(with: request) { result, error in
            if let result = result {
                let transcript = result.bestTranscription.formattedString
                // Send transcript to LLM via text API (fallback)
                self.sendTextToLLM(transcript)
            }
        }
    }
}
```

#### 6. Improved Error Handling

**Problem:** Limited error recovery, poor user feedback.

**Solution:** Comprehensive error handling with automatic recovery.

```swift
// VoiceErrorHandler.swift
class VoiceErrorHandler {
    enum ErrorType {
        case networkError
        case audioError
        case permissionError
        case apiError
        case timeout
    }
    
    func handleError(_ error: Error, type: ErrorType) {
        switch type {
        case .networkError:
            // Retry with exponential backoff
            retryConnection(maxAttempts: 5)
            
        case .audioError:
            // Restart audio engine
            restartAudioEngine()
            
        case .permissionError:
            // Show permission request
            requestMicrophonePermission()
            
        case .apiError:
            // Fallback to text mode
            fallbackToTextMode()
            
        case .timeout:
            // Increase timeout, retry
            increaseTimeout()
            retryConnection()
        }
    }
}
```

### Voice I/O Architecture (Improved)

```
┌─────────────────────────────────────────────────────────┐
│  Microphone Input                                        │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
         ┌───────────────────────┐
         │  AVAudioEngine        │
         │  - Echo Cancellation  │
         │  - Noise Suppression  │
         └───────────┬───────────┘
                     │
         ┌───────────┴───────────┐
         │                       │
         ▼                       ▼
┌──────────────┐      ┌──────────────────┐
│ VAD          │      │ Quality Monitor  │
│ (Voice       │      │ (RMS, Clipping)  │
│ Activity)    │      └──────────────────┘
└──────┬───────┘
       │ (if speaking)
       ▼
┌──────────────┐
│ Format       │
│ Conversion   │
│ 48kHz→24kHz  │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ WebSocket    │
│ (OpenAI      │
│ Realtime)    │
└──────┬───────┘
       │
       ▼
┌──────────────┐      ┌──────────────────┐
│ Response     │      │ Offline Fallback │
│ Audio        │◄─────┤ (Speech          │
│              │      │  Framework)      │
└──────┬───────┘      └──────────────────┘
       │
       ▼
┌──────────────┐
│ AVAudio      │
│ PlayerNode   │
│ (Playback)   │
└──────────────┘
```

---

## Part 2: Web Platform Stack - Alternatives to React/Vercel

### Why Not Vercel?

**Vercel is React/Next.js focused:**
- Optimized for Next.js
- React Server Components
- React ecosystem
- If you don't like React, Vercel doesn't add much value

### Alternative Stacks

#### Option 1: SvelteKit (Recommended)

**Why Svelte:**
- ✅ No virtual DOM (faster, smaller)
- ✅ Compile-time optimizations
- ✅ Less boilerplate
- ✅ Better DX (developer experience)
- ✅ Smaller bundle sizes
- ✅ Works on Vercel (or any Node.js host)

**Stack:**
```
Frontend: SvelteKit
Backend: SvelteKit API routes (or separate Rust/Go API)
Database: PostgreSQL + Redis
Hosting: Vercel, Cloudflare Pages, or self-hosted
```

**Example:**
```svelte
<!-- VoiceChat.svelte -->
<script>
  import { onMount } from 'svelte';
  import { voiceStore } from '$lib/stores/voice';
  
  let isRecording = false;
  let transcript = '';
  
  async function startRecording() {
    const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
    const mediaRecorder = new MediaRecorder(stream);
    
    mediaRecorder.ondataavailable = async (event) => {
      if (event.data.size > 0) {
        await sendAudioChunk(event.data);
      }
    };
    
    mediaRecorder.start(100); // 100ms chunks
    isRecording = true;
  }
  
  async function sendAudioChunk(chunk: Blob) {
    const response = await fetch('/api/voice/stream', {
      method: 'POST',
      body: chunk
    });
    
    const data = await response.json();
    transcript = data.transcript;
  }
</script>

<button on:click={startRecording}>
  {isRecording ? 'Stop' : 'Start'} Recording
</button>

<p>{transcript}</p>
```

**Deployment:**
- Vercel: `vercel deploy` (works great)
- Cloudflare Pages: Free, fast CDN
- Self-hosted: Docker container

#### Option 2: SolidStart

**Why Solid:**
- ✅ Fine-grained reactivity (like Svelte)
- ✅ No virtual DOM
- ✅ TypeScript-first
- ✅ Similar API to React (easier migration if needed)
- ✅ Excellent performance

**Stack:**
```
Frontend: SolidStart
Backend: SolidStart API routes
Database: PostgreSQL + Redis
Hosting: Any Node.js host
```

#### Option 3: HTMX + Rust/Go Backend

**Why HTMX:**
- ✅ Minimal JavaScript
- ✅ Server-side rendering
- ✅ Simple, declarative
- ✅ Fast page loads
- ✅ Works with any backend

**Stack:**
```
Frontend: HTMX + Tailwind CSS
Backend: Rust (Axum) or Go (Gin)
Database: PostgreSQL + Redis
Hosting: Fly.io, Railway, or self-hosted
```

**Example:**
```html
<!-- Voice interface with HTMX -->
<div hx-ws="connect:/ws/voice">
  <button hx-ws="send:start_recording">
    Start Recording
  </button>
  
  <div id="transcript" hx-ws="recv:transcript">
    <!-- Transcript appears here -->
  </div>
</div>
```

**Backend (Rust):**
```rust
// axum server
use axum::{Router, routing::get, extract::ws::WebSocket};

async fn voice_websocket(ws: WebSocket) {
    // Handle WebSocket connection
    // Process audio, call LLM, stream response
}
```

#### Option 4: Qwik

**Why Qwik:**
- ✅ Resumability (instant page loads)
- ✅ Minimal JavaScript
- ✅ Great for voice (low latency)
- ✅ Works on Vercel

**Stack:**
```
Frontend: Qwik
Backend: Qwik API routes
Database: PostgreSQL + Redis
Hosting: Vercel, Cloudflare, etc.
```

#### Option 5: Vanilla Web Components

**Why Web Components:**
- ✅ No framework needed
- ✅ Native browser support
- ✅ Small bundle size
- ✅ Framework-agnostic

**Stack:**
```
Frontend: Web Components + Lit (optional)
Backend: Rust/Go API
Database: PostgreSQL + Redis
Hosting: Any
```

**Example:**
```javascript
// voice-chat.js
class VoiceChat extends HTMLElement {
  connectedCallback() {
    this.innerHTML = `
      <button id="record">Start</button>
      <div id="transcript"></div>
    `;
    
    this.querySelector('#record').addEventListener('click', () => {
      this.startRecording();
    });
  }
  
  async startRecording() {
    const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
    // Process audio...
  }
}

customElements.define('voice-chat', VoiceChat);
```

### Recommendation: SvelteKit

**Why:**
1. ✅ Best DX (developer experience)
2. ✅ Small bundle sizes (important for voice)
3. ✅ Fast (no virtual DOM)
4. ✅ Works on Vercel (if you want) or anywhere
5. ✅ Easy to learn
6. ✅ Great for real-time (voice needs low latency)

**Voice Integration:**
```typescript
// lib/stores/voice.ts
import { writable } from 'svelte/store';
import { browser } from '$app/environment';

export const voiceStore = writable({
  isRecording: false,
  transcript: '',
  audioLevel: 0,
  error: null
});

export async function startVoiceSession() {
  if (!browser) return;
  
  const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
  const mediaRecorder = new MediaRecorder(stream);
  
  // WebSocket connection
  const ws = new WebSocket('wss://api.convergio.io/voice');
  
  mediaRecorder.ondataavailable = (event) => {
    if (event.data.size > 0) {
      ws.send(event.data);
    }
  };
  
  ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    voiceStore.update(state => ({
      ...state,
      transcript: data.transcript,
      audioLevel: data.audioLevel
    }));
  };
  
  mediaRecorder.start(100);
  voiceStore.update(state => ({ ...state, isRecording: true }));
}
```

---

## Part 3: Voice on Web Platform

### Web Audio API Integration

**Challenge:** Web Audio API is different from AVAudioEngine.

**Solution:** Use Web Audio API for browser-based voice.

```typescript
// lib/voice/web-audio.ts
export class WebVoiceManager {
  private audioContext: AudioContext;
  private mediaStream: MediaStream;
  private processor: ScriptProcessorNode;
  private websocket: WebSocket;
  
  async initialize() {
    // Get microphone
    this.mediaStream = await navigator.mediaDevices.getUserMedia({
      audio: {
        sampleRate: 24000,
        channelCount: 1,
        echoCancellation: true,
        noiseSuppression: true,
        autoGainControl: true
      }
    });
    
    // Create audio context
    this.audioContext = new AudioContext({ sampleRate: 24000 });
    
    // Create source from stream
    const source = this.audioContext.createMediaStreamSource(this.mediaStream);
    
    // Create processor for audio chunks
    this.processor = this.audioContext.createScriptProcessor(4096, 1, 1);
    
    this.processor.onaudioprocess = (event) => {
      const inputBuffer = event.inputBuffer;
      const pcmData = this.convertToPCM16(inputBuffer);
      
      if (this.websocket.readyState === WebSocket.OPEN) {
        this.websocket.send(pcmData);
      }
    };
    
    source.connect(this.processor);
    this.processor.connect(this.audioContext.destination);
  }
  
  private convertToPCM16(buffer: AudioBuffer): ArrayBuffer {
    const length = buffer.length;
    const pcm16 = new Int16Array(length);
    const channelData = buffer.getChannelData(0);
    
    for (let i = 0; i < length; i++) {
      const sample = Math.max(-1, Math.min(1, channelData[i]));
      pcm16[i] = sample * 32767;
    }
    
    return pcm16.buffer;
  }
  
  connectWebSocket(url: string) {
    this.websocket = new WebSocket(url);
    
    this.websocket.onmessage = (event) => {
      if (event.data instanceof Blob) {
        this.playAudioResponse(event.data);
      } else {
        const data = JSON.parse(event.data);
        // Handle transcript, emotion, etc.
      }
    };
  }
  
  private async playAudioResponse(audioBlob: Blob) {
    const arrayBuffer = await audioBlob.arrayBuffer();
    const audioBuffer = await this.audioContext.decodeAudioData(arrayBuffer);
    
    const source = this.audioContext.createBufferSource();
    source.buffer = audioBuffer;
    source.connect(this.audioContext.destination);
    source.start();
  }
}
```

### WebSocket Backend (Rust)

```rust
// api_gateway/src/voice.rs
use axum::extract::ws::{WebSocket, Message};
use tokio_tungstenite::tungstenite::Message as WsMessage;

pub async fn voice_websocket_handler(ws: WebSocket) {
    let (mut sender, mut receiver) = ws.split();
    
    // Connect to OpenAI Realtime API
    let mut openai_ws = connect_to_openai_realtime().await;
    
    // Forward audio from client to OpenAI
    tokio::spawn(async move {
        while let Some(Ok(msg)) = receiver.next().await {
            if let Message::Binary(audio_data) = msg {
                // Send to OpenAI Realtime
                openai_ws.send(WsMessage::Binary(audio_data)).await?;
            }
        }
    });
    
    // Forward responses from OpenAI to client
    tokio::spawn(async move {
        while let Some(Ok(msg)) = openai_ws.next().await {
            if let WsMessage::Binary(audio_data) = msg {
                sender.send(Message::Binary(audio_data)).await?;
            }
        }
    });
}
```

---

## Implementation Priority

### Phase 1: Voice Improvements (Mac Native) - Weeks 1-2
1. ✅ Fix TCC permission issue (reboot)
2. ✅ Implement VAD (Voice Activity Detection)
3. ✅ Add echo cancellation
4. ✅ Add noise suppression
5. ✅ Add audio quality monitoring
6. ✅ Improve error handling

### Phase 2: Web Platform (SvelteKit) - Weeks 3-6
1. ✅ Setup SvelteKit project
2. ✅ Implement Web Audio API voice manager
3. ✅ Create voice chat UI
4. ✅ Connect to backend WebSocket
5. ✅ Test end-to-end

### Phase 3: Backend Integration - Weeks 7-8
1. ✅ Rust API gateway with WebSocket support
2. ✅ Connect to OpenAI Realtime API
3. ✅ Audio streaming pipeline
4. ✅ Error handling & reconnection

### Phase 4: Polish - Weeks 9-10
1. ✅ UI/UX improvements
2. ✅ Performance optimization
3. ✅ Testing & bug fixes
4. ✅ Documentation

---

## Conclusion

**Voice I/O:** Must be perfect. Implement VAD, echo cancellation, noise suppression.

**Web Platform:** SvelteKit recommended. No React needed, works great, fast, small bundles.

**Next Steps:**
1. Fix Mac voice issues (VAD, echo cancellation)
2. Build SvelteKit web platform
3. Integrate Web Audio API
4. Connect to Rust backend
5. Test end-to-end voice flow

---

## Related Documents

**Master Index:** [V7Plan-MASTER-INDEX.md](./V7Plan-MASTER-INDEX.md) - Complete documentation hub

**Single Source of Truth:**
- [V7Plan-CRITICAL-REVIEW.md](./V7Plan-CRITICAL-REVIEW.md) - Optimized unified plan ⭐

**Architecture:**
- [V7Plan.md](./V7Plan.md) - Core architecture
- [V7Plan-Enhanced.md](./V7Plan-Enhanced.md) - Web platform architecture
- [V7Plan-Architecture-DeepDive.md](./V7Plan-Architecture-DeepDive.md) - Backend integration

