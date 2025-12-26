# Piano: Footer Fisso per ConvergioCLI

## Obiettivo
Implementare un footer che rimane SEMPRE fisso in fondo al terminale, mai coperto dall'output.

## Approccio Scelto: Margin-Based (NO DECSTBM)

**Perche' NON usare scroll regions (DECSTBM):**
- Lascia cursore in posizione undefined
- Readline non sa della scroll region e calcola spazio sbagliato
- Era il problema della vecchia implementazione

**Approccio alternativo:**
1. Creare un output wrapper che intercetta tutti gli stdout
2. Tracciare posizione cursore manualmente
3. Quando output sta per entrare nella zona footer, scrollare su
4. Usare posizionamento assoluto `\033[row;1H` per il footer
5. Dire a readline lo screen size ridotto

---

## File da Modificare

| File | Modifica |
|------|----------|
| `src/ui/statusbar.c` | Riscrivere senza DECSTBM, solo posizionamento assoluto |
| `src/core/main.c` | Init footer, configurare `rl_set_screen_size()` |
| `src/core/stream_md.c` | Sostituire `emit()`/`emit_char()` con footer-aware |
| `src/core/ansi_md.c` | Sostituire printf in `md_print()` |
| `src/ui/terminal.c` | SIGWINCH handler deve riconfigurare readline |
| `include/nous/statusbar.h` | Aggiornare API |

**Nuovi file:**
- `src/ui/footer_output.c` - Layer di output wrapper
- `include/nous/footer_output.h` - Header
- `src/core/version_check.c` - Check GitHub per nuove versioni
- `include/nous/version_check.h` - Header

---

## Implementazione Step-by-Step

### Step 1: Footer Output Layer

Creare `src/ui/footer_output.c`:

```c
typedef struct {
    int cursor_row;       // Riga corrente (1-based)
    int cursor_col;       // Colonna corrente
    int term_height;      // Altezza terminale
    int term_width;       // Larghezza terminale
    int footer_lines;     // Righe footer (2)
    bool footer_visible;  // Footer attivo?
} FooterState;

// Funzioni principali
void footer_output_init(void);
void footer_write(const char* str);     // Sostituisce fputs
void footer_write_char(char c);         // Sostituisce putchar
void footer_scroll_up(int lines);       // Scrolla quando serve
void footer_render(void);               // Renderizza footer
```

**Logica `footer_write()`:**
- Per ogni carattere, traccia posizione
- Se newline e cursor_row >= (height - footer_lines), chiama scroll_up
- Altrimenti scrivi normalmente

**Logica `footer_scroll_up()`:**
1. Salva cursore `\033[s`
2. Muovi a ultima riga contenuto
3. Scrivi newline (terminale scrolla naturalmente)
4. Ri-renderizza footer
5. Ripristina cursore `\033[u` (aggiustato)

### Step 2: Riscrivere statusbar.c

Rimuovere tutto il codice DECSTBM. Usare solo:

```c
void footer_render(void) {
    printf("\033[s");           // Salva cursore
    printf("\033[?25l");        // Nascondi cursore

    // Riga 1: Separatore
    printf("\033[%d;1H\033[2K", height - 1);
    printf("\033[2m");
    for (int i = 0; i < width; i++) printf("-");
    printf("\033[0m");

    // Riga 2: Contenuto
    printf("\033[%d;1H\033[2K", height);
    printf(" %s | %s | v%s%s",
           folder, agents, version, update_notice);

    printf("\033[?25h");        // Mostra cursore
    printf("\033[u");           // Ripristina cursore
    fflush(stdout);
}
```

### Step 3: Integrare con Readline

In `main.c` prima del REPL loop:

```c
footer_output_init();
footer_set_visible(true);

// Dire a readline lo spazio ridotto
int h, w;
footer_get_dimensions(&w, &h);
rl_set_screen_size(h - 2, w);  // Meno le 2 righe footer
```

Modificare SIGWINCH handler in `terminal.c`:

```c
static void sigwinch_handler(int sig) {
    update_size();
    footer_handle_resize();
    rl_set_screen_size(g_terminal.height - 2, g_terminal.width);
    rl_resize_terminal();
}
```

### Step 4: Connettere Streaming

In `stream_md.c`, cambiare:

```c
// Prima:
static void emit(const char* text) {
    fputs(text, stdout);
    fflush(stdout);
}

// Dopo:
static void emit(const char* text) {
    footer_write(text);
}
```

### Step 5: Connettere Output Batch

In `ansi_md.c`, cambiare `md_print()`:

```c
void md_print(const char* markdown) {
    char* formatted = md_to_ansi(markdown);
    footer_write(formatted);  // Non printf
    free(formatted);
}
```

---

## Contenuto Footer

Layout con emoji:
```
────────────────────────────────────────────────────
 📁 ConvergioCLI | 🤖 Ali, Dev | v4.0.0 | ⬆️ v4.0.1 available
```

Componenti:
- 📁 Folder corrente (basename)
- 🤖 Agenti attivi (virgola-separati)
- Versione corrente
- ⬆️ Notifica update (se disponibile)

---

## Check Nuove Versioni

All'avvio, chiamare GitHub API per verificare ultima release:

```c
// In src/core/version_check.c (nuovo file)
char* check_latest_version(void) {
    // GET https://api.github.com/repos/OWNER/REPO/releases/latest
    // Parse JSON per tag_name
    // Confronta con CONVERGIO_VERSION
    // Return version string se nuova, NULL altrimenti
}
```

**Note:**
- Timeout breve (2 sec) per non bloccare avvio
- Cache risultato per sessione
- Non-blocking (thread separato o async)

---

## Comando /footer

Aggiungere comando per gestire footer:

```
/footer         - Mostra stato footer
/footer on      - Attiva footer
/footer off     - Disattiva footer
```

Implementare in `src/core/commands/commands.c`

---

## Ordine di Implementazione

1. **Fase 1** - footer_output.c (testare isolato)
2. **Fase 2** - Riscrivere statusbar.c
3. **Fase 3** - Integrare con main.c e readline
4. **Fase 4** - Connettere stream_md.c e ansi_md.c
5. **Fase 5** - version_check.c (check GitHub API)
6. **Fase 6** - Comando /footer in commands.c
7. **Fase 7** - Test e fix edge cases

---

## Rischi e Mitigazioni

| Rischio | Mitigazione |
|---------|-------------|
| Performance overhead | Buffer batching, check solo su newline |
| Readline sbaglia ancora | `rl_set_screen_size()` + `rl_resize_terminal()` |
| Flickering | Nascondere cursore durante render |
| Tracking cursore va fuori sync | Reset su eventi noti (newline, clear) |

---

## Test

1. Footer visibile all'avvio
2. Output lungo NON copre footer
3. Streaming NON copre footer
4. Resize terminale OK
5. Prompt readline nella posizione giusta
6. Funziona in iTerm2, Terminal.app
