// cli_project_add — `cvg project add <path>` — onboard a repo via daemon API.

use crate::cli_error::CliError;
use crate::cli_http::post_and_return;

const CYAN: &str = "\x1b[36m";
const GREEN: &str = "\x1b[32m";
const YELLOW: &str = "\x1b[33m";
const RESET: &str = "\x1b[0m";

pub async fn handle_add(repo_path: &str, api_url: &str) -> Result<(), CliError> {
    let abs_path = std::fs::canonicalize(repo_path)
        .map_err(|e| CliError::InvalidInput(format!("invalid path: {e}")))?;

    let body = serde_json::json!({
        "repo_path": abs_path.to_string_lossy(),
    });

    let resp = post_and_return(&format!("{api_url}/api/org/projects/onboard"), &body)
        .await
        .map_err(|_| CliError::ApiCallFailed("onboard call failed".into()))?;

    if resp["ok"].as_bool() != Some(true) {
        let err = resp["error"].as_str().unwrap_or("unknown error");
        return Err(CliError::ApiCallFailed(err.to_string()));
    }

    format_onboard_response(&resp);
    Ok(())
}

pub fn format_onboard_response(resp: &serde_json::Value) {
    let org_id = resp["org_id"].as_str().unwrap_or("-");
    let mission = resp["mission"].as_str().unwrap_or("-");
    println!("{GREEN}✓{RESET} Project onboarded: {CYAN}{org_id}{RESET}");
    println!("  Mission: {mission}");

    if let Some(members) = resp["members"].as_array() {
        println!(
            "\n  {CYAN}{:<30} {:<20} {:<15} {:<20}{RESET}",
            "AGENT", "ROLE", "DEPARTMENT", "MODEL"
        );
        for m in members {
            let name = m["name"].as_str().unwrap_or("-");
            let role = m["role"].as_str().unwrap_or("-");
            let dept = m["department"].as_str().unwrap_or("-");
            let model = m["model"].as_str().unwrap_or("-");
            let model_short = model.split('-').next_back().unwrap_or(model);
            println!(
                "  {:<30} {:<20} {:<15} {:<20}",
                name, role, dept, model_short
            );
        }
    }

    if let Some(night_agents) = resp["night_agents"].as_array() {
        if !night_agents.is_empty() {
            println!("\n  {CYAN}NIGHT AGENTS{RESET}");
            for na in night_agents {
                let name = na["name"].as_str().unwrap_or("-");
                let schedule = na["schedule"].as_str().unwrap_or("-");
                let time = na["time"].as_str().unwrap_or("-");
                println!("  {name} — {schedule} ({time})");
            }
        }
    }

    let dir_ok = resp["convergio_dir"].as_bool().unwrap_or(false);
    if dir_ok {
        println!("\n  {GREEN}✓{RESET} .convergio/ created");
    } else {
        println!("\n  {YELLOW}⚠{RESET} .convergio/ skipped");
    }
}
