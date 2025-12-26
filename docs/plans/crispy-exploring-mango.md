# Runtime Edition Switching Implementation Plan

## Overview

Implement runtime edition switching for ConvergioCLI while keeping Education edition compile-time only (per ADR-003).

**Tasks**: R01-R06 from master plan

## Current State

- Edition is compile-time via `#define CONVERGIO_EDITION_*`
- `edition_current()` returns a constant
- No runtime switching capability
- Education must remain compile-time for child safety

## Implementation Steps

### R01: Refactor edition selection to runtime

**File: `src/core/edition.c`**

1. Replace compile-time constant with mutable global:
```c
// Before: #define CURRENT_EDITION EDITION_MASTER
// After:
static ConvergioEdition g_current_edition = EDITION_MASTER;
#ifdef CONVERGIO_EDITION_EDUCATION
static const bool g_edition_locked = true;  // Cannot change at runtime
#else
static const bool g_edition_locked = false;
#endif
```

2. Add setter function:
```c
bool edition_set(ConvergioEdition edition) {
    if (g_edition_locked) return false;  // Education binary = locked
    if (edition == EDITION_EDUCATION) return false;  // Cannot switch TO education
    g_current_edition = edition;
    return true;
}
```

3. Modify `edition_current()` to return the mutable global

### R02: Add `--edition` CLI flag

**File: `src/core/main.c`** (around line 280)

```c
} else if ((strcmp(argv[i], "--edition") == 0 || strcmp(argv[i], "-e") == 0) && i + 1 < argc) {
    const char* ed = argv[++i];
    if (strcmp(ed, "master") == 0) edition_set(EDITION_MASTER);
    else if (strcmp(ed, "business") == 0) edition_set(EDITION_BUSINESS);
    else if (strcmp(ed, "developer") == 0) edition_set(EDITION_DEVELOPER);
    else {
        fprintf(stderr, "Unknown edition: %s\n", ed);
        return 1;
    }
}
```

Add to help text in `show_help()`.

### R03: Add `CONVERGIO_EDITION` env var

**File: `src/core/config.c`** (in `convergio_config_load()`)

```c
// Check env var (priority over config file)
const char* env_edition = getenv("CONVERGIO_EDITION");
if (env_edition && strlen(env_edition) > 0) {
    if (strcmp(env_edition, "master") == 0) edition_set(EDITION_MASTER);
    else if (strcmp(env_edition, "business") == 0) edition_set(EDITION_BUSINESS);
    else if (strcmp(env_edition, "developer") == 0) edition_set(EDITION_DEVELOPER);
}
```

### R04: Add edition to config.toml

**File: `include/nous/config.h`**

Add field to ConvergioConfig:
```c
char edition[32];  // "master", "business", "developer"
```

**File: `src/core/config.c`**

1. Add to `parse_config_line()` in [ui] section:
```c
} else if (strcmp(key, "edition") == 0) {
    strncpy(g_config.edition, value, sizeof(g_config.edition) - 1);
}
```

2. Add to `set_defaults()`:
```c
strncpy(g_config.edition, "master", sizeof(g_config.edition) - 1);
```

3. Add to `convergio_config_save()`:
```c
fprintf(f, "edition = \"%s\"\n", g_config.edition[0] ? g_config.edition : "master");
```

4. Apply config edition in load (before env var check):
```c
if (strlen(g_config.edition) > 0) {
    // Parse and set edition from config
}
```

### R05: Hot-reload agent registry on switch

**File: `src/core/edition.c`**

```c
bool edition_set(ConvergioEdition edition) {
    if (g_edition_locked) return false;
    if (edition == EDITION_EDUCATION) return false;
    if (edition == g_current_edition) return true;  // No change

    g_current_edition = edition;

    // Trigger registry reload
    edition_notify_change();
    return true;
}

void edition_notify_change(void) {
    // Callback to registry to reload agent list
    extern void agent_registry_reload(void);
    agent_registry_reload();
}
```

**File: `src/orchestrator/registry.c`**

Add `agent_registry_reload()`:
```c
void agent_registry_reload(void) {
    CONVERGIO_MUTEX_LOCK(&g_registry_mutex);

    // Clear current agents
    for (size_t i = 0; i < orchestrator->agent_count; i++) {
        agent_destroy(orchestrator->agents[i]);
    }
    orchestrator->agent_count = 0;

    // Clear hash tables
    agent_hash_clear(orchestrator->agent_by_id);
    agent_hash_clear(orchestrator->agent_by_name);

    // Reload with new edition filtering
    agent_load_definitions(NULL);

    // Regenerate Ali's system prompt with new agent list
    load_agent_list();

    CONVERGIO_MUTEX_UNLOCK(&g_registry_mutex);
}
```

Add edition filtering to `agent_load_definitions()`:
```c
// After parsing agent, check edition whitelist
if (!edition_has_agent(agent->id)) {
    agent_destroy(agent);
    continue;
}
```

### R06: Update documentation

**Files to update**:
- `--help` output in `show_help()` (main.c)
- README.md CLI section
- editions/README.md

Help text addition:
```
  --edition, -e <name>  Set edition (master, business, developer)
                        Can also use CONVERGIO_EDITION env var
                        or edition in config.toml
```

## Priority Order

1. Environment variable priority: CLI > env var > config.toml
2. Education edition: Always compile-time locked, cannot switch to/from

## Files to Modify

| File | Changes |
|------|---------|
| `include/nous/edition.h` | Add `edition_set()`, `edition_notify_change()` declarations |
| `src/core/edition.c` | Mutable global, setter, notify callback |
| `include/nous/config.h` | Add `edition[32]` field |
| `src/core/config.c` | Parse/save edition, env var check |
| `src/core/main.c` | CLI flag parsing, help text |
| `src/orchestrator/registry.c` | `agent_registry_reload()`, edition filtering |

## Testing

1. `convergio --edition business` - Should only show business agents
2. `CONVERGIO_EDITION=developer convergio` - Should only show dev agents
3. Add `edition = "business"` to config.toml - Should persist
4. `convergio-edu --edition master` - Should fail (locked)
5. Hot-reload: Change edition mid-session, verify agent list updates

## Security Considerations

- Education binary cannot switch editions (compile-time lock)
- Cannot switch TO education at runtime (prevents bypass)
- Whitelists enforced at usage time, not just display time

---

## Distribution (D05 + D03)

### GitHub Actions Release Workflow

**File: `.github/workflows/release.yml`**

```yaml
name: Release
on:
  push:
    tags: ['v*']

jobs:
  build:
    strategy:
      matrix:
        edition: [master, education]
        os: [macos-14]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
      - name: Build ${{ matrix.edition }}
        run: make EDITION=${{ matrix.edition }}
      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: convergio-${{ matrix.edition }}-${{ matrix.os }}
          path: build/bin/convergio*

  release:
    needs: build
    runs-on: ubuntu-latest
    steps:
      - uses: actions/download-artifact@v4
      - name: Create Release
        run: gh release create $TAG --generate-notes ./convergio-*/*
```

### Homebrew Tap

**Repository: `github.com/Roberdan/homebrew-convergio`**

```
homebrew-convergio/
├── Formula/
│   ├── convergio.rb      # Master edition
│   └── convergio-edu.rb  # Education edition
```

**Formula/convergio.rb:**
```ruby
class Convergio < Formula
  desc "AI-powered CLI with 60+ specialized agents"
  homepage "https://github.com/Roberdan/convergio-cli"
  version "5.4.0"

  on_macos do
    on_arm do
      url "https://github.com/Roberdan/convergio-cli/releases/download/v#{version}/convergio-#{version}-macos-arm64.tar.gz"
      sha256 "..."
    end
    on_intel do
      url "https://github.com/Roberdan/convergio-cli/releases/download/v#{version}/convergio-#{version}-macos-x64.tar.gz"
      sha256 "..."
    end
  end

  def install
    bin.install "convergio"
  end
end
```

**Installation:**
```bash
brew tap roberdan/convergio
brew install convergio      # Master
brew install convergio-edu  # Education
```
