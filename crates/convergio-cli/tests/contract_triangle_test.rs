//! Contract triangle test: CLI ↔ API ↔ MCP alignment.
//!
//! Verifies all three layers reference the same API surface:
//! 1. Every MCP tool defined in mcp_defs.rs files maps to a server route
//! 2. CLI and MCP call the same normalized API paths for shared endpoints
//!
//! Post-rmcp migration: tools are defined via declarative ToolDef/McpToolDef
//! structs (not hand-coded handlers), so we parse mcp_defs.rs files for paths.

mod contract_parsers;

use std::collections::{BTreeSet, HashSet};
use std::path::Path;

use contract_parsers::*;

fn crates_dir() -> &'static Path {
    Path::new(env!("CARGO_MANIFEST_DIR")).parent().unwrap()
}

fn mcp_src() -> std::path::PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .unwrap()
        .join("convergio-mcp/src")
}

/// Collect all API paths from mcp_defs.rs files across all crates.
fn collect_mcp_api_paths() -> BTreeSet<String> {
    let mut paths = BTreeSet::new();
    let crates = crates_dir();
    // Check all crate directories for mcp_defs.rs
    if let Ok(entries) = std::fs::read_dir(crates) {
        for entry in entries.flatten() {
            let defs_file = entry.path().join("src/mcp_defs.rs");
            if defs_file.exists() {
                if let Ok(content) = std::fs::read_to_string(&defs_file) {
                    paths.extend(extract_mcp_api_urls(&content));
                }
            }
        }
    }
    // Also check registry_defs in convergio-mcp for synthetic tools
    let registry_defs_dir = mcp_src().join("registry_defs");
    if registry_defs_dir.exists() {
        for (_name, content) in read_rs_files(&registry_defs_dir) {
            paths.extend(extract_mcp_api_urls(&content));
        }
    }
    paths
}

#[test]
fn triangle_leg1_mcp_tools_map_to_server_routes() {
    let mcp_paths = collect_mcp_api_paths();
    let server_routes = collect_server_routes(crates_dir());

    // Known paths that exist but aren't found by the source parser
    // (either synthetic, in comments, or registered dynamically in router.rs)
    let known_synthetic: HashSet<&str> = [
        "/api/notify",
        "/api/node/recover",
        "/api/node/readiness",
        "/api/agent/interrupt",
        "/api/plan-db/agent/start",
        "/api/plan-db/agent/complete",
        "/api/doctor/run", // registered in doctor ext, not via .route() pattern
        "/api/meta/mcp-tools`.", // parsed from comment with trailing chars
    ]
    .into_iter()
    .collect();

    let missing: Vec<_> = mcp_paths
        .iter()
        .filter(|u| !server_routes.contains(u.as_str()) && !known_synthetic.contains(u.as_str()))
        .collect();

    assert!(
        missing.is_empty(),
        "MCP tool paths missing from server routes:\n  {}",
        missing
            .iter()
            .map(|s| s.as_str())
            .collect::<Vec<_>>()
            .join("\n  ")
    );
}

#[test]
fn triangle_leg2_cli_and_mcp_shared_endpoints_exist() {
    let mcp_paths = collect_mcp_api_paths();

    let cli_src = Path::new(env!("CARGO_MANIFEST_DIR")).join("src");
    let mut cli_all = HashSet::new();
    for (_name, content) in read_rs_files(&cli_src) {
        cli_all.extend(extract_cli_urls(&content));
    }
    let server_routes = collect_server_routes(crates_dir());

    let shared: BTreeSet<_> = mcp_paths
        .iter()
        .filter(|u| cli_all.contains(u.as_str()))
        .cloned()
        .collect();

    let broken: Vec<_> = shared
        .iter()
        .filter(|u| !server_routes.contains(u.as_str()))
        .collect();

    assert!(
        broken.is_empty(),
        "CLI+MCP shared endpoints missing server route:\n  {}",
        broken
            .iter()
            .map(|s| s.as_str())
            .collect::<Vec<_>>()
            .join("\n  ")
    );

    eprintln!(
        "Triangle: CLI={} MCP={} shared={} server={}",
        cli_all.len(),
        mcp_paths.len(),
        shared.len(),
        server_routes.len()
    );
}
