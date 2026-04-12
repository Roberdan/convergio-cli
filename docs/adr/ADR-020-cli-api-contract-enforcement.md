---
version: "1.0"
last_updated: "2026-04-07"
author: "convergio-team"
tags: ["adr"]
---

# ADR-020: CLI-API Contract Enforcement

**Status:** Accepted
**Date:** 2026-04-05
**Deciders:** Roberto D'Angelo

## Context

Session 10.5 audit found 60+ CLI commands calling non-existent server endpoints. Commands compiled, passed unit tests, but silently failed at runtime with 404 errors. Root cause: CLI commands were written speculatively before server endpoints existed, with no validation that the two sides match.

## Decision

Enforce CLI-API contract through three mechanisms:

1. **Static contract test** (`api_contract_test.rs`): scans CLI source for HTTP calls, scans server for route registrations, asserts every CLI endpoint has a matching server route. Runs in CI on every PR.

2. **CONSTITUTION Rule 11**: "Every CLI command MUST have a matching, tested server endpoint before merge. No speculative CLI commands."

3. **Challenger gate** (`challenger.rs`): adversarial reachability audit that verifies every plan output is connected and reachable. Integrated into Thor's validate-completion. Catches orphan waves, limbo tasks, and undelivered outputs.

## Consequences

- New CLI commands require a server route to exist first (contract test fails otherwise)
- Known-pending endpoints tracked in allowlist (shrinks as routes are added)
- Challenger runs automatically on every plan completion — no manual audit needed
- Future plans of any type (code, business, design) get reachability checks
