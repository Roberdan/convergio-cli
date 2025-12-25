/**
 * CONVERGIO NATIVE - Libretto View
 *
 * Main view for the digital gradebook showing overview of all subjects,
 * grade history, averages, and trends.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

// MARK: - Libretto View

struct LibrettoView: View {
    @StateObject private var gradebookManager = GradebookManager()
    @State private var selectedSubject: Subject?
    @State private var showingExportOptions = false
    @State private var showingAddGrade = false
    @State private var exportFormat: ExportFormat = .html

    var body: some View {
        NavigationSplitView {
            // Sidebar - Subject List
            sidebarContent
        } detail: {
            // Detail View
            if let subject = selectedSubject {
                SubjectGradesView(
                    subject: subject,
                    gradebookManager: gradebookManager
                )
            } else {
                overviewContent
            }
        }
        .navigationTitle("📚 Libretto Scolastico")
        .toolbar {
            toolbarContent
        }
        .sheet(isPresented: $showingExportOptions) {
            exportOptionsSheet
        }
        .sheet(isPresented: $showingAddGrade) {
            AddGradeView(gradebookManager: gradebookManager)
        }
    }

    // MARK: - Sidebar Content

    private var sidebarContent: some View {
        List(selection: $selectedSubject) {
            // Overview Section
            Section {
                NavigationLink(value: nil as Subject?) {
                    Label("Panoramica", systemImage: "chart.bar.doc.horizontal")
                        .font(.headline)
                }
            }

            // Subjects Section
            Section("Materie") {
                ForEach(gradebookManager.subjects) { subject in
                    NavigationLink(value: subject) {
                        SubjectRowView(
                            subject: subject,
                            statistics: gradebookManager.statistics(for: subject)
                        )
                    }
                }
            }
        }
        .navigationTitle("Libretto")
    }

    // MARK: - Overview Content

    private var overviewContent: some View {
        ScrollView {
            VStack(spacing: 24) {
                // Header Stats
                overallStatsSection

                // Performance Chart
                performanceChartSection

                // Recent Grades
                recentGradesSection

                // Subject Cards Grid
                subjectCardsGrid
            }
            .padding()
        }
        .background(Color(NSColor.windowBackgroundColor))
    }

    // MARK: - Overall Stats Section

    private var overallStatsSection: some View {
        VStack(spacing: 16) {
            Text("Rendimento Generale")
                .font(.title2)
                .fontWeight(.bold)

            HStack(spacing: 20) {
                LibrettoStatCard(
                    title: "Media Generale",
                    value: String(format: "%.2f", gradebookManager.overallAverage),
                    subtitle: GradeLevel.from(value: gradebookManager.overallAverage).rawValue,
                    color: GradeLevel.from(value: gradebookManager.overallAverage).color,
                    icon: "chart.line.uptrend.xyaxis"
                )

                LibrettoStatCard(
                    title: "Voti Totali",
                    value: "\(gradebookManager.totalGradesCount)",
                    subtitle: "\(gradebookManager.subjects.count) materie",
                    color: .blue,
                    icon: "doc.text"
                )

                LibrettoStatCard(
                    title: "Tasso di Successo",
                    value: String(format: "%.0f%%", gradebookManager.performanceReport().passingPercentage),
                    subtitle: "\(gradebookManager.passingGradesCount) sufficienti",
                    color: .green,
                    icon: "checkmark.circle"
                )
            }
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    // MARK: - Performance Chart Section

    private var performanceChartSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Andamento per Materia")
                .font(.headline)

            LazyVGrid(columns: [GridItem(.adaptive(minimum: 150))], spacing: 12) {
                ForEach(gradebookManager.allStatistics(), id: \.subject) { stats in
                    TrendCard(statistics: stats)
                        .onTapGesture {
                            selectedSubject = stats.subject
                        }
                }
            }
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    // MARK: - Recent Grades Section

    private var recentGradesSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("Voti Recenti")
                    .font(.headline)

                Spacer()

                Button("Vedi Tutti") {
                    selectedSubject = nil
                }
                .buttonStyle(.link)
            }

            ForEach(gradebookManager.recentGrades(limit: 5)) { grade in
                RecentGradeRow(grade: grade)
            }
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    // MARK: - Subject Cards Grid

    private var subjectCardsGrid: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Tutte le Materie")
                .font(.headline)

            LazyVGrid(columns: [GridItem(.adaptive(minimum: 280))], spacing: 16) {
                ForEach(gradebookManager.subjects) { subject in
                    SubjectCard(
                        subject: subject,
                        statistics: gradebookManager.statistics(for: subject)
                    )
                    .onTapGesture {
                        selectedSubject = subject
                    }
                }
            }
        }
        .padding()
    }

    // MARK: - Toolbar Content

    private var toolbarContent: some ToolbarContent {
        Group {
            ToolbarItem(placement: .primaryAction) {
                Button {
                    showingAddGrade = true
                } label: {
                    Label("Aggiungi Voto", systemImage: "plus")
                }
            }

            ToolbarItem(placement: .primaryAction) {
                Button {
                    showingExportOptions = true
                } label: {
                    Label("Esporta", systemImage: "square.and.arrow.up")
                }
            }
        }
    }

    // MARK: - Export Options Sheet

    private var exportOptionsSheet: some View {
        VStack(spacing: 20) {
            Text("Esporta Libretto")
                .font(.title2)
                .fontWeight(.bold)

            Picker("Formato", selection: $exportFormat) {
                Text("HTML").tag(ExportFormat.html)
                Text("PDF").tag(ExportFormat.pdf)
                Text("CSV").tag(ExportFormat.csv)
            }
            .pickerStyle(.segmented)
            .padding()

            HStack(spacing: 16) {
                Button("Annulla") {
                    showingExportOptions = false
                }
                .keyboardShortcut(.cancelAction)

                Button("Esporta") {
                    exportGradebook()
                    showingExportOptions = false
                }
                .keyboardShortcut(.defaultAction)
            }
        }
        .padding()
        .frame(width: 400, height: 200)
    }

    // MARK: - Export Function

    private func exportGradebook() {
        let panel = NSSavePanel()
        panel.nameFieldLabel = "Salva con nome:"
        panel.canCreateDirectories = true

        switch exportFormat {
        case .html:
            panel.allowedContentTypes = [.html]
            panel.nameFieldStringValue = "Libretto_\(Date().formatted(date: .abbreviated, time: .omitted)).html"

            if panel.runModal() == .OK, let url = panel.url {
                let html = gradebookManager.exportToHTML()
                try? html.write(to: url, atomically: true, encoding: .utf8)
            }

        case .pdf:
            panel.allowedContentTypes = [.pdf]
            panel.nameFieldStringValue = "Libretto_\(Date().formatted(date: .abbreviated, time: .omitted)).pdf"

            if panel.runModal() == .OK, let url = panel.url {
                if let pdfDocument = gradebookManager.exportToPDF() {
                    pdfDocument.write(to: url)
                }
            }

        case .csv:
            panel.allowedContentTypes = [.commaSeparatedText]
            panel.nameFieldStringValue = "Libretto_\(Date().formatted(date: .abbreviated, time: .omitted)).csv"

            if panel.runModal() == .OK, let url = panel.url {
                let csv = gradebookManager.exportToCSV()
                try? csv.write(to: url, atomically: true, encoding: .utf8)
            }
        }
    }
}

// MARK: - Export Format

enum ExportFormat {
    case html
    case pdf
    case csv
}

// MARK: - Subject Row View

struct SubjectRowView: View {
    let subject: Subject
    let statistics: SubjectStatistics

    var body: some View {
        HStack {
            Image(systemName: subject.icon)
                .foregroundColor(subject.color)
                .frame(width: 24)

            VStack(alignment: .leading, spacing: 2) {
                Text(subject.rawValue)
                    .font(.body)

                Text(String(format: "Media: %.2f", statistics.average))
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            Spacer()

            // Trend indicator
            Image(systemName: statistics.trend.icon)
                .foregroundColor(statistics.trend.color)
                .font(.caption)
        }
    }
}

// MARK: - Libretto Stat Card

private struct LibrettoStatCard: View {
    let title: String
    let value: String
    let subtitle: String
    let color: Color
    let icon: String

    var body: some View {
        VStack(spacing: 12) {
            Image(systemName: icon)
                .font(.system(size: 28))
                .foregroundColor(color)

            Text(value)
                .font(.system(size: 36, weight: .bold, design: .rounded))
                .foregroundColor(color)

            Text(title)
                .font(.caption)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)

            Text(subtitle)
                .font(.caption2)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity)
        .padding()
        .background(color.opacity(0.1))
        .cornerRadius(12)
    }
}

// MARK: - Trend Card

struct TrendCard: View {
    let statistics: SubjectStatistics

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Image(systemName: statistics.subject.icon)
                    .foregroundColor(statistics.subject.color)

                Text(statistics.subject.rawValue)
                    .font(.caption)
                    .lineLimit(1)

                Spacer()

                Image(systemName: statistics.trend.icon)
                    .foregroundColor(statistics.trend.color)
                    .font(.caption2)
            }

            Text(String(format: "%.2f", statistics.average))
                .font(.title2)
                .fontWeight(.bold)
                .foregroundColor(statistics.level.color)

            Text(statistics.level.rawValue)
                .font(.caption2)
                .foregroundColor(.secondary)
        }
        .padding()
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(statistics.subject.color.opacity(0.1))
        .cornerRadius(8)
    }
}

// MARK: - Recent Grade Row

struct RecentGradeRow: View {
    let grade: Grade

    var body: some View {
        HStack {
            Image(systemName: grade.subject.icon)
                .foregroundColor(grade.subject.color)
                .frame(width: 24)

            VStack(alignment: .leading, spacing: 2) {
                Text(grade.subject.rawValue)
                    .font(.body)

                Text(grade.topic)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            Spacer()

            VStack(alignment: .trailing, spacing: 2) {
                Text(grade.displayValue)
                    .font(.title3)
                    .fontWeight(.bold)
                    .foregroundColor(grade.color)

                Text(grade.formattedDate)
                    .font(.caption2)
                    .foregroundColor(.secondary)
            }
        }
        .padding(.vertical, 4)
    }
}

// MARK: - Subject Card

struct SubjectCard: View {
    let subject: Subject
    let statistics: SubjectStatistics

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            // Header
            HStack {
                Image(systemName: subject.icon)
                    .font(.title2)
                    .foregroundColor(subject.color)

                Text(subject.rawValue)
                    .font(.headline)

                Spacer()

                Image(systemName: statistics.trend.icon)
                    .foregroundColor(statistics.trend.color)
            }

            Divider()

            // Stats
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Media")
                        .font(.caption)
                        .foregroundColor(.secondary)

                    Text(String(format: "%.2f", statistics.average))
                        .font(.title)
                        .fontWeight(.bold)
                        .foregroundColor(statistics.level.color)

                    Text(statistics.level.rawValue)
                        .font(.caption2)
                        .foregroundColor(.secondary)
                }

                Spacer()

                VStack(alignment: .trailing, spacing: 4) {
                    Text("Voti")
                        .font(.caption)
                        .foregroundColor(.secondary)

                    Text("\(statistics.grades.count)")
                        .font(.title2)
                        .fontWeight(.semibold)

                    if let lastGrade = statistics.lastGrade {
                        Text("Ultimo: \(lastGrade.displayValue)")
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                }
            }

            // Trend info
            Text(statistics.trend.rawValue)
                .font(.caption)
                .foregroundColor(statistics.trend.color)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(statistics.trend.color.opacity(0.1))
                .cornerRadius(4)
        }
        .padding()
        .background(subject.color.opacity(0.05))
        .cornerRadius(12)
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(subject.color.opacity(0.3), lineWidth: 1)
        )
    }
}

// MARK: - Add Grade View

struct AddGradeView: View {
    @Environment(\.dismiss) private var dismiss
    @ObservedObject var gradebookManager: GradebookManager

    @State private var selectedSubject: Subject = .matematica
    @State private var gradeValue: Double = 6.0
    @State private var selectedType: GradeType = .test
    @State private var maestroName: String = ""
    @State private var topic: String = ""
    @State private var notes: String = ""
    @State private var date: Date = Date()

    var body: some View {
        VStack(spacing: 20) {
            Text("Aggiungi Voto")
                .font(.title2)
                .fontWeight(.bold)

            Form {
                Picker("Materia", selection: $selectedSubject) {
                    ForEach(Subject.allCases) { subject in
                        Label(subject.rawValue, systemImage: subject.icon)
                            .tag(subject)
                    }
                }

                Slider(value: $gradeValue, in: 1...10, step: 0.5) {
                    Text("Voto: \(String(format: "%.1f", gradeValue))")
                } minimumValueLabel: {
                    Text("1")
                } maximumValueLabel: {
                    Text("10")
                }

                Picker("Tipo", selection: $selectedType) {
                    ForEach(GradeType.allCases) { type in
                        Text(type.rawValue).tag(type)
                    }
                }

                TextField("Nome Maestro", text: $maestroName)

                TextField("Argomento", text: $topic)

                DatePicker("Data", selection: $date, displayedComponents: .date)

                TextField("Note", text: $notes, axis: .vertical)
                    .lineLimit(3...6)
            }
            .formStyle(.grouped)

            HStack(spacing: 16) {
                Button("Annulla") {
                    dismiss()
                }
                .keyboardShortcut(.cancelAction)

                Button("Aggiungi") {
                    addGrade()
                    dismiss()
                }
                .keyboardShortcut(.defaultAction)
                .disabled(maestroName.isEmpty || topic.isEmpty)
            }
        }
        .padding()
        .frame(width: 500, height: 600)
    }

    private func addGrade() {
        let grade = Grade(
            subject: selectedSubject,
            value: gradeValue,
            date: date,
            maestroName: maestroName,
            type: selectedType,
            notes: notes,
            topic: topic
        )
        gradebookManager.addGrade(grade)
    }
}

// MARK: - Preview

#Preview {
    LibrettoView()
        .frame(width: 1200, height: 800)
}
