# SLO, Observability & Ops Guardrails

Purpose: minimal operations baseline for V7.

## SLO/SLI (initial targets)
- Availability: 99.5% API uptime (SLI: success rate).  
- Latency: P95 HTTP < 800 ms, WebSocket first-token < 1.5 s.  
- Error budget burn: alert if 25% budget consumed in 1 day.

## Logging & Tracing
- Structured JSON logs with request id, user/tenant id, model/provider, cost, latency, error.  
- Log levels: info (start/stop), warn (degraded), error (fail).  
- Correlate with tracing (OpenTelemetry) for API Gateway and core calls.

## Metrics (Prometheus-style)
- Request count/success/error by endpoint and provider.  
- Latency histogram (P50/P95/P99).  
- LLM cost per tenant and per provider.  
- Queue depth and worker utilization.  
- Voice: RTT, reconnects, dropout rate.

## Alerts (initial)
- API error rate > 2% for 5 min.  
- P95 latency > SLO for 10 min.  
- LLM cost overrun: daily spend > 80% of budget.  
- DB errors or replication lag > threshold.  
- Voice: dropout rate > 5% over 10 min.

## Backups & DR
- PostgreSQL: daily snapshots + PITR; test restore monthly.  
- Redis: persistence or reconstructable cache; document strategy.  
- Artifacts: encryption at rest; store backup keys in KMS.  
- DR plan: RPO 24h, RTO 24h initial target.

## Runbooks (skeletons to fill)
- High error rate.  
- Latency regression.  
- Cost overrun / throttling triggered.  
- Voice pipeline broken (WebSocket/RTC).  
- DB outage/restore.  
Each runbook: detection, quick triage, rollback/mitigation steps, comms template.

## Deployment & Rollout
- Blue/green or canary for API Gateway and plugins.  
- Feature flags for risky changes (routing, billing, voice).  
- Post-deploy verification checklist (smoke tests + dashboards green).

## Audit & Compliance
- Access logging for admin actions.  
- Key/secret rotation policy documented.  
- Quarterly review of SLOs and alert thresholds.

