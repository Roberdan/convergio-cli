# ADR 007: Projects Feature

## Status
Accepted (Implemented in v3.0.12)

## Date
2025-12-13

## Context

Users work on different types of projects that require different agent teams:
- **App Development**: Needs architects, developers, DevOps, testers
- **Marketing Campaign**: Needs copywriters, designers, analysts
- **Executive Presentation**: Needs strategists, data analysts, writers
- **Financial Analysis**: Needs CFO, market analysts, economists

Currently, all 50+ agents are available for every conversation, which:
1. Creates cognitive overhead for users
2. Wastes context tokens on irrelevant agents
3. Doesn't persist context between sessions
4. Lacks project-specific memory

## Decision

Implement a **Projects** feature with the following capabilities:

### 1. Project Definition
```
project:
  name: "MyApp 2.0"
  purpose: "Build a new mobile application"
  team:
    - baccio (tech architect)
    - davide (backend dev)
    - stefano (frontend dev)
    - tester (QA)
  created: 2025-12-13
  last_active: 2025-12-13
```

### 2. Storage Location
Projects stored in `~/.convergio/projects/`:
```
~/.convergio/projects/
├── myapp-2.0/
│   ├── project.yaml       # Project definition
│   ├── context.json       # Shared context/memory
│   ├── history.jsonl      # Conversation history
│   └── artifacts/         # Generated files
├── marketing-q1/
│   └── ...
└── exec-presentation/
    └── ...
```

### 3. Commands
```bash
# Create new project
convergio project create "MyApp 2.0" --purpose "Build mobile app" --team baccio,davide,stefano

# List projects
convergio project list

# Switch to project
convergio project use "MyApp 2.0"

# Show project status
convergio project status

# Add/remove team members
convergio project team add amy
convergio project team remove tester

# Archive project
convergio project archive "MyApp 2.0"
```

### 4. Context Sharing
- Agents within a project share context via semantic memory
- Project-specific memories prefixed with `[project:name]`
- Cross-session continuity via history.jsonl
- Automatic context summarization for long conversations

### 5. Project Templates
Pre-defined templates for common project types:
- `app-dev`: Architecture, development, testing, DevOps
- `marketing`: Creative, analytics, strategy
- `research`: Data science, analysis, documentation
- `executive`: Strategy, finance, presentation

### 6. Implementation Plan

#### Phase 1: Core Infrastructure
1. Create `src/projects/` module
2. Implement project YAML parser
3. Add project storage manager
4. Create `project` command handler

#### Phase 2: Team Management
1. Filter available agents based on project team
2. Implement team add/remove commands
3. Update orchestrator to respect project scope

#### Phase 3: Context Persistence
1. Integrate with semantic memory for project-scoped memories
2. Implement history save/restore
3. Add context summarization for session continuity

#### Phase 4: Templates & UX
1. Create default templates
2. Add project initialization wizard
3. Implement project status dashboard

## Consequences

### Positive
- Focused agent teams for better relevance
- Persistent context across sessions
- Reduced token usage (fewer irrelevant agents)
- Better organization for multi-project users
- Enables team collaboration (shared project files)

### Negative
- Additional complexity in codebase
- Storage management overhead
- Need to handle project migration/versioning
- Potential confusion if user forgets current project context

### Neutral
- New commands to learn
- Project files stored locally (not cloud-synced by default)

## Implementation Notes

### Data Structures

```c
typedef struct {
    char* name;
    char* purpose;
    char** team_members;
    size_t team_count;
    time_t created;
    time_t last_active;
    char* storage_path;
} ConvergioProject;

typedef struct {
    ConvergioProject* current;
    ConvergioProject** all_projects;
    size_t project_count;
} ProjectManager;
```

### Memory Integration

Project memories use prefix to namespace:
```c
// Store project-specific memory
memory_store("[project:myapp-2.0] API architecture uses REST with GraphQL gateway");

// Search only project memories
memory_search("architecture", "[project:myapp-2.0]");
```

### File Formats

**project.yaml**
```yaml
version: 1
name: MyApp 2.0
purpose: Build a new mobile application for iOS and Android
team:
  - name: baccio
    role: lead architect
  - name: davide
    role: backend developer
  - name: stefano
    role: frontend developer
created: 2025-12-13T10:00:00Z
last_active: 2025-12-13T15:30:00Z
settings:
  auto_save: true
  max_history: 1000
```

**context.json**
```json
{
  "summary": "Building a mobile app with React Native frontend and Node.js backend",
  "key_decisions": [
    "Using React Native for cross-platform",
    "PostgreSQL for database",
    "AWS for hosting"
  ],
  "current_focus": "Setting up CI/CD pipeline",
  "open_questions": [
    "Which auth provider to use?"
  ]
}
```

## References
- Microsoft Engineering Fundamentals Playbook (project organization)
- Semantic memory architecture (ADR-001)
- Multi-provider architecture (ADR-006)

## Author
Roberto with AI team assistance
