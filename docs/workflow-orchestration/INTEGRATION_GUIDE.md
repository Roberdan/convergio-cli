# Integration Guide: Workflow Orchestration in Convergio

**Date**: 2025-12-18  
**Status**: Integration Plan

---

## Overview

This guide explains how to integrate workflow orchestration features into Convergio, including:
- Immediate integration (optional, from Phase 1)
- CLI commands for workflow management
- Documentation updates
- README updates
- Help system updates

---

## Immediate Integration (Optional, From Phase 1)

### Strategy: Progressive Enhancement

Workflow engine can be used **immediately** (from Phase 1) as an **optional enhancement**, without breaking existing functionality.

### Integration Points

#### 1. Ali Can Use Workflows Immediately (Phase 1+)

**In Ali's tool use or system prompt:**

```c
// Option: Ali decides to use workflow for complex tasks
bool should_use_workflow(const char* user_input) {
    // Heuristic: Use workflow for multi-step tasks
    if (strstr(user_input, "plan") || 
        strstr(user_input, "strategy") ||
        strstr(user_input, "architecture")) {
        return true;
    }
    return false;
}

// In orchestrator or Ali's response generation
if (should_use_workflow(user_input) && workflow_engine_available()) {
    // Use workflow engine (if available)
    Workflow* wf = workflow_load_by_name("strategic-planning");
    if (wf) {
        return workflow_execute(wf, user_input, NULL);
    }
}
// Fallback to existing orchestrator
return orchestrator_parallel_analyze(...);
```

**Implementation:**
- Add workflow check in `orchestrator.c` (optional, behind feature flag)
- Ali can suggest using workflows for complex tasks
- Zero breaking changes

#### 2. CLI Commands (Phase 1+)

**Add workflow commands immediately:**

```c
// In src/core/commands/commands.c
static const ReplCommand COMMANDS[] = {
    // ... existing commands ...
    {"workflow",     "Manage workflows",                    cmd_workflow},
    {"workflows",    "List available workflows",            cmd_workflows},
    {NULL, NULL, NULL}
};
```

**Commands to implement:**

```c
// src/core/commands/workflow.c
int cmd_workflow(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: workflow <subcommand> [args]\n");
        printf("Subcommands:\n");
        printf("  list              List all workflows\n");
        printf("  show <name>       Show workflow details\n");
        printf("  create <name>     Create new workflow\n");
        printf("  execute <name>    Execute a workflow\n");
        printf("  resume <id>       Resume workflow from checkpoint\n");
        printf("  checkpoint <id>   Show checkpoints for workflow\n");
        return 0;
    }
    
    const char* subcmd = argv[1];
    
    if (strcmp(subcmd, "list") == 0) {
        return cmd_workflow_list(argc - 1, argv + 1);
    } else if (strcmp(subcmd, "show") == 0) {
        return cmd_workflow_show(argc - 1, argv + 1);
    } else if (strcmp(subcmd, "create") == 0) {
        return cmd_workflow_create(argc - 1, argv + 1);
    } else if (strcmp(subcmd, "execute") == 0) {
        return cmd_workflow_execute(argc - 1, argv + 1);
    } else if (strcmp(subcmd, "resume") == 0) {
        return cmd_workflow_resume(argc - 1, argv + 1);
    } else if (strcmp(subcmd, "checkpoint") == 0) {
        return cmd_workflow_checkpoint(argc - 1, argv + 1);
    }
    
    printf("Unknown subcommand: %s\n", subcmd);
    return 1;
}
```

#### 3. Ali Tool Integration (Phase 1+)

**Add workflow tools to Ali's tool set:**

```c
// In Ali's tool definitions
static const ToolDef ALI_TOOLS[] = {
    // ... existing tools ...
    {
        "workflow_execute",
        "Execute a workflow by name",
        "{\"type\": \"object\", \"properties\": {\"workflow_name\": {\"type\": \"string\"}, \"input\": {\"type\": \"string\"}}}",
        tool_workflow_execute
    },
    {
        "workflow_create",
        "Create a new workflow",
        "{\"type\": \"object\", \"properties\": {\"name\": {\"type\": \"string\"}, \"description\": {\"type\": \"string\"}}}",
        tool_workflow_create
    },
    {NULL, NULL, NULL, NULL}
};
```

**Ali can now:**
- Suggest using workflows for complex tasks
- Create workflows on the fly
- Execute workflows as part of multi-step plans

---

## CLI Commands Implementation

### Phase 1: Basic Commands

**File**: `src/core/commands/workflow.c`

```c
// List all workflows
int cmd_workflow_list(int argc, char** argv) {
    Workflow** workflows = NULL;
    size_t count = 0;
    
    if (workflow_list(&workflows, &count) != 0) {
        printf("Error listing workflows\n");
        return 1;
    }
    
    printf("Available workflows:\n");
    for (size_t i = 0; i < count; i++) {
        printf("  %s - %s\n", workflows[i]->name, 
               workflows[i]->description ? workflows[i]->description : "");
    }
    
    // Free workflows
    for (size_t i = 0; i < count; i++) {
        workflow_destroy(workflows[i]);
    }
    free(workflows);
    
    return 0;
}

// Show workflow details
int cmd_workflow_show(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: workflow show <name>\n");
        return 1;
    }
    
    Workflow* wf = workflow_load_by_name(argv[1]);
    if (!wf) {
        printf("Workflow '%s' not found\n", argv[1]);
        return 1;
    }
    
    printf("Workflow: %s\n", wf->name);
    printf("Description: %s\n", wf->description ? wf->description : "None");
    printf("Status: %s\n", workflow_status_to_string(wf->status));
    printf("Current node: %s\n", wf->current_node_id > 0 ? "Yes" : "No");
    
    // Export as Mermaid diagram
    char* mermaid = workflow_export_mermaid(wf);
    if (mermaid) {
        printf("\nWorkflow diagram:\n```mermaid\n%s\n```\n", mermaid);
        free(mermaid);
    }
    
    workflow_destroy(wf);
    return 0;
}

// Execute workflow
int cmd_workflow_execute(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: workflow execute <name> [input]\n");
        return 1;
    }
    
    Workflow* wf = workflow_load_by_name(argv[1]);
    if (!wf) {
        printf("Workflow '%s' not found\n", argv[1]);
        return 1;
    }
    
    const char* input = argc > 2 ? argv[2] : "";
    char* output = NULL;
    
    int result = workflow_execute(wf, input, &output);
    if (result == 0 && output) {
        printf("%s\n", output);
        free(output);
    } else {
        printf("Workflow execution failed: %s\n", 
               wf->error_message ? wf->error_message : "Unknown error");
    }
    
    workflow_destroy(wf);
    return result;
}

// Resume from checkpoint
int cmd_workflow_resume(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: workflow resume <workflow_id> [checkpoint_id]\n");
        return 1;
    }
    
    uint64_t workflow_id = strtoull(argv[1], NULL, 10);
    if (workflow_id == 0) {
        printf("Invalid workflow ID\n");
        return 1;
    }
    
    Workflow* wf = workflow_load(workflow_id);
    if (!wf) {
        printf("Workflow %llu not found\n", workflow_id);
        return 1;
    }
    
    uint64_t checkpoint_id = 0;
    if (argc > 2) {
        checkpoint_id = strtoull(argv[2], NULL, 10);
    } else {
        // Use latest checkpoint
        Checkpoint* checkpoints = NULL;
        size_t count = 0;
        workflow_list_checkpoints(wf, &checkpoints, &count);
        if (count > 0) {
            checkpoint_id = checkpoints[count - 1]->id;
        }
        free(checkpoints);
    }
    
    if (checkpoint_id == 0) {
        printf("No checkpoint found\n");
        workflow_destroy(wf);
        return 1;
    }
    
    int result = workflow_restore_from_checkpoint(wf, checkpoint_id);
    if (result == 0) {
        char* output = NULL;
        result = workflow_execute(wf, NULL, &output);
        if (output) {
            printf("%s\n", output);
            free(output);
        }
    } else {
        printf("Failed to restore checkpoint\n");
    }
    
    workflow_destroy(wf);
    return result;
}
```

### Phase 4: Advanced Commands

**Add pattern commands:**

```c
// Create workflow from pattern
int cmd_workflow_pattern(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: workflow pattern <pattern_name> <workflow_name> [args]\n");
        printf("Available patterns:\n");
        printf("  review-refine      Review-Refine Loop\n");
        printf("  parallel-analysis  Parallel Analysis\n");
        printf("  sequential-plan    Sequential Planning\n");
        printf("  consensus          Consensus Building\n");
        return 1;
    }
    
    const char* pattern_name = argv[1];
    const char* workflow_name = argv[2];
    
    Workflow* wf = NULL;
    
    if (strcmp(pattern_name, "review-refine") == 0) {
        // Create review-refine pattern
        SemanticID generator = agent_find_by_name("baccio")->id;
        SemanticID critic = agent_find_by_name("thor")->id;
        SemanticID refiner = agent_find_by_name("baccio")->id;
        wf = pattern_create_review_refine_loop(generator, critic, refiner, 5);
    } else if (strcmp(pattern_name, "parallel-analysis") == 0) {
        // Create parallel analysis pattern
        SemanticID analysts[] = {
            agent_find_by_name("baccio")->id,
            agent_find_by_name("sofia")->id,
            agent_find_by_name("omri")->id
        };
        SemanticID converger = agent_find_by_name("ali")->id;
        wf = pattern_create_parallel_analysis(analysts, 3, converger);
    }
    // ... other patterns ...
    
    if (!wf) {
        printf("Unknown pattern: %s\n", pattern_name);
        return 1;
    }
    
    // Set workflow name
    workflow_set_name(wf, workflow_name);
    
    // Save workflow
    workflow_save(wf);
    printf("Created workflow '%s' from pattern '%s'\n", workflow_name, pattern_name);
    
    workflow_destroy(wf);
    return 0;
}
```

---

## Help System Updates

### Add Workflow Help

**File**: `src/core/commands/commands.c` (DETAILED_HELP array)

```c
static const CommandHelp DETAILED_HELP[] = {
    // ... existing help entries ...
    {
        "workflow",
        "workflow <subcommand> [args]",
        "Manage and execute workflows",
        "Workflows enable complex multi-step agent coordination with checkpointing.\n\n"
        "Subcommands:\n"
        "  list                    List all available workflows\n"
        "  show <name>             Show workflow details and diagram\n"
        "  create <name>           Create a new workflow (interactive)\n"
        "  execute <name> [input]  Execute a workflow\n"
        "  resume <id> [checkpoint] Resume workflow from checkpoint\n"
        "  checkpoint <id>         List checkpoints for workflow\n"
        "  pattern <pattern> <name> Create workflow from pattern\n\n"
        "Workflows can be used by Ali or any agent for complex multi-step tasks.",
        "workflow list\n"
        "workflow show strategic-planning\n"
        "workflow execute parallel-analysis \"Analyze this project\"\n"
        "workflow resume 12345\n"
        "workflow pattern review-refine code-review"
    },
    {
        "workflows",
        "workflows",
        "List all available workflows (alias for 'workflow list')",
        "Shows all workflows in the system, including:\n"
        "- Built-in patterns\n"
        "- User-created workflows\n"
        "- Workflow status (pending, running, completed)",
        "workflows"
    },
    // ... rest of help entries ...
};
```

---

## README Updates

### Add Workflow Section to README

**Location**: After "Features" section, before "Quick Start"

```markdown
## Advanced Workflow Orchestration

Convergio now supports **state machine-based workflows** for complex multi-step agent coordination.

### Key Features

- **Workflow Persistence**: Checkpointing enables resume from any point
- **Pattern Library**: Pre-built workflows for common scenarios
- **Conditional Routing**: Dynamic workflow adaptation based on results
- **Group Chat**: Multi-agent discussions with consensus building
- **Task Decomposition**: Automatic hierarchical task breakdown

### Quick Example

```bash
# List available workflows
> /workflow list

# Execute a workflow
> /workflow execute parallel-analysis "Analyze this project"

# Resume from checkpoint
> /workflow resume 12345

# Create workflow from pattern
> /workflow pattern review-refine code-review
```

### Workflow Patterns

| Pattern | Description | Use Case |
|---------|-------------|----------|
| **Review-Refine Loop** | Generator → Critic → Refiner → Validator | Code review, content creation |
| **Parallel Analysis** | Multiple agents analyze → Convergence | Project analysis, strategy |
| **Sequential Planning** | Strategy → Plan → Architecture → Implementation | Product development |
| **Consensus Building** | Discussion → Voting → Consensus → Action | Decision making |

### Ali Integration

Ali can automatically use workflows for complex tasks:

```
> Launch a global SaaS platform

Ali: "This is a complex multi-step task. I'll use the strategic-planning workflow:
     1. Domik (Strategy) → 2. Amy (Finance) → 3. Baccio (Tech) → 4. Convergence"
```

See [Workflow Documentation](docs/workflow-orchestration/MASTER_PLAN.md) for details.
```

### Update Features Table

**In README Features section, add:**

```markdown
### Advanced Workflow Orchestration

- **State Machine Execution**: Workflows defined as graphs, not hardcoded
- **Checkpointing**: Resume from any point in workflow execution
- **Pattern Library**: Reusable workflow templates
- **Conditional Routing**: Dynamic workflow adaptation
- **Task Decomposition**: Automatic hierarchical task breakdown
- **Group Chat**: Multi-agent discussions with consensus
```

---

## Documentation Updates

### Create Workflow User Guide

**File**: `docs/workflow-orchestration/USER_GUIDE.md`

```markdown
# Workflow Orchestration User Guide

## Introduction

Workflow orchestration enables complex multi-step agent coordination with persistence and recovery.

## Basic Usage

### Listing Workflows

```bash
> /workflow list
Available workflows:
  strategic-planning    - Multi-step strategic planning
  parallel-analysis    - Parallel agent analysis
  review-refine        - Review and refinement loop
```

### Executing a Workflow

```bash
> /workflow execute parallel-analysis "Analyze this project"
```

### Resuming from Checkpoint

```bash
> /workflow resume 12345
```

## Workflow Patterns

### Review-Refine Loop

Perfect for code review or content creation:

```bash
> /workflow pattern review-refine code-review
Created workflow 'code-review'

> /workflow execute code-review "Review this code"
```

### Parallel Analysis

Multiple agents analyze in parallel, then converge:

```bash
> /workflow pattern parallel-analysis project-analysis
Created workflow 'project-analysis'

> /workflow execute project-analysis "Analyze this project"
```

## Ali Integration

Ali automatically suggests workflows for complex tasks:

```
> Plan a product launch

Ali: "This requires multiple steps. I'll use the strategic-planning workflow:
     - Domik (Strategy)
     - Amy (Finance)  
     - Baccio (Tech)
     - Convergence"
```

## Advanced Features

### Creating Custom Workflows

Workflows can be created programmatically or via CLI (future).

### Checkpoint Management

Workflows automatically create checkpoints. Resume from any point:

```bash
> /workflow checkpoint 12345
Checkpoints for workflow 12345:
  1. 2025-12-18 10:00 - strategy-complete
  2. 2025-12-18 10:05 - finance-complete
  3. 2025-12-18 10:10 - tech-complete (current)

> /workflow resume 12345 2
Resuming from checkpoint 2...
```

## Troubleshooting

### Workflow Not Found

```bash
> /workflow execute unknown-workflow
Workflow 'unknown-workflow' not found

> /workflow list  # Check available workflows
```

### Workflow Execution Failed

```bash
> /workflow execute my-workflow
Workflow execution failed: Agent 'baccio' not available

# Check agent availability
> /agents
```

## See Also

- [Master Plan](MASTER_PLAN.md) - Implementation details
- [Architecture](architecture.md) - System architecture
- [Patterns](PATTERNS.md) - Available patterns
```

### Update Main Documentation Index

**File**: `docs/README.md` (if exists) or add to main README

```markdown
## Documentation

- [Workflow Orchestration](workflow-orchestration/MASTER_PLAN.md) - Advanced workflow system
- [Workflow User Guide](workflow-orchestration/USER_GUIDE.md) - How to use workflows
- [Workflow Patterns](workflow-orchestration/PATTERNS.md) - Available patterns
```

---

## Integration Checklist

### Phase 1: Basic Integration

- [ ] Add workflow commands to `commands.c`
- [ ] Implement `cmd_workflow()` and subcommands
- [ ] Add workflow help entries
- [ ] Update README with workflow section
- [ ] Create user guide

### Phase 2-3: Enhanced Integration

- [ ] Add workflow tools to Ali's tool set
- [ ] Implement pattern commands
- [ ] Add workflow visualization (Mermaid export)

### Phase 4: Advanced Features

- [ ] Interactive workflow creation
- [ ] Workflow templates
- [ ] Workflow sharing

### Phase 5: Full Integration

- [ ] Migrate existing patterns to workflows
- [ ] Update all documentation
- [ ] Create migration guide
- [ ] Update CHANGELOG.md

---

## Example: Immediate Usage (Phase 1)

**User can use workflows immediately after Phase 1:**

```bash
convergio
> /workflow list
Available workflows:
  (No workflows yet - create one or use a pattern)

> /workflow create test-workflow
Creating workflow 'test-workflow'...
Workflow created successfully.

> /workflow execute test-workflow "Test input"
Executing workflow...
Workflow completed successfully.
```

**Ali can suggest workflows:**

```
> Plan a product launch

Ali: "This is a complex multi-step task. Would you like me to:
     1. Use the existing orchestrator (fast, simple)
     2. Use a workflow (checkpointing, recovery, more features)
     
     I recommend option 2 for this complex task."
```

---

## Conclusion

Workflow orchestration integrates seamlessly into Convergio:
- **Immediate availability** (Phase 1+)
- **Optional usage** (no breaking changes)
- **Progressive enhancement** (gradual adoption)
- **Full backward compatibility**







