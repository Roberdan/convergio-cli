// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Domain-skill mapping CLI subcommands — all endpoints unimplemented on server side.

use clap::Subcommand;

#[derive(Debug, Subcommand)]
pub enum DomainCommands {
    /// List all domain->skill mappings
    List {
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
        #[arg(long)]
        human: bool,
    },
    /// Add a domain->skill mapping
    Map {
        domain: String,
        skill: String,
        #[arg(long)]
        description: Option<String>,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
        #[arg(long)]
        human: bool,
    },
}

const NOT_IMPL: &str = "Not implemented — planned for future release";

pub async fn dispatch(cmd: DomainCommands) -> Result<(), crate::cli_error::CliError> {
    match cmd {
        DomainCommands::List { .. } | DomainCommands::Map { .. } => {
            eprintln!("{NOT_IMPL}");
        }
    }
    Ok(())
}
