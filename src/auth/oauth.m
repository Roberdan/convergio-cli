/**
 * OAuth Authentication Implementation for Convergio CLI
 *
 * Implements dual-mode authentication:
 * 1. Claude Max subscription via OAuth PKCE flow
 * 2. Traditional API key authentication
 *
 * Security features:
 * - PKCE (Proof Key for Code Exchange) prevents authorization code interception
 * - macOS Keychain for encrypted token storage
 * - Automatic token refresh before expiry
 * - Local-only callback server (binds to 127.0.0.1)
 *
 * Roberto D'Angelo - 2025
 */

#include "oauth.h"
#include "nous/nous.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <CommonCrypto/CommonDigest.h>
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#import <AppKit/AppKit.h>

// ============================================================================
// GLOBAL STATE
// ============================================================================

static AuthMode g_auth_mode = AUTH_MODE_NONE;
static char* g_api_key = NULL;
static OAuthTokens g_tokens = {0};
static bool g_initialized = false;
static char* g_oauth_state = NULL;  // CSRF protection state parameter

// ============================================================================
// SECURITY UTILITIES
// ============================================================================

/**
 * Secure memory wipe - prevents compiler optimization from removing the zeroing
 * Uses volatile to ensure the write is not optimized away
 */
static void secure_zero(void* ptr, size_t len) {
    if (!ptr || len == 0) return;
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    while (len--) {
        *p++ = 0;
    }
}

/**
 * Secure string free - wipes memory before freeing
 */
static void secure_free(char** ptr) {
    if (!ptr || !*ptr) return;
    size_t len = strlen(*ptr);
    secure_zero(*ptr, len);
    free(*ptr);
    *ptr = NULL;
}

/**
 * Constant-time string comparison (prevents timing attacks)
 * Compares exactly 'len' bytes, returns 0 if equal
 */
static int secure_memcmp(const void* a, const void* b, size_t len) {
    const volatile unsigned char* pa = (const volatile unsigned char*)a;
    const volatile unsigned char* pb = (const volatile unsigned char*)b;
    unsigned char result = 0;
    for (size_t i = 0; i < len; i++) {
        result |= pa[i] ^ pb[i];
    }
    return result;
}

/**
 * Validate token format (constant-time to prevent timing attacks)
 * Access tokens start with sk-ant-oat01-
 * Refresh tokens start with sk-ant-ort01-
 */
static bool validate_token_format(const char* token, bool is_refresh) {
    if (!token) return false;
    const char* expected_prefix = is_refresh ? "sk-ant-ort01-" : "sk-ant-oat01-";
    size_t prefix_len = strlen(expected_prefix);
    size_t token_len = strlen(token);
    if (token_len < prefix_len + 10) return false;  // Minimum reasonable length
    // Use constant-time comparison for prefix
    return secure_memcmp(token, expected_prefix, prefix_len) == 0;
}

/**
 * Generate random state parameter for CSRF protection
 */
static char* generate_state_param(void) {
    unsigned char random_bytes[16];
    FILE* f = fopen("/dev/urandom", "rb");
    if (!f) return NULL;
    size_t read = fread(random_bytes, 1, sizeof(random_bytes), f);
    fclose(f);
    if (read != sizeof(random_bytes)) return NULL;

    // Convert to hex string (16 bytes -> 32 hex chars + null)
    char* state = malloc(33);
    if (!state) return NULL;
    for (int i = 0; i < 16; i++) {
        snprintf(state + i * 2, 3, "%02x", random_bytes[i]);
    }
    state[32] = '\0';
    return state;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Base64 URL-safe encoding (RFC 4648)
 * Used for PKCE code challenge
 */
static char* base64url_encode(const unsigned char* data, size_t len) {
    static const char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    size_t out_len = ((len + 2) / 3) * 4;
    char* result = malloc(out_len + 1);
    if (!result) return NULL;

    size_t i, j;
    for (i = 0, j = 0; i < len; i += 3) {
        unsigned int octet_a = data[i];
        unsigned int octet_b = (i + 1 < len) ? data[i + 1] : 0;
        unsigned int octet_c = (i + 2 < len) ? data[i + 2] : 0;

        unsigned int triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        result[j++] = base64_chars[(triple >> 18) & 0x3F];
        result[j++] = base64_chars[(triple >> 12) & 0x3F];
        result[j++] = (i + 1 < len) ? base64_chars[(triple >> 6) & 0x3F] : '\0';
        result[j++] = (i + 2 < len) ? base64_chars[triple & 0x3F] : '\0';
    }

    // Remove padding (URL-safe base64 doesn't use padding)
    while (j > 0 && result[j - 1] == '\0') j--;
    result[j] = '\0';

    return result;
}

/**
 * Generate cryptographically secure random bytes
 */
static int secure_random(unsigned char* buf, size_t len) {
    FILE* f = fopen("/dev/urandom", "rb");
    if (!f) return -1;
    size_t read = fread(buf, 1, len, f);
    fclose(f);
    return (read == len) ? 0 : -1;
}

/**
 * URL encode a string
 */
static char* url_encode(const char* str) {
    CURL* curl = curl_easy_init();
    if (!curl) return NULL;
    char* encoded = curl_easy_escape(curl, str, 0);
    char* result = encoded ? strdup(encoded) : NULL;
    curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
}

// ============================================================================
// PKCE IMPLEMENTATION
// ============================================================================

/**
 * Generate PKCE code verifier
 * RFC 7636: 43-128 characters from unreserved URI characters
 */
static char* generate_code_verifier(void) {
    unsigned char random_bytes[32];
    if (secure_random(random_bytes, sizeof(random_bytes)) != 0) {
        return NULL;
    }
    return base64url_encode(random_bytes, sizeof(random_bytes));
}

/**
 * Generate PKCE code challenge from verifier
 * SHA256 hash, base64url encoded
 */
static char* generate_code_challenge(const char* verifier) {
    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(verifier, (CC_LONG)strlen(verifier), hash);
    return base64url_encode(hash, CC_SHA256_DIGEST_LENGTH);
}

// ============================================================================
// KEYCHAIN OPERATIONS (macOS)
// ============================================================================

/**
 * Store a string value in Keychain
 */
static int keychain_store(const char* account, const char* value) {
    if (!value) return -1;

    // Delete existing item first
    NSDictionary* query = @{
        (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrService: @KEYCHAIN_SERVICE,
        (__bridge id)kSecAttrAccount: [NSString stringWithUTF8String:account]
    };
    SecItemDelete((__bridge CFDictionaryRef)query);

    // Add new item
    NSData* data = [[NSString stringWithUTF8String:value]
                    dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary* attributes = @{
        (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrService: @KEYCHAIN_SERVICE,
        (__bridge id)kSecAttrAccount: [NSString stringWithUTF8String:account],
        (__bridge id)kSecValueData: data,
        (__bridge id)kSecAttrAccessible: (__bridge id)kSecAttrAccessibleWhenUnlocked
    };

    OSStatus status = SecItemAdd((__bridge CFDictionaryRef)attributes, NULL);
    return (status == errSecSuccess) ? 0 : -1;
}

/**
 * Load a string value from Keychain
 * Caller must free() returned string
 */
static char* keychain_load(const char* account) {
    NSDictionary* query = @{
        (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrService: @KEYCHAIN_SERVICE,
        (__bridge id)kSecAttrAccount: [NSString stringWithUTF8String:account],
        (__bridge id)kSecReturnData: @YES,
        (__bridge id)kSecMatchLimit: (__bridge id)kSecMatchLimitOne
    };

    CFDataRef data = NULL;
    OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query,
                                          (CFTypeRef*)&data);

    if (status != errSecSuccess || !data) {
        return NULL;
    }

    // CFBridgingRelease transfers ownership from CF to ARC (same as __bridge_transfer)
    NSData* nsdata = CFBridgingRelease(data);
    NSString* str = [[NSString alloc] initWithData:nsdata
                                          encoding:NSUTF8StringEncoding];
    return str ? strdup([str UTF8String]) : NULL;
}

/**
 * Delete a value from Keychain
 */
static int keychain_delete(const char* account) {
    NSDictionary* query = @{
        (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrService: @KEYCHAIN_SERVICE,
        (__bridge id)kSecAttrAccount: [NSString stringWithUTF8String:account]
    };
    SecItemDelete((__bridge CFDictionaryRef)query);
    return 0;
}

/**
 * Store all OAuth tokens to Keychain
 */
static int keychain_store_tokens(const OAuthTokens* tokens) {
    if (!tokens || !tokens->access_token || !tokens->refresh_token) {
        return -1;
    }

    if (keychain_store(KEYCHAIN_ACCOUNT_ACCESS, tokens->access_token) != 0) {
        return -1;
    }
    if (keychain_store(KEYCHAIN_ACCOUNT_REFRESH, tokens->refresh_token) != 0) {
        return -1;
    }

    // Store expiry as string
    char expires_str[32];
    snprintf(expires_str, sizeof(expires_str), "%ld", tokens->expires_at);
    if (keychain_store(KEYCHAIN_ACCOUNT_EXPIRES, expires_str) != 0) {
        return -1;
    }

    return 0;
}

/**
 * Load OAuth tokens from Keychain
 */
static int keychain_load_tokens(OAuthTokens* tokens) {
    if (!tokens) return -1;

    memset(tokens, 0, sizeof(*tokens));

    tokens->access_token = keychain_load(KEYCHAIN_ACCOUNT_ACCESS);
    tokens->refresh_token = keychain_load(KEYCHAIN_ACCOUNT_REFRESH);

    char* expires_str = keychain_load(KEYCHAIN_ACCOUNT_EXPIRES);
    if (expires_str) {
        tokens->expires_at = atol(expires_str);
        free(expires_str);
    }

    // Must have at least refresh token
    return (tokens->refresh_token != NULL) ? 0 : -1;
}

/**
 * Delete all OAuth tokens from Keychain
 */
static int keychain_delete_tokens(void) {
    keychain_delete(KEYCHAIN_ACCOUNT_ACCESS);
    keychain_delete(KEYCHAIN_ACCOUNT_REFRESH);
    keychain_delete(KEYCHAIN_ACCOUNT_EXPIRES);
    return 0;
}

// ============================================================================
// HTTP CALLBACK SERVER
// ============================================================================

/**
 * Start local HTTP server for OAuth callback
 * Returns server socket fd, writes port to *port
 */
static int start_callback_server(int* port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return -1;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = inet_addr("127.0.0.1"),  // localhost only
        .sin_port = 0  // Let OS assign port
    };

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server_fd);
        return -1;
    }

    // Get assigned port
    socklen_t addr_len = sizeof(addr);
    getsockname(server_fd, (struct sockaddr*)&addr, &addr_len);
    *port = ntohs(addr.sin_port);

    listen(server_fd, 1);
    return server_fd;
}

/**
 * Wait for OAuth callback and extract authorization code
 * Returns authorization code (caller must free) or NULL on error
 */
static char* wait_for_oauth_code(int server_fd) {
    // Set timeout
    struct timeval tv = {.tv_sec = 120, .tv_usec = 0};  // 2 minute timeout
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        return NULL;
    }

    // Read HTTP request
    char buffer[4096] = {0};
    ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(client_fd);
        return NULL;
    }

    // Extract authorization code from query string
    // Request looks like: GET /oauth/callback?code=xxx&state=yyy HTTP/1.1
    char* code = NULL;
    char* state = NULL;
    bool csrf_valid = false;

    // Extract code parameter
    char* code_start = strstr(buffer, "code=");
    if (code_start) {
        code_start += 5;
        char* code_end = code_start;
        while (*code_end && *code_end != '&' && *code_end != ' ' && *code_end != '\r') {
            code_end++;
        }
        size_t code_len = (size_t)(code_end - code_start);
        // Limit code length for security
        if (code_len > 0 && code_len < 512) {
            code = malloc(code_len + 1);
            if (code) {
                strncpy(code, code_start, code_len);
                code[code_len] = '\0';
            }
        }
    }

    // Extract and validate state parameter (CSRF protection)
    char* state_start = strstr(buffer, "state=");
    if (state_start) {
        state_start += 6;
        char* state_end = state_start;
        while (*state_end && *state_end != '&' && *state_end != ' ' && *state_end != '\r') {
            state_end++;
        }
        size_t state_len = (size_t)(state_end - state_start);
        if (state_len == 32 && g_oauth_state && strlen(g_oauth_state) == 32) {
            state = malloc(state_len + 1);
            if (state) {
                strncpy(state, state_start, state_len);
                state[state_len] = '\0';
                // Constant-time comparison to prevent timing attacks
                csrf_valid = (secure_memcmp(state, g_oauth_state, 32) == 0);
                secure_free(&state);
            }
        }
    }

    // CSRF validation failed - reject the request
    if (!csrf_valid) {
        LOG_ERROR(LOG_CAT_API, "CSRF validation failed - state parameter mismatch");
        secure_free(&code);
        code = NULL;
    }

    // Send response to browser
    const char* response = code ?
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 247\r\n"
        "Connection: close\r\n\r\n"
        "<!DOCTYPE html><html><head><title>Convergio CLI</title></head>"
        "<body style=\"font-family:system-ui;text-align:center;padding:50px\">"
        "<h1>Authentication Successful!</h1>"
        "<p>You can close this window and return to Convergio CLI.</p>"
        "</body></html>"
        :
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 227\r\n"
        "Connection: close\r\n\r\n"
        "<!DOCTYPE html><html><head><title>Error</title></head>"
        "<body style=\"font-family:system-ui;text-align:center;padding:50px\">"
        "<h1>Authentication Failed</h1>"
        "<p>Authorization failed. Please try again.</p>"
        "</body></html>";

    send(client_fd, response, strlen(response), 0);
    close(client_fd);

    return code;
}

// ============================================================================
// TOKEN EXCHANGE
// ============================================================================

/**
 * HTTP response buffer for curl
 */
typedef struct {
    char* data;
    size_t size;
} CurlBuffer;

// Maximum response size for OAuth tokens (64KB is plenty for token responses)
#define MAX_OAUTH_RESPONSE_SIZE (64 * 1024)

static size_t oauth_curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    CurlBuffer* buf = (CurlBuffer*)userp;

    // Check response size limit to prevent OOM
    if (buf->size + total > MAX_OAUTH_RESPONSE_SIZE) {
        return 0;  // Abort transfer
    }

    char* new_data = realloc(buf->data, buf->size + total + 1);
    if (!new_data) {
        // Securely wipe and free old buffer on realloc failure
        if (buf->data) {
            secure_zero(buf->data, buf->size);
            free(buf->data);
            buf->data = NULL;
            buf->size = 0;
        }
        return 0;
    }
    buf->data = new_data;
    memcpy(buf->data + buf->size, contents, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

/**
 * Extract string value from JSON (simple parser)
 */
static char* json_extract_string(const char* json, const char* key) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char* found = strstr(json, search);
    if (!found) return NULL;

    found += strlen(search);
    while (*found && (*found == ' ' || *found == '\t')) found++;
    if (*found != '"') return NULL;
    found++;

    const char* end = found;
    while (*end && *end != '"') {
        if (*end == '\\' && *(end + 1)) end++;
        end++;
    }

    size_t len = (size_t)(end - found);
    char* result = malloc(len + 1);
    if (!result) return NULL;
    strncpy(result, found, len);
    result[len] = '\0';
    return result;
}

/**
 * Extract integer value from JSON
 */
static long json_extract_int(const char* json, const char* key) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char* found = strstr(json, search);
    if (!found) return 0;

    found += strlen(search);
    while (*found && (*found == ' ' || *found == '\t')) found++;
    return atol(found);
}

/**
 * Exchange authorization code for tokens
 */
static int exchange_code_for_tokens(const char* code, const char* verifier, int port) {
    CURL* curl = curl_easy_init();
    if (!curl) return -1;

    // Build redirect URI
    char redirect_uri[256];
    snprintf(redirect_uri, sizeof(redirect_uri),
             "http://localhost:%d%s", port, OAUTH_REDIRECT_PATH);

    // Build POST data
    char* encoded_code = url_encode(code);
    char* encoded_verifier = url_encode(verifier);
    char* encoded_redirect = url_encode(redirect_uri);

    if (!encoded_code || !encoded_verifier || !encoded_redirect) {
        free(encoded_code);
        free(encoded_verifier);
        free(encoded_redirect);
        curl_easy_cleanup(curl);
        return -1;
    }

    char post_data[2048];
    snprintf(post_data, sizeof(post_data),
             "grant_type=authorization_code"
             "&code=%s"
             "&redirect_uri=%s"
             "&client_id=%s"
             "&code_verifier=%s",
             encoded_code, encoded_redirect, OAUTH_CLIENT_ID, encoded_verifier);

    free(encoded_code);
    free(encoded_verifier);
    free(encoded_redirect);

    // Setup request
    CurlBuffer response = {0};
    curl_easy_setopt(curl, CURLOPT_URL, OAUTH_TOKEN_URL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, oauth_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code != 200) {
        LOG_ERROR(LOG_CAT_API, "Token exchange failed: HTTP %ld", http_code);
        // Note: Don't log response.data - may contain sensitive information
        if (response.data) {
            secure_zero(response.data, strlen(response.data));
            free(response.data);
        }
        return -1;
    }

    // Parse response
    char* access_token = json_extract_string(response.data, "access_token");
    char* refresh_token = json_extract_string(response.data, "refresh_token");
    long expires_in = json_extract_int(response.data, "expires_in");

    // Securely clear response data (contains tokens)
    secure_zero(response.data, strlen(response.data));
    free(response.data);

    // Validate token formats
    if (!access_token || !refresh_token) {
        LOG_ERROR(LOG_CAT_API, "Invalid token response: missing tokens");
        secure_free(&access_token);
        secure_free(&refresh_token);
        return -1;
    }

    if (!validate_token_format(access_token, false) || !validate_token_format(refresh_token, true)) {
        LOG_ERROR(LOG_CAT_API, "Invalid token response: unexpected token format");
        secure_free(&access_token);
        secure_free(&refresh_token);
        return -1;
    }

    // Store tokens (securely free old ones first)
    secure_free(&g_tokens.access_token);
    secure_free(&g_tokens.refresh_token);
    g_tokens.access_token = access_token;
    g_tokens.refresh_token = refresh_token;
    g_tokens.expires_at = time(NULL) + (expires_in > 0 ? expires_in : 28800);  // Default 8 hours

    // Save to Keychain
    if (keychain_store_tokens(&g_tokens) != 0) {
        LOG_WARN(LOG_CAT_API, "Failed to save tokens to Keychain");
    }

    return 0;
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

int auth_init(void) {
    if (g_initialized) return 0;

    // Try loading OAuth tokens from Keychain first
    if (keychain_load_tokens(&g_tokens) == 0 && g_tokens.access_token) {
        // Check if token is still valid (with 5 min buffer)
        if (g_tokens.expires_at > time(NULL) + 300) {
            g_auth_mode = AUTH_MODE_OAUTH;
            g_initialized = true;
            return 0;
        }

        // Token expired, try refresh
        if (g_tokens.refresh_token && auth_oauth_refresh() == 0) {
            g_auth_mode = AUTH_MODE_OAUTH;
            g_initialized = true;
            return 0;
        }
    }

    // Try API key from environment
    const char* api_key = getenv("ANTHROPIC_API_KEY");
    if (api_key && strlen(api_key) > 0) {
        g_api_key = strdup(api_key);
        g_auth_mode = AUTH_MODE_API_KEY;
        g_initialized = true;
        return 0;
    }

    // No valid authentication found
    g_auth_mode = AUTH_MODE_NONE;
    g_initialized = true;
    return -1;
}

void auth_shutdown(void) {
    if (!g_initialized) return;

    // Securely wipe all sensitive data
    secure_free(&g_api_key);
    secure_free(&g_tokens.access_token);
    secure_free(&g_tokens.refresh_token);
    secure_free(&g_oauth_state);
    secure_zero(&g_tokens, sizeof(g_tokens));

    g_auth_mode = AUTH_MODE_NONE;
    g_initialized = false;
}

AuthMode auth_get_mode(void) {
    return g_auth_mode;
}

bool auth_is_authenticated(void) {
    if (!g_initialized) return false;

    switch (g_auth_mode) {
        case AUTH_MODE_API_KEY:
            return g_api_key != NULL;
        case AUTH_MODE_OAUTH:
            // Check expiry with 5 min buffer
            return g_tokens.access_token != NULL &&
                   g_tokens.expires_at > time(NULL) + 300;
        default:
            return false;
    }
}

char* auth_get_header(void) {
    if (!g_initialized) return NULL;

    switch (g_auth_mode) {
        case AUTH_MODE_API_KEY:
            return g_api_key ? strdup(g_api_key) : NULL;

        case AUTH_MODE_OAUTH:
            // Refresh if needed (with 5 min buffer)
            if (g_tokens.expires_at <= time(NULL) + 300) {
                if (auth_oauth_refresh() != 0) {
                    return NULL;
                }
            }
            return g_tokens.access_token ? strdup(g_tokens.access_token) : NULL;

        default:
            return NULL;
    }
}

char* auth_get_status_string(void) {
    char* result = malloc(256);
    if (!result) return NULL;

    switch (g_auth_mode) {
        case AUTH_MODE_API_KEY: {
            if (g_api_key && strlen(g_api_key) > 10) {
                // Show masked key
                snprintf(result, 256, "API Key (%.*s...%s)",
                         7, g_api_key, g_api_key + strlen(g_api_key) - 3);
            } else {
                snprintf(result, 256, "API Key (configured)");
            }
            break;
        }

        case AUTH_MODE_OAUTH: {
            time_t remaining = g_tokens.expires_at - time(NULL);
            if (remaining > 3600) {
                snprintf(result, 256, "Claude Max OAuth (expires in %ldh %ldm)",
                         remaining / 3600, (remaining % 3600) / 60);
            } else if (remaining > 0) {
                snprintf(result, 256, "Claude Max OAuth (expires in %ldm)",
                         remaining / 60);
            } else {
                snprintf(result, 256, "Claude Max OAuth (expired, refresh needed)");
            }
            break;
        }

        default:
            snprintf(result, 256, "Not authenticated");
    }

    return result;
}

int auth_oauth_login(void) {
    // Generate PKCE
    char* verifier = generate_code_verifier();
    char* challenge = verifier ? generate_code_challenge(verifier) : NULL;

    if (!verifier || !challenge) {
        LOG_ERROR(LOG_CAT_API, "Failed to generate PKCE");
        secure_free(&verifier);
        secure_free(&challenge);
        return -1;
    }

    // Generate CSRF state parameter
    secure_free(&g_oauth_state);
    g_oauth_state = generate_state_param();
    if (!g_oauth_state) {
        LOG_ERROR(LOG_CAT_API, "Failed to generate state parameter");
        secure_free(&verifier);
        secure_free(&challenge);
        return -1;
    }

    // Start callback server
    int port;
    int server_fd = start_callback_server(&port);
    if (server_fd < 0) {
        LOG_ERROR(LOG_CAT_API, "Failed to start callback server");
        secure_free(&verifier);
        secure_free(&challenge);
        return -1;
    }

    // Build authorization URL with state parameter for CSRF protection
    char auth_url[2048];
    snprintf(auth_url, sizeof(auth_url),
             "%s?client_id=%s"
             "&response_type=code"
             "&redirect_uri=http://localhost:%d%s"
             "&code_challenge=%s"
             "&code_challenge_method=S256"
             "&scope=%s"
             "&state=%s",
             OAUTH_AUTHORIZE_URL,
             OAUTH_CLIENT_ID,
             port,
             OAUTH_REDIRECT_PATH,
             challenge,
             "user:inference+org:inference",  // URL encoded scope (+ = space in query strings)
             g_oauth_state);

    secure_free(&challenge);

    // Open browser using NSWorkspace (safer than system())
    printf("Opening browser for Claude Max login...\n");
    printf("If browser doesn't open, visit:\n%s\n\n", auth_url);

    NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String:auth_url]];
    [[NSWorkspace sharedWorkspace] openURL:url];

    // Wait for callback
    printf("Waiting for authorization (timeout: 2 minutes)...\n");
    char* code = wait_for_oauth_code(server_fd);
    close(server_fd);

    if (!code) {
        LOG_ERROR(LOG_CAT_API, "Authorization failed or timed out");
        secure_free(&verifier);
        return -1;
    }

    // Exchange code for tokens
    printf("Exchanging authorization code for tokens...\n");
    int result = exchange_code_for_tokens(code, verifier, port);
    secure_free(&code);
    secure_free(&verifier);

    if (result == 0) {
        g_auth_mode = AUTH_MODE_OAUTH;
        printf("Login successful!\n");
    }

    return result;
}

int auth_oauth_refresh(void) {
    if (!g_tokens.refresh_token) {
        return -1;
    }

    CURL* curl = curl_easy_init();
    if (!curl) return -1;

    // Build POST data (contains sensitive refresh token)
    char post_data[2048];
    snprintf(post_data, sizeof(post_data),
             "grant_type=refresh_token"
             "&refresh_token=%s"
             "&client_id=%s",
             g_tokens.refresh_token, OAUTH_CLIENT_ID);

    // Setup request
    CurlBuffer response = {0};
    curl_easy_setopt(curl, CURLOPT_URL, OAUTH_REFRESH_URL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, oauth_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    // Securely wipe post_data immediately after use (contains refresh token)
    secure_zero(post_data, sizeof(post_data));

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code != 200) {
        // Securely wipe response (may contain error details)
        if (response.data) {
            secure_zero(response.data, response.size);
            free(response.data);
        }
        return -1;
    }

    // Parse response
    char* access_token = json_extract_string(response.data, "access_token");
    char* refresh_token = json_extract_string(response.data, "refresh_token");
    long expires_in = json_extract_int(response.data, "expires_in");

    // Securely wipe response data (contains tokens)
    secure_zero(response.data, response.size);
    free(response.data);

    // Validate token formats
    if (!access_token) {
        secure_free(&refresh_token);
        return -1;
    }

    if (!validate_token_format(access_token, false)) {
        secure_free(&access_token);
        secure_free(&refresh_token);
        return -1;
    }

    if (refresh_token && !validate_token_format(refresh_token, true)) {
        secure_free(&access_token);
        secure_free(&refresh_token);
        return -1;
    }

    // Update tokens (securely free old ones first)
    secure_free(&g_tokens.access_token);
    g_tokens.access_token = access_token;

    if (refresh_token) {
        secure_free(&g_tokens.refresh_token);
        g_tokens.refresh_token = refresh_token;
    }

    g_tokens.expires_at = time(NULL) + (expires_in > 0 ? expires_in : 28800);

    // Save to Keychain
    keychain_store_tokens(&g_tokens);

    return 0;
}

int auth_logout(void) {
    // Securely clear in-memory tokens
    secure_free(&g_tokens.access_token);
    secure_free(&g_tokens.refresh_token);
    secure_zero(&g_tokens, sizeof(g_tokens));

    // Clear state parameter
    secure_free(&g_oauth_state);

    // Clear Keychain
    keychain_delete_tokens();

    // Securely clear API key if set
    secure_free(&g_api_key);

    // Reset mode
    g_auth_mode = AUTH_MODE_NONE;

    // Check if API key is available (reload from env)
    const char* api_key = getenv("ANTHROPIC_API_KEY");
    if (api_key && strlen(api_key) > 0) {
        g_api_key = strdup(api_key);
        g_auth_mode = AUTH_MODE_API_KEY;
    }

    return 0;
}
