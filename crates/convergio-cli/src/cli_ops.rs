// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Mesh and Session subcommands for the cvg CLI — delegates to daemon HTTP API via reqwest.
// JSON output by default; --human flag for readable text.

use clap::Subcommand;

#[derive(Debug, Subcommand)]
pub enum MeshCommands {
    /// Send a heartbeat to the mesh (POST /api/heartbeat)
    Heartbeat {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Show current mesh status (GET /api/mesh)
    Status {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Show cluster-level heartbeat status (GET /api/mesh/peers)
    ClusterStatus {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Join a mesh by registering with a coordinator node
    Join {
        /// Coordinator daemon URL (e.g. http://100.89.245.79:8420)
        coordinator_url: String,
    },
}

#[derive(Debug, Subcommand)]
pub enum SessionCommands {
    /// Clean up old/stale sessions
    Reap {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Trigger session recovery
    Recovery {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

#[derive(Debug, Subcommand)]
pub enum MetricsCommands {
    /// Show metrics summary (GET /api/metrics/summary)
    Summary {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Trigger metrics collection (POST /api/metrics/collect)
    Collect {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

#[derive(Debug, Subcommand)]
pub enum AlertCommands {
    /// List active notifications (GET /api/notify/queue)
    List {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

pub async fn handle_mesh(cmd: MeshCommands) {
    match cmd {
        MeshCommands::Heartbeat { human, api_url } => {
            let body = serde_json::json!({
                "peer": hostname::get()
                    .map(|h| h.to_string_lossy().to_string())
                    .unwrap_or_else(|_| "unknown".into()),
                "version": env!("CARGO_PKG_VERSION"),
            });
            let body_bytes = serde_json::to_vec(&body).unwrap_or_default();

            let mut headers = reqwest::header::HeaderMap::new();
            if let Some(sig) = compute_mesh_signature(&body_bytes) {
                if let Ok(val) = reqwest::header::HeaderValue::from_str(&sig) {
                    headers.insert("x-mesh-signature", val);
                }
            }

            let client = reqwest::Client::new();
            let url = format!("{api_url}/api/heartbeat");
            let req = client
                .post(&url)
                .headers(headers)
                .header("Content-Type", "application/json")
                .body(body_bytes);
            let req = if let Ok(token) = std::env::var("CONVERGIO_AUTH_TOKEN") {
                req.header("Authorization", format!("Bearer {token}"))
            } else {
                req
            };
            match req.send().await {
                Ok(resp) => {
                    let val: serde_json::Value = resp.json().await.unwrap_or_default();
                    crate::cli_http::print_value(&val, human);
                }
                Err(e) => eprintln!("error: {e}"),
            }
        }
        MeshCommands::Status { human, api_url } => {
            if let Err(e) =
                crate::cli_http::fetch_and_print(&format!("{api_url}/api/mesh"), human).await
            {
                eprintln!("error: {e}");
            }
        }
        MeshCommands::ClusterStatus { human, api_url } => {
            if let Err(e) =
                crate::cli_http::fetch_and_print(&format!("{api_url}/api/mesh/peers"), human).await
            {
                eprintln!("error: {e}");
            }
        }
        MeshCommands::Join { coordinator_url } => {
            if let Err(e) = crate::cli_mesh_join::handle_mesh_join(&coordinator_url).await {
                eprintln!("error: {e}");
            }
        }
    }
}

pub async fn handle_session(cmd: SessionCommands) {
    match cmd {
        SessionCommands::Reap { .. } => {
            eprintln!("Not implemented — planned for future release");
        }
        SessionCommands::Recovery { .. } => {
            eprintln!("Not implemented — planned for future release");
        }
    }
}

pub async fn handle_metrics(cmd: MetricsCommands) {
    match cmd {
        MetricsCommands::Summary { human, api_url } => {
            if let Err(e) =
                crate::cli_http::fetch_and_print(&format!("{api_url}/api/metrics/summary"), human)
                    .await
            {
                eprintln!("error: {e}");
            }
        }
        MetricsCommands::Collect { .. } => {
            eprintln!("Not implemented — planned for future release");
        }
    }
}

pub async fn handle_alert(cmd: AlertCommands) {
    match cmd {
        AlertCommands::List { human, api_url } => {
            if let Err(e) =
                crate::cli_http::fetch_and_print(&format!("{api_url}/api/notify/queue"), human)
                    .await
            {
                eprintln!("error: {e}");
            }
        }
    }
}

/// Load shared_secret from peers.conf and HMAC-sign the body.
fn compute_mesh_signature(body: &[u8]) -> Option<String> {
    let conf_path = std::env::var("CONVERGIO_PEERS_CONF").unwrap_or_else(|_| {
        let home = std::env::var("HOME").unwrap_or_else(|_| ".".into());
        format!("{home}/.claude/config/peers.conf")
    });
    let text = std::fs::read_to_string(&conf_path).ok()?;
    let secret = text.lines().find_map(|line| {
        let trimmed = line.trim();
        if trimmed.starts_with("shared_secret") {
            trimmed.split('=').nth(1).map(|v| v.trim().to_string())
        } else {
            None
        }
    })?;
    if secret.is_empty() {
        return None;
    }
    use hmac::{Hmac, Mac};
    use sha2::Sha256;
    let mut mac = Hmac::<Sha256>::new_from_slice(secret.as_bytes()).ok()?;
    mac.update(body);
    Some(hex::encode(mac.finalize().into_bytes()))
}

#[cfg(test)]
#[path = "cli_ops_tests.rs"]
mod tests;
