/**
 * CONVERGIO EDUCATION - ERROR INTERPRETER
 *
 * Transforms cryptic error messages into human-friendly, empathetic messages
 * that match each maestro's personality. Students never see stack traces.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/edition.h"
#include "nous/education.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// MAESTRO PERSONALITY RESPONSES
// ============================================================================

typedef struct {
    const char* agent_id;
    const char* name;
    const char* apology_style;
    const char* suggestion_style;
} MaestroPersonality;

static const MaestroPersonality MAESTRO_PERSONALITIES[] = {
    {"euclide-matematica", "Euclide",
     "Mi dispiace, ho avuto qualche difficoltà con questo calcolo.",
     "Proviamo un approccio diverso - posso spiegarti il concetto con un disegno più semplice?"},

    {"socrate-filosofia", "Socrate",
     "Hmm, sembra che il mio ragionamento si sia perso in un labirinto.",
     "Torniamo alla domanda fondamentale: cosa vuoi davvero capire?"},

    {"feynman-fisica", "Feynman",
     "Ops! Anche i fisici fanno errori - fa parte del metodo scientifico!",
     "Ricominciamo con un esempio più semplice, come farebbe mia nonna."},

    {"darwin-scienze", "Darwin",
     "Come in natura, a volte serve adattarsi. Ho incontrato un ostacolo.",
     "Evolviamo il nostro approccio - proviamo un percorso alternativo?"},

    {"humboldt-geografia", "Humboldt",
     "Mi sono perso durante questa esplorazione, ma ogni viaggiatore sa che succede.",
     "Ritroviamo la strada - quale aspetto della geografia ti interessa di più?"},

    {"manzoni-italiano", "Manzoni",
     "Anche i promessi sposi hanno affrontato ostacoli - eccone uno anche per noi.",
     "Riscriviamo questo capitolo insieme - da dove vuoi ricominciare?"},

    {"erodoto-storia", "Erodoto",
     "La storia ci insegna che i fallimenti sono maestri. Questo è uno di quelli.",
     "Come gli antichi, impariamo e riproviamo - cosa vuoi approfondire?"},

    {"leonardo-arte", "Leonardo",
     "Anche i miei progetti a volte non funzionavano al primo tentativo!",
     "L'arte richiede pazienza - riproviamo con una nuova prospettiva?"},

    {"mozart-musica", "Mozart",
     "Sembra che questa melodia abbia stonato - succede anche ai migliori!",
     "Riaccordiamo e riprendiamo - quale aspetto musicale vuoi esplorare?"},

    {"shakespeare-inglese", "Shakespeare",
     "To err is human - anche i miei drammi avevano qualche problema!",
     "Let's try a different scene - what would you like to learn?"},

    {"cicerone-civica", "Cicerone", "Anche nel Senato romano le cose non sempre andavano lisce.",
     "Come un buon cittadino, perseveriamo - quale tema civico ti interessa?"},

    {"smith-economia", "Smith",
     "I mercati sono imprevedibili, e così questo processo. Ma impariamo.",
     "Investiamo in un nuovo approccio - cosa vuoi capire dell'economia?"},

    {"lovelace-informatica", "Lovelace",
     "Anche il primo programma della storia aveva dei bug! Questo è simile.",
     "Debugghiamo insieme - quale concetto di informatica vuoi esplorare?"},

    {"ippocrate-corpo", "Ippocrate", "Come in medicina, a volte serve una seconda diagnosi.",
     "Primum non nocere - riproviamo con calma. Cosa vuoi sapere sul corpo?"},

    {"chris-storytelling", "Chris",
     "Ogni grande storia ha i suoi momenti difficili - questo è uno.",
     "Let me tell you a different story - what topic interests you?"},

    {"ali-principal", "Ali", "Mi dispiace, qualcosa non ha funzionato come previsto.",
     "Non ti preoccupare, troveremo insieme la soluzione. Come posso aiutarti?"},

    {NULL, NULL, NULL, NULL}};

// ============================================================================
// ERROR PATTERNS
// ============================================================================

typedef struct {
    const char* pattern;
    const char* friendly_message;
    const char* suggestion;
} ErrorPattern;

static const ErrorPattern ERROR_PATTERNS[] = {
    {"Too many tool iterations",
     "Ho provato diverse strade ma non sono riuscito a completare il compito.",
     "Proviamo a semplificare - dimmi cosa vuoi in modo più specifico."},

    {"exceeded maximum iterations", "Ho fatto molti tentativi ma mi sono bloccato.",
     "Posso provare in modo diverso se mi dai indicazioni più precise."},

    {"API call failed", "C'è stato un problema di connessione.",
     "Controlla la connessione internet e riprova tra qualche secondo."},

    {"Memory allocation failed", "Il sistema è un po' affaticato.",
     "Prova a chiudere altre applicazioni e riprova."},

    {"Tool execution failed", "Non sono riuscito a usare uno strumento.",
     "Dimmi cosa vuoi fare e troverò un altro modo."},

    {"response was empty", "Non ho ricevuto risposta.",
     "Riprova - a volte serve un secondo tentativo."},

    {"Provider not configured", "Il sistema non è ancora configurato completamente.",
     "Chiedi a un adulto di eseguire /setup per configurare tutto."},

    {"parse tool call", "Ho avuto difficoltà a capire come procedere.",
     "Prova a riformulare la tua richiesta in modo più semplice."},

    {NULL, NULL, NULL}};

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Transform a technical error message into a friendly, empathetic message.
 * Returns a newly allocated string that must be freed by the caller.
 */
char* education_interpret_error(const char* error_msg, const char* agent_id) {
    if (!error_msg)
        return NULL;

    // Only apply in education edition
    if (edition_current() != EDITION_EDUCATION) {
        return strdup(error_msg); // Return original in other editions
    }

    // Find matching personality
    const MaestroPersonality* personality = NULL;
    if (agent_id) {
        for (int i = 0; MAESTRO_PERSONALITIES[i].agent_id != NULL; i++) {
            if (strcmp(MAESTRO_PERSONALITIES[i].agent_id, agent_id) == 0) {
                personality = &MAESTRO_PERSONALITIES[i];
                break;
            }
        }
    }

    // Default to Ali if no personality found
    if (!personality) {
        for (int i = 0; MAESTRO_PERSONALITIES[i].agent_id != NULL; i++) {
            if (strcmp(MAESTRO_PERSONALITIES[i].agent_id, "ali-principal") == 0) {
                personality = &MAESTRO_PERSONALITIES[i];
                break;
            }
        }
    }

    // Find matching error pattern
    const ErrorPattern* pattern = NULL;
    for (int i = 0; ERROR_PATTERNS[i].pattern != NULL; i++) {
        if (strstr(error_msg, ERROR_PATTERNS[i].pattern) != NULL) {
            pattern = &ERROR_PATTERNS[i];
            break;
        }
    }

    // Build friendly message
    size_t msg_size = 1024;
    char* friendly = malloc(msg_size);
    if (!friendly)
        return strdup(error_msg);

    if (personality && pattern) {
        snprintf(friendly, msg_size, "%s: %s\n\n%s\n\n💡 %s", personality->name,
                 personality->apology_style, pattern->friendly_message, pattern->suggestion);
    } else if (personality) {
        snprintf(friendly, msg_size, "%s: %s\n\n%s", personality->name, personality->apology_style,
                 personality->suggestion_style);
    } else if (pattern) {
        snprintf(friendly, msg_size, "%s\n\n💡 %s", pattern->friendly_message, pattern->suggestion);
    } else {
        // Generic friendly error
        snprintf(friendly, msg_size,
                 "Oops! Qualcosa non ha funzionato.\n\n"
                 "💡 Prova a riformulare la tua richiesta o riprova tra qualche secondo.");
    }

    return friendly;
}

/**
 * Check if an error message should be interpreted (education edition only)
 */
bool education_should_interpret_error(const char* error_msg) {
    if (!error_msg)
        return false;
    if (edition_current() != EDITION_EDUCATION)
        return false;

    // Check if it starts with "Error:" or contains known patterns
    if (strncmp(error_msg, "Error:", 6) == 0)
        return true;

    for (int i = 0; ERROR_PATTERNS[i].pattern != NULL; i++) {
        if (strstr(error_msg, ERROR_PATTERNS[i].pattern) != NULL) {
            return true;
        }
    }

    return false;
}
