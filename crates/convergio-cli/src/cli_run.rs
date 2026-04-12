// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Run subcommands — all endpoints unimplemented on server side.

use crate::cli_error::CliError;
use clap::Subcommand;

#[derive(Debug, Subcommand)]
pub enum RunCommands {
    /// Create a new execution run
    Create {
        plan_id: i64,
        #[arg(long)]
        label: Option<String>,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// List execution runs
    List {
        #[arg(long)]
        plan_id: Option<i64>,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Pause a running execution
    Pause {
        run_id: String,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Resume a paused execution
    Resume {
        run_id: String,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

const NOT_IMPL: &str = "Not implemented — planned for future release";

pub async fn handle(cmd: RunCommands) -> Result<(), CliError> {
    match cmd {
        RunCommands::Create { .. }
        | RunCommands::List { .. }
        | RunCommands::Pause { .. }
        | RunCommands::Resume { .. } => {
            eprintln!("{NOT_IMPL}");
            Ok(())
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn run_commands_create_variant_exists() {
        let cmd = RunCommands::Create {
            plan_id: 685,
            label: Some("wave-1".to_string()),
            human: false,
            api_url: "http://localhost:8420".to_string(),
        };
        assert!(matches!(cmd, RunCommands::Create { plan_id: 685, .. }));
    }

    #[test]
    fn run_commands_list_variant_exists() {
        let cmd = RunCommands::List {
            plan_id: None,
            human: false,
            api_url: "http://localhost:8420".to_string(),
        };
        assert!(matches!(cmd, RunCommands::List { plan_id: None, .. }));
    }
}
