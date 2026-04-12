//! CLI commands for night-agents management.
//! Pure HTTP client — calls daemon API at /api/night-agents/*.
//! Supports --human flag for pretty-printed output.

use crate::cli_error::CliError;
use crate::cli_http;
use clap::Subcommand;
use serde_json::json;

#[derive(Debug, Subcommand)]
pub enum NightAgentsCommands {
    /// List all night agent definitions
    List {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Trigger a night agent run manually
    Trigger {
        /// Agent definition ID to trigger
        id: i64,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Show recent night agent runs
    Runs {
        /// Filter by agent definition ID
        #[arg(long)]
        agent_id: Option<i64>,
        /// Limit number of results
        #[arg(long, default_value = "20")]
        limit: u32,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// List tracked projects for night agents
    Projects {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Add a project to night agent tracking
    ProjectAdd {
        /// Project name (e.g. "convergio")
        name: String,
        /// Local path to the project repository
        #[arg(long)]
        path: String,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
}

pub async fn handle(cmd: NightAgentsCommands) -> Result<(), CliError> {
    match cmd {
        NightAgentsCommands::List { human, api_url } => {
            let url = format!("{api_url}/api/night-agents");
            cli_http::fetch_and_print(&url, human).await
        }
        NightAgentsCommands::Trigger { id, human, api_url } => {
            let url = format!("{api_url}/api/night-agents/trigger");
            cli_http::post_and_print(&url, &json!({"agent_def_id": id}), human).await
        }
        NightAgentsCommands::Runs {
            agent_id,
            limit,
            human,
            api_url,
        } => {
            let mut url = format!("{api_url}/api/night-agents/runs?limit={limit}");
            if let Some(aid) = agent_id {
                url.push_str(&format!("&agent_id={aid}"));
            }
            cli_http::fetch_and_print(&url, human).await
        }
        NightAgentsCommands::Projects { human, api_url } => {
            let url = format!("{api_url}/api/night-agents/projects");
            cli_http::fetch_and_print(&url, human).await
        }
        NightAgentsCommands::ProjectAdd {
            name,
            path,
            human,
            api_url,
        } => {
            let url = format!("{api_url}/api/night-agents/projects");
            cli_http::post_and_print(&url, &json!({"name": name, "path": path}), human).await
        }
    }
}
