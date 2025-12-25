/**
 * OAuth Authentication Module for Convergio CLI
 *
 * Provides dual-mode authentication:
 * 1. Claude Max subscription via OAuth PKCE flow
 * 2. Traditional API key authentication
 *
 * OAuth flow follows the same implementation as Claude Code CLI:
 * - PKCE (Proof Key for Code Exchange) for security
 * - Local HTTP server for OAuth callback
 * - macOS Keychain for secure token storage
 * - Automatic token refresh
 *
 * Roberto D'Angelo - 2025
 */

#ifndef CONVERGIO_OAUTH_H
#define CONVERGIO_OAUTH_H

#include <stdbool.h>
#include <time.h>

// ============================================================================
// AUTHENTICATION MODES
// ============================================================================

/**
 * Authentication mode enumeration
 *
 * AUTH_MODE_NONE    - No valid authentication found
 * AUTH_MODE_API_KEY - Using ANTHROPIC_API_KEY environment variable
 * AUTH_MODE_OAUTH   - Using Claude Max OAuth tokens
 */
typedef enum { AUTH_MODE_NONE = 0, AUTH_MODE_API_KEY, AUTH_MODE_OAUTH } AuthMode;

// ============================================================================
// TOKEN STRUCTURES
// ============================================================================

/**
 * OAuth token storage structure
 *
 * access_token  - Short-lived token for API calls (format: sk-ant-oat01-...)
 * refresh_token - Long-lived token for obtaining new access tokens (format: sk-ant-ort01-...)
 * expires_at    - Unix timestamp when access token expires (typically 8 hours)
 */
typedef struct {
    char* access_token;
    char* refresh_token;
    time_t expires_at;
} OAuthTokens;

// ============================================================================
// INITIALIZATION & LIFECYCLE
// ============================================================================

/**
 * Initialize authentication system
 *
 * Attempts to authenticate in this order:
 * 1. Load existing OAuth tokens from Keychain
 * 2. Check for ANTHROPIC_API_KEY environment variable
 *
 * Returns:
 *   0  - Success (authenticated)
 *   -1 - No valid authentication found
 */
int auth_init(void);

/**
 * Shutdown authentication system
 * Frees allocated resources
 */
void auth_shutdown(void);

// ============================================================================
// AUTHENTICATION STATUS
// ============================================================================

/**
 * Get current authentication mode
 *
 * Returns: Current AuthMode (NONE, API_KEY, or OAUTH)
 */
AuthMode auth_get_mode(void);

/**
 * Check if system is currently authenticated
 *
 * For OAuth mode, also verifies token hasn't expired
 *
 * Returns: true if authenticated, false otherwise
 */
bool auth_is_authenticated(void);

/**
 * Get authorization header value for API calls
 *
 * Returns:
 *   - For API key mode: The API key string
 *   - For OAuth mode: The access token (refreshed if needed)
 *   - NULL if not authenticated
 *
 * Note: Caller must free() the returned string
 */
char* auth_get_header(void);

/**
 * Get human-readable authentication status
 *
 * Returns string like:
 *   - "API Key (sk-ant-...abc)"
 *   - "Claude Max OAuth (expires in 7h 23m)"
 *   - "Not authenticated"
 *
 * Note: Caller must free() the returned string
 */
char* auth_get_status_string(void);

// ============================================================================
// OAUTH OPERATIONS
// ============================================================================

/**
 * Start OAuth login flow for Claude Max
 *
 * This will:
 * 1. Generate PKCE code verifier and challenge
 * 2. Start local HTTP server for callback
 * 3. Open browser to Claude login page
 * 4. Wait for user to authorize
 * 5. Exchange authorization code for tokens
 * 6. Store tokens in macOS Keychain
 *
 * Returns:
 *   0  - Success
 *   -1 - Error (message printed to stderr)
 */
int auth_oauth_login(void);

/**
 * Refresh OAuth access token using refresh token
 *
 * Called automatically when token is expired
 *
 * Returns:
 *   0  - Success
 *   -1 - Error (refresh token may be invalid)
 */
int auth_oauth_refresh(void);

/**
 * Logout - clear stored credentials
 *
 * Removes OAuth tokens from Keychain
 *
 * Returns:
 *   0  - Success
 *   -1 - Error
 */
int auth_logout(void);

// ============================================================================
// CONSTANTS
// ============================================================================

/**
 * OAuth configuration constants
 * Client ID is the same as Claude Code CLI (public, not secret)
 */
#define OAUTH_CLIENT_ID "9d1c250a-e61b-44d9-88ed-5944d1962f5e"
#define OAUTH_AUTHORIZE_URL "https://claude.ai/oauth/authorize"
#define OAUTH_TOKEN_URL "https://claude.ai/api/oauth/token"
#define OAUTH_REFRESH_URL "https://console.anthropic.com/api/oauth/token"
#define OAUTH_REDIRECT_PATH "/oauth/callback"
#define OAUTH_SCOPES "user:inference org:inference"

/**
 * Keychain service name for storing OAuth tokens
 */
#define KEYCHAIN_SERVICE "com.convergio.oauth"
#define KEYCHAIN_ACCOUNT_ACCESS "access_token"
#define KEYCHAIN_ACCOUNT_REFRESH "refresh_token"
#define KEYCHAIN_ACCOUNT_EXPIRES "expires_at"

#endif // CONVERGIO_OAUTH_H
