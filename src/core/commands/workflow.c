/**
 * CONVERGIO WORKFLOW COMMANDS
 *
 * CLI commands for workflow management
 * /workflow list, /workflow execute, /workflow resume, etc.
 */

#include "nous/commands.h"
#include "nous/workflow.h"
#include "nous/workflow_visualization.h"
#include "nous/nous.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// STUB FUNCTIONS (to be implemented in workflow persistence layer)
// ============================================================================

// Load workflow by name (stub - persistence not yet implemented)
static Workflow* workflow_load_by_name(const char* name) {
    (void)name;
    return NULL; // Not yet implemented
}

// Load workflow by ID (stub - persistence not yet implemented)
static Workflow* workflow_load_by_id(uint64_t id) {
    (void)id;
    return NULL; // Not yet implemented
}

// Export workflow as Mermaid diagram - now implemented in workflow_visualization.c

// ============================================================================
// WORKFLOW LIST
// ============================================================================

static int cmd_workflow_list(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("Available workflows:\n");
    printf("  (Workflow persistence layer not yet implemented)\n");
    printf("  Use workflow templates from src/workflow/templates/\n");
    printf("\n");
    printf("Templates available:\n");
    printf("  - code-review.json\n");
    printf("  - product-launch.json\n");
    
    return 0;
}

// ============================================================================
// WORKFLOW SHOW
// ============================================================================

static int cmd_workflow_show(int argc, char** argv) {
    bool mermaid_only = false;
    const char* name = NULL;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mermaid") == 0 || strcmp(argv[i], "-m") == 0) {
            mermaid_only = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: /workflow show [--mermaid] <name>\n");
            printf("Show workflow details and Mermaid diagram\n");
            printf("\nOptions:\n");
            printf("  --mermaid, -m    Show only Mermaid diagram (no details)\n");
            return 0;
        } else if (!name) {
            name = argv[i];
        }
    }
    
    if (!name) {
        printf("Usage: /workflow show [--mermaid] <name>\n");
        printf("Show workflow details and Mermaid diagram\n");
        return 1;
    }
    
    // Try to load from database (when persistence is implemented)
    Workflow* wf = workflow_load_by_name(name);
    
    if (!wf) {
        printf("Workflow '%s' not found in database.\n", name);
        printf("(Persistence layer not yet fully implemented)\n");
        return 1;
    }
    
    // Export Mermaid
    char* mermaid = workflow_export_mermaid_alloc(wf);
    
    if (mermaid_only) {
        // Output only Mermaid diagram
        if (mermaid) {
            printf("%s\n", mermaid);
            free(mermaid);
        } else {
            printf("Error: Failed to generate Mermaid diagram\n");
            workflow_destroy(wf);
            return 1;
        }
    } else {
        // Show full details
        printf("Workflow: %s\n", wf->name ? wf->name : "Unknown");
        if (wf->description) {
            printf("Description: %s\n", wf->description);
        }
        printf("Status: %d\n", wf->status);
        printf("Current node ID: %llu\n", (unsigned long long)wf->current_node_id);
        
        if (mermaid) {
            printf("\nWorkflow diagram:\n```mermaid\n%s\n```\n", mermaid);
            free(mermaid);
        } else {
            printf("\n(Mermaid export failed)\n");
        }
    }
    
    workflow_destroy(wf);
    return 0;
}

// ============================================================================
// WORKFLOW EXECUTE
// ============================================================================

static int cmd_workflow_execute(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: /workflow execute <name> [input]\n");
        printf("Execute a workflow with optional input\n");
        return 1;
    }
    
    const char* name = argv[1];
    const char* input = argc > 2 ? argv[2] : "";
    
    // Try to load workflow
    Workflow* wf = workflow_load_by_name(name);
    
    if (!wf) {
        printf("Workflow '%s' not found.\n", name);
        printf("(Persistence layer not yet fully implemented)\n");
        printf("Use workflow_create() to create workflows programmatically.\n");
        return 1;
    }
    
    char* output = NULL;
    int result = workflow_execute(wf, input, &output);
    
    if (result == 0) {
        if (output) {
            printf("%s\n", output);
            free(output);
            output = NULL;
        } else {
            printf("Workflow completed successfully.\n");
        }
    } else {
        printf("Workflow execution failed");
        if (wf->error_message) {
            printf(": %s\n", wf->error_message);
        } else {
            printf(".\n");
        }
    }
    
    workflow_destroy(wf);
    return result;
}

// ============================================================================
// WORKFLOW RESUME
// ============================================================================

static int cmd_workflow_resume(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: /workflow resume <workflow_id> [checkpoint_id]\n");
        printf("Resume workflow from checkpoint\n");
        return 1;
    }
    
    uint64_t workflow_id = strtoull(argv[1], NULL, 10);
    if (workflow_id == 0) {
        printf("Invalid workflow ID: %s\n", argv[1]);
        return 1;
    }
    
    uint64_t checkpoint_id = 0;
    if (argc > 2) {
        checkpoint_id = strtoull(argv[2], NULL, 10);
    }
    
    // Load workflow
    Workflow* wf = workflow_load_by_id(workflow_id);
    if (!wf) {
        printf("Workflow ID %llu not found.\n", (unsigned long long)workflow_id);
        return 1;
    }
    
    // If checkpoint_id specified, restore from that checkpoint
    if (checkpoint_id > 0) {
        int result = workflow_restore_from_checkpoint(wf, checkpoint_id);
        if (result != 0) {
            printf("Failed to restore from checkpoint %llu\n", 
                   (unsigned long long)checkpoint_id);
            workflow_destroy(wf);
            return 1;
        }
        printf("Restored workflow from checkpoint %llu\n", 
               (unsigned long long)checkpoint_id);
    }
    
    // Resume execution
    char* output = NULL;
    int result = workflow_resume(wf, checkpoint_id);
    
    if (result == 0) {
        printf("Workflow resumed successfully.\n");
        if (output) {
            printf("%s\n", output);
            free(output);
            output = NULL;
        }
    } else {
        printf("Workflow resume failed");
        if (wf->error_message) {
            printf(": %s\n", wf->error_message);
        } else {
            printf(".\n");
        }
    }
    
    workflow_destroy(wf);
    return result;
}

// ============================================================================
// MAIN WORKFLOW COMMAND HANDLER
// ============================================================================

int cmd_workflow(int argc, char** argv) {
    if (argc < 2) {
        printf("Workflow management commands:\n");
        printf("  /workflow list              - List all workflows\n");
        printf("  /workflow show <name>       - Show workflow details\n");
        printf("  /workflow execute <name>    - Execute a workflow\n");
        printf("  /workflow resume <id>       - Resume from checkpoint\n");
        printf("\n");
        printf("Use /help workflow for detailed help\n");
        return 0;
    }
    
    const char* subcommand = argv[1];
    
    if (strcmp(subcommand, "list") == 0) {
        return cmd_workflow_list(argc - 1, argv + 1);
    } else if (strcmp(subcommand, "show") == 0) {
        return cmd_workflow_show(argc - 1, argv + 1);
    } else if (strcmp(subcommand, "execute") == 0) {
        return cmd_workflow_execute(argc - 1, argv + 1);
    } else if (strcmp(subcommand, "resume") == 0) {
        return cmd_workflow_resume(argc - 1, argv + 1);
    } else {
        printf("Unknown workflow subcommand: %s\n", subcommand);
        printf("Use /workflow list, show, execute, or resume\n");
        return 1;
    }
}

