# /remind - Quick Reminders

Part of the **Anna Executive Assistant** feature.

## Overview

A fast way to create reminders with natural language. Simpler than `/todo add` for quick reminders.

## Usage

```
/remind <message> <when> [--note <context>]
/remind <when> <message> [--note <context>]
```

The order of message and time is flexible - Anna figures it out.

## Time Formats

### Time of Day
```
tonight, stasera              8:00 PM today
tomorrow morning              9:00 AM tomorrow
tomorrow afternoon            2:00 PM tomorrow
tomorrow evening              7:00 PM tomorrow
```

### Specific Time
```
at 3pm                        3:00 PM today (or tomorrow if passed)
at 15:00                      3:00 PM
tomorrow at 9am               9:00 AM tomorrow
next monday at 10am           10:00 AM next Monday
alle 14                       2:00 PM (Italian)
```

### Relative
```
in 30 minutes                 30 minutes from now
in 2 hours                    2 hours from now
in 3 days                     3 days from now
tra 2 ore                     2 hours (Italian)
```

### Weekdays
```
next monday                   Next Monday
next tuesday at 3pm           Next Tuesday at 3:00 PM
thursday in two weeks         Thursday, 2 weeks from now
lunedi prossimo               Next Monday (Italian)
giovedi tra due settimane     Thursday in 2 weeks (Italian)
```

### Dates
```
dec 15                        December 15th
december 15                   December 15th
2025-12-25                    December 25, 2025
2025-12-25 14:30              December 25 at 2:30 PM
```

## Examples

```
/remind "Call mom" tomorrow morning
/remind tonight "Buy groceries"
/remind "Team meeting" next tuesday at 10am
/remind in 2 hours "Take a break"
/remind dec 1 "Pay rent"
/remind domani mattina "Chiamare il dentista" --note "Chiedere appuntamento"
```

## With Notes

Add context using `--note`:

```
/remind "Review PR" tomorrow --note "Check the auth changes in #123"
/remind "Call client" monday at 2pm --note "Discuss Q1 budget proposal"
```

## Managing Reminders

- `/reminders` - Show today's reminders
- `/reminders week` - Show next 7 days
- `/reminders all` - Show all scheduled
- `/todo done <id>` - Mark reminder complete
- `/todo delete <id>` - Delete a reminder

## See Also

- `/todo` - Full task manager
- `/reminders` - View upcoming reminders
- `/help remind` - Quick help
