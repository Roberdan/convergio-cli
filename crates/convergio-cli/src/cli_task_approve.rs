// Copyright (c) 2026 Roberto D'Angelo. All rights reserved.
// Handler for `cvg task approve <task_id>` — deliverables API unimplemented.

use crate::cli_error::CliError;

const NOT_IMPL: &str = "Not implemented — planned for future release";

/// Approve the deliverable linked to a task (not yet implemented on server).
pub async fn handle(
    _task_id: i64,
    _comment: Option<String>,
    _human: bool,
    _api_url: &str,
) -> Result<(), CliError> {
    eprintln!("{NOT_IMPL}");
    Ok(())
}
