#!/bin/bash
# Generate Zed settings.json with ALL 54 Convergio agents

CONVERGIO_BIN="/Users/roberdan/GitHub/ConvergioCLI/build/bin/convergio-acp"

# Get all agents from convergio-acp
AGENTS=$($CONVERGIO_BIN --list-agents 2>/dev/null | tail -n +2 | awk '{print $1}' | grep -v "^$")

echo '"agent_servers": {'
echo '  "Convergio": {'
echo '    "type": "custom",'
echo "    \"command\": \"$CONVERGIO_BIN\","
echo '    "args": [],'
echo '    "env": {}'
echo '  },'

first=true
for agent in $AGENTS; do
    # Create server name from agent name (capitalize first letter)
    server_name="Convergio-$(echo $agent | sed 's/-/ /g' | awk '{for(i=1;i<=NF;i++) $i=toupper(substr($i,1,1)) substr($i,2)}1' | sed 's/ //g' | cut -c1-20)"

    if [ "$first" = true ]; then
        first=false
    else
        echo ","
    fi

    echo -n "  \"$server_name\": {"
    echo -n "\"type\": \"custom\", "
    echo -n "\"command\": \"$CONVERGIO_BIN\", "
    echo -n "\"args\": [\"--agent\", \"$agent\"], "
    echo -n "\"env\": {}"
    echo -n "}"
done

echo ""
echo "}"
