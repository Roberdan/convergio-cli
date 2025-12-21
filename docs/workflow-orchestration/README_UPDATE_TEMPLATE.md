# README Update Template

**When to Update**: After Phase 1 completion (workflows available)

**Location**: Add to main `README.md` after "Features" section

---

## Section to Add: Advanced Workflow Orchestration

```markdown
## Advanced Workflow Orchestration

Convergio supports **state machine-based workflows** for complex multi-step agent coordination with persistence and recovery.

### Key Features

- **Workflow Persistence**: Checkpointing enables resume from any point in execution
- **Pattern Library**: Pre-built workflows for common scenarios (review-refine, parallel-analysis, etc.)
- **Conditional Routing**: Dynamic workflow adaptation based on intermediate results
- **Group Chat**: Multi-agent discussions with consensus building
- **Task Decomposition**: Automatic hierarchical task breakdown with dependency resolution

### Quick Start

```bash
# List available workflows
> /workflow list

# Execute a workflow
> /workflow execute parallel-analysis "Analyze this project"

# Resume from checkpoint (if workflow was interrupted)
> /workflow resume 12345

# Create workflow from pattern
> /workflow pattern review-refine code-review
```

### Workflow Patterns

| Pattern | Description | Best For |
|---------|-------------|----------|
| **Review-Refine Loop** | Generator → Critic → Refiner → Validator | Code review, content creation |
| **Parallel Analysis** | Multiple agents analyze → Convergence | Project analysis, strategy planning |
| **Sequential Planning** | Strategy → Plan → Architecture → Implementation | Product development, project planning |
| **Consensus Building** | Discussion → Voting → Consensus → Action | Decision making, team alignment |

### Ali Integration

Ali automatically suggests workflows for complex multi-step tasks:

```
> Launch a global SaaS platform

Ali: "This is a complex multi-step task. I'll use the strategic-planning workflow:
     1. Domik (Strategy) analyzes market and opportunities
     2. Amy (Finance) creates financial projections
     3. Baccio (Tech) designs architecture
     4. Convergence synthesizes integrated plan"
```

### Workflow Commands

| Command | Description |
|---------|-------------|
| `/workflow list` | List all available workflows |
| `/workflow show <name>` | Show workflow details and Mermaid diagram |
| `/workflow execute <name> [input]` | Execute a workflow |
| `/workflow resume <id> [checkpoint]` | Resume workflow from checkpoint |
| `/workflow checkpoint <id>` | List checkpoints for a workflow |
| `/workflow pattern <pattern> <name>` | Create workflow from pattern |

### Documentation

- [Workflow User Guide](docs/workflow-orchestration/USER_GUIDE.md) - Complete usage guide
- [Workflow Master Plan](docs/workflow-orchestration/MASTER_PLAN.md) - Implementation details
- [Workflow Patterns](docs/workflow-orchestration/PATTERNS.md) - Available patterns

---

**New in v5.4.0**: Advanced workflow orchestration with checkpointing and pattern library.
```

---

## Update Features Section

**Add to existing Features section:**

```markdown
### Advanced Workflow Orchestration

- **State Machine Execution**: Workflows defined as graphs, not hardcoded
- **Checkpointing**: Resume from any point in workflow execution
- **Pattern Library**: Reusable workflow templates
- **Conditional Routing**: Dynamic workflow adaptation based on state
- **Task Decomposition**: Automatic hierarchical task breakdown
- **Group Chat**: Multi-agent discussions with consensus building
- **Workflow Persistence**: State survives crashes and restarts
```

---

## Update Commands Reference

**Add to Commands Reference section:**

```markdown
### Workflow Management

| Command | Description |
|---------|-------------|
| `/workflow list` | List all workflows |
| `/workflow show <name>` | Show workflow details |
| `/workflow execute <name> [input]` | Execute workflow |
| `/workflow resume <id>` | Resume from checkpoint |
| `/workflow checkpoint <id>` | List checkpoints |
| `/workflow pattern <pattern> <name>` | Create from pattern |
```

---

## Update Version Badge

**After Phase 5 completion, update version:**

```markdown
<a href="https://github.com/Roberdan/convergio-cli/releases/latest"><img src="https://img.shields.io/badge/version-5.4.0-blue" alt="Version 5.4.0"></a>
```

---

## Update "What's New" Section

**Add to "What's New" section (if exists):**

```markdown
## What's New in v5.4.0

### Advanced Workflow Orchestration

State machine-based workflows for complex multi-step agent coordination:

- **Checkpointing**: Resume workflows from any point
- **Pattern Library**: Pre-built workflows (review-refine, parallel-analysis, etc.)
- **Conditional Routing**: Dynamic workflow adaptation
- **Task Decomposition**: Automatic hierarchical task breakdown
- **Group Chat**: Multi-agent discussions with consensus

```bash
# Execute a workflow
> /workflow execute parallel-analysis "Analyze this project"

# Resume from checkpoint
> /workflow resume 12345
```

See [Workflow Documentation](docs/workflow-orchestration/USER_GUIDE.md) for details.
```


