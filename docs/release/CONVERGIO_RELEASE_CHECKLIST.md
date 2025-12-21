# Convergio Release Checklist

This checklist is used by the `app-release-manager` agent to verify release readiness for both ConvergioCLI and Convergio Studio.

## Pre-Release Verification

### Code Quality
- [ ] All tests pass (`make test` and `./tests/test_acp_e2e.sh`)
- [ ] No compiler warnings (`make 2>&1 | grep -i warning`)
- [ ] No TODO/FIXME in critical paths
- [ ] Code review completed (GitHub Copilot review passed)

### Security
- [ ] No secrets in codebase (`git secrets --scan`)
- [ ] Dependencies up to date and no known vulnerabilities
- [ ] OWASP Top 10 compliance verified
- [ ] No hardcoded API keys or credentials

### Documentation
- [ ] CHANGELOG.md updated with all changes
- [ ] README.md reflects current features
- [ ] Help text (`convergio --help`) is accurate
- [ ] ADRs for architectural decisions are complete

### Version Alignment
- [ ] VERSION file updated (ConvergioCLI)
- [ ] CONVERGIO_VERSION updated (convergio-zed)
- [ ] ACP_PROTOCOL_VERSION matches between repos
- [ ] Git tag matches VERSION file

## ConvergioCLI Release

### Build Verification
- [ ] `make clean && make` succeeds
- [ ] Binary runs: `./build/bin/convergio --version`
- [ ] ACP server starts: `./build/bin/convergio-acp --help`
- [ ] All 54 agents accessible

### Release Artifacts
- [ ] Tarball created with correct version
- [ ] Binary is signed (Developer ID)
- [ ] Binary is notarized (Apple Notary Service)
- [ ] SHA256 checksum calculated

### Distribution
- [ ] GitHub Release created with changelog
- [ ] Homebrew formula updated in tap
- [ ] Release notes include installation instructions

## Convergio Studio Release

### Build Verification
- [ ] `cargo build --release -p zed` succeeds
- [ ] App bundle created via cargo-bundle
- [ ] Convergio Panel loads correctly
- [ ] Ali Panel functional

### macOS Specific
- [ ] ARM64 DMG created
- [ ] x86_64 DMG created
- [ ] Both DMGs code signed
- [ ] Both DMGs notarized
- [ ] Stapler applied to DMGs

### Linux Specific
- [ ] Linux binary compiled
- [ ] Tarball created
- [ ] Binary permissions correct (755)

### Release Artifacts
- [ ] All platform artifacts uploaded
- [ ] SHA256SUMS.txt generated
- [ ] GitHub Release created

## Post-Release Verification

### Smoke Tests
- [ ] Fresh install from Homebrew works
- [ ] DMG installs correctly on clean macOS
- [ ] CLI and Studio communicate via ACP
- [ ] Agent conversations work end-to-end

### Monitoring
- [ ] No crash reports in first 24 hours
- [ ] GitHub issues checked for release-related bugs
- [ ] Community feedback reviewed

## Rollback Procedure

If critical issues found:

1. **ConvergioCLI**: Update Homebrew formula to previous version
2. **Convergio Studio**: Mark GitHub release as pre-release, create hotfix
3. **Notify users**: Update release notes with known issues

## Version Numbering

| Component | Current | Format | Notes |
|-----------|---------|--------|-------|
| ConvergioCLI | 5.4.0 | SemVer | CLI version |
| Convergio Studio | 0.1.0 | SemVer | Studio features |
| Zed Upstream | 0.219.0 | Zed versioning | Base editor |
| ACP Protocol | 1.0 | Major.Minor | Protocol compat |

## Secrets Required

### ConvergioCLI (GitHub Actions)
- `APPLE_CERTIFICATE_P12`
- `APPLE_CERTIFICATE_PASSWORD`
- `APPLE_ID`
- `APPLE_APP_PASSWORD`
- `APPLE_TEAM_ID`
- `TAP_GITHUB_TOKEN`

### convergio-zed (GitHub Actions)
- Same Apple secrets as ConvergioCLI
- No Homebrew tap (DMG distribution only)

## Release Commands

```bash
# ConvergioCLI release
cd /Users/roberdan/GitHub/ConvergioCLI
echo "5.5.0" > VERSION
git add VERSION CHANGELOG.md
git commit -m "chore: bump version to 5.5.0"
git tag v5.5.0
git push origin main --tags

# Convergio Studio release
cd /Users/roberdan/GitHub/convergio-zed
echo "0.2.0" > CONVERGIO_VERSION
git add CONVERGIO_VERSION CONVERGIO_CHANGELOG.md
git commit -m "chore: bump Convergio version to 0.2.0"
git tag convergio-v0.2.0
git push origin main --tags
```
