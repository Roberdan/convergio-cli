# BRUTAL ASSESSMENT (22 Dec) — Education Worktree, Codex Deep Dive
## Scope
- Repository worktree: `/Users/roberdan/GitHub/ConvergioCLI-education`
- Branch: `feature/education-pack`
- Date: 2025-12-22
- Assessment style: Codex-level exhaustive defect hunt (issues, discrepancies, risks, optimizations)

---

## Top Critical Failures
1. **Provider selection is dead code (CRITICAL, GDPR risk)**
   - Functions `edition_get_preferred_provider()`, `edition_get_preferred_model()`, `edition_uses_azure_openai()` are never called.
   - Orchestrator hardcodes provider priority `{Anthropic, OpenAI, Gemini, Ollama}` (`src/orchestrator/orchestrator.c` around 1754), ignoring edition-specific Azure mandate.
   - Outcome: Education uses Claude (US) instead of Azure OpenAI (EU), breaking the documented compliance and safety promises.

2. **Documentation vs code: severe contradictions**
   - `EducationPackMasterPlan.md` claims Phase 11 (Learning Science) 100% done; `phase-11-learning-science.md` is TODO 0%.
   - Claims of “13/14 phases complete” conflict with multiple TODOs and untested features.
   - Provider integration, voice testing, accessibility testing are marked done in one doc and not started in another.

3. **Tests exist but are not run**
   - Scripts: `tests/e2e_education_comprehensive_test.sh` (100+) and `tests/e2e_education_llm_test.sh` (50+).
   - No evidence of execution, no CI, no saved results. Real runtime status unknown.

4. **Learning science (FSRS/Mastery) not integrated**
   - Code exists (`src/education/fsrs.c`, `src/education/mastery.c`) but is isolated:
     - No wiring into orchestrator/education flows.
     - No UI/CLI exposure, no skill-tree, no mastery visualization.
     - Tests all TODO in `phase-11-learning-science.md`.

5. **Voice stack unverified**
   - Large voice implementation (`voice_gateway.c`, `openai_realtime.c`, `azure_realtime.c`) but no proof of end-to-end tests; risk of runtime breakage and unvalidated accessibility voice flows.

6. **Accessibility unverified**
   - Code present (`accessibility_runtime.c`) but no recorded test runs with screen readers/high contrast/keyboard-only; Phase 6 lists multiple TODOs.

---

## High-Severity Issues
1. **Doc status inflation**
   - Features marked COMPLETE in `EducationPackMasterPlan.md` are partial/untested (voice, accessibility, FSRS, provider selection).
   - Features marked NOT STARTED in `EducationMasterPlan.md` are actually implemented (quiz, flashcards, mindmap, conversational onboarding).

2. **Maestri count inconsistency**
   - Code whitelist: 17 Maestri + 3 coordination (Ali, Anna, Jenny) = 20 agents.
   - Docs oscillate between 15 and 17; user-facing expectations will misalign with CLI outputs and tests.

3. **Edition system only partially honored**
   - Education locked at compile time works, but runtime code (providers, model selection) ignores edition preferences.
   - Edition-specific prompts exist but could be bypassed if provider/model not aligned.

4. **Gradebook exports missing**
   - PDF export/report tasks (LB14–LB18) remain TODO; parent-facing deliverables blocked.

5. **Active breaks, certificates not done**
   - Phase 5 leaves F12 (active breaks) and F17 (certificates) open; user experience gaps.

6. **Learning-science tests absent**
   - Mastery/FSRS tests (LST01–LST04) all TODO; algorithm correctness unvalidated.

7. **Voice a11y profile load unproven**
   - Phase 10 declared done, but no evidence of validation with accessibility profiles and Azure Realtime.

8. **Planner/Orchestrator TODOs**
   - `workflow_integration.c:144` plan_output parsing TODO.
   - `memory/persistence.c:230` Anna manager tables TODO.
   - `education/anna_integration.c:730` session elapsed-time tracking TODO.

---

## Medium-Severity Issues
1. **Test coverage claims vs reality**
   - Docs cite “132 test functions, 36 education-specific,” but no run logs; potential false confidence.
2. **Storytelling and localization**
   - Phase 12 done, Phase 13 localization 0%; internationalization unaddressed.
3. **Tooling parity**
   - HtmlInteractive, Mindmap rely on Mermaid.js but browser/open flow not fully validated; risk of max-iteration failures if tool not registered everywhere.
4. **Config drift**
   - Multiple env/provider settings in docs; no enforcement in code paths.
5. **Telemetry/metrics**
   - `metrics.c` exists; unclear if metrics are emitted in edu flows; no verification of PII-safe logging.

---

## Low-Severity / Optimizations
1. **Provider selection refactor**
   - Centralize provider/model resolution via edition helpers; remove hardcoded arrays.
2. **Fast fail on missing Azure keys**
   - At startup, validate required Azure env vars when edition=education; fail loud with actionable error.
3. **CI essential**
   - Minimal CI pipeline: build education edition, run static tests, run a smoke subset of LLM tests with mocked providers.
4. **Docs hygiene**
   - Single source of truth for status; remove conflicting tables; add “verified on date” per feature.
5. **Feature flags**
   - Guard unverified features (voice, FSRS) behind flags until tested; reduce runtime blast radius.

---

## Evidence Pointers (key files)
- Dead code: `src/core/edition.c` (provider/model helpers), unused anywhere else.
- Hardcoded providers: `src/orchestrator/orchestrator.c` (LLM facade loop).
- FSRS/Mastery code: `src/education/fsrs.c`, `src/education/mastery.c` (not wired).
- Voice stack: `src/voice/voice_gateway.c`, `src/voice/openai_realtime.c`, `src/voice/azure_realtime.c` (untested).
- Accessibility runtime: `src/education/accessibility_runtime.c` (no test evidence).
- TODOs: `workflow_integration.c:144`, `memory/persistence.c:230`, `education/anna_integration.c:730`.
- Phase docs with TODOs: `phase-05-features.md` (F12, F17, LB14–LB18), `phase-06-accessibility.md` (multiple P1 TODO), `phase-11-learning-science.md` (all TODO), `phase-13-localization.md` (not started).

---

## Prioritized Remediation Plan
**P0 (today)**
1. Wire provider/model selection:
   - In orchestrator/provider paths, replace hardcoded arrays with `edition_get_preferred_provider()/edition_get_preferred_model()`.
   - Add startup checks for Azure creds when edition=education.
2. Run tests:
   - Execute `tests/e2e_education_comprehensive_test.sh` and `tests/e2e_education_llm_test.sh`; capture logs; fix red tests.
3. Single source of truth docs:
   - Merge status tables; align with actual code and test results; remove contradictory claims.

**P1 (this week)**
4. Integrate FSRS/Mastery:
   - Wire into study/homework/flashcards flows; add mastery display; implement LST0x tests.
5. Validate voice + accessibility:
   - End-to-end voice with Azure Realtime and accessibility profiles; record outcomes.
6. Close critical TODOs:
   - workflow plan_output parsing; persistence manager tables; Anna session tracking.

**P2 (this month)**
7. Finish user-facing gaps:
   - PDF exports, certificates, active breaks.
8. Localization architecture (Phase 13):
   - Define i18n scaffolding, resources loading, and testing.

---

## Brutal Verdict
The education worktree is **feature-rich but integration-poor**. Documentation overstates completion; core promises (Azure-only, safety, learning science) are not enforced in runtime. Tests exist but are not run; dead code hides critical compliance gaps. Immediate integration and testing are required before any release or claim of readiness.

**Status**: 🔴 CRITICAL — integrate, test, and align docs before further feature work.


