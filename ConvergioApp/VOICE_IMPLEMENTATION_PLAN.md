# Voice Implementation Plan - Scuola 2026

Last Updated: 2025-12-25 10:27

## Current Status

### Completed
- [x] OpenAI Realtime Audio WebSocket connection (Azure endpoint)
- [x] Audio format conversion (Float32 48kHz → PCM16 24kHz)
- [x] Real-time waveform visualization with RMS levels
- [x] OpenDyslexic font installed and configured
- [x] Microphone entitlements added
- [x] Debug logging panel in VoiceSessionView
- [x] Global hotkey Cmd+Shift+Space wired up in AppDelegate
- [x] Student Profile tab in Settings
- [x] WebSocket reconnection with exponential backoff (5 attempts, 1s-16s delays)
- [x] Historical portrait images for 18 maestri (from Wikimedia Commons)
- [x] MaestroCardView updated to show images
- [x] MaestroDetailView updated to show images
- [x] All images converted to proper PNG format
- [x] **NEW: Added 18th maestro - Grozio (Hugo Grotius) for International Law**
- [x] **NEW: Created documentation for adding new maestri** (`docs/HOW_TO_ADD_MAESTRO.md`)

### Blocked (Requires Mac Reboot)
- [ ] Microphone audio capture returns all zeros
  - Root cause: macOS TCC permission cache stale
  - Standalone Swift test also fails = system issue, not code
  - Solution: Reboot Mac to reset TCC cache

---

## Priority Tasks

### P0: Critical (After Reboot)
1. [ ] Test voice session end-to-end
2. [ ] Verify microphone captures audio (waveform should animate)
3. [ ] Verify AI responses come through
4. [ ] Test emotion detection from voice

### P1: Azure OpenAI Enforcement
- [ ] **EDU edition MUST use ONLY Azure OpenAI**
  - Never fallback to Anthropic
  - All 18 maestri use Azure OpenAI
  - Verify model configuration
  - Remove/disable Anthropic option for EDU

### P2: UI Improvements
- [ ] **Improve Settings window UI** (current design is poor)
- [ ] **Fix theme management** (dark/light/system) - not working correctly
- [ ] Verify all 18 maestri images display correctly
- [ ] Replace placeholder images:
  - Manzoni (currently red "M" placeholder)
  - Grozio (currently indigo "G" placeholder)

### P3: Voice Integration
- [ ] Add voice transcription to chat history
- [ ] Save voice sessions to student progress
- [ ] Test global hotkey activation from background

### P4: Accessibility
- [ ] Verify OpenDyslexic font works in dyslexia mode
- [ ] Test all accessibility settings together
- [ ] Edit student profile inline (currently read-only)

---

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

### WebSocket Reconnection
```swift
// Exponential backoff: 1s, 2s, 4s, 8s, 16s
// Max attempts: 5
// Auto-reconnect on unexpected disconnect
```

### Maestri Images
- Location: `Assets.xcassets/Maestri/`
- Format: PNG (200x200 minimum)
- Current count: 18 images
- 2 placeholders remaining (Manzoni, Grozio)

### Font Configuration
- Fonts in: `Resources/Fonts/`
- Info.plist: `ATSApplicationFontsPath = "Fonts"`
- Font family: "OpenDyslexic"

### Edition Configuration
- EDU edition: 18 maestri + 3 coordinatori = 21 agents
- Provider: Azure OpenAI ONLY (no Anthropic fallback)

### Documentation
- `docs/HOW_TO_ADD_MAESTRO.md` - Guide for adding new maestri

---

## Commits Today
- `79ac559` feat(websocket): add automatic reconnection with exponential backoff
- `35558c5` feat(ui): add historical portrait images for all 17 maestri
- `e9980e3` fix(ui): update missing maestri images and add photos to detail view
- `c0ab3f2` fix(assets): convert all maestri images to proper PNG format
- `910adf2` feat: add Grozio maestro for International Law with documentation

---

## How to Resume This Session
After Mac reboot, run:
```bash
cd /Users/roberdan/GitHub/ConvergioCLI/native-scuola-2026/ConvergioApp
claude
```

Then say: "Continua con il piano voice. Ho riavviato il Mac."
