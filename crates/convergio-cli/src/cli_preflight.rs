//! `cvg preflight` — comprehensive production smoke test.

use crate::cli_error::CliError;
use crate::cli_preflight_checks as checks;
use crate::cli_preflight_mutations as mutations;

pub(crate) const GREEN: &str = "\x1b[32m";
pub(crate) const RED: &str = "\x1b[31m";
pub(crate) const YELLOW: &str = "\x1b[33m";
pub(crate) const BOLD: &str = "\x1b[1m";
pub(crate) const RESET: &str = "\x1b[0m";

pub(crate) struct Report {
    pub passed: Vec<String>,
    pub failed: Vec<String>,
    pub skipped: Vec<String>,
    pub auth_mode: String,
    pub mutations_tested: bool,
}

impl Report {
    pub fn new() -> Self {
        Self {
            passed: vec![],
            failed: vec![],
            skipped: vec![],
            auth_mode: "unknown".into(),
            mutations_tested: false,
        }
    }

    pub fn pass(&mut self, name: &str) {
        self.passed.push(name.to_string());
    }

    pub fn fail(&mut self, name: &str, reason: &str) {
        self.failed.push(format!("{name}: {reason}"));
    }

    pub fn skip(&mut self, name: &str, reason: &str) {
        self.skipped.push(format!("{name}: {reason}"));
    }

    fn total(&self) -> usize {
        self.passed.len() + self.failed.len()
    }

    fn test_mode(&self) -> &str {
        if self.mutations_tested {
            "GET + POST"
        } else {
            "GET only"
        }
    }

    fn print_summary(&self) {
        let (p, f, s) = (self.passed.len(), self.failed.len(), self.skipped.len());
        println!("\n{BOLD}─── PREFLIGHT SUMMARY ───{RESET}");
        println!("  Endpoints tested : {}", self.total());
        println!("  Mode             : {}", self.test_mode());
        println!("  Auth             : {}", self.auth_mode);
        println!(
            "  Result           : {GREEN}{p}{RESET} passed, \
             {RED}{f}{RESET} failed, {YELLOW}{s}{RESET} skipped"
        );
        if !self.failed.is_empty() {
            println!("\n{BOLD}Failures:{RESET}");
            for line in &self.failed {
                println!("  {RED}✗{RESET} {line}");
            }
        }
    }

    fn is_ok(&self) -> bool {
        self.failed.is_empty()
    }
}

pub async fn handle(api_url: &str) -> Result<(), CliError> {
    println!("{BOLD}Convergio Preflight Check{RESET}");
    println!("Target: {api_url}\n");

    let mut r = Report::new();

    checks::check_health(api_url, &mut r).await;
    mutations::check_auth(api_url, &mut r).await;
    checks::check_org_endpoints(api_url, &mut r).await;
    checks::check_plan_endpoints(api_url, &mut r).await;
    checks::check_standard_endpoints(api_url, &mut r).await;
    checks::check_mesh_peers(api_url, &mut r).await;
    mutations::check_mutations(api_url, &mut r).await;

    r.print_summary();

    if r.is_ok() {
        println!("\n{GREEN}{BOLD}✓ PREFLIGHT PASSED{RESET}");
        Ok(())
    } else {
        Err(CliError::ViolationsFound(format!(
            "{} checks failed",
            r.failed.len()
        )))
    }
}
