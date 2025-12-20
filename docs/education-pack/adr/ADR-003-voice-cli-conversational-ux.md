# ADR-003: Conversational Voice UX for CLI and Beyond

**Date**: 2025-12-20
**Status**: Accepted
**Deciders**: Roberto, AI Team
**Priority**: P0 CRITICAL
**Supersedes**: Partially updates ADR-002

## Context

We need voice interaction that feels exactly like ChatGPT Advanced Voice Mode:
- **Natural conversation**: Both parties can speak freely
- **Interruptions**: User can interrupt the maestro mid-sentence (barge-in)
- **No buttons**: Conversation flows naturally, no push-to-talk required
- **Context awareness**: Maestro remembers the full conversation
- **Emotional intelligence**: Adapts tone based on user's voice

This must work in:
1. **CLI (Terminal)** - Primary target
2. **Zed Editor** - Via ACP integration
3. **Native App** - Future macOS/iOS app

## Decision

### Full-Duplex Conversational Voice

We implement **always-listening, always-ready** voice interaction:

```
┌─────────────────────────────────────────────────────────────┐
│                    CONVERSATION FLOW                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Student: "Euclide, come si calcola—"                       │
│                                                              │
│  Euclide: "L'area del cerchio! Ottima domanda. La formula   │
│           è pi greco per il raggio al quadr—"               │
│                                                              │
│  Student: "Aspetta, cos'è pi greco?"                        │
│           ↑ INTERRUPTION - Euclide stops immediately        │
│                                                              │
│  Euclide: "Pi greco è un numero speciale, circa 3.14159...  │
│           Immagina di prendere una corda lunga quanto il    │
│           bordo di un cerchio..."                           │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### No Push-to-Talk Required

Unlike traditional voice assistants, we use **continuous Voice Activity Detection (VAD)**:

| Mode | How it works | When to use |
|------|--------------|-------------|
| **Continuous (default)** | Always listening, VAD detects speech | Natural conversation |
| **Push-to-talk** | Hold key to speak | Noisy environments |
| **Hotkey toggle** | Cmd+Shift+V to toggle | Quick activation |

### Interruption Handling (Barge-in)

When the user starts speaking while maestro is talking:

```
1. VAD detects user speech
2. Immediately send response.cancel to API
3. Stop audio playback (< 50ms)
4. Buffer user's speech
5. Send new input when user stops
6. Maestro responds to interruption naturally
```

The maestro is instructed to handle interruptions gracefully:
> "If interrupted, acknowledge naturally. Say things like 'Certo, dimmi' or 'Sì?' and wait for the question."

## CLI Implementation

### Visual Feedback (ASCII)

```
┌─────────────────────────────────────────────────────────────┐
│ convergio> /voice euclide                                    │
│                                                              │
│ ╔═══════════════════════════════════════════════════════════╗
│ ║  🎓 EUCLIDE - Maestro di Matematica                       ║
│ ║  Voice mode active. Just speak naturally.                 ║
│ ║  Press ESC to exit, M to mute mic.                        ║
│ ╚═══════════════════════════════════════════════════════════╝
│                                                              │
│  ░░░░░░░░░░░░░░░░░░░░  Listening...                         │
│                                                              │
│  You: "Come si calcola l'area del cerchio?"                 │
│       ▁▂▃▅▆▇█▇▆▅▃▂▁                                         │
│                                                              │
│  Euclide: "Ah, l'area del cerchio! È una delle formule      │
│           più eleganti della geometria..."                   │
│           🔊 ████████████░░░░░░░░                            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### State Machine

```
                    ┌──────────────┐
                    │    IDLE      │
                    │  Listening   │
                    └──────┬───────┘
                           │ VAD detects speech
                           ▼
                    ┌──────────────┐
                    │   HEARING    │
                    │ Buffering... │
                    └──────┬───────┘
                           │ Silence detected (500ms)
                           ▼
                    ┌──────────────┐
                    │  PROCESSING  │
                    │ Sending...   │
                    └──────┬───────┘
                           │ Response starts
                           ▼
                    ┌──────────────┐
        ┌──────────│   SPEAKING   │
        │          │ Maestro talk │
        │          └──────┬───────┘
        │                 │ Response ends
        │ User speaks     │
        │ (barge-in)      ▼
        │          ┌──────────────┐
        └─────────►│    IDLE      │
                   │  Listening   │
                   └──────────────┘
```

### CLI Commands

```bash
# Start voice mode
/voice                     # With current/default maestro
/voice euclide            # With specific maestro
/voice feynman fisica     # With maestro and topic context

# During voice mode
ESC                        # Exit voice mode
M                          # Toggle mute microphone
T                          # Toggle transcript display
S                          # Save conversation
SPACE                      # (Optional) Push-to-talk if VAD disabled

# Configuration
/voice config vad on       # Enable Voice Activity Detection (default)
/voice config vad off      # Disable VAD, use push-to-talk
/voice config sensitivity 0.7  # VAD sensitivity (0-1)
/voice config language it  # Set language
```

### Audio Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│                      AUDIO PIPELINE                          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   ┌──────────┐    ┌──────────┐    ┌──────────────────────┐  │
│   │   Mic    │───►│   VAD    │───►│  WebSocket Client    │  │
│   │ CoreAudio│    │ (local)  │    │  (libwebsockets)     │  │
│   └──────────┘    └──────────┘    └──────────┬───────────┘  │
│                                               │              │
│                                               ▼              │
│                                   ┌──────────────────────┐  │
│                                   │   Azure OpenAI       │  │
│                                   │   Realtime API       │  │
│                                   │   (gpt-realtime)     │  │
│                                   └──────────┬───────────┘  │
│                                               │              │
│   ┌──────────┐    ┌──────────┐    ┌──────────▼───────────┐  │
│   │ Speaker  │◄───│  Buffer  │◄───│  WebSocket Client    │  │
│   │ CoreAudio│    │ (ring)   │    │  (audio chunks)      │  │
│   └──────────┘    └──────────┘    └──────────────────────┘  │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### Real-time Transcript

While speaking, show partial transcript updating in real-time:

```
You: "Come si_"           # Partial
You: "Come si calcola_"   # Updating
You: "Come si calcola l'area del cerchio?"  # Final

Euclide: "Ah, l'area_"    # Streaming
Euclide: "Ah, l'area del cerchio! È una_"
Euclide: "Ah, l'area del cerchio! È una delle formule più eleganti..."
```

## Zed Integration

Voice works through ACP (Agent Control Protocol):

```json
{
  "type": "voice.start",
  "maestro": "euclide",
  "context": {
    "file": "geometry.md",
    "selection": "circle area formula"
  }
}
```

Zed can:
1. Show voice indicator in status bar
2. Display transcript in panel
3. Use editor context for smarter responses

## Native App (Future)

SwiftUI implementation with:
- Animated waveform visualization
- Floating voice bubble
- Transcript with speaker labels
- Conversation history

## Technical Requirements

### Dependencies

| Library | Purpose | Installation |
|---------|---------|--------------|
| libwebsockets | WebSocket client | `brew install libwebsockets` |
| CoreAudio | Audio I/O (macOS) | Built-in |
| libfvad | Voice Activity Detection | Optional, can use server VAD |

### Audio Format

| Direction | Format | Sample Rate | Channels |
|-----------|--------|-------------|----------|
| Input (mic) | PCM 16-bit LE | 24000 Hz | Mono |
| Output (speaker) | PCM 16-bit LE | 24000 Hz | Mono |

### Latency Targets

| Metric | Target | Acceptable |
|--------|--------|------------|
| VAD detection | < 100ms | < 200ms |
| Barge-in response | < 100ms | < 150ms |
| First audio chunk | < 300ms | < 500ms |
| End-to-end | < 500ms | < 800ms |

## Maestro Voice Personality

Each maestro has distinct voice characteristics:

```c
typedef struct {
    const char* maestro_id;
    const char* openai_voice;      // sage, echo, coral, alloy, verse, shimmer
    const char* speaking_style;    // Instructions for natural speech
    float default_speed;           // 0.8 - 1.2
    bool uses_pauses;              // Thoughtful pauses
    const char* interruption_ack;  // What to say when interrupted
} MaestroVoicePersonality;

// Example: Euclide
{
    .maestro_id = "euclide-matematica",
    .openai_voice = "sage",
    .speaking_style = "Speak calmly and methodically. Use deliberate pauses "
                      "when explaining complex concepts. Never rush.",
    .default_speed = 0.9,
    .uses_pauses = true,
    .interruption_ack = "Sì, dimmi pure..."
}

// Example: Feynman
{
    .maestro_id = "feynman-fisica",
    .openai_voice = "echo",
    .speaking_style = "Speak with enthusiasm and energy! Get excited about "
                      "physics. Use analogies. Laugh when appropriate.",
    .default_speed = 1.1,
    .uses_pauses = false,
    .interruption_ack = "Oh! What's on your mind?"
}
```

## Conversation Context

The voice session maintains full context:

```json
{
  "session_id": "voice_2025_12_20_001",
  "maestro": "euclide-matematica",
  "student_profile": {
    "name": "Mario",
    "grade": "3° Liceo Scientifico",
    "accessibility": {
      "dyslexia": true,
      "speech_rate": 0.9
    }
  },
  "conversation_history": [
    {"role": "user", "content": "Come si calcola l'area del cerchio?"},
    {"role": "assistant", "content": "L'area del cerchio si calcola..."}
  ],
  "current_topic": "geometria/cerchio/area",
  "emotion_state": "curious"
}
```

## Error Handling

| Error | User Experience | Recovery |
|-------|-----------------|----------|
| Network lost | "Connessione persa. Riprovo..." | Auto-reconnect 3x |
| API error | "Euclide sta pensando..." | Retry with backoff |
| Mic unavailable | "Microfono non disponibile. Usa /text" | Fallback to text |
| High latency | Show buffering indicator | Increase buffer |

## Privacy Considerations

1. **Audio is never stored locally** (unless user saves transcript)
2. **API calls use HTTPS/WSS** (encrypted)
3. **Azure EU region** (GDPR compliant)
4. **Explicit opt-in** for voice mode

## Success Metrics

1. **Latency** < 500ms end-to-end
2. **Barge-in success** > 95%
3. **Conversation naturalness** NPS > 60
4. **Zero audio glitches** in 10-minute session

## Implementation Phases

### Phase 1: Basic Voice (1 week)
- [ ] WebSocket connection to Azure
- [ ] Audio capture (CoreAudio)
- [ ] Audio playback
- [ ] Basic `/voice` command

### Phase 2: Conversational (1 week)
- [ ] Server-side VAD
- [ ] Barge-in handling
- [ ] Real-time transcript
- [ ] Maestro voice switching

### Phase 3: Polish (1 week)
- [ ] CLI visual feedback
- [ ] Error handling
- [ ] Latency optimization
- [ ] Multi-maestro sessions

## References

- [OpenAI Realtime API](https://platform.openai.com/docs/guides/realtime)
- [Azure OpenAI Realtime](https://learn.microsoft.com/en-us/azure/ai-foundry/openai/how-to/realtime-audio-websockets)
- [libwebsockets](https://libwebsockets.org/)
- [CoreAudio Programming Guide](https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/CoreAudioOverview/)

---

*This ADR defines the conversational voice experience for Convergio Education Pack.*
