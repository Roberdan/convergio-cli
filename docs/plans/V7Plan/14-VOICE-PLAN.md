# Convergio V7 — Voice Plan (Mac + Web, Azure)

**Status:** Draft for approval  
**Date:** 2025-12-26  
**Purpose:** Define voice architecture, targets, degraded modes, and instrumentation.

---

## 1) Decision Required: Transport

Pick one for Phase 1:
- **Option V1 (Recommended initially): WebSocket voice streaming**
  - simpler, faster to ship
  - easier to operate
- **Option V2: WebRTC**
  - better for real-time media QoS
  - higher complexity (NAT traversal, signaling, jitter handling)

**Rule:** do not ship both in the MVP.

---

## 2) Targets (SLO-like)

- WS connect time P95 < 1.5s
- First token P95 < 1.5s (cloud), < 1.0s (local)
- Dropout rate < 5% per 10 min session
- Reconnect success > 99% within 3 retries

---

## 3) Client Architecture

### macOS
- Audio capture + VAD
- Echo cancellation / noise suppression (where available)
- Stream chunks to `/v1/voice/ws`

### Web
- Web Audio capture
- Browser permission flows
- Handle background tab throttling

---

## 4) Backend Architecture (Azure)

- Voice WS endpoint in API Gateway
- Policy enforcement:
  - quotas
  - max audio length
  - budget checks (managed lane)
- Metering emits:
  - `audio_seconds_in/out`
  - tokens

---

## 5) Degraded Modes (Mandatory)

- If VAD fails → push-to-talk
- If WS disconnects → retry with backoff; after 3 failures → text-only
- If voice provider fails → fallback to local/alternate; else transcript-only
- If echo/feedback detected → reduce playback or prompt user

---

## 6) Instrumentation

Log/measure:
- connect time, reconnect attempts
- first-token latency
- dropout events and reasons
- audio RMS/clipping bands

**Privacy rule:** no raw audio stored by default.

---

## 7) Acceptance

- All targets met for P95
- Degraded modes work
- Telemetry is present

