# Convergio V7 — Plugin SDK & Vertical Development

**Status:** Draft for approval
**Date:** 2025-12-26
**Purpose:** Define how developers build plugins and verticals for Convergio Core.

---

## 1) Plugin Types

| Type | Scope | Example |
|------|-------|---------|
| **Tool** | Single function | `web_search`, `calculator`, `file_reader` |
| **Agent** | Specialized behavior | `code_reviewer`, `translator`, `summarizer` |
| **Vertical** | Complete domain package | Education, Healthcare, Legal |

---

## 2) Plugin Manifest (Required)

Every plugin requires a `convergio.plugin.json`:

```json
{
  "name": "my-plugin",
  "version": "1.0.0",
  "type": "tool|agent|vertical",
  "engine": "wasm|native",
  "permissions": [
    "network:fetch",
    "filesystem:read",
    "secrets:read"
  ],
  "entrypoint": "main.wasm",
  "tools": [...],
  "agents": [...],
  "metadata": {
    "author": "...",
    "license": "...",
    "repository": "..."
  }
}
```

---

## 3) Permission Catalog

Plugins must declare required permissions. Users/admins approve at install time.

| Permission | Description | Risk Level |
|------------|-------------|------------|
| `network:fetch` | HTTP requests to allowed domains | Medium |
| `network:any` | HTTP to any domain | High |
| `filesystem:read` | Read files in sandbox | Low |
| `filesystem:write` | Write files in sandbox | Medium |
| `filesystem:any` | Read/write anywhere | Critical |
| `secrets:read` | Access user secrets | High |
| `clipboard:read` | Read clipboard | Medium |
| `clipboard:write` | Write clipboard | Low |
| `shell:execute` | Run shell commands | Critical |
| `llm:invoke` | Call LLM providers | Medium |
| `agent:spawn` | Create sub-agents | Medium |

**Default policy:** Deny all. Explicit grant required.

---

## 4) Tool Development

### 4.1 Tool Schema

```json
{
  "name": "web_search",
  "description": "Search the web for information",
  "parameters": {
    "type": "object",
    "properties": {
      "query": {
        "type": "string",
        "description": "Search query"
      },
      "max_results": {
        "type": "integer",
        "default": 10
      }
    },
    "required": ["query"]
  },
  "returns": {
    "type": "array",
    "items": {
      "type": "object",
      "properties": {
        "title": {"type": "string"},
        "url": {"type": "string"},
        "snippet": {"type": "string"}
      }
    }
  }
}
```

### 4.2 Tool Implementation (WASM)

```rust
// Rust example compiled to WASM
use convergio_sdk::*;

#[convergio_tool]
fn web_search(query: String, max_results: Option<i32>) -> Result<Vec<SearchResult>> {
    let max = max_results.unwrap_or(10);
    let results = http_get(&format!("https://api.search.com?q={}&n={}", query, max))?;
    Ok(parse_results(results))
}
```

### 4.3 Tool Implementation (Native - Trusted Only)

```rust
// Native Rust - requires signing
use convergio_sdk::*;

#[convergio_tool(trusted)]
fn execute_code(language: String, code: String) -> Result<ExecutionResult> {
    // Direct system access - only for signed plugins
    let sandbox = create_sandbox(&language)?;
    sandbox.execute(&code)
}
```

---

## 5) Agent Development

### 5.1 Agent Definition

```json
{
  "name": "code_reviewer",
  "description": "Reviews code for quality and security issues",
  "system_prompt": "You are an expert code reviewer...",
  "tools": ["read_file", "grep", "run_tests"],
  "model_preference": ["gpt-4o", "claude-3-opus"],
  "max_iterations": 10,
  "memory": {
    "type": "conversation",
    "max_tokens": 8000
  }
}
```

### 5.2 Agent with Custom Logic

```typescript
// TypeScript agent with custom behavior
import { Agent, Tool, Context } from '@convergio/sdk';

export class CodeReviewer extends Agent {
  name = 'code_reviewer';

  async run(context: Context): Promise<AgentResult> {
    // Read the file
    const code = await this.useTool('read_file', { path: context.input.file });

    // Analyze with LLM
    const analysis = await this.think(`
      Review this code for:
      1. Security vulnerabilities
      2. Performance issues
      3. Code style

      Code:
      ${code}
    `);

    // Run tests if requested
    if (context.input.run_tests) {
      const testResults = await this.useTool('run_tests', {});
      analysis.testResults = testResults;
    }

    return analysis;
  }
}
```

---

## 6) Vertical Development

A vertical is a complete domain package containing:

```
education-vertical/
├── convergio.plugin.json      # Manifest
├── agents/
│   ├── tutor.json             # AI tutor agent
│   ├── grader.json            # Assignment grader
│   └── curriculum.json        # Curriculum planner
├── tools/
│   ├── quiz_generator.wasm    # Quiz generation tool
│   ├── progress_tracker.wasm  # Student progress
│   └── content_filter.wasm    # Age-appropriate filtering
├── knowledge/
│   ├── curricula/             # RAG knowledge bases
│   └── pedagogical/           # Teaching methods
├── policies/
│   ├── content_policy.json    # What content is allowed
│   ├── privacy_policy.json    # Data handling
│   └── age_restrictions.json  # Age-based restrictions
├── ui/                        # Optional UI components
│   ├── student_dashboard/
│   └── teacher_dashboard/
└── locales/                   # Internationalization
    ├── en.json
    ├── it.json
    └── es.json
```

### 6.1 Vertical Manifest

```json
{
  "name": "education",
  "version": "1.0.0",
  "type": "vertical",
  "displayName": "Convergio Education",
  "description": "AI-powered education platform",
  "agents": [
    {"ref": "./agents/tutor.json", "default": true},
    {"ref": "./agents/grader.json"},
    {"ref": "./agents/curriculum.json"}
  ],
  "tools": [
    {"ref": "./tools/quiz_generator.wasm"},
    {"ref": "./tools/progress_tracker.wasm"},
    {"ref": "./tools/content_filter.wasm"}
  ],
  "knowledge_bases": [
    {"name": "curricula", "path": "./knowledge/curricula/"},
    {"name": "pedagogical", "path": "./knowledge/pedagogical/"}
  ],
  "policies": {
    "content": "./policies/content_policy.json",
    "privacy": "./policies/privacy_policy.json"
  },
  "compliance": ["GDPR", "COPPA", "FERPA"],
  "ui": {
    "student": "./ui/student_dashboard/",
    "teacher": "./ui/teacher_dashboard/"
  },
  "license": "commercial",
  "pricing": {
    "model": "subscription",
    "tiers": ["free", "starter", "plus", "school"]
  }
}
```

---

## 7) SDK Languages

| Language | Support Level | Use Case |
|----------|---------------|----------|
| **Rust** | First-class | Performance-critical tools, WASM |
| **TypeScript** | First-class | Agents, business logic |
| **Python** | Supported | Data science, ML tools |
| **Go** | Community | Infrastructure tools |

---

## 8) Development Workflow

```bash
# Install SDK
npm install -g @convergio/cli

# Create new plugin
convergio plugin init my-tool --type=tool --lang=rust

# Develop locally
convergio dev

# Test
convergio test

# Build WASM
convergio build

# Publish (requires signing for native)
convergio publish
```

---

## 9) Plugin Distribution

### 9.1 Official Registry
- Curated plugins from Convergio team
- Security reviewed
- SLA guarantees

### 9.2 Community Registry
- Open submission
- Automated security scanning
- Community ratings

### 9.3 Private Registry
- Enterprise self-hosted
- Internal plugins
- Compliance controlled

---

## 10) Security Model

| Plugin Type | Execution | Review Required | Signing |
|-------------|-----------|-----------------|---------|
| **WASM (untrusted)** | Sandbox | Automated scan | No |
| **WASM (verified)** | Sandbox | Manual review | Yes |
| **Native (trusted)** | Direct | Full audit | Required |

---

## 11) Monetization for Plugin Developers

| Model | Description |
|-------|-------------|
| **Free/OSS** | MIT/Apache, community contribution |
| **Freemium** | Free tier + paid features |
| **Subscription** | Monthly/yearly per seat |
| **Usage-based** | Per invocation pricing |
| **Enterprise** | Custom licensing |

Revenue share: **70% developer / 30% platform** (for paid plugins)

