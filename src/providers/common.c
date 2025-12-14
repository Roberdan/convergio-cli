/**
 * CONVERGIO PROVIDER COMMON UTILITIES
 *
 * Implementation of common types and functions shared across providers.
 * See include/nous/provider_common.h for API documentation.
 */

#include "nous/provider_common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// RESPONSE BUFFER IMPLEMENTATION
// ============================================================================

bool response_buffer_init(ResponseBuffer* buf) {
    return response_buffer_init_with_capacity(buf, RESPONSE_BUFFER_DEFAULT_CAPACITY);
}

bool response_buffer_init_with_capacity(ResponseBuffer* buf, size_t capacity) {
    if (!buf) return false;

    buf->data = malloc(capacity);
    if (!buf->data) {
        buf->size = 0;
        buf->capacity = 0;
        return false;
    }

    buf->data[0] = '\0';
    buf->size = 0;
    buf->capacity = capacity;
    return true;
}

void response_buffer_free(ResponseBuffer* buf) {
    if (!buf) return;
    free(buf->data);
    buf->data = NULL;
    buf->size = 0;
    buf->capacity = 0;
}

void response_buffer_clear(ResponseBuffer* buf) {
    if (!buf || !buf->data) return;
    buf->data[0] = '\0';
    buf->size = 0;
}

bool response_buffer_append(ResponseBuffer* buf, const char* data, size_t len) {
    if (!buf || !data) return false;

    // Check max size limit
    if (buf->size + len > RESPONSE_BUFFER_MAX_SIZE) {
        return false;
    }

    // Grow if needed
    if (buf->size + len >= buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        if (new_cap < buf->size + len + 1) {
            new_cap = buf->size + len + 1;
        }
        // Don't exceed max size
        if (new_cap > RESPONSE_BUFFER_MAX_SIZE) {
            new_cap = RESPONSE_BUFFER_MAX_SIZE;
        }

        char* new_data = realloc(buf->data, new_cap);
        if (!new_data) return false;
        buf->data = new_data;
        buf->capacity = new_cap;
    }

    memcpy(buf->data + buf->size, data, len);
    buf->size += len;
    buf->data[buf->size] = '\0';

    return true;
}

// ============================================================================
// CURL CALLBACKS IMPLEMENTATION
// ============================================================================

size_t provider_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    ResponseBuffer* buf = (ResponseBuffer*)userp;

    if (!buf) return 0;

    // Lazy initialization if buffer was declared with RESPONSE_BUFFER_INIT
    if (!buf->data) {
        if (!response_buffer_init(buf)) {
            return 0;  // Signal error to curl
        }
    }

    if (!response_buffer_append(buf, contents, total)) {
        return 0;  // Signal error to curl
    }

    return total;
}

// ============================================================================
// HTTP UTILITIES IMPLEMENTATION
// ============================================================================

void provider_set_common_curl_opts(CURL* curl, long timeout) {
    if (!curl) return;

    // Follow redirects (up to 5 hops)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

    // Timeout (0 = no timeout)
    if (timeout > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    }

    // Connection timeout (30 seconds)
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);

    // SSL verification (always enabled for security)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    // User agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Convergio/4.2");

    // TCP keepalive (helps with long-running requests)
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
}

struct curl_slist* provider_json_headers(void) {
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    return headers;
}

struct curl_slist* provider_add_auth_header(struct curl_slist* headers, const char* token) {
    if (!token) return headers;

    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", token);

    return curl_slist_append(headers, auth_header);
}
