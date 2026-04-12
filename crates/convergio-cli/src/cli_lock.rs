// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Lock subcommand — all IPC lock routes unimplemented on server side.

use clap::Subcommand;

#[derive(Debug, Subcommand)]
pub enum LockCommands {
    /// Acquire a file lock for a task
    Acquire {
        file_path: String,
        task_id: i64,
        #[arg(long, default_value = "task-executor")]
        agent: String,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Release a file lock
    Release {
        file_path: String,
        task_id: i64,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// List all active file locks
    List {
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

const NOT_IMPL: &str = "Not implemented — planned for future release";

pub async fn handle(cmd: LockCommands) {
    match cmd {
        LockCommands::Acquire { .. } | LockCommands::Release { .. } | LockCommands::List { .. } => {
            eprintln!("{NOT_IMPL}");
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lock_acquire_variant_exists() {
        let cmd = LockCommands::Acquire {
            file_path: "daemon/src/main.rs".to_string(),
            task_id: 8796,
            agent: "task-executor".to_string(),
            human: false,
            api_url: "http://localhost:8420".to_string(),
        };
        assert!(matches!(cmd, LockCommands::Acquire { task_id: 8796, .. }));
    }

    #[test]
    fn lock_list_variant_exists() {
        let cmd = LockCommands::List {
            human: true,
            api_url: "http://localhost:8420".to_string(),
        };
        assert!(matches!(cmd, LockCommands::List { human: true, .. }));
    }
}
