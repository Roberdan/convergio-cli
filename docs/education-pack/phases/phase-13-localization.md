# Phase 13: Localization

**Status**: Not Started
**Priority**: P1
**Dependencies**: Phase 12 (Storytelling complete with English master)

---

## Objective

Ensure Convergio can be fully localized in multiple languages, starting with English (master version) and Italian.

---

## Architecture

### L10N-01: Locale System Design

```
locales/
├── en/                    # English (master)
│   ├── common.json        # Common UI strings
│   ├── education.json     # Education-specific strings
│   ├── maestri.json       # Teacher dialogues and prompts
│   ├── accessibility.json # A11y messages
│   └── errors.json        # Error messages
├── it/                    # Italian
│   ├── common.json
│   ├── education.json
│   ├── maestri.json
│   ├── accessibility.json
│   └── errors.json
└── locale_config.json     # Locale configuration
```

### L10N-02: String Extraction

All user-facing strings must be extracted to locale files:

1. **C Strings**: Use `_()` macro for all UI strings
2. **Agent Prompts**: Store in locale-specific JSON files
3. **Error Messages**: Centralize in errors.json
4. **Celebration Messages**: Locale-aware in accessibility.json

### L10N-03: Locale Detection

```c
typedef enum {
    LOCALE_EN,    // English (default/master)
    LOCALE_IT,    // Italian
    LOCALE_COUNT
} ConvergioLocale;

ConvergioLocale locale_detect(void);
const char* locale_get_string(const char* key);
void locale_set(ConvergioLocale locale);
```

---

## Tasks

### P0 - Essential

| ID | Task | Status | Notes |
|----|------|--------|-------|
| L10N-01 | Create locale directory structure | [ ] | locales/en, locales/it |
| L10N-02 | Extract English strings to JSON | [ ] | Master version |
| L10N-03 | Implement locale loading in C | [ ] | cJSON-based |
| L10N-04 | Add _() macro for string lookup | [ ] | Similar to gettext |
| L10N-05 | Locale detection from system | [ ] | LANG env var |

### P1 - Important

| ID | Task | Status | Notes |
|----|------|--------|-------|
| L10N-06 | Italian translations | [ ] | All JSON files |
| L10N-07 | Locale switch command | [ ] | /locale [en|it] |
| L10N-08 | Agent prompt localization | [ ] | 15 maestri prompts |
| L10N-09 | Curriculum locale awareness | [ ] | Italian curricula in IT locale |
| L10N-10 | Date/time formatting | [ ] | Locale-specific |

### P2 - Nice to Have

| ID | Task | Status | Notes |
|----|------|--------|-------|
| L10N-11 | Right-to-left support prep | [ ] | For future Arabic/Hebrew |
| L10N-12 | Pluralization rules | [ ] | 1 item, 2 items, etc. |
| L10N-13 | Number formatting | [ ] | 1,000 vs 1.000 |
| L10N-14 | Translation contribution guide | [ ] | How to add new locales |

---

## Implementation Notes

### String Key Convention

```
category.subcategory.key_name

Examples:
- ui.welcome.title
- maestro.socrates.greeting
- error.network.timeout
- a11y.celebration.level1
```

### Fallback Chain

1. Try requested locale (e.g., `it`)
2. Fall back to English (`en`)
3. Return key name as last resort

### Code Migration Path

1. Mark all strings with `_()` macro
2. Run extraction script to generate JSON
3. Verify English JSON is complete
4. Create Italian translations
5. Test both locales end-to-end

---

## Tests

| ID | Test | Status |
|----|------|--------|
| L10N-T01 | Locale detection from LANG | [ ] |
| L10N-T02 | String lookup returns correct locale | [ ] |
| L10N-T03 | Fallback to English works | [ ] |
| L10N-T04 | Missing key returns key name | [ ] |
| L10N-T05 | Locale switch at runtime | [ ] |
| L10N-T06 | All English strings present | [ ] |
| L10N-T07 | All Italian strings present | [ ] |

---

## Definition of Done

- [ ] All user-facing strings in locale JSON files
- [ ] English (master) 100% complete
- [ ] Italian 100% complete
- [ ] Locale detection works automatically
- [ ] /locale command switches language
- [ ] All 15 maestri have localized prompts
- [ ] Tests pass for both locales
- [ ] Documentation updated

---

*Phase added: 2025-12-20*
*Author: Roberto with AI agent team support*
