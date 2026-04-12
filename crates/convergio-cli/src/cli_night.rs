//! CLI commands for night agent management.
//! Pure HTTP client — calls daemon API at /api/night-agents/*.

use crate::cli_error::CliError;
use crate::cli_http;
use crate::cli_night_setup;
use clap::Subcommand;
use serde_json::json;

#[derive(Debug, Subcommand)]
pub enum NightAgentCommands {
    /// List all night agent definitions
    List {
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Trigger a night agent run manually
    Trigger {
        /// Agent definition ID
        id: i64,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Show routing stats (MLX vs cloud usage)
    Routing {
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Set routing model for an agent (auto, mlx:*, claude-*)
    SetModel {
        /// Agent definition ID
        id: i64,
        /// Model: "auto", "mlx:qwen2.5", "claude-haiku-4-5", etc.
        model: String,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Migrate all agents to smart routing (model=auto)
    MigrateAuto {
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Show memory lint findings
    Lint {
        #[arg(long)]
        project: Option<String>,
        #[arg(long)]
        severity: Option<String>,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Trigger memory lint on all projects
    LintRun {
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Show memory lint summary per project
    LintSummary {
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Bootstrap tracked projects and default night agent definitions
    Setup {
        /// Projects to track (default: convergio, convergio-frontend, convergio-design)
        #[arg(long, num_args = 1..)]
        projects: Vec<String>,
        /// Base path for project repos (default: ~/GitHub)
        #[arg(long)]
        base_path: Option<String>,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
}

pub async fn handle(cmd: NightAgentCommands) -> Result<(), CliError> {
    match cmd {
        NightAgentCommands::List { api_url } => {
            let url = format!("{api_url}/api/night-agents");
            cli_http::fetch_and_print(&url, true).await
        }
        NightAgentCommands::Trigger { id, api_url } => {
            let url = format!("{api_url}/api/night-agents/{id}/trigger");
            cli_http::post_and_print(&url, &json!({}), true).await
        }
        NightAgentCommands::Routing { api_url } => {
            let url = format!("{api_url}/api/night-agents/routing/stats");
            cli_http::fetch_and_print(&url, true).await
        }
        NightAgentCommands::SetModel { id, model, api_url } => {
            let url = format!("{api_url}/api/night-agents/{id}/routing");
            cli_http::post_and_print(&url, &json!({"model": model}), true).await
        }
        NightAgentCommands::MigrateAuto { api_url } => {
            let url = format!("{api_url}/api/night-agents/routing/migrate-all");
            cli_http::post_and_print(&url, &json!({}), true).await
        }
        NightAgentCommands::Lint {
            project,
            severity,
            api_url,
        } => {
            let mut url = format!("{api_url}/api/night-agents/memory-lint?");
            if let Some(p) = project {
                url.push_str(&format!("project={p}&"));
            }
            if let Some(s) = severity {
                url.push_str(&format!("severity={s}&"));
            }
            cli_http::fetch_and_print(&url, true).await
        }
        NightAgentCommands::LintRun { api_url } => {
            let url = format!("{api_url}/api/night-agents/memory-lint/trigger");
            cli_http::post_and_print(&url, &json!({}), true).await
        }
        NightAgentCommands::LintSummary { api_url } => {
            let url = format!("{api_url}/api/night-agents/memory-lint/summary");
            cli_http::fetch_and_print(&url, true).await
        }
        NightAgentCommands::Setup {
            projects,
            base_path,
            api_url,
        } => {
            let base = base_path.unwrap_or_else(|| {
                dirs::home_dir()
                    .map(|h| format!("{}/GitHub", h.display()))
                    .unwrap_or_else(|| "~/GitHub".to_string())
            });
            cli_night_setup::handle_setup(&api_url, &projects, &base).await
        }
    }
}
