use crate::cli_error::CliError;
use clap::Subcommand;

#[derive(Debug, Subcommand)]
pub enum MemoryCommands {
    /// Store a new memory (cvg memory remember "content" --agent <id> --type Fact)
    Remember {
        content: String,
        #[arg(long)]
        agent: String,
        #[arg(long, default_value = "Fact")]
        r#type: String,
        #[arg(long, value_delimiter = ',')]
        tags: Option<Vec<String>>,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Recall memories (cvg memory recall --query "search" --limit 10)
    Recall {
        #[arg(long)]
        query: Option<String>,
        #[arg(long)]
        semantic: Option<String>,
        #[arg(long)]
        r#type: Option<String>,
        #[arg(long)]
        agent: Option<String>,
        #[arg(long, default_value = "10")]
        limit: usize,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
        #[arg(long)]
        human: bool,
    },
    /// Forget a memory by ID (cvg memory forget <id>)
    Forget {
        id: String,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Share a memory with other agents (cvg memory share <id> --to agent1,agent2)
    Share {
        id: String,
        #[arg(long, value_delimiter = ',')]
        to: Vec<String>,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Attest a memory (cvg memory attest <id> --agent <id> --confidence 0.95)
    Attest {
        id: String,
        #[arg(long)]
        agent: String,
        #[arg(long, default_value = "1.0")]
        confidence: f64,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Export all memories as Markdown files (cvg memory export --dir <path>)
    Export {
        #[arg(long)]
        dir: String,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Rebuild indexes: Markdown → SQLite → VectorStore (cvg memory reindex)
    Reindex {
        /// Directory with Markdown memory files
        #[arg(long)]
        dir: Option<String>,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
}

const NOT_IMPL: &str = "Not implemented — planned for future release";

pub async fn handle(cmd: MemoryCommands) -> Result<(), CliError> {
    match cmd {
        MemoryCommands::Remember { .. }
        | MemoryCommands::Recall { .. }
        | MemoryCommands::Forget { .. }
        | MemoryCommands::Share { .. }
        | MemoryCommands::Attest { .. }
        | MemoryCommands::Export { .. }
        | MemoryCommands::Reindex { .. } => {
            eprintln!("{NOT_IMPL}");
            Ok(())
        }
    }
}
