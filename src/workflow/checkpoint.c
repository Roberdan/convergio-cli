/**
 * CONVERGIO WORKFLOW CHECKPOINT MANAGER
 *
 * Handles checkpoint creation, storage, and restoration
 * Uses SQLite for persistence with parameterized queries for security
 */

#include "nous/workflow.h"
#include "nous/nous.h"
#include "nous/debug_mutex.h"
#include <cjson/cJSON.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

// ============================================================================
// EXTERNAL DATABASE ACCESS
// ============================================================================

// Access to global database (shared with persistence.c)
extern sqlite3* g_db;
extern ConvergioMutex g_db_mutex;

// ============================================================================
// STATE SERIALIZATION
// ============================================================================

// Serialize workflow state to JSON
static char* serialize_workflow_state(const WorkflowState* state) {
    if (!state) {
        return NULL;
    }
    
    cJSON* json = cJSON_CreateObject();
    if (!json) {
        return NULL;
    }
    
    cJSON* entries = cJSON_CreateArray();
    if (!entries) {
        cJSON_Delete(json);
        return NULL;
    }
    
    cJSON_AddItemToObject(json, "entries", entries);
    
    for (size_t i = 0; i < state->entry_count; i++) {
        if (!state->entries[i].key || !state->entries[i].value) {
            continue;
        }
        
        cJSON* entry = cJSON_CreateObject();
        if (!entry) {
            cJSON_Delete(json);
            return NULL;
        }
        
        cJSON_AddStringToObject(entry, "key", state->entries[i].key);
        cJSON_AddStringToObject(entry, "value", state->entries[i].value);
        cJSON_AddNumberToObject(entry, "updated_at", (double)state->entries[i].updated_at);
        
        cJSON_AddItemToArray(entries, entry);
    }
    
    char* json_string = cJSON_Print(json);
    cJSON_Delete(json);
    
    return json_string;
}

// Deserialize workflow state from JSON
static int deserialize_workflow_state(WorkflowState* state, const char* json_str) {
    if (!state || !json_str) {
        return -1;
    }
    
    cJSON* json = cJSON_Parse(json_str);
    if (!json) {
        return -1;
    }
    
    cJSON* entries = cJSON_GetObjectItem(json, "entries");
    if (!cJSON_IsArray(entries)) {
        cJSON_Delete(json);
        return -1;
    }
    
    int array_size = cJSON_GetArraySize(entries);
    for (int i = 0; i < array_size; i++) {
        cJSON* entry = cJSON_GetArrayItem(entries, i);
        if (!cJSON_IsObject(entry)) {
            continue;
        }
        
        cJSON* key_obj = cJSON_GetObjectItem(entry, "key");
        cJSON* value_obj = cJSON_GetObjectItem(entry, "value");
        
        if (!cJSON_IsString(key_obj) || !cJSON_IsString(value_obj)) {
            continue;
        }
        
        const char* key = cJSON_GetStringValue(key_obj);
        const char* value = cJSON_GetStringValue(value_obj);
        
        if (key && value) {
            workflow_state_set(state, key, value);
        }
    }
    
    cJSON_Delete(json);
    return 0;
}

// ============================================================================
// CHECKPOINT CREATION
// ============================================================================

uint64_t workflow_checkpoint(Workflow* wf, const char* node_name) {
    if (!wf || !g_db) {
        return 0;
    }
    
    // Serialize workflow state
    char* state_json = serialize_workflow_state(wf->state);
    if (!state_json) {
        return 0;
    }
    
    // Get current node ID (use entry node if not set)
    uint64_t node_id = wf->current_node_id;
    if (node_id == 0 && wf->entry_node) {
        node_id = wf->entry_node->node_id;
    }
    
    CONVERGIO_MUTEX_LOCK(&g_db_mutex);
    
    // Insert checkpoint with parameterized query (SQL injection prevention)
    const char* sql = "INSERT INTO workflow_checkpoints "
                      "(workflow_id, node_id, state_json, created_at) "
                      "VALUES (?, ?, ?, ?)";
    
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        free(state_json);
        state_json = NULL;
        return 0;
    }
    
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)wf->workflow_id);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)node_id);
    sqlite3_bind_text(stmt, 3, state_json, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)time(NULL));
    
    rc = sqlite3_step(stmt);
    uint64_t checkpoint_id = 0;
    
    if (rc == SQLITE_DONE) {
        checkpoint_id = (uint64_t)sqlite3_last_insert_rowid(g_db);
    }
    
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
    
    // Update workflow last checkpoint time
    if (checkpoint_id > 0) {
        wf->last_checkpoint_at = time(NULL);
    }
    
    free(state_json);
    state_json = NULL;
    
    return checkpoint_id;
}

// ============================================================================
// CHECKPOINT RESTORATION
// ============================================================================

int workflow_restore_from_checkpoint(Workflow* wf, uint64_t checkpoint_id) {
    if (!wf || !g_db || checkpoint_id == 0) {
        return -1;
    }
    
    CONVERGIO_MUTEX_LOCK(&g_db_mutex);
    
    // Load checkpoint with parameterized query
    const char* sql = "SELECT workflow_id, node_id, state_json, created_at "
                      "FROM workflow_checkpoints WHERE id = ?";
    
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }
    
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)checkpoint_id);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }
    
    // Verify workflow_id matches
    uint64_t stored_workflow_id = (uint64_t)sqlite3_column_int64(stmt, 0);
    if (stored_workflow_id != wf->workflow_id) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }
    
    // Get checkpoint data
    uint64_t node_id = (uint64_t)sqlite3_column_int64(stmt, 1);
    const char* state_json = (const char*)sqlite3_column_text(stmt, 2);
    
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
    
    // Validate state JSON
    if (!state_json) {
        return -1;
    }
    
    // Deserialize state
    if (wf->state) {
        workflow_state_clear(wf->state);
    } else {
        wf->state = workflow_state_create();
        if (!wf->state) {
            return -1;
        }
    }
    
    int result = deserialize_workflow_state(wf->state, state_json);
    if (result != 0) {
        return -1;
    }
    
    // Restore workflow state
    wf->current_node_id = node_id;
    wf->status = WORKFLOW_STATUS_PAUSED; // Restored workflows start paused
    wf->updated_at = time(NULL);
    
    if (wf->error_message) {
        free(wf->error_message);
        wf->error_message = NULL;
    }
    
    return 0;
}

// ============================================================================
// CHECKPOINT LISTING
// ============================================================================

Checkpoint* workflow_list_checkpoints(Workflow* wf, size_t* count) {
    if (!wf || !g_db || !count) {
        return NULL;
    }
    
    *count = 0;
    
    CONVERGIO_MUTEX_LOCK(&g_db_mutex);
    
    // Query checkpoints with parameterized query
    const char* sql = "SELECT id, node_id, state_json, created_at, metadata_json "
                      "FROM workflow_checkpoints "
                      "WHERE workflow_id = ? "
                      "ORDER BY created_at DESC";
    
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }
    
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)wf->workflow_id);
    
    // Count checkpoints first
    size_t checkpoint_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        checkpoint_count++;
    }
    
    if (checkpoint_count == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }
    
    // Allocate array
    Checkpoint* checkpoints = calloc(checkpoint_count, sizeof(Checkpoint));
    if (!checkpoints) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }
    
    // Reset statement and load data
    sqlite3_reset(stmt);
    size_t idx = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < checkpoint_count) {
        checkpoints[idx].checkpoint_id = (uint64_t)sqlite3_column_int64(stmt, 0);
        checkpoints[idx].workflow_id = wf->workflow_id;
        checkpoints[idx].node_id = (uint64_t)sqlite3_column_int64(stmt, 1);
        
        const char* state_json = (const char*)sqlite3_column_text(stmt, 2);
        if (state_json) {
            checkpoints[idx].state_json = workflow_strdup(state_json);
        }
        
        checkpoints[idx].created_at = (time_t)sqlite3_column_int64(stmt, 3);
        
        const char* metadata_json = (const char*)sqlite3_column_text(stmt, 4);
        if (metadata_json) {
            checkpoints[idx].metadata_json = workflow_strdup(metadata_json);
        }
        
        idx++;
    }
    
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
    
    *count = checkpoint_count;
    return checkpoints;
}

// Free checkpoint array
void workflow_free_checkpoints(Checkpoint* checkpoints, size_t count) {
    if (!checkpoints) {
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (checkpoints[i].state_json) {
            free(checkpoints[i].state_json);
            checkpoints[i].state_json = NULL;
        }
        if (checkpoints[i].metadata_json) {
            free(checkpoints[i].metadata_json);
            checkpoints[i].metadata_json = NULL;
        }
    }
    
    free(checkpoints);
    checkpoints = NULL;
}

