/**
 * CONVERGIO CURL HELPERS
 *
 * Centralized CURL utilities:
 * - Header construction
 * - Authentication handling
 * - Common CURL options setup
 */

#ifndef CONVERGIO_CURL_HELPERS_H
#define CONVERGIO_CURL_HELPERS_H

#include <curl/curl.h>
#include <stdbool.h>

// Anthropic API version header
#define ANTHROPIC_API_VERSION "2023-06-01"

/**
 * Build standard headers for Claude API requests.
 *
 * This function creates a curl_slist with:
 * - Content-Type: application/json
 * - x-api-key or Authorization (based on auth mode)
 * - anthropic-version header
 *
 * @return curl_slist* that must be freed with curl_slist_free_all(),
 *         or NULL on error (including authentication failure)
 */
struct curl_slist* claude_build_headers(void);

/**
 * Free headers built by claude_build_headers.
 * Safe to call with NULL.
 */
void claude_free_headers(struct curl_slist* headers);

/**
 * Setup common CURL options for Claude API calls.
 *
 * Sets:
 * - CURLOPT_HTTPHEADER (from headers param)
 * - CURLOPT_TIMEOUT (60 seconds)
 * - CURLOPT_NOPROGRESS (0 to enable progress)
 * - CURLOPT_XFERINFOFUNCTION (for cancellation)
 *
 * @param curl    CURL handle
 * @param headers Headers built by claude_build_headers
 * @return        true on success
 */
bool claude_setup_common_opts(CURL* curl, struct curl_slist* headers);

/**
 * Handle CURL result and HTTP response code.
 *
 * @param curl       CURL handle
 * @param res        CURL result code
 * @param response   Response body for error messages (can be NULL)
 * @return           true if request succeeded (HTTP 200), false otherwise
 */
bool claude_handle_result(CURL* curl, CURLcode res, const char* response);

/**
 * Initialize a response buffer for CURL writes.
 *
 * @param initial_size  Initial buffer size (0 for default 4096)
 * @return              Allocated buffer or NULL on error
 */
char* claude_response_buffer_init(size_t initial_size);

#endif // CONVERGIO_CURL_HELPERS_H
