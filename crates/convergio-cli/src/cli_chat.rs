use crate::cli_error::CliError;

const NOT_IMPL: &str = "Not implemented — planned for future release";

pub async fn handle(_api_url: &str, _message: Option<String>) -> Result<(), CliError> {
    eprintln!("{NOT_IMPL}");
    Ok(())
}
