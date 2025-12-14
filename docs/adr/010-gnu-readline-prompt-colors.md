# ADR-010: GNU Readline for Prompt Colors

## Status
**Accepted** - December 2025

## Context

The Convergio CLI uses a colored, bold prompt to display the current state:
```
Convergio (Ali) >
```

On macOS, the default `readline` library is actually **libedit** (BSD licensed), not GNU readline (GPL). This caused a critical issue: **colored bold prompts did not work**.

### The Problem

Readline prompts with ANSI escape codes require special markers to tell the library which characters are "non-printing" (zero-width). Without these markers, readline miscalculates the prompt length, causing:
- Cursor positioning errors
- Line wrapping issues
- Display corruption on history navigation

The markers are:
- `\001` (RL_PROMPT_START_IGNORE) - start of non-printing sequence
- `\002` (RL_PROMPT_END_IGNORE) - end of non-printing sequence

Example:
```c
"\001\033[1m\033[38;5;208m\002Convergio (Ali) >\001\033[0m\002 "
//    ^-- bold + color --^                      ^-- reset --^
```

### libedit vs GNU readline

| Feature | GNU readline | libedit (macOS default) |
|---------|--------------|------------------------|
| `\001`/`\002` markers | Supported | **Ignored** |
| Colored prompts | Work correctly | Broken |
| License | GPL | BSD |

Apple ships libedit because GPL licensing is incompatible with proprietary software distribution. However, libedit's readline compatibility layer does not implement the `EL_PROMPT_ESC` mode needed for escape sequence handling.

References:
- [Hackzine Wiki - Color prompts with readline](https://wiki.hackzine.org/development/misc/readline-color-prompt.html)
- [Python cpython issue #113533](https://github.com/python/cpython/issues/113533)
- [Ruby bug #9204](https://bugs.ruby-lang.org/issues/9204)

## Decision

**Use GNU readline from Homebrew instead of system libedit.**

### Makefile Configuration

```makefile
# GNU Readline from Homebrew (NOT libedit - libedit doesn't support \001\002 color markers)
READLINE_PREFIX = $(shell brew --prefix readline)

CFLAGS = ... -I$(READLINE_PREFIX)/include ...

LIBS = -L$(READLINE_PREFIX)/lib -lreadline ...
```

### Prompt Format

The prompt uses a single color for the entire text, with bold applied once at the start:

```c
// Structure: [BOLD][COLOR]entire prompt text[RESET]
snprintf(prompt, sizeof(prompt),
    "\001\033[1m%s\002Convergio (%s) >\001\033[0m\002 ",
    t->prompt_name, agents_str);
```

Where `t->prompt_name` is the theme's color code (e.g., `\033[38;5;208m` for orange).

**Important**: Do NOT reset colors mid-prompt. This breaks the bold attribute.

## Consequences

### Positive
- Colored bold prompts work correctly
- Cursor positioning is accurate
- History navigation displays properly
- Themes can use any 256-color palette

### Negative
- Requires Homebrew `readline` package as build dependency
- Binary is linked against `/opt/homebrew/opt/readline/lib/libreadline.8.dylib`
- Users must have Homebrew readline installed (handled by `brew bundle`)

### Verification

Check linkage with:
```bash
otool -L build/bin/convergio | grep readline
# Should show: /opt/homebrew/opt/readline/lib/libreadline.8.dylib
# NOT: /usr/lib/libedit.3.dylib
```

## How to Modify the Prompt

### Adding New Elements

Edit `src/core/main.c` in the prompt building section:

```c
// With project:
"\001\033[1m%s\002Convergio (%s) [%s] >\001\033[0m\002 "

// Without project:
"\001\033[1m%s\002Convergio (%s) >\001\033[0m\002 "
```

Rules:
1. Wrap ALL ANSI codes in `\001...\002`
2. Put `\033[1m` (bold) at the start, before the color
3. Put `\033[0m` (reset) only at the very end
4. Keep visible text OUTSIDE the `\001...\002` markers

### Theme Colors

Theme colors are defined in `src/core/theme.c`:

```c
#define COLOR256(n) "\033[38;5;" #n "m"
#define BOLD_COLOR256(n) "\033[1;38;5;" #n "m"

static const Theme THEMES[THEME_COUNT] = {
    // THEME_SUNSET
    {
        .name = "Sunset",
        .prompt_name = BOLD_COLOR256(208),  // Orange
        // ...
    },
};
```

The `prompt_name` field is used in the prompt format string.

## Related ADRs

- ADR-001: Persistence Layer (SQLite)
- ADR-006: Multi-Provider Architecture
