# Implementation Plan: Dual-Mode Authentication (OAuth + API Key)

## Summary

Implement Claude Max OAuth authentication similar to Claude Code CLI, allowing users to choose between:
1. **Claude Max subscription** - OAuth PKCE flow (no API charges)
2. **API Key** - Traditional `ANTHROPIC_API_KEY` (pay-as-you-go)

## Current State

- **File**: `src/neural/claude.c`
- Auth uses `x-api-key` header with `ANTHROPIC_API_KEY` env var
- `CLAUDE_MAX` env var exists but only disables cost tracking
- No OAuth support, no token refresh, no keychain storage

## OAuth Flow (from Claude Code CLI)

```
1. Generate PKCE code_verifier (43-128 chars) + code_challenge (SHA256)
2. Open browser: https://claude.ai/oauth/authorize?
     client_id=9d1c250a-e61b-44d9-88ed-5944d1962f5e
     response_type=code
     redirect_uri=http://localhost:{port}/oauth/callback
     code_challenge={challenge}
     code_challenge_method=S256
     scope=user:inference org:inference
3. Start local HTTP server on {port} to receive callback
4. User logs in and grants permission
5. Exchange code for tokens: POST https://claude.ai/api/oauth/token
6. Store tokens in macOS Keychain
7. Use Bearer token for API calls
8. Refresh token when expired (8 hours)
```

## Implementation

### 1. New File: `src/auth/oauth.h` (Header)

```c
#ifndef OAUTH_H
#define OAUTH_H

#include <stdbool.h>

// Authentication mode
typedef enum {
    AUTH_MODE_NONE = 0,
    AUTH_MODE_API_KEY,
    AUTH_MODE_OAUTH
} AuthMode;

// Token structure
typedef struct {
    char* access_token;      // sk-ant-oat01-...
    char* refresh_token;     // sk-ant-ort01-...
    time_t expires_at;
} OAuthTokens;

// Initialize authentication (detect mode, load credentials)
int auth_init(void);

// Get current auth mode
AuthMode auth_get_mode(void);

// Get authorization header value (API key or Bearer token)
// Returns NULL if not authenticated
char* auth_get_header(void);

// Start OAuth login flow
int auth_oauth_login(void);

// Logout (clear stored credentials)
int auth_logout(void);

// Check if currently authenticated
bool auth_is_authenticated(void);

// Cleanup
void auth_shutdown(void);

#endif
```

### 2. New File: `src/auth/oauth.c` (Implementation)

Key functions:

```c
// PKCE Generation
static void generate_code_verifier(char* verifier, size_t len);
static void generate_code_challenge(const char* verifier, char* challenge);

// Local HTTP Server for callback
static int start_callback_server(int* port);
static char* wait_for_oauth_code(int server_fd);

// Token Operations
static int exchange_code_for_tokens(const char* code, const char* verifier);
static int refresh_access_token(void);

// Keychain Operations (macOS)
static int keychain_store_tokens(const OAuthTokens* tokens);
static int keychain_load_tokens(OAuthTokens* tokens);
static int keychain_delete_tokens(void);
```

### 3. Modify: `src/neural/claude.c`

Update all API calls to use auth module:

```c
// Before (current):
char auth_header[256];
snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", g_api_key);
headers = curl_slist_append(headers, auth_header);

// After:
char* auth_value = auth_get_header();
if (!auth_value) {
    fprintf(stderr, "Not authenticated\n");
    return NULL;
}
char auth_header[512];
if (auth_get_mode() == AUTH_MODE_OAUTH) {
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", auth_value);
} else {
    snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", auth_value);
}
headers = curl_slist_append(headers, auth_header);
free(auth_value);
```

### 4. Modify: `src/core/main.c`

Add new commands:

```c
// New commands in COMMANDS array
{"login",  "Login with Claude Max OAuth", cmd_login},
{"logout", "Logout and clear credentials", cmd_logout},
{"auth",   "Show authentication status", cmd_auth_status},
```

Update init sequence:

```c
// In main():
if (auth_init() != 0) {
    // No valid auth found - prompt user
    printf("No authentication found.\n");
    printf("Options:\n");
    printf("  1. Set ANTHROPIC_API_KEY environment variable\n");
    printf("  2. Run 'login' command to use Claude Max subscription\n");
    return 1;
}
```

### 5. Modify: `Makefile`

Add new source files and Security framework:

```makefile
C_SOURCES = ... \
            $(SRC_DIR)/auth/oauth.c

FRAMEWORKS = ... \
             -framework Security
```

## Files to Create/Modify

| File | Action | Lines |
|------|--------|-------|
| `src/auth/oauth.h` | Create | ~40 |
| `src/auth/oauth.c` | Create | ~400 |
| `include/nous/nous.h` | Modify | +5 (include oauth.h) |
| `src/neural/claude.c` | Modify | ~20 lines changed |
| `src/core/main.c` | Modify | ~50 (new commands) |
| `Makefile` | Modify | +2 lines |

## User Experience

```bash
# API Key mode (traditional)
export ANTHROPIC_API_KEY=sk-ant-...
./convergio

# Claude Max mode (OAuth)
./convergio
convergio> login
Opening browser for Claude Max login...
Waiting for authorization...
✓ Logged in successfully!

convergio> auth
Authentication: Claude Max (OAuth)
Expires: 2025-12-11 15:30:00

# Switch between modes
convergio> logout
✓ Logged out

# Set API key to use API mode
export ANTHROPIC_API_KEY=sk-ant-...
./convergio
```

## Security Considerations

1. **PKCE** prevents authorization code interception
2. **macOS Keychain** securely stores tokens (encrypted)
3. **Local callback server** binds to localhost only
4. **Token refresh** before expiry (8 hours lifetime)
5. **No secrets in code** - Client ID is public (same as Claude Code)

## Implementation Order

1. Create `src/auth/oauth.h` header
2. Create `src/auth/oauth.c` with PKCE + keychain
3. Add `login`/`logout`/`auth` commands to main.c
4. Update `claude.c` to use auth module
5. Update Makefile
6. Test both modes

## OAuth Endpoints (Reference)

| Endpoint | URL |
|----------|-----|
| Authorize | `https://claude.ai/oauth/authorize` |
| Token Exchange | `https://claude.ai/api/oauth/token` |
| Token Refresh | `https://console.anthropic.com/api/oauth/token` |
| Client ID | `9d1c250a-e61b-44d9-88ed-5944d1962f5e` |

## Token Format

- Access: `sk-ant-oat01-...` (8 hour lifetime)
- Refresh: `sk-ant-ort01-...` (long-lived)
