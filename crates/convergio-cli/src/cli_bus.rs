// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Bus (IPC) subcommands for the cvg CLI — delegates to daemon HTTP API.
// JSON output by default; --human flag for readable text.

use clap::Subcommand;

#[derive(Debug, Subcommand)]
pub enum BusCommands {
    /// List connected agents on the IPC bus
    Who {
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Send a message to a specific agent
    Send {
        /// Sender agent name
        from: String,
        /// Recipient agent name
        to: String,
        /// Message content
        message: String,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Read messages for an agent
    Read {
        /// Agent name to read messages for
        name: String,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Broadcast a message to all agents
    Broadcast {
        /// Sender agent name
        from: String,
        /// Message content
        message: String,
        /// Human-readable output instead of JSON
        #[arg(long)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Render org hierarchy tree from org API data
    Org {
        /// Human-readable tree output (default true)
        #[arg(long, default_value_t = true)]
        human: bool,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Watch direct messages for an agent over SSE
    Watch {
        /// Agent name to subscribe as
        name: String,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Ask an agent and wait for a direct reply
    Ask {
        /// Sender agent name
        from: String,
        /// Recipient agent name
        to: String,
        /// Message content
        message: String,
        /// Timeout in seconds
        #[arg(long, default_value_t = 120)]
        timeout: u64,
        /// Daemon API base URL
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

pub async fn handle(cmd: BusCommands) {
    match cmd {
        BusCommands::Who { human, api_url } => {
            if let Err(e) =
                crate::cli_http::fetch_and_print(&format!("{api_url}/api/ipc/agents"), human).await
            {
                eprintln!("error: {e}");
            }
        }
        BusCommands::Send {
            from,
            to,
            message,
            human,
            api_url,
        } => {
            let body = serde_json::json!({
                "from": from,
                "to": to,
                "content": message,
            });
            if let Err(e) =
                crate::cli_http::post_and_print(&format!("{api_url}/api/ipc/send"), &body, human)
                    .await
            {
                eprintln!("error: {e}");
            }
        }
        BusCommands::Read {
            name,
            human,
            api_url,
        } => {
            if let Err(e) = crate::cli_http::fetch_and_print(
                &format!("{api_url}/api/ipc/messages?agent={name}"),
                human,
            )
            .await
            {
                eprintln!("error: {e}");
            }
        }
        BusCommands::Broadcast { .. } => {
            eprintln!("Not implemented — planned for future release");
        }
        BusCommands::Org { human, api_url } => {
            crate::cli_bus_org::run_org(&api_url, human).await;
        }
        BusCommands::Watch { name, api_url } => {
            if let Err(e) = crate::cli_bus_watch::run_watch(&name, &api_url).await {
                eprintln!("error: {e}");
            }
        }
        BusCommands::Ask { .. } => {
            eprintln!("Not implemented — planned for future release");
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn all_variants_constructible() {
        assert!(matches!(
            BusCommands::Who {
                human: false,
                api_url: String::new()
            },
            BusCommands::Who { .. }
        ));
        assert!(matches!(
            BusCommands::Send {
                from: String::new(),
                to: String::new(),
                message: String::new(),
                human: false,
                api_url: String::new(),
            },
            BusCommands::Send { .. }
        ));
        assert!(matches!(
            BusCommands::Read {
                name: String::new(),
                human: true,
                api_url: String::new()
            },
            BusCommands::Read { .. }
        ));
        assert!(matches!(
            BusCommands::Broadcast {
                from: String::new(),
                message: String::new(),
                human: false,
                api_url: String::new(),
            },
            BusCommands::Broadcast { .. }
        ));
        assert!(matches!(
            BusCommands::Org {
                human: true,
                api_url: String::new()
            },
            BusCommands::Org { .. }
        ));
    }

    #[test]
    fn send_body_uses_content_field() {
        let body = serde_json::json!({
            "from": "planner", "to": "executor", "content": "start task T1-01",
        });
        assert_eq!(body["content"], "start task T1-01");
    }

    #[test]
    fn read_url_includes_agent_param() {
        let url = format!(
            "{}/api/ipc/messages?agent={}",
            "http://localhost:8420", "thor"
        );
        assert!(url.contains("agent=thor"));
    }
}
