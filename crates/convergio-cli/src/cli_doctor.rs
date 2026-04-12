//! CLI doctor command — `cvg doctor`
//!
//! Default: runs ALL checks (E2E + chaos included), shows summary + issues.
//! Use --fast for quick core-only, --details for verbose, --json for agents.

use crate::cli_error::CliError;
use crate::cli_http;
use clap::Subcommand;
use serde_json::Value;

const GREEN: &str = "\x1b[32m";
const RED: &str = "\x1b[31m";
const YELLOW: &str = "\x1b[33m";
const BOLD: &str = "\x1b[1m";
const DIM: &str = "\x1b[2m";
const RESET: &str = "\x1b[0m";

#[derive(Debug, Subcommand)]
pub enum DoctorCommands {
    #[command(about = "Run diagnostic checks (default: full suite, summary output)")]
    Run {
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
        #[arg(long, help = "Output raw JSON (for agents)")]
        json: bool,
        #[arg(long, help = "Quick: core + advanced + beta only (~1s)")]
        fast: bool,
        #[arg(long, help = "Show every check, not just issues")]
        details: bool,
    },
    #[command(about = "Run a single category (core, extensions, beta, e2e, chaos, cleanup)")]
    Check {
        category: String,
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
        #[arg(long, help = "Output raw JSON")]
        json: bool,
    },
    #[command(about = "Show only failing/warning checks (for agents)")]
    Issues {
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
        #[arg(long, help = "Output raw JSON")]
        json: bool,
    },
    #[command(about = "One-line pass/fail status")]
    Summary {
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    #[command(about = "Doctor version and categories")]
    Version {
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
    #[command(about = "Past report history")]
    History {
        #[arg(long, default_value = "http://localhost:8420")]
        api_url: String,
    },
}

pub async fn handle(cmd: DoctorCommands) -> Result<(), CliError> {
    match cmd {
        DoctorCommands::Run {
            api_url,
            json,
            fast,
            details,
        } => handle_run(&api_url, json, fast, details).await,
        DoctorCommands::Check {
            category,
            api_url,
            json,
        } => handle_check(&category, &api_url, json).await,
        DoctorCommands::Issues { api_url, json } => handle_issues(&api_url, json).await,
        DoctorCommands::Summary { api_url } => handle_summary(&api_url).await,
        DoctorCommands::Version { api_url } => handle_version(&api_url).await,
        DoctorCommands::History { api_url } => handle_history(&api_url).await,
    }
}

async fn handle_run(
    api_url: &str,
    raw_json: bool,
    fast: bool,
    details: bool,
) -> Result<(), CliError> {
    let path = if fast {
        "/api/doctor"
    } else {
        "/api/doctor/full"
    };
    let url = format!("{api_url}{path}");
    let report = cli_http::get_and_return(&url)
        .await
        .map_err(|_| CliError::ApiCallFailed("Cannot reach daemon — is it running?".into()))?;

    if raw_json {
        println!(
            "{}",
            serde_json::to_string_pretty(&report).unwrap_or_default()
        );
        return Ok(());
    }

    let dv = report
        .get("doctor_version")
        .and_then(|v| v.as_str())
        .unwrap_or("?");
    let sv = report
        .get("daemon_version")
        .and_then(|v| v.as_str())
        .unwrap_or("?");
    let summary = report.get("summary").cloned().unwrap_or_default();
    let checks = report.get("checks").and_then(|v| v.as_array());
    let mode = if fast { "fast" } else { "full" };

    println!();
    println!("{BOLD}Convergio Doctor{RESET} v{dv}  •  Daemon v{sv}  •  {mode}");
    println!("{DIM}{}{RESET}", "─".repeat(55));

    if let Some(checks) = checks {
        if details {
            // --details: show every check
            for c in checks {
                print_check(c);
            }
        } else {
            // Default: show only issues (warn/fail)
            let issues: Vec<_> = checks
                .iter()
                .filter(|c| c.get("status").and_then(|v| v.as_str()) != Some("pass"))
                .collect();
            if issues.is_empty() {
                println!("  {GREEN}✓ All checks passed{RESET}");
            } else {
                for c in &issues {
                    print_check(c);
                }
            }
        }
    }

    println!("{DIM}{}{RESET}", "─".repeat(55));
    print_summary(&summary);
    println!();
    Ok(())
}

fn print_check(c: &Value) {
    let name = c.get("name").and_then(|v| v.as_str()).unwrap_or("?");
    let status = c.get("status").and_then(|v| v.as_str()).unwrap_or("?");
    let msg = c.get("message").and_then(|v| v.as_str()).unwrap_or("");
    let ms = c.get("duration_ms").and_then(|v| v.as_u64()).unwrap_or(0);
    let cat = c.get("category").and_then(|v| v.as_str()).unwrap_or("");
    let icon = match status {
        "pass" => format!("{GREEN}✓{RESET}"),
        "warn" => format!("{YELLOW}⚠{RESET}"),
        "fail" => format!("{RED}✗{RESET}"),
        _ => "?".into(),
    };
    if cat.is_empty() {
        println!("  {icon} {name:<30} {DIM}{msg} ({ms}ms){RESET}");
    } else {
        println!("  {icon} [{cat}] {name:<26} {DIM}{msg} ({ms}ms){RESET}");
    }
}

async fn handle_check(category: &str, api_url: &str, raw_json: bool) -> Result<(), CliError> {
    let url = format!("{api_url}/api/doctor/check/{category}");
    let report = cli_http::get_and_return(&url)
        .await
        .map_err(|_| CliError::ApiCallFailed("Cannot reach daemon".into()))?;

    if raw_json {
        println!(
            "{}",
            serde_json::to_string_pretty(&report).unwrap_or_default()
        );
        return Ok(());
    }

    println!();
    println!("{BOLD}Doctor — {category}{RESET}");
    println!("{DIM}{}{RESET}", "─".repeat(55));
    if let Some(checks) = report.get("checks").and_then(|v| v.as_array()) {
        for c in checks {
            print_check(c);
        }
    }
    let summary = report.get("summary").cloned().unwrap_or_default();
    println!("{DIM}{}{RESET}", "─".repeat(55));
    print_summary(&summary);
    println!();
    Ok(())
}

async fn handle_version(api_url: &str) -> Result<(), CliError> {
    cli_http::fetch_and_print(&format!("{api_url}/api/doctor/version"), true).await
}

async fn handle_history(api_url: &str) -> Result<(), CliError> {
    cli_http::fetch_and_print(&format!("{api_url}/api/doctor/history"), true).await
}

async fn handle_issues(api_url: &str, raw_json: bool) -> Result<(), CliError> {
    let url = format!("{api_url}/api/doctor/issues");
    let report = cli_http::get_and_return(&url)
        .await
        .map_err(|_| CliError::ApiCallFailed("Cannot reach daemon".into()))?;

    if raw_json {
        println!(
            "{}",
            serde_json::to_string_pretty(&report).unwrap_or_default()
        );
        return Ok(());
    }

    let count = report.get("count").and_then(|v| v.as_u64()).unwrap_or(0);
    if count == 0 {
        println!("{GREEN}✓ No issues{RESET}");
        return Ok(());
    }
    println!("{BOLD}{count} issue(s):{RESET}");
    if let Some(issues) = report.get("issues").and_then(|v| v.as_array()) {
        for c in issues {
            print_check(c);
        }
    }
    Ok(())
}

async fn handle_summary(api_url: &str) -> Result<(), CliError> {
    let url = format!("{api_url}/api/doctor/summary");
    let report = cli_http::get_and_return(&url)
        .await
        .map_err(|_| CliError::ApiCallFailed("Cannot reach daemon".into()))?;
    print_summary(&report);
    Ok(())
}

fn print_summary(summary: &Value) {
    let passed = summary.get("passed").and_then(|v| v.as_u64()).unwrap_or(0);
    let warnings = summary
        .get("warnings")
        .and_then(|v| v.as_u64())
        .unwrap_or(0);
    let failed = summary.get("failed").and_then(|v| v.as_u64()).unwrap_or(0);
    let total = summary
        .get("total")
        .and_then(|v| v.as_u64())
        .unwrap_or(passed + warnings + failed);
    let ms = summary
        .get("duration_ms")
        .and_then(|v| v.as_u64())
        .unwrap_or(0);

    let icon = if failed > 0 {
        format!("{RED}FAIL{RESET}")
    } else if warnings > 0 {
        format!("{YELLOW}WARN{RESET}")
    } else {
        format!("{GREEN}PASS{RESET}")
    };
    println!(
        "  {icon}  {total} checks: {GREEN}{passed}✓{RESET} {YELLOW}{warnings}⚠{RESET} {RED}{failed}✗{RESET}  {DIM}({ms}ms){RESET}"
    );
}
