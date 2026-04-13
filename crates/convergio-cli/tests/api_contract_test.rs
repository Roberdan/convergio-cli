//! CLI-API contract test — verifies every CLI HTTP endpoint exists in server routes.
//! Scans cli_*.rs for URL patterns and routes.rs for registered routes.
use std::collections::HashSet;
use std::path::Path;

/// Extract `/api/...` URL patterns from live HTTP calls in source.
/// Only matches lines with actual HTTP client calls, not comments or gated code.
fn extract_urls(source: &str) -> Vec<String> {
    let http_markers = [
        "fetch_and_print(",
        "post_and_print(",
        "client.get(",
        "client.post(",
        "client.delete(",
        "client.put(",
        "client.patch(",
    ];
    let mut urls = Vec::new();
    let lines: Vec<&str> = source.lines().collect();
    for (i, line) in lines.iter().enumerate() {
        // Skip test modules, comments, and gated code
        if line.trim_start().starts_with("//") || line.contains("NOT_IMPL") {
            continue;
        }
        // Check if this line or the next makes an HTTP call
        let is_http = http_markers.iter().any(|m| line.contains(m));
        let next_is_http = lines
            .get(i + 1)
            .map(|l| http_markers.iter().any(|m| l.contains(m)))
            .unwrap_or(false);
        if !is_http && !next_is_http {
            continue;
        }
        // Extract /api/ URL from format string
        if let Some(url) = extract_api_url(line) {
            urls.push(normalize_path(&url));
        }
    }
    urls
}

fn extract_api_url(line: &str) -> Option<String> {
    let pos = line.find("/api/")?;
    let slice = &line[pos..];
    let end = slice.find(['"', '\'', ' ', '?']).unwrap_or(slice.len());
    let mut url = slice[..end].trim_end_matches([',', ')']);
    if let Some(bp) = url.rfind('{') {
        let after = &url[bp..];
        if after.starts_with("{qs}") || after.contains('/') {
            url = &url[..bp];
        }
    }
    (url.starts_with("/api/") && !url.contains("//") && url.len() > 5).then(|| url.to_string())
}

/// Extract registered routes from server route files (handles multiline).
fn extract_routes(source: &str) -> Vec<(String, String)> {
    let mut routes = Vec::new();
    let lines: Vec<&str> = source.lines().collect();
    for (i, line) in lines.iter().enumerate() {
        let Some(pos) = line.find(".route(") else {
            continue;
        };
        let after = &line[pos + 7..];
        let search = after.find('"').map(|q| &after[q + 1..]).or_else(|| {
            lines
                .get(i + 1)
                .and_then(|n| n.trim().find('"').map(|q| &n.trim()[q + 1..]))
        });
        let Some(rest) = search else { continue };
        let Some(q2) = rest.find('"') else { continue };
        let path = &rest[..q2];
        let ctx = format!("{}{}", line, lines.get(i + 1).unwrap_or(&""));
        let method = if ctx.contains("get(") {
            "GET"
        } else if ctx.contains("post(") {
            "POST"
        } else if ctx.contains("delete(") {
            "DELETE"
        } else if ctx.contains("put(") {
            "PUT"
        } else {
            "ANY"
        };
        routes.push((normalize_path(path), method.to_string()));
    }
    routes
}

/// Normalize a path: replace :param and {param} segments with :*.
fn normalize_path(path: &str) -> String {
    path.split('/')
        .map(|s| {
            if s.starts_with(':') || (s.starts_with('{') && s.ends_with('}')) {
                ":*"
            } else {
                s
            }
        })
        .collect::<Vec<_>>()
        .join("/")
}

fn read_dir_files(dir: &Path, prefix: &str, suffix: &str) -> Vec<(String, String)> {
    let mut files = Vec::new();
    if let Ok(entries) = std::fs::read_dir(dir) {
        for entry in entries.flatten() {
            let name = entry.file_name().to_string_lossy().to_string();
            if name.starts_with(prefix) && name.ends_with(suffix) {
                if let Ok(content) = std::fs::read_to_string(entry.path()) {
                    files.push((name, content));
                }
            }
        }
    }
    files
}

#[test]
fn cli_endpoints_match_server_routes() {
    let manifest_dir = env!("CARGO_MANIFEST_DIR");
    let cli_src = Path::new(manifest_dir).join("src");
    // 1. Collect all CLI URLs
    let cli_files = read_dir_files(&cli_src, "cli_", ".rs");
    let mut cli_urls: HashSet<String> = HashSet::new();
    for (_name, content) in &cli_files {
        for url in extract_urls(content) {
            cli_urls.insert(url);
        }
    }

    // 2. Collect all server routes from all crates
    let crates_dir = Path::new(manifest_dir).parent().unwrap();
    let mut server_routes: HashSet<String> = HashSet::new();
    let route_crates = [
        "convergio-ipc/src",
        "convergio-orchestrator/src",
        "convergio-agents/src",
        "convergio-agent-runtime/src",
        "convergio-server/src",
        "convergio-kernel/src",
        "convergio-voice/src",
        "convergio-org/src",
        "convergio-mesh/src",
        "convergio-observatory/src",
        "convergio-billing/src",
        "convergio-deploy/src",
        "convergio-inference/src",
        "convergio-night-agents/src",
        "convergio-doctor/src",
        "convergio-reports/src",
    ];
    for crate_path in &route_crates {
        let dir = crates_dir.join(crate_path);
        if !dir.exists() {
            continue;
        }
        let route_files = read_dir_files(&dir, "", ".rs");
        for (_name, content) in &route_files {
            for (path, _method) in extract_routes(content) {
                server_routes.insert(path);
            }
        }
    }

    // 3. Known unmatched — endpoints pending server implementation.
    let known_pending: HashSet<&str> = [
        "/api/build/self",
        "/api/build/status/:*",
        "/api/build/history",
        "/api/build/rollback",
        "/api/capabilities",
        "/api/capabilities/invoke",
        "/api/capabilities/permissions",
        "/api/capabilities/register",
        "/api/ipc/agents/:*",
        "/api/ipc/channels/:*/send",
        "/api/ipc/channels/",
        "/api/kernel/start",
        "/api/kernel/stop",
        "/api/night-agents/", // trailing slash from {id} format param
        "/api/night-agents/defs",
        "/api/night-agents/trigger",
        "/api/reports/generate", // extracted to external repo
        "/api/reports",          // extracted to external repo
        "/api/reports/",         // extracted to external repo
        "/api/voice/start",
        "/api/voice/stop",
        "/api/voice/test",
        "/api/voice/speak",                      // extracted to external repo
        "/api/voice/status",                     // extracted to external repo
        "/api/ipc/agents",                       // extracted to external repo
        "/api/ipc/agents/register",              // extracted to external repo
        "/api/ipc/send",                         // extracted to external repo
        "/api/agents/spawn",                     // extracted to external repo (agent-runtime)
        "/api/agents/catalog",                   // extracted to external repo (agent-runtime)
        "/api/mesh",                             // extracted to external repo (mesh)
        "/api/mesh/peers",                       // extracted to external repo (mesh)
        "/api/notify/queue",                     // extracted to external repo (mesh)
        "/api/orgs",                             // extracted to external repo (org)
        "/api/doctor/history",                   // extracted to external repo (doctor)
        "/api/doctor/version",                   // extracted to external repo (doctor)
        "/api/kernel/agent-ask",                 // extracted to external repo (kernel)
        "/api/metrics/summary",                  // extracted to external repo (observatory)
        "/api/night-agents",                     // extracted to external repo (night-agents)
        "/api/night-agents/memory-lint/summary", // extracted to external repo (night-agents)
        "/api/night-agents/memory-lint/trigger", // extracted to external repo (night-agents)
        "/api/night-agents/projects",            // extracted to external repo (night-agents)
        "/api/night-agents/routing/migrate-all", // extracted to external repo (night-agents)
        "/api/night-agents/routing/stats",       // extracted to external repo (night-agents)
        "/api/plan-db/list",                     // extracted to external repo (orchestrator)
        "/api/chain/overview",                   // daemon chain API (not in CLI crate routes)
        "/api/chain/status",                     // daemon chain API (not in CLI crate routes)
        "/api/chain/bump",                       // daemon chain API (not in CLI crate routes)
    ]
    .into_iter()
    .collect();

    // 4. Check every CLI URL has a server route (minus known pending)
    let mut missing: Vec<String> = Vec::new();
    for url in &cli_urls {
        if !server_routes.contains(url) && !known_pending.contains(url.as_str()) {
            missing.push(url.clone());
        }
    }

    if !missing.is_empty() {
        missing.sort();
        panic!(
            "CLI-API contract: {} missing routes:\n  {}",
            missing.len(),
            missing.join("\n  ")
        );
    }
}

#[test]
fn extract_urls_finds_api_paths() {
    let source = r#"
        crate::cli_http::fetch_and_print(&format!("{api_url}/api/ipc/agents"), human)
        crate::cli_http::post_and_print(&format!("{api_url}/api/plan-db/validate"), &body, human)
    "#;
    let urls = extract_urls(source);
    assert!(urls.contains(&"/api/ipc/agents".to_string()));
    assert!(urls.contains(&"/api/plan-db/validate".to_string()));
}

#[test]
fn extract_urls_skips_not_implemented() {
    let source = r#"
        eprintln!("not implemented /api/fake/route");
    "#;
    let urls = extract_urls(source);
    assert!(urls.is_empty());
}

#[test]
fn extract_routes_finds_registered_routes() {
    let source = ".route(\"/api/ipc/agents\", get(h))\n.route(\"/api/ipc/send\", post(h))";
    let routes = extract_routes(source);
    assert!(routes
        .iter()
        .any(|(p, m)| p == "/api/ipc/agents" && m == "GET"));
    assert!(routes
        .iter()
        .any(|(p, m)| p == "/api/ipc/send" && m == "POST"));
}

#[test]
fn normalize_path_replaces_params() {
    assert_eq!(normalize_path("/api/agents/:name"), "/api/agents/:*");
    assert_eq!(
        normalize_path("/api/doctor/check/{cat}"),
        "/api/doctor/check/:*"
    );
}
