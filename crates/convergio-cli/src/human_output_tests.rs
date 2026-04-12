use super::*;
use serde_json::json;

#[test]
fn format_list_auto_with_objects() {
    let arr = vec![
        json!({"id": 1, "name": "alpha", "status": "done"}),
        json!({"id": 2, "name": "beta", "status": "doing"}),
    ];
    let out = format_list_auto(&arr);
    assert!(out.contains("alpha"));
    assert!(out.contains("beta"));
    assert!(out.contains("done"));
}

#[test]
fn format_list_empty() {
    assert_eq!(format_list_auto(&[]), "(empty list)");
}

#[test]
fn format_single_shows_keys() {
    let map: serde_json::Map<String, Value> =
        serde_json::from_str(r#"{"name": "test", "version": "1.0"}"#).unwrap();
    let out = format_single(&map);
    assert!(out.contains("name"));
    assert!(out.contains("test"));
}

#[test]
fn format_human_unwraps_list() {
    let val = json!({"plans": [{"id": 1, "name": "x"}]});
    let out = format_human(&val);
    assert!(out.contains("plans"));
    assert!(out.contains("1 items"));
}

#[test]
fn format_status_shows_icon() {
    let val = json!({"status": "ok", "uptime": "2h"});
    let out = format_status(&val);
    assert!(out.contains("ok"));
    assert!(out.contains("uptime"));
}

#[test]
fn detect_columns_prioritizes_id_name_status() {
    let arr = vec![json!({"zzz": 1, "id": 10, "name": "a", "status": "ok", "extra": true})];
    let cols = detect_columns(&arr);
    assert!(cols.len() <= 6);
    assert_eq!(cols[0], "id");
    assert_eq!(cols[1], "name");
    assert_eq!(cols[2], "status");
}

#[test]
fn format_scalar_handles_all_types() {
    assert_eq!(format_scalar(&json!("hello")), "hello");
    assert_eq!(format_scalar(&json!(42)), "42");
    assert_eq!(format_scalar(&json!(true)), "true");
}

#[test]
fn human_mode_toggle() {
    assert!(!is_human_mode());
    enable_human_mode();
    assert!(is_human_mode());
    // Reset for other tests
    HUMAN_MODE.store(false, Ordering::Relaxed);
}

#[test]
fn format_list_with_explicit_columns() {
    let arr = vec![
        json!({"id": 1, "name": "a", "status": "ok", "extra": "x"}),
        json!({"id": 2, "name": "b", "status": "fail", "extra": "y"}),
    ];
    let out = format_list(&arr, &["id", "name"]);
    assert!(out.contains("ID"));
    assert!(out.contains("NAME"));
    // Should NOT contain EXTRA since we only asked for id, name
    assert!(!out.contains("EXTRA"));
}
