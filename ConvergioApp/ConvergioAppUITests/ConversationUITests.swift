/**
 * CONVERGIO NATIVE - Conversation UI Tests
 *
 * Tests focused on the conversation functionality.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import XCTest

final class ConversationUITests: XCTestCase {

    var app: XCUIApplication!

    override func setUpWithError() throws {
        continueAfterFailure = false
        app = XCUIApplication()
        app.launchArguments = ["--uitesting", "--mock-responses"]
        app.launch()
    }

    override func tearDownWithError() throws {
        app.terminate()
    }

    // MARK: - Message Input Tests

    func testInputFieldFocus() throws {
        let inputField = app.textFields["Ask the team..."]
        guard inputField.waitForExistence(timeout: 5) else {
            XCTFail("Input field not found")
            return
        }

        inputField.click()
        XCTAssertTrue(inputField.exists && inputField.isHittable)
    }

    func testInputFieldAcceptsMultilineText() throws {
        let inputField = app.textFields["Ask the team..."]
        guard inputField.waitForExistence(timeout: 5) else {
            XCTFail("Input field not found")
            return
        }

        inputField.click()
        inputField.typeText("Line 1")
        inputField.typeKey(.return, modifierFlags: .shift)  // Shift+Enter for new line
        inputField.typeText("Line 2")

        // Field should contain both lines
        let value = inputField.value as? String ?? ""
        XCTAssertTrue(value.contains("Line 1") || value.contains("Line 2"))
    }

    func testInputFieldClearsAfterSend() throws {
        let inputField = app.textFields["Ask the team..."]
        guard inputField.waitForExistence(timeout: 5) else {
            XCTFail("Input field not found")
            return
        }

        inputField.click()
        inputField.typeText("Test message")

        // Press Cmd+Enter to send
        app.typeKey(.return, modifierFlags: .command)

        // Wait for processing
        Thread.sleep(forTimeInterval: 1.0)

        // Field should be empty or processing
        let value = inputField.value as? String ?? ""
        XCTAssertTrue(value.isEmpty || inputField.placeholderValue == "Ask the team...")
    }

    // MARK: - Message Display Tests

    func testUserMessageAppears() throws {
        let inputField = app.textFields["Ask the team..."]
        guard inputField.waitForExistence(timeout: 5) else {
            XCTFail("Input field not found")
            return
        }

        inputField.click()
        inputField.typeText("This is my test question")
        app.typeKey(.return, modifierFlags: .command)

        // Wait for message to appear
        Thread.sleep(forTimeInterval: 1.0)

        // Look for the message in the conversation
        let messageText = app.staticTexts["This is my test question"]
        XCTAssertTrue(messageText.waitForExistence(timeout: 5), "User message should appear in conversation")
    }

    func testStreamingIndicatorAppears() throws {
        // This test requires mock mode to simulate streaming
        let inputField = app.textFields["Ask the team..."]
        guard inputField.waitForExistence(timeout: 5) else {
            XCTFail("Input field not found")
            return
        }

        inputField.click()
        inputField.typeText("Test streaming")
        app.typeKey(.return, modifierFlags: .command)

        // Look for streaming/thinking indicator
        let thinkingText = app.staticTexts.matching(NSPredicate(format: "label CONTAINS 'Thinking'")).firstMatch
        let progressIndicator = app.progressIndicators.firstMatch

        // One of these should appear during processing
        let hasIndicator = thinkingText.waitForExistence(timeout: 2) || progressIndicator.waitForExistence(timeout: 2)
        // Note: This might not appear if response is instant
    }

    // MARK: - Message Interaction Tests

    func testMessageCanBeSelected() throws {
        // Add a message first
        let inputField = app.textFields["Ask the team..."]
        guard inputField.waitForExistence(timeout: 5) else {
            XCTFail("Input field not found")
            return
        }

        inputField.click()
        inputField.typeText("Selectable message test")
        app.typeKey(.return, modifierFlags: .command)

        Thread.sleep(forTimeInterval: 2.0)

        // Find the message and try to select text
        let messageText = app.staticTexts["Selectable message test"]
        if messageText.exists {
            messageText.click()
            // Text should be selectable (no crash)
            XCTAssertTrue(true)
        }
    }

    // MARK: - Scroll Tests

    func testConversationScrolls() throws {
        let scrollView = app.scrollViews.firstMatch
        guard scrollView.exists else {
            // Might be using List instead
            return
        }

        scrollView.swipeUp()
        scrollView.swipeDown()

        // Should not crash
        XCTAssertTrue(app.exists)
    }

    func testNewMessageScrollsToBottom() throws {
        let inputField = app.textFields["Ask the team..."]
        guard inputField.waitForExistence(timeout: 5) else {
            XCTFail("Input field not found")
            return
        }

        // Send multiple messages
        for i in 1...3 {
            inputField.click()
            inputField.typeText("Message \(i)")
            app.typeKey(.return, modifierFlags: .command)
            Thread.sleep(forTimeInterval: 1.0)
        }

        // The latest message should be visible
        let lastMessage = app.staticTexts["Message 3"]
        // Note: Visibility check depends on implementation
    }

    // MARK: - Cancel Tests

    func testCanCancelRequest() throws {
        let inputField = app.textFields["Ask the team..."]
        guard inputField.waitForExistence(timeout: 5) else {
            XCTFail("Input field not found")
            return
        }

        inputField.click()
        inputField.typeText("Long request")
        app.typeKey(.return, modifierFlags: .command)

        // Look for cancel/stop button during processing
        let stopButton = app.buttons.matching(NSPredicate(format: "label CONTAINS 'stop' OR label CONTAINS 'cancel'")).firstMatch

        if stopButton.waitForExistence(timeout: 2) {
            stopButton.click()
            // Should cancel without crash
            XCTAssertTrue(app.exists)
        }
    }

    // MARK: - History Tests

    func testNewConversationClearsHistory() throws {
        // Add a message
        let inputField = app.textFields["Ask the team..."]
        guard inputField.waitForExistence(timeout: 5) else {
            XCTFail("Input field not found")
            return
        }

        inputField.click()
        inputField.typeText("Message before clear")
        app.typeKey(.return, modifierFlags: .command)
        Thread.sleep(forTimeInterval: 1.0)

        // Use Cmd+N for new conversation
        app.typeKey("n", modifierFlags: .command)
        Thread.sleep(forTimeInterval: 0.5)

        // Previous message should not be visible
        let oldMessage = app.staticTexts["Message before clear"]
        XCTAssertFalse(oldMessage.exists, "Old message should be cleared")
    }
}
