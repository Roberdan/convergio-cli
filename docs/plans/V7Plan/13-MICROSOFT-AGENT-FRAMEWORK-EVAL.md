# Microsoft Agent Framework vs Convergio Kernel — Brutal Evaluation

**Status:** Draft for approval  
**Date:** 2025-12-26  
**Purpose:** Decide whether adopting Microsoft’s framework accelerates V7 without killing differentiation.

---

## 1) What Problem Are We Solving?

We need:
- fast, reliable orchestration
- tool execution with guardrails
- multi-provider routing (Azure OpenAI, others, local)
- a system that can be executed by AI agents at high speed

---

## 2) Options

### Option A — Convergio kernel as the orchestration runtime (Own the runtime)
- Use Convergio C kernel for orchestration, tool engine, routing.
- Add Rust gateway for SaaS concerns.

### Option B — Microsoft framework for cloud orchestration (Azure-speed)
- Use Microsoft framework for planning/graphs.
- Convergio becomes tool/runtime + local/offline product.

### Option C — Split brain (recommended only if disciplined)
- SaaS uses Microsoft framework.
- Local editions use Convergio kernel.
- Shared contract layer keeps clients consistent.

---

## 3) Decision Matrix (Score 1–5)

| Criterion | A: Convergio | B: Microsoft | C: Split |
|---|---:|---:|---:|
| Time-to-market on Azure | 3 | 5 | 4 |
| Performance control | 5 | 3 | 4 |
| Local/offline story | 5 | 2 | 5 |
| Vendor lock-in risk | 5 | 1 | 3 |
| Differentiation | 5 | 2 | 4 |
| Hiring/skills availability | 3 | 4 | 3 |
| Long-term maintainability | 4 | 4 | 2 |

**Interpretation:** B wins for Azure speed, A wins for moat and control, C can win if contracts are strict.

---

## 4) The Hard Truth

- If Convergio is “just a wrapper” around Microsoft’s orchestration, you will lose differentiation.
- If Convergio insists on building *everything* itself, you risk missing the market window.

---

## 5) Recommended Strategy

**Default recommendation:** Option C *only if* you enforce:
- contract-first API (same client protocol regardless of backend)
- BYOK-first and portable key vault interface
- plugin model (WASM untrusted) identical in both modes

If you cannot enforce strict contracts, choose A.

---

## 6) 1-Week Spike Plan (Proof, not opinions)

Build the same minimal workflow in both A and B:
- WS streaming chat
- one tool call
- metering event

Measure:
- p95 first-token latency
- cost per 1k requests (infra only)
- engineering effort (days)
- portability risk

Record the decision in `01-SSOT-DECISIONS.md`.

