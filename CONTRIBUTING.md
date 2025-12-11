# Contributing to Convergio CLI

First off, thank you for considering contributing to Convergio CLI! It's people like you that make this project better.

## Code of Conduct

This project and everyone participating in it is governed by our [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code.

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check the existing issues to avoid duplicates. When you create a bug report, include as many details as possible:

- **Use a clear and descriptive title**
- **Describe the exact steps to reproduce the problem**
- **Describe the behavior you observed and what you expected**
- **Include your macOS version and Apple Silicon chip model**
- **Include any error messages or logs** (use `--debug` or `--trace` flags)

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion:

- **Use a clear and descriptive title**
- **Provide a detailed description of the proposed functionality**
- **Explain why this enhancement would be useful**
- **List any alternatives you've considered**

### Pull Requests

1. **Fork the repository** and create your branch from `main`
2. **Follow the coding style** (see below)
3. **Test your changes** thoroughly
4. **Update documentation** if needed
5. **Write a clear commit message**

## Development Setup

### Prerequisites

- macOS 14+ (Sonoma)
- Apple Silicon (M1/M2/M3/M4)
- Xcode Command Line Tools
- Anthropic API key for testing

### Building

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/kernel.git
cd kernel

# Build
make clean && make

# Run with debug logging
export ANTHROPIC_API_KEY="your-key"
./build/bin/convergio --debug
```

### Project Structure

```
src/
├── core/           # Main entry point, semantic fabric
├── orchestrator/   # Ali, agent registry, message bus, cost tracking
├── neural/         # Claude API integration, MLX embeddings
├── memory/         # SQLite persistence, RAG
├── tools/          # Tool execution (file, shell, web)
├── agents/         # Agent definitions
├── intent/         # Natural language parsing
├── metal/          # GPU compute
└── runtime/        # Scheduler
```

## Coding Style

### C Code

- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style (opening brace on same line)
- **Naming**:
  - Functions: `snake_case` (e.g., `nous_claude_chat`)
  - Types: `PascalCase` (e.g., `SemanticNode`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_TOKENS`)
  - Global variables: `g_` prefix (e.g., `g_orchestrator`)
- **Comments**: Use `//` for single-line, `/* */` for multi-line
- **Headers**: Include guards using `#ifndef`/`#define`/`#endif`

### Example

```c
// Good
int nous_process_input(const char* input) {
    if (!input) {
        return -1;
    }

    // Process the input
    char* result = do_something(input);
    if (result) {
        free(result);
    }

    return 0;
}
```

### Commit Messages

Follow conventional commits format:

```
type(scope): description
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `style`: Code style (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding tests
- `chore`: Maintenance tasks

Examples:
```
feat(agents): Add new data-analyst agent
fix(curl): Make API calls thread-safe
docs(readme): Update installation instructions
```

## Adding New Features

### Adding a New Agent

1. Create a markdown file in `src/agents/definitions/`
2. Follow the existing format with YAML frontmatter
3. Define the agent's role, system prompt, and tools

### Adding a New Tool

1. Add the tool definition to `TOOLS_JSON` in `src/tools/tools.c`
2. Implement the handler function
3. Add the case to `tool_execute()`
4. Update the README tools table

### Adding a New Command

1. Add the command handler in `src/core/main.c`
2. Add it to the `COMMANDS` array
3. Update the `cmd_help()` output

## Questions?

Feel free to open an issue for any questions about contributing.

Thank you for contributing!
