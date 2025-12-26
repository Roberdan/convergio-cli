# Convergio: Other Planning Documents Inventory

**Date:** December 26, 2025  
**Purpose:** Inventory of all planning documents not related to V7

---

## Executive Summary

**Total Non-V7 Documents:** 16 documents

**Categories:**
- **Specific Feature Plans:** 4 documents (MLX, Semantic Graph, Website, Workflow)
- **Historical Plans:** 1 document (v4)
- **Temporary/Generated Plans:** 11 documents (likely from previous AI sessions)

---

## Part 1: Specific Feature Plans

### 1.1 PLAN-mlx-local-provider.md

**Purpose:** MLX local provider implementation plan  
**Status:** ✅ Implemented (MLX support exists in codebase)  
**Relevance:** Integrated into V7 (see V7Plan-Local-vs-Cloud-LLM-Strategy.md)  
**Action:** Keep for reference, but V7 plan supersedes

### 1.2 PLAN-semantic-graph-persistence.md

**Purpose:** Semantic graph persistence implementation  
**Status:** ✅ Implemented (semantic memory exists in codebase)  
**Relevance:** Integrated into V7 (memory system)  
**Action:** Keep for reference, but V7 plan supersedes

### 1.3 WebsitePlan.md

**Purpose:** Website planning and structure  
**Status:** ⚠️ May need updates for V7  
**Relevance:** Website needs to reflect V7 architecture  
**Action:** Review and update for V7

### 1.4 WORKFLOW_MERGE_PLAN.md

**Purpose:** Workflow engine merge plan  
**Status:** ✅ Implemented (workflow engine exists in codebase)  
**Relevance:** Integrated into V7 (workflow system)  
**Action:** Keep for reference, but V7 plan supersedes

---

## Part 2: Historical Plans

### 2.1 v4.md

**Purpose:** Convergio v4 planning document  
**Status:** ✅ Historical (v6 is current)  
**Relevance:** Historical reference only  
**Action:** Keep for historical reference

---

## Part 3: Temporary/Generated Plans

**These appear to be temporary planning documents from previous AI sessions:**

1. `compiled-growing-tide.md`
2. `crispy-exploring-mango.md`
3. `frolicking-shimmying-moler.md`
4. `glistening-sprouting-yeti.md`
5. `kind-nibbling-yeti.md`
6. `lively-baking-alpaca.md`
7. `logical-wobbling-tarjan.md`
8. `proud-dreaming-quill.md`
9. `silly-mapping-crane.md`
10. `spicy-wandering-perlis.md`
11. `stateless-weaving-kettle.md`

**Status:** ⚠️ Unknown - need to review content  
**Relevance:** Likely temporary/obsolete  
**Action:** Review and either archive or delete

---

## Part 4: Recommended Actions

### 4.1 Keep for Reference

| Document | Reason |
|---------|--------|
| `PLAN-mlx-local-provider.md` | Historical reference, implemented |
| `PLAN-semantic-graph-persistence.md` | Historical reference, implemented |
| `WORKFLOW_MERGE_PLAN.md` | Historical reference, implemented |
| `v4.md` | Historical reference |

### 4.2 Review & Update

| Document | Action |
|----------|--------|
| `WebsitePlan.md` | Review and update for V7 architecture |

### 4.3 Review & Archive/Delete

| Documents | Action |
|-----------|--------|
| All `*-*.md` files (11 files) | Review content, archive if useful, delete if obsolete |

---

## Part 5: Organization Recommendation

### 5.1 Proposed Structure

```
docs/plans/
├── V7Plan/                    # V7 planning (current)
│   └── [all V7 documents]
│
├── Historical/                # Historical plans
│   ├── v4.md
│   ├── PLAN-mlx-local-provider.md
│   ├── PLAN-semantic-graph-persistence.md
│   └── WORKFLOW_MERGE_PLAN.md
│
├── Active/                    # Active non-V7 plans
│   └── WebsitePlan.md
│
└── Archive/                   # Temporary/obsolete plans
    └── [11 temporary files]
```

### 5.2 Alternative: Move to Worktree

**Option:** Move all non-V7 plans to worktree for organization:

```
ConvergioCLI-v7-plans/
├── docs/plans/
│   ├── V7Plan/               # V7 planning (current)
│   ├── Historical/           # Historical plans
│   ├── Active/               # Active non-V7 plans
│   └── Archive/              # Temporary/obsolete
```

---

## Part 6: Document Review Checklist

### 6.1 Review Each Document

For each non-V7 document:

- [ ] **Read content** - Understand what it contains
- [ ] **Check status** - Is it implemented? Obsolete? Active?
- [ ] **Check relevance** - Does it relate to V7? Current work?
- [ ] **Decide action** - Keep, Update, Archive, Delete

### 6.2 Review Priority

**High Priority:**
1. `WebsitePlan.md` - May need V7 updates
2. `PLAN-mlx-local-provider.md` - Check if V7 supersedes
3. `PLAN-semantic-graph-persistence.md` - Check if V7 supersedes

**Medium Priority:**
4. `WORKFLOW_MERGE_PLAN.md` - Check if V7 supersedes
5. `v4.md` - Historical reference

**Low Priority:**
6. All `*-*.md` files - Review and archive/delete

---

## Part 7: Next Steps

1. **Review WebsitePlan.md** - Update for V7 if needed
2. **Review temporary files** - Archive or delete obsolete ones
3. **Organize structure** - Create Historical/Active/Archive folders
4. **Update README.md** - Reflect new organization

---

## Related Documents

- [V7Plan-MASTER-INDEX.md](./V7Plan-MASTER-INDEX.md) - V7 documentation hub
- [V7Plan-EXECUTIVE-SUMMARY.md](./V7Plan-EXECUTIVE-SUMMARY.md) - V7 unified plan

---

*This inventory lists all non-V7 planning documents. Review each to determine if it should be kept, updated, archived, or deleted.*

