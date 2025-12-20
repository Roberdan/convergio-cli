# Immediate Usage Examples: Workflows in Convergio

**Date**: 2025-12-18  
**Status**: Examples for Phase 1+ Implementation

---

## Overview

This document shows **concrete examples** of how workflows can be used immediately after Phase 1, by both users and agents (Ali, Baccio, etc.).

---

## User Examples (CLI)

### Example 1: Basic Workflow Execution

```bash
convergio
> /workflow list
Available workflows:
  strategic-planning    - Multi-step strategic planning workflow
  parallel-analysis    - Parallel agent analysis with convergence
  review-refine        - Review and refinement loop

> /workflow execute parallel-analysis "Analyze this SaaS project"
Executing workflow 'parallel-analysis'...
├── Baccio (Tech) analyzing architecture...
├── Sofia (Marketing) analyzing market...
├── Omri (Data) analyzing metrics...
└── Ali (Convergence) synthesizing results...

Workflow completed successfully.
Result: [Integrated analysis from all agents]
```

### Example 2: Resume from Checkpoint

```bash
> /workflow execute strategic-planning "Launch new product"
Executing workflow 'strategic-planning'...
├── Domik (Strategy) - COMPLETED
├── Amy (Finance) - IN PROGRESS...
[CRASH - Convergio closed]

# Later, resume from checkpoint
> /workflow list
Available workflows:
  strategic-planning (ID: 12345) - PAUSED at checkpoint "finance-complete"

> /workflow resume 12345
Resuming workflow 12345 from checkpoint "finance-complete"...
├── Baccio (Tech) - STARTING...
└── Convergence - PENDING...

Workflow completed successfully.
```

### Example 3: Create Workflow from Pattern

```bash
> /workflow pattern review-refine code-review
Created workflow 'code-review' from pattern 'review-refine'

> /workflow show code-review
Workflow: code-review
Pattern: review-refine
Nodes:
  1. Generator (Baccio) - Generate code
  2. Critic (Thor) - Review code
  3. Refiner (Baccio) - Refine based on feedback
  4. Validator (Thor) - Final validation

> /workflow execute code-review "Review this function"
Executing workflow 'code-review'...
├── Baccio generating code...
├── Thor reviewing...
├── Baccio refining...
└── Thor validating...

Workflow completed successfully.
```

---

## Ali Integration Examples

### Example 1: Ali Suggests Workflow

```
User: Launch a global SaaS platform

Ali: "This is a complex multi-step task. I can handle this in two ways:

     Option 1: Use existing orchestrator (fast, simple)
     - Parallel analysis from Baccio, Amy, Domik
     - Quick convergence
     
     Option 2: Use strategic-planning workflow (recommended)
     - Checkpointing (resume if interrupted)
     - Multi-step coordination
     - Better error recovery
     
     I recommend Option 2 for this complex task. Proceed?"

User: Yes, use workflow

Ali: "Executing strategic-planning workflow...
     
     Step 1/4: Domik (Strategy) analyzing market opportunities...
     ✓ Strategy complete (checkpoint saved)
     
     Step 2/4: Amy (Finance) creating financial projections...
     ✓ Finance complete (checkpoint saved)
     
     Step 3/4: Baccio (Tech) designing architecture...
     ✓ Architecture complete (checkpoint saved)
     
     Step 4/4: Convergence synthesizing integrated plan...
     ✓ Complete
     
     Here's your integrated strategic plan: [result]"
```

### Example 2: Ali Uses Workflow Tool

**Ali's tool use (automatic):**

```json
{
  "tool": "workflow_execute",
  "parameters": {
    "workflow_name": "parallel-analysis",
    "input": "Analyze this project for scalability issues"
  }
}
```

**Ali's response:**

```
Ali: "I'll use the parallel-analysis workflow to get comprehensive insights:
     
     - Baccio analyzing technical architecture
     - Omri analyzing data patterns
     - Sofia analyzing market fit
     
     Then I'll synthesize all findings into an integrated analysis."
```

### Example 3: Ali Creates Workflow On-the-Fly

```
User: I need a custom workflow for code review with 3 rounds of feedback

Ali: "I'll create a custom review-refine workflow with 3 iterations:
     
     Creating workflow 'custom-code-review'...
     ✓ Workflow created
     
     Executing workflow...
     Round 1: Baccio generates → Thor reviews → Baccio refines
     Round 2: Baccio generates → Thor reviews → Baccio refines
     Round 3: Baccio generates → Thor reviews → Final validation
     
     ✓ Workflow completed
     
     Here's the final reviewed code: [result]"
```

---

## Agent Examples (Other Agents)

### Example 1: Baccio Uses Workflow for Architecture

```
User: @baccio Design a scalable microservices architecture

Baccio: "This requires multiple steps. I'll use the sequential-planning workflow:
        
        Step 1: Analyze requirements
        Step 2: Design service boundaries
        Step 3: Define APIs
        Step 4: Plan deployment strategy
        Step 5: Review with Thor
        
        Executing workflow..."
```

### Example 2: Thor Uses Review-Refine Pattern

```
User: @thor Review this code thoroughly

Thor: "I'll use the review-refine workflow for comprehensive review:
      
      Round 1: Initial review → Feedback
      Round 2: Review refined code → More feedback
      Round 3: Final review → Approval
      
      Executing workflow..."
```

### Example 3: Group Chat for Consensus

```
User: Should we use TypeScript or JavaScript for this project?

Ali: "This requires team consensus. I'll use the consensus-building workflow:
     
     Participants: Baccio (Tech), Domik (Strategy), Amy (Finance)
     
     Round 1: Each agent presents their perspective
     Round 2: Discussion and debate
     Round 3: Voting
     Round 4: Consensus reached
     
     Executing workflow..."
```

---

## Integration Code Examples

### Example 1: Ali's Decision Logic

```c
// In orchestrator.c or Ali's response generation
char* ali_handle_request(const char* user_input) {
    // Check if workflow would be beneficial
    if (is_complex_multi_step_task(user_input)) {
        // Check if workflow engine is available
        if (workflow_engine_available()) {
            // Suggest using workflow
            Workflow* wf = workflow_find_suitable(user_input);
            if (wf) {
                return workflow_execute(wf, user_input, NULL);
            }
        }
    }
    
    // Fallback to existing orchestrator
    return orchestrator_parallel_analyze(user_input, ...);
}
```

### Example 2: Workflow Tool for Ali

```c
// Tool implementation for Ali
char* tool_workflow_execute(const char* workflow_name, const char* input) {
    Workflow* wf = workflow_load_by_name(workflow_name);
    if (!wf) {
        return strdup("Workflow not found");
    }
    
    char* result = NULL;
    int status = workflow_execute(wf, input, &result);
    
    if (status == 0 && result) {
        return result;
    }
    
    return strdup("Workflow execution failed");
}
```

### Example 3: Pattern Creation Tool

```c
// Ali can create workflows from patterns
char* tool_workflow_create_from_pattern(const char* pattern_name, 
                                        const char* workflow_name) {
    Workflow* wf = NULL;
    
    if (strcmp(pattern_name, "review-refine") == 0) {
        SemanticID generator = agent_find_by_name("baccio")->id;
        SemanticID critic = agent_find_by_name("thor")->id;
        SemanticID refiner = agent_find_by_name("baccio")->id;
        wf = pattern_create_review_refine_loop(generator, critic, refiner, 3);
    }
    // ... other patterns ...
    
    if (!wf) {
        return strdup("Unknown pattern");
    }
    
    workflow_set_name(wf, workflow_name);
    workflow_save(wf);
    
    char* response = malloc(256);
    snprintf(response, 256, "Created workflow '%s' from pattern '%s'", 
             workflow_name, pattern_name);
    return response;
}
```

---

## Real-World Usage Scenarios

### Scenario 1: Product Launch Planning

```
User: Plan a product launch for Q2 2025

Ali: "I'll use the strategic-planning workflow:
     
     Workflow: strategic-planning
     Steps:
       1. Domik (Strategy) - Market analysis and positioning
       2. Amy (Finance) - Budget and ROI projections
       3. Sofia (Marketing) - Go-to-market strategy
       4. Baccio (Tech) - Technical requirements
       5. Convergence - Integrated launch plan
     
     Executing..."
```

### Scenario 2: Code Review with Multiple Rounds

```
User: @thor Review this PR thoroughly with 3 rounds of feedback

Thor: "Using review-refine workflow with 3 iterations:
      
      Round 1:
      - Baccio generates initial code
      - I review and provide feedback
      - Baccio refines
      
      Round 2:
      - Baccio generates refined code
      - I review again
      - Baccio refines further
      
      Round 3:
      - Baccio generates final code
      - I do final review
      - Approval or final feedback
      
      Executing workflow..."
```

### Scenario 3: Architecture Decision with Consensus

```
User: Should we migrate to microservices?

Ali: "This is a strategic decision. I'll use consensus-building workflow:
     
     Participants: Baccio (Tech), Domik (Strategy), Amy (Finance), Marco (DevOps)
     
     Process:
     1. Each agent presents their perspective
     2. Discussion and debate
     3. Voting
     4. Consensus reached
     
     Executing workflow..."
```

---

## Benefits Demonstrated

### 1. Checkpointing (Resume Capability)

**Before (without workflow):**
- Crash = lost progress
- Must restart from beginning

**After (with workflow):**
- Crash = resume from last checkpoint
- No lost work

### 2. Complex Multi-Step Coordination

**Before (hardcoded):**
- Limited to simple parallel/sequential
- Difficult to add new patterns

**After (workflow):**
- Any workflow pattern possible
- Easy to create new patterns

### 3. Better Error Recovery

**Before:**
- Error = restart
- No retry logic

**After:**
- Error = retry from checkpoint
- Fallback strategies

### 4. Reusability

**Before:**
- Each pattern coded separately
- No sharing between agents

**After:**
- Patterns reusable
- Shared workflow library

---

## Conclusion

Workflows are **immediately usable** after Phase 1:
- ✅ Users can execute workflows via CLI
- ✅ Ali can suggest and use workflows
- ✅ Other agents can use workflows
- ✅ All backward compatible
- ✅ Zero breaking changes

