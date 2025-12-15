# ADR-012: Centralized Output Service

**Date**: 2025-12-16
**Status**: Implemented
**Author**: AI Team

## Context

Convergio's multi-agent system generates various outputs: analysis reports, architecture diagrams, status updates, etc. Currently:

1. **No standard format** - Each agent outputs differently
2. **Terminal-only** - Rich content lost when session ends
3. **No diagrams** - Mermaid/charts not leveraged
4. **No links** - Users must copy/paste paths

Inspired by Anthropic's `claude-quickstarts/financial-data-analyst` pattern of Claude-driven visualization, we need a unified output service.

## Decision

Implement a **centralized output service** that provides:

### Core Features

1. **Document generation** - Markdown with metadata
2. **Mermaid integration** - Flowcharts, sequence, gantt, pie, mindmap
3. **Table generation** - Formatted markdown tables
4. **Terminal links** - OSC8 hyperlinks (clickable paths)
5. **File management** - Organization, cleanup, listing

### Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Output Service                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐ │
│  │   Document  │  │   Mermaid   │  │     Table       │ │
│  │  Generator  │  │   Helpers   │  │    Builder      │ │
│  └─────────────┘  └─────────────┘  └─────────────────┘ │
│          │               │                │             │
│          └───────────────┼────────────────┘             │
│                          ▼                              │
│              ┌──────────────────────┐                   │
│              │    File Manager      │                   │
│              │  - Path generation   │                   │
│              │  - OSC8 links        │                   │
│              │  - Cleanup policy    │                   │
│              └──────────────────────┘                   │
│                          │                              │
│                          ▼                              │
│              ~/.convergio/outputs/                      │
│                 └── 2025-12-16/                         │
│                     └── project-name/                   │
│                         └── report-abc123.md            │
└─────────────────────────────────────────────────────────┘
```

### API Design

```c
// Create document
OutputRequest req = {
    .title = "Analysis Report",
    .content = markdown_content,
    .agent_name = "baccio",
    .project_context = "MyProject",
    .format = OUTPUT_FORMAT_MARKDOWN,
    .include_timestamp = true
};
OutputResult result;
output_create(&req, &result);
// result.filepath = ~/.convergio/outputs/2025-12-16/myproject/analysis-report-abc123.md
// result.terminal_link = OSC8 clickable link
```

### Mermaid Helpers

| Function | Output |
|----------|--------|
| `output_mermaid_flowchart()` | flowchart LR/TD |
| `output_mermaid_sequence()` | sequenceDiagram |
| `output_mermaid_gantt()` | gantt chart |
| `output_mermaid_pie()` | pie chart |
| `output_mermaid_mindmap()` | mindmap |

### Terminal Integration

Uses existing `hyperlink.h` for OSC8 support:

```c
// Terminal output
📄 Analysis Report
   ^--- clickable link (in iTerm2, Warp, Kitty, etc.)
```

## Alternatives Considered

### Option A: Direct file writes per agent

**Pros:**
- Simple, no coordination needed

**Cons:**
- Inconsistent formats
- No organization
- No link generation

**Decision:** Rejected - We want consistency across agents.

### Option B: HTML generation

**Pros:**
- Rich formatting
- Browser viewable

**Cons:**
- More complex
- Markdown more universal
- Warp/iTerm2 render MD natively

**Decision:** Rejected - Markdown with Mermaid is sufficient.

### Option C: PDF generation

**Pros:**
- Professional output
- Fixed layout

**Cons:**
- Requires PDF library (heavy)
- Not easily editable
- Slower to generate

**Decision:** Rejected - Can convert MD to PDF externally if needed.

## Implementation

### Files Added

| File | Purpose |
|------|---------|
| `include/nous/output_service.h` | Public API |
| `src/tools/output_service.c` | Implementation (~650 LOC) |
| `tests/test_output_service.c` | Unit tests (20+ cases) |

### Directory Structure

```
~/.convergio/outputs/
├── 2025-12-16/
│   ├── project-a/
│   │   ├── analysis-abc123.md
│   │   └── architecture-def456.md
│   └── project-b/
│       └── report-ghi789.md
└── latest -> 2025-12-16/  (symlink)
```

### Templates

Built-in templates:
- `analysis` - Executive summary, findings, recommendations
- `architecture` - Overview, components, data flow diagram
- `report` - Introduction, background, methodology, results

```c
OutputResult result;
output_from_template("analysis", "Security Analysis", NULL, &result);
```

## Consequences

### Positive

- **Consistent output** - All agents use same format
- **Rich content** - Mermaid diagrams, tables
- **Discoverable** - Organized by date/project
- **Clickable** - OSC8 links in terminal
- **Cleanable** - Auto-cleanup old outputs

### Negative

- **Disk usage** - Files accumulate (mitigated by cleanup)
- **Learning curve** - New API for agents

### Risks

- **Mermaid rendering** - Not all viewers support it
- **OSC8 support** - Some terminals don't support links

## Testing

Run tests: `make output_service_test`

Coverage:
- Document creation (all formats)
- Mermaid diagram generation
- Table generation
- Template system
- File management
- OSC8 link generation

## Usage Examples

### Agent generating a report

```c
// Build content with table
const char* headers[] = {"Component", "Status", "Notes"};
const char* row1[] = {"Auth", "OK", "OAuth2 configured"};
const char* row2[] = {"API", "Warning", "Rate limits needed"};
const char** rows[] = {row1, row2, NULL};
char* table = output_table_simple(headers, 3, (const char***)rows, 2);

// Build content with diagram
char* diagram = output_mermaid_flowchart("System Flow", "LR",
    (const char*[]){"A[Client]", "B[API]", "C[DB]", NULL},
    (const char*[]){"A --> B", "B --> C", NULL});

// Create document
char content[4096];
snprintf(content, sizeof(content),
    "## Status\n\n%s\n\n## Architecture\n\n```mermaid\n%s```\n",
    table, diagram);

OutputRequest req = {
    .title = "System Status",
    .content = content,
    .agent_name = "baccio",
    .include_timestamp = true
};
OutputResult result;
output_create(&req, &result);

// Show link to user
output_print_link(result.filepath, "Status Report");

free(table);
free(diagram);
```

## Future Work

1. **Agent integration** - Add output_service calls to key agents
2. **CLI commands** - `convergio output list`, `convergio output open`
3. **PDF export** - Optional PDF generation via external tool
4. **Web preview** - Local server to preview outputs
