/**
 * CONVERGIO PROVIDER COMMON UTILITIES
 *
 * Common types and functions shared across all HTTP-based providers.
 * This eliminates code duplication between anthropic.c, openai.c, gemini.c,
 * openrouter.c, and ollama.c.
 *
 * Usage:
 *   #include "nous/provider_common.h"
 *
 *   // Initialize a response buffer
 *   ResponseBuffer buf = RESPONSE_BUFFER_INIT;
 *
 *   // Use with curl
 *   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, provider_write_callback);
 *   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
 *
 *   // After use, clean up
 *   response_buffer_free(&buf);
 */

#ifndef NOUS_PROVIDER_COMMON_H
#define NOUS_PROVIDER_COMMON_H

#include <stddef.h>
#include <stdbool.h>
#include <curl/curl.h>

// ============================================================================
// RESPONSE BUFFER - Dynamic string buffer for HTTP responses
// ============================================================================

/**
 * ResponseBuffer - A growable buffer for accumulating HTTP response data.
 *
 * Thread safety: NOT thread-safe. Each request should use its own buffer.
 * Memory: Call response_buffer_free() when done to avoid leaks.
 */
typedef struct {
    char* data;       // Null-terminated string data
    size_t size;      // Current length (excluding null terminator)
    size_t capacity;  // Allocated capacity
} ResponseBuffer;

// Initializer for stack-allocated ResponseBuffer
#define RESPONSE_BUFFER_INIT { NULL, 0, 0 }

// Default initial capacity (4KB is good for most API responses)
#define RESPONSE_BUFFER_DEFAULT_CAPACITY 4096

// Maximum response size (16MB - prevents OOM from malicious servers)
#define RESPONSE_BUFFER_MAX_SIZE (16 * 1024 * 1024)

/**
 * Initialize a ResponseBuffer with default capacity.
 *
 * @param buf  Pointer to buffer to initialize
 * @return     true on success, false on allocation failure
 */
bool response_buffer_init(ResponseBuffer* buf);

/**
 * Initialize a ResponseBuffer with specified capacity.
 *
 * @param buf       Pointer to buffer to initialize
 * @param capacity  Initial capacity in bytes
 * @return          true on success, false on allocation failure
 */
bool response_buffer_init_with_capacity(ResponseBuffer* buf, size_t capacity);

/**
 * Free a ResponseBuffer's internal data.
 * The buffer struct itself is not freed (assumes stack allocation).
 * Safe to call multiple times.
 *
 * @param buf  Pointer to buffer to free
 */
void response_buffer_free(ResponseBuffer* buf);

/**
 * Clear a ResponseBuffer's contents without freeing memory.
 * Useful for reusing a buffer across multiple requests.
 *
 * @param buf  Pointer to buffer to clear
 */
void response_buffer_clear(ResponseBuffer* buf);

/**
 * Append data to a ResponseBuffer, growing if necessary.
 *
 * @param buf   Pointer to buffer
 * @param data  Data to append
 * @param len   Length of data
 * @return      true on success, false on allocation failure
 */
bool response_buffer_append(ResponseBuffer* buf, const char* data, size_t len);

// ============================================================================
// CURL CALLBACKS - Common callbacks for curl operations
// ============================================================================

/**
 * Standard write callback for curl that appends to a ResponseBuffer.
 *
 * Usage:
 *   ResponseBuffer buf = RESPONSE_BUFFER_INIT;
 *   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, provider_write_callback);
 *   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
 *
 * @param contents  Data received from server
 * @param size      Size of each element (always 1)
 * @param nmemb     Number of elements
 * @param userp     Pointer to ResponseBuffer
 * @return          Number of bytes handled (0 on error)
 */
size_t provider_write_callback(void* contents, size_t size, size_t nmemb, void* userp);

// ============================================================================
// HTTP UTILITIES - Common HTTP request helpers
// ============================================================================

/**
 * Set common curl options for API requests.
 * Sets timeout, SSL verification, user agent, etc.
 *
 * @param curl     CURL handle
 * @param timeout  Timeout in seconds (0 = no timeout)
 */
void provider_set_common_curl_opts(CURL* curl, long timeout);

/**
 * Create a standard JSON Content-Type header list.
 * Caller must free with curl_slist_free_all().
 *
 * @return  Newly allocated slist with "Content-Type: application/json"
 */
struct curl_slist* provider_json_headers(void);

/**
 * Add an Authorization Bearer header to a header list.
 *
 * @param headers  Existing header list (can be NULL)
 * @param token    Bearer token
 * @return         Updated header list
 */
struct curl_slist* provider_add_auth_header(struct curl_slist* headers, const char* token);

#endif // NOUS_PROVIDER_COMMON_H
