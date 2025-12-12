/**
 * CONVERGIO CLIPBOARD
 *
 * Clipboard utilities for macOS - image detection and extraction
 * Uses NSPasteboard to interact with system clipboard
 */

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include "nous/clipboard.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

bool clipboard_has_image(void) {
    @autoreleasepool {
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];

        // Check for image types in clipboard
        NSArray *imageTypes = @[
            NSPasteboardTypePNG,
            NSPasteboardTypeTIFF,
            @"public.jpeg",
            @"public.png"
        ];

        NSString *available = [pasteboard availableTypeFromArray:imageTypes];
        return available != nil;
    }
}

char* clipboard_save_image(const char* directory) {
    @autoreleasepool {
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];

        // Try to get image data
        NSData *imageData = nil;
        NSString *extension = @"png";

        // Try PNG first
        imageData = [pasteboard dataForType:NSPasteboardTypePNG];
        if (!imageData) {
            imageData = [pasteboard dataForType:@"public.png"];
        }

        // Try TIFF (convert to PNG)
        if (!imageData) {
            imageData = [pasteboard dataForType:NSPasteboardTypeTIFF];
            if (imageData) {
                NSBitmapImageRep *imageRep = [NSBitmapImageRep imageRepWithData:imageData];
                if (imageRep) {
                    imageData = [imageRep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
                }
            }
        }

        // Try JPEG
        if (!imageData) {
            imageData = [pasteboard dataForType:@"public.jpeg"];
            if (imageData) {
                extension = @"jpg";
            }
        }

        if (!imageData || [imageData length] == 0) {
            return NULL;
        }

        // Generate unique filename with timestamp
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);

        NSString *dir = directory ? [NSString stringWithUTF8String:directory] : NSTemporaryDirectory();
        NSString *filename = [NSString stringWithFormat:@"clipboard_%s.%@", timestamp, extension];
        NSString *filepath = [dir stringByAppendingPathComponent:filename];

        // Write to file
        BOOL success = [imageData writeToFile:filepath atomically:YES];
        if (!success) {
            return NULL;
        }

        // Return path as C string
        const char *pathCStr = [filepath UTF8String];
        char *result = strdup(pathCStr);
        return result;
    }
}

char* clipboard_get_text(void) {
    @autoreleasepool {
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        NSString *text = [pasteboard stringForType:NSPasteboardTypeString];

        if (!text || [text length] == 0) {
            return NULL;
        }

        return strdup([text UTF8String]);
    }
}
