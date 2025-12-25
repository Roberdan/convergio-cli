/**
 * CONVERGIO NATIVE - Photo Homework View
 *
 * Camera/photo library picker for homework input with OCR support.
 * Allows students to photograph their homework problems.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import SwiftUI
import AppKit
import UniformTypeIdentifiers

struct PhotoHomeworkView: View {
    @ObservedObject var viewModel: HomeworkViewModel
    @Binding var isPresented: Bool

    @State private var selectedImage: NSImage?
    @State private var imageData: Data?
    @State private var extractedText = ""
    @State private var isProcessingOCR = false
    @State private var detectedSubject: HomeworkType = .other
    @State private var showingFilePicker = false

    var body: some View {
        VStack(spacing: 20) {
            Text("Importa da Foto")
                .font(.title2.bold())

            // Image preview or picker
            if let image = selectedImage {
                ImagePreviewSection(
                    image: image,
                    onRemove: {
                        selectedImage = nil
                        imageData = nil
                        extractedText = ""
                    }
                )
            } else {
                ImagePickerSection(
                    onSelectPhoto: { selectPhotoFromLibrary() },
                    onTakePhoto: { takePhoto() }
                )
            }

            Divider()

            // OCR processing status
            if isProcessingOCR {
                OCRProcessingView()
            } else if !extractedText.isEmpty {
                ExtractedTextSection(
                    text: extractedText,
                    detectedSubject: detectedSubject,
                    onSubjectChange: { subject in
                        detectedSubject = subject
                    },
                    onTextChange: { text in
                        extractedText = text
                    }
                )
            }

            Spacer()

            // Actions
            HStack(spacing: 12) {
                Button("Annulla") {
                    isPresented = false
                }
                .buttonStyle(.bordered)

                Button("Inizia Assistenza") {
                    Task {
                        await viewModel.startHomework(
                            problem: extractedText,
                            subject: detectedSubject,
                            imageData: imageData
                        )
                        isPresented = false
                    }
                }
                .buttonStyle(.borderedProminent)
                .disabled(extractedText.isEmpty || viewModel.isProcessing)
            }
        }
        .padding(24)
        .frame(width: 600, height: 500)
        .onChange(of: selectedImage) { oldImage, newImage in
            if let image = newImage {
                processImage(image)
            }
        }
    }

    // MARK: - Image Selection

    private func selectPhotoFromLibrary() {
        let panel = NSOpenPanel()
        panel.allowsMultipleSelection = false
        panel.canChooseDirectories = false
        panel.allowedContentTypes = [.image, .png, .jpeg, .heic]
        panel.title = "Select Homework Photo"
        panel.message = "Choose a photo of your homework problem"

        panel.begin { response in
            if response == .OK, let url = panel.url {
                loadImage(from: url)
            }
        }
    }

    private func takePhoto() {
        // Note: macOS doesn't have built-in camera UI like iOS
        // This is a placeholder that opens file picker
        // In a real implementation, you would use AVFoundation for camera access
        selectPhotoFromLibrary()
    }

    private func loadImage(from url: URL) {
        guard let image = NSImage(contentsOf: url) else { return }
        selectedImage = image

        // Convert to data
        if let tiffData = image.tiffRepresentation,
           let bitmap = NSBitmapImageRep(data: tiffData) {
            imageData = bitmap.representation(using: .jpeg, properties: [:])
        }
    }

    // MARK: - OCR Processing

    private func processImage(_ image: NSImage) {
        isProcessingOCR = true

        Task {
            // Simulate OCR processing
            // In a real implementation, use Vision framework or cloud OCR
            try? await Task.sleep(nanoseconds: 1_500_000_000) // 1.5 seconds

            // Placeholder: In reality, use Vision.framework VNRecognizeTextRequest
            extractedText = extractTextFromImage(image)

            // Auto-detect subject
            detectedSubject = await viewModel.detectSubject(from: extractedText)

            isProcessingOCR = false
        }
    }

    /// Extract text from image using Vision framework (placeholder)
    private func extractTextFromImage(_ image: NSImage) -> String {
        // TODO: Implement real OCR using Vision framework
        // For now, return placeholder text
        return """
        [OCR Placeholder]

        Il testo verrà estratto dall'immagine usando Vision framework.

        Per ora, inserisci manualmente il problema qui sotto.
        """
    }
}

// MARK: - Image Picker Section

struct ImagePickerSection: View {
    let onSelectPhoto: () -> Void
    let onTakePhoto: () -> Void

    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: "photo.on.rectangle.angled")
                .font(.system(size: 64))
                .foregroundStyle(.purple.gradient)

            Text("Scegli come importare il problema")
                .font(.headline)

            HStack(spacing: 16) {
                Button {
                    onSelectPhoto()
                } label: {
                    VStack(spacing: 8) {
                        Image(systemName: "photo.on.rectangle")
                            .font(.title)
                        Text("Scegli Foto")
                            .font(.caption)
                    }
                    .frame(width: 140, height: 100)
                }
                .buttonStyle(.bordered)

                Button {
                    onTakePhoto()
                } label: {
                    VStack(spacing: 8) {
                        Image(systemName: "camera")
                            .font(.title)
                        Text("Scatta Foto")
                            .font(.caption)
                    }
                    .frame(width: 140, height: 100)
                }
                .buttonStyle(.bordered)
            }

            // Info text
            Text("Assicurati che il testo sia leggibile e ben illuminato")
                .font(.caption)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
                .padding(.horizontal)
        }
        .frame(maxHeight: .infinity)
    }
}

// MARK: - Image Preview

struct ImagePreviewSection: View {
    let image: NSImage
    let onRemove: () -> Void

    var body: some View {
        VStack(spacing: 12) {
            HStack {
                Text("Anteprima Immagine")
                    .font(.headline)

                Spacer()

                Button {
                    onRemove()
                } label: {
                    Label("Rimuovi", systemImage: "xmark.circle.fill")
                        .font(.caption)
                }
                .buttonStyle(.plain)
                .foregroundStyle(.red)
            }

            // Image preview
            Image(nsImage: image)
                .resizable()
                .scaledToFit()
                .frame(maxHeight: 200)
                .clipShape(RoundedRectangle(cornerRadius: 12))
                .overlay(
                    RoundedRectangle(cornerRadius: 12)
                        .stroke(Color.secondary.opacity(0.2), lineWidth: 1)
                )
        }
    }
}

// MARK: - OCR Processing

struct OCRProcessingView: View {
    @State private var animating = false

    var body: some View {
        VStack(spacing: 16) {
            ProgressView()
                .scaleEffect(1.5)

            HStack(spacing: 8) {
                Image(systemName: "doc.text.viewfinder")
                    .font(.title3)
                    .foregroundStyle(.purple)
                    .symbolEffect(.pulse, isActive: true)

                Text("Riconoscimento testo in corso...")
                    .font(.callout)
            }

            Text("Sto analizzando l'immagine per estrarre il testo")
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .padding()
        .frame(maxWidth: .infinity)
        .background(Color.purple.opacity(0.1))
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

// MARK: - Extracted Text Section

struct ExtractedTextSection: View {
    let text: String
    let detectedSubject: HomeworkType
    let onSubjectChange: (HomeworkType) -> Void
    let onTextChange: (String) -> Void

    @State private var editableText: String

    init(
        text: String,
        detectedSubject: HomeworkType,
        onSubjectChange: @escaping (HomeworkType) -> Void,
        onTextChange: @escaping (String) -> Void
    ) {
        self.text = text
        self.detectedSubject = detectedSubject
        self.onSubjectChange = onSubjectChange
        self.onTextChange = onTextChange
        self._editableText = State(initialValue: text)
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            // Success indicator
            HStack {
                Image(systemName: "checkmark.circle.fill")
                    .foregroundStyle(.green)
                Text("Testo estratto")
                    .font(.headline)

                Spacer()

                // Auto-detected subject
                Label(detectedSubject.rawValue, systemImage: detectedSubject.icon)
                    .font(.caption)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(detectedSubject.color.opacity(0.2))
                    .clipShape(RoundedRectangle(cornerRadius: 6))
            }

            // Subject picker
            Picker("Materia", selection: Binding(
                get: { detectedSubject },
                set: { onSubjectChange($0) }
            )) {
                ForEach(HomeworkType.allCases) { subject in
                    Label(subject.rawValue, systemImage: subject.icon)
                        .tag(subject)
                }
            }
            .pickerStyle(.menu)

            // Editable text
            VStack(alignment: .leading, spacing: 4) {
                Text("Testo riconosciuto (modificabile):")
                    .font(.caption)
                    .foregroundStyle(.secondary)

                TextEditor(text: $editableText)
                    .font(.body)
                    .frame(minHeight: 120)
                    .padding(8)
                    .background(Color(nsColor: .textBackgroundColor))
                    .clipShape(RoundedRectangle(cornerRadius: 8))
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(Color.secondary.opacity(0.2), lineWidth: 1)
                    )
                    .onChange(of: editableText) { oldValue, newValue in
                        onTextChange(newValue)
                    }
            }
        }
    }
}

// MARK: - OCR Helper (Placeholder for Vision Framework)

/// Helper class for OCR text extraction
/// TODO: Implement using Vision framework's VNRecognizeTextRequest
class OCRHelper {
    static func extractText(from image: NSImage, completion: @escaping (Result<String, Error>) -> Void) {
        // Placeholder implementation
        // In production, use:
        /*
        import Vision

        guard let cgImage = image.cgImage(forProposedRect: nil, context: nil, hints: nil) else {
            completion(.failure(OCRError.invalidImage))
            return
        }

        let request = VNRecognizeTextRequest { request, error in
            if let error = error {
                completion(.failure(error))
                return
            }

            guard let observations = request.results as? [VNRecognizedTextObservation] else {
                completion(.failure(OCRError.noTextFound))
                return
            }

            let recognizedStrings = observations.compactMap { observation in
                observation.topCandidates(1).first?.string
            }

            let fullText = recognizedStrings.joined(separator: "\n")
            completion(.success(fullText))
        }

        request.recognitionLevel = .accurate
        request.recognitionLanguages = ["it-IT", "en-US"]

        let handler = VNImageRequestHandler(cgImage: cgImage, options: [:])
        DispatchQueue.global(qos: .userInitiated).async {
            do {
                try handler.perform([request])
            } catch {
                completion(.failure(error))
            }
        }
        */

        // Placeholder
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.5) {
            completion(.success("Placeholder text from OCR"))
        }
    }
}

enum OCRError: LocalizedError {
    case invalidImage
    case noTextFound

    var errorDescription: String? {
        switch self {
        case .invalidImage:
            return "Unable to process image"
        case .noTextFound:
            return "No text found in image"
        }
    }
}

// MARK: - Preview

#Preview("Photo Homework View") {
    PhotoHomeworkView(
        viewModel: HomeworkViewModel.preview,
        isPresented: .constant(true)
    )
}
