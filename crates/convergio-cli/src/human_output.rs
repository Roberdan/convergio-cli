// Human-readable output formatters for CLI commands.
// Formats JSON responses as aligned tables/key-value pairs instead of raw JSON.
// No external dependencies — simple padded columns using ANSI escapes.

use serde_json::Value;
use std::sync::atomic::{AtomicBool, Ordering};

const CYAN: &str = "\x1b[36m";
const DIM: &str = "\x1b[2m";
const BOLD: &str = "\x1b[1m";
const RESET: &str = "\x1b[0m";

/// Global flag: when true, all `print_value` calls use human formatting.
static HUMAN_MODE: AtomicBool = AtomicBool::new(false);

/// Enable global human-readable output mode.
pub fn enable_human_mode() {
    HUMAN_MODE.store(true, Ordering::Relaxed);
}

/// Check if global human mode is enabled.
pub fn is_human_mode() -> bool {
    HUMAN_MODE.load(Ordering::Relaxed)
}

/// Format a JSON value for human consumption.
/// Dispatches to `format_list` for arrays, `format_single` for objects.
pub fn format_human(val: &Value) -> String {
    match val {
        Value::Array(arr) => format_list_auto(arr),
        Value::Object(map) => {
            // Check for common wrapper patterns: {"plans": [...]}, {"orgs": [...]}, etc.
            if let Some((key, inner)) = find_list_field(map) {
                if let Some(arr) = inner.as_array() {
                    let header = format!("{BOLD}{key}{RESET} ({} items)\n", arr.len());
                    return format!("{header}{}", format_list_auto(arr));
                }
            }
            format_single(map)
        }
        other => other.to_string(),
    }
}

/// Find the first array-valued field in a JSON object (common API wrapper pattern).
fn find_list_field(map: &serde_json::Map<String, Value>) -> Option<(&str, &Value)> {
    // Prioritize known list keys
    for key in &[
        "plans",
        "orgs",
        "agents",
        "tasks",
        "waves",
        "workers",
        "checks",
        "issues",
        "sessions",
        "workspaces",
        "items",
    ] {
        if let Some(val) = map.get(*key) {
            if val.is_array() {
                return Some((key, val));
            }
        }
    }
    None
}

/// Format a JSON array as an aligned table with auto-detected columns.
fn format_list_auto(arr: &[Value]) -> String {
    if arr.is_empty() {
        return "(empty list)".to_string();
    }
    // Collect all unique keys from all objects in the array
    let columns = detect_columns(arr);
    if columns.is_empty() {
        // Not objects — just print values line by line
        return arr.iter().map(format_scalar).collect::<Vec<_>>().join("\n");
    }
    format_table(arr, &columns)
}

/// Format a JSON array as a table with explicit column selection.
pub fn format_list(arr: &[Value], columns: &[&str]) -> String {
    if arr.is_empty() {
        return "(empty list)".to_string();
    }
    let cols: Vec<String> = columns.iter().map(|c| c.to_string()).collect();
    format_table(arr, &cols)
}

/// Format a JSON object as key-value pairs.
fn format_single(map: &serde_json::Map<String, Value>) -> String {
    if map.is_empty() {
        return "(empty)".to_string();
    }
    let max_key_len = map.keys().map(|k| k.len()).max().unwrap_or(0);
    let mut lines = Vec::new();
    for (key, val) in map {
        let display = format_scalar(val);
        lines.push(format!(
            "  {CYAN}{:<width$}{RESET}  {display}",
            key,
            width = max_key_len
        ));
    }
    lines.join("\n")
}

/// Format a single scalar value for display.
fn format_scalar(val: &Value) -> String {
    match val {
        Value::String(s) => s.clone(),
        Value::Null => DIM.to_string() + "null" + RESET,
        Value::Bool(b) => b.to_string(),
        Value::Number(n) => n.to_string(),
        Value::Array(arr) => {
            if arr.len() <= 3 {
                let items: Vec<String> = arr.iter().map(format_scalar).collect();
                format!("[{}]", items.join(", "))
            } else {
                format!("[{} items]", arr.len())
            }
        }
        Value::Object(m) => {
            if m.len() <= 3 {
                let items: Vec<String> = m
                    .iter()
                    .map(|(k, v)| format!("{k}: {}", format_scalar(v)))
                    .collect();
                format!("{{{}}}", items.join(", "))
            } else {
                format!("{{{} fields}}", m.len())
            }
        }
    }
}

/// Detect columns from the first few objects in an array.
/// Picks commonly useful fields, capped at 6 columns for readability.
fn detect_columns(arr: &[Value]) -> Vec<String> {
    let mut all_keys: Vec<String> = Vec::new();
    // Gather keys from first 3 items for a representative sample
    for item in arr.iter().take(3) {
        if let Some(obj) = item.as_object() {
            for key in obj.keys() {
                if !all_keys.contains(key) {
                    all_keys.push(key.clone());
                }
            }
        }
    }
    // Prioritize known important fields, then fill remaining slots
    let priority = [
        "id",
        "name",
        "status",
        "title",
        "task_id",
        "wave_id",
        "plan_id",
        "agent_id",
        "model",
        "priority",
        "type",
        "created_at",
        "updated_at",
        "description",
        "message",
    ];
    let mut selected: Vec<String> = Vec::new();
    for p in &priority {
        if all_keys.contains(&p.to_string()) && selected.len() < 6 {
            selected.push(p.to_string());
        }
    }
    // Fill remaining from all_keys if under 6
    for key in &all_keys {
        if !selected.contains(key) && selected.len() < 6 {
            selected.push(key.clone());
        }
    }
    selected
}

/// Render a table with headers and aligned columns.
fn format_table(arr: &[Value], columns: &[String]) -> String {
    // Compute column widths
    let mut widths: Vec<usize> = columns.iter().map(|c| c.len()).collect();
    let mut rows: Vec<Vec<String>> = Vec::new();

    for item in arr {
        let mut row = Vec::new();
        for (i, col) in columns.iter().enumerate() {
            let cell = item
                .get(col)
                .map(format_scalar)
                .unwrap_or_else(|| "-".to_string());
            // Truncate long cells
            let cell = if cell.len() > 40 {
                format!("{}...", &cell[..37])
            } else {
                cell
            };
            if cell.len() > widths[i] {
                widths[i] = cell.len();
            }
            row.push(cell);
        }
        rows.push(row);
    }

    // Cap column widths
    for w in widths.iter_mut() {
        *w = (*w).min(40);
    }

    let mut out = String::new();

    // Header
    let header: String = columns
        .iter()
        .enumerate()
        .map(|(i, c)| {
            let upper = c.to_uppercase();
            format!("{CYAN}{:<width$}{RESET}", upper, width = widths[i])
        })
        .collect::<Vec<_>>()
        .join("  ");
    out.push_str(&header);
    out.push('\n');

    // Separator
    let sep: String = widths
        .iter()
        .map(|w| DIM.to_string() + &"-".repeat(*w) + RESET)
        .collect::<Vec<_>>()
        .join("  ");
    out.push_str(&sep);
    out.push('\n');

    // Rows
    for row in &rows {
        let line: String = row
            .iter()
            .enumerate()
            .map(|(i, cell)| format!("{:<width$}", cell, width = widths[i]))
            .collect::<Vec<_>>()
            .join("  ");
        out.push_str(&line);
        out.push('\n');
    }

    out
}

/// Format a status/health response with visual indicators.
pub fn format_status(val: &Value) -> String {
    let mut lines = Vec::new();
    if let Some(status) = val.get("status").and_then(|v| v.as_str()) {
        let icon = if status == "ok" || status == "healthy" {
            "\x1b[32m\u{2713}\x1b[0m"
        } else {
            "\x1b[31m\u{2717}\x1b[0m"
        };
        lines.push(format!("{icon} Status: {BOLD}{status}{RESET}"));
    }
    if let Some(obj) = val.as_object() {
        for (key, val) in obj {
            if key == "status" {
                continue;
            }
            lines.push(format!("  {key}: {}", format_scalar(val)));
        }
    }
    lines.join("\n")
}

#[cfg(test)]
#[path = "human_output_tests.rs"]
mod tests;
