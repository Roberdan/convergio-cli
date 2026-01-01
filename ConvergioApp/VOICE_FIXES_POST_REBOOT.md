# Fix Problemi Voce - Post Reboot

**Stato**: Microfono funziona dopo reboot, ma ci sono altri problemi.

---

## 🔍 PROBLEMI IDENTIFICATI

### 1. ❌ Audio Non Viene Committato al Server

**Problema**: L'audio viene inviato continuamente ma non viene mai "committato" esplicitamente.  
**Impatto**: Il server potrebbe non sapere quando l'utente ha finito di parlare.

**Fix Necessario**: Il server usa `server_vad` che dovrebbe rilevare automaticamente, ma è meglio essere espliciti.

### 2. ⚠️ Normalizzazione Waveform Non Ottimale

**Problema**: La normalizzazione usa `rms * 3.0` che potrebbe non mostrare livelli reali.  
**Impatto**: Waveform non si anima correttamente anche quando c'è audio.

### 3. ⚠️ Playback Buffer Potrebbe Essere Vuoto

**Problema**: Non c'è controllo se il buffer di playback è vuoto o pieno.  
**Impatto**: Audio potrebbe non essere riprodotto o avere interruzioni.

### 4. ⚠️ Errori Durante Invio Audio Non Gestiti

**Problema**: Errori durante `sendPCMData` vengono loggati ma non propagati.  
**Impatto**: L'utente non sa se l'audio viene inviato correttamente.

---

## 🛠️ FIX IMMEDIATI

### Fix 1: Migliorare Normalizzazione Waveform

**File**: `Services/VoiceManager.swift`  
**Linea**: ~522-544

```swift
private func updateInputLevels(floatData: UnsafePointer<UnsafeMutablePointer<Float>>, frameCount: Int) {
    let barCount = 40
    let samplesPerBar = max(1, frameCount / barCount)
    var newLevels: [Float] = []

    for bar in 0..<barCount {
        let startSample = bar * samplesPerBar
        let endSample = min(startSample + samplesPerBar, frameCount)

        var sumSquares: Float = 0
        var peak: Float = 0
        for i in startSample..<endSample {
            let sample = abs(floatData[0][i])
            sumSquares += sample * sample
            if sample > peak { peak = sample }
        }
        
        let rms = sqrt(sumSquares / Float(endSample - startSample))
        
        // MIGLIORATO: Normalizzazione più intelligente
        // Usa sia RMS che peak, con amplificazione dinamica
        let combined = (rms * 0.7 + peak * 0.3)
        let amplified = combined * 5.0  // Amplifica per vedere meglio
        let normalized = min(1.0, amplified)
        
        newLevels.append(normalized)
    }

    // Smoothing migliore (più reattivo)
    for i in 0..<barCount {
        inputAudioLevels[i] = inputAudioLevels[i] * 0.15 + newLevels[i] * 0.85
    }
}
```

### Fix 2: Aggiungere Validazione Audio Buffer

**File**: `Services/VoiceManager.swift`  
**Linea**: ~354-367

```swift
inputNode.installTap(onBus: 0, bufferSize: 4800, format: inputFormat) { [weak self] buffer, _ in
    guard let self = self, !self.isMuted, self.isConnected else { return }

    // VALIDARE che l'audio non sia tutto zeri
    guard self.validateAudioBuffer(buffer) else {
        // Log solo ogni 100 buffer per non spammare
        if self.audioBufferCount % 100 == 0 {
            self.onDebugLog?("⚠️ Audio buffer validation failed (zeros detected)")
        }
        return
    }

    // Convert and send audio
    if let pcmData = self.convertToPCM16(buffer: buffer) {
        // Verificare che pcmData non sia vuoto
        guard !pcmData.isEmpty else {
            self.onDebugLog?("⚠️ Converted PCM data is empty")
            return
        }
        
        Task { @MainActor in
            await self.sendAudioData(pcmData)
        }
    } else {
        self.onDebugLog?("⚠️ Failed to convert audio buffer to PCM16")
    }
}

// Aggiungere metodo di validazione
private func validateAudioBuffer(_ buffer: AVAudioPCMBuffer) -> Bool {
    guard let floatData = buffer.floatChannelData else { return false }
    let frameCount = Int(buffer.frameLength)
    
    guard frameCount > 0 else { return false }
    
    // Verifica che non siano tutti zeri o silenzio
    var hasNonZero = false
    var maxAmplitude: Float = 0
    
    for i in 0..<min(100, frameCount) {
        let sample = abs(floatData[0][i])
        if sample > 0.001 {
            hasNonZero = true
        }
        if sample > maxAmplitude {
            maxAmplitude = sample
        }
    }
    
    // Se non c'è audio significativo, non inviare
    if !hasNonZero || maxAmplitude < 0.01 {
        return false
    }
    
    return true
}
```

### Fix 3: Migliorare Gestione Errori Invio Audio

**File**: `Services/VoiceManager.swift`  
**Linea**: ~446-454

```swift
/// Send PCM16 audio data to WebSocket
private func sendAudioData(_ data: Data) async {
    guard !data.isEmpty else {
        onDebugLog?("⚠️ Attempted to send empty audio data")
        return
    }
    
    audioBufferCount += 1

    if audioBufferCount == 1 || audioBufferCount % 100 == 0 {
        onDebugLog?("📤 Sent \(audioBufferCount) audio buffers (\(data.count) bytes)")
    }

    // Gestire errori esplicitamente
    do {
        await webSocket?.sendPCMData(data)
    } catch {
        logger.error("Failed to send audio data: \(error.localizedDescription)")
        onDebugLog?("❌ Failed to send audio: \(error.localizedDescription)")
        
        // Se ci sono troppi errori consecutivi, potrebbe essere un problema di connessione
        // (questo verrà gestito dalla reconnessione automatica)
    }
}
```

**File**: `Services/OpenAIRealtimeWebSocket.swift`  
**Linea**: ~366-386

```swift
func sendPCMData(_ data: Data) async {
    guard isConnected else {
        logWarning("Cannot send audio: not connected")
        onDebugLog?("⚠️ Cannot send audio: WebSocket not connected")
        return
    }
    
    guard !data.isEmpty else {
        logWarning("Cannot send empty audio data")
        return
    }

    audioChunksSent += 1

    // Encode as base64
    let base64Audio = data.base64EncodedString()
    
    guard !base64Audio.isEmpty else {
        logError("Failed to encode audio to base64")
        onDebugLog?("❌ Failed to encode audio")
        return
    }

    // Send audio append event
    let audioEvent: [String: Any] = [
        "type": "input_audio_buffer.append",
        "audio": base64Audio
    ]

    do {
        try await sendJSON(audioEvent)
        
        // Log solo ogni 50 chunks per non spammare
        if audioChunksSent % 50 == 0 {
            onDebugLog?("📤 Sent \(audioChunksSent) audio chunks")
        }
    } catch {
        logError("OpenAI Realtime: Failed to send audio: \(error.localizedDescription)")
        onDebugLog?("❌ Send failed: \(error.localizedDescription)")
        
        // Rilanciare errore per gestione upstream
        throw error
    }
}
```

### Fix 4: Migliorare Playback Buffer Management

**File**: `Services/VoiceManager.swift`  
**Linea**: ~458-490

```swift
/// Play received audio data
private func playAudioData(_ audioData: Data) {
    guard let player = playerNode, isEngineRunning else {
        onDebugLog?("⚠️ Cannot play: engine not running or player not available")
        return
    }
    
    guard !audioData.isEmpty else {
        onDebugLog?("⚠️ Attempted to play empty audio data")
        return
    }

    // Convert PCM16 data to Float32 buffer
    guard let buffer = createPlaybackBuffer(from: audioData) else {
        onDebugLog?("⚠️ Failed to create playback buffer")
        return
    }
    
    guard buffer.frameLength > 0 else {
        onDebugLog?("⚠️ Playback buffer has zero frames")
        return
    }

    playbackBufferCount += 1

    // Update output audio levels
    if let floatData = buffer.floatChannelData {
        updateOutputLevels(floatData: floatData, frameCount: Int(buffer.frameLength))
    }

    // Schedule buffer for playback con completion handler per debugging
    player.scheduleBuffer(buffer) { [weak self] in
        // Buffer completato - utile per debugging
        if let self = self, self.playbackBufferCount % 20 == 0 {
            self.onDebugLog?("🔊 Completed playback of buffer #\(self.playbackBufferCount)")
        }
    }

    // Start playing if not already
    if !player.isPlaying {
        player.play()
        onDebugLog?("🔊 Started audio playback")
    }

    if playbackBufferCount == 1 || playbackBufferCount % 20 == 0 {
        onDebugLog?("🔊 Scheduled buffer #\(playbackBufferCount) for playback")
    }
}
```

### Fix 5: Aggiungere Logging Dettagliato per Debug

**File**: `Services/VoiceManager.swift`  
**Aggiungere all'inizio di `convertToPCM16`**:

```swift
private func convertToPCM16(buffer: AVAudioPCMBuffer) -> Data? {
    guard let floatData = buffer.floatChannelData else {
        onDebugLog?("⚠️ No float channel data in buffer")
        return nil
    }

    let inputSampleRate = buffer.format.sampleRate
    let outputSampleRate = Self.openAISampleRate
    let ratio = outputSampleRate / inputSampleRate

    let inputFrames = Int(buffer.frameLength)
    let outputFrames = Int(Double(inputFrames) * ratio)
    
    // Log dettagli formato ogni 100 buffer
    if audioBufferCount % 100 == 0 {
        onDebugLog?("🎤 Audio: \(Int(inputSampleRate))Hz → \(Int(outputSampleRate))Hz, \(inputFrames) → \(outputFrames) frames")
    }

    // Calculate audio levels for visualization
    updateInputLevels(floatData: floatData, frameCount: inputFrames)

    var pcmData = Data(capacity: outputFrames * 2)

    // Resample and convert Float32 → Int16
    for i in 0..<outputFrames {
        let srcIndex = Double(i) / ratio
        let srcIndexInt = Int(srcIndex)
        let frac = Float(srcIndex - Double(srcIndexInt))

        let sample1 = floatData[0][min(srcIndexInt, inputFrames - 1)]
        let sample2 = floatData[0][min(srcIndexInt + 1, inputFrames - 1)]

        let interpolated = sample1 + (sample2 - sample1) * frac
        let clamped = max(-1.0, min(1.0, interpolated))
        let int16Value = Int16(clamped * 32767.0)

        withUnsafeBytes(of: int16Value.littleEndian) { pcmData.append(contentsOf: $0) }
    }
    
    // Verificare che i dati convertiti non siano tutti zeri
    if pcmData.allSatisfy({ $0 == 0 }) {
        onDebugLog?("⚠️ Converted PCM data is all zeros!")
        return nil
    }

    return pcmData
}
```

---

## 🧪 TEST DA FARE

Dopo aver applicato i fix, testare:

1. **Microfono cattura audio**
   - [ ] Waveform si anima quando parli
   - [ ] Debug log mostra "📤 Sent X audio buffers"
   - [ ] Non ci sono warning "Audio buffer validation failed"

2. **Server riceve audio**
   - [ ] Debug log mostra "🎙️ Server detected speech START"
   - [ ] Debug log mostra "🎙️ Server detected speech STOP"
   - [ ] Transcript viene ricevuto ("🎤 You: ...")

3. **Risposta audio**
   - [ ] Debug log mostra "🔊 Started audio playback"
   - [ ] Audio viene riprodotto chiaramente
   - [ ] Waveform output si anima durante playback
   - [ ] Debug log mostra "✅ Response complete"

4. **Errori**
   - [ ] Se c'è un errore, viene mostrato chiaramente nel debug log
   - [ ] Reconnessione automatica funziona se connessione si perde

---

## 📋 CHECKLIST IMPLEMENTAZIONE

- [ ] Fix 1: Migliorare normalizzazione waveform
- [ ] Fix 2: Aggiungere validazione audio buffer
- [ ] Fix 3: Migliorare gestione errori invio
- [ ] Fix 4: Migliorare playback buffer management
- [ ] Fix 5: Aggiungere logging dettagliato
- [ ] Test completo end-to-end
- [ ] Verificare che waveform si animi correttamente
- [ ] Verificare che audio venga riprodotto

---

## 🐛 DEBUGGING

Se i problemi persistono, controllare nel debug log:

1. **Se waveform non si anima**:
   - Cercare "Audio buffer validation failed" → problema validazione
   - Cercare "Converted PCM data is all zeros" → problema conversione
   - Verificare che `updateInputLevels` venga chiamato

2. **Se audio non viene inviato**:
   - Cercare "📤 Sent X audio buffers" → se non appare, problema invio
   - Cercare "❌ Send failed" → errore WebSocket
   - Verificare che `isConnected` sia true

3. **Se audio non viene riprodotto**:
   - Cercare "🔊 Started audio playback" → se non appare, problema playback
   - Cercare "⚠️ Cannot play" → problema engine/player
   - Verificare che `response.audio.delta` venga ricevuto

4. **Se non ci sono risposte**:
   - Cercare "🎙️ Server detected speech START/STOP" → server VAD funziona?
   - Cercare "conversation.item.input_audio_transcription.completed" → transcript ricevuto?
   - Verificare che `create_response: true` sia nella config

---

**Prossimi Passi**: Applicare i fix e testare. Se problemi persistono, condividere i log del debug per analisi più approfondita.











