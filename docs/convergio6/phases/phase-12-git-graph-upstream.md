# PHASE 12 - Git Graph Upstream Proposal

**Status**: ⏸️ PENDING
**Depends on**: All previous phases complete

## Objective

Prepare Git Graph feature for proposal to Zed team as upstream contribution. Create separate worktree with clean implementation suitable for PR.

## Tasks

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| U1 | Create clean worktree from zed/main | ⏸️ | 0.5 day | Fresh branch without Convergio deps |
| U2 | Port git_graph crate (standalone) | ⏸️ | 1 day | Remove Convergio-specific code |
| U3 | Write comprehensive tests | ⏸️ | 1 day | Unit + integration tests |
| U4 | Add documentation and screenshots | ⏸️ | 0.5 day | README, inline docs |
| U5 | Open PR to zed-industries/zed | ⏸️ | 0.5 day | Follow Zed contribution guidelines |

## Preparation

### What to Include
- `crates/git_graph/` - Core graph data structures
- Integration with `git_ui` crate
- Lane assignment algorithm
- Virtual scrolling for performance
- 8-color palette for branches

### What to Remove
- References to Convergio
- Ali/agent integrations
- Any Convergio-specific styling

## PR Strategy

1. **Title**: "Add Git Graph visualization to Git panel"
2. **Motivation**: "Visual commit history like VS Code Git Graph extension"
3. **Screenshots**: Before/after of git panel
4. **Performance**: Benchmark data for large repos
5. **Tests**: Comprehensive test coverage

## Inspiration Reference

[VS Code Git Graph](https://marketplace.visualstudio.com/items?itemName=mhutchie.git-graph) - 4M+ installs, highly requested feature.
