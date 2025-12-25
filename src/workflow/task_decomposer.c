/**
 * CONVERGIO TASK DECOMPOSER
 *
 * CrewAI-inspired hierarchical task decomposition
 * Uses LLM to break down complex goals into actionable subtasks
 */

#include "nous/task_decomposer.h"
#include "nous/nous.h"
#include "nous/orchestrator.h" // For llm_chat, llm_chat_with_model, etc.
#include "nous/planning.h"
#include <cjson/cJSON.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_DESCRIPTION_LENGTH 512
#define MAX_VALIDATION_LENGTH 256
#define INITIAL_PREREQ_CAPACITY 4

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

extern ExecutionPlan* orch_plan_create(const char* goal);
extern Task* orch_task_create(const char* description, SemanticID assignee);
extern void orch_plan_add_task(ExecutionPlan* plan, Task* task);
extern void orch_task_complete(Task* task, const char* result);
extern ManagedAgent* agent_find_by_role(AgentRole role);
extern ManagedAgent* agent_spawn(AgentRole role, const char* name, const char* context);
extern void cost_record_agent_usage(ManagedAgent* agent, uint64_t input_tokens,
                                    uint64_t output_tokens);

// workflow_strdup is defined in workflow_types.c
extern char* workflow_strdup(const char* str);

// ============================================================================
// TASK DECOMPOSITION (LLM-based)
// ============================================================================

// Security: Maximum limits for LLM output validation
#define MAX_TASKS_FROM_LLM 50
#define MAX_PREREQUISITES_PER_TASK 20
#define MAX_ROLE_LENGTH 32

// Security: Validate and sanitize LLM-provided string
static char* sanitize_llm_string(const char* str, size_t max_len) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str);
    if (len == 0 || len > max_len) {
        return NULL;
    }

    // Check for dangerous patterns
    const char* dangerous[] = {"<script", "javascript:", "eval(", "exec(", NULL};
    for (int i = 0; dangerous[i]; i++) {
        if (strstr(str, dangerous[i])) {
            return NULL;
        }
    }

    return workflow_strdup(str);
}

// Security: Validate role string from LLM
static AgentRole validate_role_string(const char* role_str) {
    if (!role_str || strlen(role_str) > MAX_ROLE_LENGTH) {
        return AGENT_ROLE_EXECUTOR; // Default fallback
    }

    // Case-insensitive comparison with allowed roles only
    if (strcasecmp(role_str, "analyst") == 0)
        return AGENT_ROLE_ANALYST;
    if (strcasecmp(role_str, "coder") == 0)
        return AGENT_ROLE_CODER;
    if (strcasecmp(role_str, "writer") == 0)
        return AGENT_ROLE_WRITER;
    if (strcasecmp(role_str, "critic") == 0)
        return AGENT_ROLE_CRITIC;
    if (strcasecmp(role_str, "planner") == 0)
        return AGENT_ROLE_PLANNER;
    if (strcasecmp(role_str, "executor") == 0)
        return AGENT_ROLE_EXECUTOR;

    return AGENT_ROLE_EXECUTOR; // Unknown role defaults to executor
}

// Parse JSON response from LLM into DecomposedTask array
// Security: Implements JSON schema validation for LLM outputs
static DecomposedTask* parse_decomposition_json(const char* json_str, size_t* out_count) {
    if (!json_str || !out_count) {
        return NULL;
    }

    // Security: Limit JSON input size
    size_t json_len = strlen(json_str);
    if (json_len > 1024 * 1024) { // Max 1MB JSON
        return NULL;
    }

    cJSON* json = cJSON_Parse(json_str);
    if (!json) {
        return NULL;
    }

    // Schema validation: root must be object with "tasks" array
    if (!cJSON_IsObject(json)) {
        cJSON_Delete(json);
        return NULL;
    }

    cJSON* tasks_array = cJSON_GetObjectItem(json, "tasks");
    if (!cJSON_IsArray(tasks_array)) {
        cJSON_Delete(json);
        return NULL;
    }

    int array_size = cJSON_GetArraySize(tasks_array);

    // Security: Limit number of tasks from LLM
    if (array_size <= 0 || array_size > MAX_TASKS_FROM_LLM) {
        cJSON_Delete(json);
        return NULL;
    }

    DecomposedTask* tasks = calloc((size_t)array_size, sizeof(DecomposedTask));
    if (!tasks) {
        cJSON_Delete(json);
        return NULL;
    }

    static uint64_t next_task_id = 1;
    int valid_task_count = 0;

    for (int i = 0; i < array_size; i++) {
        cJSON* task_obj = cJSON_GetArrayItem(tasks_array, i);

        // Schema validation: each task must be an object
        if (!cJSON_IsObject(task_obj)) {
            continue;
        }

        // Schema validation: description is required
        cJSON* desc_obj = cJSON_GetObjectItem(task_obj, "description");
        if (!cJSON_IsString(desc_obj)) {
            continue; // Skip tasks without description
        }

        const char* desc = cJSON_GetStringValue(desc_obj);
        char* sanitized_desc = sanitize_llm_string(desc, MAX_DESCRIPTION_LENGTH);
        if (!sanitized_desc) {
            continue; // Skip tasks with invalid description
        }

        // Task is valid, populate it
        int idx = valid_task_count++;
        tasks[idx].task_id = next_task_id++;
        tasks[idx].status = TASK_STATUS_PENDING;
        tasks[idx].max_retries = 3;
        tasks[idx].current_retry = 0;
        tasks[idx].created_at = time(NULL);
        tasks[idx].completed_at = 0;
        tasks[idx].assigned_agent_id = 0;
        tasks[idx].description = sanitized_desc;

        // Parse role with validation
        cJSON* role_obj = cJSON_GetObjectItem(task_obj, "role");
        if (cJSON_IsString(role_obj)) {
            tasks[idx].required_role = validate_role_string(cJSON_GetStringValue(role_obj));
        } else {
            tasks[idx].required_role = AGENT_ROLE_EXECUTOR;
        }

        // Parse prerequisites with validation
        cJSON* prereq_obj = cJSON_GetObjectItem(task_obj, "prerequisites");
        if (cJSON_IsArray(prereq_obj)) {
            int prereq_size = cJSON_GetArraySize(prereq_obj);

            // Security: Limit prerequisites per task
            if (prereq_size > MAX_PREREQUISITES_PER_TASK) {
                prereq_size = MAX_PREREQUISITES_PER_TASK;
            }

            if (prereq_size > 0) {
                tasks[idx].prerequisite_capacity = (size_t)prereq_size;
                tasks[idx].prerequisite_ids = calloc((size_t)prereq_size, sizeof(uint64_t));
                if (tasks[idx].prerequisite_ids) {
                    for (int j = 0; j < prereq_size; j++) {
                        cJSON* prereq_item = cJSON_GetArrayItem(prereq_obj, j);
                        if (cJSON_IsNumber(prereq_item)) {
                            int prereq_idx = (int)cJSON_GetNumberValue(prereq_item);
                            // Security: Validate prerequisite index bounds
                            if (prereq_idx >= 0 && prereq_idx < array_size && prereq_idx != i) {
                                tasks[idx].prerequisite_ids[tasks[idx].prerequisite_count] =
                                    tasks[prereq_idx].task_id;
                                tasks[idx].prerequisite_count++;
                            }
                        }
                    }
                }
            }
        }

        // Parse validation criteria with sanitization
        cJSON* validation_obj = cJSON_GetObjectItem(task_obj, "validation");
        if (cJSON_IsString(validation_obj)) {
            tasks[idx].validation_criteria =
                sanitize_llm_string(cJSON_GetStringValue(validation_obj), MAX_VALIDATION_LENGTH);
        }
    }

    cJSON_Delete(json);

    // Return NULL if no valid tasks were parsed
    if (valid_task_count == 0) {
        free(tasks);
        return NULL;
    }

    *out_count = (size_t)valid_task_count;
    return tasks;
}

// Decompose goal into subtasks using LLM
DecomposedTask* task_decompose(const char* goal, AgentRole* roles, size_t role_count,
                               size_t* out_count) {
    if (!goal || !out_count) {
        return NULL;
    }

    *out_count = 0;

    // Security: Validate goal length to prevent excessive memory allocation
    size_t goal_len = strlen(goal);
    if (goal_len > 8192) { // Max 8KB goal
        return NULL;
    }

    // Build prompt for task decomposition with dynamic allocation
    const char* prompt_template =
        "Break down the following goal into actionable subtasks. "
        "Return a JSON object with a 'tasks' array. Each task should have:\n"
        "- 'description': clear task description\n"
        "- 'role': one of [analyst, coder, writer, critic, planner, executor]\n"
        "- 'prerequisites': array of task indices (0-based) that must complete first\n"
        "- 'validation': how to validate task completion\n\n"
        "Goal: %s\n\n"
        "Return only valid JSON, no markdown formatting.";

    size_t prompt_size = strlen(prompt_template) + goal_len + 1;
    char* prompt = malloc(prompt_size);
    if (!prompt) {
        return NULL;
    }
    // Suppress false positive: prompt_template is a controlled literal
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    snprintf(prompt, prompt_size, prompt_template, goal);
#pragma clang diagnostic pop

    // Use LLM facade to decompose
    if (!llm_is_available()) {
        free(prompt);
        return NULL;
    }

    const char* system_prompt =
        "You are a task decomposition expert. Break down complex goals into "
        "actionable subtasks with clear dependencies. Return only valid JSON.";

    TokenUsage usage = {0};
    char* response = llm_chat_with_model("claude-sonnet-4-20250514", system_prompt, prompt, &usage);

    // Free prompt after use
    free(prompt);
    prompt = NULL;

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
static bool has_cycle_dfs(const DecomposedTask* task, const DecomposedTask* all_tasks,
                          size_t task_count, bool* visited, bool* rec_stack) {
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
// TOPOLOGICAL SORT (Kahn's Algorithm) - Helper Functions
// ============================================================================

// Find task index by ID
static size_t find_task_index_by_id(DecomposedTask* tasks, size_t task_count, uint64_t task_id) {
    for (size_t i = 0; i < task_count; i++) {
        if (tasks[i].task_id == task_id) {
            return i;
        }
    }
    return SIZE_MAX;
}

// Calculate in-degrees for all tasks
static int calculate_in_degrees(DecomposedTask* tasks, size_t task_count, size_t* in_degree) {
    for (size_t i = 0; i < task_count; i++) {
        in_degree[i] = 0;
    }

    for (size_t i = 0; i < task_count; i++) {
        for (size_t j = 0; j < tasks[i].prerequisite_count; j++) {
            uint64_t prereq_id = tasks[i].prerequisite_ids[j];
            size_t prereq_idx = find_task_index_by_id(tasks, task_count, prereq_id);
            if (prereq_idx != SIZE_MAX) {
                in_degree[prereq_idx]++;
            }
        }
    }

    return 0;
}

// Initialize queue with tasks that have no prerequisites
static int initialize_queue(DecomposedTask* tasks, size_t task_count, size_t* in_degree,
                            uint64_t* queue, size_t* queue_back) {
    *queue_back = 0;
    for (size_t i = 0; i < task_count; i++) {
        if (in_degree[i] == 0) {
            queue[(*queue_back)++] = tasks[i].task_id;
        }
    }
    return 0;
}

// Reduce in-degree of tasks that depend on current task
static void reduce_dependent_in_degrees(DecomposedTask* tasks, size_t task_count,
                                        uint64_t current_id, size_t* in_degree, uint64_t* queue,
                                        size_t* queue_back) {
    for (size_t i = 0; i < task_count; i++) {
        // Check if this task depends on current_id
        bool depends_on_current = false;
        for (size_t j = 0; j < tasks[i].prerequisite_count; j++) {
            if (tasks[i].prerequisite_ids[j] == current_id) {
                depends_on_current = true;
                break;
            }
        }

        if (depends_on_current && in_degree[i] > 0) {
            in_degree[i]--;
            if (in_degree[i] == 0) {
                queue[(*queue_back)++] = tasks[i].task_id;
            }
        }
    }
}

// ============================================================================
// TOPOLOGICAL SORT (Kahn's Algorithm)
// ============================================================================

int task_topological_sort(DecomposedTask* tasks, size_t task_count, uint64_t* sorted_ids,
                          size_t* out_count) {
    if (!tasks || !sorted_ids || !out_count || task_count == 0) {
        return -1;
    }

    *out_count = 0;

    // Allocate in-degree array
    size_t* in_degree = calloc(task_count, sizeof(size_t));
    if (!in_degree) {
        return -1;
    }

    // Calculate in-degrees
    if (calculate_in_degrees(tasks, task_count, in_degree) != 0) {
        free(in_degree);
        return -1;
    }

    // Initialize queue with tasks that have no prerequisites
    uint64_t* queue = calloc(task_count, sizeof(uint64_t));
    size_t queue_front = 0;
    size_t queue_back = 0;

    if (!queue) {
        free(in_degree);
        return -1;
    }

    initialize_queue(tasks, task_count, in_degree, queue, &queue_back);

    // Process queue
    while (queue_front < queue_back) {
        uint64_t current_id = queue[queue_front++];
        sorted_ids[(*out_count)++] = current_id;

        // Reduce in-degree of tasks that depend on current
        reduce_dependent_in_degrees(tasks, task_count, current_id, in_degree, queue, &queue_back);
    }

    free(in_degree);
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

bool task_prerequisites_met(const DecomposedTask* task, const DecomposedTask* all_tasks,
                            size_t task_count) {
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
        task->result = workflow_strdup(result);
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
        task->result = workflow_strdup(error);
    }

    return 0;
}

// ============================================================================
// PARALLEL EXECUTION
// ============================================================================

// Execute a single task via agent
static int task_execute_via_agent(DecomposedTask* task) {
    if (!task || !task->description) {
        return -1;
    }

    // Find or spawn agent for the required role
    ManagedAgent* agent = agent_find_by_role(task->required_role);

    if (!agent) {
        // Spawn new agent for this role
        const char* role_names[] = {"analyst", "coder", "writer", "critic", "planner", "executor"};
        const char* role_name =
            (task->required_role < 6) ? role_names[task->required_role] : "executor";

        char agent_name[128];
        snprintf(agent_name, sizeof(agent_name), "%s-agent", role_name);

        agent = agent_spawn(task->required_role, agent_name,
                            "Execute the assigned task efficiently and accurately.");

        if (!agent) {
            task_mark_failed(task, "Failed to spawn agent for task execution");
            return -1;
        }
    }

    // Check LLM availability via facade
    if (!llm_is_available()) {
        task_mark_failed(task, "LLM not available for task execution");
        return -1;
    }

    // Build task prompt with dynamic allocation (security: prevent buffer overflow)
    const char* validation_text = task->validation_criteria
                                      ? task->validation_criteria
                                      : "Ensure the task is completed successfully.";
    size_t desc_len = task->description ? strlen(task->description) : 0;
    size_t validation_len = strlen(validation_text);

    // Security: Validate lengths to prevent excessive memory allocation
    if (desc_len > 16384 || validation_len > 4096) {
        task_mark_failed(task, "Task description or validation too long");
        return -1;
    }

    const char* prompt_template =
        "Task: %s\n\nPlease execute this task and provide a clear result. %s";
    size_t prompt_size = strlen(prompt_template) + desc_len + validation_len + 1;
    char* task_prompt = malloc(prompt_size);
    if (!task_prompt) {
        task_mark_failed(task, "Memory allocation failed for task prompt");
        return -1;
    }
    // Suppress false positive: prompt_template is a controlled literal
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    snprintf(task_prompt, prompt_size, prompt_template, task->description, validation_text);
#pragma clang diagnostic pop

    // Execute task via LLM facade
    TokenUsage usage = {0};
    char* response = llm_chat_with_model(
        "claude-sonnet-4-20250514",
        agent->system_prompt ? agent->system_prompt : "You are a helpful assistant.", task_prompt,
        &usage);

    // Free task prompt after use
    free(task_prompt);
    task_prompt = NULL;

    if (!response) {
        task_mark_failed(task, "Agent execution failed: no response");
        return -1;
    }

    // Record cost
    if (agent) {
        cost_record_agent_usage(agent, usage.input_tokens, usage.output_tokens);
    }

    // Mark task as completed with result
    task->assigned_agent_id = agent->id;
    task_mark_completed(task, response);

    // Free response (task_mark_completed makes a copy)
    free(response);

    return 0;
}

int task_execute_parallel(DecomposedTask* tasks, size_t task_count, dispatch_group_t group) {
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

            // Execute task via agent
            int result = task_execute_via_agent(task);

            if (result != 0) {
                // Error already logged by task_execute_via_agent
                // Task is marked as failed
            }
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
