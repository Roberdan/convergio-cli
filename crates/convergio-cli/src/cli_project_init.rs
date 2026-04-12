// Project init handler — scaffolds a new project locally, then registers
// with the daemon via POST /api/org/projects/onboard.

use crate::cli_error::CliError;
use std::path::Path;

/// Options for `cvg project init`.
pub struct InitOpts<'a> {
    pub name: &'a str,
    pub lang: &'a str,
    pub license: &'a str,
    pub visibility: &'a str,
    pub org_id: &'a str,
    pub template: Option<&'a str>,
    pub local: bool,
    pub api_url: &'a str,
}

/// Run the full project init flow:
/// 1. Validate name
/// 2. Create project directory with scaffold files
/// 3. Initialize git repo
/// 4. Register project with the daemon API
pub async fn handle_init(opts: &InitOpts<'_>) -> Result<(), CliError> {
    scaffold_project(opts).await?;
    register_project(opts).await?;

    eprintln!("project '{}' initialized successfully", opts.name);
    Ok(())
}

pub(crate) async fn scaffold_project(opts: &InitOpts<'_>) -> Result<(), CliError> {
    validate_name(opts.name)?;
    let root = Path::new(opts.name);
    if root.exists() {
        return Err(CliError::InvalidInput(format!(
            "directory '{}' already exists",
            opts.name
        )));
    }

    // 1. Create directory structure
    std::fs::create_dir_all(root.join(".github/workflows")).map_err(CliError::Io)?;
    std::fs::create_dir_all(root.join(".claude")).map_err(CliError::Io)?;
    std::fs::create_dir_all(root.join(".vscode")).map_err(CliError::Io)?;

    // 2. Write scaffold files
    let templates = crate::cli_project_init_templates::Templates::new(opts.name, opts.lang);
    write_file(root, ".gitignore", &templates.gitignore())?;
    write_file(root, "AGENTS.md", &templates.agents_md())?;
    write_file(root, ".claude/CLAUDE.md", &templates.claude_md())?;
    write_file(
        root,
        ".github/copilot-instructions.md",
        &templates.copilot_instructions_md(),
    )?;
    write_file(root, ".vscode/mcp.json", &templates.vscode_mcp_json())?;
    write_file(root, ".mcp.json", &templates.mcp_json())?;
    write_file(root, ".github/workflows/ci.yml", &templates.ci_yml())?;
    write_lang_files(root, opts.name, opts.lang)?;
    eprintln!("  created scaffold in {}/", opts.name);

    // 3. Initialize git repo
    run_cmd_in("git", &["init"], opts.name).await?;
    run_cmd_in("git", &["add", "."], opts.name).await?;
    run_cmd_in(
        "git",
        &[
            "commit",
            "-m",
            "feat: initial project scaffold by Convergio",
        ],
        opts.name,
    )
    .await?;
    eprintln!("  git repo initialized with first commit");
    Ok(())
}

fn validate_name(name: &str) -> Result<(), CliError> {
    if name.is_empty() {
        return Err(CliError::InvalidInput(
            "project name cannot be empty".into(),
        ));
    }
    let valid = name
        .chars()
        .all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_');
    if !valid {
        return Err(CliError::InvalidInput(
            "project name must be alphanumeric, hyphens, or underscores".into(),
        ));
    }
    Ok(())
}

fn write_file(root: &Path, rel: &str, content: &str) -> Result<(), CliError> {
    let full = root.join(rel);
    if let Some(parent) = full.parent() {
        std::fs::create_dir_all(parent).map_err(CliError::Io)?;
    }
    std::fs::write(&full, content).map_err(CliError::Io)?;
    Ok(())
}

fn write_lang_files(root: &Path, name: &str, lang: &str) -> Result<(), CliError> {
    let templates = crate::cli_project_init_templates::Templates::new(name, lang);
    match lang {
        "rust" => {
            write_file(root, "Cargo.toml", &templates.rust_cargo_toml())?;
            write_file(
                root,
                "src/main.rs",
                "fn main() {\n    println!(\"hello\");\n}\n",
            )?;
        }
        "typescript" => {
            write_file(root, "package.json", &templates.typescript_package_json())?;
            write_file(root, "tsconfig.json", templates.typescript_tsconfig())?;
            write_file(root, "src/index.ts", "console.log('hello');\n")?;
        }
        "python" => {
            write_file(root, "pyproject.toml", &templates.python_pyproject())?;
            write_file(root, "src/__init__.py", "")?;
            write_file(
                root,
                "src/main.py",
                "def main() -> None:\n    print('hello')\n\n\nif __name__ == '__main__':\n    main()\n",
            )?;
            write_file(
                root,
                "tests/test_smoke.py",
                "from src.main import main\n\n\ndef test_main_is_callable() -> None:\n    assert callable(main)\n",
            )?;
        }
        _ => {} // unknown lang — skip language-specific files
    }
    Ok(())
}

async fn register_project(opts: &InitOpts<'_>) -> Result<(), CliError> {
    let abs_path = std::env::current_dir()
        .map_err(CliError::Io)?
        .join(opts.name);
    let body = serde_json::json!({
        "repo_path": abs_path.to_string_lossy(),
    });
    let url = format!("{}/api/org/projects/onboard", opts.api_url);
    let client = crate::security::hardened_http_client();

    let max_attempts: u32 = 3;
    for attempt in 1..=max_attempts {
        let mut req = client.post(&url).json(&body);
        if let Ok(token) = std::env::var("CONVERGIO_AUTH_TOKEN") {
            let token = token.trim().to_string();
            if !token.is_empty() {
                req = req.header("Authorization", format!("Bearer {token}"));
            }
        }
        match req.send().await {
            Ok(resp) => {
                let status = resp.status();
                let val: serde_json::Value = resp.json().await.unwrap_or(serde_json::json!({}));
                if status.is_success() {
                    eprintln!("  registered project with daemon");
                    return Ok(());
                }
                let is_rate_limit = status.as_u16() == 429
                    || val
                        .get("error")
                        .and_then(|e| e.as_str())
                        .is_some_and(|s| s.to_lowercase().contains("rate limit"));
                if is_rate_limit && attempt < max_attempts {
                    let delay = attempt as u64;
                    eprintln!(
                        "  daemon rate limit exceeded — retrying in {delay}s \
                         (attempt {attempt}/{max_attempts})"
                    );
                    tokio::time::sleep(std::time::Duration::from_secs(delay)).await;
                    continue;
                }
                eprintln!("  warning: daemon registration failed: {val}");
                return Ok(());
            }
            Err(e) => {
                if attempt < max_attempts {
                    let delay = attempt as u64;
                    eprintln!(
                        "  could not reach daemon ({e}) — retrying in {delay}s \
                         (attempt {attempt}/{max_attempts})"
                    );
                    tokio::time::sleep(std::time::Duration::from_secs(delay)).await;
                    continue;
                }
                eprintln!(
                    "  warning: could not register with daemon \
                     (is it running at {}?)",
                    opts.api_url
                );
                return Ok(());
            }
        }
    }
    Ok(())
}

async fn run_cmd_in(prog: &str, args: &[&str], dir: &str) -> Result<(), CliError> {
    let status = tokio::process::Command::new(prog)
        .args(args)
        .current_dir(dir)
        .stdout(std::process::Stdio::null())
        .stderr(std::process::Stdio::null())
        .status()
        .await
        .map_err(|e| CliError::ApiCallFailed(format!("failed to run {prog}: {e}")))?;
    if !status.success() {
        return Err(CliError::ApiCallFailed(format!(
            "{prog} exited with {}",
            status.code().unwrap_or(-1)
        )));
    }
    Ok(())
}

#[cfg(test)]
#[path = "cli_project_init_tests.rs"]
mod tests;
