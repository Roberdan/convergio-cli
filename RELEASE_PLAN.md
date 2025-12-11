# Convergio CLI - Piano di Release v1.0.0

## Obiettivo
Trasformare Convergio in un CLI professionale distribuibile via Homebrew, con auto-update, versioning semantico, e ottimizzazione automatica per tutti i chip Apple Silicon.

## Architettura Repository
**UN SOLO REPO:** `https://github.com/Roberdan/convergio-cli`

```
convergio-cli/
├── Formula/
│   └── convergio.rb          # Homebrew formula (dentro lo stesso repo)
├── .github/
│   └── workflows/
│       ├── ci.yml            # Build su ogni PR
│       └── release.yml       # Build + release su tag
├── VERSION                   # Versione semantica
├── include/nous/
│   ├── nous.h
│   ├── hardware.h            # NUOVO
│   ├── config.h              # NUOVO
│   └── updater.h             # NUOVO
├── src/
│   ├── core/
│   │   ├── main.c
│   │   ├── fabric.c
│   │   ├── hardware.m        # NUOVO
│   │   ├── config.c          # NUOVO
│   │   └── updater.c         # NUOVO
│   └── auth/
│       └── keychain.m        # NUOVO (rinominare oauth.m?)
└── ...
```

**Installazione utente:**
```bash
brew tap Roberdan/convergio-cli
brew install convergio
```

---

## Esecuzione Parallelizzata

### BLOCCO A: Può partire SUBITO (nessuna dipendenza)

| ID | Task | File | Dipendenze |
|----|------|------|------------|
| A1 | Creare `VERSION` | `VERSION` | - |
| A2 | Creare header hardware | `include/nous/hardware.h` | - |
| A3 | Creare header config | `include/nous/config.h` | - |
| A4 | Creare header updater | `include/nous/updater.h` | - |
| A5 | Creare workflow CI | `.github/workflows/ci.yml` | - |
| A6 | Creare workflow Release | `.github/workflows/release.yml` | - |
| A7 | Creare Formula Homebrew | `Formula/convergio.rb` | - |
| A8 | Verificare `.gitignore` | `.gitignore` | - |

**Tutti questi 8 task possono essere eseguiti IN PARALLELO.**

---

### BLOCCO B: Dipende da A2 (hardware.h)

| ID | Task | File | Dipendenze |
|----|------|------|------------|
| B1 | Implementare hardware detection | `src/core/hardware.m` | A2 |
| B2 | Aggiornare nous.h (rimuovere M3 hardcoded) | `include/nous/nous.h` | A2 |

**B1 e B2 possono essere eseguiti IN PARALLELO tra loro.**

---

### BLOCCO C: Dipende da A3 (config.h)

| ID | Task | File | Dipendenze |
|----|------|------|------------|
| C1 | Implementare parser TOML | `src/core/config.c` | A3 |
| C2 | Implementare Keychain | `src/auth/keychain.m` | A3 |

**C1 e C2 possono essere eseguiti IN PARALLELO tra loro.**

---

### BLOCCO D: Dipende da A4 (updater.h)

| ID | Task | File | Dipendenze |
|----|------|------|------------|
| D1 | Implementare updater | `src/core/updater.c` | A4 |

---

### BLOCCO E: Dipende da B1, B2 (hardware completo)

| ID | Task | File | Dipendenze |
|----|------|------|------------|
| E1 | Aggiornare fabric.c | `src/core/fabric.c` | B1, B2 |
| E2 | Aggiornare gpu.m | `src/metal/gpu.m` | B1, B2 |
| E3 | Aggiornare scheduler.c | `src/runtime/scheduler.c` | B1, B2 |
| E4 | Aggiornare mlx_embed.m | `src/neural/mlx_embed.m` | B1, B2 |
| E5 | Aggiornare Makefile | `Makefile` | B1, A1 |

**E1, E2, E3, E4, E5 possono essere eseguiti IN PARALLELO tra loro.**

---

### BLOCCO F: Dipende da TUTTO il codice (E*, C*, D*)

| ID | Task | File | Dipendenze |
|----|------|------|------------|
| F1 | Aggiornare main.c (version, setup, update, hardware) | `src/core/main.c` | E*, C*, D1 |

---

### BLOCCO G: Dipende da F1 (integrazione completa)

| ID | Task | File | Dipendenze |
|----|------|------|------------|
| G1 | Build test completo | - | F1 |
| G2 | Aggiornare README.md | `README.md` | F1 |
| G3 | Aggiornare CHANGELOG.md | `CHANGELOG.md` | F1 |
| G4 | Aggiornare docs/adr/ | `docs/adr/*.md` | F1 |

**G1, G2, G3, G4 possono essere eseguiti IN PARALLELO tra loro.**

---

### BLOCCO H: Release (dipende da G*)

| ID | Task | Dipendenze |
|----|------|------------|
| H1 | Configurare remote GitHub | G* |
| H2 | Push iniziale | H1 |
| H3 | Verificare CI passa | H2 |
| H4 | Creare tag v1.0.0 | H3 |
| H5 | Verificare release automatica | H4 |
| H6 | Aggiornare SHA256 in Formula | H5 |
| H7 | Test `brew install` | H6 |

**Questi sono SEQUENZIALI per necessità.**

---

## Diagramma Dipendenze (DAG)

```
    ┌─────────────────────────────────────────────────────────────────┐
    │                         BLOCCO A (PARALLELO)                     │
    │  A1(VERSION) A2(hw.h) A3(cfg.h) A4(upd.h) A5(ci) A6(rel) A7 A8  │
    └─────────────────────────────────────────────────────────────────┘
           │           │           │           │
           │     ┌─────┴─────┐     │           │
           │     │           │     │           │
           │     ▼           ▼     ▼           ▼
           │  ┌─────┐     ┌─────┐ ┌─────┐   ┌─────┐
           │  │ B1  │     │ B2  │ │ C1  │   │ D1  │
           │  │hw.m │     │nous │ │cfg.c│   │upd.c│
           │  └──┬──┘     └──┬──┘ └──┬──┘   └──┬──┘
           │     │           │      │          │
           │     └─────┬─────┘      │          │
           │           │            │          │
           │           ▼            │          │
           │  ┌─────────────────────┴──────────┘
           │  │    BLOCCO E (PARALLELO)        │
           │  │  E1(fabric) E2(gpu) E3(sched)  │
           │  │  E4(mlx)                       │
           │  └────────────┬───────────────────┘
           │               │
           └───────┬───────┘
                   │
                   ▼
              ┌─────────┐
              │   E5    │ Makefile (dipende da A1 + B*)
              └────┬────┘
                   │
                   ▼
              ┌─────────┐
              │   F1    │ main.c (integra tutto)
              └────┬────┘
                   │
    ┌──────────────┼──────────────┐
    │              │              │
    ▼              ▼              ▼
┌──────┐      ┌──────┐       ┌──────┐
│  G1  │      │  G2  │       │ G3/4 │
│build │      │README│       │ docs │
└──┬───┘      └──┬───┘       └──┬───┘
   │             │              │
   └─────────────┴──────────────┘
                 │
                 ▼
         ┌───────────────┐
         │   BLOCCO H    │
         │   (Release)   │
         │  SEQUENZIALE  │
         └───────────────┘
```

---

## Dettaglio Implementazioni

### A1: VERSION
```
1.0.0
```

### A2: include/nous/hardware.h
```c
#ifndef CONVERGIO_HARDWARE_H
#define CONVERGIO_HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CHIP_VARIANT_BASE = 0,
    CHIP_VARIANT_PRO = 1,
    CHIP_VARIANT_MAX = 2,
    CHIP_VARIANT_ULTRA = 3
} ChipVariant;

typedef enum {
    CHIP_FAMILY_UNKNOWN = 0,
    CHIP_FAMILY_M1 = 1,
    CHIP_FAMILY_M2 = 2,
    CHIP_FAMILY_M3 = 3,
    CHIP_FAMILY_M4 = 4
} ChipFamily;

typedef struct {
    // Identification
    char chip_name[64];           // "Apple M3 Max"
    ChipFamily family;            // M1, M2, M3, M4
    ChipVariant variant;          // base, Pro, Max, Ultra

    // Core counts (rilevati via sysctl)
    uint8_t p_cores;              // Performance cores
    uint8_t e_cores;              // Efficiency cores
    uint8_t total_cores;          // p_cores + e_cores

    // GPU (rilevato via Metal API)
    uint8_t gpu_cores;            // Metal GPU cores

    // Neural Engine (stimato da chip family/variant)
    uint8_t neural_cores;         // Neural Engine cores

    // Memory
    uint64_t memory_bytes;        // Unified memory totale
    uint32_t memory_bandwidth_gbps; // Bandwidth stimata

    // Parametri ottimizzati (calcolati)
    uint32_t optimal_fabric_shards;   // Per SemanticFabric
    uint32_t optimal_gpu_batch;       // Per GPU operations
    uint32_t optimal_embedding_buffer; // Dimensione buffer embeddings
    uint32_t optimal_threadgroup_size; // Per Metal compute

} AppleSiliconInfo;

// Singleton globale
extern AppleSiliconInfo g_hardware;

// Funzioni
int convergio_detect_hardware(void);
void convergio_print_hardware_info(void);
const char* convergio_chip_family_name(ChipFamily family);
const char* convergio_chip_variant_name(ChipVariant variant);

#endif // CONVERGIO_HARDWARE_H
```

### A3: include/nous/config.h
```c
#ifndef CONVERGIO_CONFIG_H
#define CONVERGIO_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    // API
    char anthropic_api_key[256];

    // Budget
    double budget_limit;
    int budget_warn_percent;

    // UI
    bool color_enabled;
    char debug_level[16];  // none, error, warn, info, debug, trace

    // Updates
    bool check_updates_on_startup;
    bool auto_update;

    // Paths (calcolati)
    char config_dir[512];    // ~/.convergio
    char config_file[512];   // ~/.convergio/config.toml
    char db_path[512];       // ~/.convergio/convergio.db
    char notes_dir[512];     // ~/.convergio/notes
    char knowledge_dir[512]; // ~/.convergio/knowledge

} ConvergioConfig;

// Singleton globale
extern ConvergioConfig g_config;

// Funzioni
int convergio_config_init(void);           // Crea dirs, carica config
int convergio_config_load(void);           // Carica da file
int convergio_config_save(void);           // Salva su file
const char* convergio_config_get(const char* key);
int convergio_config_set(const char* key, const char* value);

// API Key (con fallback chain)
const char* convergio_get_api_key(void);   // Keychain -> env -> config

// Setup wizard
int convergio_setup_wizard(void);

#endif // CONVERGIO_CONFIG_H
```

### A4: include/nous/updater.h
```c
#ifndef CONVERGIO_UPDATER_H
#define CONVERGIO_UPDATER_H

#include <stdbool.h>

typedef struct {
    char current_version[32];
    char latest_version[32];
    char download_url[512];
    char release_notes[8192];
    char published_at[32];
    bool update_available;
} UpdateInfo;

// Funzioni
int convergio_check_update(UpdateInfo* info);
int convergio_download_update(const UpdateInfo* info, const char* dest_path);
int convergio_apply_update(const char* new_binary_path);
void convergio_print_update_info(const UpdateInfo* info);

// Comandi CLI
int convergio_cmd_update_check(void);
int convergio_cmd_update_install(void);
int convergio_cmd_update_changelog(void);

#endif // CONVERGIO_UPDATER_H
```

### A5: .github/workflows/ci.yml
```yaml
name: CI

on:
  pull_request:
    branches: [main]
  push:
    branches: [main]

jobs:
  build:
    name: Build & Test
    runs-on: macos-14  # Apple Silicon (M1)

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Build
        run: |
          make clean
          make 2>&1 | tee build.log

      - name: Check for warnings
        run: |
          if grep -i "warning:" build.log; then
            echo "::error::Build produced warnings"
            exit 1
          fi

      - name: Verify binary
        run: |
          ./build/bin/convergio --version

      - name: Verify hardware detection
        run: |
          ./build/bin/convergio version
```

### A6: .github/workflows/release.yml
```yaml
name: Release

on:
  push:
    tags:
      - 'v*'

permissions:
  contents: write

jobs:
  build-and-release:
    name: Build & Release
    runs-on: macos-14

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Get version from tag
        id: version
        run: echo "VERSION=${GITHUB_REF#refs/tags/v}" >> $GITHUB_OUTPUT

      - name: Verify VERSION file matches tag
        run: |
          FILE_VERSION=$(cat VERSION)
          TAG_VERSION=${{ steps.version.outputs.VERSION }}
          if [ "$FILE_VERSION" != "$TAG_VERSION" ]; then
            echo "::error::VERSION file ($FILE_VERSION) doesn't match tag ($TAG_VERSION)"
            exit 1
          fi

      - name: Build release binary
        run: make clean && make

      - name: Verify binary
        run: |
          ./build/bin/convergio --version | grep "${{ steps.version.outputs.VERSION }}"

      - name: Create tarball
        run: |
          mkdir -p dist
          cd build/bin
          tar -czvf ../../dist/convergio-${{ steps.version.outputs.VERSION }}-arm64-apple-darwin.tar.gz convergio
          cd ../..

      - name: Calculate SHA256
        id: sha256
        run: |
          SHA=$(shasum -a 256 dist/convergio-*.tar.gz | awk '{print $1}')
          echo "SHA256=$SHA" >> $GITHUB_OUTPUT
          echo "SHA256: $SHA"

      - name: Create GitHub Release
        uses: softprops/action-gh-release@v1
        with:
          files: dist/*.tar.gz
          body: |
            ## Convergio v${{ steps.version.outputs.VERSION }}

            ### Installation
            ```bash
            brew tap Roberdan/convergio-cli
            brew install convergio
            ```

            ### Binary SHA256
            ```
            ${{ steps.sha256.outputs.SHA256 }}
            ```

            ### First Run
            ```bash
            convergio setup
            ```
          generate_release_notes: true

      - name: Update Homebrew Formula
        run: |
          VERSION=${{ steps.version.outputs.VERSION }}
          SHA256=${{ steps.sha256.outputs.SHA256 }}

          sed -i '' "s/version \".*\"/version \"${VERSION}\"/" Formula/convergio.rb
          sed -i '' "s/sha256 \".*\"/sha256 \"${SHA256}\"/" Formula/convergio.rb
          sed -i '' "s|/v[0-9]*\.[0-9]*\.[0-9]*/|/v${VERSION}/|g" Formula/convergio.rb

          git config user.name "GitHub Actions"
          git config user.email "actions@github.com"
          git add Formula/convergio.rb
          git commit -m "chore: Update Homebrew formula to v${VERSION}"
          git push origin HEAD:main
```

### A7: Formula/convergio.rb
```ruby
class Convergio < Formula
  desc "Multi-agent AI orchestration CLI for Apple Silicon"
  homepage "https://github.com/Roberdan/convergio-cli"
  version "1.0.0"
  license "MIT"

  on_macos do
    on_arm do
      url "https://github.com/Roberdan/convergio-cli/releases/download/v1.0.0/convergio-1.0.0-arm64-apple-darwin.tar.gz"
      sha256 "PLACEHOLDER_WILL_BE_UPDATED_BY_CI"
    end
  end

  depends_on :macos
  depends_on arch: :arm64

  def install
    bin.install "convergio"
  end

  def caveats
    <<~EOS
      To get started, run:
        convergio setup

      This will configure your Anthropic API key securely in macOS Keychain.

      Documentation: https://github.com/Roberdan/convergio-cli
    EOS
  end

  test do
    assert_match version.to_s, shell_output("#{bin}/convergio --version")
  end
end
```

### A8: .gitignore (verifiche)
```gitignore
# Build artifacts
build/
*.o
*.air
*.metallib

# Runtime data
data/
*.db

# Secrets - NEVER COMMIT
.env
.env.*
*.pem
*.key

# macOS
.DS_Store
*.xcodeproj/
*.xcworkspace/
xcuserdata/

# IDEs
.vscode/
.idea/
*.swp
*.swo
*~

# Distribution
dist/

# Debug
*.dSYM/
```

---

## Checklist File da Modificare

### Grep pre-esecuzione (OBBLIGATORIO)
```bash
# Eseguire PRIMA di iniziare per trovare TUTTE le occorrenze

# Costanti M3 hardcoded
rg -n "NOUS_M3_|M3_P_CORES|M3_E_CORES|M3_GPU|M3_NEURAL" --type c --type objc

# Riferimenti M3 Max
rg -n "M3 Max|apple-m3|mcpu=apple-m3"

# Numeri magici da parametrizzare
rg -n "P_CORES.*=.*10|E_CORES.*=.*4|GPU_CORES.*=.*30|NEURAL.*=.*16" --type c

# Path hardcoded ./data
rg -n '"\./data|"data/' --type c
```

### File che DEVONO essere modificati
| File | Cosa cercare/modificare |
|------|------------------------|
| `include/nous/nous.h` | Rimuovere `NOUS_M3_*` costanti, includere `hardware.h` |
| `src/core/main.c` | Aggiungere `--version`, `setup`, `update`, `version` commands, chiamare `convergio_detect_hardware()` |
| `src/core/fabric.c` | Sostituire `NOUS_FABRIC_SHARDS` con `g_hardware.optimal_fabric_shards` |
| `src/metal/gpu.m` | Usare `g_hardware.optimal_*` invece di costanti |
| `src/runtime/scheduler.c` | Usare `g_hardware.p_cores`, `g_hardware.e_cores` |
| `src/neural/mlx_embed.m` | Adattare batch sizes |
| `src/neural/claude.c` | Usare `convergio_get_api_key()` invece di `getenv()` diretto |
| `src/memory/persistence.c` | Usare `g_config.db_path` invece di path hardcoded |
| `src/tools/tools.c` | Usare `g_config.notes_dir`, `g_config.knowledge_dir` |
| `Makefile` | Rimuovere `-mcpu=apple-m3`, aggiungere `-DCONVERGIO_VERSION`, nuovi source files |

---

## Documentazione da Aggiornare

| File | Modifiche |
|------|-----------|
| `README.md` | Aggiungere sezione Homebrew install, rimuovere riferimenti M3-only, aggiungere requisiti |
| `CHANGELOG.md` | Entry per v1.0.0 con tutte le nuove features |
| `CONTRIBUTING.md` | Aggiungere info su CI/CD e release process |
| `SECURITY.md` | Documentare Keychain usage per API key |
| `docs/adr/001-persistence-layer.md` | Aggiornare path `~/.convergio/` |
| `docs/adr/004-mlx-embeddings.md` | Rimuovere riferimenti M3-specific |
| **NUOVO** `docs/adr/005-apple-silicon-detection.md` | Documentare sistema hardware detection |
| **NUOVO** `docs/adr/006-homebrew-distribution.md` | Documentare strategia distribuzione |

---

## Timeline Stimata (con parallelizzazione)

| Fase | Task | Tempo |
|------|------|-------|
| 1 | BLOCCO A (8 task paralleli) | ~30 min |
| 2 | BLOCCO B + C + D (5 task, 3 paralleli) | ~1h |
| 3 | BLOCCO E (5 task paralleli) | ~45 min |
| 4 | BLOCCO F (main.c integration) | ~1h |
| 5 | BLOCCO G (docs + test, paralleli) | ~30 min |
| 6 | BLOCCO H (release, sequenziale) | ~30 min |
| **TOTALE** | | **~4-5 ore** |

---

## Rischi e Mitigazioni

| Rischio | Probabilità | Impatto | Mitigazione |
|---------|-------------|---------|-------------|
| Build fallisce su M1/M2 | Media | Alto | Test su GitHub Actions (M1) prima di release |
| Keychain API complesse | Bassa | Medio | Wrapper semplice, fallback su env var |
| TOML parsing bugs | Media | Basso | Parser minimale, test coverage |
| GitHub Actions quota | Bassa | Basso | Repo pubblico = unlimited |
| Formula Homebrew rifiutata | N/A | N/A | Usiamo tap custom, non homebrew-core |
| Auto-update rompe binary | Media | Alto | Backup prima di replace, rollback automatico |

---

## Criteri di Accettazione (Zero Tolerance)

- [ ] `make clean && make` completa con **ZERO WARNING**
- [ ] `./build/bin/convergio --version` mostra versione corretta
- [ ] Hardware detection funziona (testato su almeno M1 via CI)
- [ ] `convergio setup` salva API key in Keychain
- [ ] `convergio update check` funziona
- [ ] Nessun path hardcoded `./data` nel codice
- [ ] Nessuna costante `M3_` o `apple-m3` hardcoded
- [ ] `.gitignore` non permette commit di secrets
- [ ] CI passa su ogni PR
- [ ] Release automatica funziona su tag
- [ ] `brew install` funziona
- [ ] Tutta la documentazione è aggiornata e coerente

