// cli_project_switch — `cvg project switch <id>` — set active project.

use crate::cli_error::CliError;

/// Write the active project ID to `~/.convergio/active_project`.
pub async fn handle_switch(project_id: &str) -> Result<(), CliError> {
    let home = dirs::home_dir()
        .ok_or_else(|| CliError::InvalidInput("could not determine home directory".to_string()))?;
    let config_dir = home.join(".convergio");
    std::fs::create_dir_all(&config_dir).map_err(CliError::Io)?;

    let file = config_dir.join("active_project");
    std::fs::write(&file, project_id).map_err(CliError::Io)?;

    eprintln!("Active project set to: {project_id}");
    eprintln!("Written to: {}", file.display());
    Ok(())
}
