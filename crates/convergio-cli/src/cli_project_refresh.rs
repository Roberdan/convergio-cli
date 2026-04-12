// Refresh onboarding files in an existing project.
// Regenerates AGENTS.md, MCP configs, and agent instruction files
// without touching source code, CI config, or language-specific files.

use crate::cli_error::CliError;
use std::path::Path;

/// Options for `cvg project refresh`.
pub struct RefreshOpts<'a> {
    pub path: &'a Path,
    pub lang: Option<&'a str>,
}

/// Regenerate onboarding files in an existing project directory.
pub fn handle_refresh(opts: &RefreshOpts<'_>) -> Result<(), CliError> {
    if !opts.path.exists() {
        return Err(CliError::InvalidInput(format!(
            "directory '{}' does not exist",
            opts.path.display()
        )));
    }

    let name = opts
        .path
        .file_name()
        .and_then(|n| n.to_str())
        .unwrap_or("project");

    let lang = match opts.lang {
        Some(l) => l.to_string(),
        None => detect_language(opts.path),
    };

    // Ensure directories exist
    let dirs = [".github", ".claude", ".vscode"];
    for d in &dirs {
        let dir = opts.path.join(d);
        if !dir.exists() {
            std::fs::create_dir_all(&dir).map_err(CliError::Io)?;
        }
    }

    let tpl = crate::cli_project_init_templates::Templates::new(name, &lang);

    // Write onboarding files (overwrite existing)
    write(opts.path, "AGENTS.md", &tpl.agents_md())?;
    write(opts.path, ".claude/CLAUDE.md", &tpl.claude_md())?;
    write(
        opts.path,
        ".github/copilot-instructions.md",
        &tpl.copilot_instructions_md(),
    )?;
    write(opts.path, ".vscode/mcp.json", &tpl.vscode_mcp_json())?;
    write(opts.path, ".mcp.json", &tpl.mcp_json())?;

    eprintln!("refreshed onboarding files in {}/", opts.path.display());
    eprintln!("  ✓ AGENTS.md");
    eprintln!("  ✓ .claude/CLAUDE.md");
    eprintln!("  ✓ .github/copilot-instructions.md");
    eprintln!("  ✓ .vscode/mcp.json");
    eprintln!("  ✓ .mcp.json");
    eprintln!("  detected language: {lang}");
    Ok(())
}

/// Detect project language from manifest files.
fn detect_language(path: &Path) -> String {
    if path.join("Cargo.toml").exists() {
        "rust".into()
    } else if path.join("package.json").exists() {
        "typescript".into()
    } else if path.join("pyproject.toml").exists()
        || path.join("requirements.txt").exists()
        || path.join("setup.py").exists()
    {
        "python".into()
    } else {
        eprintln!("  warning: could not detect language, defaulting to rust");
        "rust".into()
    }
}

fn write(root: &Path, rel: &str, content: &str) -> Result<(), CliError> {
    let full = root.join(rel);
    if let Some(parent) = full.parent() {
        std::fs::create_dir_all(parent).map_err(CliError::Io)?;
    }
    std::fs::write(&full, content).map_err(CliError::Io)?;
    Ok(())
}

#[cfg(test)]
#[path = "cli_project_refresh_tests.rs"]
mod tests;
