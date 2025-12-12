/**
 * CONVERGIO CLIPBOARD
 *
 * Clipboard utilities for macOS - image detection and extraction
 */

#ifndef CONVERGIO_CLIPBOARD_H
#define CONVERGIO_CLIPBOARD_H

#include <stdbool.h>

// Check if clipboard contains an image
bool clipboard_has_image(void);

// Save clipboard image to a file, returns the path or NULL on failure
// Caller must free the returned string
char* clipboard_save_image(const char* directory);

// Get text content from clipboard (if any)
// Caller must free the returned string
char* clipboard_get_text(void);

#endif // CONVERGIO_CLIPBOARD_H
