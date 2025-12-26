# Convergio V7 — API Contract (HTTP + WebSocket)

**Status:** Draft for approval  
**Date:** 2025-12-26  
**Purpose:** Contract-first interface so teams (and AI agents) can build in parallel.

---

## 1) Versioning Rules

- HTTP base path: `/v1/...`
- WebSocket endpoint: `/v1/ws`
- Breaking changes require `/v2`.
- Deprecations: minimum 90-day notice for SaaS.

---

## 2) Identity & Auth

- `tenant_id` is required for all usage attribution.
- Auth modes:
  - consumer: session token
  - schools/business: org admin + users + roles

**BYOK model:** keys are stored server-side (Key Vault) and referenced by key id; raw keys never returned.

---

## 3) Core HTTP Endpoints (Minimum)

### Session & Auth
- `POST /v1/auth/login`
- `POST /v1/auth/logout`
- `GET  /v1/me`

### Chat / Orchestration
- `POST /v1/chat` (non-stream)
- `POST /v1/chat/stream` (upgrades to WS or SSE; choose one and standardize)

### WebSocket
- `GET /v1/ws` (upgrade)

### Voice (if WS-based)
- `GET /v1/voice/ws` (upgrade)

### Agents & Plugins
- `GET  /v1/agents`
- `GET  /v1/plugins`
- `POST /v1/plugins/install` (admin only)

### Usage & Billing
- `GET  /v1/usage/current`
- `GET  /v1/usage/events` (admin)
- `POST /v1/billing/portal` (Stripe/Paddle hosted portal)

---

## 4) WebSocket Protocol (Event Types)

All WS frames are JSON objects with:
- `type` (string)
- `request_id` (string)
- `timestamp_ms`

### Client → Server
- `chat.start` { prompt, agent_id, mode: byok|managed, model_hint? }
- `chat.cancel` { request_id }
- `voice.start` { codec, sample_rate, mode }
- `voice.chunk` { request_id, bytes_base64 }
- `voice.stop` { request_id }

### Server → Client
- `chat.token` { request_id, text }
- `chat.complete` { request_id, usage: {in_tokens, out_tokens, tool_calls, audio_in_s, audio_out_s} }
- `tool.call` { request_id, tool_name, arguments }
- `tool.result` { request_id, tool_name, result }
- `error` { request_id?, code, message, retryable }

**Reconnect rule:** client may reconnect and resume by providing last seen `event_id` (optional phase 2).

---

## 5) Error Taxonomy (Minimum)

- `auth.invalid`
- `quota.exceeded`
- `billing.payment_required`
- `byok.missing_key`
- `provider.timeout`
- `provider.rate_limited`
- `provider.unavailable`
- `internal`

**Rule:** errors must be stable; UI copy can change, error codes cannot.

---

## 6) Performance Requirements (API)

- P95 first-token < 1.5s for streaming in typical conditions.
- Budget checks must happen before provider calls.
- Every response must include a `request_id` and usage summary (even in BYOK mode).

