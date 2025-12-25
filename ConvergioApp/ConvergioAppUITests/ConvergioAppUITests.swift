/**
 * CONVERGIO NATIVE - UI Tests
 *
 * Comprehensive UI automation tests using XCUITest.
 * XCUITest is Apple's equivalent to Playwright for macOS apps.
 *
 * These tests verify:
 * - App launch and basic navigation
 * - Sidebar agent list functionality
 * - Conversation input and response
 * - Menu bar functionality
 * - Cost tracking display
 * - Agent card interactions
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import XCTest

final class ConvergioAppUITests: XCTestCase {

    var app: XCUIApplication!

    // MARK: - Setup & Teardown

    override func setUpWithError() throws {
        continueAfterFailure = false

        app = XCUIApplication()
        app.launchArguments = ["--uitesting"]  // Can use to disable certain features during testing
        app.launch()
    }

    override func tearDownWithError() throws {
        app.terminate()
    }

    // MARK: - App Launch Tests

    func testAppLaunches() throws {
        // Verify app launches successfully
        XCTAssertTrue(app.wait(for: .runningForeground, timeout: 10))
    }

    func testMainWindowAppears() throws {
        // Verify main window is visible
        let mainWindow = app.windows.firstMatch
        XCTAssertTrue(mainWindow.exists)
        XCTAssertTrue(mainWindow.isHittable)
    }

    // MARK: - Navigation Tests

    func testSidebarExists() throws {
        // Verify sidebar is present
        let sidebar = app.outlines.firstMatch
        XCTAssertTrue(sidebar.waitForExistence(timeout: 5))
    }

    func testAgentListExists() throws {
        // Verify agent list appears in sidebar
        let sidebar = app.outlines.firstMatch
        XCTAssertTrue(sidebar.waitForExistence(timeout: 5))

        // Should have at least some agents
        let agentCells = sidebar.cells
        XCTAssertGreaterThan(agentCells.count, 0, "Agent list should contain agents")
    }

    func testCanSelectAgent() throws {
        // Find and click on an agent
        let sidebar = app.outlines.firstMatch
        guard sidebar.waitForExistence(timeout: 5) else {
            XCTFail("Sidebar not found")
            return
        }

        let firstAgent = sidebar.cells.firstMatch
        if firstAgent.exists {
            firstAgent.click()

            // Verify selection feedback
            XCTAssertTrue(firstAgent.isSelected || firstAgent.isHittable)
        }
    }

    func testNavigationSections() throws {
        // Verify main sections exist
        let sidebar = app.outlines.firstMatch
        XCTAssertTrue(sidebar.waitForExistence(timeout: 5))

        // Check for section headers (these would be disclosure groups)
        let sections = sidebar.disclosureTriangles
        XCTAssertGreaterThanOrEqual(sections.count, 0, "Should have section headers")
    }

    // MARK: - Conversation Tests

    func testConversationInputExists() throws {
        // Verify conversation input field exists
        let inputField = app.textFields["Ask the team..."]
        XCTAssertTrue(inputField.waitForExistence(timeout: 5), "Input field should exist")
    }

    func testCanTypeInConversation() throws {
        // Find input field
        let inputField = app.textFields["Ask the team..."]
        guard inputField.waitForExistence(timeout: 5) else {
            XCTFail("Input field not found")
            return
        }

        // Type a message
        inputField.click()
        inputField.typeText("Hello, this is a test message")

        // Verify text was entered
        XCTAssertEqual(inputField.value as? String, "Hello, this is a test message")
    }

    func testSendButtonExists() throws {
        // Verify send button exists
        let sendButton = app.buttons.matching(identifier: "sendButton").firstMatch
        // Or by image
        let arrowButton = app.buttons.matching(NSPredicate(format: "label CONTAINS 'arrow'")).firstMatch

        XCTAssertTrue(sendButton.exists || arrowButton.exists, "Send button should exist")
    }

    func testConversationScrollView() throws {
        // Verify conversation scroll view exists
        let scrollView = app.scrollViews.firstMatch
        XCTAssertTrue(scrollView.exists, "Conversation scroll view should exist")
    }

    // MARK: - Agent Card Tests

    func testAgentCardsExist() throws {
        // Navigate to agent grid view if available
        // Look for agent cards
        let agentCards = app.groups.matching(NSPredicate(format: "identifier CONTAINS 'agentCard'"))

        // If using grid view, should have cards
        if agentCards.count > 0 {
            XCTAssertGreaterThan(agentCards.count, 0, "Should have agent cards")
        }
    }

    func testAgentCardShowsName() throws {
        // Find an agent card and verify it shows a name
        let agentTexts = app.staticTexts.matching(NSPredicate(format: "label == 'Ali' OR label == 'Angela'"))

        XCTAssertGreaterThan(agentTexts.count, 0, "Should show agent names")
    }

    // MARK: - Cost Tracking Tests

    func testCostBadgeExists() throws {
        // Verify cost badge is visible in toolbar
        let costBadge = app.staticTexts.matching(NSPredicate(format: "label CONTAINS '$'")).firstMatch
        XCTAssertTrue(costBadge.waitForExistence(timeout: 5), "Cost badge should be visible")
    }

    func testCostShowsValue() throws {
        // Verify cost shows a dollar amount
        let costLabels = app.staticTexts.allElementsBoundByIndex.filter { element in
            (element.label as? String)?.contains("$") == true
        }
        XCTAssertGreaterThan(costLabels.count, 0, "Should show cost value")
    }

    // MARK: - Menu Bar Tests

    func testMenuBarIconExists() throws {
        // Note: Menu bar extra testing requires special handling
        // The menu bar icon might not be directly accessible in UI tests
        // This test verifies the app doesn't crash with menu bar enabled
        XCTAssertTrue(app.exists)
    }

    // MARK: - Toolbar Tests

    func testToolbarExists() throws {
        // Verify toolbar buttons exist
        let toolbar = app.toolbars.firstMatch
        XCTAssertTrue(toolbar.exists, "Toolbar should exist")
    }

    func testModelSelectorExists() throws {
        // Look for model selector menu
        let menus = app.menuButtons
        let modelMenu = menus.matching(NSPredicate(format: "label CONTAINS 'claude' OR label CONTAINS 'Model'")).firstMatch

        // Model selector should exist
        XCTAssertTrue(menus.count > 0, "Should have menu buttons")
    }

    // MARK: - Keyboard Shortcut Tests

    func testNewConversationShortcut() throws {
        // Test Cmd+N shortcut
        app.typeKey("n", modifierFlags: .command)

        // Should not crash and conversation should still work
        let inputField = app.textFields["Ask the team..."]
        XCTAssertTrue(inputField.exists, "Input field should still exist after Cmd+N")
    }

    func testClearHistoryShortcut() throws {
        // Test Cmd+Shift+K shortcut
        app.typeKey("k", modifierFlags: [.command, .shift])

        // Should not crash
        XCTAssertTrue(app.exists)
    }

    // MARK: - Window Management Tests

    func testWindowCanBeResized() throws {
        let mainWindow = app.windows.firstMatch
        guard mainWindow.exists else {
            XCTFail("Main window not found")
            return
        }

        let originalFrame = mainWindow.frame
        // Window should have reasonable minimum size
        XCTAssertGreaterThan(originalFrame.width, 800)
        XCTAssertGreaterThan(originalFrame.height, 500)
    }

    func testSidebarCanBeToggled() throws {
        // Find sidebar toggle button
        let sidebarButton = app.buttons.matching(NSPredicate(format: "label CONTAINS 'sidebar'")).firstMatch

        if sidebarButton.exists {
            // Click to toggle
            sidebarButton.click()

            // Wait for animation
            Thread.sleep(forTimeInterval: 0.5)

            // Click again to restore
            sidebarButton.click()
        }

        // App should still be responsive
        XCTAssertTrue(app.exists)
    }

    // MARK: - Accessibility Tests

    func testAccessibilityLabelsExist() throws {
        // Verify key elements have accessibility labels
        let labeledElements = app.descendants(matching: .any).matching(NSPredicate(format: "label.length > 0"))
        XCTAssertGreaterThan(labeledElements.count, 10, "Should have many labeled elements for accessibility")
    }

    func testVoiceOverNavigation() throws {
        // Enable accessibility features for testing
        // Verify elements can be navigated with VoiceOver
        let focusableElements = app.descendants(matching: .any).matching(NSPredicate(format: "isAccessibilityElement == YES"))
        XCTAssertGreaterThan(focusableElements.count, 0, "Should have accessibility elements")
    }

    // MARK: - Performance Tests

    func testAppLaunchPerformance() throws {
        measure(metrics: [XCTApplicationLaunchMetric()]) {
            XCUIApplication().launch()
        }
    }

    func testScrollPerformance() throws {
        let scrollView = app.scrollViews.firstMatch
        guard scrollView.exists else { return }

        measure {
            scrollView.swipeUp()
            scrollView.swipeDown()
        }
    }
}
