# Action Plan - Cosa Fare Ora

**Created**: 2025-12-20  
**Status**: Ready to Execute  
**Priority**: Critical → High → Medium

---

## 🚀 QUICK WINS (Oggi - 2-4 ore)

### 1. Makefile Targets Workflow-Specific
**Effort**: 1-2 ore  
**Priority**: 🔴 CRITICAL  
**Blocca**: Quality gates, test execution

**Cosa fare**:
- Aggiungere target `test_workflow_quick` al Makefile
- Aggiungere target `integration_test_workflow` al Makefile
- Aggiungere target `fuzz_test_workflow` al Makefile
- Aggiungere target `coverage_workflow` al Makefile
- Aggiungere target `quality_gate_workflow` al Makefile
- Aggiungere target `security_audit_workflow` al Makefile

**File**: `Makefile`

---

### 2. Eseguire Verifiche Immediate
**Effort**: 1-2 ore  
**Priority**: 🔴 CRITICAL  
**Blocca**: Non sappiamo se il codice funziona

**Cosa fare**:
```bash
# 1. Code coverage
make coverage
# Verificare: >= 80% per workflow code

# 2. Sanitizer tests
make DEBUG=1 SANITIZE=address,undefined,thread test
# Verificare: zero leaks, zero races, zero undefined behavior

# 3. Pre-release check
./scripts/pre_release_check.sh --verbose
# Verificare: tutti i check passano

# 4. Build warnings
make clean && make 2>&1 | grep -i warning
# Verificare: zero warnings
```

**Output**: Report con risultati. Se falliscono, fixare.

---

## 🔴 CRITICI (Questa Settimana - 5-10 giorni)

### 3. Mermaid Visualization Export (Phase 4, R3)
**Effort**: 2-3 giorni  
**Priority**: 🔴 CRITICAL  
**Blocca**: Feature promessa ma mancante

**Cosa fare**:
1. Creare `src/workflow/workflow_visualization.c`
2. Implementare `workflow_export_mermaid(Workflow* wf)`
3. Aggiungere CLI command `--mermaid` a `workflow show`
4. Integrare con `output_service` per export SVG/PNG
5. Aggiungere test in `tests/test_workflow_visualization.c`

**File da creare**:
- `src/workflow/workflow_visualization.c`
- `include/nous/workflow_visualization.h`
- `tests/test_workflow_visualization.c`

**File da modificare**:
- `src/core/commands/workflow.c` (aggiungere `--mermaid` flag)
- `Makefile` (aggiungere test target)

---

### 4. Test Files Mancanti
**Effort**: 3-4 giorni  
**Priority**: 🔴 CRITICAL  
**Blocca**: Testing completo

#### 4.1 `tests/test_workflow_migration.c`
**Effort**: 1 giorno

**Cosa fare**:
- Test migration execution
- Test idempotency (eseguire 2 volte)
- Test rollback su error
- Test foreign keys
- Test indexes
- Test schema correctness

#### 4.2 `tests/test_workflow_integration.c`
**Effort**: 2-3 giorni

**Cosa fare**:
- Test end-to-end con tutti i componenti
- Test backward compatibility (orchestrator esistente)
- Test performance (tutti i target)
- Test error recovery (retry, fallback)
- Test cost tracking integration
- Test full system integration (tutte le fasi insieme)

**File da creare**:
- `tests/test_workflow_migration.c`
- `tests/test_workflow_integration.c`

**File da modificare**:
- `Makefile` (aggiungere test targets)

---

### 5. Documentazione Mancante
**Effort**: 2-3 giorni  
**Priority**: 🔴 CRITICAL  
**Blocca**: Utenti non sanno come usare la feature

#### 5.1 `docs/workflow-orchestration/architecture.md`
**Effort**: 1 giorno

**Cosa fare**:
- Architettura completa del workflow engine
- Diagrammi (usare Mermaid quando disponibile)
- Design decisions
- Component relationships
- Data flow
- Control flow

#### 5.2 `docs/adr/018-workflow-orchestration.md`
**Effort**: 0.5 giorni

**Cosa fare**:
- Context: Perché workflow engine
- Decision: Implementare workflow engine
- Consequences: Pro/contro
- Status: Accepted

#### 5.3 `docs/workflow-orchestration/MIGRATION_GUIDE.md`
**Effort**: 1 giorno

**Cosa fare**:
- Come migrare da orchestrator esistente
- Esempi di migrazione pattern
- Breaking changes (se ci sono)
- Step-by-step guide

#### 5.4 `docs/workflow-orchestration/PATTERN_GUIDE.md`
**Effort**: 1 giorno

**Cosa fare**:
- Guida completa ai pattern disponibili
- Come usare ogni pattern
- Esempi pratici
- Quando usare quale pattern

**File da creare**:
- `docs/workflow-orchestration/architecture.md`
- `docs/adr/018-workflow-orchestration.md`
- `docs/workflow-orchestration/MIGRATION_GUIDE.md`
- `docs/workflow-orchestration/PATTERN_GUIDE.md`

---

## 🟡 HIGH PRIORITY (Prossime 2 Settimane - 10-15 giorni)

### 6. Orchestrator Integration Completa
**Effort**: 3-5 giorni  
**Priority**: 🟡 HIGH  
**Blocca**: Workflow engine non può essere usato con orchestrator esistente

**Cosa fare**:
1. Migrare pattern esistenti a workflow
   - Parallel analysis → workflow
   - Sequential planning → workflow
   - Critic loops → workflow
2. Verificare backward compatibility
   - Test che funzioni esistente ancora funziona
   - Test che nuovo codice non rompe nulla
3. Creare piano deprecazione graduale
   - Timeline per deprecare funzioni vecchie
   - Migration path per utenti
4. Implementare wrapper functions
   - `orchestrator_parallel_analyze_v2` (workflow-based)
   - Mantenere `orchestrator_parallel_analyze` (legacy, deprecated)

**File da modificare**:
- `src/orchestrator/orchestrator.c`
- `src/orchestrator/delegation.c`
- `src/orchestrator/planning.c`

**File da creare**:
- `docs/workflow-orchestration/MIGRATION_GUIDE.md` (già sopra)

---

### 7. Performance Optimization
**Effort**: 3-5 giorni  
**Priority**: 🟡 HIGH  
**Blocca**: Workflow potrebbe essere lento

**Cosa fare**:
1. Checkpoint optimization (incremental)
   - Salvare solo delta invece di stato completo
   - Compressione checkpoint
2. State serialization optimization
   - Usare binary format invece di JSON
   - Streaming serialization per grandi stati
3. Memory management improvements
   - Pool allocation per workflow objects
   - Lazy loading per checkpoint
4. Eseguire performance benchmarks
   - Misurare tutti i target
   - Identificare bottleneck
   - Ottimizzare hot paths

**File da modificare**:
- `src/workflow/checkpoint.c`
- `src/workflow/workflow_types.c`
- `src/workflow/workflow_engine.c`

**File da creare**:
- `tests/test_workflow_performance.c`

---

### 8. Security Verification Completa (60% → 100%)
**Effort**: 2-3 giorni  
**Priority**: 🟡 HIGH  
**Blocca**: Potrebbero esserci vulnerabilità

**Cosa fare**:
1. Verificare tutti i file operations usano `safe_path_open`
   - `src/core/config.c`
   - `src/telemetry/telemetry.c` (già fatto parzialmente)
   - `src/memory/semantic_persistence.c`
   - `src/projects/projects.c`
   - `src/tools/output_service.c`
2. Verificare tutti i command executions usano `tools_is_command_safe`
   - `src/orchestrator/orchestrator.c`
   - `src/agents/agent.c`
3. Verificare tutti gli input sono validati
   - `src/core/config.c`
   - `src/agents/agent.c`
   - `src/core/commands/*.c`

**File da modificare**:
- Tutti i file sopra elencati

---

### 9. Test Incompleti - Completare
**Effort**: 3-5 giorni  
**Priority**: 🟡 HIGH  
**Blocca**: Edge cases non testati

**Cosa fare**:
1. Migration tests
   - Idempotency test
   - Rollback test
2. Memory tests
   - Memory leak detection (100 iterations)
   - Double-free prevention
   - Use-after-free prevention
3. Thread safety tests
   - Thread Sanitizer tests
   - Concurrent execution tests
4. Fuzz tests
   - LLVMFuzzerTestOneInput per checkpoint restoration
   - Fuzz tests per condition parser
5. Access control tests
   - User-specific checkpoints
6. Template matching tests
   - Task decomposer template matching
7. Group chat tests
   - Message threading
   - Timeout handling

**File da modificare**:
- `tests/test_workflow_checkpoint.c`
- `tests/test_workflow_types.c`
- `tests/test_workflow_engine.c`
- `tests/test_task_decomposer.c`
- `tests/test_group_chat.c`
- `tests/test_router.c`

**File da creare**:
- `tests/test_workflow_migration.c` (già sopra)
- `tests/test_workflow_fuzz.c`

---

## 🟢 MEDIUM PRIORITY (Dopo - 5-10 giorni)

### 10. Features Medie Mancanti
**Effort**: 2-3 giorni

**Cosa fare**:
- Template matching verification in task decomposer
- Access control per checkpoints (user-specific)
- Checkpoint cleanup functionality
- Message threading in group chat
- Timeout handling in group chat

---

### 11. Help System Completo
**Effort**: 2-3 ore

**Cosa fare**:
- Help dettagliato per ogni subcommand
- Esempi di uso nei help
- Troubleshooting guide

---

### 12. Example Workflows
**Effort**: 1 giorno

**Cosa fare**:
- Esempi pratici di workflow semplici
- Esempi di workflow complessi
- Tutorial step-by-step
- Esempi di integrazione con agenti esistenti

---

## 📋 PRIORITÀ ESECUZIONE

### Fase 1: Quick Wins (Oggi)
1. ✅ Makefile targets (1-2 ore)
2. ✅ Eseguire verifiche (1-2 ore)

**Totale**: 2-4 ore

### Fase 2: Critici (Questa Settimana)
3. Mermaid export (2-3 giorni)
4. Test files mancanti (3-4 giorni)
5. Documentazione mancante (2-3 giorni)

**Totale**: 7-10 giorni

### Fase 3: High Priority (Prossime 2 Settimane)
6. Orchestrator integration (3-5 giorni)
7. Performance optimization (3-5 giorni)
8. Security verification (2-3 giorni)
9. Test incompleti (3-5 giorni)

**Totale**: 11-18 giorni

### Fase 4: Medium Priority (Dopo)
10. Features medie (2-3 giorni)
11. Help system (2-3 ore)
12. Example workflows (1 giorno)

**Totale**: 3-4 giorni

---

## 🎯 ORDINE DI ESECUZIONE RACCOMANDATO

### Oggi (4 ore)
1. Makefile targets
2. Eseguire verifiche
3. Fixare problemi trovati nelle verifiche

### Questa Settimana (7-10 giorni)
1. Mermaid export (2-3 giorni)
2. Test files mancanti (3-4 giorni)
3. Documentazione mancante (2-3 giorni)

### Prossime 2 Settimane (11-18 giorni)
1. Orchestrator integration (3-5 giorni)
2. Performance optimization (3-5 giorni)
3. Security verification (2-3 giorni)
4. Test incompleti (3-5 giorni)

### Dopo (3-4 giorni)
1. Features medie (2-3 giorni)
2. Help system (2-3 ore)
3. Example workflows (1 giorno)

---

## ✅ DEFINITION OF DONE (Per Ogni Item)

Ogni item deve avere:
- [ ] Codice implementato
- [ ] Test scritti e passanti
- [ ] Documentazione aggiornata
- [ ] Verifiche eseguite (coverage, sanitizers)
- [ ] Code review (se necessario)
- [ ] Commit con messaggio descrittivo

---

**Last Updated**: 2025-12-20  
**Next Review**: Dopo completamento Fase 1

