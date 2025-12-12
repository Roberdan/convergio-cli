/**
 * Streaming Markdown Renderer
 *
 * Renders markdown incrementally as chunks arrive from streaming API.
 */

#ifndef STREAM_MD_H
#define STREAM_MD_H

#include <stddef.h>

// Opaque streaming markdown context
typedef struct StreamMd StreamMd;

/**
 * Create a new streaming markdown renderer.
 */
StreamMd* stream_md_create(void);

/**
 * Destroy streaming markdown renderer.
 */
void stream_md_destroy(StreamMd* sm);

/**
 * Process a chunk of incoming text.
 * Outputs formatted text to stdout as it becomes unambiguous.
 */
void stream_md_process(StreamMd* sm, const char* chunk, size_t len);

/**
 * Process a single character.
 */
void stream_md_process_char(StreamMd* sm, char c);

/**
 * Finalize rendering - flush any pending state.
 * Call this when the stream is complete.
 */
void stream_md_finish(StreamMd* sm);

#endif // STREAM_MD_H
