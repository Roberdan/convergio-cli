# ADR-013: Native Notification Helper (ConvergioNotify.app)

**Status**: Accepted
**Date**: 2025-12-16
**Author**: Roberto with AI Team
**Supersedes**: None
**Related**: ADR-009 (Anna Executive Assistant Architecture)

---

## Context

The Convergio CLI notification system (ADR-009) originally relied on third-party tools for displaying macOS notifications:

1. **terminal-notifier**: External tool that must be installed via Homebrew
2. **osascript**: Built-in but limited (no custom icon support)

Both approaches have significant limitations on recent macOS versions:

### terminal-notifier Issues
- The `-sender` flag (for bundle ID-based icon) is **ignored on macOS 12+**
- The `-appIcon` flag is also **ignored** due to Apple security restrictions
- Results in Terminal.app or generic icon displayed regardless of settings
- This is a [known bug](https://github.com/julienXX/terminal-notifier/issues/314) with no upstream fix

### osascript Issues
- No way to specify custom icon at all
- Always shows Script Editor icon
- Limited notification options (no actions, no sounds in some cases)

### User Impact
- Notifications from Convergio showed Terminal/Script Editor icon
- Users couldn't identify Convergio notifications at a glance
- Reduced professional appearance of the application
- Confusion with other terminal-based notifications

## Decision

We will create a **native Swift helper application** (ConvergioNotify.app) that:

1. Uses the legacy `NSUserNotification` API for icon display
2. Is bundled with its own app icon (Convergio logo)
3. Is signed and notarized as part of the release process
4. Is installed to `/Applications` during Homebrew/manual installation
5. Takes precedence over terminal-notifier in the fallback chain

### Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                    NOTIFICATION FALLBACK CHAIN                       │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  1. ConvergioNotify.app    ← Native helper (best - proper icon)     │
│         │                                                            │
│         ▼ (if not installed)                                        │
│  2. terminal-notifier      ← Third-party (requires brew install)    │
│         │                                                            │
│         ▼ (if not installed)                                        │
│  3. osascript              ← Built-in (no icon control)             │
│         │                                                            │
│         ▼ (if fails)                                                │
│  4. Terminal output        ← Direct TTY output                      │
│         │                                                            │
│         ▼ (if no TTY)                                               │
│  5. Log file               ← Last resort                            │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### Helper App Structure

```
ConvergioNotify.app/
├── Contents/
│   ├── Info.plist          # Bundle metadata (com.convergio.notify)
│   ├── MacOS/
│   │   └── ConvergioNotify # Swift executable
│   └── Resources/
│       └── AppIcon.icns    # Convergio logo in icns format
```

### Usage

```bash
# Command-line interface
ConvergioNotify -title "Reminder" -message "Call mom" [-subtitle "Task"]

# From notify.c
/Applications/ConvergioNotify.app/Contents/MacOS/ConvergioNotify \
    -title 'Reminder' -message 'Call mom' -subtitle 'Personal'
```

### Why NSUserNotification (Legacy API)

Apple deprecated `NSUserNotification` in macOS 10.14 in favor of `UNUserNotificationCenter`. However:

1. **NSUserNotification** displays the app's bundle icon automatically
2. **UNUserNotificationCenter** requires explicit permission requests
3. **NSUserNotification** works without any user permission prompts
4. The deprecation warning is cosmetic - the API still functions on macOS 12-15

This is a deliberate choice to prioritize user experience over API modernity.

## File Changes

### New Files

| File | Purpose |
|------|---------|
| `src/notifications/helper/main.swift` | Swift notification helper |
| `src/notifications/helper/Info.plist` | Bundle configuration |

### Modified Files

| File | Change |
|------|--------|
| `src/notifications/notify.c` | Added ConvergioNotify detection and fallback |
| `src/orchestrator/orchestrator.c` | Auto-start daemon on Convergio launch |
| `Makefile` | Added `notify-helper` build target |
| `.github/workflows/release.yml` | Sign/notarize/bundle helper in releases |
| `Formula/convergio.rb` | Install helper to /Applications |

## Alternatives Considered

### Alternative 1: Fix terminal-notifier

**Rejected because**:
- Upstream issue open since 2019, no fix forthcoming
- Would require maintaining a fork
- Doesn't solve the fundamental macOS security restrictions

### Alternative 2: Use UNUserNotificationCenter

**Rejected because**:
- Requires explicit user permission approval
- Permission flow complex from CLI context
- Icon still problematic without proper bundle

### Alternative 3: Create full Convergio.app

**Rejected because**:
- Overkill for notification-only use case
- Convergio is primarily a CLI tool
- Would confuse users expecting terminal-based workflow

### Alternative 4: Accept Terminal icon

**Rejected because**:
- Poor user experience
- Unprofessional appearance
- User confusion with other terminal notifications

## Build and Release Integration

### Local Build
```makefile
notify-helper: $(NOTIFY_HELPER_APP)
    swiftc -o $(APP)/Contents/MacOS/ConvergioNotify main.swift -framework Cocoa
    iconutil -c icns iconset -o $(APP)/Contents/Resources/AppIcon.icns
    codesign --force --deep --sign - $(APP)
```

### CI/Release
1. Build helper during `make all`
2. Sign with Developer ID certificate
3. Include in notarization submission
4. Bundle in release tarball
5. Homebrew formula installs to /Applications

### Homebrew Installation
```ruby
def post_install
  notify_app = prefix/"ConvergioNotify.app"
  if notify_app.exist?
    target = Pathname.new("/Applications/ConvergioNotify.app")
    FileUtils.cp_r(notify_app, target)
    system "lsregister", "-f", target
  end
end
```

## Security Considerations

1. **Code Signing**: Helper is signed with Developer ID (same as main binary)
2. **Notarization**: Included in notarization submission to Apple
3. **Gatekeeper**: App passes Gatekeeper verification
4. **No Permissions**: Uses legacy API that doesn't require notification permissions
5. **LSUIElement**: Runs as background app (no dock icon, no menu bar)

## Performance Impact

- **Binary size**: +50KB (Swift runtime already linked in main binary)
- **Memory**: ~5MB when invoked (exits after 0.5s)
- **Startup**: <100ms to display notification
- **No persistent process**: Spawned only when needed, exits immediately

## Testing

1. **Manual**: `make install && convergio` → create reminder → verify icon
2. **Unit**: Icon file existence check in CI
3. **E2E**: Notification delivery verification (visual)

## Consequences

### Positive
- Convergio notifications display proper logo
- Professional appearance
- Works on all recent macOS versions (12-15)
- No external dependencies (terminal-notifier optional)
- Seamless integration with existing notification system

### Negative
- Additional build artifact to maintain
- Swift dependency (already present for MLX)
- Uses deprecated API (functional but may break in future macOS)

### Neutral
- Helper must be installed to /Applications for icon to work
- First notification may prompt for notification permissions

## Migration Path

If Apple removes NSUserNotification entirely in future macOS:
1. Switch to UNUserNotificationCenter
2. Add permission request flow
3. Or use terminal-notifier as primary with degraded UX

## References

- [NSUserNotification Deprecation](https://developer.apple.com/documentation/foundation/nsusernotification)
- [terminal-notifier icon bug](https://github.com/julienXX/terminal-notifier/issues/314)
- [Apple Notification Programming Guide](https://developer.apple.com/documentation/usernotifications)
- [Code Signing Guide](https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution)
