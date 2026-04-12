// Setup step: Inference/AI model configuration.
// Detects MLX (macOS only), configures cloud API keys, model selection.

use crate::cli_error::CliError;
use dialoguer::{Confirm, Input, Select};

/// Interactive inference setup.
pub(crate) async fn setup_inference() -> Result<InferenceChoice, CliError> {
    println!();
    println!("AI Model Configuration");
    println!("----------------------");

    // Detect available backends
    let has_mlx = detect_mlx();
    let has_api_key = std::env::var("ANTHROPIC_API_KEY")
        .map(|k| !k.is_empty())
        .unwrap_or(false);

    let mut options = Vec::new();
    let mut descriptions = Vec::new();

    if has_mlx {
        options.push("mlx");
        descriptions.push("MLX local (macOS only) — Qwen 7B Coder, free, fast");
    }
    options.push("cloud");
    descriptions.push("Cloud API (Anthropic) — Claude Sonnet/Opus, requires API key");
    options.push("both");
    descriptions.push("Both — MLX for local tasks, Cloud for complex ones");
    options.push("none");
    descriptions.push("Skip — configure later");

    println!();
    for desc in &descriptions {
        println!("  {desc}");
    }
    println!();

    let choice_idx = Select::new()
        .with_prompt("Select inference backend")
        .items(&options)
        .default(if has_mlx { 2 } else { 0 })
        .interact()
        .map_err(|e| CliError::InvalidInput(e.to_string()))?;

    let choice = options[choice_idx];

    let mut result = InferenceChoice {
        use_mlx: false,
        mlx_model: None,
        use_cloud: false,
        api_key: None,
    };

    // MLX setup (macOS only)
    if choice == "mlx" || choice == "both" {
        result.use_mlx = true;
        if has_mlx {
            result.mlx_model = Some(select_mlx_model()?);
        } else {
            println!("  MLX not available on this platform.");
            println!("  MLX requires macOS with Apple Silicon (M1/M2/M3/M4).");
            result.use_mlx = false;
        }
    }

    // Cloud API setup
    if choice == "cloud" || choice == "both" {
        result.use_cloud = true;
        if has_api_key {
            println!("  Anthropic API key found in environment.");
        } else {
            let key: String = Input::new()
                .with_prompt("  Anthropic API key")
                .allow_empty(true)
                .interact_text()
                .map_err(|e| CliError::InvalidInput(e.to_string()))?;

            if !key.is_empty() {
                print!("  Validating... ");
                if crate::cli_setup_steps::validate_api_key(&key).await {
                    println!("OK");
                } else {
                    println!("FAILED (saving anyway)");
                }
                // Backup env before writing (with restrictive perms)
                let env_path = crate::paths::env_file_path();
                if env_path.exists() {
                    let backup = env_path.with_extension("env.bak");
                    let _ = std::fs::copy(&env_path, &backup);
                    #[cfg(unix)]
                    {
                        use std::os::unix::fs::PermissionsExt;
                        let _ = std::fs::set_permissions(
                            &backup,
                            std::fs::Permissions::from_mode(0o600),
                        );
                    }
                }
                crate::cli_setup_steps::write_env_file(&key).map_err(CliError::Io)?;
                result.api_key = Some(key);
            }
        }
    }

    Ok(result)
}

pub(crate) struct InferenceChoice {
    pub use_mlx: bool,
    pub mlx_model: Option<String>,
    pub use_cloud: bool,
    pub api_key: Option<String>,
}

/// Detect if MLX is available (macOS Apple Silicon only).
fn detect_mlx() -> bool {
    if cfg!(not(target_os = "macos")) {
        return false;
    }
    // Check for Apple Silicon
    let arch = std::env::consts::ARCH;
    if arch != "aarch64" {
        return false;
    }
    // Check if mlx_lm is installed
    let result = std::process::Command::new("python3")
        .args(["-c", "import mlx_lm; print('ok')"])
        .output();
    matches!(result, Ok(out) if out.status.success())
}

fn select_mlx_model() -> Result<String, CliError> {
    let models = [
        "mlx-community/Qwen2.5-Coder-7B-Instruct-4bit",
        "mlx-community/Qwen2.5-Coder-0.5B-Instruct-4bit",
        "custom",
    ];
    let descriptions = [
        "Qwen 7B Coder 4bit — recommended, excellent quality",
        "Qwen 0.5B Coder 4bit — fast, lower quality",
        "Custom model path",
    ];

    println!();
    for desc in &descriptions {
        println!("  {desc}");
    }
    let idx = Select::new()
        .with_prompt("  Select MLX model")
        .items(&models)
        .default(0)
        .interact()
        .map_err(|e| CliError::InvalidInput(e.to_string()))?;

    if models[idx] == "custom" {
        let path: String = Input::new()
            .with_prompt("  Model path or HuggingFace ID")
            .interact_text()
            .map_err(|e| CliError::InvalidInput(e.to_string()))?;
        Ok(path)
    } else {
        Ok(models[idx].to_string())
    }
}

/// Check if Copilot CLI is installed and offer configuration.
#[allow(dead_code)]
pub(crate) fn detect_copilot() -> bool {
    let result = std::process::Command::new("gh")
        .args(["copilot", "--version"])
        .output();
    matches!(result, Ok(out) if out.status.success())
}

/// Offer to install MLX dependencies (macOS only).
#[allow(dead_code)]
pub(crate) fn offer_mlx_install() -> bool {
    if cfg!(not(target_os = "macos")) || std::env::consts::ARCH != "aarch64" {
        println!("  MLX requires macOS with Apple Silicon. Skipping.");
        return false;
    }

    let install = Confirm::new()
        .with_prompt("  Install MLX dependencies (pip install mlx-lm)?")
        .default(true)
        .interact()
        .unwrap_or(false);

    if !install {
        return false;
    }

    let result = std::process::Command::new("pip3")
        .args(["install", "mlx-lm"])
        .output();
    match result {
        Ok(out) if out.status.success() => {
            println!("  MLX installed successfully.");
            true
        }
        Ok(out) => {
            let stderr = String::from_utf8_lossy(&out.stderr);
            println!("  MLX install failed: {stderr}");
            false
        }
        Err(e) => {
            println!("  pip3 not found: {e}");
            false
        }
    }
}
