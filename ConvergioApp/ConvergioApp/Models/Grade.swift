/**
 * CONVERGIO NATIVE - Grade Model
 *
 * Model for student grades following the Italian grading scale (1-10).
 * Integrates with Maestro model for tracking educational progress.
 *
 * Italian Grading Scale:
 * - 10: Eccellente
 * - 9: Ottimo
 * - 8: Distinto
 * - 7: Buono
 * - 6: Sufficiente (passing)
 * - 5: Insufficiente
 * - 4: Gravemente insufficiente
 * - 1-3: Very rare, severe failure
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import SwiftUI

// MARK: - Grade Type

enum GradeType: String, CaseIterable, Codable, Identifiable {
    case quiz = "Quiz"
    case homework = "Compito a Casa"
    case test = "Verifica"
    case participation = "Partecipazione"
    case project = "Progetto"
    case oral = "Interrogazione"

    var id: String { rawValue }

    var icon: String {
        switch self {
        case .quiz: return "questionmark.circle"
        case .homework: return "pencil.and.outline"
        case .test: return "doc.text"
        case .participation: return "hand.raised"
        case .project: return "folder"
        case .oral: return "mic"
        }
    }

    var weight: Double {
        switch self {
        case .quiz: return 0.5
        case .homework: return 0.7
        case .test: return 1.5
        case .participation: return 0.5
        case .project: return 1.2
        case .oral: return 1.3
        }
    }
}

// MARK: - Grade Level

enum GradeLevel: String, Codable {
    case eccellente = "Eccellente"
    case ottimo = "Ottimo"
    case distinto = "Distinto"
    case buono = "Buono"
    case sufficiente = "Sufficiente"
    case insufficiente = "Insufficiente"
    case gravementeInsufficiente = "Gravemente Insufficiente"

    static func from(value: Double) -> GradeLevel {
        switch value {
        case 10.0: return .eccellente
        case 9.0..<10.0: return .ottimo
        case 8.0..<9.0: return .distinto
        case 7.0..<8.0: return .buono
        case 6.0..<7.0: return .sufficiente
        case 5.0..<6.0: return .insufficiente
        default: return .gravementeInsufficiente
        }
    }

    var color: Color {
        switch self {
        case .eccellente: return .purple
        case .ottimo: return .green
        case .distinto: return .blue
        case .buono: return .cyan
        case .sufficiente: return .yellow
        case .insufficiente: return .orange
        case .gravementeInsufficiente: return .red
        }
    }

    var isPassing: Bool {
        switch self {
        case .eccellente, .ottimo, .distinto, .buono, .sufficiente:
            return true
        case .insufficiente, .gravementeInsufficiente:
            return false
        }
    }
}

// MARK: - Grade Model

struct Grade: Identifiable, Codable, Hashable {
    let id: UUID
    let subject: Subject
    let value: Double
    let date: Date
    let maestroName: String
    let type: GradeType
    let notes: String
    let topic: String

    init(
        id: UUID = UUID(),
        subject: Subject,
        value: Double,
        date: Date = Date(),
        maestroName: String,
        type: GradeType,
        notes: String = "",
        topic: String = ""
    ) {
        self.id = id
        self.subject = subject
        self.value = min(10.0, max(1.0, value)) // Clamp to 1-10 range
        self.date = date
        self.maestroName = maestroName
        self.type = type
        self.notes = notes
        self.topic = topic
    }

    // Computed properties
    var level: GradeLevel {
        GradeLevel.from(value: value)
    }

    var isPassing: Bool {
        level.isPassing
    }

    var color: Color {
        level.color
    }

    var displayValue: String {
        if value == floor(value) {
            return String(format: "%.0f", value)
        } else {
            return String(format: "%.1f", value)
        }
    }

    var formattedDate: String {
        let formatter = DateFormatter()
        formatter.dateStyle = .medium
        formatter.timeStyle = .none
        formatter.locale = Locale(identifier: "it_IT")
        return formatter.string(from: date)
    }
}

// MARK: - Subject Extension for Grading

extension Subject: Codable {
    enum CodingKeys: String, CodingKey {
        case rawValue
    }

    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        let rawValue = try container.decode(String.self, forKey: .rawValue)
        self = Subject(rawValue: rawValue) ?? .matematica
    }

    func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(self.rawValue, forKey: .rawValue)
    }
}

// MARK: - Grade Statistics

struct SubjectStatistics {
    let subject: Subject
    let grades: [Grade]

    var average: Double {
        guard !grades.isEmpty else { return 0 }
        let weightedSum = grades.reduce(0.0) { $0 + ($1.value * $1.type.weight) }
        let totalWeight = grades.reduce(0.0) { $0 + $1.type.weight }
        return weightedSum / totalWeight
    }

    var simpleAverage: Double {
        guard !grades.isEmpty else { return 0 }
        return grades.reduce(0.0) { $0 + $1.value } / Double(grades.count)
    }

    var trend: Trend {
        guard grades.count >= 2 else { return .stable }

        let sortedGrades = grades.sorted { $0.date < $1.date }
        let recentCount = min(3, sortedGrades.count)
        let recent = Array(sortedGrades.suffix(recentCount))
        let older = Array(sortedGrades.prefix(sortedGrades.count - recentCount))

        guard !older.isEmpty else { return .stable }

        let recentAvg = recent.reduce(0.0) { $0 + $1.value } / Double(recent.count)
        let olderAvg = older.reduce(0.0) { $0 + $1.value } / Double(older.count)

        let difference = recentAvg - olderAvg

        if difference > 0.5 {
            return .improving
        } else if difference < -0.5 {
            return .declining
        } else {
            return .stable
        }
    }

    var level: GradeLevel {
        GradeLevel.from(value: average)
    }

    var lastGrade: Grade? {
        grades.sorted { $0.date > $1.date }.first
    }

    var bestGrade: Grade? {
        grades.max { $0.value < $1.value }
    }

    var worstGrade: Grade? {
        grades.min { $0.value < $1.value }
    }

    var gradeDistribution: [GradeLevel: Int] {
        var distribution: [GradeLevel: Int] = [:]
        for grade in grades {
            distribution[grade.level, default: 0] += 1
        }
        return distribution
    }
}

// MARK: - Trend

enum Trend: String {
    case improving = "In Miglioramento"
    case stable = "Stabile"
    case declining = "In Calo"

    var icon: String {
        switch self {
        case .improving: return "arrow.up.circle.fill"
        case .stable: return "arrow.forward.circle.fill"
        case .declining: return "arrow.down.circle.fill"
        }
    }

    var color: Color {
        switch self {
        case .improving: return .green
        case .stable: return .blue
        case .declining: return .orange
        }
    }
}

// MARK: - Preview Data

extension Grade {
    static let preview = Grade(
        subject: .matematica,
        value: 8.5,
        date: Date(),
        maestroName: "Euclide",
        type: .test,
        notes: "Ottimo lavoro sulla geometria analitica",
        topic: "Geometria Analitica"
    )

    static let previewGrades: [Grade] = [
        Grade(
            subject: .matematica,
            value: 8.5,
            date: Date().addingTimeInterval(-86400 * 7),
            maestroName: "Euclide",
            type: .test,
            notes: "Ottimo lavoro sulla geometria analitica",
            topic: "Geometria Analitica"
        ),
        Grade(
            subject: .matematica,
            value: 7.0,
            date: Date().addingTimeInterval(-86400 * 14),
            maestroName: "Euclide",
            type: .homework,
            notes: "Buon lavoro, continua così",
            topic: "Equazioni di secondo grado"
        ),
        Grade(
            subject: .fisica,
            value: 9.0,
            date: Date().addingTimeInterval(-86400 * 3),
            maestroName: "Feynman",
            type: .oral,
            notes: "Eccellente comprensione dei concetti",
            topic: "Dinamica"
        ),
        Grade(
            subject: .italiano,
            value: 6.5,
            date: Date().addingTimeInterval(-86400 * 10),
            maestroName: "Manzoni",
            type: .project,
            notes: "Sufficiente, ma può fare di più",
            topic: "Analisi del testo"
        )
    ]
}
