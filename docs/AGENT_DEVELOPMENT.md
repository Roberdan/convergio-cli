# Agent Development Guide

This guide explains how to create, configure, and customize agents in Convergio.

## Overview

Agents in Convergio are specialized AI personalities that handle specific types of tasks. Each agent has:
- A unique name and role
- Configured model and provider
- Custom system prompt
- Specific capabilities and limitations

---

## Built-in Agents

### Ali (Orchestrator)
The Chief of Staff who coordinates all other agents.

| Property | Value |
|----------|-------|
| Role | Orchestrator |
| Default Model | Claude Opus 4.5 |
| Fallback | Claude Sonnet 4.5 |
| Specialization | Task decomposition, coordination, convergence |

### Marco (Coder)
Expert software engineer for code generation and review.

| Property | Value |
|----------|-------|
| Role | Coder |
| Default Model | Claude Sonnet 4.5 |
| Fallback | GPT-5.2-thinking |
| Specialization | Code generation, debugging, refactoring |

### Sara (Writer)
Content creator for documentation and copywriting.

| Property | Value |
|----------|-------|
| Role | Writer |
| Default Model | Claude Sonnet 4.5 |
| Fallback | Gemini 3 Pro |
| Specialization | Documentation, technical writing, content |

### Leo (Analyst)
Deep researcher for analysis and investigation.

| Property | Value |
|----------|-------|
| Role | Analyst |
| Default Model | GPT-5.2-pro |
| Fallback | Claude Sonnet 4.5 |
| Specialization | Research, analysis, data interpretation |

### Nina (Critic)
Quality reviewer for validation and feedback.

| Property | Value |
|----------|-------|
| Role | Critic |
| Default Model | Claude Haiku 4.5 |
| Fallback | GPT-5-nano |
| Specialization | Code review, validation, quality assurance |

---

## Creating Custom Agents

### Agent Configuration File

Create a JSON file in `~/.convergio/agents/`:

```json
{
  "name": "alex",
  "description": "Security specialist for vulnerability analysis",
  "role": "analyst",
  "model": {
    "provider": "anthropic",
    "model_id": "claude-sonnet-4.5"
  },
  "fallback": {
    "provider": "openai",
    "model_id": "gpt-5.2-pro"
  },
  "settings": {
    "max_tokens": 8192,
    "temperature": 0.3,
    "streaming": true,
    "tools": true
  },
  "budget": {
    "max_per_call": 0.5,
    "session": 5.0
  },
  "system_prompt": "You are Alex, a security expert specializing in vulnerability assessment and secure coding practices. You analyze code for security issues, suggest fixes, and follow OWASP guidelines."
}
```

### Configuration Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Unique agent identifier |
| `description` | string | Yes | Human-readable description |
| `role` | string | Yes | Agent role (see below) |
| `model` | object | Yes | Primary model configuration |
| `fallback` | object | No | Fallback model for errors |
| `settings` | object | No | Behavior settings |
| `budget` | object | No | Cost limits |
| `system_prompt` | string | No | Custom system prompt |

### Available Roles

| Role | Description | Typical Use |
|------|-------------|-------------|
| `orchestrator` | Coordinates other agents | Task management |
| `coder` | Code generation/review | Development |
| `writer` | Content creation | Documentation |
| `analyst` | Research and analysis | Investigation |
| `critic` | Validation and review | Quality assurance |
| `planner` | Task decomposition | Planning |
| `executor` | Direct task execution | Simple tasks |
| `memory` | Context retrieval | RAG |

---

## System Prompts

### Best Practices

1. **Be Specific**: Define the agent's expertise clearly
2. **Set Boundaries**: Specify what the agent should NOT do
3. **Include Context**: Provide domain-specific knowledge
4. **Define Tone**: Specify communication style

### Example System Prompts

**Security Expert:**
```
You are a cybersecurity expert with deep knowledge of:
- OWASP Top 10 vulnerabilities
- Secure coding practices
- Penetration testing methodologies
- Compliance frameworks (SOC2, GDPR, HIPAA)

When reviewing code:
1. Identify potential vulnerabilities
2. Rate severity (Critical/High/Medium/Low)
3. Provide secure alternatives
4. Explain the attack vector

Always prioritize security over convenience.
```

**API Designer:**
```
You are an API design expert specializing in REST and GraphQL.

Your principles:
- RESTful best practices
- Consistent naming conventions
- Proper HTTP status codes
- Comprehensive error handling
- API versioning strategies

When designing APIs:
1. Consider backward compatibility
2. Document all endpoints
3. Include authentication requirements
4. Provide example requests/responses
```

---

## Model Selection

### Choosing the Right Model

| Task Type | Recommended Model | Why |
|-----------|-------------------|-----|
| Complex reasoning | Claude Opus 4.5 | Best for nuanced decisions |
| Code generation | Claude Sonnet 4.5 | Optimal code quality/cost |
| Fast classification | GPT-5-nano | Speed and low cost |
| Long documents | Gemini 3 Pro | Large context window |
| Creative writing | Claude Sonnet 4.5 | Natural language quality |

### Budget-Aware Selection

Convergio automatically selects cheaper models when budget is low:

```
Remaining Budget > $1.00 → Primary Model
$0.50 - $1.00 → Fallback Model
< $0.50 → Economy Model
```

---

## Agent Communication

### Inter-Agent Messages

Agents can communicate via the message bus:

```c
// Send message to specific agent
Message* msg = message_create(MSG_TYPE_TASK_DELEGATE,
                               ali_id, marco_id,
                               "Review this code for bugs");
message_send(msg);

// Broadcast to all agents
Message* broadcast = message_create(MSG_TYPE_AGENT_THOUGHT,
                                     ali_id, 0,
                                     "Starting analysis phase");
message_broadcast(broadcast);
```

### Message Types

| Type | Description | Use Case |
|------|-------------|----------|
| `TASK_DELEGATE` | Assign task to agent | Ali → Marco |
| `TASK_REPORT` | Report task completion | Marco → Ali |
| `AGENT_THOUGHT` | Share reasoning | Internal processing |
| `AGENT_RESPONSE` | Final response | To user |
| `CONVERGENCE` | Synthesized result | Final answer |

---

## Tool Calling

### Enabling Tools

```json
{
  "settings": {
    "tools": true
  }
}
```

### Built-in Tools

| Tool | Description | Parameters |
|------|-------------|------------|
| `read_file` | Read file contents | `path` |
| `write_file` | Write to file | `path`, `content` |
| `execute_command` | Run shell command | `command` |
| `search_files` | Search file contents | `query`, `path` |

### Custom Tools

Register custom tools programmatically:

```c
Tool* tool = tool_create("analyze_image",
                          "Analyze image for objects");
tool_add_parameter(tool, "image_path",
                    "Path to image file",
                    TOOL_PARAM_STRING, true);
tool_set_handler(tool, my_image_handler, NULL);
tools_register(tool);
```

---

## Testing Agents

### Unit Testing

```c
// Create mock provider
MockProvider* mock = mock_provider_success("Test response");

// Configure agent to use mock
agent_config_set_model("marco", PROVIDER_COUNT, "mock");

// Test agent behavior
char* response = agent_process("marco", "Write hello world");
assert(response != NULL);
```

### Integration Testing

```bash
# Test agent via CLI
convergio --agent marco "Review this code: ..."

# Verify model selection
convergio agents status
```

---

## Performance Optimization

### Streaming

Enable streaming for long responses:

```json
{
  "settings": {
    "streaming": true
  }
}
```

### Token Limits

Set appropriate limits to control costs:

```json
{
  "settings": {
    "max_tokens": 4096
  }
}
```

### Temperature

Adjust for task type:

| Temperature | Use Case |
|-------------|----------|
| 0.0 - 0.3 | Code, analysis (deterministic) |
| 0.5 - 0.7 | General conversation |
| 0.8 - 1.0 | Creative writing |

---

## Troubleshooting

### Agent Not Responding

1. Check model availability: `convergio providers test`
2. Verify budget: `convergio cost status`
3. Check configuration: `convergio agents config <name>`

### Wrong Model Used

1. Check agent config: `cat ~/.convergio/agents/<name>.json`
2. Verify provider keys: `convergio providers test`
3. Check fallback chain

### High Costs

1. Review model selection
2. Lower `max_tokens`
3. Increase `temperature` (may reduce output length)
4. Use economy model for simple tasks

---

## Examples

### DevOps Agent

```json
{
  "name": "ops",
  "description": "DevOps specialist for infrastructure",
  "role": "executor",
  "model": {
    "provider": "anthropic",
    "model_id": "claude-sonnet-4.5"
  },
  "settings": {
    "max_tokens": 4096,
    "temperature": 0.2,
    "tools": true
  },
  "system_prompt": "You are a DevOps engineer expert in:\n- Kubernetes and Docker\n- CI/CD pipelines\n- Infrastructure as Code\n- Cloud platforms (AWS, GCP, Azure)\n\nProvide practical, production-ready solutions."
}
```

### Data Scientist Agent

```json
{
  "name": "data",
  "description": "Data scientist for analysis",
  "role": "analyst",
  "model": {
    "provider": "openai",
    "model_id": "gpt-5.2-pro"
  },
  "settings": {
    "max_tokens": 8192,
    "temperature": 0.4,
    "tools": true
  },
  "system_prompt": "You are a data scientist specializing in:\n- Statistical analysis\n- Machine learning\n- Data visualization\n- Python/R/SQL\n\nProvide code examples with explanations."
}
```

---

## Reference

### CLI Commands

```bash
# List agents
convergio agents list

# Show agent config
convergio agents config <name>

# Create agent
convergio agents create --name <name> --role <role>

# Test agent
convergio agents test <name>

# Update agent model
convergio agents set-model <name> <provider> <model_id>
```

### API Reference

See the source code in `src/agents/` for the complete API.
