use super::*;

#[test]
fn refresh_creates_onboarding_files() {
    let tmp = tempfile::tempdir().unwrap();
    let root = tmp.path().join("my-project");
    std::fs::create_dir_all(&root).unwrap();
    // Create a Cargo.toml so language detection picks Rust
    std::fs::write(
        root.join("Cargo.toml"),
        "[package]\nname = \"my-project\"\n",
    )
    .unwrap();

    let opts = RefreshOpts {
        path: &root,
        lang: None,
    };
    handle_refresh(&opts).unwrap();

    assert!(root.join("AGENTS.md").exists());
    assert!(root.join(".claude/CLAUDE.md").exists());
    assert!(root.join(".github/copilot-instructions.md").exists());
    assert!(root.join(".vscode/mcp.json").exists());
    assert!(root.join(".mcp.json").exists());

    let agents = std::fs::read_to_string(root.join("AGENTS.md")).unwrap();
    assert!(agents.contains("Convergio"));
    assert!(agents.contains("cargo"));
}

#[test]
fn refresh_detects_typescript() {
    let tmp = tempfile::tempdir().unwrap();
    let root = tmp.path().join("ts-app");
    std::fs::create_dir_all(&root).unwrap();
    std::fs::write(root.join("package.json"), "{}").unwrap();

    let opts = RefreshOpts {
        path: &root,
        lang: None,
    };
    handle_refresh(&opts).unwrap();

    let agents = std::fs::read_to_string(root.join("AGENTS.md")).unwrap();
    assert!(agents.contains("npm"), "should have npm commands for TS");
}

#[test]
fn refresh_detects_python() {
    let tmp = tempfile::tempdir().unwrap();
    let root = tmp.path().join("py-app");
    std::fs::create_dir_all(&root).unwrap();
    std::fs::write(root.join("pyproject.toml"), "[project]\nname=\"py-app\"\n").unwrap();

    let opts = RefreshOpts {
        path: &root,
        lang: None,
    };
    handle_refresh(&opts).unwrap();

    let agents = std::fs::read_to_string(root.join("AGENTS.md")).unwrap();
    assert!(agents.contains("pytest"), "should have pytest for Python");
}

#[test]
fn refresh_respects_lang_override() {
    let tmp = tempfile::tempdir().unwrap();
    let root = tmp.path().join("override-app");
    std::fs::create_dir_all(&root).unwrap();
    // Has Cargo.toml but we force python
    std::fs::write(root.join("Cargo.toml"), "[package]").unwrap();

    let opts = RefreshOpts {
        path: &root,
        lang: Some("python"),
    };
    handle_refresh(&opts).unwrap();

    let agents = std::fs::read_to_string(root.join("AGENTS.md")).unwrap();
    assert!(agents.contains("pytest"), "lang override should win");
}

#[test]
fn refresh_does_not_touch_source_files() {
    let tmp = tempfile::tempdir().unwrap();
    let root = tmp.path().join("src-safe");
    std::fs::create_dir_all(root.join("src")).unwrap();
    std::fs::write(root.join("src/main.rs"), "fn main() {}").unwrap();
    std::fs::write(root.join("Cargo.toml"), "[package]").unwrap();

    let opts = RefreshOpts {
        path: &root,
        lang: None,
    };
    handle_refresh(&opts).unwrap();

    let src = std::fs::read_to_string(root.join("src/main.rs")).unwrap();
    assert_eq!(src, "fn main() {}", "source file must not be modified");
}

#[test]
fn refresh_mcp_config_content() {
    let tmp = tempfile::tempdir().unwrap();
    let root = tmp.path().join("mcp-check");
    std::fs::create_dir_all(&root).unwrap();

    let opts = RefreshOpts {
        path: &root,
        lang: Some("rust"),
    };
    handle_refresh(&opts).unwrap();

    let vscode_mcp = std::fs::read_to_string(root.join(".vscode/mcp.json")).unwrap();
    assert!(vscode_mcp.contains("convergio-mcp-server"));
    assert!(vscode_mcp.contains("servers"));

    let claude_mcp = std::fs::read_to_string(root.join(".mcp.json")).unwrap();
    assert!(claude_mcp.contains("convergio-mcp-server"));
    assert!(claude_mcp.contains("mcpServers"));
}

#[test]
fn refresh_fails_on_missing_dir() {
    let opts = RefreshOpts {
        path: Path::new("/nonexistent/path/xyz"),
        lang: None,
    };
    assert!(handle_refresh(&opts).is_err());
}
