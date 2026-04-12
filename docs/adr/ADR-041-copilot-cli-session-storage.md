# ADR-041 — Copilot CLI Session Storage Prompt

**Status:** Accepted
**Date:** 2026-04-10
**Author:** Roberdan

## Context

Copilot CLI (v1.0.23+) introduces a **cloud session sync** feature gated by the
`CLOUD_SESSION_STORE` feature flag. When enabled (default for GitHub staff accounts),
the CLI shows an interactive prompt on every session start:

```
Session storage
Choose where your Copilot sessions are stored.

❯1. Keep on this device only
 2. Sync to my account
 3. Sync to the repository for my team
```

This blocks automated and interactive workflows, requiring manual selection each time
unless a persistent preference is saved.

## Problem

- The prompt appears on **every new session** if no preference is persisted.
- There is **no documented config key** in official Copilot CLI docs or `copilot help config`.
- The `--allow-all` / `--yolo` flags do **not** suppress this prompt.
- Spawned autonomous agents cannot answer interactive prompts, breaking automation.

## Decision

Set the `sessionSync` key in `~/.copilot/config.json` to persist the preference globally:

```json
{
  "sessionSync": [
    {
      "origin": "*",
      "level": "local"
    }
  ]
}
```

### Key schema (from source code analysis)

| Field    | Type   | Description                                                        |
|----------|--------|--------------------------------------------------------------------|
| `origin` | string | Repository identifier or `"*"` for all repositories               |
| `level`  | string | `"local"`, `"user"` (sync to account), or `"repo_and_user"` (team)|

### Scope values (when selecting interactively)

| Scope              | Meaning                                      |
|--------------------|----------------------------------------------|
| `"this_time"`      | Apply only to current session (not persisted) |
| `"repo"`           | Persist for this repository only              |
| `"all_repos"`      | Persist for all repositories (`origin: "*"`)  |

### How it was found

The config key is **not documented** as of v1.0.24. It was discovered by:

1. Searching Copilot CLI logs (`~/.copilot/logs/`) for `CLOUD_SESSION_STORE` feature flag
2. Extracting the JS source from `~/Library/Caches/copilot/pkg/universal/1.0.24-0/app.js`
3. Finding the `SessionSyncPrompt` component and `ygo()` function that calls
   `Dr.writeKey("sessionSync", ...)` to persist the preference

## Consequences

- The session storage prompt no longer appears on startup.
- All sessions are stored locally only (no cloud sync).
- If cloud sync is desired in the future, update `level` to `"user"` or `"repo_and_user"`.
- Autonomous agents (spawned via `/api/agents/spawn`) are no longer blocked by this prompt.
