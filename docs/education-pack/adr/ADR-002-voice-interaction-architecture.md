# ADR-002: Real-Time Voice Interaction Architecture

**Date**: 2025-12-20
**Status**: Proposed
**Deciders**: Roberto, AI Team
**Priority**: P0 CRITICAL

## Context

The Education Pack needs fluid, natural voice interaction with all 14+ maestri - similar to ChatGPT Advanced Voice Mode or Gemini Live. Students should be able to converse with their teachers naturally, with emotional awareness, interruption handling, and customizable voices per maestro.

### Requirements

1. **Ultra-low latency** (<300ms response time for natural conversation)
2. **Emotional intelligence** - Detect student frustration, confusion, engagement
3. **Interruption handling** (barge-in) - Students can interrupt mid-response
4. **Multi-language** - Italian primary, with English, Spanish, French, German
5. **Custom voices** - Each maestro should have distinct personality/voice
6. **Accessibility** - Works with screen readers, adjustable speech rate

## Options Evaluated (December 2025)

### Option 1: OpenAI GPT-Realtime API
**Status**: Generally Available (August 2025)

**Pros**:
- Speech-to-speech native architecture (no transcription overhead)
- Emotional inflection and tone adjustment
- Mid-sentence language switching
- Function calling support
- Best instruction-following (30.5% on MultiChallenge)
- Backed by GPT-4o intelligence

**Cons**:
- Pricing: $32/1M audio input, $64/1M audio output
- Limited voice customization (6 preset voices)
- No voice cloning

**Latency**: ~500ms time-to-first-byte

### Option 2: Gemini Live API (Gemini 2.5 Flash Native Audio)
**Status**: Generally Available on Vertex AI (December 2025)

**Pros**:
- 24 languages supported
- Voice Activity Detection (VAD) with barge-in
- Affective dialog (adapts to user emotions)
- Tool use / function calling
- Audio transcriptions for both sides
- WebRTC support via Daily, Twilio, LiveKit, Voximplant

**Cons**:
- Requires Google Cloud / Vertex AI setup
- Less emotional expressiveness than competitors

**Latency**: Sub-300ms in production

### Option 3: ElevenLabs Conversational AI 2.0
**Status**: Available (May 2025)

**Pros**:
- 5,000+ voices in 70+ languages
- Voice cloning for custom maestro voices
- Ultra-low latency (~75ms Flash model)
- Connects to GPT-4, Claude, Gemini as backend LLM
- "Um/ah" detection for natural interruption timing
- WebSocket API + SDKs (JS, React, Python, iOS)

**Cons**:
- Requires separate LLM for intelligence
- Additional orchestration needed

**Latency**: ~75-130ms to first audio

### Option 4: Hume AI EVI 3 (Empathic Voice Interface)
**Status**: Available (May 2025)

**Pros**:
- BEST emotional intelligence (outperforms GPT-4o on 8/9 emotions)
- 100,000+ custom voices via prompt
- Integration with Claude (most popular, 36% of users)
- WebSocket API for real-time
- Detects frustration, confusion, excitement in voice

**Cons**:
- Less mainstream adoption
- Requires custom voice prompting

**Latency**: <200ms

### Option 5: Cartesia Sonic-3
**Status**: Available 2025

**Pros**:
- FASTEST latency (90ms)
- High-quality, virtually human voices
- Conversational AI optimized

**Cons**:
- Less emotional intelligence
- Smaller ecosystem

## Decision

### Primary: Hume AI EVI 3 + Claude
### Fallback: OpenAI GPT-Realtime

**Rationale**:

1. **Emotional Intelligence is Critical for Education**
   - Students exhibit frustration, confusion, boredom, excitement
   - Hume EVI 3 outperforms all others in emotion detection
   - A frustrated student needs different response than an engaged one
   - Perfect match for accessibility (dyscalculia anxiety, ADHD frustration)

2. **Claude Integration Already In Place**
   - Our maestri already use Claude as backend
   - Hume's most popular LLM choice is Claude (36%)
   - Seamless integration with existing architecture

3. **100K+ Custom Voices**
   - Each of our 15 maestri needs distinct voice personality:
     - Euclide: Calm, patient, methodical (Greek accent hint)
     - Feynman: Enthusiastic, playful, New York accent
     - Manzoni: Warm, literary, Milanese elegance
     - etc.
   - Hume allows prompt-based voice creation

4. **Latency is Acceptable**
   - <200ms is within conversational tolerance
   - Students won't perceive delay

### Fallback Scenario
If Hume integration proves complex, OpenAI GPT-Realtime provides:
- Simpler integration (single API)
- Good instruction following
- Acceptable latency

## Architecture

```
                    ┌─────────────────────────────────────┐
                    │        STUDENT DEVICE               │
                    │  ┌─────────────┐  ┌─────────────┐  │
                    │  │ Microphone  │  │   Speaker   │  │
                    │  └──────┬──────┘  └──────▲──────┘  │
                    │         │                │         │
                    │         ▼                │         │
                    │  ┌─────────────────────────────┐   │
                    │  │   Convergio Education App   │   │
                    │  │   - Voice Activity Detect   │   │
                    │  │   - Audio Streaming         │   │
                    │  └──────┬─────────────────▲────┘   │
                    └─────────┼─────────────────┼────────┘
                              │ WebSocket        │
                              ▼                  │
                    ┌─────────────────────────────────────┐
                    │       CONVERGIO VOICE GATEWAY       │
                    │                                     │
                    │  ┌─────────────────────────────┐   │
                    │  │     Hume EVI 3 WebSocket    │   │
                    │  │   - Emotion detection       │   │
                    │  │   - Voice synthesis         │   │
                    │  │   - Barge-in handling       │   │
                    │  └──────┬─────────────────▲────┘   │
                    │         │ Emotion+Text     │ Response│
                    │         ▼                  │        │
                    │  ┌─────────────────────────────┐   │
                    │  │    Maestro Router + Claude  │   │
                    │  │   - Route to correct maestro│   │
                    │  │   - Inject student profile  │   │
                    │  │   - Apply accessibility     │   │
                    │  │   - Emotion-aware response  │   │
                    │  └─────────────────────────────┘   │
                    └─────────────────────────────────────┘
```

## Voice Profiles per Maestro

| Maestro | Voice Character | Tone | Accent | Speed |
|---------|----------------|------|--------|-------|
| Euclide | Calm, precise | Methodical | Greek-Italian | Moderate |
| Feynman | Enthusiastic | Playful | American | Fast |
| Manzoni | Warm, literary | Elegant | Milanese | Measured |
| Darwin | Curious, gentle | Inquisitive | British | Moderate |
| Erodoto | Storyteller | Dramatic | Greek | Variable |
| Leonardo | Creative, passionate | Inspiring | Tuscan | Expressive |
| Mozart | Musical, joyful | Melodic | Austrian | Lively |
| Shakespeare | Theatrical | Dramatic | British | Rich |
| Feynman | Enthusiastic | Casual | American | Quick |
| Cicerone | Authoritative | Persuasive | Roman | Oratorical |
| Smith | Analytical | Clear | Scottish | Steady |
| Lovelace | Precise, encouraging | Logical | British | Clear |
| Ippocrate | Caring, calm | Nurturing | Greek | Soothing |
| Socrate | Questioning | Curious | Greek | Thoughtful |
| Chris (NEW) | Engaging | Inspiring | American | Dynamic |

## Emotion-Aware Responses

When Hume EVI 3 detects:

| Emotion | Action |
|---------|--------|
| **Frustration** | Slow down, simplify, offer break |
| **Confusion** | Rephrase, use visual, step back |
| **Boredom** | Add challenge, gamify, change topic |
| **Excitement** | Match energy, go deeper |
| **Anxiety** | Reassure, praise effort, reduce pressure |
| **Distraction** | Re-engage, summarize, ask question |

## Implementation Phases

### Phase 1: Core Integration (2 weeks)
- [ ] Hume EVI 3 WebSocket integration
- [ ] Basic voice input/output
- [ ] Single maestro voice test (Euclide)

### Phase 2: Emotion Pipeline (1 week)
- [ ] Emotion detection integration
- [ ] Response adaptation based on emotion
- [ ] Logging for analytics

### Phase 3: All Maestri Voices (1 week)
- [ ] Create 15 voice profiles via Hume prompts
- [ ] Voice routing per active maestro
- [ ] Voice switching on maestro change

### Phase 4: Accessibility (1 week)
- [ ] Adjustable speech rate
- [ ] TTS fallback for offline
- [ ] Screen reader compatibility

## Fallback Chain

```
1. Hume EVI 3 (primary)
   ↓ (if unavailable)
2. OpenAI GPT-Realtime
   ↓ (if unavailable)
3. ElevenLabs + Claude pipeline
   ↓ (if unavailable)
4. Local TTS (macOS `say` command)
```

## Cost Estimation

### Hume EVI 3
- ~$0.10-0.20 per minute of conversation
- Estimated 20 min/day average student usage
- ~$4/month per active student

### OpenAI Realtime (fallback)
- $32/1M input tokens, $64/1M output tokens
- 1 minute ≈ 2500 tokens each direction
- ~$0.24 per minute
- ~$144/month per active student (more expensive)

**Recommendation**: Hume as primary is 30x more cost-effective

## Success Metrics

1. **Latency** < 200ms to first audio
2. **Emotion accuracy** > 80% correct detection
3. **Barge-in success** > 95% handled gracefully
4. **Student satisfaction** NPS > 50
5. **Accessibility compliance** WCAG 2.2 AA

## References

- [OpenAI Realtime API](https://openai.com/index/introducing-gpt-realtime/)
- [Gemini Live API](https://ai.google.dev/gemini-api/docs/live)
- [ElevenLabs Conversational AI 2.0](https://elevenlabs.io/conversational-ai)
- [Hume AI EVI 3](https://www.hume.ai/blog/introducing-evi-3)
- [Cartesia Sonic-3](https://cartesia.ai/sonic)
- [A16Z Voice AI Agents 2025](https://a16z.com/ai-voice-agents-2025-update/)
- [Stream Real-Time Speech APIs](https://getstream.io/blog/speech-apis/)

---

*This ADR documents the architectural decision for voice interaction in Convergio Education Pack.*
