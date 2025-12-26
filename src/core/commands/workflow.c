/**
 * CONVERGIO WORKFLOW COMMANDS
 *
 * CLI commands for workflow management
 * /workflow list, /workflow execute, /workflow resume, etc.
 */

#include "nous/workflow.h"
#include "nous/commands.h"
#include "nous/nous.h"
#include "nous/orchestrator.h"
#include "nous/workflow_visualization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// WORKFLOW REGISTRY - Simple in-memory workflow storage
// ============================================================================

#define MAX_REGISTERED_WORKFLOWS 64

typedef struct {
    Workflow* workflows[MAX_REGISTERED_WORKFLOWS];
    size_t count;
    bool initialized;
} WorkflowRegistry;

static WorkflowRegistry g_workflow_registry = {0};

// Initialize registry with built-in workflow templates
// Note: This is called lazily when workflows are accessed, ensuring agents are loaded
static void workflow_registry_init(void) {
    if (g_workflow_registry.initialized) {
        return;
    }
    g_workflow_registry.initialized = true;
    g_workflow_registry.count = 0;

    // Register built-in workflow templates
    extern Workflow* pattern_create_review_refine_loop(SemanticID, SemanticID, SemanticID, int);

    // Try to find agents, spawning them if needed (lazy initialization)
    ManagedAgent* rex = agent_find_by_name("rex");
    if (!rex) rex = agent_spawn(AGENT_ROLE_ANALYST, "rex", NULL);

    ManagedAgent* paolo = agent_find_by_name("paolo");
    if (!paolo) paolo = agent_spawn(AGENT_ROLE_ANALYST, "paolo", NULL);

    ManagedAgent* baccio = agent_find_by_name("baccio");
    if (!baccio) baccio = agent_spawn(AGENT_ROLE_ANALYST, "baccio", NULL);

    // Create a simple code-review workflow template
    if (rex && paolo) {
        Workflow* code_review = pattern_create_review_refine_loop(
            rex->id, paolo->id, rex->id, 3);
        if (code_review) {
            if (code_review->name) free(code_review->name);
            code_review->name = strdup("code-review");
            if (code_review->description) free(code_review->description);
            code_review->description = strdup("Code review workflow: Rex reviews, Paolo checks best practices");
            code_review->workflow_id = 1;
            if (g_workflow_registry.count < MAX_REGISTERED_WORKFLOWS) {
                g_workflow_registry.workflows[g_workflow_registry.count++] = code_review;
            }
        }
    }

    // Create architecture-review workflow
    if (baccio && rex) {
        Workflow* arch_review = pattern_create_review_refine_loop(
            baccio->id, rex->id, baccio->id, 2);
        if (arch_review) {
            if (arch_review->name) free(arch_review->name);
            arch_review->name = strdup("architecture-review");
            if (arch_review->description) free(arch_review->description);
            arch_review->description = strdup("Architecture review: Baccio designs, Rex reviews for quality");
            arch_review->workflow_id = 2;
            if (g_workflow_registry.count < MAX_REGISTERED_WORKFLOWS) {
                g_workflow_registry.workflows[g_workflow_registry.count++] = arch_review;
            }
        }
    }
}

// Register a workflow in the registry
int workflow_register(Workflow* wf) {
    if (!wf) return -1;
    workflow_registry_init();
    if (g_workflow_registry.count >= MAX_REGISTERED_WORKFLOWS) {
        return -1; // Registry full
    }
    // Assign ID if not set
    if (wf->workflow_id == 0) {
        wf->workflow_id = (uint64_t)(g_workflow_registry.count + 1);
    }
    g_workflow_registry.workflows[g_workflow_registry.count++] = wf;
    return 0;
}

// Load workflow by name from registry
static Workflow* workflow_load_by_name(const char* name) {
    if (!name) return NULL;
    workflow_registry_init();
    for (size_t i = 0; i < g_workflow_registry.count; i++) {
        Workflow* wf = g_workflow_registry.workflows[i];
        if (wf && wf->name && strcmp(wf->name, name) == 0) {
            return wf;
        }
    }
    return NULL;
}

// Load workflow by ID from registry
static Workflow* workflow_load_by_id(uint64_t id) {
    if (id == 0) return NULL;
    workflow_registry_init();
    for (size_t i = 0; i < g_workflow_registry.count; i++) {
        Workflow* wf = g_workflow_registry.workflows[i];
        if (wf && wf->workflow_id == id) {
            return wf;
        }
    }
    return NULL;
}

// Get all registered workflows
size_t workflow_get_all(Workflow** out, size_t max) {
    workflow_registry_init();
    size_t count = 0;
    for (size_t i = 0; i < g_workflow_registry.count && count < max; i++) {
        if (g_workflow_registry.workflows[i]) {
            out[count++] = g_workflow_registry.workflows[i];
        }
    }
    return count;
}

// Export workflow as Mermaid diagram - now implemented in workflow_visualization.c

// ============================================================================
// WORKFLOW LIST
// ============================================================================

static int cmd_workflow_list(int argc, char** argv) {
    (void)argc;
    (void)argv;

    workflow_registry_init();

    printf("Available workflows:\n\n");

    if (g_workflow_registry.count == 0) {
        printf("  (No workflows registered)\n");
        printf("  Built-in workflows will be available when agents are loaded.\n");
    } else {
        for (size_t i = 0; i < g_workflow_registry.count; i++) {
            Workflow* wf = g_workflow_registry.workflows[i];
            if (wf) {
                printf("  [%llu] %s\n", (unsigned long long)wf->workflow_id,
                       wf->name ? wf->name : "unnamed");
                if (wf->description) {
                    printf("      %s\n", wf->description);
                }
                printf("\n");
            }
        }
    }

    printf("Use /workflow execute <name> to run a workflow\n");
    printf("Use /workflow show <name> to view workflow details\n");

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

    // Note: Don't destroy workflow from registry, it's shared
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
        printf("Use /workflow list to see available workflows.\n");
        return 1;
    }

    printf("Executing workflow: %s\n", wf->name ? wf->name : name);
    if (wf->description) {
        printf("  %s\n\n", wf->description);
    }

    char* output = NULL;
    int result = workflow_execute(wf, input, &output);

    if (result == 0) {
        printf("\n--- Workflow Output ---\n");
        if (output) {
            printf("%s\n", output);
            free(output);
            output = NULL;
        } else {
            printf("Workflow completed successfully.\n");
        }
    } else {
        printf("\nWorkflow execution failed");
        if (wf->error_message) {
            printf(": %s\n", wf->error_message);
        } else {
            printf(".\n");
        }
    }

    // Note: Don't destroy workflow from registry, it's shared
    // Reset workflow status for next execution
    wf->status = WORKFLOW_STATUS_PENDING;
    wf->current_node_id = 0;

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
            printf("Failed to restore from checkpoint %llu\n", (unsigned long long)checkpoint_id);
            workflow_destroy(wf);
            return 1;
        }
        printf("Restored workflow from checkpoint %llu\n", (unsigned long long)checkpoint_id);
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
