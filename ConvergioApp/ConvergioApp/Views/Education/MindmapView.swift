/**
 * CONVERGIO NATIVE - MindmapView
 *
 * Interactive concept map visualization for educational content.
 * Displays hierarchical relationships between concepts with
 * pan, zoom, and node interaction support.
 *
 * Part of the Scuola 2026 Student Experience (Tasks 3.3.1 - 3.3.4)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import UniformTypeIdentifiers

// MARK: - Data Models

struct MindmapNode: Identifiable, Codable, Equatable {
    let id: UUID
    var title: String
    var description: String?
    var color: String
    var position: CGPoint
    var children: [UUID]
    var isExpanded: Bool
    var level: Int
    var maestroId: String?

    init(
        id: UUID = UUID(),
        title: String,
        description: String? = nil,
        color: String = "blue",
        position: CGPoint = .zero,
        children: [UUID] = [],
        isExpanded: Bool = true,
        level: Int = 0,
        maestroId: String? = nil
    ) {
        self.id = id
        self.title = title
        self.description = description
        self.color = color
        self.position = position
        self.children = children
        self.isExpanded = isExpanded
        self.level = level
        self.maestroId = maestroId
    }

    var nodeColor: Color {
        switch color {
        case "red": return .red
        case "orange": return .orange
        case "yellow": return .yellow
        case "green": return .green
        case "blue": return .blue
        case "purple": return .purple
        case "pink": return .pink
        default: return .blue
        }
    }

    var nodeSize: CGFloat {
        switch level {
        case 0: return 120  // Root
        case 1: return 100  // Main topics
        case 2: return 80   // Subtopics
        default: return 60  // Details
        }
    }
}

struct MindmapConnection: Identifiable {
    let id = UUID()
    let from: UUID
    let to: UUID
}

// MARK: - Mindmap Data

@MainActor
final class MindmapData: ObservableObject {
    @Published var nodes: [UUID: MindmapNode] = [:]
    @Published var rootId: UUID?
    @Published var selectedNodeId: UUID?
    @Published var editingNodeId: UUID?

    var connections: [MindmapConnection] {
        var result: [MindmapConnection] = []
        for (_, node) in nodes {
            for childId in node.children {
                result.append(MindmapConnection(from: node.id, to: childId))
            }
        }
        return result
    }

    // MARK: - CRUD Operations

    func createRoot(title: String, color: String = "blue") -> MindmapNode {
        let node = MindmapNode(
            title: title,
            color: color,
            position: CGPoint(x: 400, y: 300),
            level: 0
        )
        nodes[node.id] = node
        rootId = node.id
        return node
    }

    func addChild(to parentId: UUID, title: String, color: String? = nil) -> MindmapNode? {
        guard var parent = nodes[parentId] else { return nil }

        let childColor = color ?? parent.color
        let angle = Double(parent.children.count) * (2 * .pi / 6) - .pi / 2
        let radius: Double = 180 + Double(parent.level) * 30

        let childPosition = CGPoint(
            x: parent.position.x + CGFloat(Foundation.cos(angle) * radius),
            y: parent.position.y + CGFloat(Foundation.sin(angle) * radius)
        )

        let child = MindmapNode(
            title: title,
            color: childColor,
            position: childPosition,
            level: parent.level + 1
        )

        nodes[child.id] = child
        parent.children.append(child.id)
        nodes[parentId] = parent

        return child
    }

    func updateNode(_ id: UUID, title: String? = nil, description: String? = nil, color: String? = nil) {
        guard var node = nodes[id] else { return }
        if let title = title { node.title = title }
        if let description = description { node.description = description }
        if let color = color { node.color = color }
        nodes[id] = node
    }

    func moveNode(_ id: UUID, to position: CGPoint) {
        guard var node = nodes[id] else { return }
        node.position = position
        nodes[id] = node
    }

    func deleteNode(_ id: UUID) {
        guard let node = nodes[id] else { return }

        // Delete children recursively
        for childId in node.children {
            deleteNode(childId)
        }

        // Remove from parent
        for (parentId, var parentNode) in nodes {
            if let index = parentNode.children.firstIndex(of: id) {
                parentNode.children.remove(at: index)
                nodes[parentId] = parentNode
            }
        }

        nodes.removeValue(forKey: id)

        if rootId == id {
            rootId = nil
        }
    }

    func toggleExpanded(_ id: UUID) {
        guard var node = nodes[id] else { return }
        node.isExpanded.toggle()
        nodes[id] = node
    }

    // MARK: - Layout

    func autoLayout() {
        guard let rootId = rootId, let root = nodes[rootId] else { return }
        layoutNode(root, at: root.position, angle: 0, spread: 2 * .pi)
    }

    private func layoutNode(_ node: MindmapNode, at position: CGPoint, angle: Double, spread: Double) {
        var updatedNode = node
        updatedNode.position = position
        nodes[node.id] = updatedNode

        guard node.isExpanded, !node.children.isEmpty else { return }

        let childCount = node.children.count
        let angleStep = spread / Double(max(childCount, 1))
        let startAngle = angle - spread / 2 + angleStep / 2
        let radius: Double = 150 + Double(node.level) * 30

        for (index, childId) in node.children.enumerated() {
            guard let child = nodes[childId] else { continue }
            let childAngle = startAngle + angleStep * Double(index)
            let childPosition = CGPoint(
                x: position.x + CGFloat(Foundation.cos(childAngle) * radius),
                y: position.y + CGFloat(Foundation.sin(childAngle) * radius)
            )
            layoutNode(child, at: childPosition, angle: childAngle, spread: angleStep * 0.8)
        }
    }

    // MARK: - Export

    func exportAsImage() -> NSImage? {
        guard !nodes.isEmpty else { return nil }

        // Calculate bounds
        var minX: CGFloat = .infinity
        var minY: CGFloat = .infinity
        var maxX: CGFloat = -.infinity
        var maxY: CGFloat = -.infinity

        for node in nodes.values {
            minX = min(minX, node.position.x - node.nodeSize)
            minY = min(minY, node.position.y - node.nodeSize)
            maxX = max(maxX, node.position.x + node.nodeSize)
            maxY = max(maxY, node.position.y + node.nodeSize)
        }

        let padding: CGFloat = 50
        let width = maxX - minX + padding * 2
        let height = maxY - minY + padding * 2

        let image = NSImage(size: NSSize(width: width, height: height))
        image.lockFocus()

        // Draw background
        NSColor.white.setFill()
        NSRect(x: 0, y: 0, width: width, height: height).fill()

        // Offset for drawing
        let offset = CGPoint(x: -minX + padding, y: -minY + padding)

        // Draw connections
        for connection in connections {
            guard let from = nodes[connection.from],
                  let to = nodes[connection.to] else { continue }

            let path = NSBezierPath()
            path.move(to: NSPoint(
                x: from.position.x + offset.x,
                y: from.position.y + offset.y
            ))
            path.line(to: NSPoint(
                x: to.position.x + offset.x,
                y: to.position.y + offset.y
            ))
            NSColor.gray.withAlphaComponent(0.5).setStroke()
            path.lineWidth = 2
            path.stroke()
        }

        // Draw nodes
        for node in nodes.values {
            let rect = NSRect(
                x: node.position.x + offset.x - node.nodeSize / 2,
                y: node.position.y + offset.y - node.nodeSize / 2,
                width: node.nodeSize,
                height: node.nodeSize
            )

            let path = NSBezierPath(ovalIn: rect)
            NSColor(node.nodeColor).withAlphaComponent(0.3).setFill()
            path.fill()
            NSColor(node.nodeColor).setStroke()
            path.lineWidth = 2
            path.stroke()

            // Draw title
            let textRect = NSRect(
                x: rect.minX + 10,
                y: rect.midY - 10,
                width: rect.width - 20,
                height: 20
            )
            let attributes: [NSAttributedString.Key: Any] = [
                .font: NSFont.systemFont(ofSize: 12, weight: .semibold),
                .foregroundColor: NSColor.black
            ]
            node.title.draw(in: textRect, withAttributes: attributes)
        }

        image.unlockFocus()
        return image
    }
}

// MARK: - MindmapView

struct MindmapView: View {
    @StateObject private var data = MindmapData()
    @State private var scale: CGFloat = 1.0
    @State private var offset: CGSize = .zero
    @State private var showingAddNode = false
    @State private var showingExport = false

    let subject: String?
    let maestro: Maestro?

    init(subject: String? = nil, maestro: Maestro? = nil) {
        self.subject = subject
        self.maestro = maestro
    }

    var body: some View {
        ZStack {
            // Canvas background
            Color(nsColor: .windowBackgroundColor)
                .ignoresSafeArea()

            // Mindmap canvas
            GeometryReader { geometry in
                ZStack {
                    // Grid background
                    GridBackground()
                        .scaleEffect(scale)
                        .offset(offset)

                    // Connections
                    ForEach(data.connections) { connection in
                        ConnectionLine(
                            from: data.nodes[connection.from]?.position ?? .zero,
                            to: data.nodes[connection.to]?.position ?? .zero
                        )
                        .scaleEffect(scale)
                        .offset(offset)
                    }

                    // Nodes
                    ForEach(Array(data.nodes.values), id: \.id) { node in
                        if shouldShowNode(node) {
                            NodeView(
                                node: node,
                                isSelected: data.selectedNodeId == node.id,
                                isEditing: data.editingNodeId == node.id,
                                onSelect: { data.selectedNodeId = node.id },
                                onDoubleClick: { data.editingNodeId = node.id },
                                onMove: { newPos in data.moveNode(node.id, to: newPos) },
                                onToggleExpand: { data.toggleExpanded(node.id) }
                            )
                            .scaleEffect(scale)
                            .offset(x: offset.width + node.position.x * scale,
                                   y: offset.height + node.position.y * scale)
                        }
                    }
                }
                .gesture(magnificationGesture)
                .gesture(panGesture)
                .onTapGesture {
                    data.selectedNodeId = nil
                    data.editingNodeId = nil
                }
            }

            // Toolbar overlay
            VStack {
                toolbar
                Spacer()
                zoomControls
            }
        }
        .onAppear { setupInitialData() }
    }

    // MARK: - Toolbar

    private var toolbar: some View {
        HStack {
            Text(subject ?? "Concept Map")
                .font(.headline)

            Spacer()

            Button {
                if let selectedId = data.selectedNodeId {
                    let _ = data.addChild(to: selectedId, title: "New Concept")
                } else if let rootId = data.rootId {
                    let _ = data.addChild(to: rootId, title: "New Concept")
                } else {
                    let _ = data.createRoot(title: subject ?? "Main Topic")
                }
            } label: {
                Label("Add Node", systemImage: "plus.circle.fill")
            }

            Button {
                data.autoLayout()
            } label: {
                Label("Auto Layout", systemImage: "rectangle.3.group")
            }

            Button {
                if let selectedId = data.selectedNodeId {
                    data.deleteNode(selectedId)
                }
            } label: {
                Label("Delete", systemImage: "trash")
            }
            .disabled(data.selectedNodeId == nil)

            Menu {
                Button("Export as PNG") {
                    exportAsPNG()
                }
                Button("Export as PDF") {
                    exportAsPDF()
                }
            } label: {
                Label("Export", systemImage: "square.and.arrow.up")
            }
        }
        .padding()
        .background(.ultraThinMaterial)
    }

    // MARK: - Zoom Controls

    private var zoomControls: some View {
        HStack {
            Spacer()

            VStack(spacing: 8) {
                Button {
                    withAnimation { scale = min(scale * 1.2, 3.0) }
                } label: {
                    Image(systemName: "plus.magnifyingglass")
                }

                Text("\(Int(scale * 100))%")
                    .font(.caption)
                    .foregroundColor(.secondary)

                Button {
                    withAnimation { scale = max(scale / 1.2, 0.3) }
                } label: {
                    Image(systemName: "minus.magnifyingglass")
                }

                Divider()
                    .frame(width: 20)

                Button {
                    withAnimation {
                        scale = 1.0
                        offset = .zero
                    }
                } label: {
                    Image(systemName: "arrow.counterclockwise")
                }
            }
            .padding(8)
            .background(.ultraThinMaterial)
            .cornerRadius(8)
        }
        .padding()
    }

    // MARK: - Gestures

    private var magnificationGesture: some Gesture {
        MagnificationGesture()
            .onChanged { value in
                scale = min(max(value, 0.3), 3.0)
            }
    }

    private var panGesture: some Gesture {
        DragGesture()
            .onChanged { value in
                offset = CGSize(
                    width: offset.width + value.translation.width,
                    height: offset.height + value.translation.height
                )
            }
    }

    // MARK: - Helpers

    private func shouldShowNode(_ node: MindmapNode) -> Bool {
        // Check if any ancestor is collapsed
        for (_, potentialParent) in data.nodes {
            if potentialParent.children.contains(node.id) && !potentialParent.isExpanded {
                return false
            }
        }
        return true
    }

    private func setupInitialData() {
        if data.nodes.isEmpty {
            let root = data.createRoot(
                title: subject ?? "Central Topic",
                color: maestro?.color.description ?? "blue"
            )

            // Add some example children
            if let _ = subject {
                let _ = data.addChild(to: root.id, title: "Concept 1")
                let _ = data.addChild(to: root.id, title: "Concept 2")
                let _ = data.addChild(to: root.id, title: "Concept 3")
            }

            data.autoLayout()
        }
    }

    private func exportAsPNG() {
        guard let image = data.exportAsImage() else { return }

        let savePanel = NSSavePanel()
        savePanel.allowedContentTypes = [.png]
        savePanel.nameFieldStringValue = "\(subject ?? "mindmap").png"

        savePanel.begin { response in
            if response == .OK, let url = savePanel.url {
                if let tiffData = image.tiffRepresentation,
                   let bitmap = NSBitmapImageRep(data: tiffData),
                   let pngData = bitmap.representation(using: .png, properties: [:]) {
                    try? pngData.write(to: url)
                }
            }
        }
    }

    private func exportAsPDF() {
        // PDF export implementation would go here
    }
}

// MARK: - Supporting Views

struct GridBackground: View {
    let gridSize: CGFloat = 50

    var body: some View {
        Canvas { context, size in
            let columns = Int(size.width / gridSize) + 1
            let rows = Int(size.height / gridSize) + 1

            for col in 0...columns {
                let x = CGFloat(col) * gridSize
                var path = Path()
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: size.height))
                context.stroke(path, with: .color(.gray.opacity(0.1)), lineWidth: 1)
            }

            for row in 0...rows {
                let y = CGFloat(row) * gridSize
                var path = Path()
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: size.width, y: y))
                context.stroke(path, with: .color(.gray.opacity(0.1)), lineWidth: 1)
            }
        }
    }
}

struct ConnectionLine: View {
    let from: CGPoint
    let to: CGPoint

    var body: some View {
        Path { path in
            path.move(to: from)

            // Bezier curve for smooth connection
            let midX = (from.x + to.x) / 2
            let control1 = CGPoint(x: midX, y: from.y)
            let control2 = CGPoint(x: midX, y: to.y)

            path.addCurve(to: to, control1: control1, control2: control2)
        }
        .stroke(Color.gray.opacity(0.5), lineWidth: 2)
    }
}

struct NodeView: View {
    let node: MindmapNode
    let isSelected: Bool
    let isEditing: Bool
    let onSelect: () -> Void
    let onDoubleClick: () -> Void
    let onMove: (CGPoint) -> Void
    let onToggleExpand: () -> Void

    @State private var editText: String = ""

    var body: some View {
        ZStack {
            // Node circle
            Circle()
                .fill(node.nodeColor.opacity(0.2))
                .frame(width: node.nodeSize, height: node.nodeSize)
                .overlay(
                    Circle()
                        .stroke(
                            isSelected ? Color.accentColor : node.nodeColor,
                            lineWidth: isSelected ? 3 : 2
                        )
                )
                .shadow(color: node.nodeColor.opacity(0.3), radius: isSelected ? 8 : 4)

            // Content
            VStack(spacing: 4) {
                if isEditing {
                    TextField("", text: $editText)
                        .textFieldStyle(.plain)
                        .font(.system(size: node.level == 0 ? 14 : 12, weight: .semibold))
                        .multilineTextAlignment(.center)
                        .frame(width: node.nodeSize - 20)
                        .onSubmit {
                            // Save edit
                        }
                } else {
                    Text(node.title)
                        .font(.system(size: node.level == 0 ? 14 : 12, weight: .semibold))
                        .lineLimit(2)
                        .multilineTextAlignment(.center)
                }

                if !node.children.isEmpty {
                    Button {
                        onToggleExpand()
                    } label: {
                        Image(systemName: node.isExpanded ? "chevron.up.circle.fill" : "chevron.down.circle.fill")
                            .font(.caption)
                            .foregroundColor(node.nodeColor)
                    }
                    .buttonStyle(.plain)
                }
            }
            .frame(width: node.nodeSize - 20)
        }
        .onTapGesture(count: 2) {
            editText = node.title
            onDoubleClick()
        }
        .onTapGesture {
            onSelect()
        }
        .gesture(
            DragGesture()
                .onChanged { value in
                    onMove(CGPoint(
                        x: node.position.x + value.translation.width,
                        y: node.position.y + value.translation.height
                    ))
                }
        )
    }
}

// MARK: - Previews

#if DEBUG
struct MindmapView_Previews: PreviewProvider {
    static var previews: some View {
        MindmapView(subject: "Mathematics")
            .frame(width: 800, height: 600)
    }
}
#endif
