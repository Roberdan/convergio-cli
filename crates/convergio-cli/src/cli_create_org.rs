// CLI handlers for creating orgs — create-from-mission and create-from-repo.

use crate::cli_error::CliError;

const NOT_IMPL: &str = "Not implemented — planned for future release";

/// Create an org from a mission statement (not yet implemented on server).
pub async fn handle_create_org(
    _name: &str,
    _mission: &str,
    _budget: f64,
    _yes: bool,
    _api_url: &str,
) -> Result<(), CliError> {
    eprintln!("{NOT_IMPL}");
    Ok(())
}

/// Create an org from a scanned repo — delegates to the onboard API.
pub async fn handle_create_org_from(
    path: &str,
    _name: Option<&str>,
    _budget: f64,
    _yes: bool,
    api_url: &str,
) -> Result<(), CliError> {
    let abs_path = std::fs::canonicalize(path)
        .map_err(|e| CliError::InvalidInput(format!("invalid path: {e}")))?;

    let body = serde_json::json!({
        "repo_path": abs_path.to_string_lossy(),
    });

    let resp =
        crate::cli_http::post_and_return(&format!("{api_url}/api/org/projects/onboard"), &body)
            .await
            .map_err(|_| CliError::ApiCallFailed("onboard call failed".into()))?;

    if resp["ok"].as_bool() != Some(true) {
        let err = resp["error"].as_str().unwrap_or("unknown error");
        return Err(CliError::ApiCallFailed(err.to_string()));
    }

    crate::cli_project_add::format_onboard_response(&resp);
    Ok(())
}
