# Voice Implementation Status - 27 December 2025

## Current Status: PARTIALLY WORKING

Voice functionality has been significantly improved but still has issues requiring further investigation.

---

## What Was Done Today

### 1. Feedback Loop Prevention
- Added `isSpeakingAtomic` flag to prevent microphone from capturing AI's own audio output
- When state changes to `.speaking`, the tap callback skips audio capture
- This prevents the "echo" issue where AI would transcribe its own speech

### 2. Audio Capture Fix
- **Critical fix**: Moved `installTap()` to BEFORE `engine.start()`
- This matches the working `MicrophoneTestView` implementation
- Fixed buffer size from 4800 to 1024 for better latency

### 3. Format Mismatch Resolution
- Fixed player node connection to use `playbackFormat` (mono 24kHz) instead of mixer's output format (stereo)
- This resolved the crash: `required condition is false: _outputFormat.channelCount == buffer.format.channelCount`

### 4. Memory Leak Fix Attempt
- Removed aggressive `Task.cancel()` that was preventing audio from being sent
- Changed to `Task.detached` for sending audio without blocking
- Added throttling for UI updates (every 50 buffers for logs, every 5 for visualization)

### 5. Language Configuration
- Updated default instructions to explicitly require Italian responses
- Added fallback instruction: "Scusa, non ho capito bene. Puoi ripetere?"

### 6. MicrophoneTestView
- Created standalone test view to verify microphone works independently
- Accessible via View menu (Cmd+Option+M)
- Confirms 24kHz capture works correctly

---

## What Was Learned

### Audio Architecture on macOS
1. **Tap installation order matters**: Must install tap BEFORE starting engine
2. **Format: nil is safest**: Let AVAudioEngine choose the native format
3. **Thread safety critical**: Audio callback runs on audio thread, must use atomic flags
4. **Task.cancel() is dangerous**: Canceling tasks before they complete = lost data

### CoreAudio Issues on macOS Tahoe (beta)
1. **HAL errors are common**: `AudioObjectSetPropertyData: no object with given ID` appears frequently
2. **Error -10877 (kAudioUnitErr_NoConnection)**: Usually means sample rate mismatch or timing issue
3. **coreaudiod reset helps**: `sudo killall coreaudiod` can fix stuck audio state

### OpenAI Realtime API
1. **Server VAD works**: Server detects speech start/stop automatically
2. **Streaming deltas**: Response comes in small pieces, must accumulate
3. **Language detection**: If audio is unclear, model may respond in wrong language

---

## Known Issues (Still Open)

### P0: Critical
1. **Error -10877 still appearing**: Sample rate mismatch during playback or capture
   - Appears as "throwing -10877" in console
   - May be related to macOS Tahoe beta issues

2. **Memory usage concerns**: Need to monitor if Task.detached causes accumulation
   - SwiftUI warning: "Publishing changes from within view updates is not allowed"

### P1: High Priority
3. **Occasional hangs**: App becomes unresponsive sometimes
   - End Session button may not work
   - May require force quit

4. **Audio occasionally stops**: After some time, audio capture may stop
   - Buffer count stops incrementing
   - Need to restart session

### P2: Medium Priority
5. **Language confusion**: AI may respond in wrong language if audio is unclear
   - Added explicit Italian instructions but not fully tested

6. **HAL errors on disconnect**: `AudioObjectSetPropertyData: no object with given ID` when ending session
   - Cosmetic issue, doesn't affect functionality

---

## Technical Details

### Audio Pipeline
```
Microphone (hardware) → AVAudioEngine inputNode
    │
    ├── installTap (format: nil, bufferSize: 1024)
    │   └── Callback runs on audio thread
    │
    ├── Check atomic flags: connected, muted, speaking
    │   └── If speaking=true, skip to prevent feedback
    │
    ├── Convert Float32 → PCM16 @ 24kHz
    │   └── Linear interpolation for resampling
    │
    └── Task.detached → WebSocket.sendPCMData()
        └── Base64 encode → input_audio_buffer.append

Response Audio:
WebSocket → response.audio.delta
    │
    ├── Base64 decode → PCM16 data
    │
    ├── Convert PCM16 → Float32 buffer
    │
    └── AVAudioPlayerNode.scheduleBuffer()
        └── Plays through mainMixerNode → speakers
```

### Thread Safety Implementation
```swift
// Atomic flags (OSAllocatedUnfairLock)
private let isConnectedAtomic = OSAllocatedUnfairLock(initialState: false)
private let isMutedAtomic = OSAllocatedUnfairLock(initialState: false)
private let isSpeakingAtomic = OSAllocatedUnfairLock(initialState: false)

// In tap callback (audio thread):
let speaking = self.isSpeakingAtomic.withLock { $0 }
guard !speaking else { return }  // Skip if AI is speaking

// In updateState (main thread):
isSpeakingAtomic.withLock { $0 = (newState == .speaking) }
```

### Key Files Modified
- `VoiceManager.swift` - Core audio capture and playback
- `OpenAIRealtimeWebSocket.swift` - WebSocket communication
- `ConvergioApp.swift` - Added MicrophoneTestView

---

## Next Steps

### Immediate (P0)
1. [ ] Investigate -10877 errors root cause
2. [ ] Test memory usage over extended session
3. [ ] Fix SwiftUI publishing warning

### Short Term (P1)
4. [ ] Add explicit audio commit to help server VAD
5. [ ] Implement session timeout/auto-recovery
6. [ ] Test with different microphones

### Medium Term (P2)
7. [ ] Add voice activity detection on client side
8. [ ] Implement barge-in properly (interrupt AI while speaking)
9. [ ] Add audio quality indicators to UI

---

## Testing Checklist

After changes, verify:
- [ ] MicrophoneTestView shows audio level > 0 when speaking
- [ ] VoiceSessionView shows "Sent X buffers (rms=0.XXXX)" with non-zero RMS
- [ ] Server detects speech START and STOP
- [ ] AI responds in Italian
- [ ] Audio playback is audible
- [ ] No feedback loop (AI doesn't repeat itself)
- [ ] End Session works
- [ ] Memory usage is stable

---

## How to Debug

### Enable verbose logging
The debug panel in VoiceSessionView shows:
- Connection status
- Buffer counts
- RMS levels
- Server events (speech START/STOP)
- Transcripts

### Console logging
Filter Xcode console for:
- `[TAP]` - Microphone tap events
- `[ENGINE]` - Audio engine lifecycle
- `throwing` - CoreAudio errors
- `Error` - General errors

### Reset audio if stuck
```bash
sudo killall coreaudiod
```
Then restart the app.

---

## References
- [OpenAI Realtime API](https://platform.openai.com/docs/guides/realtime)
- [AVAudioEngine Documentation](https://developer.apple.com/documentation/avfaudio/avaudioengine)
- [CoreAudio Error Codes](https://developer.apple.com/documentation/coreaudiotypes/1572096-audio_unit_errors)
