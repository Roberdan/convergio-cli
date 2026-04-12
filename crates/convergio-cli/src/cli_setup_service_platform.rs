//! Platform-specific service installation: launchd (macOS) and systemd (Linux).

use crate::cli_error::CliError;
use dialoguer::Confirm;
use std::path::PathBuf;

use super::find_binary;

// ---------------------------------------------------------------------------
// macOS: launchd
// ---------------------------------------------------------------------------

pub(crate) fn setup_launchd() -> Result<Option<PathBuf>, CliError> {
    println!("  Detected: macOS (launchd)");

    let install = Confirm::new()
        .with_prompt("  Install as launchd service (auto-start on login)?")
        .default(true)
        .interact()
        .unwrap_or(false);

    if !install {
        return Ok(None);
    }

    let plist_dir = dirs::home_dir()
        .ok_or_else(|| CliError::InvalidInput("Cannot find home dir".into()))?
        .join("Library/LaunchAgents");
    std::fs::create_dir_all(&plist_dir).map_err(CliError::Io)?;

    let plist_path = plist_dir.join("com.convergio.daemon.plist");

    if plist_path.exists() {
        let backup = plist_path.with_extension("plist.bak");
        println!("  Backing up existing plist to {}", backup.display());
        std::fs::copy(&plist_path, &backup).map_err(CliError::Io)?;
    }

    let binary = find_binary();
    let data_dir = convergio_types::platform_paths::convergio_data_dir();
    let log_path = std::env::temp_dir().join("convergio-daemon.log");
    let err_path = std::env::temp_dir().join("convergio-daemon.err");

    let plist_content = format!(
        r#"<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.convergio.daemon</string>
    <key>ProgramArguments</key>
    <array>
        <string>{binary}</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
    <key>StandardOutPath</key>
    <string>{log}</string>
    <key>StandardErrorPath</key>
    <string>{err}</string>
    <key>WorkingDirectory</key>
    <string>{workdir}</string>
    <key>EnvironmentVariables</key>
    <dict>
        <key>PATH</key>
        <string>/usr/local/bin:/usr/bin:/bin:/opt/homebrew/bin</string>
    </dict>
</dict>
</plist>
"#,
        binary = binary,
        log = log_path.display(),
        err = err_path.display(),
        workdir = data_dir.display(),
    );

    std::fs::write(&plist_path, plist_content).map_err(CliError::Io)?;
    println!("  Written: {}", plist_path.display());

    let load = Confirm::new()
        .with_prompt("  Start the daemon now?")
        .default(true)
        .interact()
        .unwrap_or(false);

    if load {
        let out = std::process::Command::new("launchctl")
            .args(["load", &plist_path.to_string_lossy()])
            .output();
        match out {
            Ok(o) if o.status.success() => println!("  Daemon started."),
            _ => println!("  launchctl load failed. Start manually."),
        }
    }

    Ok(Some(plist_path))
}

// ---------------------------------------------------------------------------
// Linux: systemd
// ---------------------------------------------------------------------------

pub(crate) fn setup_systemd() -> Result<Option<PathBuf>, CliError> {
    println!("  Detected: Linux (systemd)");

    let install = Confirm::new()
        .with_prompt("  Install as systemd user service?")
        .default(true)
        .interact()
        .unwrap_or(false);

    if !install {
        return Ok(None);
    }

    let service_dir = dirs::home_dir()
        .ok_or_else(|| CliError::InvalidInput("Cannot find home dir".into()))?
        .join(".config/systemd/user");
    std::fs::create_dir_all(&service_dir).map_err(CliError::Io)?;

    let service_path = service_dir.join("convergio.service");

    if service_path.exists() {
        let backup = service_path.with_extension("service.bak");
        println!("  Backing up existing service to {}", backup.display());
        std::fs::copy(&service_path, &backup).map_err(CliError::Io)?;
    }

    let binary = find_binary();
    let data_dir = convergio_types::platform_paths::convergio_data_dir();

    let service_content = format!(
        r#"[Unit]
Description=Convergio Daemon
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
ExecStart={binary}
WorkingDirectory={workdir}
Restart=on-failure
RestartSec=5
Environment=PATH=/usr/local/bin:/usr/bin:/bin

[Install]
WantedBy=default.target
"#,
        binary = binary,
        workdir = data_dir.display(),
    );

    std::fs::write(&service_path, service_content).map_err(CliError::Io)?;
    println!("  Written: {}", service_path.display());

    let _ = std::process::Command::new("systemctl")
        .args(["--user", "daemon-reload"])
        .output();

    let enable = Confirm::new()
        .with_prompt("  Enable and start now?")
        .default(true)
        .interact()
        .unwrap_or(false);

    if enable {
        let _ = std::process::Command::new("systemctl")
            .args(["--user", "enable", "convergio"])
            .output();
        let out = std::process::Command::new("systemctl")
            .args(["--user", "start", "convergio"])
            .output();
        match out {
            Ok(o) if o.status.success() => println!("  Daemon started."),
            _ => println!("  systemctl start failed. Check: systemctl --user status convergio"),
        }
    }

    Ok(Some(service_path))
}
