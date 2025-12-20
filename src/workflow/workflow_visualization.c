/**
 * CONVERGIO WORKFLOW VISUALIZATION
 *
 * Mermaid diagram export for workflow visualization
 * Exports workflows to Mermaid flowchart format for documentation and tools
 */

#include "nous/workflow_visualization.h"
#include "nous/workflow.h"
#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Sanitize node name for Mermaid (remove special chars)
 */
static void sanitize_mermaid_name(const char* name, char* output, size_t output_size) {
    if (!name || !output || output_size == 0) {
        if (output && output_size > 0) {
            output[0] = '\0';
        }
        return;
    }
    
    size_t len = strlen(name);
    size_t j = 0;
    for (size_t i = 0; i < len && j < output_size - 1; i++) {
        char c = name[i];
        // Allow alphanumeric, spaces, underscores, hyphens
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == ' ' || c == '_' || c == '-') {
            output[j++] = c;
        } else if (c == '\n' || c == '\r') {
            output[j++] = ' '; // Replace newlines with spaces
        }
        // Skip other special characters
    }
    output[j] = '\0';
}

/**
 * @brief Get node ID string for Mermaid (sanitized)
 */
static void get_node_id(const WorkflowNode* node, char* output, size_t output_size) {
    if (!node || !output || output_size == 0) {
        if (output && output_size > 0) {
            output[0] = '\0';
        }
        return;
    }
    
    // Use node_id as base, sanitize name for label
    char sanitized[256];
    sanitize_mermaid_name(node->name, sanitized, sizeof(sanitized));
    
    // Create ID: N<node_id> (e.g., N1, N2)
    snprintf(output, output_size, "N%llu", (unsigned long long)node->node_id);
}

/**
 * @brief Get node label for Mermaid (sanitized name)
 */
static void get_node_label(const WorkflowNode* node, char* output, size_t output_size) {
    if (!node || !output || output_size == 0) {
        if (output && output_size > 0) {
            output[0] = '\0';
        }
        return;
    }
    
    sanitize_mermaid_name(node->name, output, output_size);
}

// ============================================================================
// NODE TYPE HELPERS
// ============================================================================

const char* workflow_mermaid_node_type_name(NodeType node_type) {
    switch (node_type) {
        case NODE_TYPE_ACTION:
            return "Action";
        case NODE_TYPE_DECISION:
            return "Decision";
        case NODE_TYPE_HUMAN_INPUT:
            return "Human Input";
        case NODE_TYPE_SUBGRAPH:
            return "Subgraph";
        case NODE_TYPE_PARALLEL:
            return "Parallel";
        case NODE_TYPE_CONVERGE:
            return "Converge";
        default:
            return "Unknown";
    }
}

const char* workflow_mermaid_node_shape(NodeType node_type) {
    switch (node_type) {
        case NODE_TYPE_ACTION:
            return "["; // [Node]
        case NODE_TYPE_DECISION:
            return "{"; // {Decision}
        case NODE_TYPE_HUMAN_INPUT:
            return "(["; // ([Human])
        case NODE_TYPE_SUBGRAPH:
            return "[["; // [[Subgraph]]
        case NODE_TYPE_PARALLEL:
            return "(["; // ([Parallel])
        case NODE_TYPE_CONVERGE:
            return "(["; // ([Converge])
        default:
            return "[";
    }
}

/**
 * @brief Get closing shape for Mermaid node
 */
static const char* get_mermaid_node_shape_close(NodeType node_type) {
    switch (node_type) {
        case NODE_TYPE_ACTION:
            return "]";
        case NODE_TYPE_DECISION:
            return "}";
        case NODE_TYPE_HUMAN_INPUT:
            return "])";
        case NODE_TYPE_SUBGRAPH:
            return "]]";
        case NODE_TYPE_PARALLEL:
            return "])";
        case NODE_TYPE_CONVERGE:
            return "])";
        default:
            return "]";
    }
}

// ============================================================================
// MERMAID EXPORT
// ============================================================================

/**
 * @brief Recursively collect all nodes from workflow starting from entry
 */
static void collect_nodes(WorkflowNode* node, WorkflowNode** collected, size_t* count, size_t capacity) {
    if (!node || !collected || !count || *count >= capacity) {
        return;
    }
    
    // Check if already collected
    for (size_t i = 0; i < *count; i++) {
        if (collected[i] == node) {
            return; // Already collected
        }
    }
    
    // Add this node
    collected[*count] = node;
    (*count)++;
    
    // Recursively collect next nodes
    for (size_t i = 0; i < node->next_node_count; i++) {
        if (node->next_nodes[i]) {
            collect_nodes(node->next_nodes[i], collected, count, capacity);
        }
    }
    
    // Collect fallback node if present
    if (node->fallback_node) {
        collect_nodes(node->fallback_node, collected, count, capacity);
    }
}

int workflow_export_mermaid(const Workflow* wf, char* output, size_t output_size) {
    if (!wf || !output || output_size < 100) {
        return -1;
    }
    
    if (!wf->entry_node) {
        snprintf(output, output_size, "flowchart TD\n  Start[No Entry Node]\n");
        return 0;
    }
    
    // Collect all nodes
    WorkflowNode* collected[256];
    size_t node_count = 0;
    collect_nodes((WorkflowNode*)wf->entry_node, collected, &node_count, 256);
    
    // Start building Mermaid diagram
    size_t pos = 0;
    int written = snprintf(output + pos, output_size - pos, "flowchart TD\n");
    if (written < 0 || (size_t)written >= output_size - pos) {
        return -1;
    }
    pos += written;
    
    // Define all nodes
    for (size_t i = 0; i < node_count; i++) {
        WorkflowNode* node = collected[i];
        if (!node) continue;
        
        char node_id[64];
        char node_label[256];
        get_node_id(node, node_id, sizeof(node_id));
        get_node_label(node, node_label, sizeof(node_label));
        
        const char* shape_open = workflow_mermaid_node_shape(node->type);
        const char* shape_close = get_mermaid_node_shape_close(node->type);
        
        written = snprintf(output + pos, output_size - pos, "  %s%s%s%s\n",
                          node_id, shape_open, node_label, shape_close);
        if (written < 0 || (size_t)written >= output_size - pos) {
            return -1;
        }
        pos += written;
    }
    
    // Define edges
    for (size_t i = 0; i < node_count; i++) {
        WorkflowNode* node = collected[i];
        if (!node) continue;
        
        char from_id[64];
        get_node_id(node, from_id, sizeof(from_id));
        
        // Regular next nodes
        for (size_t j = 0; j < node->next_node_count; j++) {
            WorkflowNode* next = node->next_nodes[j];
            if (!next) continue;
            
            char to_id[64];
            get_node_label(next, to_id, sizeof(to_id));
            get_node_id(next, to_id, sizeof(to_id));
            
            // Add edge with optional condition
            if (node->condition_expr && strlen(node->condition_expr) > 0) {
                char condition[128];
                sanitize_mermaid_name(node->condition_expr, condition, sizeof(condition));
                written = snprintf(output + pos, output_size - pos, "  %s -->|%s| %s\n",
                                  from_id, condition, to_id);
            } else {
                written = snprintf(output + pos, output_size - pos, "  %s --> %s\n",
                                  from_id, to_id);
            }
            
            if (written < 0 || (size_t)written >= output_size - pos) {
                return -1;
            }
            pos += written;
        }
        
        // Fallback node
        if (node->fallback_node) {
            char fallback_id[64];
            get_node_id(node->fallback_node, fallback_id, sizeof(fallback_id));
            written = snprintf(output + pos, output_size - pos, "  %s -->|fallback| %s\n",
                              from_id, fallback_id);
            if (written < 0 || (size_t)written >= output_size - pos) {
                return -1;
            }
            pos += written;
        }
    }
    
    // Mark entry node
    char entry_id[64];
    get_node_id((WorkflowNode*)wf->entry_node, entry_id, sizeof(entry_id));
    written = snprintf(output + pos, output_size - pos, "  Start([Start]) --> %s\n", entry_id);
    if (written < 0 || (size_t)written >= output_size - pos) {
        return -1;
    }
    pos += written;
    
    output[pos] = '\0';
    return 0;
}

char* workflow_export_mermaid_alloc(const Workflow* wf) {
    if (!wf) {
        return NULL;
    }
    
    // Estimate size: ~200 bytes per node + edges
    size_t estimated_size = 4096; // Start with reasonable default
    char* output = malloc(estimated_size);
    if (!output) {
        return NULL;
    }
    
    int result = workflow_export_mermaid(wf, output, estimated_size);
    if (result != 0) {
        free(output);
        return NULL;
    }
    
    // Reallocate to actual size
    size_t actual_size = strlen(output) + 1;
    char* resized = realloc(output, actual_size);
    if (resized) {
        return resized;
    }
    
    return output; // Return original if realloc fails
}

