/**
 * CONVERGIO NATIVE - Error Views
 *
 * Graceful error handling and display components.
 * Shows network failures, provider errors, and budget exceeded states.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import ConvergioCore

// MARK: - Error Banner

struct ErrorBanner: View {
    let error: OrchestratorError
    let onDismiss: () -> Void
    let onRetry: (() -> Void)?

    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: errorIcon)
                .font(.title2)
                .foregroundStyle(errorColor)

            VStack(alignment: .leading, spacing: 4) {
                Text(errorTitle)
                    .font(.subheadline.weight(.semibold))

                Text(error.localizedDescription)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            if let onRetry {
                Button("Retry") {
                    onRetry()
                }
                .buttonStyle(.bordered)
            }

            Button {
                onDismiss()
            } label: {
                Image(systemName: "xmark.circle.fill")
                    .foregroundStyle(.secondary)
            }
            .buttonStyle(.plain)
        }
        .padding(12)
        .background(errorColor.opacity(0.1))
        .clipShape(RoundedRectangle(cornerRadius: 12))
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(errorColor.opacity(0.3), lineWidth: 1)
        )
    }

    private var errorIcon: String {
        switch error {
        case .notInitialized:
            return "exclamationmark.circle"
        case .initializationFailed:
            return "xmark.circle"
        case .processingFailed:
            return "exclamationmark.triangle"
        case .cancelled:
            return "stop.circle"
        case .budgetExceeded:
            return "dollarsign.circle"
        }
    }

    private var errorTitle: String {
        switch error {
        case .notInitialized:
            return "Not Ready"
        case .initializationFailed:
            return "Initialization Failed"
        case .processingFailed:
            return "Processing Error"
        case .cancelled:
            return "Request Cancelled"
        case .budgetExceeded:
            return "Budget Exceeded"
        }
    }

    private var errorColor: Color {
        switch error {
        case .cancelled:
            return .orange
        case .budgetExceeded:
            return .purple
        default:
            return .red
        }
    }
}

// MARK: - Empty State View

struct EmptyStateView: View {
    let icon: String
    let title: String
    let message: String
    let action: (() -> Void)?
    let actionTitle: String?

    init(
        icon: String,
        title: String,
        message: String,
        action: (() -> Void)? = nil,
        actionTitle: String? = nil
    ) {
        self.icon = icon
        self.title = title
        self.message = message
        self.action = action
        self.actionTitle = actionTitle
    }

    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: icon)
                .font(.system(size: 48))
                .foregroundStyle(.secondary)

            Text(title)
                .font(.title2.weight(.semibold))

            Text(message)
                .font(.body)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
                .frame(maxWidth: 300)

            if let action, let actionTitle {
                Button(actionTitle) {
                    action()
                }
                .buttonStyle(.borderedProminent)
                .padding(.top, 8)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

// MARK: - Network Error View

struct NetworkErrorView: View {
    let onRetry: () -> Void

    var body: some View {
        EmptyStateView(
            icon: "wifi.exclamationmark",
            title: "Connection Error",
            message: "Unable to connect to the AI providers. Please check your internet connection and try again.",
            action: onRetry,
            actionTitle: "Retry Connection"
        )
    }
}

// MARK: - Budget Exceeded View

struct BudgetExceededView: View {
    let currentCost: Double
    let budgetLimit: Double
    let onAdjustBudget: () -> Void

    var body: some View {
        VStack(spacing: 20) {
            // Warning icon
            ZStack {
                Circle()
                    .fill(Color.orange.opacity(0.1))
                    .frame(width: 80, height: 80)

                Image(systemName: "dollarsign.circle.fill")
                    .font(.system(size: 40))
                    .foregroundStyle(.orange)
            }

            Text("Budget Limit Reached")
                .font(.title2.weight(.semibold))

            Text("You've spent $\(String(format: "%.2f", currentCost)) of your $\(String(format: "%.0f", budgetLimit)) budget.")
                .font(.body)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)

            // Progress bar
            VStack(spacing: 8) {
                GlassProgressBar(progress: 1.0, tint: .orange)
                    .frame(width: 200, height: 8)

                HStack {
                    Text("$0")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    Spacer()
                    Text("$\(String(format: "%.0f", budgetLimit))")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
                .frame(width: 200)
            }

            Button {
                onAdjustBudget()
            } label: {
                Label("Increase Budget", systemImage: "plus.circle")
            }
            .buttonStyle(.borderedProminent)
            .padding(.top, 8)

            Text("Increase your budget to continue using the AI team.")
                .font(.caption)
                .foregroundStyle(.tertiary)
        }
        .padding(32)
        .background(
            RoundedRectangle(cornerRadius: 20)
                .fill(Color(nsColor: .windowBackgroundColor))
                .shadow(color: .black.opacity(0.1), radius: 20)
        )
    }
}

// MARK: - Loading Overlay

struct LoadingOverlay: View {
    let message: String

    var body: some View {
        VStack(spacing: 16) {
            ProgressView()
                .scaleEffect(1.5)

            Text(message)
                .font(.subheadline)
                .foregroundStyle(.secondary)
        }
        .padding(32)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(.ultraThinMaterial)
        )
    }
}

// MARK: - Initialization View

struct InitializationView: View {
    @ObservedObject var viewModel: OrchestratorViewModel

    var body: some View {
        VStack(spacing: 24) {
            // App icon placeholder
            Image(systemName: "brain.head.profile")
                .font(.system(size: 64))
                .foregroundStyle(.purple.gradient)

            Text("Convergio")
                .font(.largeTitle.weight(.bold))

            if viewModel.isLoading {
                VStack(spacing: 12) {
                    ProgressView()
                    Text("Initializing AI team...")
                        .font(.subheadline)
                        .foregroundStyle(.secondary)
                }
            } else if !viewModel.isReady {
                Button {
                    Task {
                        await viewModel.initialize()
                    }
                } label: {
                    Label("Start", systemImage: "play.circle.fill")
                }
                .buttonStyle(.borderedProminent)
                .controlSize(.large)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(
            LinearGradient(
                colors: [
                    Color.purple.opacity(0.05),
                    Color.blue.opacity(0.05)
                ],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
        )
    }
}

// MARK: - Preview

#Preview("Error Banner") {
    VStack(spacing: 16) {
        ErrorBanner(
            error: .processingFailed("Connection timeout"),
            onDismiss: {},
            onRetry: {}
        )

        ErrorBanner(
            error: .budgetExceeded,
            onDismiss: {},
            onRetry: nil
        )

        ErrorBanner(
            error: .cancelled,
            onDismiss: {},
            onRetry: nil
        )
    }
    .padding()
}

#Preview("Empty States") {
    VStack(spacing: 32) {
        EmptyStateView(
            icon: "bubble.left.and.bubble.right",
            title: "No Conversations",
            message: "Start a conversation with your AI team.",
            action: {},
            actionTitle: "New Conversation"
        )
        .frame(height: 250)

        NetworkErrorView(onRetry: {})
            .frame(height: 250)
    }
}

#Preview("Budget Exceeded") {
    BudgetExceededView(
        currentCost: 10.45,
        budgetLimit: 10.0,
        onAdjustBudget: {}
    )
    .frame(width: 400, height: 400)
}
