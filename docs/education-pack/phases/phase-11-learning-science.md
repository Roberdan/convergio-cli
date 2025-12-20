# Phase 11 - Learning Science

**Status**: TODO
**Progress**: 0%
**Last Updated**: 2025-12-20

---

## Objective

Implement learning science techniques based on Khan Academy (mastery learning) and Duolingo (engagement engineering) to maximize learning effectiveness.

---

## Inspiration

- **Khan Academy**: Mastery learning, skill tracking, personalized paths
- **Duolingo**: FSRS algorithm, streaks, gamification, variable rewards

---

## 11.1 Mastery Learning (Khan Academy)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| ML01 | Skill mastery tracking | [ ] | P0 | 80% correct = mastered |
| ML02 | Skill gaps detection | [ ] | P0 | AI identifies weak areas |
| ML03 | Adaptive difficulty | [ ] | P0 | Harder if doing well |
| ML04 | Visual skill tree | [ ] | P1 | Subject -> Topic -> Skill |
| ML05 | Prerequisite enforcement | [ ] | P1 | Can't skip foundations |
| ML06 | Learning path recommendation | [ ] | P0 | "Next: Fractions" |
| ML07 | Progress visualization | [ ] | P0 | Clear mastery indicators |

---

## 11.2 Engagement Engineering (Duolingo)

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| EE01 | FSRS spaced repetition | [ ] | P0 | 2024 algorithm |
| EE02 | Streak freeze | [ ] | P1 | Protect streak |
| EE03 | Weekend amulet | [ ] | P2 | Weekend off allowed |
| EE04 | Streak wager | [ ] | P2 | Bet on yourself |
| EE05 | Variable reward schedule | [ ] | P1 | Surprise bonuses |
| EE06 | Loss aversion mechanics | [ ] | P1 | "Don't lose streak!" |
| EE07 | Micro-celebrations | [ ] | P0 | Every correct answer |
| EE08 | Daily challenges | [ ] | P1 | Mini-quiz of the day |
| EE09 | Leaderboard (optional) | [ ] | P2 | Classroom competition |
| EE10 | Notification optimization | [ ] | P1 | Smart reminders |

---

## 11.3 AI-Powered Personalization

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| AP01 | Learning style detection | [ ] | P1 | Visual/auditory/kinesthetic |
| AP02 | Emotion-aware adjustment | [ ] | P0 | Via voice emotion |
| AP03 | Time-of-day optimization | [ ] | P2 | Best study hours |
| AP04 | Pace adjustment | [ ] | P0 | Slower for struggling |
| AP05 | Content recommendation | [ ] | P1 | "Students like you..." |

---

## FSRS Algorithm

Free Spaced Repetition Scheduler (2024):

```
Stability = S * (11^D - 1) * e^(k*(1-R)) * e^(0.2*t) * e^(-0.1*lapse)

Where:
- S = previous stability
- D = difficulty (0.0-1.0)
- R = retrievability at review time
- t = time since last review
- lapse = number of forgetting events
```

---

## Skill Tree Structure

```
Mathematics
+-- Arithmetic
|   +-- Natural numbers [v 85%]
|   +-- Basic operations [v 90%]
|   +-- Fractions [> 45%]
|       +-- Concept [v]
|       +-- Comparison [>]
|       +-- Operations [o]
|       +-- Word problems [o]
+-- Algebra
|   +-- 1st degree equations [o]
|   +-- Inequalities [o]
+-- Geometry
    +-- ...
```

---

## Files to Create

- `src/education/mastery.c`
- `src/education/fsrs.c`
- `src/education/engagement.c`
- `src/education/skill_tree.c`

---

## Tests

| ID | Test | Status | Note |
|----|------|--------|------|
| LST01 | Mastery tracking test | [ ] | 80% threshold |
| LST02 | FSRS algorithm test | [ ] | Correct intervals |
| LST03 | Streak mechanics test | [ ] | 7-day bonus |
| LST04 | Adaptive difficulty test | [ ] | Adjusts correctly |

---

## Acceptance Criteria

- [ ] Mastery tracking per skill
- [ ] FSRS implemented
- [ ] Skill tree visualizable
- [ ] Streak mechanics working
- [ ] Emotion-aware pacing

---

## Result

Not yet started. Requires algorithm and UI implementation.
