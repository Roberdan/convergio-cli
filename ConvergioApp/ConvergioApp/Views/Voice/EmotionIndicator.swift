//
//  EmotionIndicator.swift
//  ConvergioApp
//
//  Created by Roberto on 2025-12-24.
//

import SwiftUI

/// Visual representation of detected emotions
struct EmotionIndicator: View {

    // MARK: - Properties

    let emotion: EmotionType
    let confidence: Double

    @State private var animationScale: CGFloat = 1.0
    @State private var animationOpacity: Double = 1.0

    // MARK: - Body

    var body: some View {
        HStack(spacing: 8) {
            // Emotion color indicator
            Circle()
                .fill(emotionColor)
                .frame(width: 12, height: 12)
                .shadow(color: emotionColor.opacity(0.6), radius: 4)
                .scaleEffect(animationScale)
                .animation(
                    .easeInOut(duration: 1.0).repeatForever(autoreverses: true),
                    value: animationScale
                )

            VStack(alignment: .leading, spacing: 2) {
                // Emotion name
                Text(emotion.displayName)
                    .font(.caption)
                    .fontWeight(.semibold)
                    .foregroundColor(.white)

                // Confidence bar
                GeometryReader { geometry in
                    ZStack(alignment: .leading) {
                        // Background
                        RoundedRectangle(cornerRadius: 2)
                            .fill(Color.white.opacity(0.2))
                            .frame(height: 3)

                        // Confidence level
                        RoundedRectangle(cornerRadius: 2)
                            .fill(emotionColor)
                            .frame(width: geometry.size.width * confidence, height: 3)
                            .animation(.easeInOut(duration: 0.3), value: confidence)
                    }
                }
                .frame(height: 3)
            }
            .frame(width: 80)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.black.opacity(0.3))
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .stroke(emotionColor.opacity(0.5), lineWidth: 1)
                )
        )
        .onAppear {
            animationScale = 1.2
        }
        .onChange(of: emotion) { _, _ in
            // Pulse animation on emotion change
            withAnimation(.easeInOut(duration: 0.2)) {
                animationOpacity = 0.5
            }
            withAnimation(.easeInOut(duration: 0.2).delay(0.2)) {
                animationOpacity = 1.0
            }
        }
        .opacity(animationOpacity)
    }

    // MARK: - Computed Properties

    private var emotionColor: Color {
        let rgb = emotion.color
        return Color(red: rgb.red, green: rgb.green, blue: rgb.blue)
    }
}

// MARK: - Emotion Grid View

/// Grid view showing all emotions with their current confidence levels
struct EmotionGridView: View {

    // MARK: - Properties

    let emotions: [EmotionType: Double]
    let currentEmotion: EmotionType

    private let columns = [
        GridItem(.flexible()),
        GridItem(.flexible()),
        GridItem(.flexible())
    ]

    // MARK: - Body

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Emotion Analysis")
                .font(.headline)
                .foregroundColor(.white)

            LazyVGrid(columns: columns, spacing: 12) {
                ForEach(EmotionType.allCases, id: \.self) { emotion in
                    emotionCell(emotion: emotion)
                }
            }
        }
        .padding()
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(Color.black.opacity(0.3))
        )
    }

    // MARK: - Emotion Cell

    private func emotionCell(emotion: EmotionType) -> some View {
        let confidence = emotions[emotion] ?? 0.0
        let isActive = emotion == currentEmotion

        return VStack(spacing: 8) {
            // Emotion icon/circle
            Circle()
                .fill(emotionColor(emotion))
                .frame(width: 40, height: 40)
                .shadow(
                    color: isActive ? emotionColor(emotion).opacity(0.8) : .clear,
                    radius: isActive ? 8 : 0
                )
                .overlay(
                    Circle()
                        .stroke(
                            isActive ? Color.white : Color.clear,
                            lineWidth: 2
                        )
                )
                .scaleEffect(isActive ? 1.1 : 1.0)
                .animation(.spring(response: 0.3, dampingFraction: 0.6), value: isActive)

            // Emotion name
            Text(emotion.displayName)
                .font(.caption2)
                .foregroundColor(isActive ? .white : .gray)
                .lineLimit(1)
                .minimumScaleFactor(0.8)

            // Confidence level
            Text(String(format: "%.0f%%", confidence * 100))
                .font(.caption2)
                .fontWeight(isActive ? .bold : .regular)
                .foregroundColor(isActive ? emotionColor(emotion) : .gray)
        }
        .padding(.vertical, 8)
        .frame(maxWidth: .infinity)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(isActive ? Color.white.opacity(0.1) : Color.clear)
        )
    }

    // MARK: - Helpers

    private func emotionColor(_ emotion: EmotionType) -> Color {
        let rgb = emotion.color
        return Color(red: rgb.red, green: rgb.green, blue: rgb.blue)
    }
}

// MARK: - Emotion Timeline View

/// Timeline view showing emotion changes over time
struct EmotionTimelineView: View {

    // MARK: - Properties

    struct EmotionPoint: Identifiable {
        let id = UUID()
        let timestamp: Date
        let emotion: EmotionType
        let confidence: Double
    }

    let emotionHistory: [EmotionPoint]
    let maxPoints: Int = 20

    // MARK: - Body

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Emotion Timeline")
                .font(.headline)
                .foregroundColor(.white)

            if emotionHistory.isEmpty {
                Text("No emotion data yet")
                    .font(.caption)
                    .foregroundColor(.gray)
                    .frame(maxWidth: .infinity, alignment: .center)
                    .padding(.vertical, 20)
            } else {
                ScrollView(.horizontal, showsIndicators: false) {
                    HStack(spacing: 4) {
                        ForEach(emotionHistory.suffix(maxPoints)) { point in
                            VStack(spacing: 4) {
                                // Emotion bar
                                RoundedRectangle(cornerRadius: 2)
                                    .fill(emotionColor(point.emotion))
                                    .frame(width: 12)
                                    .frame(height: max(20, point.confidence * 60))

                                // Timestamp (optional, for detailed view)
                                if emotionHistory.count <= 10 {
                                    Text(formatTime(point.timestamp))
                                        .font(.system(size: 8))
                                        .foregroundColor(.gray)
                                }
                            }
                        }
                    }
                    .padding(.vertical, 8)
                }
                .frame(height: 100)
            }
        }
        .padding()
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(Color.black.opacity(0.3))
        )
    }

    // MARK: - Helpers

    private func emotionColor(_ emotion: EmotionType) -> Color {
        let rgb = emotion.color
        return Color(red: rgb.red, green: rgb.green, blue: rgb.blue)
    }

    private func formatTime(_ date: Date) -> String {
        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm:ss"
        return formatter.string(from: date)
    }
}

// MARK: - Previews

#Preview("Emotion Indicator") {
    VStack(spacing: 20) {
        EmotionIndicator(emotion: .joy, confidence: 0.85)
        EmotionIndicator(emotion: .confusion, confidence: 0.60)
        EmotionIndicator(emotion: .curiosity, confidence: 0.92)
    }
    .padding()
    .background(Color.black)
}

#Preview("Emotion Grid") {
    EmotionGridView(
        emotions: [
            .joy: 0.85,
            .curiosity: 0.60,
            .neutral: 0.30,
            .confusion: 0.15
        ],
        currentEmotion: .joy
    )
    .padding()
    .background(Color.black)
}

#Preview("Emotion Timeline") {
    EmotionTimelineView(
        emotionHistory: [
            .init(timestamp: Date(), emotion: .neutral, confidence: 0.5),
            .init(timestamp: Date().addingTimeInterval(5), emotion: .curiosity, confidence: 0.7),
            .init(timestamp: Date().addingTimeInterval(10), emotion: .joy, confidence: 0.9),
            .init(timestamp: Date().addingTimeInterval(15), emotion: .excitement, confidence: 0.8),
            .init(timestamp: Date().addingTimeInterval(20), emotion: .confusion, confidence: 0.6)
        ]
    )
    .padding()
    .background(Color.black)
}
