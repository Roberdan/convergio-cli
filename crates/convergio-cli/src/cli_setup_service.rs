// Setup step: System service installation.
// macOS: launchd plist, Linux: systemd unit, Windows: service
// Handles backup, rollback, permission checks.

#[path = "cli_setup_service_platform.rs"]
mod cli_setup_service_platform;

use crate::cli_error::CliError;
use dialoguer::Confirm;
use std::path::PathBuf;

/// Interactive service installation. Returns path to installed config.
pub(crate) fn setup_service() -> Result<Option<PathBuf>, CliError> {
    println!();
    println!("System Service Installation");
    println!("---------------------------");

    let platform = detect_service_platform();
    match platform {
        ServicePlatform::Launchd => cli_setup_service_platform::setup_launchd(),
        ServicePlatform::Systemd => cli_setup_service_platform::setup_systemd(),
        ServicePlatform::WindowsService => setup_windows(),
        ServicePlatform::Unsupported => {
            println!("  No supported service manager detected.");
            println!("  Run the daemon manually: convergio");
            Ok(None)
        }
    }
}

pub(crate) enum ServicePlatform {
    Launchd,
    Systemd,
    WindowsService,
    Unsupported,
}

fn detect_service_platform() -> ServicePlatform {
    if cfg!(target_os = "macos") {
        ServicePlatform::Launchd
    } else if cfg!(target_os = "linux") {
        if std::path::Path::new("/run/systemd/system").exists() {
            ServicePlatform::Systemd
        } else {
            ServicePlatform::Unsupported
        }
    } else if cfg!(target_os = "windows") {
        ServicePlatform::WindowsService
    } else {
        ServicePlatform::Unsupported
    }
}

fn setup_windows() -> Result<Option<PathBuf>, CliError> {
    println!("  Detected: Windows");
    println!("  Windows service installation is not yet automated.");
    println!("  Run the daemon manually: convergio.exe");
    println!("  Or use NSSM to install as a service:");
    println!("    nssm install Convergio <path-to-convergio.exe>");
    Ok(None)
}

pub(crate) fn find_binary() -> String {
    if let Ok(exe) = std::env::current_exe() {
        let parent = exe.parent().unwrap_or(std::path::Path::new("."));
        let convergio = parent.join("convergio");
        if convergio.exists() {
            return convergio.to_string_lossy().to_string();
        }
    }
    for path in &["/usr/local/bin/convergio", "/opt/homebrew/bin/convergio"] {
        if std::path::Path::new(path).exists() {
            return path.to_string();
        }
    }
    "convergio".to_string()
}

/// Detect and offer Tailscale installation.
#[allow(dead_code, clippy::needless_return)]
pub(crate) fn offer_tailscale_install() -> bool {
    if is_tailscale_installed() {
        return true;
    }

    println!();
    println!("  Tailscale is not installed.");
    println!("  Tailscale provides secure mesh networking between nodes.");

    let install = Confirm::new()
        .with_prompt("  Install Tailscale?")
        .default(false)
        .interact()
        .unwrap_or(false);

    if !install {
        return false;
    }

    #[cfg(target_os = "macos")]
    {
        println!("  Installing via Homebrew...");
        let out = std::process::Command::new("brew")
            .args(["install", "--cask", "tailscale"])
            .output();
        return match out {
            Ok(o) if o.status.success() => {
                println!("  Tailscale installed. Open Tailscale.app to log in.");
                true
            }
            _ => {
                println!("  Install failed. Visit https://tailscale.com/download");
                false
            }
        };
    }

    #[cfg(target_os = "linux")]
    {
        println!("  Installing via official script...");
        let out = std::process::Command::new("sh")
            .args(["-c", "curl -fsSL https://tailscale.com/install.sh | sh"])
            .output();
        match out {
            Ok(o) if o.status.success() => {
                println!("  Tailscale installed. Run: sudo tailscale up");
                return true;
            }
            _ => {
                println!("  Install failed. Visit https://tailscale.com/download");
                return false;
            }
        }
    }

    #[cfg(target_os = "windows")]
    {
        println!("  Download Tailscale from: https://tailscale.com/download");
        return false;
    }

    #[cfg(not(any(target_os = "macos", target_os = "linux", target_os = "windows")))]
    false
}

#[allow(dead_code)]
fn is_tailscale_installed() -> bool {
    std::process::Command::new("tailscale")
        .arg("version")
        .output()
        .map(|o| o.status.success())
        .unwrap_or(false)
}
