// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Agent command dispatch + transpile logic — extracted from cli_agent.rs (Plan F, T4-02).

use crate::cli_agent::AgentCommands;
use crate::cli_error::CliError;

pub(crate) async fn dispatch(cmd: AgentCommands) -> Result<(), CliError> {
    match cmd {
        AgentCommands::Transpile {
            name,
            provider,
            api_url,
        } => {
            handle_transpile(&name, &provider, &api_url).await?;
        }
        AgentCommands::Start {
            name: _,
            task_id: _,
            human: _,
            api_url: _,
        } => {
            eprintln!("Not implemented — planned for future release");
        }
        AgentCommands::Complete {
            agent_id: _,
            summary: _,
            human: _,
            api_url: _,
        } => {
            eprintln!("Not implemented — planned for future release");
        }
        AgentCommands::List { human, api_url } => {
            crate::cli_http::fetch_and_print(&format!("{api_url}/api/agents/catalog"), human)
                .await?;
        }
        AgentCommands::Sync { .. } => {
            eprintln!("Not implemented — planned for future release");
        }
        AgentCommands::Enable { .. } => {
            eprintln!("Not implemented — planned for future release");
        }
        AgentCommands::Disable { .. } => {
            eprintln!("Not implemented — planned for future release");
        }
        AgentCommands::Catalog {
            category,
            human,
            api_url,
        } => {
            let url = if let Some(cat) = category {
                format!("{api_url}/api/agents/catalog?category={cat}")
            } else {
                format!("{api_url}/api/agents/catalog")
            };
            crate::cli_http::fetch_and_print(&url, human).await?;
        }
        AgentCommands::Triage {
            description,
            domain,
            human,
            api_url,
        } => {
            handle_triage(&description, domain.as_deref(), human, &api_url).await?;
        }
        AgentCommands::History { .. } => {
            eprintln!("Not implemented — planned for future release");
        }
        AgentCommands::Spawn {
            name,
            task,
            human,
            api_url,
        } => {
            let body = serde_json::json!({
                "agent_name": name,
                "org_id": "convergio",
                "task_id": task,
                "capabilities": [],
                "budget_usd": 0.0,
                "priority": 0,
            });
            crate::cli_http::post_and_print(&format!("{api_url}/api/agents/spawn"), &body, human)
                .await?;
        }
        AgentCommands::Create {
            name,
            category,
            description,
            model,
            human,
            api_url,
        } => {
            let body = serde_json::json!({
                "name": name, "category": category,
                "description": description, "model": model,
            });
            crate::cli_http::post_and_print(&format!("{api_url}/api/agents/catalog"), &body, human)
                .await?;
        }
    }
    Ok(())
}

async fn handle_transpile(name: &str, provider: &str, api_url: &str) -> Result<(), CliError> {
    let enc: String = name
        .chars()
        .map(|c| {
            if c.is_ascii_alphanumeric() || c == '-' || c == '_' || c == '.' {
                c.to_string()
            } else {
                format!("%{:02X}", c as u32)
            }
        })
        .collect();
    let url = format!("{api_url}/api/agents/catalog?name={enc}");
    let resp = reqwest::get(&url)
        .await
        .map_err(|e| CliError::ApiCallFailed(format!("error connecting to daemon: {e}")))?;
    let val: serde_json::Value = resp
        .json()
        .await
        .map_err(|e| CliError::ApiCallFailed(format!("error parsing response: {e}")))?;
    let agent = if val.is_array() {
        val.as_array().and_then(|a| a.first()).cloned()
    } else {
        Some(val)
    };
    let agent =
        agent.ok_or_else(|| CliError::NotFound(format!("agent '{name}' not found in catalog")))?;
    let desc = agent["description"].as_str().unwrap_or("");
    let model = agent["model"].as_str().unwrap_or("claude-sonnet-4-6");
    let tools = agent["tools"].as_str().unwrap_or("view,edit,bash");
    let output = match provider {
        "claude-code" => crate::transpiler::transpile_claude_code(name, desc, model, tools),
        "copilot-cli" => crate::transpiler::transpile_copilot_cli(name, desc, model, tools),
        "generic-llm" => crate::transpiler::transpile_generic_llm(name, desc, model),
        other => {
            return Err(CliError::InvalidInput(format!(
                "unknown provider '{other}'; use: claude-code, copilot-cli, generic-llm"
            )));
        }
    };
    print!("{output}");
    Ok(())
}

async fn handle_triage(
    description: &str,
    domain: Option<&str>,
    human: bool,
    api_url: &str,
) -> Result<(), CliError> {
    let url = format!("{api_url}/api/agents/catalog");
    let data = crate::cli_http::get_json_or_default(&url, serde_json::json!([])).await;
    let agents = match data.as_array() {
        Some(arr) => arr,
        None => {
            eprintln!("error: could not fetch agent catalog");
            return Ok(());
        }
    };

    let keywords: Vec<String> = description
        .to_lowercase()
        .split_whitespace()
        .filter(|w| w.len() > 2)
        .map(|w| w.to_string())
        .collect();

    let mut scored: Vec<(f64, &serde_json::Value)> = agents
        .iter()
        .filter_map(|agent| {
            let name = agent.get("name")?.as_str()?;
            let role = agent.get("role").and_then(|v| v.as_str()).unwrap_or("");
            let category = agent.get("category").and_then(|v| v.as_str()).unwrap_or("");
            let caps: Vec<String> = agent
                .get("capabilities")
                .and_then(|v| v.as_array())
                .map(|arr| {
                    arr.iter()
                        .filter_map(|c| c.as_str().map(|s| s.to_lowercase()))
                        .collect()
                })
                .unwrap_or_default();

            if let Some(d) = domain {
                let d_lower = d.to_lowercase();
                if !category.to_lowercase().contains(&d_lower) {
                    return None;
                }
            }

            let haystack = format!("{name} {role} {}", caps.join(" ")).to_lowercase();
            let score: f64 = keywords
                .iter()
                .filter(|kw| haystack.contains(kw.as_str()))
                .count() as f64;

            if score > 0.0 {
                Some((score, agent))
            } else {
                None
            }
        })
        .collect();

    scored.sort_by(|a, b| b.0.partial_cmp(&a.0).unwrap_or(std::cmp::Ordering::Equal));
    let top: Vec<_> = scored.into_iter().take(5).collect();

    if top.is_empty() {
        println!("No matching agents found for: {description}");
        return Ok(());
    }

    if human {
        println!("\x1b[1mAgent triage results:\x1b[0m");
        for (score, agent) in &top {
            let name = agent["name"].as_str().unwrap_or("?");
            let role = agent["role"].as_str().unwrap_or("?");
            println!("  \x1b[36m{name}\x1b[0m (score: {score:.0}) — {role}");
        }
    } else {
        let results: Vec<serde_json::Value> = top
            .iter()
            .map(|(score, agent)| {
                serde_json::json!({
                    "name": agent["name"],
                    "role": agent["role"],
                    "score": score,
                    "category": agent["category"],
                })
            })
            .collect();
        crate::cli_http::print_value(&serde_json::json!({"matches": results}), false);
    }

    Ok(())
}

/// Score an agent against a keyword list. Exported for testing.
#[cfg(test)]
fn score_agent_match(name: &str, role: &str, capabilities: &[String], keywords: &[String]) -> f64 {
    let haystack = format!(
        "{} {} {}",
        name.to_lowercase(),
        role.to_lowercase(),
        capabilities
            .iter()
            .map(|c| c.to_lowercase())
            .collect::<Vec<_>>()
            .join(" ")
    );
    keywords
        .iter()
        .filter(|kw| haystack.contains(kw.as_str()))
        .count() as f64
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn score_agent_match_basic() {
        let score = score_agent_match(
            "baccio-tech-architect",
            "Technical architect",
            &["architecture".into(), "design".into(), "code review".into()],
            &["code".into(), "review".into(), "architecture".into()],
        );
        assert!(score >= 2.0, "expected at least 2 matches, got {score}");
    }

    #[test]
    fn score_agent_match_no_match() {
        let score = score_agent_match(
            "sara-ux-ui-designer",
            "UX/UI designer",
            &["design".into(), "wireframe".into()],
            &["python".into(), "deployment".into()],
        );
        assert_eq!(score, 0.0);
    }

    #[test]
    fn score_agent_match_partial() {
        let score = score_agent_match(
            "marco-devops-engineer",
            "DevOps engineer",
            &[
                "deployment".into(),
                "infrastructure".into(),
                "monitoring".into(),
            ],
            &["deployment".into(), "python".into()],
        );
        assert_eq!(score, 1.0);
    }
}
