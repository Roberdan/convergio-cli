// Local path helpers — replaces convergio_core::platform_paths and config.
// CLI is a pure HTTP client; these are the only local filesystem paths it needs.

use std::path::PathBuf;

/// Root directory for Convergio local data: ~/.convergio
pub fn convergio_dir() -> PathBuf {
    dirs::home_dir()
        .unwrap_or_else(|| PathBuf::from("."))
        .join(".convergio")
}

/// Path to the main config file: ~/.convergio/config.toml
pub fn config_path() -> PathBuf {
    convergio_dir().join("config.toml")
}

/// Path to the env file: ~/.convergio/env
pub fn env_file_path() -> PathBuf {
    convergio_dir().join("env")
}

/// Output directory for a project: ~/.convergio/projects/{name}/output
/// Validates `name` to prevent path traversal.
pub fn project_output_dir(name: &str) -> PathBuf {
    if let Err(e) = crate::security::validate_identifier(name, "project name") {
        eprintln!("error: {e}");
        // Return a safe fallback that won't create dirs in unexpected places
        convergio_dir()
            .join("projects")
            .join("_invalid")
            .join("output")
    } else {
        convergio_dir().join("projects").join(name).join("output")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn config_path_ends_with_config_toml() {
        let p = config_path();
        assert!(p.ends_with("config.toml"));
    }

    #[test]
    fn project_output_dir_contains_project_name() {
        let p = project_output_dir("acme");
        assert!(p.to_string_lossy().contains("acme"));
        assert!(p.ends_with("output"));
    }
}
