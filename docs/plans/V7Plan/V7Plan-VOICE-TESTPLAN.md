# Voice Test Plan (Mac & Web)

Goals: validate latency, stability, and degraded/offline behaviors.

## Targets
- Connect time (WS) < 1.5 s P95.  
- First token < 1.5 s P95 (cloud), < 1.0 s local.  
- Dropout rate < 5% per 10 min session.  
- Reconnect success > 99% within 3 retries.  
- Audio quality score (RMS/clipping) within acceptable band.

## Scenarios (Mac)
- Happy path: start/stop, continuous speech, long turn (2 min).  
- VAD: short utterances, silence gaps, background chatter.  
- AEC/Noise suppression: speaker on, ambient noise.  
- Network: wifi drop/reconnect, high latency, packet loss.  
- CPU pressure: simulate load while streaming.  
- Offline fallback: force offline, ensure local STT/TTS path works or graceful degrade to text mode.

## Scenarios (Web)
- Browsers: Chrome, Safari, Firefox (latest).  
- Permissions: mic allow/deny, re-prompt.  
- Web Audio: buffer underrun/overrun, sample rate mismatch.  
- Network: throttle to 3G, drop, reconnect.  
- Mobile: responsive UI, background tab throttling.

## Degraded Modes
- If VAD fails → default to push-to-talk.  
- If WS disconnects → retry with backoff; fall back to text after 3 fails.  
- If TTS/STT provider fails → switch to local/alternate; otherwise show transcript-only.  
- If echo/feedback detected → auto-reduce playback volume or prompt user.

## Instrumentation
- Log connect time, first-token latency, dropout events, reconnect attempts, audio levels.  
- Metrics per provider (local vs cloud), per platform.  
- Client logs redacted; no raw audio stored.

## Acceptance
- All targets met for P95; no critical bugs in degraded paths.  
- Run on every release touching voice stack; include in smoke suite.

