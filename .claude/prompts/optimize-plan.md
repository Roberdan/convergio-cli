# Prompt: Optimize Execution Plan

Use this prompt to reorganize a large execution plan into a modular structure.

---

## Prompt

```
The file [FILE_NAME].md is getting too large. Reorganize it so that:

1. **It remains the main plan** to execute with status of all work
2. **It's lean** (~100-150 lines max in main file)
3. **Uses external references** to other files for details

### Target Structure

docs/
├── [ProjectName]MasterPlan.md      # Lean main plan
└── [project-name]/
    ├── phases/                      # One file per phase
    │   ├── phase-1-[name].md
    │   ├── phase-2-[name].md
    │   └── ...
    ├── adr/                         # Feature-specific ADRs (avoid merge conflicts)
    │   └── NNN-decision-name.md
    ├── architecture.md              # Diagrams and architecture
    └── execution-log.md             # Chronological log

### What to Keep in Master Plan

- Header with metadata (created, updated, status, version, branch)
- QUICK STATUS table with links to phase files
- DEFINITION OF DONE (checklist)
- Links to related DOCUMENTS
- REQUEST MANAGEMENT section for tracking new requests

### What to Extract

- Detailed phase descriptions → `phases/phase-N-name.md`
- ASCII diagrams and architecture → `architecture.md`
- Detailed chronological log → `execution-log.md`

### Each Phase Must Include (MANDATORY)

- Objective
- Task table with ID, Status, Effort, Note
- Modified files
- **TEST section with mandatory tests**
- Acceptance criteria
- Result

### Request Management

Add to Master Plan a section explaining how to track new requests:
- Classification (Bug Fix, Enhancement, New Feature)
- Tracking (unique ID, description, effort, status)
- Update (phase file, status table, log)
```

---

## Usage Example

```
The file docs/MyProjectPlan.md is getting too large (400+ lines).
Reorganize it following the modular structure.
```

---

## Expected Result

- Master Plan reduced to ~100-150 lines
- Separate files for each phase with complete details
- Architecture file with diagrams
- Log file with event chronology
- Request management section in Master Plan
- Mandatory tests in each phase file
