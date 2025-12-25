/**
 * CONVERGIO NATIVE - Safety Filter Service
 *
 * P0 CRITICAL: Child safety system for ages 6-19
 * Detects harmful content patterns in Italian AND English
 * with appropriate responses (block, redirect, or warn)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

import Foundation

// MARK: - Safety Category

public enum SafetyCategory: String, Codable, CaseIterable {
    case selfHarm = "self_harm"
    case violence = "violence"
    case drugs = "drugs"
    case adult = "adult"
    case bullying = "bullying"
    case jailbreak = "jailbreak"
    case cheating = "cheating"
    case privacy = "privacy"
    case isolation = "isolation"

    public var description: String {
        switch self {
        case .selfHarm: return "Self-harm concern"
        case .violence: return "Violence"
        case .drugs: return "Drugs/Substances"
        case .adult: return "Adult content"
        case .bullying: return "Bullying"
        case .jailbreak: return "Jailbreak attempt"
        case .cheating: return "Academic dishonesty"
        case .privacy: return "Privacy concern"
        case .isolation: return "Isolation/Loneliness"
        }
    }

    public var emoji: String {
        switch self {
        case .selfHarm: return "💔"
        case .violence: return "⚠️"
        case .drugs: return "🚫"
        case .adult: return "🔞"
        case .bullying: return "😢"
        case .jailbreak: return "🔒"
        case .cheating: return "📝"
        case .privacy: return "🔐"
        case .isolation: return "🤗"
        }
    }
}

// MARK: - Safety Response

public struct SafetyResponse: Codable {
    public let blocked: Bool
    public let category: SafetyCategory?
    public let redirectMessage: String?
    public let shouldLog: Bool

    public init(
        blocked: Bool,
        category: SafetyCategory? = nil,
        redirectMessage: String? = nil,
        shouldLog: Bool = true
    ) {
        self.blocked = blocked
        self.category = category
        self.redirectMessage = redirectMessage
        self.shouldLog = shouldLog
    }

    // Safe response (no issues detected)
    public static let safe = SafetyResponse(
        blocked: false,
        category: nil,
        redirectMessage: nil,
        shouldLog: false
    )
}

// MARK: - Safety Filter

@MainActor
public final class SafetyFilter: ObservableObject {
    public static let shared = SafetyFilter()

    // MARK: - Pattern Definitions

    // SAF01: Bad grade distress - REDIRECT to adult
    private let badGradePatterns = [
        // Italian
        "voglio morire", "voglio ammazzarmi", "mi ammazzo", "finirla qui",
        "non voglio più vivere", "voglio farla finita", "mi butto",
        "brutto voto", "voto brutto", "bocciato", "bocciata",
        "preso un.{0,5}[2345]", "ho preso.{0,5}insufficiente",
        // English
        "want to die", "gonna kill myself", "end it all", "don't want to live",
        "bad grade", "failed", "failing grade", "got an f"
    ]

    // SAF02: Self-harm - REDIRECT to adult
    private let selfHarmPatterns = [
        // Italian
        "farmi del male", "fare del male a me stess", "tagliarmi", "ferirmi",
        "autolesion", "suicid", "uccidermi", "togliermi la vita",
        "odio me stess", "vorrei scomparire", "vorrei non esistere",
        // English
        "hurt myself", "harm myself", "cut myself", "self harm", "self-harm",
        "suicide", "suicidal", "kill myself", "take my life",
        "hate myself", "wish i was dead", "wish i didn't exist"
    ]

    // SAF03: Violence - BLOCK completely
    private let violencePatterns = [
        // Italian
        "bomba", "esplosiv", "arma", "pistola", "fucile", "coltello",
        "uccidere", "ammazzare", "far del male a", "picchiare",
        "sparare", "accoltellare", "colpire", "aggredire",
        "vendetta", "vendicarsi", "fargliela pagare",
        // English
        "bomb", "explosive", "weapon", "gun", "rifle", "knife",
        "kill", "murder", "hurt someone", "beat up",
        "shoot", "stab", "hit", "attack", "assault",
        "revenge", "get back at", "make them pay"
    ]

    // SAF04: Drugs - BLOCK completely
    private let drugsPatterns = [
        // Italian
        "droga", "droghe", "cocaina", "eroina", "marijuana", "hashish",
        "fumare", "spinello", "canna", "sballo", "sballarsi",
        "pasticche", "pillole", "ecstasy", "mdma", "lsd",
        // English
        "drugs", "cocaine", "heroin", "marijuana", "weed", "pot",
        "smoke", "joint", "high", "get high",
        "pills", "ecstasy", "mdma", "lsd", "meth"
    ]

    // SAF05: Bullying - REDIRECT to adult
    private let bullyingPatterns = [
        // Italian
        "fare il bullo", "bullismo", "prendere in giro", "offendere",
        "minacciare", "intimidire", "umiliare", "deridere",
        "tutti mi odiano", "nessuno mi vuole", "sono solo",
        "mi prendono in giro", "mi insultano", "cyberbullismo",
        // English
        "bully", "bullying", "make fun of", "insult",
        "threaten", "intimidate", "humiliate", "mock",
        "everyone hates me", "nobody wants me", "alone",
        "making fun of me", "insulting me", "cyberbullying"
    ]

    // SAF06: Adult content - BLOCK completely
    private let adultPatterns = [
        // Italian
        "sesso", "porn", "porno", "xxx", "nudo", "nuda",
        "fare l'amore", "rapporto sessuale", "video per adulti",
        // English
        "sex", "porn", "porno", "xxx", "nude", "naked",
        "make love", "sexual", "adult video"
    ]

    // SAF07: Jailbreak - IGNORE and continue in role
    private let jailbreakPatterns = [
        // Italian
        "dimentica", "ignora", "non seguire", "disattiva",
        "comportati come", "fai finta di essere", "sei liberato",
        "non hai limiti", "puoi fare qualsiasi cosa",
        // English
        "forget", "ignore", "don't follow", "disable",
        "act as", "pretend to be", "you are free",
        "no limits", "you can do anything"
    ]

    // SAF08: Cheating - IGNORE and continue in role
    private let cheatingPatterns = [
        // Italian
        "fammi i compiti", "fai i compiti al posto mio", "dammi le risposte",
        "risolvimi tutto", "scrivi il tema per me", "fa il compito",
        // English
        "do my homework", "give me the answers", "solve everything",
        "write the essay for me", "do the assignment"
    ]

    // SAF09: Privacy - WARN about privacy
    private let privacyPatterns = [
        // Italian
        "il mio indirizzo è", "abito in via", "il mio numero è",
        "telefono:", "cellulare:", "whatsapp:", "email:",
        "password", "carta di credito", "codice fiscale",
        // English
        "my address is", "i live at", "my phone number is",
        "phone:", "mobile:", "whatsapp:", "email:",
        "password", "credit card", "social security"
    ]

    // SAF10: Isolation - REDIRECT to adult
    private let isolationPatterns = [
        // Italian
        "nessuno mi vuole bene", "sono solo", "sola", "nessuno mi capisce",
        "non ho amici", "mi sento solo", "mi sento sola",
        "non ho nessuno", "tutti mi hanno abbandonato",
        // English
        "nobody loves me", "i'm alone", "nobody understands me",
        "i have no friends", "feel alone", "feel lonely",
        "i have nobody", "everyone abandoned me"
    ]

    // MARK: - Initialization

    private init() {
        logInfo("SafetyFilter initialized with \(SafetyCategory.allCases.count) categories", category: "Safety")
    }

    // MARK: - Public API

    /// Analyzes text for safety concerns
    /// Returns SafetyResponse with appropriate action
    public func analyze(_ text: String) -> SafetyResponse {
        let normalized = text.lowercased()

        // Check in priority order (most critical first)

        // 1. Self-harm (SAF02) - REDIRECT
        if containsPattern(normalized, patterns: selfHarmPatterns) {
            logWarning("Self-harm pattern detected", category: "Safety")
            return SafetyResponse(
                blocked: true,
                category: .selfHarm,
                redirectMessage: "Mi sembra che tu stia affrontando un momento difficile. È molto importante che tu parli con un adulto di fiducia - un genitore, un insegnante, o uno psicologo scolastico. Non sei solo/a. 💙\n\nI notice you might be going through a difficult time. It's very important that you talk to a trusted adult - a parent, teacher, or school counselor. You're not alone. 💙"
            )
        }

        // 2. Bad grade distress (SAF01) - REDIRECT
        if containsPattern(normalized, patterns: badGradePatterns) {
            logWarning("Bad grade distress pattern detected", category: "Safety")
            return SafetyResponse(
                blocked: true,
                category: .selfHarm,
                redirectMessage: "Capisco che un brutto voto possa far stare male, ma non è la fine del mondo! Parliamone con un insegnante o con i tuoi genitori. Sono qui per aiutarti a migliorare. 📚\n\nI understand a bad grade can be upsetting, but it's not the end of the world! Let's talk to a teacher or your parents. I'm here to help you improve. 📚"
            )
        }

        // 3. Violence (SAF03) - BLOCK
        if containsPattern(normalized, patterns: violencePatterns) {
            logWarning("Violence pattern detected", category: "Safety")
            return SafetyResponse(
                blocked: true,
                category: .violence,
                redirectMessage: "Non posso aiutarti con questo argomento. Se stai vivendo una situazione di pericolo, per favore parla con un adulto di fiducia o chiama il 112.\n\nI can't help with this topic. If you're in a dangerous situation, please talk to a trusted adult or call emergency services."
            )
        }

        // 4. Drugs (SAF04) - BLOCK
        if containsPattern(normalized, patterns: drugsPatterns) {
            logWarning("Drugs pattern detected", category: "Safety")
            return SafetyResponse(
                blocked: true,
                category: .drugs,
                redirectMessage: "Non posso aiutarti con domande sulle sostanze stupefacenti. Se hai domande sulla salute, parlane con i tuoi genitori o un medico.\n\nI can't help with questions about drugs. If you have health questions, talk to your parents or a doctor."
            )
        }

        // 5. Adult content (SAF06) - BLOCK
        if containsPattern(normalized, patterns: adultPatterns) {
            logWarning("Adult content pattern detected", category: "Safety")
            return SafetyResponse(
                blocked: true,
                category: .adult,
                redirectMessage: "Questo non è un argomento adatto per questa app educativa. Se hai domande, parlane con i tuoi genitori o un insegnante.\n\nThis is not an appropriate topic for this educational app. If you have questions, talk to your parents or a teacher."
            )
        }

        // 6. Bullying (SAF05) - REDIRECT
        if containsPattern(normalized, patterns: bullyingPatterns) {
            logWarning("Bullying pattern detected", category: "Safety")
            return SafetyResponse(
                blocked: true,
                category: .bullying,
                redirectMessage: "Mi dispiace che tu stia vivendo questa situazione. Il bullismo è una cosa seria e non devi affrontarlo da solo/a. Parla con un genitore, insegnante o uno psicologo scolastico. 🤗\n\nI'm sorry you're experiencing this. Bullying is serious and you shouldn't face it alone. Talk to a parent, teacher, or school counselor. 🤗"
            )
        }

        // 7. Isolation (SAF10) - REDIRECT
        if containsPattern(normalized, patterns: isolationPatterns) {
            logWarning("Isolation pattern detected", category: "Safety")
            return SafetyResponse(
                blocked: true,
                category: .isolation,
                redirectMessage: "Mi dispiace che ti senta così. Sentirsi soli può essere davvero difficile. Per favore, parla con qualcuno che ti vuole bene - un genitore, un insegnante, o un amico. Non sei davvero solo/a. 💙\n\nI'm sorry you feel this way. Feeling lonely can be really hard. Please talk to someone who cares about you - a parent, teacher, or friend. You're not really alone. 💙"
            )
        }

        // 8. Privacy (SAF09) - WARN
        if containsPattern(normalized, patterns: privacyPatterns) {
            logWarning("Privacy concern detected", category: "Safety")
            return SafetyResponse(
                blocked: true,
                category: .privacy,
                redirectMessage: "⚠️ ATTENZIONE: Non condividere mai informazioni personali come indirizzi, numeri di telefono, password o dati della carta di credito online!\n\n⚠️ WARNING: Never share personal information like addresses, phone numbers, passwords, or credit card details online!"
            )
        }

        // 9. Jailbreak (SAF07) - IGNORE (return safe, let AI continue in role)
        if containsPattern(normalized, patterns: jailbreakPatterns) {
            logInfo("Jailbreak attempt detected (ignored)", category: "Safety")
            return .safe
        }

        // 10. Cheating (SAF08) - IGNORE (return safe, let AI explain why it won't)
        if containsPattern(normalized, patterns: cheatingPatterns) {
            logInfo("Cheating attempt detected (ignored)", category: "Safety")
            return .safe
        }

        // No safety concerns detected
        return .safe
    }

    // MARK: - Helper Methods

    private func containsPattern(_ text: String, patterns: [String]) -> Bool {
        for pattern in patterns {
            do {
                let regex = try NSRegularExpression(pattern: pattern, options: [.caseInsensitive])
                let range = NSRange(text.startIndex..., in: text)
                if regex.firstMatch(in: text, range: range) != nil {
                    return true
                }
            } catch {
                // Fallback to simple contains check if regex fails
                if text.contains(pattern.lowercased()) {
                    return true
                }
            }
        }
        return false
    }

    // MARK: - Statistics

    public func getPatternCount() -> Int {
        return badGradePatterns.count +
               selfHarmPatterns.count +
               violencePatterns.count +
               drugsPatterns.count +
               bullyingPatterns.count +
               adultPatterns.count +
               jailbreakPatterns.count +
               cheatingPatterns.count +
               privacyPatterns.count +
               isolationPatterns.count
    }
}
