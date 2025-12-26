# Model Cost Matrix & Fallback Policy

Purpose: single source for model choices, costs, and failover rules.

## Cost Table (indicative, update with current provider pricing)
- Free tier: GPT-4o-mini (input+output) ≈ $0.001/question.  
- Paid default: GPT-4o ≈ $0.01/question.  
- Anthropic: Claude Haiku ≈ $0.0008-0.0014/question (simple), Claude Sonnet ≈ $0.006-0.01, Claude Opus ≈ $0.02+.  
- Local: MLX/Ollama = $0 (one-time download), but device-dependent.  
- Education EU: Azure OpenAI (regional) same pricing as GPT-4o/mini.

## Selection Policy
1) Privacy/offline required → Local (MLX macOS, Ollama others).  
2) Budget low or quota exceeded → Local first, then GPT-4o-mini/Haiku.  
3) Complexity high → Claude Sonnet/Opus or GPT-4o full.  
4) Platform: macOS prefer MLX; Linux/Windows prefer Ollama.  
5) Education EU → Azure OpenAI regional endpoints.

## Fallbacks
- Provider down/timeouts: retry same provider; on repeated failure, switch to next best in class (local → cloud cheap → cloud quality).  
- Model missing locally: attempt pull (Ollama) with size guard; if insufficient resources, fall back to cloud mini.  
- Cost guard: if tenant budget >90%, force cheapest acceptable (local/mini) until reset.

## Routing Examples
- Simple Q&A, low cost: GPT-4o-mini or Claude Haiku; local if available.  
- Code review or complex reasoning: GPT-4o full or Claude Sonnet; Opus for highest quality.  
- Voice/Web real-time: prefer low-latency local; otherwise GPT-4o-mini streaming.  
- Education default: GPT-4o-mini; upgrade only on teacher opt-in or paid tier.

## Data to Maintain (ops)
- Current $/1M tokens per provider/model.  
- Context limits, rate limits, regions.  
- Latency benchmarks per region.  
- Model compatibility (vision, tools, audio).

## Governance
- Update matrix monthly or on provider price change.  
- Keep changelog; notify pricing/ops when costs change.  
- Test routing after updates (golden tests per scenario).

