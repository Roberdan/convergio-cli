// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Workspace subcommands — all endpoints unimplemented on server side.

use clap::Subcommand;

#[derive(Debug, Subcommand)]
pub enum WorkspaceCommands {
    /// Create a plan/wave workspace
    Create {
        #[arg(long)]
        plan: Option<i64>,
        #[arg(long)]
        wave: Option<i64>,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Create a feature branch workspace
    CreateFeature {
        branch: String,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Delete a workspace
    Delete {
        workspace_id: String,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// List workspaces
    List {
        #[arg(long)]
        plan: Option<i64>,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Show workspace status
    Status {
        workspace_id: String,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Show workspace events
    Events {
        workspace_id: String,
        #[arg(long, default_value = "20")]
        limit: i64,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

const NOT_IMPL: &str = "Not implemented — planned for future release";

pub async fn handle(cmd: WorkspaceCommands) {
    match cmd {
        WorkspaceCommands::Create { .. }
        | WorkspaceCommands::CreateFeature { .. }
        | WorkspaceCommands::Delete { .. }
        | WorkspaceCommands::List { .. }
        | WorkspaceCommands::Status { .. }
        | WorkspaceCommands::Events { .. } => {
            eprintln!("{NOT_IMPL}");
        }
    }
}

#[cfg(test)]
#[path = "cli_workspace_tests.rs"]
mod tests;
