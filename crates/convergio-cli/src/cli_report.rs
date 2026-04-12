//! CLI commands for CTT report generation.
//! Pure HTTP client — calls daemon API at /api/reports/*.

use crate::cli_error::CliError;
use crate::cli_http;
use clap::Subcommand;
use serde_json::json;

#[derive(Debug, Subcommand)]
pub enum ReportCommands {
    /// Generate a new CTT research report
    Generate {
        /// Report topic (e.g., "Dedalus Group", "Roberto D'Angelo")
        topic: String,
        /// Report type
        #[arg(long, short = 't', default_value = "general")]
        report_type: String,
        /// Output format
        #[arg(long, short = 'f', default_value = "markdown")]
        format: String,
        /// Target audience
        #[arg(long)]
        audience: Option<String>,
        /// Research depth: brief, standard, full
        #[arg(long, default_value = "standard")]
        depth: String,
        /// Additional context or instructions
        #[arg(long)]
        context: Option<String>,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// List existing reports
    List {
        /// Filter by report type
        #[arg(long, short = 't')]
        report_type: Option<String>,
        /// Filter by status
        #[arg(long)]
        status: Option<String>,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Show a report by ID
    Show {
        /// Report ID
        id: String,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
    /// Download a PDF report
    Download {
        /// Report ID
        id: String,
        /// Output file path
        #[arg(long, short = 'o', default_value = "report.pdf")]
        output: String,
        #[arg(long, default_value = "http://127.0.0.1:8420")]
        api_url: String,
    },
}

pub async fn handle(cmd: ReportCommands) -> Result<(), CliError> {
    match cmd {
        ReportCommands::Generate {
            topic,
            report_type,
            format,
            audience,
            depth,
            context,
            api_url,
        } => {
            let body = json!({
                "topic": topic,
                "report_type": report_type,
                "format": format,
                "audience": audience,
                "depth": depth,
                "extra_context": context,
            });
            let url = format!("{api_url}/api/reports/generate");
            cli_http::post_and_print(&url, &body, true).await
        }
        ReportCommands::List {
            report_type,
            status,
            api_url,
        } => {
            let mut params = Vec::new();
            if let Some(rt) = &report_type {
                params.push(format!("report_type={rt}"));
            }
            if let Some(st) = &status {
                params.push(format!("status={st}"));
            }
            let qs = if params.is_empty() {
                String::new()
            } else {
                format!("?{}", params.join("&"))
            };
            let url = format!("{api_url}/api/reports{qs}");
            let val = cli_http::get_and_return(&url)
                .await
                .map_err(|code| CliError::ApiCallFailed(format!("exit {code}")))?;
            println!("{}", serde_json::to_string_pretty(&val).unwrap_or_default());
            Ok(())
        }
        ReportCommands::Show { id, api_url } => {
            let url = format!("{api_url}/api/reports/{id}");
            let val = cli_http::get_and_return(&url)
                .await
                .map_err(|code| CliError::ApiCallFailed(format!("exit {code}")))?;
            // If content_md exists, print it directly for readability
            if let Some(content) = val.get("content_md").and_then(|v| v.as_str()) {
                println!("{content}");
            } else {
                println!("{}", serde_json::to_string_pretty(&val).unwrap_or_default());
            }
            Ok(())
        }
        ReportCommands::Download {
            id,
            output,
            api_url,
        } => {
            let url = format!("{api_url}/api/reports/{id}/download");
            let client = crate::security::hardened_http_client();
            let resp = client
                .get(&url)
                .send()
                .await
                .map_err(|e| CliError::ApiCallFailed(format!("download failed: {e}")))?;
            if !resp.status().is_success() {
                let body = resp.text().await.unwrap_or_default();
                return Err(CliError::ApiCallFailed(format!("download failed: {body}")));
            }
            let bytes = resp
                .bytes()
                .await
                .map_err(|e| CliError::ApiCallFailed(format!("read failed: {e}")))?;
            std::fs::write(&output, &bytes)
                .map_err(|e| CliError::ApiCallFailed(format!("write failed: {e}")))?;
            println!("Downloaded to {output} ({} bytes)", bytes.len());
            Ok(())
        }
    }
}
