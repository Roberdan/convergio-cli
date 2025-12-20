# Workflow Pattern Guide

**Created**: 2025-12-20  
**Status**: Complete  
**Version**: 1.0.0

---

## Overview

This guide describes all available workflow patterns in the Convergio workflow orchestration system. Patterns are reusable workflow templates that solve common multi-agent coordination problems.

---

## Available Patterns

### 1. Review-Refine Loop

**Purpose**: Iterative improvement through review and refinement cycles

**Use Cases**:
- Code review and refinement
- Document editing and review
- Design iteration
- Content generation and improvement

**Pattern Structure**:
```
Start → Generate → Review → Decision
                          ↓
                    [Needs Refinement?]
                          ↓
                    Yes → Refine → Review (loop)
                    No → End
```

**API**:
```c
Workflow* pattern_create_review_refine_loop(
    SemanticID generator_id,  // Agent that generates content
    SemanticID reviewer_id,   // Agent that reviews content
    int max_iterations        // Maximum refinement cycles
);
```

**Example**:
```c
Workflow* wf = pattern_create_review_refine_loop(
    CODER_ID,      // Generator
    CRITIC_ID,     // Reviewer
    5              // Max 5 iterations
);

char* output = NULL;
workflow_execute(wf, "Write a function to sort arrays", &output);
```

**Configuration**:
- `max_iterations`: Maximum number of refinement cycles (default: 5)
- `consensus_threshold`: Review score threshold for acceptance (default: 0.8)

**State Variables**:
- `iteration_count`: Current iteration number
- `review_score`: Latest review score (0.0 - 1.0)
- `refinement_needed`: Boolean flag

---

### 2. Parallel Analysis

**Purpose**: Multiple agents analyze the same input from different perspectives

**Use Cases**:
- Multi-perspective code analysis
- Security audit from multiple angles
- Performance analysis with different tools
- Design review by multiple experts

**Pattern Structure**:
```
Start → [Agent 1 Analysis] ┐
       [Agent 2 Analysis] ├→ Converge → Synthesis → End
       [Agent 3 Analysis] ┘
```

**API**:
```c
Workflow* pattern_create_parallel_analysis(
    SemanticID* agent_ids,     // Array of agent IDs
    size_t agent_count,        // Number of agents
    const char* task_prompt    // What each agent should analyze
);
```

**Example**:
```c
SemanticID agents[] = {SECURITY_ID, PERFORMANCE_ID, QUALITY_ID};
Workflow* wf = pattern_create_parallel_analysis(
    agents,
    3,
    "Analyze this code for issues"
);

char* output = NULL;
workflow_execute(wf, code_input, &output);
```

**Configuration**:
- `agent_ids`: Array of agent IDs to run in parallel
- `task_prompt`: Prompt for each agent
- `convergence_strategy`: How to combine results (default: "synthesize")

**State Variables**:
- `agent_N_result`: Result from agent N
- `synthesis_result`: Combined analysis result

---

### 3. Sequential Planning

**Purpose**: Step-by-step planning and execution

**Use Cases**:
- Project planning
- Feature implementation
- Task breakdown and execution
- Multi-phase development

**Pattern Structure**:
```
Start → Plan → Execute Step 1 → Execute Step 2 → ... → Execute Step N → End
```

**API**:
```c
Workflow* pattern_create_sequential_planning(
    SemanticID planner_id,     // Agent that creates plan
    SemanticID executor_id,    // Agent that executes steps
    const char* goal           // Overall goal
);
```

**Example**:
```c
Workflow* wf = pattern_create_sequential_planning(
    PLANNER_ID,
    EXECUTOR_ID,
    "Implement user authentication system"
);

char* output = NULL;
workflow_execute(wf, requirements_input, &output);
```

**Configuration**:
- `planner_id`: Agent that creates the plan
- `executor_id`: Agent that executes plan steps
- `max_steps`: Maximum number of execution steps (default: 10)

**State Variables**:
- `plan`: Generated execution plan (JSON)
- `current_step`: Current step number
- `step_N_result`: Result from step N

---

### 4. Consensus Building

**Purpose**: Multi-agent discussion to reach consensus

**Use Cases**:
- Design decisions
- Architecture choices
- Code style decisions
- Team alignment

**Pattern Structure**:
```
Start → Group Chat → Consensus Check
                    ↓
              [Consensus?]
                    ↓
              Yes → End
              No → Group Chat (continue)
```

**API**:
```c
Workflow* pattern_create_consensus_building(
    SemanticID* agent_ids,     // Array of agent IDs
    size_t agent_count,         // Number of agents
    const char* topic           // Topic to discuss
);
```

**Example**:
```c
SemanticID agents[] = {ARCHITECT_ID, DEVELOPER_ID, QA_ID};
Workflow* wf = pattern_create_consensus_building(
    agents,
    3,
    "Choose database technology for new project"
);

char* output = NULL;
workflow_execute(wf, project_requirements, &output);
```

**Configuration**:
- `agent_ids`: Array of agent IDs participating in discussion
- `consensus_threshold`: Voting threshold for consensus (default: 0.75)
- `max_rounds`: Maximum discussion rounds (default: 10)

**State Variables**:
- `round_count`: Current discussion round
- `consensus_reached`: Boolean flag
- `votes`: Voting results from each agent

---

## Pattern Composition

Patterns can be composed to create complex workflows:

### Example: Review-Refine with Parallel Analysis

```c
// Create parallel analysis pattern
Workflow* analysis = pattern_create_parallel_analysis(
    analysis_agents, 3, "Analyze code"
);

// Create review-refine pattern
Workflow* refinement = pattern_create_review_refine_loop(
    CODER_ID, CRITIC_ID, 5
);

// Compose: analysis → refinement
WorkflowNode* analysis_end = workflow_get_current_node(analysis);
WorkflowNode* refinement_start = workflow_get_current_node(refinement);
workflow_node_add_edge(analysis_end, refinement_start, NULL);
```

---

## Custom Patterns

You can create custom patterns by building workflows manually:

### Example: Custom Security Audit Pattern

```c
WorkflowNode* scan = workflow_node_create("security_scan", NODE_TYPE_ACTION);
WorkflowNode* analyze = workflow_node_create("analyze_vulns", NODE_TYPE_ACTION);
WorkflowNode* fix = workflow_node_create("fix_vulns", NODE_TYPE_ACTION);
WorkflowNode* verify = workflow_node_create("verify_fix", NODE_TYPE_ACTION);

workflow_node_set_agent(scan, SECURITY_ID, "Scan for vulnerabilities");
workflow_node_set_agent(analyze, ANALYST_ID, "Analyze vulnerabilities");
workflow_node_set_agent(fix, CODER_ID, "Fix vulnerabilities");
workflow_node_set_agent(verify, QA_ID, "Verify fixes");

workflow_node_add_edge(scan, analyze, NULL);
workflow_node_add_edge(analyze, fix, "state.vuln_count > 0");
workflow_node_add_edge(fix, verify, NULL);
workflow_node_add_edge(verify, scan, "state.fixes_verified == false"); // Loop

Workflow* wf = workflow_create("security_audit", "Security audit pattern", scan);
```

---

## Pattern Templates

Pre-built workflow templates are available in `src/workflow/templates/`:

### Available Templates

1. **code_review.json**: Code review workflow
2. **product_launch.json**: Product launch workflow
3. **bug_triage.json**: Bug triage and fix workflow
4. **security_audit.json**: Security audit workflow
5. **performance_optimization.json**: Performance optimization workflow
6. **api_design_review.json**: API design review workflow
7. **incident_response.json**: Incident response workflow
8. **pre_release_checklist.json**: Pre-release quality gates
9. **class_council.json**: Class council evaluation workflow

### Loading Templates

Templates can be loaded and executed:

```c
// Load template (implementation needed)
Workflow* wf = workflow_load_from_template("code_review.json");
workflow_execute(wf, code_input, &output);
```

---

## Pattern Selection Guide

### When to Use Review-Refine Loop

- ✅ Iterative improvement needed
- ✅ Quality is more important than speed
- ✅ Multiple refinement cycles expected
- ❌ One-shot tasks
- ❌ Time-critical tasks

### When to Use Parallel Analysis

- ✅ Multiple perspectives needed
- ✅ Independent analysis possible
- ✅ Speed is important
- ❌ Sequential dependencies
- ❌ Limited resources

### When to Use Sequential Planning

- ✅ Complex multi-step tasks
- ✅ Dependencies between steps
- ✅ Planning before execution
- ❌ Simple one-step tasks
- ❌ Unpredictable execution order

### When to Use Consensus Building

- ✅ Team decisions needed
- ✅ Multiple stakeholders
- ✅ Alignment important
- ❌ Single decision maker
- ❌ Time-critical decisions

---

## Best Practices

1. **Choose the Right Pattern**: Match pattern to use case
2. **Configure Properly**: Set appropriate thresholds and limits
3. **Monitor State**: Track state variables for debugging
4. **Handle Errors**: Implement error handling for each pattern
5. **Test Thoroughly**: Test patterns with various inputs
6. **Document Custom Patterns**: Document any custom patterns you create

---

## Performance Considerations

### Review-Refine Loop

- **Cost**: High (multiple LLM calls per iteration)
- **Time**: Slow (iterative process)
- **Optimization**: Set appropriate `max_iterations`

### Parallel Analysis

- **Cost**: Medium (parallel LLM calls)
- **Time**: Fast (parallel execution)
- **Optimization**: Limit number of agents

### Sequential Planning

- **Cost**: Medium (planning + execution)
- **Time**: Medium (sequential steps)
- **Optimization**: Optimize plan granularity

### Consensus Building

- **Cost**: High (multiple discussion rounds)
- **Time**: Slow (iterative discussion)
- **Optimization**: Set appropriate `consensus_threshold`

---

## References

- [User Guide](USER_GUIDE.md)
- [Architecture Documentation](architecture.md)
- [Technical Documentation](TECHNICAL_DOCUMENTATION.md)
- [Use Cases](USE_CASES.md)

---

**Last Updated**: 2025-12-20  
**Maintainer**: Convergio Team

