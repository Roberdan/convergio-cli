# /daemon - Notification Daemon Management

Manage the Convergio notification daemon for background reminder delivery.

## Usage

```
/daemon <command>
```

## Commands

| Command | Description |
|---------|-------------|
| `start` | Start the notification daemon |
| `stop` | Stop the notification daemon |
| `restart` | Restart the daemon |
| `status` | Show daemon status (running/stopped, PID) |
| `health` | Show detailed health information |
| `install` | Install LaunchAgent for auto-start at login |
| `uninstall` | Remove LaunchAgent |
| `test` | Send a test notification |

## Examples

```
/daemon start      # Start the daemon
/daemon status     # Check if daemon is running
/daemon health     # Detailed health info
/daemon install    # Auto-start at login
/daemon test       # Send test notification
```

## About the Daemon

The notification daemon runs in the background to deliver scheduled reminders even when Convergio isn't running.

**Features:**
- Native macOS notifications (terminal-notifier or osascript)
- Apple Silicon optimized (runs on E-cores for power efficiency)
- Adaptive polling (60s normal, 5min when idle)
- Automatic retry on delivery failure

**LaunchAgent:**
When installed, the daemon starts automatically at login and restarts if it crashes. The LaunchAgent is installed at:
`~/Library/LaunchAgents/io.convergio.daemon.plist`

## Notification Methods

The daemon tries notification methods in order:
1. **terminal-notifier** - Best UX, supports actions (install via `brew install terminal-notifier`)
2. **osascript** - Built-in macOS, always available
3. **terminal** - Print to terminal if active
4. **sound** - Sound only
5. **log** - Log file only (last resort)

## See Also

- `/remind` - Set reminders
- `/reminders` - View scheduled reminders
- `/todo` - Task management
