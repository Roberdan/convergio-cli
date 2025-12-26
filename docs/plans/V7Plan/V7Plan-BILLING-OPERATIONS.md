# Billing Operations Playbook

Purpose: operational details for subscriptions, usage, tax, and compliance.

## Subscription Lifecycle
- Create: Stripe customer + subscription; store stripe ids in `subscriptions` table.  
- Update: plan changes at period boundary; pro-rate only if supported.  
- Cancel: cancel_at_period_end; keep service until period end.  
- Trials: optional, time-boxed; convert or auto-cancel.

## Usage & Invoicing
- Track usage per tenant per month (questions, models, costs).  
- Before billing: compute included vs overage; create invoice item with breakdown.  
- Invoicing: auto-finalize and auto-pay; store invoice id and amounts; keep history.  
- Display to user: current usage, forecast, budget threshold, invoices.

## Dunning & Suspension
- On payment failed: retry per Stripe schedule; notify user; grace period configurable (default 3 days).  
- After grace: downgrade to Free or suspend API; keep data accessible read-only.  
- Re-activation: on successful payment, restore plan and limits.

## Tax/IVA
- Use Stripe Tax if available; otherwise configure per region (EU/US).  
- Collect billing address + VAT/IVA where required.  
- Show tax separately on invoice.  
- Keep records for audit.

## GDPR/PCI Checklist
- We never store card data; Stripe handles PCI.  
- Secure webhook with signature verification.  
- Data deletion: remove subscriptions/usage on user delete; keep invoices per legal requirement.  
- Regional routing: use EU endpoints/providers for EU education users.

## Controls & Alerts
- Alert on failed payments, chargebacks, high dispute rate.  
- Alert on invoice generation errors.  
- Budget alerts: per-tenant 80%/100% and global daily cap.

## Support Runbook (high level)
- Payment failed: confirm webhook, inform user, extend grace if needed.  
- Wrong charges: verify usage records vs invoice items; issue credit if error.  
- Refunds: follow policy; process via Stripe, log case.

## Change Management
- Pricing changes: 30-day notice; grandfather for one cycle.  
- Tax changes: update configs, QA invoices, announce if user-facing.  
- Keep billing docs and FAQs updated after every change.

