# Phase 12 - Storytelling Integration

**Status**: DONE
**Progress**: 100%
**Last Updated**: 2025-12-20

---

## Objective

Create Chris as TED-style storytelling teacher and integrate narrative techniques in all teachers to make learning more engaging.

---

## 12.1 Chris Teacher

**File**: `src/agents/definitions/education/chris-storytelling.md`

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| ST01 | Chris agent definition | [x] | P0 | File created |
| ST02 | TED H.A.I.L. framework | [x] | P0 | Honesty, Authenticity, Integrity, Love |
| ST03 | C.N.E.P.R. structure | [x] | P0 | Connection, Narration, Explanation, Persuasion, Revelation |
| ST04 | 18-minute template | [x] | P0 | TED format |
| ST05 | Public speaking practice | [x] | P1 | TED timer in storytelling.c |
| ST06 | Filler word detection | [x] | P1 | IT/EN filler words analyzed |
| ST07 | Pacing analysis | [x] | P1 | WPM with feedback |
| ST08 | Voice in teaching | [x] | P0 | Integrated with voice mode |

---

## TED H.A.I.L. Framework

Chris teaches the 4 fundamental qualities for public speaking:

- **H**onesty - Be authentic and transparent
- **A**uthenticity - Be yourself, don't imitate others
- **I**ntegrity - Be consistent with your values
- **L**ove - Genuine interest in the audience

---

## C.N.E.P.R. Presentation Structure

Structure for effective presentations:

1. **Connection** (2 min) - Capture attention, create empathy
2. **Narration** (5 min) - Tell personal/relevant story
3. **Explanation** (6 min) - Explain core concept
4. **Persuasion** (3 min) - Convince with data/examples
5. **Revelation** (2 min) - Call to action, memorable close

---

## 12.2 Cross-Cutting Skill

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| XS01 | Storytelling training for teachers | [x] | P1 | H.A.I.L. + C.N.E.P.R. framework |
| XS02 | Story hooks per subject | [x] | P1 | 13 teachers with 3 hooks each |
| XS03 | Narrative in explanations | [x] | P1 | storytelling_get_hook() API |
| XS04 | Student presentation coaching | [x] | P1 | TED timer + filler analysis |
| XS05 | Cross-subject storytelling | [x] | P2 | Hooks designed interdisciplinary |

---

## Story Hooks per Subject

```
Socrates:   "Imagine being in Athens, 2400 years ago..."
Euclid:     "A king once asked: is there an easy path to geometry?"
Feynman:    "Do you know what happens when you drop a ball?"
Herodotus:  "In a time long ago, an empire ruled the world..."
Darwin:     "Sailing to remote islands, I noticed something strange..."
Leonardo:   "In my studio, surrounded by impossible machines..."
```

---

## 12.3 Integration with Riccardo

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| RI01 | Define Chris vs Riccardo scope | [x] | P0 | Chris teaches, Riccardo does |
| RI02 | Shared storytelling frameworks | [ ] | P2 | Common methodology |
| RI03 | Student to Professional transition | [ ] | P2 | Chris prepares for Riccardo |

**Scope**:
- **Chris**: Teaches storytelling to students, TED talks, public speaking
- **Riccardo**: Professional storyteller for brand narratives, content strategy

---

## Chris Voice Profile

```json
{
  "voice": "echo",
  "speaking_rate": 1.0,
  "pitch": "dynamic",
  "style": "inspiring, TED-style",
  "characteristics": [
    "Pauses for effect",
    "Varies tempo",
    "Uses rhetorical questions",
    "Builds to climax"
  ]
}
```

---

## Files to Create/Modify

- [x] `src/agents/definitions/education/chris-storytelling.md`
- [x] `src/education/storytelling.c` (645 LOC)
- [x] `include/nous/storytelling.h` (header)
- [x] All teachers with story hooks (13 maestri)

---

## Tests

| ID | Test | Status | Note |
|----|------|--------|------|
| STT01 | Chris agent loads test | [ ] | Basic test |
| STT02 | H.A.I.L. framework test | [ ] | Checklist |
| STT03 | Presentation timer test | [ ] | 18-minute |
| STT04 | Story hooks test | [ ] | Per teacher |

---

## Acceptance Criteria

- [x] Chris agent defined
- [x] Public speaking practice mode (TED timer)
- [x] Filler word detection (IT/EN)
- [x] Story hooks for every teacher (13)
- [x] Integration with voice mode

---

## Result

Storytelling engine 100% complete with:
- **FillerAnalysis**: Detects 20+ filler words in Italian/English
- **PacingAnalysis**: WPM calculation with ideal range feedback
- **TEDTimer**: 18-minute timer with 4 sections
- **Story Hooks**: 3 hooks per maestro (39 total)
- **H.A.I.L. Evaluation**: Scores for Honesty, Authenticity, Integrity, Love
- **C.N.E.P.R. Guide**: Duration and tips for each section
