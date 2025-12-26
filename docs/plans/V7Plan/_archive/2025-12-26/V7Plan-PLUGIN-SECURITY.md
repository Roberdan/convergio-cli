# Plugin Security & Review

Goals: sandbox plugins, verify provenance, manage secrets safely.

## Trust Model
- Default: plugins are untrusted.  
- Separate concerns: code integrity (signing), runtime isolation (sandbox), permissioning (least privilege).

## Signing & Provenance
- Require signature for marketplace/official plugins.  
- Signing key held by Convergio release bot; public key distributed with core.  
- Verify signature and checksum before load; fail closed.  
- Record plugin id, version, hash, signer in registry.

## Sandboxing
- Default profile: no network, limited filesystem (whitelist per plugin), CPU/memory quotas.  
- OS isolation: process per plugin + seccomp/App Sandbox (platform-specific).  
- WASM plugins: run in WASM runtime with capability-based host calls.  
- Native plugins: dylib/so, but behind broker process enforcing policy.

## Permissions & Manifest
- Manifest fields: name, version, author, description, required core version, capabilities, permissions (fs paths, network domains), needs_license flag.  
- Deny by default; allowlist per permission.  
- Enforce at load time and per call.

## Secrets Handling
- Plugins never read host env by default.  
- Secrets injected via scoped token or per-plugin vault entry; revokeable.  
- No secrets in logs; redact sensitive fields.

## Review Workflow
- Official/marketplace submission: automated scans (static, SBOM, known vulns), manual review for permissions and behavior.  
- High-risk permissions (network, file write, exec) require human approval.  
- Version updates repeat verification; block downgrade if signature mismatch.

## Runtime Monitoring
- Log plugin loads, denials, crashes.  
- Metrics: failures, timeouts, resource usage.  
- Kill/evict on policy violation; surface to user/admin.

## Backward Compatibility
- Older plugins without manifest: run in strict sandbox with no network and read-only temp fs; prompt to update manifest.

## Governance
- Security exceptions require approval (Security + Eng).  
- Maintain allow/deny lists; publish policy to developers.  
- Annual review of sandbox defaults and permissions catalog.

