# Voice Implementation Plan - Scuola 2026

Last Updated: 2025-12-25 11:20

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
- [x] Historical portrait images for 17 maestri (from Wikimedia Commons)
- [x] MaestroCardView updated to show images
- [x] MaestroDetailView updated to show images
- [x] All images converted to proper PNG format

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
  - All 17+ maestri use Azure OpenAI
  - Verify model configuration
  - Remove/disable Anthropic option for EDU

### P2: New Maestro
- [ ] Add 18th maestro: **Diritto Internazionale** (International Law)
  - Choose historical figure (e.g., Grozio/Hugo Grotius)
  - Add to Maestro.swift
  - Add portrait image
  - Configure voice and teaching style
  - Update EditionManager.swift maestri list

### P3: UI Improvements
- [ ] **Improve Settings window UI** (current design is poor)
- [ ] **Fix theme management** (dark/light/system) - not working correctly
- [ ] Verify all 17 maestri images display correctly
- [ ] Replace 3 placeholder images:
  - Manzoni (currently red "M" placeholder)
  - Note: Erodoto and Humboldt now have real images

### P4: Voice Integration
- [ ] Add voice transcription to chat history
- [ ] Save voice sessions to student progress
- [ ] Test global hotkey activation from background

### P5: Accessibility
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
- Current count: 17 images
- 1 placeholder remaining (Manzoni)

### Font Configuration
- Fonts in: `Resources/Fonts/`
- Info.plist: `ATSApplicationFontsPath = "Fonts"`
- Font family: "OpenDyslexic"

### Edition Configuration
- EDU edition: 17 maestri + 3 coordinatori = 20 agents
- Provider: Azure OpenAI ONLY (no Anthropic fallback)

---

## Commits Today
- `79ac559` feat(websocket): add automatic reconnection with exponential backoff
- `35558c5` feat(ui): add historical portrait images for all 17 maestri
- `e9980e3` fix(ui): update missing maestri images and add photos to detail view
- `c0ab3f2` fix(assets): convert all maestri images to proper PNG format

---

## How to Resume This Session
After Mac reboot, run:
```bash
cd /Users/roberdan/GitHub/ConvergioCLI/native-scuola-2026/ConvergioApp
claude
```

Then say: "Continua con il piano voice. Ho riavviato il Mac."
