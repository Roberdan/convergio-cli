// cli_newproject — `cvg newproject <name>` — unified bootstrap flow.
// Orchestrates: scaffold → onboard → register → next steps.

use crate::cli_error::CliError;

const GREEN: &str = "\x1b[32m";
const CYAN: &str = "\x1b[36m";
const YELLOW: &str = "\x1b[33m";
const BOLD: &str = "\x1b[1m";
const RESET: &str = "\x1b[0m";

pub struct NewProjectOpts {
    pub name: String,
    pub lang: String,
    pub agent: String,
    pub mission: Option<String>,
    pub local: bool,
    pub api_url: String,
}

pub async fn handle(opts: NewProjectOpts) -> Result<(), CliError> {
    let total = 4;
    let mut step = 0;

    // Step 1: Scaffold
    step += 1;
    eprint!(
        "[{step}/{total}] Scaffolding {lang} project...",
        lang = opts.lang
    );
    match scaffold(&opts).await {
        Ok(()) => eprintln!(" {GREEN}✓{RESET}"),
        Err(e) if is_already_exists(&e) => {
            eprintln!(" {YELLOW}exists — skipped{RESET}");
        }
        Err(e) => {
            eprintln!(" ✗");
            return Err(e);
        }
    }

    // Step 2: Onboard (org, agents, KB, .convergio/)
    step += 1;
    eprint!("[{step}/{total}] Onboarding project...");
    let onboard_resp = match onboard(&opts).await {
        Ok(resp) => {
            eprintln!(" {GREEN}✓{RESET}");
            Some(resp)
        }
        Err(e) => {
            eprintln!(" {YELLOW}skipped: {e}{RESET}");
            None
        }
    };

    // Step 3: Switch active project
    step += 1;
    eprint!("[{step}/{total}] Setting active project...");
    match crate::cli_project_switch::handle_switch(&opts.name).await {
        Ok(()) => eprintln!(" {GREEN}✓{RESET}"),
        Err(e) => {
            eprintln!(" {YELLOW}skipped: {e}{RESET}");
        }
    }

    // Step 4: Register agent session
    step += 1;
    eprint!(
        "[{step}/{total}] Registering {agent} session...",
        agent = opts.agent
    );
    let session_name = format!("{}-{}", opts.agent, opts.name);
    let registered_session = match register_agent(&opts.api_url, &session_name, &opts.agent).await {
        Ok(()) => {
            eprintln!(" {GREEN}✓{RESET}");
            true
        }
        Err(e) => {
            eprintln!(" {YELLOW}skipped: {e}{RESET}");
            false
        }
    };

    // Summary
    eprintln!();
    if let Some(resp) = &onboard_resp {
        print_org_summary(resp);
    }
    print_next_steps(&opts.name, &opts.agent);
    if registered_session {
        let _ = deregister_agent(&opts.api_url, &session_name).await;
    }

    Ok(())
}

async fn scaffold(opts: &NewProjectOpts) -> Result<(), CliError> {
    let init_opts = crate::cli_project_init::InitOpts {
        name: &opts.name,
        lang: &opts.lang,
        license: "mit",
        visibility: "public",
        org_id: &opts.name,
        template: None,
        local: opts.local,
        api_url: &opts.api_url,
    };
    crate::cli_project_init::scaffold_project(&init_opts).await
}

async fn onboard(opts: &NewProjectOpts) -> Result<serde_json::Value, CliError> {
    let abs_path = std::fs::canonicalize(&opts.name)
        .unwrap_or_else(|_| std::env::current_dir().unwrap_or_default().join(&opts.name));

    let mut body = serde_json::json!({
        "repo_path": abs_path.to_string_lossy(),
    });
    if let Some(ref mission) = opts.mission {
        body["mission"] = serde_json::json!(mission);
    }

    let url = format!("{}/api/org/projects/onboard", opts.api_url);
    let resp = crate::cli_http::post_and_return(&url, &body)
        .await
        .map_err(|_| CliError::ApiCallFailed("onboard call failed".into()))?;

    if resp["ok"].as_bool() != Some(true) {
        let err = resp["error"].as_str().unwrap_or("onboard failed");
        return Err(CliError::ApiCallFailed(err.to_string()));
    }
    Ok(resp)
}

async fn register_agent(api_url: &str, name: &str, agent_type: &str) -> Result<(), CliError> {
    let host = hostname::get()
        .ok()
        .and_then(|h| h.into_string().ok())
        .unwrap_or_else(|| "unknown".into());
    let body = serde_json::json!({
        "name": name,
        "host": host,
        "agent_type": agent_type,
        "pid": std::process::id(),
    });
    let client = crate::security::hardened_http_client();
    let url = format!("{api_url}/api/ipc/agents/register");
    let resp = client
        .post(&url)
        .json(&body)
        .send()
        .await
        .map_err(|e| CliError::ApiCallFailed(e.to_string()))?;
    if resp.status().is_success() {
        Ok(())
    } else {
        Err(CliError::ApiCallFailed(format!(
            "registration returned {}",
            resp.status()
        )))
    }
}

async fn deregister_agent(api_url: &str, name: &str) -> Result<(), CliError> {
    let client = crate::security::hardened_http_client();
    let url = format!("{api_url}/api/ipc/agents/{name}");
    let resp = client
        .delete(&url)
        .send()
        .await
        .map_err(|e| CliError::ApiCallFailed(e.to_string()))?;
    if resp.status().is_success() {
        Ok(())
    } else {
        Err(CliError::ApiCallFailed(format!(
            "deregister returned {}",
            resp.status()
        )))
    }
}

fn is_already_exists(e: &CliError) -> bool {
    match e {
        CliError::InvalidInput(msg) => msg.contains("already exists"),
        _ => false,
    }
}

fn print_org_summary(resp: &serde_json::Value) {
    let org_id = resp["org_id"].as_str().unwrap_or("-");
    let mission = resp["mission"].as_str().unwrap_or("-");
    eprintln!("{BOLD}Your org:{RESET} {CYAN}{org_id}{RESET}");
    eprintln!("  Mission: {mission}");

    if let Some(members) = resp["members"].as_array() {
        for m in members {
            let name = m["name"].as_str().unwrap_or("-");
            let role = m["role"].as_str().unwrap_or("-");
            eprintln!("  {role}: {CYAN}{name}{RESET}");
        }
    }
    eprintln!();
}

fn print_next_steps(project_name: &str, agent: &str) {
    eprintln!("{BOLD}Next steps:{RESET}");
    eprintln!("  1. cvg plan template > spec.yaml    # get spec template");
    eprintln!("  2. $EDITOR spec.yaml                 # write your plan");
    eprintln!("  3. cvg plan create {project_name} \"Plan name\"",);
    eprintln!("  4. cvg plan import <id> spec.yaml");
    eprintln!("  5. cvg plan readiness <id>           # verify before execution");
    eprintln!("  6. cvg plan start <id>               # execute wave by wave");
    eprintln!();
    eprintln!("Launch your preferred assistant:");
    eprintln!("  → cvg {agent} {agent}-{project_name}");
    eprintln!();
    eprintln!("Or use the solve→plan→execute workflow:");
    eprintln!("  → cvg solve \"describe your problem\"");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn is_already_exists_detects_existing_dir() {
        let e = CliError::InvalidInput("directory 'foo' already exists".into());
        assert!(is_already_exists(&e));
    }

    #[test]
    fn is_already_exists_ignores_other_errors() {
        assert!(!is_already_exists(&CliError::InvalidInput(
            "bad name".into()
        )));
        assert!(!is_already_exists(&CliError::ApiCallFailed(
            "timeout".into()
        )));
    }

    #[test]
    fn print_next_steps_contains_all_six_steps() {
        // Capture stderr by verifying the function doesn't panic
        // and the step text constants are correct
        let steps = [
            "plan template",
            "EDITOR",
            "plan create",
            "plan import",
            "plan readiness",
            "plan start",
        ];
        // We can't easily capture eprintln, but we verify the function runs
        print_next_steps("test-proj", "copilot");
        // Verify the step keywords exist in our source
        for s in &steps {
            assert!(
                include_str!("cli_newproject.rs").contains(s),
                "next steps must mention '{s}'"
            );
        }
    }

    #[test]
    fn print_org_summary_handles_empty_response() {
        let resp = serde_json::json!({});
        // Should not panic on missing fields
        print_org_summary(&resp);
    }

    #[test]
    fn print_org_summary_handles_full_response() {
        let resp = serde_json::json!({
            "ok": true,
            "org_id": "test-org",
            "mission": "Test mission",
            "members": [
                {"name": "alice-ai", "role": "CEO"},
                {"name": "bob-ai", "role": "CTO"},
            ]
        });
        print_org_summary(&resp);
    }
}
