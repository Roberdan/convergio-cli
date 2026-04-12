// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// KB (knowledge base) subcommands — search, write, and seed.

use crate::cli_error::CliError;
use crate::cli_http;
use clap::Subcommand;
use serde_json::json;

#[derive(Debug, Subcommand)]
pub enum KbCommands {
    /// Search the knowledge base
    Search {
        query: String,
        #[arg(long, default_value_t = 5)]
        limit: u32,
        #[arg(long)]
        domain: Option<String>,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Write an entry to the knowledge base
    Write {
        title: String,
        content: String,
        #[arg(long, default_value = "general")]
        domain: String,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    /// Seed the convergio-io KB with platform documentation
    Seed {
        #[arg(long, default_value = "convergio-io")]
        org: String,
        #[arg(long)]
        human: bool,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

pub async fn handle(cmd: KbCommands) -> Result<(), CliError> {
    match cmd {
        KbCommands::Search {
            query,
            limit,
            domain,
            human,
            api_url,
        } => handle_search(&api_url, &query, limit, domain.as_deref(), human).await,
        KbCommands::Write {
            title,
            content,
            domain,
            human,
            api_url,
        } => handle_write(&api_url, &title, &content, &domain, human).await,
        KbCommands::Seed {
            org,
            human,
            api_url,
        } => handle_seed(&api_url, &org, human).await,
    }
}

async fn handle_search(
    api_url: &str,
    query: &str,
    limit: u32,
    domain: Option<&str>,
    human: bool,
) -> Result<(), CliError> {
    let encoded_q: String = url::form_urlencoded::byte_serialize(query.as_bytes()).collect();
    let mut url = format!("{api_url}/api/plan-db/kb-search?q={encoded_q}&limit={limit}",);
    if let Some(d) = domain {
        let encoded_d: String = url::form_urlencoded::byte_serialize(d.as_bytes()).collect();
        url.push_str(&format!("&domain={encoded_d}"));
    }
    cli_http::fetch_and_print(&url, human).await
}

async fn handle_write(
    api_url: &str,
    title: &str,
    content: &str,
    domain: &str,
    human: bool,
) -> Result<(), CliError> {
    let url = format!("{api_url}/api/plan-db/kb-write");
    let body = json!({
        "domain": domain,
        "title": title,
        "content": content,
    });
    cli_http::post_and_print(&url, &body, human).await
}

async fn handle_seed(api_url: &str, org: &str, human: bool) -> Result<(), CliError> {
    let encoded_org: String = url::form_urlencoded::byte_serialize(org.as_bytes()).collect();
    let url = format!("{api_url}/api/orgs/{encoded_org}/kb/seed");
    let body = json!({});
    cli_http::post_and_print(&url, &body, human).await
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn kb_commands_search_variant_exists() {
        let cmd = KbCommands::Search {
            query: "rust async".to_string(),
            limit: 10,
            domain: None,
            human: false,
            api_url: "http://localhost:8420".to_string(),
        };
        assert!(matches!(cmd, KbCommands::Search { .. }));
    }

    #[test]
    fn kb_commands_search_with_domain() {
        let cmd = KbCommands::Search {
            query: "gate chain".to_string(),
            limit: 5,
            domain: Some("convergio-io".to_string()),
            human: true,
            api_url: "http://localhost:8420".to_string(),
        };
        assert!(matches!(
            cmd,
            KbCommands::Search {
                domain: Some(_),
                ..
            }
        ));
    }

    #[test]
    fn kb_commands_write_variant_exists() {
        let cmd = KbCommands::Write {
            title: "TDD pattern".to_string(),
            content: "Write tests first".to_string(),
            domain: "testing".to_string(),
            human: false,
            api_url: "http://localhost:8420".to_string(),
        };
        assert!(matches!(cmd, KbCommands::Write { .. }));
    }

    #[test]
    fn kb_commands_seed_variant_exists() {
        let cmd = KbCommands::Seed {
            org: "convergio-io".to_string(),
            human: false,
            api_url: "http://localhost:8420".to_string(),
        };
        assert!(matches!(cmd, KbCommands::Seed { .. }));
    }
}
