-- Migration 016: Workflow Engine
-- Creates tables for workflow orchestration system
-- Supports state machine execution, checkpointing, and workflow persistence

BEGIN TRANSACTION;

-- Workflows table: stores workflow definitions and execution state
CREATE TABLE IF NOT EXISTS workflows (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT,
    entry_node_id INTEGER,
    status INTEGER NOT NULL DEFAULT 0,  -- 0=pending, 1=running, 2=paused, 3=completed, 4=failed, 5=cancelled
    current_node_id INTEGER,
    created_at INTEGER NOT NULL,
    updated_at INTEGER,
    last_checkpoint_at INTEGER,
    error_message TEXT,
    metadata_json TEXT,
    FOREIGN KEY (entry_node_id) REFERENCES workflow_nodes(id) ON DELETE SET NULL,
    FOREIGN KEY (current_node_id) REFERENCES workflow_nodes(id) ON DELETE SET NULL
);

-- Workflow nodes: individual steps in a workflow
CREATE TABLE IF NOT EXISTS workflow_nodes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workflow_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    type INTEGER NOT NULL,  -- 0=action, 1=decision, 2=human_input, 3=subgraph, 4=parallel, 5=converge
    agent_id INTEGER,  -- SemanticID of agent to execute (for ACTION nodes)
    action_prompt TEXT,  -- What the agent should do
    condition_expr TEXT,  -- Condition expression for conditional edges
    node_data_json TEXT,  -- Type-specific data (JSON)
    created_at INTEGER NOT NULL,
    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE
);

-- Workflow edges: connections between nodes (defines workflow graph)
CREATE TABLE IF NOT EXISTS workflow_edges (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workflow_id INTEGER NOT NULL,
    from_node_id INTEGER NOT NULL,
    to_node_id INTEGER NOT NULL,
    condition_expr TEXT,  -- Optional condition for conditional routing
    is_default INTEGER DEFAULT 0,  -- 1 if this is the default edge (when condition fails)
    created_at INTEGER NOT NULL,
    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE,
    FOREIGN KEY (from_node_id) REFERENCES workflow_nodes(id) ON DELETE CASCADE,
    FOREIGN KEY (to_node_id) REFERENCES workflow_nodes(id) ON DELETE CASCADE,
    UNIQUE(workflow_id, from_node_id, to_node_id)
);

-- Workflow state: key-value store for workflow execution state
CREATE TABLE IF NOT EXISTS workflow_state (
    workflow_id INTEGER NOT NULL,
    key TEXT NOT NULL,
    value TEXT NOT NULL,
    updated_at INTEGER NOT NULL,
    PRIMARY KEY (workflow_id, key),
    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE
);

-- Workflow checkpoints: snapshots of workflow state for recovery
CREATE TABLE IF NOT EXISTS workflow_checkpoints (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workflow_id INTEGER NOT NULL,
    node_id INTEGER NOT NULL,
    state_json TEXT NOT NULL,  -- Serialized workflow state
    created_at INTEGER NOT NULL,
    metadata_json TEXT,  -- Additional checkpoint metadata
    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE,
    FOREIGN KEY (node_id) REFERENCES workflow_nodes(id) ON DELETE SET NULL
);

-- Indexes for performance optimization
CREATE INDEX IF NOT EXISTS idx_workflows_status ON workflows(status);
CREATE INDEX IF NOT EXISTS idx_workflows_created ON workflows(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_workflows_current_node ON workflows(current_node_id);

CREATE INDEX IF NOT EXISTS idx_nodes_workflow ON workflow_nodes(workflow_id);
CREATE INDEX IF NOT EXISTS idx_nodes_type ON workflow_nodes(type);

CREATE INDEX IF NOT EXISTS idx_edges_workflow ON workflow_edges(workflow_id);
CREATE INDEX IF NOT EXISTS idx_edges_from ON workflow_edges(from_node_id);
CREATE INDEX IF NOT EXISTS idx_edges_to ON workflow_edges(to_node_id);

CREATE INDEX IF NOT EXISTS idx_state_workflow ON workflow_state(workflow_id);

CREATE INDEX IF NOT EXISTS idx_checkpoints_workflow ON workflow_checkpoints(workflow_id);
CREATE INDEX IF NOT EXISTS idx_checkpoints_created ON workflow_checkpoints(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_checkpoints_node ON workflow_checkpoints(node_id);

-- Enable foreign key constraints
PRAGMA foreign_keys = ON;

COMMIT;

