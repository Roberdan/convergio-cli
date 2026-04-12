// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// `cvg reap [--dry-run]` — endpoint unimplemented on server side.

use clap::Subcommand;

#[derive(Debug, Subcommand)]
pub enum ReapCommands {
    /// Run all reapers (worktree, branch, lock-file). Add --dry-run to preview only.
    Run {
        #[arg(long)]
        dry_run: bool,
        #[arg(long, default_value = ".")]
        repo_root: String,
        #[arg(long, default_value = "/tmp")]
        lock_dir: String,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

const NOT_IMPL: &str = "Not implemented — planned for future release";

pub async fn handle(cmd: ReapCommands) {
    match cmd {
        ReapCommands::Run { .. } => {
            eprintln!("{NOT_IMPL}");
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reap_run_command_dry_run_field() {
        let cmd = ReapCommands::Run {
            dry_run: true,
            repo_root: ".".into(),
            lock_dir: "/tmp".into(),
            human: false,
            api_url: "http://localhost:8420".into(),
        };
        assert!(matches!(cmd, ReapCommands::Run { dry_run: true, .. }));
    }
}
