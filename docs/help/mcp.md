# /mcp - Model Context Protocol Client

Manage connections to MCP (Model Context Protocol) servers for external tool integration.

## Usage

```
/mcp <command> [args]
```

## Commands

| Command | Description |
|---------|-------------|
| `list` | List all configured MCP servers |
| `status` | Show connection status |
| `health` | Show detailed health information |
| `connect <name>` | Connect to a specific server |
| `disconnect <name>` | Disconnect from a server |
| `connect-all` | Connect to all enabled servers |
| `enable <name>` | Enable a server |
| `disable <name>` | Disable a server |
| `tools` | List all available tools from connected servers |
| `tools <server>` | List tools from a specific server |
| `call <tool> [json]` | Call a tool with optional JSON arguments |

## Examples

```
/mcp list                           # Show configured servers
/mcp connect filesystem             # Connect to filesystem server
/mcp connect-all                    # Connect to all enabled servers
/mcp tools                          # List all available tools
/mcp tools filesystem               # List tools from filesystem server
/mcp call read_file '{"path":"/tmp/test.txt"}'  # Call a tool
/mcp health                         # Detailed connection health
```

## Configuration

MCP servers are configured in `~/.convergio/mcp.json`:

```json
{
  "servers": {
    "filesystem": {
      "enabled": true,
      "transport": "stdio",
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-filesystem", "/tmp"],
      "timeout_ms": 30000
    },
    "web-api": {
      "enabled": true,
      "transport": "http",
      "url": "http://localhost:8080/mcp",
      "headers": {
        "Authorization": "Bearer ${MCP_TOKEN}"
      }
    }
  }
}
```

## Transport Types

| Transport | Description |
|-----------|-------------|
| `stdio` | Subprocess communication via stdin/stdout |
| `http` | HTTP POST requests |
| `sse` | Server-Sent Events (planned) |

## Environment Variables

In the configuration, use `${VAR_NAME}` to reference environment variables:
- `${HOME}` - User home directory
- `${MCP_TOKEN}` - API tokens
- Any other environment variable

## Tool Discovery

When connected, the MCP client automatically discovers:
- **Tools** - Available functions to call
- **Resources** - Data sources to read
- **Prompts** - Pre-defined prompt templates

## Error Handling

The client includes:
- Automatic retry on connection failure (configurable)
- Connection pooling for multiple servers
- Graceful timeout handling
- Error tracking and reporting

## See Also

- `/daemon` - Notification daemon
- `/todo` - Task management
- [MCP Specification](https://modelcontextprotocol.io)
