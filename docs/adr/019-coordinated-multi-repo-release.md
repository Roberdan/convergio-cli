# ADR-019: Coordinated Multi-Repository Release System

## Status
Accepted

## Date
2025-12-21

## Context

Convergio consists of two main repositories that must be released in coordination:

1. **ConvergioCLI** - The CLI tool and ACP server (`convergio`, `convergio-acp`)
2. **convergio-zed** - The Zed fork with Convergio panels (`Convergio Studio`)

These repositories have interdependencies:
- Convergio Studio embeds `convergio-acp` for agent communication
- Both share the ACP protocol version
- Users need compatible versions of CLI and Studio

Current state:
- ConvergioCLI has full release automation (build, sign, notarize, Homebrew)
- convergio-zed has basic release workflow (builds, no signing)

## Decision

### Version Strategy

Each repository maintains independent version numbers:

| Repository | Version File | Current | Format |
|------------|--------------|---------|--------|
| ConvergioCLI | `VERSION` | 5.4.0 | SemVer |
| convergio-zed | `CONVERGIO_VERSION` | 0.1.0 | SemVer |
| convergio-zed | `VERSION` (upstream) | 0.219.0 | Zed version |

**Rationale**: Independent versions allow:
- CLI to iterate faster on agent features
- Studio to track Zed upstream merges separately
- Clear distinction between Convergio features and Zed base

### ACP Protocol Compatibility

A new file `ACP_PROTOCOL_VERSION` will be added to both repos:
- Currently: `1.0` (all existing implementations)
- Bumped only when breaking protocol changes occur
- Release workflows verify protocol compatibility

### Release Artifacts

| Artifact | Source | Distribution | Signed |
|----------|--------|--------------|--------|
| `convergio` CLI | ConvergioCLI | Homebrew tap, GitHub Releases | Yes |
| `convergio-acp` | ConvergioCLI | Bundled with CLI | Yes |
| `Convergio Studio.dmg` | convergio-zed | GitHub Releases | Yes (planned) |
| `convergio-studio` (Linux) | convergio-zed | GitHub Releases | N/A |

### Release Process

#### ConvergioCLI Release (existing)
1. Update `VERSION` file
2. Update `CHANGELOG.md`
3. Create tag `v{VERSION}`
4. GitHub Actions builds, signs, notarizes
5. Updates Homebrew tap automatically

#### Convergio Studio Release (enhanced)
1. Update `CONVERGIO_VERSION` file
2. Update `CONVERGIO_CHANGELOG.md`
3. Verify ACP protocol compatibility with ConvergioCLI
4. Create tag `convergio-v{VERSION}`
5. GitHub Actions:
   - Builds for macOS (arm64, x86_64) and Linux
   - Signs and notarizes macOS builds
   - Creates DMG for macOS
   - Creates GitHub Release

### Code Signing Configuration

For Convergio Studio, use existing Fight The Stroke Foundation credentials:
- Developer ID: `Developer ID Application: Fight The Stroke Foundation (93T3LG4NPG)`
- Same Apple ID and Team ID as ConvergioCLI

Secrets required in convergio-zed:
- `APPLE_CERTIFICATE_P12`
- `APPLE_CERTIFICATE_PASSWORD`
- `APPLE_ID`
- `APPLE_APP_PASSWORD`
- `APPLE_TEAM_ID`

### Cross-Repository Compatibility Check

The `app-release-manager` agent will verify:
1. ACP protocol versions match between repos
2. Required CLI version is documented in Studio release notes
3. No breaking changes without version bump

## Consequences

### Positive
- Clear versioning strategy
- Automated, reproducible releases
- Code-signed and notarized macOS binaries
- Protocol compatibility enforced

### Negative
- More complex release process
- Secrets must be duplicated across repos
- Manual coordination still needed for major releases

### Risks
- Zed upstream breaking changes
- Apple certificate expiration
- Protocol drift between repos

## Implementation Tasks

| ID | Task | Status |
|----|------|--------|
| R1 | Enhance convergio-zed release workflow | Pending |
| R2 | Add ACP_PROTOCOL_VERSION to both repos | Pending |
| R3 | Create DMG builder using Zed's bundle-mac | Pending |
| R4 | Configure code signing secrets | Pending |
| R5 | Update app-release-manager checklist | Pending |
| R6 | Add protocol version check to workflows | Pending |

## References

- [ConvergioCLI release.yml](/.github/workflows/release.yml)
- [convergio-zed convergio-release.yml](https://github.com/Roberdan/convergio-zed/blob/main/.github/workflows/convergio-release.yml)
- [Zed bundle-mac script](https://github.com/Roberdan/convergio-zed/blob/main/script/bundle-mac)
