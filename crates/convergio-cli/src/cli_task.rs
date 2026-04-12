// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Task subcommands for the cvg CLI — delegates to daemon HTTP API via reqwest.
// JSON output by default; --human flag for readable text.

use clap::Subcommand;

// Re-export for tests that use `super::*`
#[cfg(test)]
pub use crate::cli_task_format::print_mechanical_human;

#[derive(Debug, Subcommand)]
pub enum TaskCommands {
    /// Update a task status
    Update {
        /// Task DB ID
        task_id: i64,
        /// New status (e.g. in_progress, done, blocked)
        status: String,
        /// Agent identity required by the task gates
        #[arg(long)]
        agent_id: Option<String>,
        /// Optional notes/summary message
        #[arg(long)]
        summary: Option<String>,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Validate a task (Thor gate)
    Validate {
        /// Task DB ID
        task_id: i64,
        /// Plan ID
        plan_id: i64,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Search the knowledge base
    KbSearch {
        /// Search query
        query: String,
        /// Maximum results to return
        #[arg(long, default_value_t = 5)]
        limit: u32,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Create a new task in an existing plan/wave
    Create {
        /// Plan DB ID
        plan_id: i64,
        /// Wave DB ID (wave_id_fk)
        wave_db_id: i64,
        /// Task string identifier (e.g. T1-01)
        task_id: String,
        /// Task title
        title: String,
        /// Priority (default: P2)
        #[arg(long, default_value = "P2")]
        priority: String,
        /// Task type (default: feature)
        #[arg(long = "type", default_value = "feature")]
        task_type: String,
        /// Model override
        #[arg(long, default_value = "")]
        model: String,
        /// Task description
        #[arg(long, default_value = "")]
        description: String,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Approve the deliverable linked to a task
    Approve {
        /// Task DB ID
        task_id: i64,
        /// Approver name or comment
        #[arg(long)]
        comment: Option<String>,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Delete a task from a plan (only draft/todo plans)
    Delete {
        /// Task DB ID
        task_id: i64,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Complete a task atomically (evidence + gates + submit in one call)
    Complete {
        /// Task DB ID
        task_id: i64,
        /// Your agent name
        #[arg(long)]
        agent_id: String,
        /// PR URL or commit hash
        #[arg(long)]
        pr_url: String,
        /// Test command that was run
        #[arg(long, default_value = "cargo test --workspace")]
        test_command: String,
        /// Test output summary
        #[arg(long, default_value = "all tests pass")]
        test_output: String,
        /// Test exit code (0=pass)
        #[arg(long, default_value_t = 0)]
        test_exit_code: i32,
        /// Optional notes
        #[arg(long)]
        notes: Option<String>,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

// Handler split to cli_task_handlers.rs (250 line limit)
pub use crate::cli_task_handlers::handle;

#[cfg(test)]
#[path = "cli_task_tests.rs"]
mod tests;
