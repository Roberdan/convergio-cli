// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Repository CLI subcommands — all endpoints unimplemented on server side.

use crate::cli_error::CliError;
use clap::Subcommand;

#[derive(Debug, Subcommand)]
pub enum RepoCommands {
    /// Register a repository in the platform
    Add {
        name: String,
        #[arg(long)]
        path: String,
        #[arg(long)]
        github_url: Option<String>,
        #[arg(long)]
        description: Option<String>,
        #[arg(long, default_value = "local")]
        transport: String,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// List all registered repositories
    List {
        #[arg(long)]
        json: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Show details for a single repository
    Show {
        name: String,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Link a repository to a project
    Link {
        repo_name: String,
        project_id: String,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Verify all registered repos exist on disk and check health
    Sync {
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

const NOT_IMPL: &str = "Not implemented — planned for future release";

pub async fn handle(cmd: RepoCommands) -> Result<(), CliError> {
    match cmd {
        RepoCommands::Add { .. }
        | RepoCommands::List { .. }
        | RepoCommands::Show { .. }
        | RepoCommands::Link { .. }
        | RepoCommands::Sync { .. } => {
            eprintln!("{NOT_IMPL}");
            Ok(())
        }
    }
}
