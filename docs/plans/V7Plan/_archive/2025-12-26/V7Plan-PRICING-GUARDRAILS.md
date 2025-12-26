# Pricing Guardrails & Throttling

Goals: keep margins non-negative, enforce limits, define fallback pricing.

## Base Tiers (recap)
- Free: 30 questions/month, 3 agents, local-only when possible.  
- Pro: $6.99/mo, 100 included, $0.01 extra/question.  
- Team: $19.99/mo, 500 included, $0.01 extra/question.  
- Enterprise: contract.

## Hard Limits & Throttling
- Free: stop at 30/month; allow paid upgrade; no overage.  
- Pro: throttle at 3× included (300/month) unless user opts-in; beyond 300 requires consent or upgrade to Team.  
- Team: throttle at 3× included (1500/month) unless contract.  
- Burst controls: per-minute cap to avoid bill shock; deny with friendly message and upsell path.

## Pricing Fallbacks (if margins negative)
- If monthly gross margin per tier < 30% for two consecutive months:  
  - Raise Pro base to $8.99 OR reduce included to 70; pick whichever restores margin ≥ 40%.  
  - Raise Team base to $24.99 OR reduce included to 400.  
- Announce 30 days before change; grandfather existing subscribers for one cycle.

## Budget & Alerts
- Per-tenant budget: default $20/month; alert at 80%, block at 110% unless approved.  
- Global budget: daily and monthly caps; auto-throttle to local/cheaper models when >90% of daily cap.  
- Alert channels: on-call + billing inbox.

## BYOK Handling
- If BYOK present, skip LLM cost charges; still enforce request limits to protect infra.  
- If BYOK missing/invalid, fall back to house keys only within tier limits.

## Education Specifics
- Free education stays free but strictly limited to 30 questions/month and 3 teachers.  
- Paid education tiers must follow same overage $0.01 rule unless subsidized by grant; document subsidy separately.

## Governance
- Pricing and limit changes require approval from Product + Finance/ops.  
- Maintain change log; update docs, FAQs, and in-app copy on every change.

