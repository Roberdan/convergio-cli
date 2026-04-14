use crate::cli_error::CliError;

/// ANSI color helpers (no external crate).
const GREEN: &str = "\x1b[32m";
const RED: &str = "\x1b[31m";
const YELLOW: &str = "\x1b[33m";
const CYAN: &str = "\x1b[36m";
const BOLD: &str = "\x1b[1m";
const DIM: &str = "\x1b[2m";
const RESET: &str = "\x1b[0m";

pub async fn handle(api_url: &str) -> Result<(), CliError> {
    let health = fetch_json(&format!("{api_url}/api/health")).await;
    let plans = fetch_json(&format!("{api_url}/api/plan-db/list?status=all")).await;
    let summary = fetch_json(&format!("{api_url}/api/metrics/summary")).await;
    let runtime = fetch_json(&format!("{api_url}/api/agents/runtime")).await;
    let orgs = fetch_json(&format!("{api_url}/api/orgs")).await;
    let mesh = fetch_json(&format!("{api_url}/api/mesh")).await;
    let _deploy = fetch_json(&format!("{api_url}/api/deploy/status")).await;

    let proj_total = summary
        .get("total_tasks")
        .and_then(|v| v.as_i64())
        .unwrap_or(0);
    let proj_done = summary
        .get("done_plans")
        .and_then(|v| v.as_i64())
        .unwrap_or(0);
    let proj_plans = summary
        .get("total_plans")
        .and_then(|v| v.as_i64())
        .unwrap_or(0) as usize;
    let org_count = orgs
        .get("orgs")
        .and_then(|v| v.as_array())
        .map(|a| a.len())
        .unwrap_or(0);

    let ok = health.get("status").and_then(|v| v.as_str()) == Some("ok");
    let version = health
        .get("version")
        .and_then(|v| v.as_str())
        .unwrap_or(env!("CARGO_PKG_VERSION"));
    let peers_online = mesh
        .get("peers_online")
        .and_then(|v| v.as_u64())
        .unwrap_or(0);
    let active_agents = runtime
        .get("active_agents")
        .and_then(|v| v.as_array())
        .map(|a| a.len())
        .unwrap_or(0);
    let cost = summary
        .get("total_cost_usd")
        .and_then(|v| v.as_f64())
        .unwrap_or(0.0);
    let _tokens = summary
        .get("total_tokens")
        .and_then(|v| v.as_i64())
        .unwrap_or(0);
    let w = term_width();

    // plan-db/list returns a JSON array directly
    let plan_list = plans.as_array();
    let Some(all) = plan_list else {
        println!("{RED}ERROR: Cannot reach daemon{RESET}");
        return Ok(());
    };

    let (mut active, mut queued, mut paused): (Vec<_>, Vec<_>, Vec<_>) =
        (Vec::new(), Vec::new(), Vec::new());
    for p in all {
        match p.get("status").and_then(|v| v.as_str()).unwrap_or("") {
            "doing" | "in_progress" => active.push(p),
            "draft" | "todo" | "approved" => queued.push(p),
            "paused" => paused.push(p),
            _ => {}
        }
    }

    println!();
    border(w, 'T');
    let (dot, label) = if ok {
        (GREEN, "online")
    } else {
        (RED, "OFFLINE")
    };
    row(w, &format!("{BOLD}Convergio Platform{RESET} v{version}"));
    row(
        w,
        &format!(
            "Status: {dot}\u{25CF}{RESET} {label}  \u{2502}  \
         Agents: {active_agents}  \u{2502}  Mesh: {peers_online} peers  \u{2502}  \
         Cost: ${cost:.2}",
        ),
    );
    border(w, 'M');

    if !active.is_empty() || !queued.is_empty() || !paused.is_empty() {
        row(w, &format!("{BOLD}{CYAN}ACTIVE PLANS{RESET}"));
        for p in &active {
            plan_row(w, p, "doing");
        }
        for p in &paused {
            plan_row(w, p, "paused");
        }
        for p in &queued {
            plan_row(w, p, "queued");
        }
        border(w, 'M');
    }

    // Show last 5 completed plans (most recent first — API returns newest first)
    let done_plans: Vec<_> = all
        .iter()
        .filter(|p| p.get("status").and_then(|v| v.as_str()) == Some("done"))
        .take(5)
        .collect();
    if !done_plans.is_empty() {
        row(w, &format!("{BOLD}{CYAN}RECENTLY COMPLETED{RESET}"));
        for p in &done_plans {
            plan_row(w, p, "recent");
        }
        border(w, 'M');
    }

    row(
        w,
        &format!(
            "{DIM}{proj_done}/{proj_total} tasks  \u{2502}  \
         {proj_plans} plans  \u{2502}  {org_count} orgs{RESET}",
        ),
    );
    border(w, 'B');
    println!();
    Ok(())
}

// -- box drawing ----------------------------------------------------------

fn term_width() -> usize {
    std::env::var("COLUMNS")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or(60_usize)
        .clamp(50, 120)
}

fn border(w: usize, kind: char) {
    let fill = "\u{2550}".repeat(w - 2);
    match kind {
        'T' => println!("\u{2554}{fill}\u{2557}"),
        'M' => println!("\u{2560}{fill}\u{2563}"),
        _ => println!("\u{255A}{fill}\u{255D}"),
    }
}

fn row(w: usize, text: &str) {
    let vis = visible_len(text);
    let inner = w.saturating_sub(4);
    let pad = inner.saturating_sub(vis);
    println!("\u{2551}  {text}{} \u{2551}", " ".repeat(pad));
}

fn plan_row(w: usize, p: &serde_json::Value, mode: &str) {
    let id = p.get("id").and_then(|v| v.as_i64()).unwrap_or(0);
    let name = p.get("name").and_then(|v| v.as_str()).unwrap_or("?");
    let status = p.get("status").and_then(|v| v.as_str()).unwrap_or("?");
    let done = p.get("tasks_done").and_then(|v| v.as_i64()).unwrap_or(0);
    let total = p
        .get("tasks_total")
        .and_then(|v| v.as_i64())
        .unwrap_or(1)
        .max(1);
    let short: String = name.chars().take(30).collect();

    let line = match mode {
        "doing" => {
            let bar = progress_bar(done, total, 8);
            format!(
                "{YELLOW}\u{25B8}{RESET} #{id:<5} {short:<30} \
                 [{done}/{total} {bar}]",
            )
        }
        "paused" => {
            format!(
                "{YELLOW}\u{25A0}{RESET} #{id:<5} {short:<30} \
                 {YELLOW}[paused {done}/{total}]{RESET}",
            )
        }
        "queued" => format!("{DIM}\u{25E6}{RESET} #{id:<5} {short:<30} {DIM}[queued]{RESET}",),
        _ => {
            let icon = match status {
                "completed" | "done" => format!("{GREEN}\u{2713}{RESET}"),
                "cancelled" => format!("{RED}\u{2717}{RESET}"),
                _ => format!("{DIM}\u{2022}{RESET}"),
            };
            format!("{icon} #{id:<5} {status:<11} {done:>2}/{total:<3} {short}")
        }
    };
    row(w, &format!("  {line}"));
}

fn progress_bar(done: i64, total: i64, width: usize) -> String {
    let ratio = (done as f64) / (total as f64);
    let filled = (ratio * width as f64).round() as usize;
    let empty = width.saturating_sub(filled);
    format!(
        "{GREEN}{}{RESET}{DIM}{}{RESET}",
        "\u{2588}".repeat(filled),
        "\u{2591}".repeat(empty),
    )
}

/// Visible length ignoring ANSI escape sequences.
fn visible_len(s: &str) -> usize {
    let mut len = 0usize;
    let mut in_esc = false;
    for c in s.chars() {
        if in_esc {
            if c.is_ascii_alphabetic() {
                in_esc = false;
            }
        } else if c == '\x1b' {
            in_esc = true;
        } else {
            len += 1;
        }
    }
    len
}

async fn fetch_json(url: &str) -> serde_json::Value {
    crate::cli_http::get_json_or_default(url, serde_json::json!({})).await
}
