//! Extension wiring test — verifies every Extension impl is registered
//! in both daemon main.rs and the E2E test harness.
//!
//! If you add a new extension crate and forget to register it, this test fails.

use std::collections::BTreeSet;
use std::path::Path;

fn crates_dir() -> &'static Path {
    Path::new(env!("CARGO_MANIFEST_DIR")).parent().unwrap()
}

fn daemon_dir() -> std::path::PathBuf {
    crates_dir().parent().unwrap().to_path_buf()
}

/// Scan all ext.rs / lib.rs files for `impl Extension for XxxExtension`
/// and return the set of extension type names.
fn find_extension_impls() -> BTreeSet<String> {
    let mut impls = BTreeSet::new();
    let crates = crates_dir();
    for entry in std::fs::read_dir(crates).unwrap() {
        let entry = entry.unwrap();
        if !entry.file_type().unwrap().is_dir() {
            continue;
        }
        let src = entry.path().join("src");
        if !src.exists() {
            continue;
        }
        for file in &["ext.rs", "lib.rs"] {
            let path = src.join(file);
            if path.exists() {
                let content = std::fs::read_to_string(&path).unwrap();
                for line in content.lines() {
                    if let Some(name) = parse_extension_impl(line) {
                        impls.insert(name);
                    }
                }
            }
        }
    }
    impls
}

fn parse_extension_impl(line: &str) -> Option<String> {
    let trimmed = line.trim();
    if !trimmed.starts_with("impl Extension for ") {
        return None;
    }
    let rest = trimmed.strip_prefix("impl Extension for ")?;
    let name = rest
        .split(|c: char| !c.is_alphanumeric() && c != '_')
        .next()?;
    if name.is_empty() {
        return None;
    }
    Some(name.to_string())
}

/// Extract extension type names from a source file by matching Arc::new(xxx::YyyExtension
fn extract_registered_extensions(source: &str) -> BTreeSet<String> {
    let mut names = BTreeSet::new();
    for line in source.lines() {
        let trimmed = line.trim();
        if !trimmed.contains("Extension") {
            continue;
        }
        // Match patterns like `SomeExtension::new(` or `SomeExtension)`
        for part in trimmed.split(|c: char| !c.is_alphanumeric() && c != '_') {
            if part.ends_with("Extension") && part.len() > "Extension".len() {
                names.insert(part.to_string());
            }
        }
    }
    names
}

// Extensions that are intentionally only in one place
fn known_exceptions() -> BTreeSet<&'static str> {
    BTreeSet::from([
        // DepgraphExtension is wired separately (needs manifests from other exts)
        "DepgraphExtension",
        // WorkspaceExtension is a local-only crate, not yet committed to git
        "WorkspaceExtension",
    ])
}

#[test]
fn all_extensions_registered_in_daemon() {
    // Skip in standalone repo — daemon source lives in the monorepo
    if !daemon_dir().join("src/main.rs").exists() {
        return;
    }
    let all_impls = find_extension_impls();
    let main_src = std::fs::read_to_string(daemon_dir().join("src/main.rs")).unwrap();
    let registered = extract_registered_extensions(&main_src);
    let exceptions = known_exceptions();

    let missing: Vec<_> = all_impls
        .iter()
        .filter(|name| !registered.contains(*name) && !exceptions.contains(name.as_str()))
        .collect();

    assert!(
        missing.is_empty(),
        "Extensions with `impl Extension` but NOT registered in main.rs: {missing:?}\n\
         Add them to register_extensions() or add to known_exceptions()."
    );
}

#[test]
fn all_extensions_registered_in_test_harness() {
    // Skip in standalone repo — test harness lives in the monorepo
    if !daemon_dir().join("tests/e2e/harness.rs").exists() {
        return;
    }
    let all_impls = find_extension_impls();
    let harness_src = std::fs::read_to_string(daemon_dir().join("tests/e2e/harness.rs")).unwrap();
    let registered = extract_registered_extensions(&harness_src);
    let exceptions = known_exceptions();

    // NightAgentsExtension is only in main.rs (needs runtime env)
    let mut test_exceptions = exceptions.clone();
    test_exceptions.insert("NightAgentsExtension");

    let missing: Vec<_> = all_impls
        .iter()
        .filter(|name| !registered.contains(*name) && !test_exceptions.contains(name.as_str()))
        .collect();

    assert!(
        missing.is_empty(),
        "Extensions with `impl Extension` but NOT in test harness: {missing:?}\n\
         Add them to build_extensions() in harness.rs or add to test_exceptions."
    );
}

#[test]
fn daemon_and_harness_have_same_extensions() {
    // Skip in standalone repo — daemon source lives in the monorepo
    if !daemon_dir().join("src/main.rs").exists()
        || !daemon_dir().join("tests/e2e/harness.rs").exists()
    {
        return;
    }
    let main_src = std::fs::read_to_string(daemon_dir().join("src/main.rs")).unwrap();
    let harness_src = std::fs::read_to_string(daemon_dir().join("tests/e2e/harness.rs")).unwrap();
    let main_exts = extract_registered_extensions(&main_src);
    let harness_exts = extract_registered_extensions(&harness_src);
    let exceptions = known_exceptions();

    // Extensions only in main (not in harness) — must be justified
    let mut harness_exceptions: BTreeSet<&str> = BTreeSet::new();
    harness_exceptions.insert("NightAgentsExtension");

    let only_main: Vec<_> = main_exts
        .difference(&harness_exts)
        .filter(|n| !harness_exceptions.contains(n.as_str()) && !exceptions.contains(n.as_str()))
        .collect();
    let only_harness: Vec<_> = harness_exts
        .difference(&main_exts)
        .filter(|n| !exceptions.contains(n.as_str()))
        .collect();

    assert!(
        only_main.is_empty(),
        "Extensions in main.rs but NOT in test harness: {only_main:?}"
    );
    assert!(
        only_harness.is_empty(),
        "Extensions in test harness but NOT in main.rs: {only_harness:?}"
    );
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_extension_impl_works() {
        assert_eq!(
            parse_extension_impl("impl Extension for FooExtension {"),
            Some("FooExtension".to_string())
        );
        assert_eq!(
            parse_extension_impl("  impl Extension for BarExtension {"),
            Some("BarExtension".to_string())
        );
        assert_eq!(parse_extension_impl("fn foo() {}"), None);
    }

    #[test]
    fn extract_registered_finds_extensions() {
        let src = r#"
            Arc::new(convergio_db::DbExtension),
            Arc::new(convergio_ipc::IpcExtension::with_bus(pool, bus)),
        "#;
        let names = extract_registered_extensions(src);
        assert!(names.contains("DbExtension"));
        assert!(names.contains("IpcExtension"));
    }
}
