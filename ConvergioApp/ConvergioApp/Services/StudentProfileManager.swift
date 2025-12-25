/**
 * CONVERGIO NATIVE - Student Profile Manager
 *
 * Manages student profile persistence and validation.
 * Handles first-run detection and profile lifecycle.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation
import Combine

// MARK: - Student Profile Manager

class StudentProfileManager: ObservableObject {
    static let shared = StudentProfileManager()

    @Published var currentProfile: StudentProfile?
    @Published var isFirstRun: Bool = true

    private let userDefaults = UserDefaults.standard
    private let profileKey = "studentProfile"
    private let firstRunKey = "educationOnboardingComplete"

    private init() {
        loadProfile()
        checkFirstRun()
    }

    // MARK: - Profile Management

    func loadProfile() {
        if let data = userDefaults.data(forKey: profileKey),
           let profile = try? JSONDecoder().decode(StudentProfile.self, from: data) {
            currentProfile = profile
        }
    }

    func saveProfile(_ profile: StudentProfile) {
        if let encoded = try? JSONEncoder().encode(profile) {
            userDefaults.set(encoded, forKey: profileKey)
            currentProfile = profile
        }
    }

    func updateProfile(_ updates: (inout StudentProfile) -> Void) {
        guard var profile = currentProfile else { return }
        updates(&profile)
        saveProfile(profile)
    }

    func deleteProfile() {
        userDefaults.removeObject(forKey: profileKey)
        currentProfile = nil
    }

    // MARK: - First Run Detection

    func checkFirstRun() {
        isFirstRun = !userDefaults.bool(forKey: firstRunKey)
    }

    func completeOnboarding() {
        userDefaults.set(true, forKey: firstRunKey)
        isFirstRun = false
    }

    // MARK: - Validation

    func validateProfile(_ profile: StudentProfile) -> ValidationResult {
        var errors: [String] = []

        // Validate first name
        if profile.firstName.isEmpty {
            errors.append("Il nome è obbligatorio")
        } else if profile.firstName.count < 2 {
            errors.append("Il nome deve contenere almeno 2 caratteri")
        } else if profile.firstName.count > 50 {
            errors.append("Il nome è troppo lungo")
        }

        // Validate age
        if profile.age < 6 || profile.age > 19 {
            errors.append("L'età deve essere tra 6 e 19 anni")
        }

        // Validate age-curriculum consistency
        switch profile.curriculum {
        case .elementare:
            if profile.age < 6 || profile.age > 11 {
                errors.append("L'età non corrisponde alla scuola elementare (6-11 anni)")
            }
        case .scuolaMedia:
            if profile.age < 11 || profile.age > 14 {
                errors.append("L'età non corrisponde alla scuola media (11-14 anni)")
            }
        case .liceoClassico, .liceoScientifico, .liceoLinguistico, .liceoArtistico, .itis, .ite, .ipsia:
            if profile.age < 14 || profile.age > 19 {
                errors.append("L'età non corrisponde alla scuola superiore (14-19 anni)")
            }
        }

        return ValidationResult(isValid: errors.isEmpty, errors: errors)
    }

    func suggestedCurriculum(for age: Int) -> Curriculum {
        switch age {
        case 6...10:
            return .elementare
        case 11...13:
            return .scuolaMedia
        case 14...19:
            return .liceoScientifico
        default:
            return .scuolaMedia
        }
    }

    func suggestedSchoolYear(for age: Int, curriculum: Curriculum) -> SchoolYear {
        switch curriculum {
        case .elementare:
            let yearIndex = min(max(age - 6, 0), 4)
            let years = SchoolYear.ElementareYear.allCases
            return .elementare(years[yearIndex])

        case .scuolaMedia:
            let yearIndex = min(max(age - 11, 0), 2)
            let years = SchoolYear.ScuolaMediaYear.allCases
            return .scuolaMedia(years[yearIndex])

        default:
            let yearIndex = min(max(age - 14, 0), 4)
            let years = SchoolYear.SuperioriYear.allCases
            return .superiori(years[yearIndex])
        }
    }

    // MARK: - Export/Import

    func exportProfile() -> Data? {
        guard let profile = currentProfile else { return nil }
        return try? JSONEncoder().encode(profile)
    }

    func importProfile(from data: Data) -> Bool {
        guard let profile = try? JSONDecoder().decode(StudentProfile.self, from: data) else {
            return false
        }

        let validation = validateProfile(profile)
        if validation.isValid {
            saveProfile(profile)
            return true
        }

        return false
    }
}

// MARK: - Validation Result

struct ValidationResult {
    let isValid: Bool
    let errors: [String]

    var errorMessage: String {
        errors.joined(separator: "\n")
    }
}
