/**
 * CONVERGIO NATIVE - Gradebook Manager
 *
 * Service for managing student grades, persistence, calculations, and export.
 * Handles local storage of grades and provides statistics and analytics.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import SwiftUI
import PDFKit

// MARK: - Gradebook Manager

@MainActor
class GradebookManager: ObservableObject {
    // MARK: - Published Properties

    @Published private(set) var grades: [Grade] = []
    @Published private(set) var isLoading: Bool = false
    @Published var errorMessage: String?

    // MARK: - Constants

    private let storageKey = "convergio.gradebook.grades"
    private let fileManager = FileManager.default

    // MARK: - Computed Properties

    var gradesBySubject: [Subject: [Grade]] {
        Dictionary(grouping: grades, by: { $0.subject })
    }

    var subjects: [Subject] {
        Array(Set(grades.map { $0.subject })).sorted { $0.rawValue < $1.rawValue }
    }

    var overallAverage: Double {
        guard !grades.isEmpty else { return 0 }
        let subjectAverages = subjects.map { subject in
            statistics(for: subject).average
        }
        return subjectAverages.reduce(0, +) / Double(subjectAverages.count)
    }

    var totalGradesCount: Int {
        grades.count
    }

    var passingGradesCount: Int {
        grades.filter { $0.isPassing }.count
    }

    var failingGradesCount: Int {
        grades.filter { !$0.isPassing }.count
    }

    // MARK: - Initialization

    init() {
        loadGrades()
    }

    // MARK: - CRUD Operations

    func addGrade(_ grade: Grade) {
        grades.append(grade)
        saveGrades()
    }

    func updateGrade(_ grade: Grade) {
        if let index = grades.firstIndex(where: { $0.id == grade.id }) {
            grades[index] = grade
            saveGrades()
        }
    }

    func deleteGrade(_ grade: Grade) {
        grades.removeAll { $0.id == grade.id }
        saveGrades()
    }

    func deleteGrades(at offsets: IndexSet, from subject: Subject) {
        let subjectGrades = grades(for: subject)
        let gradesToDelete = offsets.map { subjectGrades[$0] }
        grades.removeAll { grade in
            gradesToDelete.contains(where: { $0.id == grade.id })
        }
        saveGrades()
    }

    // MARK: - Query Methods

    func grades(for subject: Subject) -> [Grade] {
        grades.filter { $0.subject == subject }.sorted { $0.date > $1.date }
    }

    func recentGrades(limit: Int = 10) -> [Grade] {
        Array(grades.sorted { $0.date > $1.date }.prefix(limit))
    }

    func grades(for subject: Subject, type: GradeType) -> [Grade] {
        grades.filter { $0.subject == subject && $0.type == type }
    }

    func statistics(for subject: Subject) -> SubjectStatistics {
        SubjectStatistics(subject: subject, grades: grades(for: subject))
    }

    func allStatistics() -> [SubjectStatistics] {
        subjects.map { statistics(for: $0) }
    }

    // MARK: - Analytics

    func performanceReport() -> PerformanceReport {
        let stats = allStatistics()
        return PerformanceReport(
            overallAverage: overallAverage,
            totalGrades: totalGradesCount,
            passingGrades: passingGradesCount,
            failingGrades: failingGradesCount,
            subjectStatistics: stats,
            bestSubject: stats.max { $0.average < $1.average },
            worstSubject: stats.min { $0.average < $1.average },
            improvingSubjects: stats.filter { $0.trend == .improving },
            decliningSubjects: stats.filter { $0.trend == .declining }
        )
    }

    // MARK: - Persistence

    private func loadGrades() {
        isLoading = true

        guard let data = UserDefaults.standard.data(forKey: storageKey) else {
            // Load sample data if no data exists
            loadSampleData()
            isLoading = false
            return
        }

        do {
            let decoder = JSONDecoder()
            decoder.dateDecodingStrategy = .iso8601
            grades = try decoder.decode([Grade].self, from: data)
            isLoading = false
        } catch {
            errorMessage = "Errore nel caricamento dei voti: \(error.localizedDescription)"
            loadSampleData()
            isLoading = false
        }
    }

    private func saveGrades() {
        do {
            let encoder = JSONEncoder()
            encoder.dateEncodingStrategy = .iso8601
            let data = try encoder.encode(grades)
            UserDefaults.standard.set(data, forKey: storageKey)
        } catch {
            errorMessage = "Errore nel salvataggio dei voti: \(error.localizedDescription)"
        }
    }

    func clearAllGrades() {
        grades.removeAll()
        saveGrades()
    }

    // MARK: - Sample Data

    private func loadSampleData() {
        grades = [
            // Matematica
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
                subject: .matematica,
                value: 9.0,
                date: Date().addingTimeInterval(-86400 * 21),
                maestroName: "Pitagora",
                type: .quiz,
                notes: "Eccellente comprensione dei teoremi",
                topic: "Teorema di Pitagora"
            ),

            // Fisica
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
                subject: .fisica,
                value: 8.0,
                date: Date().addingTimeInterval(-86400 * 10),
                maestroName: "Galileo",
                type: .test,
                notes: "Molto bene, ottima esposizione",
                topic: "Cinematica"
            ),

            // Italiano
            Grade(
                subject: .italiano,
                value: 7.5,
                date: Date().addingTimeInterval(-86400 * 5),
                maestroName: "Manzoni",
                type: .project,
                notes: "Buona analisi del testo",
                topic: "I Promessi Sposi"
            ),
            Grade(
                subject: .italiano,
                value: 8.0,
                date: Date().addingTimeInterval(-86400 * 12),
                maestroName: "Manzoni",
                type: .homework,
                notes: "Ottimo tema, ben strutturato",
                topic: "Testo Argomentativo"
            ),

            // Storia
            Grade(
                subject: .storia,
                value: 7.0,
                date: Date().addingTimeInterval(-86400 * 8),
                maestroName: "Erodoto",
                type: .oral,
                notes: "Buona conoscenza degli eventi",
                topic: "Rivoluzione Francese"
            ),

            // Inglese
            Grade(
                subject: .inglese,
                value: 8.5,
                date: Date().addingTimeInterval(-86400 * 4),
                maestroName: "Shakespeare",
                type: .test,
                notes: "Excellent grammar and vocabulary",
                topic: "Present Perfect Tense"
            ),

            // Chimica
            Grade(
                subject: .chimica,
                value: 6.5,
                date: Date().addingTimeInterval(-86400 * 15),
                maestroName: "Curie",
                type: .test,
                notes: "Sufficiente, ma necessita più studio",
                topic: "Reazioni Chimiche"
            )
        ]
        saveGrades()
    }

    // MARK: - Export Functions

    func exportToHTML() -> String {
        var html = """
        <!DOCTYPE html>
        <html lang="it">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Libretto Scolastico - Convergio</title>
            <style>
                body {
                    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
                    margin: 40px;
                    background: #f5f5f5;
                }
                .container {
                    max-width: 1200px;
                    margin: 0 auto;
                    background: white;
                    padding: 40px;
                    border-radius: 12px;
                    box-shadow: 0 2px 10px rgba(0,0,0,0.1);
                }
                h1 {
                    color: #333;
                    border-bottom: 3px solid #007AFF;
                    padding-bottom: 10px;
                }
                h2 {
                    color: #555;
                    margin-top: 30px;
                }
                .summary {
                    display: grid;
                    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
                    gap: 20px;
                    margin: 20px 0;
                }
                .summary-card {
                    background: #f9f9f9;
                    padding: 20px;
                    border-radius: 8px;
                    border-left: 4px solid #007AFF;
                }
                .summary-card h3 {
                    margin: 0 0 10px 0;
                    color: #666;
                    font-size: 14px;
                    text-transform: uppercase;
                }
                .summary-card .value {
                    font-size: 32px;
                    font-weight: bold;
                    color: #333;
                }
                table {
                    width: 100%;
                    border-collapse: collapse;
                    margin: 20px 0;
                }
                th {
                    background: #007AFF;
                    color: white;
                    padding: 12px;
                    text-align: left;
                }
                td {
                    padding: 12px;
                    border-bottom: 1px solid #ddd;
                }
                tr:hover {
                    background: #f9f9f9;
                }
                .grade {
                    font-weight: bold;
                    font-size: 18px;
                }
                .grade.passing {
                    color: #28a745;
                }
                .grade.failing {
                    color: #dc3545;
                }
                .subject-section {
                    margin: 40px 0;
                }
                .footer {
                    margin-top: 40px;
                    padding-top: 20px;
                    border-top: 1px solid #ddd;
                    text-align: center;
                    color: #666;
                    font-size: 14px;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <h1>📚 Libretto Scolastico</h1>
                <p>Generato il: \(Date().formatted(date: .long, time: .shortened))</p>

                <div class="summary">
                    <div class="summary-card">
                        <h3>Media Generale</h3>
                        <div class="value">\(String(format: "%.2f", overallAverage))</div>
                    </div>
                    <div class="summary-card">
                        <h3>Voti Totali</h3>
                        <div class="value">\(totalGradesCount)</div>
                    </div>
                    <div class="summary-card">
                        <h3>Voti Sufficienti</h3>
                        <div class="value">\(passingGradesCount)</div>
                    </div>
                    <div class="summary-card">
                        <h3>Voti Insufficienti</h3>
                        <div class="value">\(failingGradesCount)</div>
                    </div>
                </div>
        """

        // Add each subject section
        for subject in subjects {
            let stats = statistics(for: subject)
            let subjectGrades = grades(for: subject)

            html += """
                <div class="subject-section">
                    <h2>\(subject.rawValue)</h2>
                    <p><strong>Media:</strong> \(String(format: "%.2f", stats.average)) (\(stats.level.rawValue))</p>
                    <p><strong>Andamento:</strong> \(stats.trend.rawValue)</p>

                    <table>
                        <thead>
                            <tr>
                                <th>Data</th>
                                <th>Tipo</th>
                                <th>Argomento</th>
                                <th>Voto</th>
                                <th>Maestro</th>
                                <th>Note</th>
                            </tr>
                        </thead>
                        <tbody>
            """

            for grade in subjectGrades {
                let gradeClass = grade.isPassing ? "passing" : "failing"
                html += """
                            <tr>
                                <td>\(grade.formattedDate)</td>
                                <td>\(grade.type.rawValue)</td>
                                <td>\(grade.topic)</td>
                                <td class="grade \(gradeClass)">\(grade.displayValue)</td>
                                <td>\(grade.maestroName)</td>
                                <td>\(grade.notes)</td>
                            </tr>
                """
            }

            html += """
                        </tbody>
                    </table>
                </div>
            """
        }

        html += """
                <div class="footer">
                    <p>Generato con Convergio - Sistema Educativo Intelligente</p>
                    <p>© 2025 Roberto D'Angelo & AI Team</p>
                </div>
            </div>
        </body>
        </html>
        """

        return html
    }

    func exportToPDF() -> PDFDocument? {
        let html = exportToHTML()

        guard let data = html.data(using: .utf8) else { return nil }

        // Create attributed string from HTML
        let options: [NSAttributedString.DocumentReadingOptionKey: Any] = [
            .documentType: NSAttributedString.DocumentType.html,
            .characterEncoding: String.Encoding.utf8.rawValue
        ]

        guard let attributedString = try? NSAttributedString(data: data, options: options, documentAttributes: nil) else {
            return nil
        }

        // Create PDF from attributed string
        let pdfData = NSMutableData()
        let pdfConsumer = CGDataConsumer(data: pdfData as CFMutableData)!

        var mediaBox = CGRect(x: 0, y: 0, width: 612, height: 792) // US Letter size
        let pdfContext = CGContext(consumer: pdfConsumer, mediaBox: &mediaBox, nil)!

        pdfContext.beginPDFPage(nil)
        // Note: This is a simplified version. For production, use proper PDF rendering
        pdfContext.endPDFPage()
        pdfContext.closePDF()

        return PDFDocument(data: pdfData as Data)
    }

    func exportToCSV() -> String {
        var csv = "Data,Materia,Tipo,Argomento,Voto,Livello,Maestro,Note\n"

        for grade in grades.sorted(by: { $0.date > $1.date }) {
            let row = [
                grade.formattedDate,
                grade.subject.rawValue,
                grade.type.rawValue,
                grade.topic,
                grade.displayValue,
                grade.level.rawValue,
                grade.maestroName,
                grade.notes
            ].map { $0.replacingOccurrences(of: ",", with: ";") }.joined(separator: ",")

            csv += row + "\n"
        }

        return csv
    }
}

// MARK: - Performance Report

struct PerformanceReport {
    let overallAverage: Double
    let totalGrades: Int
    let passingGrades: Int
    let failingGrades: Int
    let subjectStatistics: [SubjectStatistics]
    let bestSubject: SubjectStatistics?
    let worstSubject: SubjectStatistics?
    let improvingSubjects: [SubjectStatistics]
    let decliningSubjects: [SubjectStatistics]

    var passingPercentage: Double {
        guard totalGrades > 0 else { return 0 }
        return Double(passingGrades) / Double(totalGrades) * 100
    }

    var overallLevel: GradeLevel {
        GradeLevel.from(value: overallAverage)
    }
}
