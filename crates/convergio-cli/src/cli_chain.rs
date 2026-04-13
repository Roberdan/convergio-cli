// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Chain subcommands for cvg CLI — ecosystem dependency chain management.
// Handlers in cli_chain_handlers.rs.

use clap::Subcommand;

#[derive(Debug, Subcommand)]
pub enum ChainCommands {
    /// Show ecosystem overview (crates, versions, dependency graph)
    Overview {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Detailed dependency status: deps, CI health, latest releases
    Status {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Cascade version bump across the dependency chain
    Bump {
        /// Crate to bump (e.g. convergio-types)
        #[arg(long)]
        crate_name: String,
        /// Current version tag (e.g. v0.1.4)
        #[arg(long)]
        from: String,
        /// Target version tag (e.g. v0.2.0)
        #[arg(long)]
        to: String,
        /// Preview changes without applying
        #[arg(long)]
        dry_run: bool,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

pub async fn handle(cmd: ChainCommands) -> Result<(), crate::cli_error::CliError> {
    crate::cli_chain_handlers::dispatch(cmd).await
}
