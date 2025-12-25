/**
 * CONVERGIO NATIVE - Student Profile Model
 *
 * Defines student profile data structure for educational onboarding.
 * Supports Italian school system with comprehensive curriculum types.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation

// MARK: - Student Profile

struct StudentProfile: Codable, Equatable {
    var firstName: String
    var age: Int
    var schoolYear: SchoolYear
    var curriculum: Curriculum
    var accessibilitySettings: StudentAccessibilitySettings
    var createdAt: Date

    init(
        firstName: String = "",
        age: Int = 14,
        schoolYear: SchoolYear = .scuolaMedia(.seconda),
        curriculum: Curriculum = .scuolaMedia,
        accessibilitySettings: StudentAccessibilitySettings = StudentAccessibilitySettings(),
        createdAt: Date = Date()
    ) {
        self.firstName = firstName
        self.age = age
        self.schoolYear = schoolYear
        self.curriculum = curriculum
        self.accessibilitySettings = accessibilitySettings
        self.createdAt = createdAt
    }

    var isComplete: Bool {
        !firstName.isEmpty && age >= 6 && age <= 19
    }
}

// MARK: - Curriculum Types

enum Curriculum: String, Codable, CaseIterable {
    case liceoClassico = "liceo_classico"
    case liceoScientifico = "liceo_scientifico"
    case liceoLinguistico = "liceo_linguistico"
    case liceoArtistico = "liceo_artistico"
    case itis = "itis"
    case ite = "ite"
    case ipsia = "ipsia"
    case scuolaMedia = "scuola_media"
    case elementare = "elementare"

    var displayName: String {
        switch self {
        case .liceoClassico: return "Liceo Classico"
        case .liceoScientifico: return "Liceo Scientifico"
        case .liceoLinguistico: return "Liceo Linguistico"
        case .liceoArtistico: return "Liceo Artistico"
        case .itis: return "ITIS (Tecnico Industriale)"
        case .ite: return "ITE (Tecnico Economico)"
        case .ipsia: return "IPSIA (Professionale)"
        case .scuolaMedia: return "Scuola Media"
        case .elementare: return "Scuola Elementare"
        }
    }

    var description: String {
        switch self {
        case .liceoClassico: return "Latino, Greco, Filosofia"
        case .liceoScientifico: return "Matematica, Fisica, Scienze"
        case .liceoLinguistico: return "3 Lingue Straniere"
        case .liceoArtistico: return "Arte, Design, Espressione"
        case .itis: return "Tecnologia e Industria"
        case .ite: return "Economia e Commercio"
        case .ipsia: return "Formazione Professionale"
        case .scuolaMedia: return "Scuola Secondaria di I Grado"
        case .elementare: return "Scuola Primaria"
        }
    }

    var icon: String {
        switch self {
        case .liceoClassico: return "book.closed.fill"
        case .liceoScientifico: return "atom"
        case .liceoLinguistico: return "globe.europe.africa.fill"
        case .liceoArtistico: return "paintbrush.fill"
        case .itis: return "gearshape.2.fill"
        case .ite: return "chart.line.uptrend.xyaxis"
        case .ipsia: return "wrench.and.screwdriver.fill"
        case .scuolaMedia: return "backpack.fill"
        case .elementare: return "graduationcap.fill"
        }
    }

    var color: String {
        switch self {
        case .liceoClassico: return "purple"
        case .liceoScientifico: return "blue"
        case .liceoLinguistico: return "green"
        case .liceoArtistico: return "orange"
        case .itis: return "red"
        case .ite: return "indigo"
        case .ipsia: return "brown"
        case .scuolaMedia: return "cyan"
        case .elementare: return "pink"
        }
    }

    var availableSchoolYears: [SchoolYear] {
        switch self {
        case .liceoClassico, .liceoScientifico, .liceoLinguistico, .liceoArtistico, .itis, .ite, .ipsia:
            return [.superiori(.prima), .superiori(.seconda), .superiori(.terza), .superiori(.quarta), .superiori(.quinta)]
        case .scuolaMedia:
            return [.scuolaMedia(.prima), .scuolaMedia(.seconda), .scuolaMedia(.terza)]
        case .elementare:
            return [.elementare(.prima), .elementare(.seconda), .elementare(.terza), .elementare(.quarta), .elementare(.quinta)]
        }
    }
}

// MARK: - School Year Levels (NOT to be confused with Grade which is academic marks)

enum SchoolYear: Codable, Equatable, Hashable {
    case elementare(ElementareYear)
    case scuolaMedia(ScuolaMediaYear)
    case superiori(SuperioriYear)

    enum ElementareYear: String, Codable, CaseIterable {
        case prima, seconda, terza, quarta, quinta

        var displayName: String {
            switch self {
            case .prima: return "Prima Elementare"
            case .seconda: return "Seconda Elementare"
            case .terza: return "Terza Elementare"
            case .quarta: return "Quarta Elementare"
            case .quinta: return "Quinta Elementare"
            }
        }
    }

    enum ScuolaMediaYear: String, Codable, CaseIterable {
        case prima, seconda, terza

        var displayName: String {
            switch self {
            case .prima: return "Prima Media"
            case .seconda: return "Seconda Media"
            case .terza: return "Terza Media"
            }
        }
    }

    enum SuperioriYear: String, Codable, CaseIterable {
        case prima, seconda, terza, quarta, quinta

        var displayName: String {
            switch self {
            case .prima: return "Prima Superiore"
            case .seconda: return "Seconda Superiore"
            case .terza: return "Terza Superiore"
            case .quarta: return "Quarta Superiore"
            case .quinta: return "Quinta Superiore"
            }
        }
    }

    var displayName: String {
        switch self {
        case .elementare(let year): return year.displayName
        case .scuolaMedia(let year): return year.displayName
        case .superiori(let year): return year.displayName
        }
    }
}

// MARK: - Student Accessibility Settings (distinct from AccessibilityManager.AccessibilitySettings)

struct StudentAccessibilitySettings: Codable, Equatable {
    var fontSize: StudentFontSize
    var highContrast: Bool
    var voiceEnabled: Bool
    var simplifiedLanguage: Bool
    var dyslexiaFont: Bool

    init(
        fontSize: StudentFontSize = .medium,
        highContrast: Bool = false,
        voiceEnabled: Bool = false,
        simplifiedLanguage: Bool = false,
        dyslexiaFont: Bool = false
    ) {
        self.fontSize = fontSize
        self.highContrast = highContrast
        self.voiceEnabled = voiceEnabled
        self.simplifiedLanguage = simplifiedLanguage
        self.dyslexiaFont = dyslexiaFont
    }

    enum StudentFontSize: String, Codable, CaseIterable {
        case small = "small"
        case medium = "medium"
        case large = "large"
        case extraLarge = "extra_large"

        var displayName: String {
            switch self {
            case .small: return "Piccolo"
            case .medium: return "Medio"
            case .large: return "Grande"
            case .extraLarge: return "Molto Grande"
            }
        }

        var scale: CGFloat {
            switch self {
            case .small: return 0.9
            case .medium: return 1.0
            case .large: return 1.2
            case .extraLarge: return 1.4
            }
        }
    }
}
