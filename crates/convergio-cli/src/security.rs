// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Security utilities — input validation, safe file operations, HTTP client hardening.

use std::path::{Path, PathBuf};
use std::time::Duration;

/// Maximum allowed length for user-supplied identifiers (project names, IDs, etc.).
const MAX_IDENTIFIER_LEN: usize = 128;

/// Characters allowed in project names and identifiers.
fn is_safe_identifier_char(c: char) -> bool {
    c.is_ascii_alphanumeric() || c == '-' || c == '_' || c == '.'
}

/// Validate a user-supplied identifier (project name, ID, etc.).
/// Rejects empty strings, path traversal attempts, and unsafe characters.
pub fn validate_identifier(name: &str, label: &str) -> Result<(), String> {
    if name.is_empty() {
        return Err(format!("{label} cannot be empty"));
    }
    if name.len() > MAX_IDENTIFIER_LEN {
        return Err(format!(
            "{label} too long ({} chars, max {MAX_IDENTIFIER_LEN})",
            name.len()
        ));
    }
    if name.contains("..") || name.contains('/') || name.contains('\\') {
        return Err(format!("{label} contains path traversal characters"));
    }
    if name.starts_with('.') || name.starts_with('-') {
        return Err(format!("{label} must not start with '.' or '-'"));
    }
    if !name.chars().all(is_safe_identifier_char) {
        return Err(format!(
            "{label} contains invalid characters (allowed: a-z, A-Z, 0-9, -, _, .)"
        ));
    }
    Ok(())
}

/// Sanitize a filename from external sources (e.g., YAML `name` field).
/// Returns only the basename with unsafe characters replaced.
pub fn sanitize_filename(name: &str) -> String {
    let basename = Path::new(name)
        .file_name()
        .and_then(|n| n.to_str())
        .unwrap_or("unnamed");
    basename
        .chars()
        .map(|c| {
            if c.is_ascii_alphanumeric() || c == '-' || c == '_' || c == '.' {
                c
            } else {
                '-'
            }
        })
        .collect()
}

/// Validate that a resolved path stays within an expected parent directory.
/// Prevents path traversal via symlinks and `..` injection.
pub fn validate_path_containment(path: &Path, parent: &Path) -> Result<PathBuf, String> {
    let canonical_parent = parent
        .canonicalize()
        .map_err(|e| format!("cannot resolve parent directory: {e}"))?;
    let canonical_path = path
        .canonicalize()
        .map_err(|e| format!("cannot resolve path: {e}"))?;
    if !canonical_path.starts_with(&canonical_parent) {
        return Err(format!(
            "path escapes allowed directory: {} is not under {}",
            canonical_path.display(),
            canonical_parent.display()
        ));
    }
    Ok(canonical_path)
}

/// Validate that a URL is safe to send auth credentials to.
/// Only allows loopback addresses and HTTPS URLs.
pub fn validate_daemon_url(url: &str) -> Result<(), String> {
    let parsed = url::Url::parse(url).map_err(|e| format!("invalid URL: {e}"))?;
    let scheme = parsed.scheme();
    let host = parsed.host_str().unwrap_or("");

    // Always allow loopback (localhost, 127.x, ::1)
    if is_loopback_host(host) {
        return Ok(());
    }

    // For non-loopback, require HTTPS
    if scheme != "https" {
        return Err(format!(
            "non-loopback URL must use HTTPS (got {scheme}://{host})"
        ));
    }

    Ok(())
}

fn is_loopback_host(host: &str) -> bool {
    host == "localhost"
        || host == "127.0.0.1"
        || host == "::1"
        || host == "[::1]"
        || host.starts_with("127.")
}

/// Allowed environment variable keys that may be imported from remote sources.
const ALLOWED_ENV_KEYS: &[&str] = &[
    "CONVERGIO_AUTH_TOKEN",
    "CONVERGIO_ORG",
    "CONVERGIO_NODE_NAME",
    "ANTHROPIC_API_KEY",
    "TELEGRAM_BOT_TOKEN",
    "CONVERGIO_MESH_SECRET",
    "CONVERGIO_LOG_LEVEL",
];

/// Validate that an env key is in the allowlist.
pub fn is_allowed_env_key(key: &str) -> bool {
    ALLOWED_ENV_KEYS
        .iter()
        .any(|&allowed| allowed == key.trim())
}

/// Truncate a string to `max` characters on a valid UTF-8 boundary.
/// Appends "..." when truncated.
pub fn safe_truncate(s: &str, max: usize) -> String {
    if s.chars().count() <= max {
        s.to_string()
    } else {
        let truncated: String = s.chars().take(max.saturating_sub(3)).collect();
        format!("{truncated}...")
    }
}

/// Sanitize an environment variable value: reject newlines and control chars
/// that could inject additional KEY=VALUE lines into env files.
pub fn sanitize_env_value(val: &str) -> Result<String, String> {
    let trimmed = val.trim();
    if trimmed.contains('\n') || trimmed.contains('\r') || trimmed.contains('\0') {
        return Err("value contains newline or null characters".into());
    }
    if trimmed.chars().any(|c| c.is_control() && c != '\t') {
        return Err("value contains control characters".into());
    }
    Ok(trimmed.to_string())
}

/// Percent-encode a string for safe use as a URL path segment.
pub fn encode_path_segment(segment: &str) -> String {
    url::form_urlencoded::byte_serialize(segment.as_bytes()).collect()
}

/// Escape a string for safe inclusion in a TOML quoted value.
pub fn escape_toml_value(s: &str) -> String {
    s.replace('\\', "\\\\")
        .replace('"', "\\\"")
        .replace('\n', "\\n")
        .replace('\r', "\\r")
        .replace('\t', "\\t")
}

/// Create a hardened reqwest client with timeouts and restricted redirects.
pub fn hardened_http_client() -> reqwest::Client {
    reqwest::Client::builder()
        .timeout(Duration::from_secs(30))
        .connect_timeout(Duration::from_secs(10))
        .redirect(reqwest::redirect::Policy::limited(5))
        .build()
        .unwrap_or_else(|_| reqwest::Client::new())
}

/// HTTP client for endpoints that legitimately exceed 30s
/// (doctor full suite with mesh sync polling, stress runs, etc.).
pub fn long_running_http_client() -> reqwest::Client {
    reqwest::Client::builder()
        .timeout(Duration::from_secs(180))
        .connect_timeout(Duration::from_secs(10))
        .redirect(reqwest::redirect::Policy::limited(5))
        .build()
        .unwrap_or_else(|_| reqwest::Client::new())
}

/// Write a file with restrictive permissions (0600 on Unix).
#[cfg(unix)]
pub fn write_secret_file(path: &Path, contents: &str) -> std::io::Result<()> {
    use std::os::unix::fs::OpenOptionsExt;
    if let Some(parent) = path.parent() {
        std::fs::create_dir_all(parent)?;
    }
    let mut opts = std::fs::OpenOptions::new();
    opts.write(true).create(true).truncate(true).mode(0o600);
    use std::io::Write;
    let mut file = opts.open(path)?;
    file.write_all(contents.as_bytes())?;
    Ok(())
}

#[cfg(not(unix))]
pub fn write_secret_file(path: &Path, contents: &str) -> std::io::Result<()> {
    if let Some(parent) = path.parent() {
        std::fs::create_dir_all(parent)?;
    }
    std::fs::write(path, contents)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn valid_identifier() {
        assert!(validate_identifier("my-project", "name").is_ok());
        assert!(validate_identifier("foo_bar.v2", "name").is_ok());
    }

    #[test]
    fn rejects_path_traversal_in_identifier() {
        assert!(validate_identifier("../etc/passwd", "name").is_err());
        assert!(validate_identifier("foo/bar", "name").is_err());
        assert!(validate_identifier("foo\\bar", "name").is_err());
    }

    #[test]
    fn rejects_empty_and_long_identifiers() {
        assert!(validate_identifier("", "name").is_err());
        assert!(validate_identifier(&"a".repeat(200), "name").is_err());
    }

    #[test]
    fn rejects_leading_dot_or_dash() {
        assert!(validate_identifier(".hidden", "name").is_err());
        assert!(validate_identifier("-flag", "name").is_err());
    }

    #[test]
    fn sanitize_filename_strips_paths() {
        assert_eq!(sanitize_filename("../../../etc/passwd"), "passwd");
        assert_eq!(sanitize_filename("foo/bar/baz"), "baz");
        assert_eq!(sanitize_filename("normal-name"), "normal-name");
    }

    #[test]
    fn loopback_urls_are_safe() {
        assert!(validate_daemon_url("http://localhost:8420").is_ok());
        assert!(validate_daemon_url("http://127.0.0.1:8420").is_ok());
    }

    #[test]
    fn non_loopback_http_rejected() {
        assert!(validate_daemon_url("http://evil.com:8420").is_err());
    }

    #[test]
    fn non_loopback_https_allowed() {
        assert!(validate_daemon_url("https://daemon.example.com:8420").is_ok());
    }

    #[test]
    fn env_key_allowlist() {
        assert!(is_allowed_env_key("CONVERGIO_AUTH_TOKEN"));
        assert!(is_allowed_env_key("ANTHROPIC_API_KEY"));
        assert!(!is_allowed_env_key("PATH"));
        assert!(!is_allowed_env_key("LD_PRELOAD"));
    }

    #[test]
    fn safe_truncate_ascii() {
        assert_eq!(safe_truncate("hello", 10), "hello");
        assert_eq!(safe_truncate("hello world!", 8), "hello...");
    }

    #[test]
    fn safe_truncate_multibyte() {
        // 5 chars, each 3 bytes in UTF-8
        let s = "àèìòù";
        let result = safe_truncate(s, 4);
        assert!(result.ends_with("..."));
        // Must not panic
        let _ = safe_truncate(s, 1);
    }

    #[test]
    fn sanitize_env_value_rejects_newlines() {
        assert!(sanitize_env_value("good-value").is_ok());
        assert!(sanitize_env_value("has\nnewline").is_err());
        assert!(sanitize_env_value("has\r\nCRLF").is_err());
        assert!(sanitize_env_value("has\0null").is_err());
    }

    #[test]
    fn encode_path_segment_encodes_special_chars() {
        assert_eq!(encode_path_segment("foo/bar"), "foo%2Fbar");
        assert_eq!(encode_path_segment("a&b=c"), "a%26b%3Dc");
    }

    #[test]
    fn escape_toml_handles_special_chars() {
        assert_eq!(escape_toml_value("hello"), "hello");
        assert_eq!(escape_toml_value("a\"b"), "a\\\"b");
        assert_eq!(escape_toml_value("line\nbreak"), "line\\nbreak");
    }
}
