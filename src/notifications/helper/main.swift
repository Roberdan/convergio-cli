import Cocoa

// ConvergioNotify - Native notification helper for Convergio CLI
// Uses NSUserNotification (legacy API) which doesn't require explicit permissions
// and properly displays the app icon from the bundle.
//
// Usage: ConvergioNotify -title "Title" -message "Message" [-subtitle "Subtitle"]

class AppDelegate: NSObject, NSApplicationDelegate, NSUserNotificationCenterDelegate {
    func userNotificationCenter(_ center: NSUserNotificationCenter,
                                shouldPresent notification: NSUserNotification) -> Bool {
        return true // Always show, even if app is frontmost
    }
}

func parseArgs() -> (title: String, message: String, subtitle: String?) {
    var title = "Convergio"
    var message = ""
    var subtitle: String? = nil

    let args = CommandLine.arguments
    var i = 1
    while i < args.count {
        switch args[i] {
        case "-title":
            if i + 1 < args.count { title = args[i + 1]; i += 1 }
        case "-message":
            if i + 1 < args.count { message = args[i + 1]; i += 1 }
        case "-subtitle":
            if i + 1 < args.count { subtitle = args[i + 1]; i += 1 }
        default:
            break
        }
        i += 1
    }
    return (title, message, subtitle)
}

// Initialize app
let app = NSApplication.shared
let delegate = AppDelegate()
app.delegate = delegate

// Set up notification center
let center = NSUserNotificationCenter.default
center.delegate = delegate

// Parse arguments and create notification
let args = parseArgs()

let notification = NSUserNotification()
notification.title = args.title
notification.informativeText = args.message
if let sub = args.subtitle {
    notification.subtitle = sub
}
notification.soundName = NSUserNotificationDefaultSoundName

// Deliver notification
center.deliver(notification)

// Give time for notification to be delivered, then exit
DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
    app.terminate(nil)
}

app.run()
