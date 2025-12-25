/**
 * CONVERGIO NATIVE - Maestri Grid View
 *
 * Grid layout displaying all 18 AI maestri grouped by subject.
 * Features Liquid Glass design with hover effects and smooth transitions.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI

struct MaestriGridView: View {
    @Binding var selectedMaestro: Maestro?
    @State private var searchText = ""
    @State private var selectedSubject: Subject?
    @Namespace private var namespace

    init(selectedMaestro: Binding<Maestro?> = .constant(nil)) {
        _selectedMaestro = selectedMaestro
    }

    var body: some View {
        ScrollView {
            VStack(spacing: 24) {
                // Header with search and filters
                headerSection

                // Subject filter pills
                subjectFilterSection

                // Maestri grid
                maestriGrid
            }
            .padding()
        }
        .navigationTitle("I Tuoi Maestri AI")
    }

    // MARK: - Header Section

    private var headerSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Scegli il tuo maestro")
                .font(.title2.weight(.semibold))

            Text("18 maestri AI esperti pronti ad accompagnarti nello studio")
                .font(.subheadline)
                .foregroundStyle(.secondary)

            // Search field
            GlassTextField(
                placeholder: "Cerca maestro o materia...",
                text: $searchText,
                icon: "magnifyingglass"
            )
        }
    }

    // MARK: - Subject Filter Section

    private var subjectFilterSection: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 8) {
                // "All" filter
                filterPill(subject: nil, label: "Tutti")

                // Subject filters
                ForEach(Subject.allCases) { subject in
                    if Maestro.maestri(for: subject).count > 0 {
                        filterPill(subject: subject, label: subject.rawValue)
                    }
                }
            }
        }
    }

    private func filterPill(subject: Subject?, label: String) -> some View {
        Button {
            withAnimation(.spring(duration: 0.3)) {
                selectedSubject = subject
            }
        } label: {
            HStack(spacing: 6) {
                if let subject = subject {
                    Image(systemName: subject.icon)
                        .font(.caption)
                }
                Text(label)
                    .font(.subheadline.weight(.medium))
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(
                ZStack {
                    if selectedSubject == subject {
                        subject?.color ?? Color.accentColor
                    } else {
                        VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)
                        Color.primary.opacity(0.05)
                    }
                }
            )
            .foregroundStyle(selectedSubject == subject ? .white : .primary)
            .clipShape(Capsule())
            .overlay(
                Capsule()
                    .stroke(
                        selectedSubject == subject
                            ? (subject?.color ?? Color.accentColor).opacity(0.5)
                            : Color.primary.opacity(0.1),
                        lineWidth: 1
                    )
            )
        }
        .buttonStyle(.plain)
    }

    // MARK: - Maestri Grid

    private var maestriGrid: some View {
        let columns = [
            GridItem(.adaptive(minimum: 160, maximum: 200), spacing: 16)
        ]

        return LazyVGrid(columns: columns, spacing: 16) {
            ForEach(filteredMaestri) { maestro in
                MaestroCardView(maestro: maestro)
                    .onTapGesture {
                        selectedMaestro = maestro
                    }
            }
        }
    }

    // MARK: - Filtered Maestri

    private var filteredMaestri: [Maestro] {
        var maestri = Maestro.allMaestri

        // Filter by selected subject
        if let subject = selectedSubject {
            maestri = maestri.filter { $0.subject == subject }
        }

        // Filter by search text
        if !searchText.isEmpty {
            maestri = maestri.filter { maestro in
                maestro.name.localizedCaseInsensitiveContains(searchText) ||
                maestro.subject.rawValue.localizedCaseInsensitiveContains(searchText) ||
                maestro.specialization.localizedCaseInsensitiveContains(searchText)
            }
        }

        return maestri
    }
}

// MARK: - Maestro Card View

struct MaestroCardView: View {
    let maestro: Maestro
    @State private var isHovered = false

    var body: some View {
        VStack(spacing: 12) {
            // Avatar with subject ring
            ZStack {
                // Glow effect on hover
                if isHovered {
                    Circle()
                        .fill(maestro.color.opacity(0.3))
                        .frame(width: 80, height: 80)
                        .blur(radius: 10)
                }

                // Subject ring
                Circle()
                    .stroke(
                        LinearGradient(
                            colors: [maestro.color, maestro.color.opacity(0.6)],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        ),
                        lineWidth: 3
                    )
                    .frame(width: 70, height: 70)

                // Avatar image or fallback icon
                if let nsImage = NSImage(named: maestro.avatarName) {
                    Image(nsImage: nsImage)
                        .resizable()
                        .aspectRatio(contentMode: .fill)
                        .frame(width: 64, height: 64)
                        .clipShape(Circle())
                } else {
                    Image(systemName: maestro.icon)
                        .font(.system(size: 32))
                        .foregroundStyle(maestro.color)
                }
            }
            .frame(width: 80, height: 80)

            // Name
            Text(maestro.name)
                .font(.headline)
                .lineLimit(1)

            // Subject badge
            HStack(spacing: 4) {
                Image(systemName: maestro.subject.icon)
                    .font(.caption2)
                Text(maestro.subject.rawValue)
                    .font(.caption)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(maestro.color.opacity(0.15))
            .foregroundStyle(maestro.color)
            .clipShape(Capsule())

            // Specialization
            Text(maestro.specialization)
                .font(.caption2)
                .foregroundStyle(.secondary)
                .lineLimit(1)
        }
        .padding()
        .frame(maxWidth: .infinity)
        .frame(height: 200)
        .background(cardBackground)
        .clipShape(RoundedRectangle(cornerRadius: 16))
        .overlay(
            RoundedRectangle(cornerRadius: 16)
                .stroke(
                    isHovered ? maestro.color.opacity(0.5) : Color.white.opacity(0.1),
                    lineWidth: isHovered ? 2 : 0.5
                )
        )
        .shadow(
            color: isHovered ? maestro.color.opacity(0.3) : .black.opacity(0.1),
            radius: isHovered ? 12 : 8,
            x: 0,
            y: 4
        )
        .scaleEffect(isHovered ? 1.05 : 1.0)
        .animation(.spring(duration: 0.3), value: isHovered)
        .onHover { hovering in
            isHovered = hovering
        }
    }

    @ViewBuilder
    private var cardBackground: some View {
        ZStack {
            VisualEffectBlur(material: .hudWindow, blendingMode: .behindWindow)

            LinearGradient(
                colors: [
                    maestro.color.opacity(isHovered ? 0.15 : 0.08),
                    maestro.color.opacity(isHovered ? 0.08 : 0.03)
                ],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )

            // Subtle glass shine
            LinearGradient(
                colors: [
                    .white.opacity(0.15),
                    .clear,
                    .black.opacity(0.05)
                ],
                startPoint: .top,
                endPoint: .bottom
            )
        }
    }
}

// MARK: - Preview

#Preview {
    MaestriGridView()
        .frame(width: 900, height: 700)
}
