use super::*;
use clap::Parser;

#[derive(Debug, Parser)]
struct TestCli {
    #[command(subcommand)]
    cmd: OrgCommands,
}

#[test]
fn parse_org_create_command() {
    let cli = TestCli::parse_from([
        "cvg",
        "create",
        "acme-labs",
        "--mission",
        "Build autonomous orgs",
        "--objectives",
        "Ship quickly",
        "--budget",
        "1200",
    ]);
    match cli.cmd {
        OrgCommands::Create {
            name,
            mission,
            objectives,
            budget,
            ceo_agent,
            ..
        } => {
            assert_eq!(name, "acme-labs");
            assert_eq!(mission, "Build autonomous orgs");
            assert_eq!(objectives, "Ship quickly");
            assert_eq!(budget, 1200.0);
            assert_eq!(ceo_agent, "ceo");
        }
        _ => panic!("expected create command"),
    }
}

#[test]
fn parse_org_list_command() {
    let cli = TestCli::parse_from(["cvg", "list"]);
    match cli.cmd {
        OrgCommands::List { api_url } => {
            assert_eq!(api_url, "http://localhost:8420");
        }
        _ => panic!("expected list command"),
    }
}

#[test]
fn format_org_list_contains_expected_columns() {
    let rendered = crate::cli_org_show::format_org_row(&serde_json::json!({
        "id": "fitness-project",
        "status": "active",
        "ceo_agent": "fitness-ceo",
        "member_count": 7,
        "budget_usage_pct": 60.0
    }));
    assert!(rendered.contains("fitness-project"));
    assert!(rendered.contains("fitness-ceo"));
    assert!(rendered.contains("60.0%"));
}
