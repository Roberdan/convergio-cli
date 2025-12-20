/**
 * CONVERGIO TASK DECOMPOSER
 *
 * CrewAI-inspired hierarchical task decomposition
 * Uses LLM to break down complex goals into actionable subtasks
 */

#include "nous/task_decomposer.h"
#include "nous/orchestrator.h"
#include "nous/provider.h"
#include "nous/planning.h"
#include "nous/nous.h"
#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_DESCRIPTION_LENGTH 512
#define MAX_VALIDATION_LENGTH 256
#define INITIAL_PREREQ_CAPACITY 4

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

extern Provider* provider_get(ProviderType type);
extern ExecutionPlan* orch_plan_create(const char* goal);
extern Task* orch_task_create(const char* description, SemanticID assignee);
extern void orch_plan_add_task(ExecutionPlan* plan, Task* task);
extern void orch_task_complete(Task* task, const char* result);

// ============================================================================
// TASK DECOMPOSITION (LLM-based)
// ============================================================================

// Parse JSON response from LLM into DecomposedTask array
static DecomposedTask* parse_decomposition_json(const char* json_str, size_t* out_count) {
    if (!json_str || !out_count) {
        return NULL;
    }
    
    cJSON* json = cJSON_Parse(json_str);
    if (!json) {
        return NULL;
    }
    
    cJSON* tasks_array = cJSON_GetObjectItem(json, "tasks");
    if (!cJSON_IsArray(tasks_array)) {
        cJSON_Delete(json);
        return NULL;
    }
    
    int array_size = cJSON_GetArraySize(tasks_array);
    if (array_size <= 0) {
        cJSON_Delete(json);
        return NULL;
    }
    
    DecomposedTask* tasks = calloc(array_size, sizeof(DecomposedTask));
    if (!tasks) {
        cJSON_Delete(json);
        return NULL;
    }
    
    static uint64_t next_task_id = 1;
    
    for (int i = 0; i < array_size; i++) {
        cJSON* task_obj = cJSON_GetArrayItem(tasks_array, i);
        if (!cJSON_IsObject(task_obj)) {
            continue;
        }
        
        tasks[i].task_id = next_task_id++;
        tasks[i].status = TASK_STATUS_PENDING;
        tasks[i].max_retries = 3;
        tasks[i].current_retry = 0;
        tasks[i].created_at = time(NULL);
        tasks[i].completed_at = 0;
        tasks[i].assigned_agent_id = 0;
        
        // Parse description
        cJSON* desc_obj = cJSON_GetObjectItem(task_obj, "description");
        if (cJSON_IsString(desc_obj)) {
            const char* desc = cJSON_GetStringValue(desc_obj);
            if (desc && strlen(desc) < MAX_DESCRIPTION_LENGTH) {
                tasks[i].description = strdup(desc);
            }
        }
        
        // Parse role
        cJSON* role_obj = cJSON_GetObjectItem(task_obj, "role");
        if (cJSON_IsString(role_obj)) {
            const char* role_str = cJSON_GetStringValue(role_obj);
            // Map string to AgentRole (simplified)
            if (strcmp(role_str, "analyst") == 0 || strcmp(role_str, "ANALYST") == 0) {
                tasks[i].required_role = AGENT_ROLE_ANALYST;
            } else if (strcmp(role_str, "coder") == 0 || strcmp(role_str, "CODER") == 0) {
                tasks[i].required_role = AGENT_ROLE_CODER;
            } else if (strcmp(role_str, "writer") == 0 || strcmp(role_str, "WRITER") == 0) {
                tasks[i].required_role = AGENT_ROLE_WRITER;
            } else if (strcmp(role_str, "critic") == 0 || strcmp(role_str, "CRITIC") == 0) {
                tasks[i].required_role = AGENT_ROLE_CRITIC;
            } else if (strcmp(role_str, "planner") == 0 || strcmp(role_str, "PLANNER") == 0) {
                tasks[i].required_role = AGENT_ROLE_PLANNER;
            } else {
                tasks[i].required_role = AGENT_ROLE_EXECUTOR;
            }
        }
        
        // Parse prerequisites
        cJSON* prereq_obj = cJSON_GetObjectItem(task_obj, "prerequisites");
        if (cJSON_IsArray(prereq_obj)) {
            int prereq_size = cJSON_GetArraySize(prereq_obj);
            if (prereq_size > 0) {
                tasks[i].prerequisite_capacity = prereq_size;
                tasks[i].prerequisite_ids = calloc(prereq_size, sizeof(uint64_t));
                if (tasks[i].prerequisite_ids) {
                    for (int j = 0; j < prereq_size; j++) {
                        cJSON* prereq_item = cJSON_GetArrayItem(prereq_obj, j);
                        if (cJSON_IsNumber(prereq_item)) {
                            // Prerequisite is referenced by index in array
                            int prereq_idx = (int)cJSON_GetNumberValue(prereq_item);
                            if (prereq_idx >= 0 && prereq_idx < array_size) {
                                tasks[i].prerequisite_ids[tasks[i].prerequisite_count] = 
                                    tasks[prereq_idx].task_id;
                                tasks[i].prerequisite_count++;
                            }
                        }
                    }
                }
            }
        }
        
        // Parse validation criteria
        cJSON* validation_obj = cJSON_GetObjectItem(task_obj, "validation");
        if (cJSON_IsString(validation_obj)) {
            const char* validation = cJSON_GetStringValue(validation_obj);
            if (validation && strlen(validation) < MAX_VALIDATION_LENGTH) {
                tasks[i].validation_criteria = strdup(validation);
            }
        }
    }
    
    cJSON_Delete(json);
    *out_count = array_size;
    return tasks;
}

// Decompose goal into subtasks using LLM
DecomposedTask* task_decompose(
    const char* goal,
    AgentRole* roles,
    size_t role_count,
    size_t* out_count
) {
    if (!goal || !out_count) {
        return NULL;
    }
    
    *out_count = 0;
    
    // Build prompt for task decomposition
    char prompt[2048];
    snprintf(prompt, sizeof(prompt),
        "Break down the following goal into actionable subtasks. "
        "Return a JSON object with a 'tasks' array. Each task should have:\n"
        "- 'description': clear task description\n"
        "- 'role': one of [analyst, coder, writer, critic, planner, executor]\n"
        "- 'prerequisites': array of task indices (0-based) that must complete first\n"
        "- 'validation': how to validate task completion\n\n"
        "Goal: %s\n\n"
        "Return only valid JSON, no markdown formatting.",
        goal);
    
    // Use LLM to decompose
    Provider* provider = provider_get(PROVIDER_ANTHROPIC);
    if (!provider || !provider->chat) {
        return NULL;
    }
    
    const char* system_prompt = 
        "You are a task decomposition expert. Break down complex goals into "
        "actionable subtasks with clear dependencies. Return only valid JSON.";
    
    TokenUsage usage = {0};
    char* response = provider->chat(
        provider,
        "claude-sonnet-4-20250514",
        system_prompt,
        prompt,
        &usage
    );
    
    if (!response) {
        return NULL;
    }
    
    // Parse response
    DecomposedTask* tasks = parse_decomposition_json(response, out_count);
    
    free(response);
    response = NULL;
    
    return tasks;
}

// ============================================================================
// DEPENDENCY RESOLUTION
// ============================================================================

// Detect circular dependencies using DFS
static bool has_cycle_dfs(
    const DecomposedTask* task,
    const DecomposedTask* all_tasks,
    size_t task_count,
    bool* visited,
    bool* rec_stack
) {
    if (!task) {
        return false;
    }
    
    uint64_t task_idx = task->task_id - 1; // Assuming IDs start at 1
    if (task_idx >= task_count) {
        return false;
    }
    
    if (rec_stack[task_idx]) {
        return true; // Cycle detected
    }
    
    if (visited[task_idx]) {
        return false;
    }
    
    visited[task_idx] = true;
    rec_stack[task_idx] = true;
    
    // Check all prerequisites
    for (size_t i = 0; i < task->prerequisite_count; i++) {
        uint64_t prereq_id = task->prerequisite_ids[i];
        
        // Find prerequisite task
        for (size_t j = 0; j < task_count; j++) {
            if (all_tasks[j].task_id == prereq_id) {
                if (has_cycle_dfs(&all_tasks[j], all_tasks, task_count, visited, rec_stack)) {
                    return true;
                }
                break;
            }
        }
    }
    
    rec_stack[task_idx] = false;
    return false;
}

// Detect circular dependencies
bool task_detect_cycles(DecomposedTask* tasks, size_t task_count) {
    if (!tasks || task_count == 0) {
        return false;
    }
    
    bool* visited = calloc(task_count, sizeof(bool));
    bool* rec_stack = calloc(task_count, sizeof(bool));
    
    if (!visited || !rec_stack) {
        free(visited);
        free(rec_stack);
        return false;
    }
    
    for (size_t i = 0; i < task_count; i++) {
        if (!visited[i]) {
            if (has_cycle_dfs(&tasks[i], tasks, task_count, visited, rec_stack)) {
                free(visited);
                free(rec_stack);
                return true;
            }
        }
    }
    
    free(visited);
    free(rec_stack);
    return false;
}

// Resolve dependencies and validate
int task_resolve_dependencies(DecomposedTask* tasks, size_t task_count) {
    if (!tasks || task_count == 0) {
        return -1;
    }
    
    // Detect cycles
    if (task_detect_cycles(tasks, task_count)) {
        return -1; // Circular dependency detected
    }
    
    // Validate prerequisite IDs exist
    for (size_t i = 0; i < task_count; i++) {
        for (size_t j = 0; j < tasks[i].prerequisite_count; j++) {
            uint64_t prereq_id = tasks[i].prerequisite_ids[j];
            bool found = false;
            
            for (size_t k = 0; k < task_count; k++) {
                if (tasks[k].task_id == prereq_id) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                return -1; // Invalid prerequisite ID
            }
        }
    }
    
    return 0;
}

// ============================================================================
// TOPOLOGICAL SORT (Kahn's Algorithm)
// ============================================================================

int task_topological_sort(
    DecomposedTask* tasks,
    size_t task_count,
    uint64_t* sorted_ids,
    size_t* out_count
) {
    if (!tasks || !sorted_ids || !out_count || task_count == 0) {
        return -1;
    }
    
    *out_count = 0;
    
    // Count incoming edges for each task
    size_t* in_degree = calloc(task_count, sizeof(size_t));
    if (!in_degree) {
        return -1;
    }
    
    // Build task ID to index map
    uint64_t* id_to_idx = calloc(task_count, sizeof(uint64_t));
    if (!id_to_idx) {
        free(in_degree);
        return -1;
    }
    
    for (size_t i = 0; i < task_count; i++) {
        id_to_idx[i] = tasks[i].task_id;
    }
    
    // Calculate in-degrees
    for (size_t i = 0; i < task_count; i++) {
        for (size_t j = 0; j < tasks[i].prerequisite_count; j++) {
            uint64_t prereq_id = tasks[i].prerequisite_ids[j];
            for (size_t k = 0; k < task_count; k++) {
                if (tasks[k].task_id == prereq_id) {
                    in_degree[k]++;
                    break;
                }
            }
        }
    }
    
    // Find tasks with no prerequisites (in-degree = 0)
    uint64_t* queue = calloc(task_count, sizeof(uint64_t));
    size_t queue_front = 0;
    size_t queue_back = 0;
    
    if (!queue) {
        free(in_degree);
        free(id_to_idx);
        return -1;
    }
    
    for (size_t i = 0; i < task_count; i++) {
        if (in_degree[i] == 0) {
            queue[queue_back++] = tasks[i].task_id;
        }
    }
    
    // Process queue
    while (queue_front < queue_back) {
        uint64_t current_id = queue[queue_front++];
        sorted_ids[(*out_count)++] = current_id;
        
        // Find current task and reduce in-degree of dependents
        size_t current_idx = SIZE_MAX;
        for (size_t i = 0; i < task_count; i++) {
            if (tasks[i].task_id == current_id) {
                current_idx = i;
                break;
            }
        }
        
        if (current_idx == SIZE_MAX) {
            continue;
        }
        
        // Reduce in-degree of tasks that depend on current
        for (size_t i = 0; i < task_count; i++) {
            for (size_t j = 0; j < tasks[i].prerequisite_count; j++) {
                if (tasks[i].prerequisite_ids[j] == current_id) {
                    // Find index of task i
                    size_t dep_idx = SIZE_MAX;
                    for (size_t k = 0; k < task_count; k++) {
                        if (tasks[k].task_id == tasks[i].task_id) {
                            dep_idx = k;
                            break;
                        }
                    }
                    
                    if (dep_idx != SIZE_MAX && in_degree[dep_idx] > 0) {
                        in_degree[dep_idx]--;
                        if (in_degree[dep_idx] == 0) {
                            queue[queue_back++] = tasks[dep_idx].task_id;
                        }
                    }
                    break;
                }
            }
        }
    }
    
    free(in_degree);
    free(id_to_idx);
    free(queue);
    
    // Check if all tasks were sorted (no cycles)
    if (*out_count != task_count) {
        return -1; // Cycle detected or invalid graph
    }
    
    return 0;
}

// ============================================================================
// EXECUTION PLAN CREATION
// ============================================================================

ExecutionPlan* task_create_execution_plan(DecomposedTask* tasks, size_t task_count) {
    if (!tasks || task_count == 0) {
        return NULL;
    }
    
    // Resolve dependencies first
    if (task_resolve_dependencies(tasks, task_count) != 0) {
        return NULL;
    }
    
    // Topological sort
    uint64_t* sorted_ids = calloc(task_count, sizeof(uint64_t));
    size_t sorted_count = 0;
    
    if (!sorted_ids) {
        return NULL;
    }
    
    if (task_topological_sort(tasks, task_count, sorted_ids, &sorted_count) != 0) {
        free(sorted_ids);
        return NULL;
    }
    
    // Create execution plan
    ExecutionPlan* plan = orch_plan_create("Decomposed Task Execution");
    if (!plan) {
        free(sorted_ids);
        return NULL;
    }
    
    // Create tasks in topological order
    for (size_t i = 0; i < sorted_count; i++) {
        // Find task by ID
        DecomposedTask* decomposed = NULL;
        for (size_t j = 0; j < task_count; j++) {
            if (tasks[j].task_id == sorted_ids[i]) {
                decomposed = &tasks[j];
                break;
            }
        }
        
        if (!decomposed || !decomposed->description) {
            continue;
        }
        
        // Create orchestrator task
        Task* task = orch_task_create(decomposed->description, decomposed->assigned_agent_id);
        if (task) {
            orch_plan_add_task(plan, task);
        }
    }
    
    free(sorted_ids);
    return plan;
}

// ============================================================================
// TASK MANAGEMENT
// ============================================================================

bool task_prerequisites_met(
    const DecomposedTask* task,
    const DecomposedTask* all_tasks,
    size_t task_count
) {
    if (!task || !all_tasks) {
        return false;
    }
    
    if (task->prerequisite_count == 0) {
        return true;
    }
    
    for (size_t i = 0; i < task->prerequisite_count; i++) {
        uint64_t prereq_id = task->prerequisite_ids[i];
        bool found = false;
        
        for (size_t j = 0; j < task_count; j++) {
            if (all_tasks[j].task_id == prereq_id) {
                if (all_tasks[j].status != TASK_STATUS_COMPLETED) {
                    return false;
                }
                found = true;
                break;
            }
        }
        
        if (!found) {
            return false;
        }
    }
    
    return true;
}

DecomposedTask* task_get_ready(DecomposedTask* tasks, size_t task_count, size_t* out_count) {
    if (!tasks || !out_count) {
        return NULL;
    }
    
    *out_count = 0;
    
    // Count ready tasks
    size_t ready_count = 0;
    for (size_t i = 0; i < task_count; i++) {
        if (tasks[i].status == TASK_STATUS_PENDING &&
            task_prerequisites_met(&tasks[i], tasks, task_count)) {
            ready_count++;
        }
    }
    
    if (ready_count == 0) {
        return NULL;
    }
    
    // Allocate and populate
    DecomposedTask* ready = calloc(ready_count, sizeof(DecomposedTask));
    if (!ready) {
        return NULL;
    }
    
    size_t idx = 0;
    for (size_t i = 0; i < task_count; i++) {
        if (tasks[i].status == TASK_STATUS_PENDING &&
            task_prerequisites_met(&tasks[i], tasks, task_count)) {
            ready[idx++] = tasks[i];
        }
    }
    
    *out_count = ready_count;
    return ready;
}

int task_mark_completed(DecomposedTask* task, const char* result) {
    if (!task) {
        return -1;
    }
    
    task->status = TASK_STATUS_COMPLETED;
    task->completed_at = time(NULL);
    
    if (result) {
        if (task->result) {
            free(task->result);
            task->result = NULL;
        }
        task->result = strdup(result);
    }
    
    return 0;
}

int task_mark_failed(DecomposedTask* task, const char* error) {
    if (!task) {
        return -1;
    }
    
    task->status = TASK_STATUS_FAILED;
    task->completed_at = time(NULL);
    
    if (error) {
        if (task->result) {
            free(task->result);
            task->result = NULL;
        }
        task->result = strdup(error);
    }
    
    return 0;
}

// ============================================================================
// PARALLEL EXECUTION
// ============================================================================

int task_execute_parallel(
    DecomposedTask* tasks,
    size_t task_count,
    dispatch_group_t group
) {
    if (!tasks || task_count == 0 || !group) {
        return -1;
    }
    
    // Get ready tasks
    size_t ready_count = 0;
    DecomposedTask* ready = task_get_ready(tasks, task_count, &ready_count);
    
    if (!ready || ready_count == 0) {
        return 0; // No ready tasks
    }
    
    // Execute ready tasks in parallel
    dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);
    
    for (size_t i = 0; i < ready_count; i++) {
        DecomposedTask* task = &ready[i];
        
        dispatch_group_async(group, queue, ^{
            // Mark as in progress
            task->status = TASK_STATUS_IN_PROGRESS;
            
            // TODO: Execute task via agent
            // For now, just mark as completed
            task_mark_completed(task, "Task executed");
        });
    }
    
    // Note: ready array is a copy, caller should free it
    // But we don't free it here as it's used in the async blocks
    
    return 0;
}

// ============================================================================
// CLEANUP
// ============================================================================

void task_free_decomposed(DecomposedTask* tasks, size_t count) {
    if (!tasks) {
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (tasks[i].description) {
            free(tasks[i].description);
            tasks[i].description = NULL;
        }
        if (tasks[i].validation_criteria) {
            free(tasks[i].validation_criteria);
            tasks[i].validation_criteria = NULL;
        }
        if (tasks[i].result) {
            free(tasks[i].result);
            tasks[i].result = NULL;
        }
        if (tasks[i].prerequisite_ids) {
            free(tasks[i].prerequisite_ids);
            tasks[i].prerequisite_ids = NULL;
        }
    }
    
    free(tasks);
    tasks = NULL;
}

