# ADR 005: OAuth Authentication (Deferred)

## Status

**Deferred** - OAuth login removed in v2.0.2, API key authentication only.

## Context

Convergio CLI initially included OAuth authentication support to allow users with Claude Max subscriptions to authenticate without needing an API key. The implementation used Anthropic's OAuth endpoints.

## Problem

The OAuth implementation cannot work because:

1. **Client ID Registration Required**: Anthropic requires third-party applications to register as OAuth clients. Using Claude Code's client_id is not permitted.

2. **Redirect URI Mismatch**: Each OAuth application has registered redirect URIs. Convergio's localhost callback doesn't match Claude Code's registered URIs.

3. **No Public OAuth API**: As of December 2025, Anthropic does not offer a public OAuth registration process for third-party CLI tools.

## Decision

Remove the `login` command and OAuth functionality from the user-facing interface. Users must authenticate via API key.

### Current Authentication Methods

1. **Environment Variable**: `export ANTHROPIC_API_KEY=sk-ant-...`
2. **macOS Keychain**: Run `convergio setup` to store securely
3. **Config File**: `~/.convergio/config.toml`

## OAuth Implementation (Preserved for Future)

The OAuth code is preserved in `src/auth/oauth.m` and `src/auth/oauth.h` for future use if Anthropic opens OAuth registration.

### Requirements to Enable OAuth

To enable OAuth authentication in the future:

1. **Register with Anthropic**: Apply for OAuth client registration at Anthropic
2. **Obtain Client Credentials**:
   - `client_id`: Unique identifier for Convergio
   - `client_secret`: (if required for confidential clients)
3. **Register Redirect URI**: `http://localhost:PORT/callback`
4. **Update Code**:
   ```c
   // In src/auth/oauth.h, update:
   #define OAUTH_CLIENT_ID "your-registered-client-id"
   ```
5. **Re-enable Login Command**: Restore `cmd_login` in `src/core/main.c` COMMANDS array
6. **Test OAuth Flow**: Verify PKCE flow works with registered credentials

### OAuth Scopes Required

```
user:inference org:inference
```

### PKCE Implementation

The existing implementation uses PKCE (Proof Key for Code Exchange):
- Code verifier: 64-byte random string
- Code challenge: SHA256 hash of verifier, base64url encoded
- Challenge method: S256

## Consequences

### Positive
- Simpler user experience (one clear authentication method)
- No confusion about "Claude Max" login that doesn't work
- Code is preserved for future enablement

### Negative
- Users with Claude Max subscriptions must still obtain an API key
- Cannot leverage subscription-based pricing model

## References

- Anthropic API Documentation: https://docs.anthropic.com/
- OAuth 2.0 PKCE: RFC 7636
- Related files: `src/auth/oauth.m`, `src/auth/oauth.h`, `src/auth/keychain.m`

## Date

2025-12-11
