//! `cvg night setup` — bootstrap tracked projects and default night agent defs.
//! Pure HTTP client: calls POST /api/night-agents/projects, POST /api/night-agents,
//! POST scan, then verifies via GET endpoints.

use crate::cli_error::CliError;
use crate::cli_http;
use serde_json::json;

/// Default projects when none are specified.
const DEFAULT_PROJECTS: &[&str] = &["convergio", "convergio-frontend", "convergio-design"];

/// Default agent definitions created per project.
const AGENT_DEFS: &[(&str, &str, &str)] = &[
    (
        "daily-report",
        "Generate a daily activity report for the project",
        "0 6 * * *",
    ),
    (
        "pr-monitor",
        "Monitor open PRs, flag stale ones, summarize review status",
        "0 */4 * * *",
    ),
    (
        "dep-updater",
        "Check for outdated dependencies and security advisories",
        "0 3 * * 1",
    ),
    (
        "ci-optimizer",
        "Analyze CI workflows for missing cache, parallelism, and timeout optimizations",
        "0 2 * * 0",
    ),
];

/// Run the full night-agents onboarding sequence.
pub async fn handle_setup(
    api_url: &str,
    projects: &[String],
    base_path: &str,
) -> Result<(), CliError> {
    let names: Vec<&str> = if projects.is_empty() {
        DEFAULT_PROJECTS.to_vec()
    } else {
        projects.iter().map(|s| s.as_str()).collect()
    };

    println!("🌙 Night agents setup — tracking {} projects", names.len());
    println!();

    // Phase 1: create tracked projects
    let mut project_ids: Vec<(String, i64)> = Vec::new();
    for name in &names {
        let repo_path = format!("{base_path}/{name}");
        let remote = format!("https://github.com/AlessandroRoberdan/{name}.git");
        let body = json!({
            "name": name,
            "repo_path": repo_path,
            "remote_url": remote,
        });
        let url = format!("{api_url}/api/night-agents/projects");
        match cli_http::post_and_return(&url, &body).await {
            Ok(val) => {
                let id = val["id"].as_i64().unwrap_or(-1);
                println!("  ✅ Project '{name}' created (id={id})");
                project_ids.push((name.to_string(), id));
            }
            Err(_) => {
                eprintln!("  ⚠️  Project '{name}' — may already exist, continuing");
            }
        }
    }
    println!();

    // Phase 2: trigger scan for each project
    for (name, id) in &project_ids {
        if *id < 0 {
            continue;
        }
        let url = format!("{api_url}/api/night-agents/projects/{id}/scan");
        match cli_http::post_and_return(&url, &json!({})).await {
            Ok(_) => println!("  🔍 Scan triggered for '{name}' (id={id})"),
            Err(_) => eprintln!("  ⚠️  Scan trigger failed for '{name}'"),
        }
    }
    println!();

    // Phase 3: create night agent defs
    let mut def_count = 0u32;
    for (suffix, desc, schedule) in AGENT_DEFS {
        let body = json!({
            "name": *suffix,
            "org_id": "convergio",
            "description": *desc,
            "schedule": *schedule,
            "agent_prompt": format!(
                "You are a night agent. Task: {desc}. \
                 Tracked projects: {project_list}.",
                project_list = names.join(", "),
            ),
            "model": "auto",
            "max_runtime_secs": 3600,
        });
        let url = format!("{api_url}/api/night-agents");
        match cli_http::post_and_return(&url, &body).await {
            Ok(val) => {
                let id = val["id"].as_i64().unwrap_or(-1);
                println!("  🤖 Agent def '{suffix}' created (id={id})");
                def_count += 1;
            }
            Err(_) => {
                eprintln!("  ⚠️  Agent def '{suffix}' — may already exist, skipping");
            }
        }
    }
    println!();

    // Phase 4: verify via GET endpoints
    println!("📋 Verification:");
    verify_projects(api_url).await;
    verify_defs(api_url).await;
    println!();

    println!(
        "✨ Setup complete — {n} projects tracked, {d} agent defs created",
        n = project_ids.len(),
        d = def_count,
    );
    Ok(())
}

async fn verify_projects(api_url: &str) {
    let url = format!("{api_url}/api/night-agents/projects");
    match cli_http::get_and_return(&url).await {
        Ok(val) => {
            if let Some(arr) = val.as_array() {
                println!("  Projects ({}):", arr.len());
                for p in arr {
                    let name = p["name"].as_str().unwrap_or("?");
                    let enabled = p["enabled"].as_bool().unwrap_or(false);
                    let mark = if enabled { "✅" } else { "❌" };
                    println!("    {mark} {name}");
                }
            }
        }
        Err(_) => eprintln!("  ⚠️  Could not verify projects"),
    }
}

async fn verify_defs(api_url: &str) {
    let url = format!("{api_url}/api/night-agents");
    match cli_http::get_and_return(&url).await {
        Ok(val) => {
            if let Some(arr) = val.as_array() {
                println!("  Agent defs ({}):", arr.len());
                for d in arr {
                    let name = d["name"].as_str().unwrap_or("?");
                    let sched = d["schedule"].as_str().unwrap_or("?");
                    let enabled = d["enabled"].as_bool().unwrap_or(false);
                    let mark = if enabled { "✅" } else { "❌" };
                    println!("    {mark} {name} [{sched}]");
                }
            }
        }
        Err(_) => eprintln!("  ⚠️  Could not verify agent defs"),
    }
}
