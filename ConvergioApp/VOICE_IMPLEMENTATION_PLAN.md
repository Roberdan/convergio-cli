# Voice Implementation Plan - Scuola 2026

Last Updated: 2025-12-25

## Current Status

### Completed
- [x] OpenAI Realtime Audio WebSocket connection (Azure endpoint)
- [x] Audio format conversion (Float32 48kHz → PCM16 24kHz)
- [x] Real-time waveform visualization with RMS levels
- [x] OpenDyslexic font installed and configured
- [x] Microphone entitlements added
- [x] Debug logging panel in VoiceSessionView

### Blocked (Requires Mac Reboot)
- [ ] Microphone audio capture returns all zeros
  - Root cause: macOS TCC permission cache stale
  - Standalone Swift test also fails = system issue, not code
  - Solution: Reboot Mac to reset TCC cache

## Post-Reboot Tasks

### Priority 1: Voice Session
1. [ ] Test voice session end-to-end
2. [ ] Verify microphone captures audio (waveform should animate)
3. [ ] Verify AI responses come through
4. [ ] Test emotion detection from voice

### Priority 2: Integration
1. [ ] Add voice transcription to chat history
2. [ ] Handle WebSocket reconnection gracefully
3. [ ] Save voice sessions to student progress

### Priority 3: Accessibility
1. [ ] Verify OpenDyslexic font works in dyslexia mode
2. [ ] Test all accessibility settings together

## Remaining Features

### Student Profile
- [ ] Review student profile section completeness
- [ ] Learning style preferences
- [ ] Subject interests
- [ ] Progress tracking dashboard

### Global Hotkey
- [ ] Implement Cmd+Shift+Space global hotkey
- [ ] Quick voice session start from anywhere

### Apple Intelligence (Future)
- [ ] Investigate Apple Intelligence APIs
- [ ] On-device processing where possible

## Technical Notes

### Audio Pipeline
```
Microphone → AVAudioEngine (48kHz Float32)
    ↓
Convert to PCM16 24kHz mono (little-endian)
    ↓
WebSocket → Azure OpenAI Realtime
    ↓
Response audio → AVAudioPlayerNode
```

### Font Configuration
- Fonts in: `Resources/Fonts/`
- Info.plist: `ATSApplicationFontsPath = "Fonts"`
- Font family: "OpenDyslexic"

### How to Resume This Session
After Mac reboot, run:
```bash
cd /Users/roberdan/GitHub/ConvergioCLI/native-scuola-2026/ConvergioApp
claude
```

Then say: "Continua con il piano voice per Scuola 2026. Il microfono dovrebbe funzionare ora."
