# How to Add a New Maestro

This guide explains how to add a new historical AI maestro to Convergio Education.

## Overview

A maestro is an AI teaching persona based on a historical figure. Each maestro has:
- Subject specialization
- Unique teaching style
- Voice personality (for OpenAI Realtime Audio)
- Portrait image
- Integration with Ali (the coordinator)

## Steps to Add a New Maestro

### 1. Add Subject (if new)

If the maestro teaches a new subject not already in the system:

**File:** `ConvergioApp/Models/Maestro.swift`

```swift
enum Subject: String, CaseIterable, Identifiable {
    // ... existing cases ...
    case newSubject = "Display Name"

    var color: Color {
        switch self {
        // ... existing cases ...
        case .newSubject: return .yourColor
        }
    }

    var icon: String {
        switch self {
        // ... existing cases ...
        case .newSubject: return "sf.symbol.name"
        }
    }
}
```

### 2. Add Maestro Entry

**File:** `ConvergioApp/Models/Maestro.swift`

Add to `allMaestri` array:

```swift
Maestro(
    name: "HistoricalFigureName",
    subject: .subjectCase,
    specialization: "Specific Expertise Area",
    description: "Italian description of who they are and what they teach.",
    teachingStyle: "Italian description of their teaching methodology.",
    avatarName: "lowercase_name",  // Must match imageset name
    voice: .voiceChoice,           // See MaestroVoice enum
    voiceInstructions: "English voice persona instructions for OpenAI Realtime Audio..."
)
```

**Voice Options:** `alloy`, `ash`, `ballad`, `coral`, `echo`, `sage`, `shimmer`, `verse`

### 3. Add Portrait Image

1. Create imageset folder:
   ```
   ConvergioApp/Assets.xcassets/Maestri/{name}.imageset/
   ```

2. Add `Contents.json`:
   ```json
   {
     "images": [
       { "filename": "{name}.png", "idiom": "universal", "scale": "1x" },
       { "idiom": "universal", "scale": "2x" },
       { "idiom": "universal", "scale": "3x" }
     ],
     "info": { "author": "xcode", "version": 1 }
   }
   ```

3. Add portrait image as `{name}.png` (minimum 200x200 pixels, PNG format)

**Image Sources:**
- Wikimedia Commons (public domain portraits)
- World History Encyclopedia
- Use `sips -s format png` to convert JPEG to PNG if needed

### 4. Register in EditionManager

**File:** `ConvergioApp/App/EditionManager.swift`

1. Add to `maestri` array:
   ```swift
   public static let maestri: [String] = [
       // ... existing maestri ...
       "newname-subject"
   ]
   ```

2. Update counts:
   - Comment: `case education  // N Maestri + 3 coordinatori = N+3 agents`
   - `educationAgents` comment
   - `allowedAgentCount` return value for education

### 5. Configure Azure OpenAI

Ensure the maestro uses Azure OpenAI (EDU requirement):

**File:** `ConvergioApp/Services/AI/AzureOpenAIConfig.swift` (if exists)

The maestro should automatically use Azure OpenAI through the existing configuration. Verify no Anthropic fallback is enabled for EDU edition.

### 6. Ali Integration

Ali (the coordinator) needs to know about the new maestro for workflow coordination:

- Ali uses the `EditionManager.maestri` list dynamically
- No manual update needed if registered correctly

### 7. Test Checklist

- [ ] Image displays in MaestriGridView
- [ ] Image displays in MaestroDetailView
- [ ] Subject filter shows the new subject
- [ ] Study session works with maestro
- [ ] Voice session activates (after reboot for TCC)
- [ ] Voice persona sounds correct
- [ ] Ali can reference and coordinate with maestro

## Example: Adding Grozio (International Law)

```swift
// 1. Subject (Maestro.swift)
case dirittoInternazionale = "Diritto Internazionale"
var color: Color { .indigo }
var icon: String { "scale.3d" }

// 2. Maestro entry (Maestro.swift)
Maestro(
    name: "Grozio",
    subject: .dirittoInternazionale,
    specialization: "Diritto delle Nazioni",
    description: "Hugo Grotius, il padre del diritto internazionale...",
    teachingStyle: "Accademico e sistematico...",
    avatarName: "grozio",
    voice: .sage,
    voiceInstructions: "You are Hugo Grotius..."
)

// 3. EditionManager
"grozio-diritto"  // Added to maestri array
```

## Troubleshooting

### Image Not Showing
- Verify `avatarName` matches imageset folder name (case-sensitive)
- Ensure image is actual PNG format (use `file` command to check)
- Check Contents.json `filename` matches actual file

### Voice Not Working
- Requires Mac reboot if TCC cache is stale
- Verify `voiceInstructions` are in English
- Check Azure OpenAI WebSocket connection

### Subject Filter Missing
- Ensure at least one maestro has the subject
- Verify `Subject.allCases` includes new case
