// Setup step: Telegram bot configuration.
// Validates bot token via getMe API, stores in env file.

use crate::cli_error::CliError;
use dialoguer::{Confirm, Input};

/// Interactive Telegram setup. Returns true if enabled.
pub(crate) async fn setup_telegram() -> Result<bool, CliError> {
    println!();
    println!("Telegram Bot Configuration");
    println!("--------------------------");
    println!("  Convergio can receive commands via Telegram (Jarvis watchdog).");
    println!("  You need a bot token from @BotFather.");
    println!();

    let enable = Confirm::new()
        .with_prompt("Enable Telegram integration?")
        .default(false)
        .interact()
        .unwrap_or(false);

    if !enable {
        println!("  Skipped. Run `cvg setup` later to enable.");
        return Ok(false);
    }

    let token: String = Input::new()
        .with_prompt("Bot token (from @BotFather)")
        .interact_text()
        .map_err(|e| CliError::InvalidInput(e.to_string()))?;

    if token.is_empty() {
        println!("  Empty token — skipping Telegram.");
        return Ok(false);
    }

    // Validate via getMe API
    print!("  Validating token... ");
    match validate_bot_token(&token).await {
        Ok(bot_name) => {
            println!("OK (bot: @{bot_name})");
        }
        Err(e) => {
            println!("FAILED ({e})");
            let save_anyway = Confirm::new()
                .with_prompt("  Save token anyway?")
                .default(false)
                .interact()
                .unwrap_or(false);
            if !save_anyway {
                return Ok(false);
            }
        }
    }

    // Store in env file with backup
    store_telegram_token(&token)?;
    println!("  Token saved to env file.");

    // macOS: optionally store in Keychain
    #[cfg(target_os = "macos")]
    {
        let use_keychain = Confirm::new()
            .with_prompt("  Also store in macOS Keychain? (more secure)")
            .default(true)
            .interact()
            .unwrap_or(false);
        if use_keychain {
            store_in_keychain(&token);
        }
    }

    Ok(true)
}

async fn validate_bot_token(token: &str) -> Result<String, String> {
    let url = format!("https://api.telegram.org/bot{token}/getMe");
    let client = crate::security::hardened_http_client();
    let resp = client
        .get(&url)
        .send()
        .await
        .map_err(|e| format!("HTTP error: {e}"))?;

    if !resp.status().is_success() {
        return Err(format!("HTTP {}", resp.status()));
    }

    let json: serde_json::Value = resp.json().await.map_err(|e| format!("JSON error: {e}"))?;

    if json["ok"].as_bool() != Some(true) {
        return Err("API returned ok=false".into());
    }

    Ok(json["result"]["username"]
        .as_str()
        .unwrap_or("unknown")
        .to_string())
}

fn store_telegram_token(token: &str) -> Result<(), CliError> {
    let env_path = crate::paths::env_file_path();
    // Backup existing env file before modifying (with restrictive perms)
    if env_path.exists() {
        let backup = env_path.with_extension("env.bak");
        std::fs::copy(&env_path, &backup).map_err(CliError::Io)?;
        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;
            let _ = std::fs::set_permissions(&backup, std::fs::Permissions::from_mode(0o600));
        }
    }

    let existing = std::fs::read_to_string(&env_path).unwrap_or_default();
    let mut lines: Vec<String> = existing
        .lines()
        .filter(|l| !l.starts_with("TELEGRAM_BOT_TOKEN="))
        .map(|l| l.to_string())
        .collect();
    lines.push(format!("TELEGRAM_BOT_TOKEN={token}"));

    if let Some(parent) = env_path.parent() {
        std::fs::create_dir_all(parent).map_err(CliError::Io)?;
    }
    let contents = lines.join("\n") + "\n";
    crate::security::write_secret_file(&env_path, &contents).map_err(CliError::Io)
}

#[cfg(target_os = "macos")]
fn store_in_keychain(token: &str) {
    let result = std::process::Command::new("security")
        .args([
            "add-generic-password",
            "-a",
            "convergio",
            "-s",
            "convergio-telegram-bot-token",
            "-w",
            token,
            "-U", // update if exists
        ])
        .output();
    match result {
        Ok(out) if out.status.success() => {
            println!("  Stored in Keychain.");
        }
        _ => {
            println!("  Keychain storage failed — token is in env file.");
        }
    }
}
