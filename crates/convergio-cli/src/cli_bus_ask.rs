// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Bus ask handler — /api/ipc/ask endpoint unimplemented on server side.
// Gated in cli_bus.rs; this module kept for future use.

pub async fn run_ask(_from: &str, _to: &str, _message: &str, _timeout_secs: u64, _api_url: &str) {
    eprintln!("Not implemented — planned for future release");
}
