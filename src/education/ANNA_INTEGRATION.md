# Anna Education Integration

## Overview

The Anna Education Integration connects Anna (the executive assistant agent) with the Education module to provide intelligent, accessibility-aware reminders and notifications for students.

## Features (F18-F22)

### F18: Connection Initialization
- `anna_education_connect()` - Initialize the integration
- `anna_education_disconnect()` - Shutdown the integration
- `anna_education_is_connected()` - Check connection status

### F19: Homework Reminders
- `anna_homework_reminder(student_id, subject, assignment, due_date)`
- Automatically schedules two reminders:
  - 24 hours before due date
  - 1 hour before due date

### F20: Spaced Repetition Reminders
- `anna_spaced_repetition_reminder(student_id, topic, next_review)`
- Integrates with SM-2 algorithm from flashcard system
- Reminds students when topics need review

### F21: ADHD-Aware Break Reminders
- `anna_adhd_break_reminder(student_id, session_id)`
- Adjusts break frequency based on ADHD severity:
  - **Mild ADHD**: Every 25 minutes (standard Pomodoro)
  - **Moderate ADHD**: Every 15 minutes
  - **Severe ADHD**: Every 10 minutes
  - **Hyperactive type**: Even more frequent (8-12 minutes)

### F22: Achievement Celebrations
- `anna_celebration_notify(student_id, celebration)`
- Immediate notifications for achievements:
  - Perfect quiz scores
  - Study streaks (7, 14, 30 days)
  - Level ups
  - Goal completions

## Architecture

### Data Storage

Reminders are stored in the `inbox` table of the education database with the following format:

```
REMINDER|type|scheduled_at|content
```

Example:
```
REMINDER|homework|1735689600|Homework due in Matematica: Chapter 5 exercises
```

### Notification Delivery

Notifications are delivered via native macOS `osascript`:

```bash
osascript -e 'display notification "message" with title "title" sound name "Glass"'
```

### Accessibility Adaptations

The system respects student accessibility profiles:

1. **Text-to-Speech (TTS)**
   - If enabled, notifications are also spoken using `say` command
   - Speed adjusted to student's TTS speed preference

2. **ADHD Support**
   - Break interval adjusted based on ADHD type and severity
   - More frequent micro-celebrations for motivation

3. **Visual Impairment**
   - Audio notifications always sent when TTS is enabled
   - High contrast settings respected

## Usage Examples

### Initialize Integration

```c
// After education_init()
if (anna_education_connect() == 0) {
    printf("Anna integration connected\n");
}
```

### Schedule Homework Reminder

```c
time_t due_date = time(NULL) + (2 * 86400);  // 2 days from now
int64_t reminder_id = anna_homework_reminder(
    student_id,
    "Matematica",
    "Complete Chapter 5 exercises",
    due_date
);
```

### Schedule ADHD Break

```c
// During active study session
int64_t reminder_id = anna_adhd_break_reminder(student_id, session_id);
// Break reminder will fire based on student's ADHD profile
```

### Celebrate Achievement

```c
AnnaCelebration celebration = {
    .achievement_type = "quiz_perfect",
    .title = "Perfect Score!",
    .message = "You got 100% on the History quiz!",
    .emoji = "🎉"
};
anna_celebration_notify(student_id, &celebration);
```

### Check Due Reminders

```c
// Call on app startup or periodically
int sent = anna_check_due_reminders();
printf("Sent %d due reminders\n", sent);
```

## Reminder Types

| Type | Description | Scheduling |
|------|-------------|------------|
| `ANNA_REMINDER_HOMEWORK` | Homework deadline | 24h and 1h before |
| `ANNA_REMINDER_SPACED_REPETITION` | Topic review | Based on SM-2 algorithm |
| `ANNA_REMINDER_BREAK` | Study break | Based on ADHD profile |
| `ANNA_REMINDER_CELEBRATION` | Achievement | Immediate |
| `ANNA_REMINDER_SESSION` | Study session | User-scheduled |
| `ANNA_REMINDER_GOAL` | Goal deadline | 7d, 3d, 1d before |

## Database Schema

The `inbox` table is used to store reminders:

```sql
CREATE TABLE IF NOT EXISTS inbox (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id INTEGER NOT NULL REFERENCES student_profiles(id),
    content TEXT NOT NULL,
    source TEXT DEFAULT 'cli' CHECK(source IN ('cli', 'voice', 'agent', 'reminder')),
    processed INTEGER DEFAULT 0,
    processed_to_task_id INTEGER,
    created_at INTEGER DEFAULT (strftime('%s','now'))
);
```

Reminders use `source='reminder'` and `processed=0` for pending reminders.

## Accessibility Profile Integration

The system reads from the `student_accessibility` table to adapt reminders:

```c
// Get break interval based on ADHD profile
int interval = anna_get_break_interval(student_id);

// Check if TTS is enabled
EducationAccessibility* acc = education_accessibility_get(student_id);
if (acc && acc->tts_enabled) {
    // Speak notification
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "say -r %.0f \"%s\"",
             acc->tts_speed * 100.0f, message);
    system(cmd);
}
```

## Thread Safety

All functions use the `g_anna_edu_mutex` to ensure thread-safe access to the database.

## Error Handling

Functions return:
- `0` on success
- `-1` on error
- `int64_t` reminder ID for scheduling functions

Error messages are logged to stderr with the `[anna_edu]` prefix.

## Testing

To test the integration:

1. **Initialize**:
   ```c
   education_init();
   anna_education_connect();
   ```

2. **Create Test Student**:
   ```c
   EducationCreateOptions options = {
       .name = "Test Student",
       .age = 15,
       .grade_level = 10,
       .curriculum_id = "it_liceo_scientifico"
   };
   int64_t student_id = education_profile_create(&options);
   ```

3. **Schedule Test Reminder**:
   ```c
   time_t test_time = time(NULL) + 60;  // 1 minute from now
   anna_homework_reminder(student_id, "Test", "Test homework", test_time);
   ```

4. **Check Reminders**:
   ```c
   sleep(65);
   anna_check_due_reminders();  // Should send notification
   ```

## Integration with Education Module

The Anna integration works seamlessly with other Education features:

- **Flashcard Reviews**: Automatically schedules spaced repetition reminders
- **Learning Sessions**: Tracks session time and suggests breaks
- **Gamification**: Celebrates XP gains, level ups, streaks
- **Goals**: Reminds about goal deadlines
- **Accessibility**: Adapts all notifications to student needs

## Future Enhancements

Potential future improvements:

1. **Smart Scheduling**
   - ML-based optimal reminder timing
   - Learn student's productivity patterns

2. **Multi-Modal Notifications**
   - SMS/Email for critical reminders
   - Calendar integration

3. **Contextual Reminders**
   - Location-based (when student arrives home)
   - Focus mode integration (Do Not Disturb)

4. **Advanced ADHD Support**
   - Pomodoro timer with visual/audio cues
   - Distraction parking lot integration
   - Hyperfocus detection and break enforcement

5. **Parent/Guardian Integration**
   - Summary emails to parents
   - Shared goal tracking

## License

Copyright (c) 2025 Convergio.io
Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International

## See Also

- [ADR-009: Anna Executive Assistant](../../docs/adr/009-anna-executive-assistant.md)
- [Education Pack Plan](../../docs/plans/EducationPackPlan.md)
- [Education Module](education_db.c)
- [Notification System](../notifications/notify.c)
