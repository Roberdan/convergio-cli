use crate::cli_chain::ChainCommands;
use crate::cli_error::CliError;

pub async fn dispatch(cmd: ChainCommands) -> Result<(), CliError> {
    match cmd {
        ChainCommands::Overview { human, api_url } => {
            let url = format!("{api_url}/api/chain/overview");
            if let Err(e) = crate::cli_http::fetch_and_print(&url, human).await {
                eprintln!("error: {e}");
            }
        }
        ChainCommands::Status { human, api_url } => {
            let url = format!("{api_url}/api/chain/status");
            if let Err(e) = crate::cli_http::fetch_and_print(&url, human).await {
                eprintln!("error: {e}");
            }
        }
        ChainCommands::Bump {
            crate_name,
            from,
            to,
            dry_run,
            human,
            api_url,
        } => {
            let body = serde_json::json!({
                "crate_name": crate_name,
                "from_tag": from,
                "to_tag": to,
                "dry_run": dry_run,
            });
            if let Err(e) =
                crate::cli_http::post_and_print(&format!("{api_url}/api/chain/bump"), &body, human)
                    .await
            {
                eprintln!("error: {e}");
            }
        }
    }
    Ok(())
}
