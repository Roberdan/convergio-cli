# Legacy Files > 250 Lines

This document lists files that exceed the 250-line workspace rule. These are marked as **LEGACY** and are exempt from the rule for historical/architectural reasons.

**Last Updated**: 2025-12-23

## Education Module Legacy Files

| File | Lines | Reason | Status |
|------|-------|--------|--------|
| `src/education/education_db.c` | 4562 | Core database module - monolithic by design | LEGACY |
| `src/education/tools/flashcards.c` | 1096 | Complete flashcard system with FSRS integration | LEGACY |
| `src/core/commands/education_commands.c` | 1488 | All education CLI commands in one file | LEGACY |

## Core System Legacy Files

| File | Lines | Reason | Status |
|------|-------|--------|--------|
| `src/agents/embedded_agents.c` | 14830 | Auto-generated from agent definitions | LEGACY (auto-generated) |
| `src/core/commands/commands.c` | 4882 | All CLI commands - monolithic by design | LEGACY |
| `src/tools/tools.c` | 3601 | Complete tool system - monolithic by design | LEGACY |
| `src/orchestrator/orchestrator.c` | 1852 | Core orchestrator - monolithic by design | LEGACY |
| `src/mcp/mcp_client.c` | 1834 | MCP protocol client - monolithic by design | LEGACY |
| `src/memory/persistence.c` | 1744 | Database persistence layer - monolithic by design | LEGACY |
| `src/orchestrator/registry.c` | 1498 | Agent registry - monolithic by design | LEGACY |
| `src/todo/todo.c` | 1441 | Todo system - monolithic by design | LEGACY |
| `src/notifications/notify.c` | 1389 | Notification system - monolithic by design | LEGACY |
| `src/neural/claude.c` | 1348 | Claude provider implementation - monolithic by design | LEGACY |
| `src/acp/acp_server.c` | 1231 | ACP server - monolithic by design | LEGACY |
| `src/projects/projects.c` | 1209 | Project management - monolithic by design | LEGACY |
| `src/orchestrator/plan_db.c` | 1190 | Plan database - monolithic by design | LEGACY |
| `src/providers/anthropic.c` | 1109 | Anthropic provider - monolithic by design | LEGACY |
| `src/providers/provider.c` | 1098 | Provider abstraction - monolithic by design | LEGACY |
| `src/providers/openai.c` | 1066 | OpenAI provider - monolithic by design | LEGACY |
| `src/voice/voice_gateway.c` | 1048 | Voice gateway - monolithic by design | LEGACY |

## Notes

- **Auto-generated files** (like `embedded_agents.c`) are exempt from line limits
- **Monolithic modules** are kept as single files for architectural reasons (easier to maintain, better performance)
- **Future refactoring**: These files may be split in future phases, but are not blocking for release
- **New code**: All new files MUST follow the 250-line rule

## Verification

To verify this list:
```bash
find src -name "*.c" -exec wc -l {} \; | awk '$1 > 250 {print $1, $2}' | sort -rn
```

