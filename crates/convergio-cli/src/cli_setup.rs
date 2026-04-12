// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// `cvg setup` — interactive first-run wizard for Convergio Platform.

#[path = "cli_setup_config.rs"]
mod cli_setup_config;

use crate::cli_error::CliError;
use crate::cli_setup_steps as steps;
use crate::{cli_setup_inference, cli_setup_service, cli_setup_telegram};
use dialoguer::{Confirm, Input, Select};

const ROLES: [&str; 5] = ["all", "orchestrator", "kernel", "voice", "worker"];
const ROLE_DESC: [&str; 5] = [
    "all         — all extensions (single node, default)",
    "orchestrator — plans, delegation, billing, observatory",
    "kernel      — local AI (Jarvis), Telegram watchdog",
    "voice       — voice I/O only",
    "worker      — receives delegated tasks, runs agents",
];

/// Run the interactive setup wizard. `--defaults` skips all prompts.
pub(crate) async fn handle_setup(defaults: bool) -> Result<(), CliError> {
    if defaults {
        return write_defaults().await;
    }

    println!();
    println!("Welcome to Convergio Setup!");
    println!("===========================");
    println!();

    // Step 1: Node name
    let detected = steps::detect_hostname();
    let node_name: String = Input::new()
        .with_prompt("Step 1/8 — Node name")
        .default(detected)
        .interact_text()
        .map_err(|e| CliError::InvalidInput(e.to_string()))?;

    // Step 2: Role
    println!();
    println!("Step 2/8 — Node role");
    for desc in &ROLE_DESC {
        println!("  {desc}");
    }
    let role_idx = Select::new()
        .with_prompt("Select role")
        .items(ROLES)
        .default(0)
        .interact()
        .map_err(|e| CliError::InvalidInput(e.to_string()))?;
    let role = ROLES[role_idx];

    // Step 3: Network (+ Tailscale install offer)
    println!();
    println!("Step 3/8 — Network");
    let use_tailscale = detect_network();

    // Step 4: AI Model / Inference
    println!();
    println!("Step 4/8 — AI Model");
    let inference = cli_setup_inference::setup_inference().await?;

    // Step 5: Telegram (if kernel or all role)
    let telegram_enabled = if role == "kernel" || role == "all" {
        println!();
        println!("Step 5/8 — Telegram");
        cli_setup_telegram::setup_telegram().await?
    } else {
        false
    };

    // Step 6: Write config + summary
    println!();
    println!("Step 6/8 — Writing configuration...");
    let config_path = crate::paths::config_path();

    // Backup existing config before overwriting
    if config_path.exists() {
        let backup = config_path.with_extension("toml.bak");
        println!("  Backing up existing config to {}", backup.display());
        std::fs::copy(&config_path, &backup).map_err(CliError::Io)?;
    }

    let content = cli_setup_config::render_full_config(
        &node_name,
        role,
        use_tailscale,
        &inference,
        telegram_enabled,
    );
    if let Some(parent) = config_path.parent() {
        std::fs::create_dir_all(parent).map_err(CliError::Io)?;
    }
    std::fs::write(&config_path, &content).map_err(CliError::Io)?;

    // Generate default aliases if not present
    let aliases_path = config_path
        .parent()
        .unwrap_or(std::path::Path::new("."))
        .join("aliases.toml");
    if !aliases_path.exists() {
        let defaults = crate::cli_ask::generate_default_aliases();
        std::fs::write(&aliases_path, &defaults).map_err(CliError::Io)?;
        println!("  Agent aliases written to {}", aliases_path.display());
    }

    println!();
    println!("  Configuration saved to {}", config_path.display());
    println!();
    print_summary(&node_name, role, use_tailscale);

    // Step 7: Service installation
    println!();
    println!("Step 7/8 — System Service");
    cli_setup_service::setup_service()?;

    // Step 8: Mesh join for non-standalone roles
    if role != "all" && use_tailscale {
        println!();
        println!("Step 8/8 — Mesh Join");
        offer_mesh_join().await?;
    }

    println!();
    println!("Setup complete!");
    println!();
    println!("Next steps:");
    println!("  cvg status   — check platform health");
    println!("  cvg deploy status — check version");
    println!();
    Ok(())
}

// ---------------------------------------------------------------------------
// Network detection (Step 3)
// ---------------------------------------------------------------------------

fn detect_network() -> bool {
    if let Some(ts) = steps::detect_tailscale() {
        println!(
            "  Tailscale detected (IP: {})",
            if ts.ip.is_empty() { "unknown" } else { &ts.ip }
        );
        if !ts.peers.is_empty() {
            println!("  Peers found:");
            for peer in &ts.peers {
                println!("    - {peer}");
            }
        }
        let use_ts = Confirm::new()
            .with_prompt("  Use Tailscale for mesh networking?")
            .default(true)
            .interact()
            .unwrap_or(true);
        return use_ts;
    }

    let ifaces = steps::detect_lan_interfaces();
    if !ifaces.is_empty() {
        println!("  LAN mode (interfaces: {})", ifaces.join(", "));
    } else {
        println!("  LAN mode (no Tailscale detected)");
    }
    false
}

// API key configuration moved to cli_setup_inference.rs

// ---------------------------------------------------------------------------
// Defaults mode (--defaults)
// ---------------------------------------------------------------------------

async fn write_defaults() -> Result<(), CliError> {
    let node_name = steps::detect_hostname();
    let ts = steps::detect_tailscale().is_some();
    let content = steps::render_config(&node_name, "standalone", ts);
    let config_path = crate::paths::config_path();
    if let Some(parent) = config_path.parent() {
        std::fs::create_dir_all(parent).map_err(CliError::Io)?;
    }
    std::fs::write(&config_path, &content).map_err(CliError::Io)?;
    println!("Default config written to {}", config_path.display());
    print_summary(&node_name, "standalone", ts);
    Ok(())
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------

fn print_summary(name: &str, role: &str, tailscale: bool) {
    let net = if tailscale { "tailscale" } else { "lan" };
    println!("Summary:");
    println!("  Node:      {name}");
    println!("  Role:      {role}");
    println!("  Network:   {net}");
    println!("  Model:     claude-sonnet-4-6");
}

async fn offer_mesh_join() -> Result<(), CliError> {
    let join = Confirm::new()
        .with_prompt("Join an existing mesh now?")
        .default(true)
        .interact()
        .unwrap_or(false);
    if !join {
        println!("  Skipped. Run `cvg mesh join <coordinator_url>` later.");
        return Ok(());
    }
    let url: String = Input::new()
        .with_prompt("Coordinator URL (e.g. http://100.89.245.79:8420)")
        .interact_text()
        .map_err(|e| CliError::InvalidInput(e.to_string()))?;
    crate::cli_mesh_join::handle_mesh_join(&url).await
}
