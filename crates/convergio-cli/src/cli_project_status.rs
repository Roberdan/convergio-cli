// cli_project_status — `cvg project status` — show orgs and project health.

use crate::cli_error::CliError;

pub async fn handle_status(api_url: &str) -> Result<(), CliError> {
    crate::cli_http::fetch_and_print(&format!("{api_url}/api/orgs"), true).await
}
