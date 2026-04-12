//! HTTP API routes for convergio-cli.

use axum::Router;

/// Returns the router for this crate's API endpoints.
pub fn routes() -> Router {
    Router::new()
    // .route("/api/cli/health", get(health))
}
