# Workflow Orchestration Migration Guide

**Created**: 2025-12-20  
**Status**: Complete  
**Version**: 1.0.0

---

## Overview

This guide helps you migrate from the existing Convergio orchestrator to the new workflow orchestration system, or integrate workflows into your existing workflows.

---

## Migration Strategy

### Option 1: Gradual Migration (Recommended)

Migrate workflows incrementally, keeping existing orchestrator functionality:

1. **Start with Simple Workflows**: Use workflows for new, simple use cases
2. **Migrate Complex Patterns**: Gradually migrate complex orchestrator patterns to workflows
3. **Keep Orchestrator**: Maintain orchestrator for backward compatibility
4. **Deprecate Gradually**: Mark orchestrator functions as deprecated, migrate over time

### Option 2: Full Migration

Migrate all orchestrator patterns to workflows immediately:

1. **Identify Patterns**: List all orchestrator patterns in use
2. **Create Workflow Equivalents**: Create workflow versions of each pattern
3. **Update Code**: Update all code to use workflows
4. **Remove Orchestrator**: Remove orchestrator code (not recommended - breaks compatibility)

---

## Pattern Migration

### Pattern 1: Parallel Analysis

**Before (Orchestrator)**:
```c
// Orchestrator parallel analysis
ExecutionPlan* plan = orch_plan_create("Analyze code");
// ... parallel execution via orchestrator
```

**After (Workflow)**:
```c
// Workflow parallel analysis pattern
Workflow* wf = pattern_create_parallel_analysis(
    agent_ids,  // Array of agent IDs
    agent_count,
    "Analyze code from different perspectives"
);
workflow_execute(wf, code_input, &output);
```

### Pattern 2: Sequential Planning

**Before (Orchestrator)**:
```c
// Orchestrator sequential planning
ExecutionPlan* plan = orch_plan_create("Plan feature");
// ... sequential execution
```

**After (Workflow)**:
```c
// Workflow sequential planning pattern
Workflow* wf = pattern_create_sequential_planning(
    planner_id,
    executor_id,
    "Plan and execute feature"
);
workflow_execute(wf, feature_input, &output);
```

### Pattern 3: Review-Refine Loop

**Before (Orchestrator)**:
```c
// Manual review-refine loop
while (!satisfied) {
    result = generate_code();
    review = review_code(result);
    if (needs_refinement(review)) {
        refine_code(result, review);
    } else {
        satisfied = true;
    }
}
```

**After (Workflow)**:
```c
// Workflow review-refine loop pattern
Workflow* wf = pattern_create_review_refine_loop(
    generator_id,
    reviewer_id,
    max_iterations
);
workflow_execute(wf, initial_input, &output);
```

### Pattern 4: Consensus Building

**Before (Orchestrator)**:
```c
// Manual consensus building
GroupChat* chat = group_chat_create(agent_ids, agent_count);
// ... manual turn-taking and consensus checking
```

**After (Workflow)**:
```c
// Workflow consensus building pattern
Workflow* wf = pattern_create_consensus_building(
    agent_ids,
    agent_count,
    "Reach consensus on design decision"
);
workflow_execute(wf, design_input, &output);
```

---

## Backward Compatibility

### Existing Orchestrator Still Works

The workflow orchestration system is **fully backward compatible**:

- ✅ Existing orchestrator code continues to work
- ✅ No breaking changes to orchestrator API
- ✅ Workflows are opt-in (not required)
- ✅ Workflows can use orchestrator for agent execution

### Integration Points

Workflows integrate with orchestrator:

1. **Agent Execution**: Workflows use orchestrator's agent delegation
2. **Cost Tracking**: Workflows use orchestrator's cost tracking
3. **Session Management**: Workflows use orchestrator's session system
4. **Provider System**: Workflows use orchestrator's provider system

---

## Step-by-Step Migration

### Step 1: Identify Use Cases

List all orchestrator patterns you're using:
- Parallel analysis
- Sequential planning
- Review-refine loops
- Consensus building
- Custom patterns

### Step 2: Create Workflow Equivalents

For each pattern, create a workflow equivalent:

```c
// Example: Migrating parallel analysis
Workflow* wf = pattern_create_parallel_analysis(
    agent_ids,
    agent_count,
    "Your task description"
);
```

### Step 3: Test Workflow

Test the workflow thoroughly:
```c
char* output = NULL;
int result = workflow_execute(wf, input, &output);
// Verify output matches expected result
```

### Step 4: Update Code

Replace orchestrator calls with workflow calls:
```c
// Before
ExecutionPlan* plan = orch_plan_create("Task");
// ... orchestrator execution

// After
Workflow* wf = pattern_create_parallel_analysis(...);
workflow_execute(wf, input, &output);
```

### Step 5: Verify Backward Compatibility

Ensure existing orchestrator code still works:
```c
// This should still work
ExecutionPlan* plan = orch_plan_create("Task");
// ... existing code
```

---

## Migration Checklist

- [ ] Identify all orchestrator patterns in use
- [ ] Create workflow equivalents for each pattern
- [ ] Test workflows thoroughly
- [ ] Update code to use workflows
- [ ] Verify backward compatibility
- [ ] Update documentation
- [ ] Train team on workflow system
- [ ] Monitor for issues
- [ ] Gradually deprecate orchestrator patterns (optional)

---

## Common Migration Scenarios

### Scenario 1: Code Review Workflow

**Before**: Manual orchestrator coordination
**After**: Use `code_review.json` template

```bash
# Load template
convergio workflow execute code_review --input "path/to/code.c"
```

### Scenario 2: Product Launch Workflow

**Before**: Manual orchestrator coordination
**After**: Use `product_launch.json` template

```bash
convergio workflow execute product_launch --input "product_spec.json"
```

### Scenario 3: Custom Workflow

**Before**: Custom orchestrator code
**After**: Create custom workflow

```c
WorkflowNode* start = workflow_node_create("start", NODE_TYPE_ACTION);
// ... build workflow
Workflow* wf = workflow_create("custom_workflow", "Description", start);
workflow_execute(wf, input, &output);
```

---

## Troubleshooting

### Issue: Workflow doesn't execute

**Solution**: Check workflow state and error messages:
```c
if (wf->status == WORKFLOW_STATUS_FAILED) {
    printf("Error: %s\n", wf->error_message);
}
```

### Issue: Checkpoint restoration fails

**Solution**: Verify checkpoint exists and workflow ID matches:
```c
Checkpoint* checkpoints = workflow_list_checkpoints(wf, &count);
// Verify checkpoint_id exists
```

### Issue: State not persisting

**Solution**: Ensure workflow is saved before checkpointing:
```c
workflow_save(wf);
uint64_t checkpoint_id = workflow_checkpoint(wf, "checkpoint_name");
```

---

## Best Practices

1. **Start Simple**: Begin with simple workflows, gradually add complexity
2. **Use Templates**: Leverage existing templates when possible
3. **Test Thoroughly**: Test workflows before migrating production code
4. **Monitor Performance**: Track workflow execution time and costs
5. **Document Changes**: Document all workflow migrations
6. **Maintain Backward Compatibility**: Keep orchestrator working during migration

---

## Deprecation Timeline (Optional)

If you choose to fully migrate:

- **Phase 1 (Months 1-3)**: Mark orchestrator functions as deprecated
- **Phase 2 (Months 4-6)**: Migrate all code to workflows
- **Phase 3 (Months 7-9)**: Remove orchestrator code (not recommended)

**Recommendation**: Keep orchestrator for backward compatibility, use workflows for new features.

---

## References

- [User Guide](USER_GUIDE.md)
- [Architecture Documentation](architecture.md)
- [Pattern Guide](PATTERN_GUIDE.md)
- [Technical Documentation](TECHNICAL_DOCUMENTATION.md)

---

**Last Updated**: 2025-12-20  
**Maintainer**: Convergio Team

