#!/bin/bash
# Generate Zed agent_servers configuration for all Convergio agents
# Usage: ./generate_zed_config.sh > zed_agents.json
#        ./generate_zed_config.sh --apply  # Apply to ~/.config/zed/settings.json

set -e

CONVERGIO_BIN="${CONVERGIO_BIN:-$(dirname "$0")/../build/bin/convergio-acp}"

# Check if binary exists
if [ ! -x "$CONVERGIO_BIN" ]; then
    echo "Error: convergio-acp not found at $CONVERGIO_BIN" >&2
    echo "Run 'make convergio-acp' first" >&2
    exit 1
fi

# Get absolute path
CONVERGIO_BIN=$(cd "$(dirname "$CONVERGIO_BIN")" && pwd)/$(basename "$CONVERGIO_BIN")

# Get list of agents
agents=$("$CONVERGIO_BIN" --list-agents 2>/dev/null | tail -n +2 | awk '{print $1}')

# Build JSON
echo "{"
echo '  "agent_servers": {'

first=true
for agent in $agents; do
    # Skip empty lines
    [ -z "$agent" ] && continue

    if [ "$first" = true ]; then
        first=false
    else
        echo ","
    fi

    printf '    "Convergio: %s": {\n' "$agent"
    printf '      "type": "custom",\n'
    printf '      "command": "%s",\n' "$CONVERGIO_BIN"
    printf '      "args": ["--agent", "%s"],\n' "$agent"
    printf '      "env": {}\n'
    printf '    }'
done

echo ""
echo "  }"
echo "}"
