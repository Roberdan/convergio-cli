// Task-edit CLI handler — extracted from cli_plan_handlers to stay under 300 lines.

pub struct TaskEditArgs {
    pub task_id: i64,
    pub title: Option<String>,
    pub description: Option<String>,
    pub model: Option<String>,
    pub effort: Option<i64>,
    pub executor: Option<String>,
    pub human: bool,
    pub api_url: String,
}

pub async fn handle_task_edit(args: TaskEditArgs) {
    let mut body = serde_json::Map::new();
    if let Some(t) = args.title {
        body.insert("title".into(), serde_json::json!(t));
    }
    if let Some(d) = args.description {
        body.insert("description".into(), serde_json::json!(d));
    }
    if let Some(m) = args.model {
        body.insert("model".into(), serde_json::json!(m));
    }
    if let Some(e) = args.effort {
        body.insert("effort_level".into(), serde_json::json!(e));
    }
    if let Some(a) = args.executor {
        body.insert("executor_agent".into(), serde_json::json!(a));
    }
    if body.is_empty() {
        eprintln!("error: at least one field to update is required");
        return;
    }
    let url = format!("{}/api/plan-db/task/{}", args.api_url, args.task_id);
    if let Err(e) =
        crate::cli_http::patch_and_print(&url, &serde_json::Value::Object(body), args.human).await
    {
        eprintln!("error: {e}");
    }
}
