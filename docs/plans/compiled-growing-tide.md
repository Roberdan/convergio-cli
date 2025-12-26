# Plan: Advanced Filesystem Tools for Convergio CLI

## Obiettivo
Aggiungere tool filesystem avanzati stile Claude Code a Convergio CLI, con sistemi di sicurezza robusti.

---

## 1. NUOVI TOOL DA IMPLEMENTARE

### 1.1 `glob` - Pattern Matching Files
```json
{
  "name": "glob",
  "description": "Find files matching glob pattern (e.g., **/*.c)",
  "properties": {
    "pattern": "Glob pattern",
    "path": "Starting directory (default: workspace)",
    "max_results": "Max files (default: 100)"
  }
}
```

### 1.2 `grep` - Search in Files
```json
{
  "name": "grep",
  "description": "Search file contents with regex",
  "properties": {
    "pattern": "Regex pattern",
    "path": "File or directory",
    "glob": "File filter (e.g., *.c)",
    "context_before": "Lines before match",
    "context_after": "Lines after match",
    "ignore_case": "Case insensitive",
    "output_mode": "content|files_with_matches|count"
  }
}
```

### 1.3 `edit` - Precise String Replacement
```json
{
  "name": "edit",
  "description": "Edit file by replacing exact string",
  "properties": {
    "path": "File to edit",
    "old_string": "String to find (must be unique)",
    "new_string": "Replacement string"
  }
}
```

### 1.4 `file_delete` - Safe File Deletion (NEW)
```json
{
  "name": "file_delete",
  "description": "Safely delete file (moves to Trash)",
  "properties": {
    "path": "File to delete",
    "permanent": "Skip trash (default: false, requires confirmation)"
  }
}
```

---

## 2. SISTEMI DI SICUREZZA

### 2.1 Trash Instead of Delete
- **macOS**: Usare `NSFileManager trashItemAtURL` via helper o `osascript`
- **Fallback**: Muovere a `~/.convergio/trash/` con timestamp
- **Permanent delete**: Solo con `permanent: true` + conferma utente

### 2.2 Backup Prima di Edit
- Prima di ogni `edit`, creare backup in `~/.convergio/backups/`
- Nome: `{filename}.{timestamp}.bak`
- Retention: ultimi 50 backup per file

### 2.3 Confirmation per Operazioni Distruttive
- `file_delete` con `permanent: true` → richiede conferma
- `edit` su file di sistema → richiede conferma
- `shell_exec` con comandi rm/mv → warning

### 2.4 Dry Run Mode
- Flag `--dry-run` per vedere cosa farebbe senza eseguire
- Output: "Would delete: file.txt" invece di eseguire

### 2.5 Undo System
- Salvare log operazioni in `~/.convergio/undo_log.json`
- Comando `/undo` per annullare ultima operazione
- Struttura: `{action, path, backup_path, timestamp}`

---

## 3. DIRECT BASH PREFIX

### Sintassi
- `!comando` o `$comando` → esegue bash direttamente
- Esempio: `!ls -la` esegue senza passare da AI

### Sicurezza
- Default: richiede conferma (`direct_bash_confirm = true`)
- Comandi distruttivi (rm, mv, dd) → sempre conferma
- Blocklist esistente applicata anche qui

### Implementazione
In `src/core/repl.c`:
```c
if (input[0] == '!' || input[0] == '$') {
    return repl_execute_direct_bash(input + 1);
}
```

---

## 4. AGGIORNAMENTO SYSTEM PROMPT ALI

Aggiungere a `ALI_SYSTEM_PROMPT_TEMPLATE`:
```
### Advanced File Tools
- **glob**: Find files by pattern (prefer over shell find)
- **grep**: Search contents with regex (prefer over shell grep)
- **edit**: Precise modifications with old/new string
- **file_delete**: Safe deletion (uses Trash)

**Best Practices:**
1. Use glob/grep instead of shell commands when possible
2. Use edit for modifications, not file_write (more precise)
3. Always confirm destructive operations with user
```

---

## 5. FILE DA MODIFICARE

### Phase 1: Core Infrastructure
| File | Modifiche |
|------|-----------|
| `include/nous/tools.h` | Add TOOL_GLOB, TOOL_GREP, TOOL_EDIT, TOOL_FILE_DELETE |
| `src/tools/tools.c` | Implement tool_glob(), tool_grep(), tool_edit(), tool_file_delete() |
| `src/tools/tools.c` | Add JSON definitions to TOOLS_JSON |
| `src/tools/tools.c` | Add backup_file(), move_to_trash() helpers |

### Phase 2: Safety Systems
| File | Modifiche |
|------|-----------|
| `src/tools/tools.c` | Implement trash system (macOS + fallback) |
| `src/tools/tools.c` | Implement backup system |
| `src/core/undo.c` (NEW) | Undo log management |
| `include/nous/undo.h` (NEW) | Undo declarations |

### Phase 3: REPL Integration
| File | Modifiche |
|------|-----------|
| `src/core/repl.c` | Add direct bash prefix detection |
| `src/core/repl.c` | Add /undo command |
| `src/orchestrator/orchestrator.c` | Update Ali system prompt |

### Phase 4: Config & Testing
| File | Modifiche |
|------|-----------|
| `src/core/config.c` | Add direct_bash_confirm, backup_enabled options |
| `tests/test_tools.c` | Add tests for new tools |
| `docs/ADR-014-advanced-tools.md` | Document architecture |

---

## 6. IMPLEMENTAZIONE DETTAGLIATA

### 6.1 move_to_trash() - macOS
```c
static int move_to_trash(const char* path) {
    // Option 1: AppleScript (più affidabile)
    char cmd[PATH_MAX + 128];
    snprintf(cmd, sizeof(cmd),
        "osascript -e 'tell app \"Finder\" to delete POSIX file \"%s\"'",
        path);
    return system(cmd);

    // Option 2: Fallback a directory interna
    // mv path ~/.convergio/trash/{timestamp}_{basename}
}
```

### 6.2 backup_before_edit()
```c
static char* backup_before_edit(const char* path) {
    char backup_dir[PATH_MAX];
    snprintf(backup_dir, sizeof(backup_dir), "%s/.convergio/backups", getenv("HOME"));
    mkdir(backup_dir, 0755);

    time_t now = time(NULL);
    char backup_path[PATH_MAX];
    snprintf(backup_path, sizeof(backup_path), "%s/%s.%ld.bak",
             backup_dir, basename(path), now);

    // Copy file
    copy_file(path, backup_path);
    return strdup(backup_path);
}
```

### 6.3 tool_edit() con backup
```c
ToolResult* tool_edit(const char* path, const char* old_str, const char* new_str) {
    // 1. Safety check
    if (!tools_is_path_safe(path)) return result_error("Path not allowed");

    // 2. Read file
    char* content = read_file(path);

    // 3. Find occurrences
    int count = count_occurrences(content, old_str);
    if (count == 0) return result_error("String not found");
    if (count > 1) return result_error("Multiple matches - provide more context");

    // 4. Create backup BEFORE modification
    char* backup = backup_before_edit(path);

    // 5. Replace and write atomically
    char* new_content = replace_string(content, old_str, new_str);
    write_file_atomic(path, new_content);  // write to .tmp, then rename

    // 6. Log for undo
    undo_log_add("edit", path, backup);

    return result_success("File edited (backup: %s)", backup);
}
```

---

## 7. PRIORITÀ IMPLEMENTAZIONE

1. **Alta**: glob, grep, edit (core functionality)
2. **Alta**: Trash system per file_delete
3. **Media**: Backup system per edit
4. **Media**: Direct bash prefix
5. **Bassa**: Undo system completo
6. **Bassa**: Dry-run mode

---

## 8. STIMA EFFORT

- Phase 1 (Core tools): ~400 LOC
- Phase 2 (Safety): ~200 LOC
- Phase 3 (REPL): ~100 LOC
- Phase 4 (Config/Test): ~150 LOC
- **Totale**: ~850 LOC

---

## Approvazione Richiesta

Confermi questo piano? Posso procedere con l'implementazione in ordine di priorità.
