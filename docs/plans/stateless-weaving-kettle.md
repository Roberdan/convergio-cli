# Generic Executive Report Generator Plan

## Obiettivo
Creare una versione generica di `generate_report.py` che funzioni per qualsiasi studio/industria senza modificare i file esistenti di Studio 3.

**CRITICO**: Tutti i file vanno creati nel worktree `../VirtualBPM-exec-generic`, NON nel repo principale.

## Analisi Attuale

### Elementi Hardcoded per Studio 3
| Categoria | Attuale | Da Parametrizzare |
|-----------|---------|-------------------|
| Nome Studio | "GTM EMEA Studio 3" | CLI parameter `--studio` |
| Output file | `EMEA_S3_Executive_Report_H1_FY26.html` | Generato da studio + periodo |
| Output path | `.../FY26/Studio3/ExecReports` | `.../FY26/{studio_slug}/ExecReports` |
| Cutoff date | 2025-12-31 | CLI parameter `--cutoff` |
| Narratives | Storie specifiche UK/MEA/Pharma | Template generico o skip |

### Dipendenze Esterne
- `client_summaries.py` - Contiene summary per clienti Studio 3
- `fy25_comparison.py` - Contiene dati FY25 per Studio 3 (lista manuale di engagements da ADO)
- `config/rsi_msx_scenarios.yml` - Mappature RSI (generico, OK)

## Approccio Proposto

### Opzione A: Fork Parametrizzato (Consigliato)
Creare `generate_report_generic.py` che:
1. Accetta `--studio "GTM EMEA Studio 4"` come parametro
2. Genera automaticamente slug/nomi file: `EMEA_S4_Executive_Report_H1_FY26.html`
3. Usa output path: `.../FY26/Studio4/ExecReports/`
4. **Narrative generiche** o skip delle sezioni narrative hardcoded
5. Mantiene la struttura visuale (cards, charts, RSI, geography)

### Opzione B: Config-Driven
File YAML per ogni studio con:
- Nome, slug, regioni geografiche
- Narrative custom (opzionali)
- Dati FY25 comparison (opzionali)

## File da Creare (nel worktree separato)
```
scripts/python/executive_reports/
├── generate_report.py          # ESISTENTE - NON TOCCARE
├── generate_report_generic.py  # NUOVO - versione parametrizzata
├── studio_config.py            # NUOVO - config per studio
└── studio_configs/             # NUOVO - config per studio
    └── default.yml
```

## Output File Naming Convention
```
{REGION}_{STUDIO_NUM}_Executive_Report_{PERIOD}.html

Esempi:
- EMEA_S3_Executive_Report_H1_FY26.html (Studio 3)
- EMEA_S4_Executive_Report_H1_FY26.html (Studio 4)
- APAC_S1_Executive_Report_H1_FY26.html (Asia)
```

## Decisioni Utente
1. **Narrative**: Custom per Studio 4 (richiede analisi portfolio)
2. **FY25 Data**: Query ADO come per Studio 3
3. **Geography**: Dinamico, calcolato dai dati engagement

## Piano di Implementazione Dettagliato

### Step 1: Query FY25 Data per Studio 4 (da ADO)
```bash
# WIQL query: Custom.LedByTeam = "GTM EMEA Studio 4" AND end date in FY25
# FY25 = Jul 1, 2024 to Jun 30, 2025
```
Output: Lista di engagements con (ID, Title, Start, End) per popolare `fy25_comparison_studio4.py`

### Step 2: Creare Struttura File (nel worktree)
```
../VirtualBPM-exec-generic/scripts/python/executive_reports/
├── generate_report.py              # ESISTENTE - copiato da main
├── generate_report_generic.py      # NUOVO - versione parametrizzata
├── studio_config.py                # NUOVO - studio configuration loader
├── fy25_comparison.py              # ESISTENTE (Studio 3)
├── fy25_comparison_studio4.py      # NUOVO - dati FY25 Studio 4
├── client_summaries.py             # ESISTENTE (Studio 3)
├── client_summaries_studio4.py     # NUOVO - summaries Studio 4
└── narratives/                     # NUOVO - narrative templates
    ├── studio3.py                  # Narrative Studio 3 (estratte da generate_report.py)
    └── studio4.py                  # Narrative Studio 4 (nuove)
```

### Step 3: Modifiche a generate_report_generic.py
1. **CLI Arguments**:
   - `--studio "GTM EMEA Studio 4"` (obbligatorio)
   - `--input engagements.json` (come ora)
   - `--output` (default: `~/OneDrive-Microsoft/FY26/{studio_slug}/ExecReports`)

2. **Output File Naming**:
   ```python
   # "GTM EMEA Studio 4" -> "EMEA_S4"
   def get_studio_slug(studio_name):
       # "GTM EMEA Studio 4" -> "EMEA_S4"
       # "GTM Americas Studio 2" -> "AMER_S2"
       pass

   OUTPUT_FILE = f"{slug}_Executive_Report_H1_FY26.html"
   ```

3. **Dynamic Geography**:
   - Calcolare countries/regions dai customer nel JSON
   - Generare Geographic Coverage section dinamicamente

4. **Module Loading Dinamico**:
   ```python
   # Load studio-specific modules
   fy25_module = importlib.import_module(f"fy25_comparison_{studio_slug}")
   narratives_module = importlib.import_module(f"narratives.{studio_slug}")
   summaries_module = importlib.import_module(f"client_summaries_{studio_slug}")
   ```

### Step 4: Analisi Portfolio Studio 4 per Narrative
1. Eseguire RTB per Studio 4 (gia fatto - 18 engagements)
2. Identificare:
   - Industry mix (Healthcare, Gov, Energy)
   - Customer list e paesi
   - Trends: quali settori in crescita? quali mercati?
3. Scrivere narrative in `narratives/studio4.py`

### Step 5: Client Summaries Studio 4
Per ogni customer in Studio 4, creare summary con:
- `business_context`
- `technical_focus`
- `client_impact`
- `microsoft_impact`

## File da Modificare (nel worktree `../VirtualBPM-exec-generic`)

| File | Azione | Note |
|------|--------|------|
| `generate_report_generic.py` | CREATE | Fork di generate_report.py con parametri |
| `studio_config.py` | CREATE | Mappature studio name -> slug, region |
| `fy25_comparison_studio4.py` | CREATE | Dati FY25 da ADO query |
| `client_summaries_studio4.py` | CREATE | Summaries per clienti Studio 4 |
| `narratives/studio4.py` | CREATE | Narrative custom Studio 4 |

## Output Garantiti (nessun conflitto con Studio 3)

| Studio | Output Path | Output File |
|--------|-------------|-------------|
| Studio 3 | `.../FY26/Studio3/ExecReports/` | `EMEA_S3_Executive_Report_H1_FY26.html` |
| Studio 4 | `.../FY26/Studio4/ExecReports/` | `EMEA_S4_Executive_Report_H1_FY26.html` |

## Rischi e Mitigazioni
- **Rischio**: Sovrascrivere file Studio 3
  - **Mitigazione**: Output path separato + nome file diverso (S4 vs S3)
- **Rischio**: Client summaries mancanti per Studio 4
  - **Mitigazione**: Fallback a "Summary not available for this customer"
- **Rischio**: Narrative non rilevanti
  - **Mitigazione**: Analisi portfolio prima di scrivere narrative

---

## Progress Tracking (2025-12-18)

### Completati
- [x] Query FY25 Studio 4 engagements (6 found)
- [x] Query FY26 Studio 4 engagements (18 active)
- [x] `fy25_comparison_studio4.py` - dati storici
- [x] `studio_config.py` - configurazione studio
- [x] `client_summaries_studio4.py` - 15+ customers
- [x] `narratives/studio4.py` - narrative custom
- [x] `generate_report_generic.py` - generatore parametrizzato
- [x] Esportare engagements.json per Studio 4 ✅
- [x] Eseguire generate_report_generic.py --studio "Studio 4" ✅
- [x] Verificare output in Studio4/ExecReports/ ✅
- [x] Rimozione contenuto hardcoded per Studio 3 da generate_report.py
  - Geographic Coverage dinamico (non più hardcoded EMEA/MEA)
  - Executive Summary usa H1_NARRATIVE/H2_NARRATIVE se disponibili
  - Delta comparison usa DELTA_NARRATIVE se disponibile
  - Titoli e sottotitoli usano STUDIO_NAME/STUDIO_SHORT
  - Tutti i riferimenti a owner ora usano .get() per handle missing fields

### Report Generato
- **File**: `EMEA_S4_Executive_Report_H1_FY26.html` (165,458 bytes)
- **Location**: `/Users/roberdan/Library/CloudStorage/OneDrive-Microsoft/FY26/Studio4/ExecReports/`
- **No-JS versions**: 13 files in `nojs/` directory

### Prossimi Passi
- [x] Download logos per Studio 4 customers ✅ (14 logos downloaded 2025-12-18)
- [ ] Verificare Azure OpenAI per narrative LLM-generated (se richiesto)
- [ ] Consolidare FY year/quarter/semester config
- [ ] Testare report per industria (filtro Custom.Industry)

### Logos Downloaded (2025-12-18)
14 customer logos successfully downloaded:
- nokia.png (2,910 bytes)
- sainsburys.png (683 bytes)
- kantar.png (11,612 bytes)
- migros.png (2,274 bytes)
- prh.png (11,379 bytes)
- vodafone.png (8,749 bytes)
- luxottica.png (15,406 bytes)
- asos.png (1,790 bytes)
- sitecore.png (1,895 bytes)
- siemens.png (2,379 bytes)
- bertelsmann.png (1,150 bytes)
- givaudan.png (5,111 bytes)
- nestle.png (3,650 bytes)
- outbrain.png (3,877 bytes)

### Portfolio Studio 4 Analysis
**Industries:**
- Retail & Consumer Goods: Sainsbury's, Nestle, Migros, Luxottica, ASOS, Givaudan, Heineken
- Media & Communications: Nokia, Bertelsmann, Vodafone, Kantar, BT, KPN, Sitecore
- Manufacturing: Siemens

**Key Themes:**
1. Retail AI: Contact centers, theft prevention, e-commerce
2. Telco AI Native: Network operations, AIOps
3. Publishing Innovation: Manuscript processing, accessibility

**Geographic Focus:**
- UK: Sainsbury's, Vodafone, BT, ASOS, Kantar
- Germany: Bertelsmann, Siemens
- Netherlands: KPN
- Switzerland: Nestle, Migros, Givaudan
- Finland: Nokia
- Italy: Luxottica
