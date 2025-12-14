# /todo - Task Manager

Part of the **Anna Executive Assistant** feature.

## Overview

A native local task manager with reminders. All data is stored locally in SQLite database - no external services required.

## Subcommands

### Add a Task

```
/todo add <title> [options]
```

**Options:**
- `--due <date>` - Due date (e.g., "tomorrow", "next monday", "2025-12-25")
- `--remind <time>` - Reminder (e.g., "1h", "30m", "1d" before due)
- `--priority <1-3>` - 1=urgent, 2=normal (default), 3=low
- `--context <ctx>` - Project or context tag

**Examples:**
```
/todo add "Review PR"
/todo add "Team meeting" --due "tomorrow 10am" --remind 1h
/todo add "Quarterly report" --due "next friday" --priority 1 --context work
```

### List Tasks

```
/todo list [filter]
```

**Filters:**
- `today` - Today's tasks + overdue
- `overdue` - Only overdue tasks
- `upcoming [days]` - Tasks due in next N days (default 7)
- `all` - All tasks including completed

**Examples:**
```
/todo list           # Pending tasks
/todo list today     # Today + overdue
/todo list upcoming 14
```

### Complete a Task

```
/todo done <id>
```

Marks a task as completed. If the task has recurrence, creates the next occurrence.

### Start a Task

```
/todo start <id>
```

Marks a task as "in progress".

### Delete a Task

```
/todo delete <id>
/todo rm <id>
```

Permanently deletes a task.

### Quick Capture (Inbox)

```
/todo inbox [content]
```

Without content: shows inbox items.
With content: quickly captures a thought to process later.

**Examples:**
```
/todo inbox                    # List inbox
/todo inbox "Call dentist"     # Quick capture
```

### Search Tasks

```
/todo search <query>
/todo find <query>
```

Full-text search across task titles and descriptions.

### Statistics

```
/todo stats
```

Shows:
- Pending tasks
- In progress tasks
- Completed today/this week
- Overdue count
- Inbox items

## Task Display

Tasks are displayed with:
- `[ ]` Pending
- `[>]` In progress (yellow)
- `[x]` Completed (green)
- `[-]` Cancelled (gray)
- `!!` Urgent priority (red)
- `~` Low priority (gray)
- `@context` Context tag (purple)
- Due dates in relative format

## See Also

- `/help` - All commands
- `ADR-009` - Architecture decision record
