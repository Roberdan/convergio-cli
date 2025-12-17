# ADR-014: Advanced Filesystem Tools

**Date**: 2025-12-16
**Status**: Approved
**Author**: AI Team

## Context

Convergio CLI originally provided basic file tools (`file_read`, `file_write`, `file_list`, `shell_exec`) that required the AI to use shell commands for advanced filesystem operations. This approach had several limitations:

1. **Unsafe operations**: Using `rm` commands could permanently delete files without recovery
2. **Inconsistent patterns**: Different ways to search files (find vs ls vs glob)
3. **No backup protection**: File modifications had no rollback capability
4. **Verbose shell commands**: Simple operations required complex shell pipelines

Inspiration from Claude Code showed that dedicated filesystem tools provide:
- Better safety guarantees
- Cleaner AI interactions
- More predictable results
- Automatic safety features (trash, backups)

## Decision

### Implement Four Advanced Filesystem Tools

#### 1. `glob` - Pattern Matching Files
```json
{
  "name": "glob",
  "description": "Find files matching glob pattern (e.g., **/*.c)",
  "parameters": {
    "pattern": "Glob pattern with ** for recursive",
    "path": "Starting directory (default: workspace)",
    "max_results": "Max files to return (default: 100)"
  }
}
```

Features:
- Supports `**` for recursive directory traversal
- Results sorted by modification time (newest first)
- Respects workspace boundaries (path safety)
- Custom implementation without external dependencies

#### 2. `grep` - Search File Contents
```json
{
  "name": "grep",
  "description": "Search file contents with regex",
  "parameters": {
    "pattern": "Regex pattern to search",
    "path": "File or directory to search",
    "glob": "Optional glob filter (e.g., *.c)",
    "context_before": "Lines before match (default: 0)",
    "context_after": "Lines after match (default: 0)",
    "ignore_case": "Case insensitive (default: false)",
    "output_mode": "content|files_with_matches|count",
    "max_matches": "Max results (default: 50)"
  }
}
```

Features:
- Uses `ripgrep` when available, falls back to `grep`
- Multiple output modes for different use cases
- Context lines support for understanding matches
- Glob filtering to narrow search scope

#### 3. `edit` - Precise String Replacement
```json
{
  "name": "edit",
  "description": "Edit file by replacing exact string",
  "parameters": {
    "path": "File to edit",
    "old_string": "Exact string to find (must be unique)",
    "new_string": "Replacement string"
  }
}
```

Safety features:
- **Automatic backup**: Before any edit, creates backup in `~/.convergio/backups/`
- **Uniqueness check**: Fails if `old_string` matches multiple locations
- **Atomic writes**: Writes to temp file, then renames for crash safety

#### 4. `file_delete` - Safe File Deletion
```json
{
  "name": "file_delete",
  "description": "Safely delete file (moves to Trash)",
  "parameters": {
    "path": "File to delete",
    "permanent": "Skip trash (default: false, NOT RECOMMENDED)"
  }
}
```

Safety features:
- **macOS Trash integration**: Uses Finder via AppleScript
- **Fallback trash**: If Finder fails, moves to `~/.convergio/trash/`
- **Timestamped names**: Trash files include timestamp to prevent collisions

### Direct Bash Prefix

Added shortcut for direct shell execution:
- `!ls -la` or `$ls -la` → executes directly without AI processing
- Safety checks still applied (blocked commands list)
- Immediate feedback without AI overhead

### Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    TOOL SAFETY LAYERS                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Layer 1: Path Safety                                           │
│   └─ tools_is_path_safe() - workspace boundary check             │
│                                                                  │
│   Layer 2: Backup Protection (edit tool)                         │
│   └─ backup_before_edit() - ~/.convergio/backups/                │
│                                                                  │
│   Layer 3: Trash Protection (file_delete tool)                   │
│   └─ move_to_trash() - macOS Finder or ~/.convergio/trash/       │
│                                                                  │
│   Layer 4: Command Safety (shell_exec, direct bash)              │
│   └─ tools_is_command_safe() - blocklist check                   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Backup System

```c
// Backup location: ~/.convergio/backups/{filename}.{timestamp}.bak
char* backup_before_edit(const char* path) {
    // 1. Create backup directory if needed
    // 2. Generate timestamped backup name
    // 3. Copy file to backup location
    // 4. Return backup path for undo capability
}
```

### Trash System

```c
int move_to_trash(const char* path) {
    // Option 1: macOS Finder integration
    osascript -e 'tell app "Finder" to delete POSIX file "/path/to/file"'

    // Option 2: Fallback to internal trash
    // Move to ~/.convergio/trash/{timestamp}_{basename}
}
```

### Ali System Prompt Update

Added to Ali's instructions:
- Prefer `glob` over shell `find`
- Prefer `grep` over shell `grep`
- Prefer `edit` over `file_write` for modifications
- Prefer `file_delete` over shell `rm`

## Consequences

### Positive
- Files are protected from accidental permanent deletion
- Edits can be recovered from backups
- Cleaner tool usage patterns
- Better safety guarantees

### Negative
- Disk space usage for backups and trash
- Slight overhead for backup/trash operations

### Mitigations
- Backup retention policy (future): limit to last 50 per file
- Trash cleanup policy (future): auto-purge after 30 days

## Files Changed

| File | Changes |
|------|---------|
| `include/nous/tools.h` | Added TOOL_GLOB, TOOL_GREP, TOOL_EDIT, TOOL_FILE_DELETE |
| `src/tools/tools.c` | Implemented all four tools + helpers |
| `src/core/repl.c` | Added direct bash prefix (! or $) |
| `src/orchestrator/orchestrator.c` | Updated Ali system prompt |

## Testing

All existing tests pass. New tools tested manually:
- `glob`: Pattern matching works recursively
- `grep`: Uses ripgrep when available, falls back to grep
- `edit`: Creates backups, enforces uniqueness
- `file_delete`: Moves to Trash successfully

## Future Enhancements

1. **Undo system**: Command to restore from backup/trash
2. **Backup cleanup**: Automatic old backup purging
3. **Trash management**: `/trash` command to view/restore
4. **Dry-run mode**: Preview operations before executing
