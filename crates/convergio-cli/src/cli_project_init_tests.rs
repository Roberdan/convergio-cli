use super::*;

#[test]
fn valid_names_accepted() {
    assert!(validate_name("my-project").is_ok());
    assert!(validate_name("cool_app").is_ok());
    assert!(validate_name("app123").is_ok());
}

#[test]
fn invalid_names_rejected() {
    assert!(validate_name("").is_err());
    assert!(validate_name("has space").is_err());
    assert!(validate_name("path/traversal").is_err());
    assert!(validate_name("special!chars").is_err());
}

#[test]
fn scaffold_creates_files() {
    let tmp = tempfile::tempdir().unwrap();
    let root = tmp.path().join("test-proj");
    std::fs::create_dir_all(root.join(".github/workflows")).unwrap();
    std::fs::create_dir_all(root.join(".claude")).unwrap();
    std::fs::create_dir_all(root.join(".vscode")).unwrap();

    let tpl = crate::cli_project_init_templates::Templates::new("test-proj", "rust");
    write_file(&root, ".gitignore", &tpl.gitignore()).unwrap();
    write_file(&root, "AGENTS.md", &tpl.agents_md()).unwrap();
    write_file(&root, ".claude/CLAUDE.md", &tpl.claude_md()).unwrap();
    write_file(
        &root,
        ".github/copilot-instructions.md",
        &tpl.copilot_instructions_md(),
    )
    .unwrap();
    write_file(&root, ".vscode/mcp.json", &tpl.vscode_mcp_json()).unwrap();
    write_file(&root, ".mcp.json", &tpl.mcp_json()).unwrap();
    write_file(&root, ".github/workflows/ci.yml", &tpl.ci_yml()).unwrap();
    write_lang_files(&root, "test-proj", "rust").unwrap();

    assert!(root.join(".gitignore").exists());
    assert!(root.join("AGENTS.md").exists());
    assert!(root.join(".claude/CLAUDE.md").exists());
    assert!(root.join(".github/copilot-instructions.md").exists());
    assert!(root.join(".vscode/mcp.json").exists());
    assert!(root.join(".mcp.json").exists());
    assert!(root.join(".github/workflows/ci.yml").exists());
    assert!(root.join("Cargo.toml").exists());
    assert!(root.join("src/main.rs").exists());

    let agents = std::fs::read_to_string(root.join("AGENTS.md")).unwrap();
    assert!(agents.contains("Convergio"), "AGENTS.md mentions Convergio");
    assert!(agents.contains("Gate chain"), "AGENTS.md has gate chain");
    let mcp = std::fs::read_to_string(root.join(".vscode/mcp.json")).unwrap();
    assert!(
        mcp.contains("convergio-mcp-server"),
        "MCP config has server"
    );
}

#[test]
fn gitignore_matches_language() {
    let rust_tpl = crate::cli_project_init_templates::Templates::new("p", "rust");
    assert!(rust_tpl.gitignore().contains("target/"));

    let ts_tpl = crate::cli_project_init_templates::Templates::new("p", "typescript");
    assert!(ts_tpl.gitignore().contains("node_modules/"));

    let py_tpl = crate::cli_project_init_templates::Templates::new("p", "python");
    assert!(py_tpl.gitignore().contains("__pycache__/"));
}

#[test]
fn non_rust_scaffolds_create_manifests() {
    let tmp = tempfile::tempdir().unwrap();

    let ts_root = tmp.path().join("ts-proj");
    write_lang_files(&ts_root, "ts-proj", "typescript").unwrap();
    assert!(ts_root.join("package.json").exists());
    assert!(ts_root.join("tsconfig.json").exists());

    let py_root = tmp.path().join("py-proj");
    write_lang_files(&py_root, "py-proj", "python").unwrap();
    assert!(py_root.join("pyproject.toml").exists());
    assert!(py_root.join("tests/test_smoke.py").exists());
}
