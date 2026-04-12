// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Shared HTTP helpers for CLI subcommands that delegate to the daemon HTTP API.

use crate::cli_error::CliError;

fn auth_header_value(token: Option<&str>) -> Option<String> {
    token
        .map(str::trim)
        .filter(|value| !value.is_empty())
        .map(|value| format!("Bearer {value}"))
}

/// Attach auth header only when the target URL is safe (loopback or HTTPS).
fn with_auth(req: reqwest::RequestBuilder, url: &str) -> reqwest::RequestBuilder {
    if crate::security::validate_daemon_url(url).is_err() {
        return req;
    }
    match auth_header_value(std::env::var("CONVERGIO_AUTH_TOKEN").ok().as_deref()) {
        Some(value) => req.header("Authorization", value),
        None => req,
    }
}

pub async fn fetch_and_print(url: &str, human: bool) -> Result<(), CliError> {
    let client = crate::security::hardened_http_client();
    match with_auth(client.get(url), url).send().await {
        Ok(resp) => {
            let status = resp.status();
            let val: serde_json::Value = resp
                .json()
                .await
                .map_err(|e| CliError::ApiCallFailed(format!("error parsing response: {e}")))?;
            print_value(&val, human);
            if !status.is_success() {
                return Err(CliError::NotFound(val.to_string()));
            }
            Ok(())
        }
        Err(e) => Err(CliError::ApiCallFailed(format!(
            "error connecting to daemon: {e}"
        ))),
    }
}

pub async fn post_and_print(
    url: &str,
    body: &serde_json::Value,
    human: bool,
) -> Result<(), CliError> {
    let client = crate::security::hardened_http_client();
    match with_auth(client.post(url), url).json(body).send().await {
        Ok(resp) => {
            let status = resp.status();
            let val: serde_json::Value = resp
                .json()
                .await
                .map_err(|e| CliError::ApiCallFailed(format!("error parsing response: {e}")))?;
            print_value(&val, human);
            if !status.is_success() {
                return Err(CliError::NotFound(val.to_string()));
            }
            Ok(())
        }
        Err(e) => Err(CliError::ApiCallFailed(format!(
            "error connecting to daemon: {e}"
        ))),
    }
}

/// POST to `url` and return the parsed JSON value without printing.
/// Returns Err with an exit-code hint on failure (caller should exit).
pub async fn patch_and_print(
    url: &str,
    body: &serde_json::Value,
    human: bool,
) -> Result<(), CliError> {
    let client = crate::security::hardened_http_client();
    match with_auth(client.patch(url), url).json(body).send().await {
        Ok(resp) => {
            let status = resp.status();
            let val: serde_json::Value = resp
                .json()
                .await
                .map_err(|e| CliError::ApiCallFailed(format!("error parsing response: {e}")))?;
            print_value(&val, human);
            if !status.is_success() {
                return Err(CliError::NotFound(val.to_string()));
            }
            Ok(())
        }
        Err(e) => Err(CliError::ApiCallFailed(format!(
            "error connecting to daemon: {e}"
        ))),
    }
}

/// POST to `url` and return the parsed JSON value without printing.
pub async fn post_and_return(
    url: &str,
    body: &serde_json::Value,
) -> Result<serde_json::Value, i32> {
    let client = crate::security::hardened_http_client();
    match with_auth(client.post(url), url).json(body).send().await {
        Ok(resp) => {
            let status = resp.status();
            match resp.json::<serde_json::Value>().await {
                Ok(val) => {
                    if status.is_success() {
                        Ok(val)
                    } else {
                        eprintln!("error: daemon returned HTTP {status}");
                        Err(1)
                    }
                }
                Err(_) => {
                    eprintln!("error: failed to parse daemon response");
                    Err(2)
                }
            }
        }
        Err(_) => {
            eprintln!("error: cannot connect to daemon");
            Err(2)
        }
    }
}

/// GET `url` and return the parsed JSON value without printing.
pub async fn get_and_return(url: &str) -> Result<serde_json::Value, i32> {
    let client = crate::security::hardened_http_client();
    match with_auth(client.get(url), url).send().await {
        Ok(resp) => {
            let status = resp.status();
            match resp.json::<serde_json::Value>().await {
                Ok(val) => {
                    if status.is_success() {
                        Ok(val)
                    } else {
                        eprintln!("error: daemon returned HTTP {status}");
                        Err(1)
                    }
                }
                Err(_) => {
                    eprintln!("error: failed to parse daemon response");
                    Err(2)
                }
            }
        }
        Err(_) => {
            eprintln!("error: cannot connect to daemon");
            Err(2)
        }
    }
}

/// GET `url` and return parsed JSON, or `fallback` on any error.
/// Unlike `get_and_return`, this never prints to stderr — useful
/// for enrichment calls where failure is non-fatal.
pub async fn get_json_or_default(url: &str, fallback: serde_json::Value) -> serde_json::Value {
    let client = crate::security::hardened_http_client();
    let Ok(resp) = with_auth(client.get(url), url).send().await else {
        return fallback;
    };
    if !resp.status().is_success() {
        return fallback;
    }
    resp.json::<serde_json::Value>().await.unwrap_or(fallback)
}

pub fn print_value(val: &serde_json::Value, human: bool) {
    let use_human = human || crate::human_output::is_human_mode();
    if use_human {
        println!("{}", crate::human_output::format_human(val));
    } else {
        println!("{val}");
    }
}

#[cfg(test)]
mod tests {
    use super::auth_header_value;

    #[test]
    fn builds_bearer_header_when_token_present() {
        assert_eq!(
            auth_header_value(Some("secret")),
            Some("Bearer secret".to_string())
        );
    }

    #[test]
    fn skips_bearer_header_when_token_missing() {
        assert!(auth_header_value(None).is_none());
        assert!(auth_header_value(Some("   ")).is_none());
    }
}
