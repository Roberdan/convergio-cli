# Phase 7 - Coordination

**Status**: DONE
**Progress**: 100%
**Last Updated**: 2025-12-20

---

## Objective

Implement Ali as "principal" of the virtual class council and inter-teacher communication for interdisciplinary projects.

---

## Ali as Principal

**File**: `src/education/ali_preside.c` (754 LOC)

| ID | Task | Status | Note |
|----|------|--------|------|
| AL01 | Ali principal role extension | [x] | New education role |
| AL02 | Student dashboard | [x] | `preside_get_dashboard()` |
| AL03 | Virtual class council | [x] | `preside_prepare_class_council()` |
| AL04 | Auto weekly report | [x] | `preside_generate_weekly_report()` |
| AL05 | Difficult cases management | [x] | `preside_detect_difficult_case()` |
| AL06 | Parent communication | [x] | `preside_generate_parent_message()` |

---

## Teacher Communication

| ID | Task | Status | Note |
|----|------|--------|------|
| CM01 | Shared student context | [x] | `preside_get_shared_context()` |
| CM02 | Cross-subject signals | [x] | `preside_suggest_interdisciplinary()` |
| CM03 | Interdisciplinary projects | [x] | Integrated in CM02 |

---

## Coordination Architecture

```
                    +---------------------+
                    |   ALI (Principal)   |
                    |  Dashboard + Report |
                    +---------+-----------+
                              |
         +--------------------+--------------------+
         |                    |                    |
         v                    v                    v
+----------------+  +----------------+  +----------------+
| JENNY (A11y)   |  | ANNA (Remind)  |  | MARCUS (Mem)   |
+----------------+  +----------------+  +----------------+
         |                    |                    |
         +--------------------+--------------------+
                              |
                              v
                    +-------------------+
                    |  STUDENT PROFILE  |
                    +---------+---------+
                              |
         +------+------+------+------+------+------+
         v      v      v      v      v      v      v
       ED01   ED02   ED03   ...   ED14   ED15
```

---

## Principal Functions

### `preside_get_dashboard()`

Returns:
- XP/level statistics
- Current streak
- Best/worst performing subjects
- Recommended next topics
- Alerts (overdue homework, review needed)

### `preside_prepare_class_council()`

Prepares class council report with:
- Overview of progress per subject
- Identified strengths
- Areas for improvement
- Pedagogical recommendations

### `preside_detect_difficult_case()`

Detects:
- Sudden performance drop
- Repeatedly broken streaks
- Declining study time
- Subject avoidance patterns

### `preside_suggest_interdisciplinary()`

Suggests connections:
- History + Literature (e.g., Risorgimento + Manzoni)
- Math + Physics (e.g., Calculus + Mechanics)
- Art + History (e.g., Renaissance)

---

## Modified Files

- `src/education/ali_preside.c` (new, 754 LOC)
- `src/agents/definitions/ali-chief-of-staff.md` (principal role)

---

## Tests

| ID | Test | Status | Note |
|----|------|--------|------|
| COT01 | Ali dashboard test | [x] | Compiles |
| COT02 | Class council test | [x] | Compiles |
| COT03 | Shared context test | [x] | Compiles |
| COT04 | Weekly report test | [x] | Compiles |

---

## Acceptance Criteria

- [x] Ali has full view of each student
- [x] Dashboard with key metrics
- [x] Virtual class council working
- [x] Automated parent communication
- [x] Interdisciplinary suggestions

---

## Result

Ali principal fully operational with 754 LOC. Dashboard, class council, report, parent communication all working.
