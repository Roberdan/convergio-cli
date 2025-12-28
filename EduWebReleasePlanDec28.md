# EduWebReleasePlanDec28 - Convergio Education Release Plan

**Data**: 2025-12-28
**Target**: Production Ready + WCAG AA
**Metodo**: VERIFICA BRUTALE - ogni task testato prima di dichiararlo fatto

---

## 🎭 RUOLI CLAUDE

| Claude | Ruolo | Task Assegnati |
|--------|-------|----------------|
| **CLAUDE 1** | 🎯 COORDINATORE | Monitora piano, verifica coerenza, aggrega risultati |
| **CLAUDE 2** | 👨‍💻 IMPLEMENTER | BUG-01: Provider Selection UI |
| **CLAUDE 3** | 👨‍💻 IMPLEMENTER | BUG-02: Cost Config UI Form |
| **CLAUDE 4** | 👨‍💻 IMPLEMENTER | A11Y-01 + A11Y-02: Accessibility |

---

## ⚠️ REGOLE OBBLIGATORIE PER TUTTI I CLAUDE

```
1. PRIMA di iniziare: leggi TUTTO questo file
2. Trova i task assegnati a te (cerca "CLAUDE X" dove X è il tuo numero)
3. Per OGNI task:
   a. Leggi i file indicati
   b. Implementa la fix
   c. Esegui TUTTI i comandi di verifica
   d. Solo se TUTTI passano, aggiorna questo file marcando ✅ DONE

4. VERIFICA OBBLIGATORIA dopo ogni task:
   npm run lint        # DEVE essere 0 errors, 0 warnings
   npm run typecheck   # DEVE compilare senza errori
   npm run build       # DEVE buildare senza errori

5. NON DIRE MAI "FATTO" SE:
   - Non hai eseguito i 3 comandi sopra
   - Anche UN SOLO warning appare
   - Non hai aggiornato questo file

6. Se trovi problemi/blocchi: CHIEDI invece di inventare soluzioni

7. Dopo aver completato: aggiorna la sezione EXECUTION TRACKER con ✅

8. CONFLITTI GIT: Se ci sono conflitti, risolvi mantenendo ENTRAMBE le modifiche
```

---

## 🎯 EXECUTION TRACKER

### Phase 1: Security — 7/7 ✅ COMPLETE

| Status | ID | Task | Assignee | Note |
|:------:|-----|------|----------|------|
| ✅ | SEC-01 | Fix API Key Leak | ALTRO | WebSocket proxy implemented |
| ✅ | SEC-02 | Fix XSS HTML Preview | ALTRO | DOMPurify hardened |
| ✅ | SEC-03 | Remove progress/sync stub | CLAUDE 1 | DONE |
| ✅ | SEC-04 | Fix CORS wildcard | CLAUDE 1 | DONE |
| ✅ | SEC-05 | Session Expiry (P0-S2) | VERIFICATO | maxAge: 365 days in user route |
| ✅ | SEC-06 | IDOR Protection (P0-S3) | VERIFICATO | userId scoping in conversations |
| ✅ | SEC-07 | DATABASE_URL Fallback (P0-D1) | VERIFICATO | SQLite fallback in prisma.config.ts |

### Phase 2: Code Quality — 3/3 ✅ COMPLETE

| Status | ID | Task | Assignee | Note |
|:------:|-----|------|----------|------|
| ✅ | CQ-01 | Fix ESLint errors | CLAUDE 1 | 0 errors |
| ✅ | CQ-02 | Remove console.log | CLAUDE 1 | Replaced with logger |
| ✅ | TD-01 | Add typecheck script | CLAUDE 1 | DONE |

### Phase 3: Missing Features — 2/2 ✅ COMPLETE

| Status | ID | Task | Assignee | Note |
|:------:|-----|------|----------|------|
| ✅ | BUG-01 | Provider Selection UI | **CLAUDE 2** | preferredProvider + clickable cards |
| ✅ | BUG-02 | Cost Config UI | **CLAUDE 3** | Form + localStorage done |

### Phase 4: Accessibility — 2/2 ✅ COMPLETE

| Status | ID | Task | Assignee | Note |
|:------:|-----|------|----------|------|
| ✅ | A11Y-01 | Keyboard Navigation | **CLAUDE 4** | Escape handlers added to all modals |
| ✅ | A11Y-02 | ARIA Labels | **CLAUDE 4** | aria-labels added to all icon-only buttons |

### Phase 5: Performance — 1/1 ✅ COMPLETE

| Status | ID | Task | Assignee | Note |
|:------:|-----|------|----------|------|
| ✅ | PERF-01 | Lazy Loading | ALTRO | Dynamic imports done |

### Phase 6: Critical Fixes — 1/1 ✅ COMPLETE

| Status | ID | Task | Assignee | Note |
|:------:|-----|------|----------|------|
| ✅ | MINDMAP-01 | Replace Mermaid with MarkMap | **CLAUDE 3** | MarkMapRenderer implemented, verified |

**Problema**: Mermaid mindmaps tronca il testo (10+ segnalazioni utente)
**Soluzione**: Migrazione a MarkMap (MIT license, Markdown input)
**ADR**: `docs/adr/0001-markmap-for-mindmaps.md`
**Verifica**: lint ✅ | typecheck ✅ | build ✅

---

## 📋 TASK DETTAGLIATI PER CLAUDE

---

## CLAUDE 1: COORDINATORE

### Responsabilità
1. **Monitoraggio Piano**: Controlla periodicamente questo file per aggiornamenti
2. **Verifica Coerenza**: Assicurati che lint/typecheck/build passino sempre
3. **Aggregazione**: Quando tutti i task sono ✅, prepara merge/PR
4. **Supporto**: Se un Claude chiede aiuto, fornisci guidance

### Comandi di Monitoraggio
```bash
# Stato attuale lint/types
npm run lint && npm run typecheck

# Controlla modifiche non committate
git status

# Controlla se altri hanno pushato
git fetch && git log HEAD..origin/feature/web-app --oneline
```

### Checklist Pre-Merge (quando tutto è ✅)
- [ ] Tutti i task nel tracker sono ✅
- [ ] `npm run lint` = 0 errors, 0 warnings
- [ ] `npm run typecheck` = no errors
- [ ] `npm run build` = success
- [ ] Security check: `curl -s http://localhost:3000/api/realtime/token | grep -q "apiKey"` = FAIL (no leak)
- [ ] Console.log check: `grep -r "console\.log" src --include="*.ts" --include="*.tsx" | wc -l` = 0

---

## CLAUDE 2: BUG-01 - Provider Selection UI

### Obiettivo
Permettere all'utente di selezionare manualmente il provider AI (Azure/Ollama) invece di usare solo la logica automatica.

### File da leggere PRIMA
```bash
cat src/lib/ai/providers.ts
cat src/lib/stores/app-store.ts
cat src/components/settings/settings-view.tsx
```

### Problema attuale
- `getActiveProvider()` in providers.ts seleziona SEMPRE Azure se configurato
- L'utente NON può forzare Ollama anche se vuole
- settings-view.tsx mostra solo lo stato, nessun toggle per selezionare

### Azioni richieste

**1. Modifica `src/lib/stores/app-store.ts`**:
Aggiungi nuovo stato:
```typescript
// Nella sezione settings
preferredProvider: 'azure' | 'ollama' | 'auto',
setPreferredProvider: (provider: 'azure' | 'ollama' | 'auto') => void,
```

**2. Modifica `src/lib/ai/providers.ts`**:
```typescript
export function getActiveProvider(preference?: 'azure' | 'ollama' | 'auto'): ProviderConfig | null {
  // Se preferenza esplicita per ollama, usa ollama
  if (preference === 'ollama') {
    return getOllamaConfig();
  }
  // Se preferenza esplicita per azure E azure configurato
  if (preference === 'azure' && isAzureConfigured()) {
    return getAzureConfig();
  }
  // auto: logica esistente (Azure se configurato, altrimenti Ollama)
  // ... resto del codice esistente
}
```

**3. Modifica `src/components/settings/settings-view.tsx`**:
Nella sezione Provider (circa linea 954-1034), aggiungi onClick ai Card:
```typescript
<Card
  className={cn("cursor-pointer", preferredProvider === 'azure' && "ring-2 ring-blue-500")}
  onClick={() => setPreferredProvider('azure')}
>
```

### Verifica OBBLIGATORIA
```bash
npm run lint        # 0 errors, 0 warnings
npm run typecheck   # no errors
npm run build       # success
```

### Quando hai finito
Aggiorna questo file: cambia `⬜ | BUG-01` in `✅ | BUG-01` nella tabella sopra.

---

## CLAUDE 3: BUG-02 - Cost Config UI Form

### Obiettivo
Aggiungere un form per configurare Azure Cost Management da UI invece di richiedere modifica .env.

### File da leggere PRIMA
```bash
cat src/components/settings/settings-view.tsx
cat src/app/api/azure/costs/route.ts
```

### Problema attuale
- L'API richiede 4 env vars: AZURE_TENANT_ID, AZURE_CLIENT_ID, AZURE_CLIENT_SECRET, AZURE_SUBSCRIPTION_ID
- Se mancano, mostra solo un messaggio "configura .env"
- Nessun form per inserirle da UI

### Azioni richieste

**1. Aggiungi stato in settings-view.tsx** (circa linea 50):
```typescript
const [azureCostConfig, setAzureCostConfig] = useState({
  tenantId: '',
  clientId: '',
  clientSecret: '',
  subscriptionId: '',
});
const [savingCostConfig, setSavingCostConfig] = useState(false);
```

**2. Aggiungi funzione per salvare** (usa localStorage per ora):
```typescript
const saveCostConfig = async () => {
  setSavingCostConfig(true);
  try {
    localStorage.setItem('azure_cost_config', JSON.stringify(azureCostConfig));
    // Ricarica i costi
    setCostsConfigured(true);
  } finally {
    setSavingCostConfig(false);
  }
};
```

**3. Aggiungi form UI** nella sezione "Cost Management non configurato" (circa linea 1143):
```typescript
{!costsConfigured && (
  <div className="space-y-4 p-4 bg-slate-50 dark:bg-slate-800 rounded-lg">
    <p className="text-sm text-slate-600">Configura Azure Cost Management:</p>
    <Input
      placeholder="AZURE_TENANT_ID"
      value={azureCostConfig.tenantId}
      onChange={(e) => setAzureCostConfig(prev => ({...prev, tenantId: e.target.value}))}
    />
    <Input
      placeholder="AZURE_CLIENT_ID"
      value={azureCostConfig.clientId}
      onChange={(e) => setAzureCostConfig(prev => ({...prev, clientId: e.target.value}))}
    />
    <Input
      type="password"
      placeholder="AZURE_CLIENT_SECRET"
      value={azureCostConfig.clientSecret}
      onChange={(e) => setAzureCostConfig(prev => ({...prev, clientSecret: e.target.value}))}
    />
    <Input
      placeholder="AZURE_SUBSCRIPTION_ID"
      value={azureCostConfig.subscriptionId}
      onChange={(e) => setAzureCostConfig(prev => ({...prev, subscriptionId: e.target.value}))}
    />
    <Button onClick={saveCostConfig} disabled={savingCostConfig}>
      {savingCostConfig ? 'Salvataggio...' : 'Salva Configurazione'}
    </Button>
  </div>
)}
```

**4. Modifica api/azure/costs/route.ts** per leggere da header o cookie:
(Opzionale - se hai tempo, altrimenti localStorage è sufficiente per MVP)

### Verifica OBBLIGATORIA
```bash
npm run lint        # 0 errors, 0 warnings
npm run typecheck   # no errors
npm run build       # success
```

### Quando hai finito
Aggiorna questo file: cambia `⬜ | BUG-02` in `✅ | BUG-02` nella tabella sopra.

---

## CLAUDE 4: A11Y-01 & A11Y-02 - Accessibility

### A11Y-01: Keyboard Navigation

### Obiettivo
Aggiungere gestione tasto Escape per chiudere tutti i modali e migliorare focus management.

### File da cercare
```bash
grep -r "fixed inset-0" src/components/ --include="*.tsx" -l
```

### Azioni richieste

**Per OGNI file con modal (fixed inset-0), aggiungi questo useEffect:**
```typescript
// Handle Escape key to close modal
useEffect(() => {
  const handleEscape = (e: KeyboardEvent) => {
    if (e.key === 'Escape') {
      onClose(); // o la funzione di chiusura del componente
    }
  };
  window.addEventListener('keydown', handleEscape);
  return () => window.removeEventListener('keydown', handleEscape);
}, [onClose]);
```

**File probabili da modificare:**
- src/components/voice/voice-session.tsx (se non già fatto)
- src/components/voice/session-grade.tsx (se non già fatto)
- src/components/tools/webcam-capture.tsx
- src/components/education/html-preview.tsx (quando fullscreen)

**Verifica che ogni Button abbia focus ring visibile:**
```typescript
className="... focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
```

### A11Y-02: ARIA Labels Audit

**Dopo A11Y-01**, verifica che:
1. Tutti i button icon-only abbiano `aria-label`
2. I form input abbiano `aria-label` o `aria-labelledby`

```bash
# Cerca button senza aria-label
grep -r "<Button" src/components/ --include="*.tsx" | grep -v "aria-label"
```

### Verifica OBBLIGATORIA
```bash
npm run lint        # 0 errors, 0 warnings
npm run typecheck   # no errors
npm run build       # success
```

### Quando hai finito
Aggiorna questo file:
- cambia `⬜ | A11Y-01` in `✅ | A11Y-01`
- cambia `⬜ | A11Y-02` in `✅ | A11Y-02`

---

## 📊 PROGRESS SUMMARY

| Category | Done | Total | Status |
|----------|:----:|:-----:|--------|
| Security | 7 | 7 | ✅ COMPLETE |
| Code Quality | 3 | 3 | ✅ COMPLETE |
| Missing Features | 2 | 2 | ✅ COMPLETE |
| Accessibility | 2 | 2 | ✅ COMPLETE |
| Performance | 1 | 1 | ✅ COMPLETE |
| Critical Fixes | 1 | 1 | ✅ COMPLETE |
| **TOTAL** | **16** | **16** | **100%** |

---

## VERIFICATION CHECKLIST (Prima del merge)

```bash
# Tutti questi DEVONO passare:
npm run lint        # 0 errors, 0 warnings
npm run typecheck   # no errors
npm run build       # success

# Security check
curl -s http://localhost:3000/api/realtime/token | grep -q "apiKey" && echo "FAIL: API key exposed" || echo "PASS"

# Console.log check
grep -r "console\.log" src --include="*.ts" --include="*.tsx" | wc -l
# Deve essere 0
```

---

**Versione**: 6.0 (ALL 16 TASKS COMPLETE - MINDMAP MIGRATED TO MARKMAP)
**Ultimo aggiornamento**: 2025-12-28 by CLAUDE 3 (IMPLEMENTER)
