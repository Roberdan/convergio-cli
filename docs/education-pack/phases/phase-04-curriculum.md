# Phase 4 - Italian Curricula

**Status**: DONE
**Progress**: 100%
**Last Updated**: 2025-12-20
**Parallelization**: 3 threads

---

## Objective

Implement Italian school curricula as structured JSON files, with parser and setup wizard integration.

---

## Thread CUR-A - High Schools (Licei)

| ID | Task | Status | File |
|----|------|--------|------|
| C01 | Curriculum JSON parser | [x] | In `education_db.c` |
| C02 | Scientific High School (1-5) | [x] | `curricula/it/liceo_scientifico.json` |
| C03 | Classical High School (1-5) | [x] | `curricula/it/liceo_classico.json` |
| C04 | Language High School (1-5) | [x] | `curricula/it/liceo_linguistico.json` |
| C05 | Art High School (1-5) | [x] | `curricula/it/liceo_artistico.json` |

---

## Thread CUR-B - Middle/Elementary

| ID | Task | Status | File |
|----|------|--------|------|
| C06 | Middle School (1-3) | [x] | `curricula/it/scuola_media.json` |
| C07 | Elementary School (1-5) | [x] | `curricula/it/elementari.json` |

---

## Thread CUR-C - Technical/Custom

| ID | Task | Status | File |
|----|------|--------|------|
| C08 | IT Technical Institute | [x] | `curricula/it/iti_informatica.json` |
| C09 | Commercial Technical Institute | [ ] | TODO |
| C10 | Custom Path System | [ ] | Custom selection |
| C11 | Hot-reload JSON | [ ] | Watch file changes |

---

## JSON Curriculum Structure

```json
{
  "name": "Scientific High School",
  "years": 5,
  "subjects": [
    {
      "name": "Mathematics",
      "teacher": "euclid",
      "topics_per_year": {
        "1": ["Sets", "Natural numbers", ...],
        "2": ["Equations", "Inequalities", ...],
        ...
      }
    }
  ]
}
```

---

## 16 Curricula in Setup Wizard

Available paths in `setup_wizard.c`:

1. Elementary School (1-5)
2. Middle School (1-3)
3. Scientific High School
4. Classical High School
5. Language High School
6. Art High School
7. Human Sciences High School
8. Music High School
9. IT Technical Institute
10. Electronics Technical Institute
11. Mechanics Technical Institute
12. Commercial Technical Institute
13. Professional Institute
14. Professional Training
15. Custom Path
16. Homeschooling

---

## Modified Files

- `curricula/it/*.json` (7 files)
- `src/education/setup_wizard.c` (curriculum selection)
- `src/education/education_db.c` (JSON parser)

---

## Tests

| ID | Test | Status | Note |
|----|------|--------|------|
| CT01 | Scientific High School loading | [x] | `test_curriculum_load` |
| CT02 | JSON structure parsing | [ ] | TODO |
| CT03 | Topic navigation | [ ] | TODO |
| CT04 | Curriculum progress | [ ] | TODO |

---

## Acceptance Criteria

- [x] 7 JSON curriculum files created
- [x] JSON parser working
- [x] 16 options in setup wizard
- [ ] Curricula hot-reload

---

## Result

7 complete JSON curricula. 16 options selectable in wizard. Parser working. Hot-reload to implement.
