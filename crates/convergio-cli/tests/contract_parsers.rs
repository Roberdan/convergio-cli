//! Shared source-code parsers for contract tests.
//!
//! Parses CLI URLs, server routes, and MCP tool definitions from source files
//! without any compile-time dependency on those crates.

use std::collections::{BTreeSet, HashSet};
use std::path::Path;

pub fn normalize_path(path: &str) -> String {
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

pub fn read_rs_files(dir: &Path) -> Vec<(String, String)> {
    let mut files = Vec::new();
    if let Ok(entries) = std::fs::read_dir(dir) {
        for entry in entries.flatten() {
            let name = entry.file_name().to_string_lossy().to_string();
            if name.ends_with(".rs") {
                if let Ok(content) = std::fs::read_to_string(entry.path()) {
                    files.push((name, content));
                }
            }
        }
    }
    files
}

/// Extract HTTP API URLs from CLI source files.
pub fn extract_mcp_api_urls(source: &str) -> BTreeSet<String> {
    let mut urls = BTreeSet::new();
    for line in source.lines() {
        if let Some(pos) = line.find("/api/") {
            let slice = &line[pos..];
            let end = slice.find(['"', '\'', ')', ' ']).unwrap_or(slice.len());
            let raw = slice[..end].trim_end_matches([',', ')']);
            if raw.starts_with("/api/") && raw.len() > 5 && !raw.contains("//") {
                urls.insert(normalize_path(raw));
            }
        }
    }
    urls
}

/// Extract HTTP API URLs from CLI source files.
pub fn extract_cli_urls(source: &str) -> HashSet<String> {
    let markers = [
        "fetch_and_print(",
        "post_and_print(",
        "client.get(",
        "client.post(",
        "client.delete(",
        "client.put(",
        "client.patch(",
    ];
    let mut urls = HashSet::new();
    let lines: Vec<&str> = source.lines().collect();
    for (i, line) in lines.iter().enumerate() {
        if line.trim_start().starts_with("//") || line.contains("NOT_IMPL") {
            continue;
        }
        let is_http = markers.iter().any(|m| line.contains(m));
        let next_is_http = lines
            .get(i + 1)
            .map(|l| markers.iter().any(|m| l.contains(m)))
            .unwrap_or(false);
        if !is_http && !next_is_http {
            continue;
        }
        if let Some(pos) = line.find("/api/") {
            let slice = &line[pos..];
            let end = slice.find(['"', '\'', ' ', '?']).unwrap_or(slice.len());
            let raw = slice[..end].trim_end_matches([',', ')']);
            if raw.starts_with("/api/") && raw.len() > 5 && !raw.contains("//") {
                urls.insert(normalize_path(raw));
            }
        }
    }
    urls
}

/// Extract registered server routes from an axum source file.
pub fn extract_routes(source: &str) -> HashSet<String> {
    let mut routes = HashSet::new();
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
        routes.insert(normalize_path(&rest[..q2]));
    }
    routes
}

pub const ROUTE_CRATES: &[&str] = &[
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
    "convergio-evidence/src",
    "convergio-prompts/src",
    "convergio-reports/src",
    "convergio-workspace/src",
    "convergio-backup/src",
    "convergio-longrunning/src",
    "convergio-multitenancy/src",
    "convergio-build/src",
    "convergio-autoresearch/src",
    "convergio-delegation/src",
    "convergio-file-transport/src",
    "convergio-org-package/src",
    "convergio-depgraph/src",
    "convergio-provisioning/src",
    "convergio-scheduler/src",
    "convergio-knowledge/src",
];

pub fn collect_server_routes(crates_dir: &Path) -> HashSet<String> {
    let mut routes = HashSet::new();
    for crate_path in ROUTE_CRATES {
        let dir = crates_dir.join(crate_path);
        if !dir.exists() {
            continue;
        }
        for (_name, content) in read_rs_files(&dir) {
            routes.extend(extract_routes(&content));
        }
    }
    routes
}
