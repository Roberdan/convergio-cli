# Istruzioni per Claude - Completamento Workflow Orchestration

**Created**: 2025-12-21  
**Status**: 🎯 READY FOR EXECUTION  
**Purpose**: Exact instructions for Claude to complete remaining work

---

## 🎯 CONTEXT

**Worktree**: `/Users/roberdan/GitHub/ConvergioCLI-workflow`  
**Branch**: `feature/workflow-orchestration`  
**PR**: #72 - "feat(workflow): Advanced Workflow Orchestration - Feature Complete"

---

## 📋 MASTER PLAN DI RIFERIMENTO

**⚠️ IMPORTANTE: Il documento principale di riferimento è:**

**`docs/workflow-orchestration/MASTER_PLAN.md`**

Questo è il **master plan ufficiale** che contiene:
- Status completo di tutte le fasi
- Definition of Done
- Tutti i task completati e pendenti
- Riferimenti a tutti gli altri documenti
- Statistiche e metriche

**Usa MASTER_PLAN.md come fonte di verità** per:
- Verificare lo status attuale
- Capire cosa è completato e cosa manca
- Aggiornare lo status dopo ogni task completato

**Altri documenti importanti** (ma secondari rispetto al master plan):
- `PENDING_TASKS_EXECUTABLE.md` - Lista esecutiva dei task (derivata dal master plan)
- `INSTRUCTIONS_FOR_CLAUDE.md` - Questo documento (istruzioni operative)

---

**Current Status** (dal MASTER_PLAN.md): 
- ✅ Core implementation: 100% complete
- ✅ Testing: 100% complete (all test files created)
- ⏳ Verifications: 0% (not executed)
- ⏳ Documentation: ~60% (4 files missing)
- ⏳ Best Practices: Phase 1-2 partial, Phase 3 pending
- ⏳ Future Enhancements: 0% (not started)

---

## 📖 DOCUMENTI DA LEGGERE (IN ORDINE)

### 1. PRIMA DI INIZIARE (OBBLIGATORIO)

**🎯 MASTER PLAN (LEGGI PRIMA DI TUTTO):**
- **MASTER_PLAN.md** - ⭐ **DOCUMENTO PRINCIPALE DI RIFERIMENTO**
  - Contiene lo status completo di tutto il progetto
  - Definition of Done
  - Tutti i task completati e pendenti
  - Aggiorna questo documento dopo ogni task completato

**📋 DOCUMENTI OPERATIVI:**
- **PENDING_TASKS_EXECUTABLE.md** - Lista esecutiva dei task (derivata dal master plan)
- **ZERO_TOLERANCE_POLICY.md** - Standard di qualità (zero tolleranza) - **MANDATORY**

### 2. PER OGNI TASK
- **BEST_PRACTICES.md** - Best practices per implementazione
- **SECURITY_ENFORCEMENT_PLAN.md** - Piano sicurezza (se lavori su security)
- **CODEBASE_AUDIT.md** - Audit code quality (se lavori su code quality)

### 3. RIFERIMENTI
- **GLOBAL_BEST_PRACTICES_PROPOSAL.md** - Best practices globali
- **CONTRIBUTING.md** - Linee guida contribuzione (include zero tolerance)

---

## 🚀 TASK DA COMPLETARE (PRIORITÀ)

### PRIORITÀ ALTA (Fare PRIMA)

#### 1. Fix Linking Error ⚠️ CRITICAL - BLOCKS COVERAGE

**Location**: `/Users/roberdan/GitHub/ConvergioCLI-workflow`  
**Branch**: `feature/workflow-orchestration`  
**Status**: 🔴 **BLOCKING** - Questo errore blocca l'esecuzione di `make coverage_workflow`

**Problema**: 
```
duplicate symbol '_nous_log' in:
    /private/var/folders/.../test_workflow_integration-*.o
    /private/var/folders/.../test_stubs-*.o
ld: 4 duplicate symbols
```

**File coinvolti**:
- `tests/test_workflow_integration.c` (riga 45) - Definisce `void nous_log(LogLevel level, LogCategory cat, ...)`
- `tests/unit/test_stubs.c` (riga 11) - Definisce `void nous_log(int level, ...)` con signature diversa
- Nel Makefile: `WORKFLOW_INTEGRATION_SOURCES` include sia `test_workflow_integration.c` che `$(TEST_STUBS)`

**Problema identificato**:
- `test_workflow_integration.c` definisce `nous_log` con signature: `(LogLevel level, LogCategory cat, ...)`
- `tests/unit/test_stubs.c` definisce `nous_log` con signature: `(int level, ...)`
- Entrambi vengono compilati e linkati insieme, causando duplicate symbol

**Comando per verificare l'errore**: 
```bash
cd /Users/roberdan/GitHub/ConvergioCLI-workflow
make coverage_workflow 2>&1 | grep -A 5 "duplicate symbol"
```

**Azione da eseguire**:

1. **Verifica il problema**:
   ```bash
   # Controlla se test_workflow_integration.c include test_stubs.c
   grep -n "test_stubs" tests/test_workflow_integration.c
   
   # Controlla le definizioni di nous_log
   grep -n "nous_log\|_nous_log" tests/test_workflow_integration.c tests/unit/test_stubs.c
   ```

2. **Soluzione**:
   - **Opzione A (Raccomandata)**: Rimuovi la definizione di `nous_log` da `test_workflow_integration.c` (riga 45-48)
     - Usa invece quella in `tests/unit/test_stubs.c` o `tests/test_stubs.c`
     - Oppure aggiungi `extern` declaration se necessario
   
   - **Opzione B**: Unifica le signature - assicurati che entrambe le definizioni abbiano la stessa signature
     - Verifica quale signature è corretta guardando `include/nous/nous.h`
     - Aggiorna quella sbagliata per matchare
   
   - **Opzione C**: Rendi `static` una delle definizioni se è usata solo in quel file
     - Ma questo potrebbe non funzionare se è usata da altri file di test
   
   - **Verifica Makefile**: 
     - `WORKFLOW_INTEGRATION_SOURCES = tests/test_workflow_integration.c $(TEST_STUBS)`
     - `TEST_STUBS = tests/test_stubs.c` (non `tests/unit/test_stubs.c`)
     - **Problema**: `test_workflow_integration.c` definisce `nous_log` ma anche `$(TEST_STUBS)` (che è `tests/test_stubs.c`) potrebbe definirlo
     - **Verifica**: Controlla se `tests/test_stubs.c` o `tests/unit/test_stubs.c` vengono compilati come oggetti separati e linkati
     - **Soluzione**: Rimuovi la definizione di `nous_log` da `test_workflow_integration.c` (riga 45-47) e usa quella da `tests/test_stubs.c` o `tests/unit/test_stubs.c`

3. **Verifica la fix**:
   ```bash
   make clean
   make coverage_workflow
   # Deve compilare senza errori di linking
   ```

4. **Se l'errore persiste**:
   - Cerca altre definizioni duplicate di `nous_log` o altre funzioni
   - Verifica che le funzioni stub siano dichiarate `static` se usate solo in un file
   - Usa `extern` per dichiarazioni in header se necessario

**Documentazione**: 
- Vedi `PENDING_TASKS_EXECUTABLE.md` sezione "6.1 Fix Linking Error"
- Vedi `MASTER_PLAN.md` sezione "Code Quality & Optimization"

**Nota**: Questo errore deve essere risolto PRIMA di poter eseguire le verifiche di coverage.

---

#### 2. Eseguire Verifiche

**Location**: `/Users/roberdan/GitHub/ConvergioCLI-workflow`  
**Branch**: `feature/workflow-orchestration`

##### 2.1 Code Coverage Measurement
- **Comando**: `make coverage_workflow`
- **Verifica**: Coverage >= 80%
- **Output**: `coverage/workflow_coverage.info`
- **Documentazione**: `MASTER_PLAN.md` sezione "Pending Verification"

##### 2.2 Sanitizer Tests
- **Comando**: `make DEBUG=1 SANITIZE=address,undefined,thread test`
- **Verifica**: Zero leaks, zero undefined behavior, zero data races
- **Documentazione**: `MASTER_PLAN.md` sezione "Quality Gates"

##### 2.3 Security Audit
- **Azione**: Review manuale (puoi usare `make security_audit_workflow`)
- **Verifica**: Zero vulnerabilità
- **Documentazione**: `SECURITY_CHECKLIST.md` e `SECURITY_ENFORCEMENT_PLAN.md`

##### 2.4 Performance Benchmarks
- **Azione**: Creare benchmark tests e eseguire
- **Verifica**: Performance targets met
- **Documentazione**: `MASTER_PLAN.md` sezione "Performance targets"

---

#### 3. Creare Documentazione Mancante

**Location**: `/Users/roberdan/GitHub/ConvergioCLI-workflow`  
**Branch**: `feature/workflow-orchestration`

##### 3.1 architecture.md
- **Path**: `docs/workflow-orchestration/architecture.md`
- **Template**: Vedi `MASTER_PLAN.md` sezione "Architecture Overview"
- **Contenuto**: System architecture, component interactions, data flow

##### 3.2 ADR 018
- **Path**: `docs/workflow-orchestration/ADR/018-workflow-orchestration.md`
- **Template**: Segui formato ADR esistente (vedi `ADR/001-error-handling-strategy.md`)
- **Contenuto**: Decisione di implementare workflow orchestration, rationale, alternative

##### 3.3 MIGRATION_GUIDE.md
- **Path**: `docs/workflow-orchestration/MIGRATION_GUIDE.md`
- **Contenuto**: Guida step-by-step per migrare codice esistente a workflows, esempi, pattern comuni

##### 3.4 PATTERN_GUIDE.md
- **Path**: `docs/workflow-orchestration/PATTERN_GUIDE.md`
- **Contenuto**: Documentazione pattern library, esempi d'uso, best practices

---

### PRIORITÀ MEDIA (Fare DOPO)

#### 4. CI/CD Coverage Tracking

**Location**: `/Users/roberdan/GitHub/ConvergioCLI-workflow`  
**Branch**: `feature/workflow-orchestration`

- **File**: `.github/workflows/workflow-coverage.yml` (creare)
- **Azione**: 
  - Workflow GitHub Actions che esegue `make coverage_workflow` su ogni PR
  - Upload coverage a codecov.io o simile
  - Fail PR se coverage < 80%
- **Documentazione**: `BEST_PRACTICES.md` sezione "Code Coverage Tracking"
- **Template**: Vedi `.github/workflows/ci.yml` esistente

---

#### 5. Security Enforcement Phase 2

**Location**: `/Users/roberdan/GitHub/ConvergioCLI-workflow`  
**Branch**: `feature/workflow-orchestration`

**File da aggiornare** (5 file):
- `src/core/main.c` - Sostituire `fopen()` con `safe_path_open()`
- `src/core/commands/commands.c` - Verificare uso `tools_is_command_safe()`
- `src/orchestrator/registry.c` - Aggiungere path validation
- `src/orchestrator/plan_db.c` - Aggiungere path validation
- `src/memory/memory.c` - Aggiungere path validation

**Documentazione**: `SECURITY_ENFORCEMENT_PLAN.md` sezione "Phase 2"

---

### PRIORITÀ BASSA (Fare DOPO)

#### 6. Future Enhancements

**Location**: `/Users/roberdan/GitHub/ConvergioCLI-workflow`  
**Branch**: `feature/workflow-orchestration`

##### 6.1 Workflow Execution History UI
- **File**: `src/workflow/workflow_history_ui.c` (nuovo)
- **Contenuto**: CLI-based UI per visualizzare workflow execution history
- **Documentazione**: `MASTER_PLAN.md` sezione "Future Enhancements"

##### 6.2 Extended Telemetry Events
- **File**: `src/telemetry/telemetry.c` (estendere)
- **Contenuto**: Event types più specifici per providers/orchestrator
- **Documentazione**: `MASTER_PLAN.md` sezione "Future Enhancements"

##### 6.3 Performance Telemetry
- **File**: `src/workflow/workflow_observability.c` (estendere)
- **Contenuto**: Metriche performance dettagliate (latency, throughput, resource usage)
- **Documentazione**: `MASTER_PLAN.md` sezione "Future Enhancements"

##### 6.4 Security Audit Logging
- **File**: `src/workflow/workflow_observability.c` (estendere)
- **Contenuto**: Enhanced security event logging, audit trail
- **Documentazione**: `MASTER_PLAN.md` sezione "Future Enhancements"

---

## 📝 WORKFLOW DI LAVORO

### Per ogni task:

1. **Leggi MASTER_PLAN.md** per capire il contesto del task
2. **Leggi la documentazione** specifica indicata nel task
3. **Verifica il branch**: `git branch --show-current` deve essere `feature/workflow-orchestration`
4. **Esegui il task** seguendo le istruzioni
5. **Testa**: Assicurati che tutto compili e funzioni
6. **Commit**: Usa conventional commits (es. `fix(workflow): resolve linking error`)
7. **Aggiorna MASTER_PLAN.md**: ⭐ **OBBLIGATORIO** - Marca il task come completato nella sezione appropriata
8. **Aggiorna PENDING_TASKS_EXECUTABLE.md**: Opzionale - per tracciamento

**⚠️ IMPORTANTE**: MASTER_PLAN.md è la fonte di verità. Aggiornalo sempre dopo ogni task.

### Standard di qualità (ZERO TOLERANCE):

- ✅ Zero errori di compilazione
- ✅ Zero warnings
- ✅ Tutti i test passano
- ✅ Coverage >= 80% (per nuovo codice)
- ✅ Zero memory leaks
- ✅ Zero data races
- ✅ Documentazione completa

**Vedi**: `ZERO_TOLERANCE_POLICY.md` e `CONTRIBUTING.md`

---

## 🎯 ORDINE DI ESECUZIONE CONSIGLIATO

1. **Fix linking error** (blocca coverage)
2. **Eseguire verifiche** (coverage, sanitizers)
3. **Creare documentazione mancante** (4 file)
4. **CI/CD coverage tracking** (automazione)
5. **Security enforcement Phase 2** (5 file)
6. **Future enhancements** (4 task)

---

## ✅ CRITERI DI COMPLETAMENTO

Ogni task è completo quando:
- ✅ Codice compila senza errori
- ✅ Tutti i test passano
- ✅ Documentazione è completa e accurata
- ✅ Cambiamenti committati con conventional commits
- ✅ MASTER_PLAN.md aggiornato con status
- ✅ PENDING_TASKS_EXECUTABLE.md aggiornato

---

## 📞 COMANDI UTILI

```bash
# Verifica branch
cd /Users/roberdan/GitHub/ConvergioCLI-workflow
git branch --show-current

# Build
make clean && make

# Test
make test

# Coverage
make coverage_workflow

# Sanitizers
make DEBUG=1 SANITIZE=address,undefined,thread test

# Quality gate
make quality_gate

# Security audit
make security_audit_workflow
```

---

## 🚨 IMPORTANTE

- **NON fare merge della PR** - solo completare i task
- **NON cambiare branch** - lavora sempre su `feature/workflow-orchestration`
- **NON committare su main** - solo su feature branch
- **SEMPRE testare** prima di committare
- **SEMPRE aggiornare documentazione** quando completi un task

---

**Last Updated**: 2025-12-21  
**Ready for**: Claude to execute tasks systematically

