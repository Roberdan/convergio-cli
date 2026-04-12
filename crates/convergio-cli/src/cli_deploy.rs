// `cvg deploy` — upgrade, push-all, status, history.

use crate::cli_error::CliError;
use crate::cli_http;
use clap::Subcommand;

#[derive(Debug, Subcommand)]
pub enum DeployCommands {
    #[command(about = "Upgrade this node to latest (or specific) version")]
    Upgrade {
        #[arg(long)]
        version: Option<String>,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    #[command(about = "Push upgrade to all mesh peers")]
    PushAll {
        #[arg(long)]
        version: String,
        #[arg(long, value_delimiter = ',')]
        peers: Vec<String>,
        #[arg(long, default_value = "rolling")]
        strategy: String,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    #[command(about = "Show current deploy status")]
    Status {
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    #[command(about = "Show upgrade history")]
    History {
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

pub async fn handle(command: DeployCommands) -> Result<(), CliError> {
    match command {
        DeployCommands::Upgrade { version, api_url } => {
            let url = format!("{api_url}/api/deploy/upgrade");
            let body = serde_json::json!({ "version": version });
            match cli_http::post_and_return(&url, &body).await {
                Ok(resp) => {
                    println!(
                        "{}",
                        serde_json::to_string_pretty(&resp).unwrap_or_default()
                    );
                    Ok(())
                }
                Err(_) => Err(CliError::ApiCallFailed("upgrade request failed".into())),
            }
        }
        DeployCommands::PushAll {
            version,
            peers,
            strategy,
            api_url,
        } => {
            let url = format!("{api_url}/api/deploy/push-all");
            let body = serde_json::json!({
                "version": version,
                "peers": peers,
                "strategy": strategy,
            });
            match cli_http::post_and_return(&url, &body).await {
                Ok(resp) => {
                    println!(
                        "{}",
                        serde_json::to_string_pretty(&resp).unwrap_or_default()
                    );
                    Ok(())
                }
                Err(_) => Err(CliError::ApiCallFailed("push-all request failed".into())),
            }
        }
        DeployCommands::Status { api_url } => {
            let url = format!("{api_url}/api/deploy/status");
            match cli_http::get_and_return(&url).await {
                Ok(resp) => {
                    println!(
                        "{}",
                        serde_json::to_string_pretty(&resp).unwrap_or_default()
                    );
                    Ok(())
                }
                Err(_) => Err(CliError::ApiCallFailed("status request failed".into())),
            }
        }
        DeployCommands::History { api_url } => {
            let url = format!("{api_url}/api/deploy/history");
            match cli_http::get_and_return(&url).await {
                Ok(resp) => {
                    println!(
                        "{}",
                        serde_json::to_string_pretty(&resp).unwrap_or_default()
                    );
                    Ok(())
                }
                Err(_) => Err(CliError::ApiCallFailed("history request failed".into())),
            }
        }
    }
}
