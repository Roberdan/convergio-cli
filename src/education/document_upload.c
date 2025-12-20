/**
 * CONVERGIO EDUCATION - DOCUMENT UPLOAD
 *
 * Allows students to upload school materials (PDFs, images, DOCX, PPTX)
 * for teachers to help with specific assignments. Uses Claude Files API.
 *
 * Key Features:
 * - Restricted file picker (Desktop, Documents, Downloads only)
 * - Student-friendly folder navigation
 * - Claude Files API upload (500 MB max, file_id reusable)
 * - OpenAI fallback (32 MB direct base64)
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/education.h"
#include "nous/edition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_PATH_LEN 1024
#define MAX_FILENAME_LEN 256
#define MAX_FILES_PER_PAGE 10
#define MAX_UPLOADED_FILES 10

// Allowed base directories for students
static const char* ALLOWED_DIRS[] = {
    "Desktop",
    "Documents",
    "Downloads",
    NULL
};

// Allowed file extensions
static const char* ALLOWED_EXTENSIONS[] = {
    ".pdf", ".PDF",
    ".docx", ".DOCX", ".doc", ".DOC",
    ".pptx", ".PPTX", ".ppt", ".PPT",
    ".xlsx", ".XLSX", ".xls", ".XLS",
    ".jpg", ".JPG", ".jpeg", ".JPEG",
    ".png", ".PNG",
    ".txt", ".TXT",
    ".rtf", ".RTF",
    ".csv", ".CSV",
    NULL
};

// ============================================================================
// UPLOADED FILE TRACKING
// ============================================================================

typedef struct {
    char filename[MAX_FILENAME_LEN];
    char file_id[128];            // Claude Files API file_id
    char mime_type[64];
    size_t file_size;
    time_t uploaded_at;
    bool active;
} UploadedFile;

static UploadedFile g_uploaded_files[MAX_UPLOADED_FILES];
static int g_uploaded_count = 0;
static int g_current_file_index = -1;  // Currently active file for chat

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Get the user's home directory
 */
static const char* get_home_dir(void) {
    const char* home = getenv("HOME");
    if (!home) {
        home = "/tmp";
    }
    return home;
}

/**
 * Check if file extension is allowed
 */
static bool is_extension_allowed(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot) return false;

    for (int i = 0; ALLOWED_EXTENSIONS[i]; i++) {
        if (strcmp(dot, ALLOWED_EXTENSIONS[i]) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * Get MIME type from file extension
 */
static const char* get_mime_type(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot) return "application/octet-stream";

    // Convert to lowercase for comparison
    char ext[16] = {0};
    strncpy(ext, dot, sizeof(ext) - 1);
    for (int i = 0; ext[i]; i++) ext[i] = (char)tolower(ext[i]);

    if (strcmp(ext, ".pdf") == 0) return "application/pdf";
    if (strcmp(ext, ".docx") == 0) return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (strcmp(ext, ".doc") == 0) return "application/msword";
    if (strcmp(ext, ".pptx") == 0) return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    if (strcmp(ext, ".ppt") == 0) return "application/vnd.ms-powerpoint";
    if (strcmp(ext, ".xlsx") == 0) return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if (strcmp(ext, ".xls") == 0) return "application/vnd.ms-excel";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    if (strcmp(ext, ".rtf") == 0) return "application/rtf";
    if (strcmp(ext, ".csv") == 0) return "text/csv";

    return "application/octet-stream";
}

/**
 * Format file size for display
 */
static void format_file_size(size_t size, char* buffer, size_t buf_size) {
    if (size < 1024) {
        snprintf(buffer, buf_size, "%zu B", size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, buf_size, "%.1f KB", (double)size / 1024);
    } else {
        snprintf(buffer, buf_size, "%.1f MB", (double)size / (1024 * 1024));
    }
}

// ============================================================================
// FILE PICKER
// ============================================================================

typedef struct {
    char name[MAX_FILENAME_LEN];
    bool is_directory;
    size_t size;
} FileEntry;

/**
 * List files in a directory (filtered for students)
 */
static int list_directory(const char* path, FileEntry* entries, int max_entries) {
    DIR* dir = opendir(path);
    if (!dir) return 0;

    int count = 0;
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL && count < max_entries) {
        // Skip hidden files and . / ..
        if (entry->d_name[0] == '.') continue;

        // Get full path for stat
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        bool is_dir = S_ISDIR(st.st_mode);

        // For files, check extension
        if (!is_dir && !is_extension_allowed(entry->d_name)) continue;

        strncpy(entries[count].name, entry->d_name, MAX_FILENAME_LEN - 1);
        entries[count].name[MAX_FILENAME_LEN - 1] = '\0';
        entries[count].is_directory = is_dir;
        entries[count].size = is_dir ? 0 : (size_t)st.st_size;
        count++;
    }

    closedir(dir);
    return count;
}

/**
 * Interactive file picker
 */
char* document_file_picker(void) {
    if (edition_current() != EDITION_EDUCATION) {
        return NULL;  // Only in education edition
    }

    const char* home = get_home_dir();
    char current_dir[MAX_PATH_LEN];
    int current_base = 0;  // 0=Desktop, 1=Documents, 2=Downloads

    // Start at Desktop
    snprintf(current_dir, sizeof(current_dir), "%s/%s", home, ALLOWED_DIRS[0]);

    printf("\n📂 Document Upload - Select a file to upload\n");
    printf("   (Only Desktop, Documents, and Downloads folders are accessible)\n\n");

    while (true) {
        // Show current location
        const char* rel_path = strrchr(current_dir, '/');
        rel_path = rel_path ? rel_path + 1 : current_dir;
        printf("📁 Current: %s\n\n", rel_path);

        // List files
        FileEntry entries[50];
        int count = list_directory(current_dir, entries, 50);

        if (count == 0) {
            printf("   (No supported files in this folder)\n\n");
        }

        // Print entries
        int page = 0;
        (void)page;  // Reserved for future pagination

        for (int i = 0; i < MAX_FILES_PER_PAGE && i < count; i++) {
            char size_str[32] = "";
            if (!entries[i].is_directory) {
                format_file_size(entries[i].size, size_str, sizeof(size_str));
            }

            printf("   %2d. %s%s %s\n",
                   i + 1,
                   entries[i].is_directory ? "📁 " : "📄 ",
                   entries[i].name,
                   size_str);
        }

        printf("\n");
        printf("Commands:\n");
        printf("   [number] - Select file/folder\n");
        printf("   d/D/doc  - Go to Documents\n");
        printf("   k/K/desk - Go to Desktop\n");
        printf("   l/L/down - Go to Downloads\n");
        printf("   ..       - Go up one level\n");
        printf("   q/Q      - Cancel\n");
        printf("\n> ");

        // Get user input
        char input[64];
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            return NULL;
        }

        // Trim newline
        input[strcspn(input, "\n")] = '\0';

        // Process input
        if (strlen(input) == 0) continue;

        // Quit
        if (strcasecmp(input, "q") == 0) {
            printf("Upload cancelled.\n");
            return NULL;
        }

        // Navigation shortcuts
        if (strcasecmp(input, "d") == 0 || strcasecmp(input, "doc") == 0) {
            snprintf(current_dir, sizeof(current_dir), "%s/Documents", home);
            current_base = 1;
            printf("\n");
            continue;
        }
        if (strcasecmp(input, "k") == 0 || strcasecmp(input, "desk") == 0) {
            snprintf(current_dir, sizeof(current_dir), "%s/Desktop", home);
            current_base = 0;
            printf("\n");
            continue;
        }
        if (strcasecmp(input, "l") == 0 || strcasecmp(input, "down") == 0) {
            snprintf(current_dir, sizeof(current_dir), "%s/Downloads", home);
            current_base = 2;
            printf("\n");
            continue;
        }

        // Go up
        if (strcmp(input, "..") == 0) {
            // Check if we're at a base directory
            char base_check[MAX_PATH_LEN];
            snprintf(base_check, sizeof(base_check), "%s/%s", home, ALLOWED_DIRS[current_base]);
            if (strcmp(current_dir, base_check) == 0) {
                printf("   (Already at top level)\n\n");
                continue;
            }

            // Go up one level
            char* last_slash = strrchr(current_dir, '/');
            if (last_slash && last_slash != current_dir) {
                *last_slash = '\0';
            }
            printf("\n");
            continue;
        }

        // Number selection
        int selection = atoi(input);
        if (selection > 0 && selection <= count) {
            FileEntry* selected = &entries[selection - 1];

            if (selected->is_directory) {
                // Navigate into directory
                char new_path[MAX_PATH_LEN];
                snprintf(new_path, sizeof(new_path), "%s/%s", current_dir, selected->name);
                strncpy(current_dir, new_path, sizeof(current_dir) - 1);
                printf("\n");
            } else {
                // File selected - return full path
                char* result = malloc(MAX_PATH_LEN);
                if (result) {
                    snprintf(result, MAX_PATH_LEN, "%s/%s", current_dir, selected->name);
                    printf("\n✓ Selected: %s\n", selected->name);
                    return result;
                }
            }
            continue;
        }

        printf("   Invalid selection. Try again.\n\n");
    }
}

// ============================================================================
// CLAUDE FILES API UPLOAD
// ============================================================================

// External function from providers/anthropic.c
extern char* anthropic_upload_file(const char* filepath, const char* purpose);

/**
 * Upload a document to Claude Files API
 */
bool document_upload(const char* filepath) {
    if (!filepath) return false;

    // Check if already uploaded
    for (int i = 0; i < g_uploaded_count; i++) {
        if (strstr(filepath, g_uploaded_files[i].filename)) {
            printf("File already uploaded. Use /doc to reference it.\n");
            g_current_file_index = i;
            return true;
        }
    }

    // Check slot availability
    if (g_uploaded_count >= MAX_UPLOADED_FILES) {
        printf("Maximum uploaded files reached. Use /doc clear to remove old files.\n");
        return false;
    }

    // Get filename from path
    const char* filename = strrchr(filepath, '/');
    filename = filename ? filename + 1 : filepath;

    // Get file size
    struct stat st;
    if (stat(filepath, &st) != 0) {
        printf("Error: Cannot access file.\n");
        return false;
    }

    // Check size (500 MB max for Claude)
    if (st.st_size > 500 * 1024 * 1024) {
        printf("Error: File too large (max 500 MB).\n");
        return false;
    }

    printf("Uploading %s...\n", filename);

    // Call Claude Files API
    char* file_id = anthropic_upload_file(filepath, "user_data");

    if (!file_id) {
        printf("Error: Upload failed. Check your API key and connection.\n");
        return false;
    }

    // Store in tracking
    UploadedFile* uf = &g_uploaded_files[g_uploaded_count];
    strncpy(uf->filename, filename, MAX_FILENAME_LEN - 1);
    strncpy(uf->file_id, file_id, sizeof(uf->file_id) - 1);
    strncpy(uf->mime_type, get_mime_type(filename), sizeof(uf->mime_type) - 1);
    uf->file_size = (size_t)st.st_size;
    uf->uploaded_at = time(NULL);
    uf->active = true;

    g_current_file_index = g_uploaded_count;
    g_uploaded_count++;

    free(file_id);

    char size_str[32];
    format_file_size((size_t)st.st_size, size_str, sizeof(size_str));

    printf("\n✓ Upload complete!\n");
    printf("  File: %s (%s)\n", filename, size_str);
    printf("\nYou can now ask questions about this document.\n");
    printf("Example: \"Explain page 1\" or \"What is this document about?\"\n\n");

    return true;
}

// ============================================================================
// DOCUMENT MANAGEMENT
// ============================================================================

/**
 * List uploaded documents
 */
void document_list(void) {
    printf("\n📚 Uploaded Documents:\n\n");

    if (g_uploaded_count == 0) {
        printf("   No documents uploaded yet.\n");
        printf("   Use /upload to add a document.\n\n");
        return;
    }

    for (int i = 0; i < g_uploaded_count; i++) {
        UploadedFile* uf = &g_uploaded_files[i];
        char size_str[32];
        format_file_size(uf->file_size, size_str, sizeof(size_str));

        printf("   %d. %s%s (%s)\n",
               i + 1,
               (i == g_current_file_index) ? "📖 " : "📄 ",
               uf->filename,
               size_str);
    }

    printf("\n   * Current document is marked with 📖\n");
    printf("   Use /doc <number> to switch documents\n");
    printf("   Use /doc clear to remove all\n\n");
}

/**
 * Select active document by index
 */
bool document_select(int index) {
    if (index < 1 || index > g_uploaded_count) {
        printf("Invalid document number. Use /doc list to see available documents.\n");
        return false;
    }

    g_current_file_index = index - 1;
    printf("✓ Now using: %s\n", g_uploaded_files[g_current_file_index].filename);
    return true;
}

/**
 * Clear all uploaded documents
 */
void document_clear(void) {
    g_uploaded_count = 0;
    g_current_file_index = -1;
    printf("✓ All documents cleared.\n");
}

/**
 * Get current document's file_id for API calls
 */
const char* document_get_current_file_id(void) {
    if (g_current_file_index < 0 || g_current_file_index >= g_uploaded_count) {
        return NULL;
    }
    return g_uploaded_files[g_current_file_index].file_id;
}

/**
 * Get current document's filename
 */
const char* document_get_current_filename(void) {
    if (g_current_file_index < 0 || g_current_file_index >= g_uploaded_count) {
        return NULL;
    }
    return g_uploaded_files[g_current_file_index].filename;
}

/**
 * Check if a document is currently active
 */
bool document_is_active(void) {
    return g_current_file_index >= 0 && g_current_file_index < g_uploaded_count;
}

// ============================================================================
// COMMAND HANDLER
// ============================================================================

/**
 * /upload command handler
 *
 * Usage:
 *   /upload          - Open file picker
 *   /upload <path>   - Upload specific file
 *   /doc             - Show uploaded documents
 *   /doc list        - List all uploaded documents
 *   /doc <n>         - Select document n
 *   /doc clear       - Clear all uploaded documents
 */
int document_command_handler(int argc, char** argv) {
    if (edition_current() != EDITION_EDUCATION) {
        printf("Document upload is only available in Education edition.\n");
        return 1;
    }

    // /upload - open file picker
    if (argc < 2 || strcmp(argv[0], "upload") == 0) {
        char* filepath = document_file_picker();
        if (filepath) {
            bool success = document_upload(filepath);
            free(filepath);
            return success ? 0 : 1;
        }
        return 1;
    }

    // /doc commands
    if (strcmp(argv[0], "doc") == 0 || strcmp(argv[0], "document") == 0) {
        if (argc == 1) {
            document_list();
            return 0;
        }

        if (strcmp(argv[1], "list") == 0) {
            document_list();
            return 0;
        }

        if (strcmp(argv[1], "clear") == 0) {
            document_clear();
            return 0;
        }

        // Try to select by number
        int num = atoi(argv[1]);
        if (num > 0) {
            return document_select(num) ? 0 : 1;
        }

        printf("Usage: /doc [list|clear|<number>]\n");
        return 1;
    }

    printf("Unknown command. Use /upload or /doc.\n");
    return 1;
}
