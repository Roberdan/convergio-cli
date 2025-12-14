/**
 * CONVERGIO NATIVE - Stub implementations
 *
 * Stub functions for symbols that are not needed in the native app.
 * These will be replaced with proper implementations as features are added.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Forward declarations for types
typedef void* CURL;

// cJSON stubs - The app uses Swift JSON, not C JSON
void* cJSON_Parse(const char* value) { return NULL; }
void* cJSON_CreateObject(void) { return NULL; }
void* cJSON_CreateArray(void) { return NULL; }
void* cJSON_CreateString(const char* string) { return NULL; }
void* cJSON_CreateNumber(double num) { return NULL; }
void* cJSON_CreateBool(int boolean) { return NULL; }
void* cJSON_CreateNull(void) { return NULL; }
void cJSON_Delete(void* item) { }
void* cJSON_AddItemToObject(void* object, const char* string, void* item) { return NULL; }
void* cJSON_AddItemToArray(void* array, void* item) { return NULL; }
void* cJSON_GetObjectItem(const void* object, const char* string) { return NULL; }
void* cJSON_GetArrayItem(const void* array, int index) { return NULL; }
int cJSON_GetArraySize(const void* array) { return 0; }
char* cJSON_Print(const void* item) { return NULL; }
char* cJSON_PrintUnformatted(const void* item) { return NULL; }
int cJSON_IsString(const void* item) { return 0; }
int cJSON_IsNumber(const void* item) { return 0; }
int cJSON_IsArray(const void* item) { return 0; }
int cJSON_IsObject(const void* item) { return 0; }
int cJSON_IsBool(const void* item) { return 0; }
int cJSON_IsNull(const void* item) { return 0; }
int cJSON_IsTrue(const void* item) { return 0; }
int cJSON_IsFalse(const void* item) { return 0; }
void* cJSON_Duplicate(const void* item, int recurse) { return NULL; }
int cJSON_AddStringToObject(void* object, const char* name, const char* string) { return 0; }
int cJSON_AddNumberToObject(void* object, const char* name, double number) { return 0; }
int cJSON_AddBoolToObject(void* object, const char* name, int boolean) { return 0; }
char* cJSON_GetStringValue(const void* item) { return NULL; }
double cJSON_GetNumberValue(const void* item) { return 0.0; }
void cJSON_ReplaceItemInObject(void* object, const char* string, void* newitem) { }
void cJSON_DetachItemFromObject(void* object, const char* string) { }
void* cJSON_AddObjectToObject(void* object, const char* name) { return NULL; }
void* cJSON_AddArrayToObject(void* object, const char* name) { return NULL; }
void cJSON_free(void* ptr) { }
void* cJSON_GetObjectItemCaseSensitive(const void* object, const char* string) { return NULL; }

// MLX bridge stubs - MLX will be handled separately in Swift
bool mlx_bridge_is_available(void) { return false; }
bool mlx_bridge_model_exists(const char* model) { return false; }
int mlx_bridge_load_model(const char* model, void* ctx) { return -1; }
void mlx_bridge_unload_model(void) { }
char* mlx_bridge_chat(const char* messages_json, const char* system_prompt) { return NULL; }
char* mlx_bridge_generate(const char* prompt, int max_tokens, float temp) { return NULL; }

// Logging stub - Uses os_log in native app
void nous_log(int level, const char* format, ...) { }

// Provider write callback stub
size_t provider_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    return size * nmemb;
}

// cJSON error pointer stub
const char* cJSON_GetErrorPtr(void) { return NULL; }

// Global running flag stub
volatile bool g_running = true;

// Additional MLX bridge stubs
void mlx_bridge_clear_cache(void) { }
int mlx_bridge_download_model(const char* model, void* progress_callback, void* ctx) { return -1; }
size_t mlx_bridge_gpu_memory_used(void) { return 0; }
