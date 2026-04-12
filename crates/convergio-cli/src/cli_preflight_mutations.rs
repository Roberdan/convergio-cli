//! Preflight auth verification and POST mutation smoke tests.

use crate::cli_http::post_and_return;
use crate::cli_preflight::Report;
use crate::cli_preflight_checks::probe;
use serde_json::{json, Value};
use std::time::{SystemTime, UNIX_EPOCH};

async fn probe_post(url: &str, body: &Value) -> Result<Value, String> {
    post_and_return(url, body)
        .await
        .map_err(|c| format!("HTTP {c}"))
}

// ── auth ─────────────────────────────────────────────────────────────

pub(crate) async fn check_auth(api: &str, r: &mut Report) {
    let url = format!("{api}/api/orgs");
    let has_token = std::env::var("CONVERGIO_AUTH_TOKEN")
        .ok()
        .filter(|t| !t.trim().is_empty())
        .is_some();

    let status = match crate::security::hardened_http_client()
        .get(&url)
        .send()
        .await
    {
        Ok(resp) => resp.status().as_u16(),
        Err(_) => {
            r.auth_mode = "unreachable".into();
            r.skip("auth/probe", "daemon unreachable");
            return;
        }
    };

    match (status, has_token) {
        (200, false) => {
            r.auth_mode = "localhost-bypass".into();
            r.pass("auth/mode");
        }
        (200, true) => {
            if probe(&url).await.is_ok() {
                r.auth_mode = "localhost-bypass + token".into();
                r.pass("auth/mode");
            } else {
                r.auth_mode = "bypass (token rejected)".into();
                r.fail("auth/token", "token present but rejected");
            }
        }
        (401, true) => {
            if probe(&url).await.is_ok() {
                r.auth_mode = "token-auth (verified)".into();
                r.pass("auth/mode");
            } else {
                r.auth_mode = "token-auth (rejected)".into();
                r.fail("auth/token", "token invalid");
            }
        }
        (401, false) => {
            r.auth_mode = "token-required (none set)".into();
            r.fail("auth/mode", "401 — set CONVERGIO_AUTH_TOKEN");
        }
        (code, _) => {
            r.auth_mode = format!("unexpected (HTTP {code})");
            r.skip("auth/mode", &format!("HTTP {code}"));
        }
    }
}

// ── mutations ────────────────────────────────────────────────────────

pub(crate) async fn check_mutations(api: &str, r: &mut Report) {
    let ts = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs();
    let org_id = format!("preflight-probe-{ts}");

    // Create org
    let body = json!({
        "id": org_id,
        "name": format!("Preflight Probe {ts}"),
        "description": "Temporary org created by preflight check"
    });
    if let Err(e) = probe_post(&format!("{api}/api/orgs"), &body).await {
        r.fail("mutation/create-org", &e);
        r.mutations_tested = true;
        return;
    }
    r.pass("mutation/create-org");

    // Create plan
    let body = json!({
        "org_id": org_id,
        "name": format!("preflight-plan-{ts}"),
        "description": "Preflight smoke test plan"
    });
    match probe_post(&format!("{api}/api/plan-db/create"), &body).await {
        Ok(_) => r.pass("mutation/create-plan"),
        Err(e) => r.fail("mutation/create-plan", &e),
    }

    // Register IPC agent
    let agent_id = format!("preflight-agent-{ts}");
    let body = json!({
        "agent_id": agent_id,
        "org_id": org_id,
        "capabilities": ["preflight-test"]
    });
    match probe_post(&format!("{api}/api/ipc/agents/register"), &body).await {
        Ok(_) => r.pass("mutation/register-ipc"),
        Err(e) => r.fail("mutation/register-ipc", &e),
    }

    // Best-effort cleanup
    let client = crate::security::hardened_http_client();
    let _ = client
        .delete(format!("{api}/api/orgs/{org_id}/members/preflight-cleanup"))
        .send()
        .await;
    r.mutations_tested = true;
}
