# Convergio Native - Test Strategy

> Comprehensive testing strategy for Convergio Native macOS application.

**Created:** 2025-12-15
**Author:** Roberto D'Angelo + AI Team

---

## Test Pyramid Overview

```
                    /\
                   /  \
                  / E2E \        <- 10% - Full user workflows
                 /--------\
                / UI Tests \     <- 20% - XCUITest automation
               /------------\
              / Integration  \   <- 30% - Swift-C bridge, Keychain
             /----------------\
            /   Unit Tests     \ <- 40% - ViewModels, Types, Logic
           /--------------------\
```

---

## 1. Unit Tests (40%)

### Location: `ConvergioAppTests/`

| Test File | Coverage | Status |
|-----------|----------|--------|
| `ViewModelTests.swift` | OrchestratorVM, ConversationVM | DONE |
| `AgentTests.swift` (in ViewModelTests) | Agent types, roles, states | DONE |
| `MessageTests.swift` (in ViewModelTests) | Message types, formatting | DONE |
| `TokenUsageTests.swift` | Token calculations | DONE |
| `ProviderTypeTests.swift` | Provider enums | DONE |
| `OrchestratorErrorTests.swift` | Error handling | DONE |

### Missing Unit Tests to Add:

- [ ] `KeychainManagerTests.swift` - Keychain operations
- [ ] `LoggerTests.swift` - Logging system
- [ ] `CostInfoTests.swift` - Budget calculations
- [ ] `StreamingTests.swift` - Streaming text handling

---

## 2. Integration Tests (30%)

### Location: `ConvergioAppTests/Integration/`

| Test Area | Description | Status |
|-----------|-------------|--------|
| Swift-C Bridge | Test CConvergio functions via Swift | TODO |
| Keychain + Env | Import/export API keys | TODO |
| Logger + Files | Log file creation and cleanup | TODO |
| Orchestrator Init | Full initialization cycle | TODO |

### C Library Tests (existing)

Located in `/tests/`:
- `test_providers.c` - Provider switching
- `test_unit.c` - Core functions
- `test_compaction.c` - Memory compaction
- `test_anna.c` - Voice assistant
- Mock providers in `/tests/mocks/`

---

## 3. UI Tests (20%)

### Location: `ConvergioAppUITests/`

| Test File | Coverage | Status |
|-----------|----------|--------|
| `ConvergioAppUITests.swift` | App launch, navigation, toolbar | DONE |
| `ConversationUITests.swift` | Input, messages, scrolling | DONE |
| `AccessibilityUITests.swift` | VoiceOver, labels | TODO |
| `SettingsUITests.swift` | Settings navigation, API keys | TODO |
| `OnboardingUITests.swift` | Onboarding wizard flow | TODO |

### Accessibility Tests (WCAG 2.1 AA Compliance)

```swift
// Tests to add:
- testAllInteractiveElementsHaveLabels()
- testMinimumContrastRatios()
- testKeyboardNavigationComplete()
- testVoiceOverAnnouncements()
- testReduceMotionRespected()
- testDynamicTypeSupported()
```

---

## 4. E2E Tests (10%)

### Location: `ConvergioAppUITests/E2E/`

| Workflow | Steps | Status |
|----------|-------|--------|
| First Launch | App opens → Onboarding → API key → Ready | TODO |
| Full Conversation | Input → Send → Stream → Response → Cost update | TODO |
| Multi-Agent | Question → Agents work → Convergence → Result | TODO |
| Settings Flow | Open → Change provider → Save → Verify | TODO |
| Export Chat | Send messages → Export → Verify file | TODO |
| Error Recovery | Bad API key → Error → Fix → Retry | TODO |

---

## 5. Performance Tests

### Metrics to Track

| Metric | Target | Tool |
|--------|--------|------|
| App Launch | < 2s | XCTApplicationLaunchMetric |
| First Paint | < 1s | Manual measurement |
| Scroll FPS | 60fps | Instruments |
| Memory Usage | < 200MB idle | Instruments |
| Streaming Latency | < 50ms per chunk | Custom timing |

### Performance Test Examples

```swift
func testAppLaunchPerformance() throws {
    measure(metrics: [XCTApplicationLaunchMetric()]) {
        XCUIApplication().launch()
    }
}

func testScrollPerformance() throws {
    measure(metrics: [XCTOSSignpostMetric.scrollDecelerationMetric]) {
        scrollView.swipeUp()
    }
}
```

---

## 6. Usability Tests

### Checklist

#### Navigation
- [ ] User can find all main features within 3 clicks
- [ ] Keyboard shortcuts work as documented
- [ ] Tab order is logical
- [ ] Focus indicators are visible

#### Feedback
- [ ] Loading states are clear
- [ ] Error messages are actionable
- [ ] Success confirmations appear
- [ ] Progress is visible during long operations

#### Consistency
- [ ] Same actions produce same results
- [ ] Icons match their functions
- [ ] Terminology is consistent
- [ ] Visual hierarchy is clear

#### Recovery
- [ ] Undo is available where appropriate
- [ ] Accidental actions can be reversed
- [ ] Data is not lost on crash
- [ ] Session can be resumed

---

## 7. Security Tests

| Area | Test | Status |
|------|------|--------|
| API Keys | Never logged, masked in UI | TODO |
| Keychain | Proper access permissions | TODO |
| Network | HTTPS only, cert validation | TODO |
| Input | Sanitization, no injection | TODO |
| Sandbox | App sandbox enforced | DONE |

---

## 8. Running Tests

### Command Line

```bash
# Run all unit tests
xcodebuild test -scheme ConvergioApp -destination 'platform=macOS'

# Run UI tests
xcodebuild test -scheme ConvergioApp -destination 'platform=macOS' -only-testing:ConvergioAppUITests

# Run with coverage
xcodebuild test -scheme ConvergioApp -enableCodeCoverage YES

# Run specific test
xcodebuild test -scheme ConvergioApp -only-testing:ConvergioAppTests/OrchestratorViewModelTests/testInitialState
```

### CI Integration

Tests run automatically on:
- Every push to `feature/*` branches
- Every PR to `main`
- Nightly on `main`

---

## 9. Test Data

### Mock Data Location

- `Agent.previewAgents` - 6 sample agents
- `Message.previewMessages` - Sample conversation
- `OrchestratorViewModel.preview` - Full mock state
- `ConversationViewModel.preview` - Mock conversation

### Test Fixtures

```
ConvergioAppTests/
├── Fixtures/
│   ├── mock_api_response.json
│   ├── sample_conversation.json
│   └── error_responses.json
```

---

## 10. Coverage Goals

| Module | Target | Current |
|--------|--------|---------|
| ConvergioCore | 80% | ~60% |
| ViewModels | 90% | ~70% |
| Views | 50% | ~20% |
| Services | 80% | ~30% |

### Priority Areas

1. **Critical**: Keychain operations, API calls, cost tracking
2. **High**: ViewModel logic, message handling
3. **Medium**: UI interactions, navigation
4. **Low**: Animations, visual polish

---

## 11. Bug Reporting

When a test fails:

1. Check Xcode test report
2. Review screenshots (UI tests auto-capture)
3. Check `~/Library/Application Support/Convergio/Logs/`
4. Create GitHub issue with:
   - Test name
   - Expected vs actual
   - Screenshots/logs
   - Steps to reproduce

---

## 12. Continuous Improvement

- Review test coverage weekly
- Add tests for every bug fix
- Refactor flaky tests immediately
- Update mocks when API changes
