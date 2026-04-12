//! Preflight check implementations — GET-based endpoint probes.

use crate::cli_http::get_and_return;
use crate::cli_preflight::Report;
use serde_json::Value;

// ── helpers ──────────────────────────────────────────────────────────

pub(crate) async fn probe(url: &str) -> Result<Value, String> {
    get_and_return(url).await.map_err(|c| format!("HTTP {c}"))
}

async fn check_ep(api: &str, path: &str, name: &str, r: &mut Report) {
    match probe(&format!("{api}{path}")).await {
        Ok(_) => r.pass(name),
        Err(e) => r.fail(name, &e),
    }
}

async fn check_soft(api: &str, path: &str, name: &str, r: &mut Report) {
    match probe(&format!("{api}{path}")).await {
        Ok(_) => r.pass(name),
        Err(_) => r.skip(name, "not configured"),
    }
}

// ── health ───────────────────────────────────────────────────────────

pub(crate) async fn check_health(api: &str, r: &mut Report) {
    match probe(&format!("{api}/api/health")).await {
        Ok(j) if j["status"] == "ok" => r.pass("health"),
        Ok(j) => r.fail("health", &format!("status={}", j["status"])),
        Err(e) => r.fail("health", &e),
    }
    for (n, p) in [
        ("health/deep", "/api/health/deep"),
        ("metrics", "/api/metrics"),
        ("telemetry", "/api/telemetry"),
        ("capabilities", "/api/capabilities"),
        ("depgraph", "/api/depgraph"),
    ] {
        check_ep(api, p, n, r).await;
    }
}

// ── orgs ─────────────────────────────────────────────────────────────

pub(crate) async fn check_org_endpoints(api: &str, r: &mut Report) {
    let orgs = match probe(&format!("{api}/api/orgs")).await {
        Ok(j) if j["orgs"].is_array() => {
            r.pass("orgs/list");
            j
        }
        Ok(j) => {
            r.fail("orgs/list", &format!("no orgs array: {j}"));
            return;
        }
        Err(e) => {
            r.fail("orgs/list", &e);
            return;
        }
    };

    let ids: Vec<String> = orgs["orgs"]
        .as_array()
        .unwrap_or(&vec![])
        .iter()
        .filter_map(|o| o["id"].as_str().map(String::from))
        .collect();

    if ids.is_empty() {
        r.skip("orgs/detail", "no orgs to test");
        return;
    }
    for id in &ids {
        check_org_detail(api, id, r).await;
    }
}

async fn check_org_detail(api: &str, id: &str, r: &mut Report) {
    let base = format!("{api}/api/orgs/{id}");
    let checks = [
        ("org", base.clone(), "detail"),
        (
            "aggregate",
            format!("{base}/telemetry?period=day"),
            "telemetry",
        ),
        ("digest", format!("{base}/digest"), "digest"),
        ("plans", format!("{base}/plans"), "plans"),
    ];
    for (field, url, suffix) in &checks {
        let tag = format!("orgs/{id}/{suffix}");
        match probe(url).await {
            Ok(j) if j[*field].is_object() || j[*field].is_array() => r.pass(&tag),
            Ok(j) => r.fail(&tag, &format!("missing {field}: {j}")),
            Err(e) => r.fail(&tag, &e),
        }
    }
}

// ── plans + standard ─────────────────────────────────────────────────

pub(crate) async fn check_plan_endpoints(api: &str, r: &mut Report) {
    for (p, n) in [
        ("/api/plan-db/list", "plans/list"),
        ("/api/metrics/summary", "metrics/summary"),
    ] {
        check_ep(api, p, n, r).await;
    }
}

pub(crate) async fn check_standard_endpoints(api: &str, r: &mut Report) {
    let eps = [
        ("agents/catalog", "/api/agents/catalog"),
        ("agents/runtime", "/api/agents/runtime"),
        ("ipc/agents", "/api/ipc/agents"),
        ("ipc/status", "/api/ipc/status"),
        ("ipc/channels", "/api/ipc/channels"),
        ("billing/usage", "/api/billing/usage?org_id=preflight"),
        ("billing/rates", "/api/billing/rates?org_id=preflight"),
        ("observatory/timeline", "/api/observatory/timeline"),
        ("observatory/dashboard", "/api/observatory/dashboard"),
        ("deploy/status", "/api/deploy/status"),
        ("deploy/history", "/api/deploy/history"),
        ("decisions", "/api/decisions"),
        ("prompts", "/api/prompts"),
        ("skills", "/api/skills"),
        ("notify/queue", "/api/notify/queue"),
        ("kernel/status", "/api/kernel/status"),
        ("backup/snapshots", "/api/backup/snapshots"),
        ("scheduler/history", "/api/scheduler/history"),
    ];
    for (name, path) in eps {
        check_ep(api, path, name, r).await;
    }
    check_soft(api, "/api/voice/status", "voice/status", r).await;
}

// ── mesh ─────────────────────────────────────────────────────────────

pub(crate) async fn check_mesh_peers(api: &str, r: &mut Report) {
    let peers = match probe(&format!("{api}/api/mesh/peers")).await {
        Ok(j) => {
            r.pass("mesh/peers");
            j
        }
        Err(e) => {
            r.fail("mesh/peers", &e);
            return;
        }
    };

    let arr = match peers.as_array() {
        Some(a) if !a.is_empty() => a,
        _ => {
            r.skip("mesh/peer-health", "no peers configured");
            return;
        }
    };

    for peer in arr {
        let addr = peer["addr"]
            .as_str()
            .or(peer["url"].as_str())
            .or(peer["peer"].as_str())
            .unwrap_or("unknown");
        let tag = format!("mesh/{addr}/health");
        if let Some(url) = peer["addr"].as_str().or(peer["url"].as_str()) {
            let health = if url.starts_with("http") {
                format!("{url}/api/health")
            } else {
                format!("http://{url}/api/health")
            };
            match probe(&health).await {
                Ok(j) if j["status"] == "ok" => r.pass(&tag),
                Ok(j) => r.fail(&tag, &format!("status={}", j["status"])),
                Err(e) => r.fail(&tag, &e),
            }
        } else {
            r.skip(&tag, "no URL");
        }
    }
}
