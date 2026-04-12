// Template generators for `cvg project init` scaffold files.

/// Resolve the absolute path to `convergio-mcp-server`.
///
/// Strategy (cross-platform):
/// 1. Same directory as the running CLI binary (most reliable)
/// 2. `which`/`where` lookup (if installed system-wide)
/// 3. Fallback to bare name (user must have it in PATH)
pub fn resolve_mcp_server_path() -> String {
    // 1. Sibling of current executable
    if let Ok(exe) = std::env::current_exe() {
        if let Some(dir) = exe.parent() {
            let candidate = dir.join("convergio-mcp-server");
            if candidate.exists() {
                return candidate.to_string_lossy().into_owned();
            }
        }
    }
    // 2. PATH lookup
    let cmd = if cfg!(windows) { "where" } else { "which" };
    if let Ok(out) = std::process::Command::new(cmd)
        .arg("convergio-mcp-server")
        .output()
    {
        if out.status.success() {
            let path = String::from_utf8_lossy(&out.stdout).trim().to_string();
            if !path.is_empty() {
                return path;
            }
        }
    }
    // 3. Bare name fallback
    "convergio-mcp-server".into()
}

/// Holds project metadata used to generate scaffold templates.
pub struct Templates<'a> {
    name: &'a str,
    lang: &'a str,
}

impl<'a> Templates<'a> {
    pub fn new(name: &'a str, lang: &'a str) -> Self {
        Self { name, lang }
    }

    pub fn gitignore(&self) -> String {
        let common = ".env\n.DS_Store\n*.log\n";
        let lang_specific = match self.lang {
            "rust" => "target/\nCargo.lock\n",
            "typescript" => "node_modules/\ndist/\n*.tsbuildinfo\n",
            "python" => "__pycache__/\n*.pyc\n.venv/\ndist/\n*.egg-info/\n",
            _ => "target/\nCargo.lock\n",
        };
        format!("{common}{lang_specific}")
    }

    fn lang_label(&self) -> &str {
        match self.lang {
            "rust" => "Rust",
            "typescript" => "TypeScript",
            "python" => "Python",
            other => other,
        }
    }

    fn running_section(&self) -> &str {
        match self.lang {
            "rust" => "## Running\n\n```bash\ncargo check --workspace\ncargo test --workspace\n```",
            "typescript" => "## Running\n\n```bash\nnpm install\nnpm run build\nnpm test\n```",
            "python" => "## Running\n\n```bash\npip install -e \".[dev]\"\npytest\n```",
            _ => "## Running\n\n```bash\ncargo check --workspace\ncargo test --workspace\n```",
        }
    }

    fn linting_section(&self) -> &str {
        match self.lang {
            "rust" => "## Linting\n\n```bash\ncargo clippy --workspace -- -D warnings\ncargo fmt --all -- --check\n```",
            "typescript" => "## Linting\n\n```bash\nnpm run lint\n```",
            "python" => "## Linting\n\n```bash\nruff check .\n```",
            _ => "## Linting\n\n```bash\ncargo clippy --workspace -- -D warnings\n```",
        }
    }

    pub fn agents_md(&self) -> String {
        format!(
            r#"# {name} — Agent Operating Manual

Read this file FIRST if you are an AI agent working on this project.

## What is this project

{name} is a Convergio-managed project.

**Stack:** {lang}
**Managed by:** [Convergio](https://github.com/Roberdan/convergio)

## How to connect (MCP)

This project is pre-configured with the Convergio MCP server. Your IDE or agent runtime
should automatically discover it via `.vscode/mcp.json` (Copilot / VS Code) or
`.mcp.json` (Claude Code).

The MCP server exposes ~43 tools for plan management, task execution, evidence recording,
agent spawning, skills, and more. Use `tools/list` to see all available tools.

## Convergio process (mandatory)

This project follows the Convergio plan-based workflow:

### Plan lifecycle
1. **Plan** → group of tasks organized in waves (sequential phases)
2. **Wave** → set of parallelizable tasks (all must pass before next wave)
3. **Task** → atomic unit of work (one branch, one PR)

### Task lifecycle
```
pending → in_progress → submitted → done (only Thor validates)
```

1. Create worktree: `git worktree add -b <branch> .worktrees/<name> main`
2. Implement the task
3. Run tests and linting
4. Commit with conventional message (`feat:`, `fix:`, `docs:`)
5. Push and create PR
6. Record evidence: `POST /api/plan-db/task/evidence`
7. Update task status to `submitted`
8. Thor validates — only Thor can set `done`

### Gate chain
```
EvidenceGate → TestGate → PrCommitGate → WaveSequenceGate → ValidatorGate
```

### Adversarial workflow (ADR-039)

Before building, Convergio challenges the approach:
- **Devil's Advocate** (`/solve`): argues against proposed approach before spec
- **Spec Stress Test** (`/planner`): finds weakest tasks and failure modes
- **Thinking Advisors** (`/execute`): simplification + risk review alongside implementation

## Key CLI commands

| Command | What it does |
|---------|-------------|
| `cvg status` | System overview |
| `cvg plan list` | List plans |
| `cvg plan show <id>` | Plan details |
| `cvg task complete <id>` | Mark task done |
| `cvg solve` | End-to-end workflow |
| `cvg copilot register` | Register Copilot session |
| `cvg claude register` | Register Claude session |

## Rules

- Code and docs in **English**
- Max **300 lines per file**
- Conventional commits
- Every PR must pass CI before merge
- One worktree per task, never work on main

{running}

{linting}
"#,
            name = self.name,
            lang = self.lang_label(),
            running = self.running_section(),
            linting = self.linting_section(),
        )
    }

    pub fn claude_md(&self) -> String {
        format!(
            r#"# {name} — CLAUDE.md

Read **AGENTS.md** first — it contains all project rules and the Convergio process.

## Claude-specific

- Co-authored-by: `Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`
- Manage context: checkpoint at 70-80% capacity
- Use `cvg claude register` to register your session

{running}

{linting}

## Testing

- Write tests first (RED-GREEN-REFACTOR).
- 80% coverage for business logic, 100% for critical paths.
- Mock external boundaries only (network, filesystem, time).
"#,
            name = self.name,
            running = self.running_section(),
            linting = self.linting_section(),
        )
    }

    pub fn copilot_instructions_md(&self) -> String {
        format!(
            r#"# {name} — Copilot Instructions

Read **AGENTS.md** in the repo root for all project rules and the Convergio process.

## Copilot-specific

- Use `cvg copilot register` to register your session with the daemon
- Co-authored-by: `Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`
- Follow the task lifecycle in AGENTS.md
- The daemon runs at `http://localhost:8420`
"#,
            name = self.name,
        )
    }

    pub fn vscode_mcp_json(&self) -> String {
        let bin = resolve_mcp_server_path();
        format!(
            r#"{{
  "servers": {{
    "convergio": {{
      "command": "{}",
      "args": ["--transport", "stdio"],
      "env": {{
        "CONVERGIO_MCP_RING": "1"
      }}
    }}
  }}
}}
"#,
            bin
        )
    }

    pub fn mcp_json(&self) -> String {
        let bin = resolve_mcp_server_path();
        format!(
            r#"{{
  "mcpServers": {{
    "convergio": {{
      "command": "{}",
      "args": ["--transport", "stdio"],
      "env": {{
        "CONVERGIO_MCP_RING": "1"
      }}
    }}
  }}
}}
"#,
            bin
        )
    }

    pub fn rust_cargo_toml(&self) -> String {
        format!(
            "[package]\nname = \"{}\"\nversion = \"0.1.0\"\nedition = \"2024\"\n\n[dependencies]\n",
            self.name
        )
    }

    pub fn typescript_package_json(&self) -> String {
        format!(
            "{{\n  \"name\": \"{}\",\n  \"version\": \"0.1.0\",\n  \"private\": true,\n  \"type\": \"module\",\n  \"scripts\": {{\n    \"build\": \"tsc -p .\",\n    \"lint\": \"tsc --noEmit\",\n    \"test\": \"npm run build\"\n  }},\n  \"devDependencies\": {{\n    \"typescript\": \"^5.9.3\"\n  }}\n}}\n",
            self.name
        )
    }

    pub fn typescript_tsconfig(&self) -> &'static str {
        "{\n  \"compilerOptions\": {\n    \"target\": \"ES2022\",\n    \"module\": \"NodeNext\",\n    \"moduleResolution\": \"NodeNext\",\n    \"rootDir\": \"src\",\n    \"outDir\": \"dist\",\n    \"strict\": true\n  },\n  \"include\": [\"src/**/*.ts\"]\n}\n"
    }

    pub fn python_pyproject(&self) -> String {
        format!(
            "[build-system]\nrequires = [\"setuptools>=68\"]\nbuild-backend = \"setuptools.build_meta\"\n\n[project]\nname = \"{}\"\nversion = \"0.1.0\"\ndescription = \"{} scaffolded by Convergio\"\nrequires-python = \">=3.12\"\n\n[project.optional-dependencies]\ndev = [\n  \"pytest>=8.0\",\n  \"ruff>=0.5\"\n]\n",
            self.name, self.name
        )
    }

    pub fn ci_yml(&self) -> String {
        let header = "name: CI\non:\n  push:\n    branches: [main]\n  pull_request:\n    branches: [main]\njobs:\n  check:\n    runs-on: ubuntu-latest\n    steps:\n      - uses: actions/checkout@v4\n";
        let steps = match self.lang {
            "typescript" => "      - uses: actions/setup-node@v4\n        with:\n          node-version: '20'\n      - run: npm install\n      - run: npm run lint\n      - run: npm test\n",
            "python" => "      - uses: actions/setup-python@v5\n        with:\n          python-version: '3.12'\n      - run: pip install -e \".[dev]\"\n      - run: ruff check .\n      - run: pytest\n",
            _ => "      - uses: dtolnay/rust-toolchain@stable\n        with:\n          components: clippy\n      - name: cargo check\n        run: cargo check --workspace\n      - name: cargo test\n        run: cargo test --workspace\n      - name: cargo clippy\n        run: cargo clippy --workspace -- -D warnings\n",
        };
        format!("{header}{steps}")
    }
}

#[cfg(test)]
#[path = "cli_project_init_tpl_tests.rs"]
mod tests;
