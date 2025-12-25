# ADR-001: HTML Generator - LLM-Generated vs Template-Based

**Date**: 2025-12-20
**Status**: Accepted
**Deciders**: Roberto, AI Team

## Context

I maestri dell'Education Pack devono poter creare visualizzazioni interattive per supportare le lezioni. La domanda è: come generare questo HTML?

### Opzioni considerate

1. **Template-based**: File HTML pre-costruiti con placeholder per contenuto
2. **LLM-generated**: I maestri chiedono all'LLM di generare HTML/CSS/JS al volo

## Decision

**Scelta: LLM-Generated HTML**

I maestri genereranno HTML tramite chiamate LLM, usando il wrapper `html_save_and_open()` per salvare e presentare il contenuto.

## Rationale

### Vantaggi LLM-Generated

1. **Flessibilità totale**: Ogni visualizzazione è personalizzata per il concetto specifico
2. **Adattamento studente**: L'LLM conosce il profilo accessibilità e può adattare
3. **Pedagogia intelligente**: Il maestro decide se e quando proporre una visualizzazione
4. **Meno codice**: Wrapper minimo (~280 LOC) vs template engine complesso (~1000+ LOC)
5. **Evolvibile**: Le capacità migliorano con i modelli LLM, non serve aggiornare codice

### Svantaggi considerati

1. **Dipendenza LLM**: Richiede connessione e token
   - Mitigazione: I maestri possono salvare HTML generati per riuso offline
2. **Consistenza**: Output potenzialmente variabile
   - Mitigazione: System prompt standardizzati per ogni maestro
3. **Costi**: Tokens per generare HTML
   - Mitigazione: HTML generati sono tipicamente 5-15KB, costo marginale

## Workflow

```
1. Euclide (maestro) analizza difficoltà studente
2. Euclide decide: "Una visualizzazione aiuterebbe"
3. Euclide genera prompt: "Crea HTML interattivo per teorema Pitagora con..."
4. LLM restituisce HTML completo
5. Euclide chiama: html_save_and_open(html, "teorema_pitagora")
6. Browser si apre con visualizzazione
7. (Opzionale) HTML salvato in ~/.convergio/education/lessons/ per riuso
```

## Implementation

```c
// API principale (maestri usano questa)
char* html_save_and_open(const char* html_content, const char* topic);

// API per salvare senza aprire
char* html_save(const char* html_content, const char* topic);

// API per aprire file esistente
int html_open_in_browser(const char* filepath);
```

## Consequences

### Positive

- Maestri possono creare qualsiasi tipo di visualizzazione
- Zero manutenzione template
- Contenuto sempre aggiornato alle capacità LLM

### Negative

- Richiede buoni prompt nei system message dei maestri
- Tempo di generazione HTML (~2-5 secondi)

## Alternatives Not Chosen

### Template-based (Scartato)

Avremmo avuto:
- TK87: Template geometry (Pitagora, cerchi, etc.)
- TK88: Template physics (pendolo, onde, etc.)
- TK91: Template timeline

Problemi:
- Ogni nuovo concetto richiede nuovo template
- Template rigidi, difficili da personalizzare
- Manutenzione codice significativa

## References

- File: `src/education/tools/html_generator.c`
- Header: `include/nous/education.h` (HtmlContentType enum)
- Directory output: `~/.convergio/education/lessons/`
