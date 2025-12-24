/**
 * CONVERGIO NATIVE - Subject Grades View
 *
 * Detailed view for a single subject showing all grades chronologically,
 * average, trend, and maestro feedback history.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import Charts

// MARK: - Subject Grades View

struct SubjectGradesView: View {
    let subject: Subject
    @ObservedObject var gradebookManager: GradebookManager

    @State private var showingAddGrade = false
    @State private var selectedGrade: Grade?
    @State private var showingDeleteAlert = false
    @State private var gradeToDelete: Grade?

    private var statistics: SubjectStatistics {
        gradebookManager.statistics(for: subject)
    }

    private var grades: [Grade] {
        gradebookManager.grades(for: subject)
    }

    var body: some View {
        ScrollView {
            VStack(spacing: 24) {
                // Header
                headerSection

                // Statistics Cards
                statisticsSection

                // Grade Chart
                if !grades.isEmpty {
                    chartSection
                }

                // Grade Distribution
                if !grades.isEmpty {
                    distributionSection
                }

                // Grades List
                gradesListSection
            }
            .padding()
        }
        .navigationTitle(subject.rawValue)
        .toolbar {
            toolbarContent
        }
        .sheet(isPresented: $showingAddGrade) {
            AddGradeView(gradebookManager: gradebookManager)
        }
        .sheet(item: $selectedGrade) { grade in
            GradeDetailView(grade: grade, gradebookManager: gradebookManager)
        }
        .alert("Elimina Voto", isPresented: $showingDeleteAlert) {
            Button("Annulla", role: .cancel) { }
            Button("Elimina", role: .destructive) {
                if let grade = gradeToDelete {
                    gradebookManager.deleteGrade(grade)
                }
            }
        } message: {
            Text("Sei sicuro di voler eliminare questo voto?")
        }
    }

    // MARK: - Header Section

    private var headerSection: some View {
        HStack(spacing: 20) {
            Image(systemName: subject.icon)
                .font(.system(size: 48))
                .foregroundColor(subject.color)
                .frame(width: 80, height: 80)
                .background(subject.color.opacity(0.1))
                .clipShape(Circle())

            VStack(alignment: .leading, spacing: 8) {
                Text(subject.rawValue)
                    .font(.largeTitle)
                    .fontWeight(.bold)

                HStack(spacing: 16) {
                    Label("\(grades.count) voti", systemImage: "doc.text")
                        .font(.subheadline)
                        .foregroundColor(.secondary)

                    if let lastGrade = statistics.lastGrade {
                        Label("Ultimo: \(lastGrade.formattedDate)", systemImage: "clock")
                            .font(.subheadline)
                            .foregroundColor(.secondary)
                    }
                }
            }

            Spacer()
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    // MARK: - Statistics Section

    private var statisticsSection: some View {
        HStack(spacing: 16) {
            StatsCard(
                title: "Media Ponderata",
                value: String(format: "%.2f", statistics.average),
                subtitle: statistics.level.rawValue,
                color: statistics.level.color,
                icon: "chart.line.uptrend.xyaxis"
            )

            StatsCard(
                title: "Media Semplice",
                value: String(format: "%.2f", statistics.simpleAverage),
                subtitle: "\(grades.count) voti",
                color: .blue,
                icon: "number"
            )

            StatsCard(
                title: "Andamento",
                value: statistics.trend.rawValue,
                subtitle: trendDescription,
                color: statistics.trend.color,
                icon: statistics.trend.icon
            )

            if let bestGrade = statistics.bestGrade {
                StatsCard(
                    title: "Miglior Voto",
                    value: bestGrade.displayValue,
                    subtitle: bestGrade.formattedDate,
                    color: .green,
                    icon: "star.fill"
                )
            }
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    private var trendDescription: String {
        switch statistics.trend {
        case .improving: return "Continua così!"
        case .stable: return "Costante"
        case .declining: return "Serve impegno"
        }
    }

    // MARK: - Chart Section

    private var chartSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Andamento Voti")
                .font(.headline)

            Chart {
                ForEach(grades.sorted { $0.date < $1.date }) { grade in
                    LineMark(
                        x: .value("Data", grade.date),
                        y: .value("Voto", grade.value)
                    )
                    .foregroundStyle(subject.color.gradient)
                    .interpolationMethod(.catmullRom)

                    PointMark(
                        x: .value("Data", grade.date),
                        y: .value("Voto", grade.value)
                    )
                    .foregroundStyle(grade.color)
                    .symbolSize(100)
                    .annotation(position: .top) {
                        Text(grade.displayValue)
                            .font(.caption2)
                            .foregroundColor(grade.color)
                    }
                }

                // Average line
                RuleMark(y: .value("Media", statistics.average))
                    .foregroundStyle(.gray)
                    .lineStyle(StrokeStyle(lineWidth: 2, dash: [5, 5]))
                    .annotation(position: .trailing) {
                        Text("Media: \(String(format: "%.2f", statistics.average))")
                            .font(.caption)
                            .foregroundColor(.gray)
                    }

                // Passing line (6.0)
                RuleMark(y: .value("Sufficienza", 6.0))
                    .foregroundStyle(.orange.opacity(0.5))
                    .lineStyle(StrokeStyle(lineWidth: 1, dash: [3, 3]))
                    .annotation(position: .leading) {
                        Text("Sufficienza")
                            .font(.caption2)
                            .foregroundColor(.orange)
                    }
            }
            .chartYScale(domain: 0...10)
            .frame(height: 300)
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    // MARK: - Distribution Section

    private var distributionSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Distribuzione Voti")
                .font(.headline)

            let distribution = statistics.gradeDistribution.sorted { $0.key.rawValue > $1.key.rawValue }

            Chart {
                ForEach(distribution, id: \.key) { level, count in
                    BarMark(
                        x: .value("Livello", level.rawValue),
                        y: .value("Conteggio", count)
                    )
                    .foregroundStyle(level.color.gradient)
                    .annotation(position: .top) {
                        Text("\(count)")
                            .font(.caption)
                            .foregroundColor(level.color)
                    }
                }
            }
            .frame(height: 200)
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    // MARK: - Grades List Section

    private var gradesListSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Tutti i Voti")
                .font(.headline)

            if grades.isEmpty {
                emptyStateView
            } else {
                ForEach(grades) { grade in
                    GradeCard(grade: grade)
                        .onTapGesture {
                            selectedGrade = grade
                        }
                        .contextMenu {
                            Button {
                                selectedGrade = grade
                            } label: {
                                Label("Dettagli", systemImage: "info.circle")
                            }

                            Button(role: .destructive) {
                                gradeToDelete = grade
                                showingDeleteAlert = true
                            } label: {
                                Label("Elimina", systemImage: "trash")
                            }
                        }
                }
            }
        }
        .padding()
    }

    private var emptyStateView: some View {
        VStack(spacing: 16) {
            Image(systemName: "doc.text.fill")
                .font(.system(size: 48))
                .foregroundColor(.secondary)

            Text("Nessun voto ancora")
                .font(.headline)
                .foregroundColor(.secondary)

            Text("Aggiungi il primo voto per iniziare a tracciare i progressi")
                .font(.subheadline)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)

            Button {
                showingAddGrade = true
            } label: {
                Label("Aggiungi Voto", systemImage: "plus")
            }
            .buttonStyle(.borderedProminent)
        }
        .frame(maxWidth: .infinity)
        .padding(40)
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
    }

    // MARK: - Toolbar Content

    private var toolbarContent: some ToolbarContent {
        ToolbarItem(placement: .primaryAction) {
            Button {
                showingAddGrade = true
            } label: {
                Label("Aggiungi Voto", systemImage: "plus")
            }
        }
    }
}

// MARK: - Stats Card

struct StatsCard: View {
    let title: String
    let value: String
    let subtitle: String
    let color: Color
    let icon: String

    var body: some View {
        VStack(spacing: 8) {
            Image(systemName: icon)
                .font(.title2)
                .foregroundColor(color)

            Text(value)
                .font(.title)
                .fontWeight(.bold)
                .foregroundColor(color)

            Text(title)
                .font(.caption)
                .foregroundColor(.secondary)

            Text(subtitle)
                .font(.caption2)
                .foregroundColor(.secondary)
                .lineLimit(2)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity)
        .padding()
        .background(color.opacity(0.1))
        .cornerRadius(12)
    }
}

// MARK: - Grade Card

struct GradeCard: View {
    let grade: Grade

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                // Grade value
                VStack(alignment: .center, spacing: 4) {
                    Text(grade.displayValue)
                        .font(.system(size: 36, weight: .bold, design: .rounded))
                        .foregroundColor(grade.color)

                    Text(grade.level.rawValue)
                        .font(.caption2)
                        .foregroundColor(.secondary)
                }
                .frame(width: 80)
                .padding()
                .background(grade.color.opacity(0.1))
                .cornerRadius(12)

                VStack(alignment: .leading, spacing: 8) {
                    // Type and date
                    HStack {
                        Label(grade.type.rawValue, systemImage: grade.type.icon)
                            .font(.subheadline)
                            .foregroundColor(grade.subject.color)

                        Spacer()

                        Text(grade.formattedDate)
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }

                    // Topic
                    Text(grade.topic)
                        .font(.headline)

                    // Maestro
                    HStack {
                        Image(systemName: "person.fill")
                            .font(.caption)
                        Text(grade.maestroName)
                            .font(.caption)
                    }
                    .foregroundColor(.secondary)

                    // Notes
                    if !grade.notes.isEmpty {
                        Text(grade.notes)
                            .font(.caption)
                            .foregroundColor(.secondary)
                            .padding(.top, 4)
                            .italic()
                    }
                }
                .padding(.leading, 8)

                Spacer()
            }
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(grade.color.opacity(0.3), lineWidth: 2)
        )
    }
}

// MARK: - Grade Detail View

struct GradeDetailView: View {
    let grade: Grade
    @ObservedObject var gradebookManager: GradebookManager
    @Environment(\.dismiss) private var dismiss

    @State private var isEditing = false
    @State private var editedValue: Double
    @State private var editedNotes: String

    init(grade: Grade, gradebookManager: GradebookManager) {
        self.grade = grade
        self.gradebookManager = gradebookManager
        _editedValue = State(initialValue: grade.value)
        _editedNotes = State(initialValue: grade.notes)
    }

    var body: some View {
        VStack(spacing: 20) {
            // Header
            HStack {
                Image(systemName: grade.subject.icon)
                    .font(.largeTitle)
                    .foregroundColor(grade.subject.color)
                    .frame(width: 60, height: 60)
                    .background(grade.subject.color.opacity(0.1))
                    .clipShape(Circle())

                VStack(alignment: .leading, spacing: 4) {
                    Text(grade.subject.rawValue)
                        .font(.title2)
                        .fontWeight(.bold)

                    Text(grade.topic)
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                }

                Spacer()

                if !isEditing {
                    Text(grade.displayValue)
                        .font(.system(size: 48, weight: .bold, design: .rounded))
                        .foregroundColor(grade.color)
                }
            }

            Divider()

            if isEditing {
                // Edit mode
                Form {
                    Slider(value: $editedValue, in: 1...10, step: 0.5) {
                        Text("Voto: \(String(format: "%.1f", editedValue))")
                    }

                    TextField("Note", text: $editedNotes, axis: .vertical)
                        .lineLimit(3...6)
                }
            } else {
                // View mode
                VStack(alignment: .leading, spacing: 16) {
                    GradeDetailRow(label: "Tipo", value: grade.type.rawValue, icon: grade.type.icon)
                    GradeDetailRow(label: "Data", value: grade.formattedDate, icon: "calendar")
                    GradeDetailRow(label: "Maestro", value: grade.maestroName, icon: "person.fill")
                    GradeDetailRow(label: "Livello", value: grade.level.rawValue, icon: "star.fill")
                    GradeDetailRow(label: "Peso", value: String(format: "%.1f", grade.type.weight), icon: "scalemass")

                    if !grade.notes.isEmpty {
                        VStack(alignment: .leading, spacing: 8) {
                            HStack {
                                Image(systemName: "note.text")
                                    .foregroundColor(.secondary)
                                Text("Note")
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                            }

                            Text(grade.notes)
                                .font(.body)
                                .padding()
                                .frame(maxWidth: .infinity, alignment: .leading)
                                .background(Color(NSColor.controlBackgroundColor))
                                .cornerRadius(8)
                        }
                    }
                }
            }

            Spacer()

            // Actions
            HStack(spacing: 16) {
                if isEditing {
                    Button("Annulla") {
                        isEditing = false
                        editedValue = grade.value
                        editedNotes = grade.notes
                    }

                    Button("Salva") {
                        saveChanges()
                        isEditing = false
                    }
                    .buttonStyle(.borderedProminent)
                } else {
                    Button("Chiudi") {
                        dismiss()
                    }
                    .keyboardShortcut(.cancelAction)

                    Button("Modifica") {
                        isEditing = true
                    }
                    .buttonStyle(.borderedProminent)
                }
            }
        }
        .padding()
        .frame(width: 500, height: 600)
    }

    private func saveChanges() {
        let updatedGrade = Grade(
            id: grade.id,
            subject: grade.subject,
            value: editedValue,
            date: grade.date,
            maestroName: grade.maestroName,
            type: grade.type,
            notes: editedNotes,
            topic: grade.topic
        )
        gradebookManager.updateGrade(updatedGrade)
    }
}

// MARK: - Grade Detail Row

private struct GradeDetailRow: View {
    let label: String
    let value: String
    let icon: String

    var body: some View {
        HStack {
            Image(systemName: icon)
                .foregroundColor(.secondary)
                .frame(width: 24)

            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
                .frame(width: 80, alignment: .leading)

            Text(value)
                .font(.body)

            Spacer()
        }
    }
}

// MARK: - Preview

#Preview {
    NavigationStack {
        SubjectGradesView(
            subject: .matematica,
            gradebookManager: GradebookManager()
        )
    }
    .frame(width: 900, height: 700)
}
