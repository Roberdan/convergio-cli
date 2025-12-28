# Structured Execution Planner

> This command uses the global planner at `~/.claude/commands/planner.md`
> which includes both planning AND orchestration capabilities.

## Quick Reference

### Usage
```
/planner [task description]
```

### What it does
1. Creates a plan file in `docs/plans/[Name]Plan[Date].md`
2. Assigns tasks to Claude instances (max 4)
3. Asks: "Vuoi eseguire in parallelo?"
4. If yes → Orchestrates execution via Kitty

### Requirements for parallel execution
- Must run FROM Kitty terminal (not Warp/iTerm)
- `wildClaude` alias configured
- Kitty remote control enabled

### Quick Check
```bash
~/.claude/scripts/kitty-check.sh
```

### Manual Execution Scripts
```bash
~/.claude/scripts/claude-parallel.sh 4    # Launch 4 Claude tabs
~/.claude/scripts/claude-monitor.sh       # Monitor progress
```

---

See full documentation: `~/.claude/commands/planner.md`
