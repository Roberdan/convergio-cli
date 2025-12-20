/**
 * CONVERGIO WORKFLOW VISUALIZATION
 *
 * Mermaid diagram export for workflow visualization
 * Supports export to Mermaid format for rendering in documentation and tools
 */

#ifndef CONVERGIO_WORKFLOW_VISUALIZATION_H
#define CONVERGIO_WORKFLOW_VISUALIZATION_H

#include "nous/workflow.h"
#include <stdbool.h>

// ============================================================================
// MERMAID EXPORT
// ============================================================================

/**
 * @brief Export workflow to Mermaid diagram format
 * @param wf The workflow to export
 * @param output Buffer to write Mermaid diagram (must be allocated by caller)
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on failure
 * 
 * Output format: Mermaid flowchart syntax
 * Example:
 * ```mermaid
 * flowchart TD
 *   A[Start] --> B[Action Node]
 *   B --> C{Decision}
 *   C -->|Yes| D[End]
 *   C -->|No| B
 * ```
 */
int workflow_export_mermaid(const Workflow* wf, char* output, size_t output_size);

/**
 * @brief Export workflow to Mermaid diagram format (allocates buffer)
 * @param wf The workflow to export
 * @return Allocated string with Mermaid diagram, or NULL on failure
 * @note Caller must free() the returned string
 */
char* workflow_export_mermaid_alloc(const Workflow* wf);

/**
 * @brief Get node type name for Mermaid
 * @param node_type Node type enum
 * @return String representation for Mermaid diagram
 */
const char* workflow_mermaid_node_type_name(NodeType node_type);

/**
 * @brief Get node shape for Mermaid based on type
 * @param node_type Node type enum
 * @return Mermaid shape string (e.g., "[Node]", "{Decision}", "([Human])")
 */
const char* workflow_mermaid_node_shape(NodeType node_type);

#endif // CONVERGIO_WORKFLOW_VISUALIZATION_H

