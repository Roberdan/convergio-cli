use super::*;

#[test]
fn agents_md_has_convergio_process() {
    let tpl = Templates::new("acme-app", "rust");
    let md = tpl.agents_md();
    assert!(md.contains("acme-app"), "has project name");
    assert!(md.contains("Plan lifecycle"), "has plan docs");
    assert!(md.contains("Gate chain"), "has gate chain");
    assert!(
        md.contains("Adversarial workflow"),
        "has adversarial workflow (ADR-039)"
    );
    assert!(md.contains("cvg status"), "has CLI commands");
    assert!(md.contains("MCP"), "has MCP section");
    assert!(md.contains("Rust"), "has language label");
}

#[test]
fn agents_md_python_has_correct_commands() {
    let tpl = Templates::new("py-proj", "python");
    let md = tpl.agents_md();
    assert!(md.contains("pytest"), "Python has pytest");
    assert!(md.contains("ruff"), "Python has ruff");
    assert!(md.contains("Python"), "has Python label");
    assert!(!md.contains("cargo"), "no cargo in Python");
}

#[test]
fn claude_md_points_to_agents_md() {
    let tpl = Templates::new("acme-app", "rust");
    let md = tpl.claude_md();
    assert!(md.contains("AGENTS.md"), "must reference AGENTS.md");
    assert!(md.contains("acme-app"));
}

#[test]
fn claude_md_rust_has_cargo_commands() {
    let tpl = Templates::new("rs-proj", "rust");
    let md = tpl.claude_md();
    assert!(md.contains("cargo check"), "rust has cargo check");
    assert!(md.contains("cargo clippy"), "rust has cargo clippy");
}

#[test]
fn claude_md_python_has_python_commands() {
    let tpl = Templates::new("py-proj", "python");
    let md = tpl.claude_md();
    assert!(md.contains("pytest"), "python has pytest");
    assert!(md.contains("ruff"), "python has ruff");
    assert!(!md.contains("cargo"), "python has no cargo");
}

#[test]
fn claude_md_typescript_has_npm_commands() {
    let tpl = Templates::new("ts-proj", "typescript");
    let md = tpl.claude_md();
    assert!(md.contains("npm install"), "ts has npm install");
    assert!(md.contains("npm test"), "ts has npm test");
    assert!(!md.contains("cargo"), "ts has no cargo");
}

#[test]
fn copilot_instructions_md_points_to_agents() {
    let tpl = Templates::new("proj", "rust");
    let md = tpl.copilot_instructions_md();
    assert!(md.contains("AGENTS.md"), "must reference AGENTS.md");
    assert!(md.contains("cvg copilot register"));
}

#[test]
fn mcp_configs_have_convergio_server() {
    let tpl = Templates::new("proj", "rust");
    assert!(tpl.vscode_mcp_json().contains("convergio-mcp-server"));
    assert!(tpl.mcp_json().contains("convergio-mcp-server"));
}

#[test]
fn ci_yml_rust_has_clippy() {
    let tpl = Templates::new("proj", "rust");
    let ci = tpl.ci_yml();
    assert!(ci.contains("cargo clippy"));
    assert!(ci.contains("cargo test"));
}

#[test]
fn ci_yml_typescript_has_npm() {
    let tpl = Templates::new("proj", "typescript");
    let ci = tpl.ci_yml();
    assert!(ci.contains("npm install"));
    assert!(ci.contains("npm test"));
}

#[test]
fn ci_yml_python_has_pytest() {
    let tpl = Templates::new("proj", "python");
    let ci = tpl.ci_yml();
    assert!(ci.contains("pytest"));
    assert!(ci.contains("ruff"));
}

#[test]
fn unknown_lang_defaults_to_rust() {
    let tpl = Templates::new("proj", "go");
    let gi = tpl.gitignore();
    assert!(gi.contains("target/"));
    let ci = tpl.ci_yml();
    assert!(ci.contains("cargo check"));
}

#[test]
fn manifests_match_selected_language() {
    let rust = Templates::new("rust-proj", "rust");
    assert!(rust.rust_cargo_toml().contains("name = \"rust-proj\""));

    let ts = Templates::new("ts-proj", "typescript");
    assert!(ts
        .typescript_package_json()
        .contains("\"build\": \"tsc -p .\""));
    assert!(ts.typescript_tsconfig().contains("\"NodeNext\""));

    let py = Templates::new("py-proj", "python");
    assert!(py.python_pyproject().contains("pytest>=8.0"));
}
