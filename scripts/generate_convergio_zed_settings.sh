#!/bin/bash
# Generate Zed settings.json with Convergio agents
# Each agent is a separate ACP server with --agent flag

CONVERGIO_BIN="/Users/roberdan/GitHub/ConvergioCLI/build/bin/convergio-acp"

# Agent definitions: name|server_name
AGENTS=(
    "ali|Convergio-Ali"
    "amy-cfo|Convergio-Amy"
    "baccio-tech-architect|Convergio-Baccio"
    "dario-debugger|Convergio-Dario"
    "rex-code-reviewer|Convergio-Rex"
    "paolo-best-practices-enforcer|Convergio-Paolo"
    "marco-devops-engineer|Convergio-Marco"
    "luca-security-expert|Convergio-Luca"
    "marcello-pm|Convergio-Marcello"
    "sara-ux-ui-designer|Convergio-Sara"
    "sofia-marketing-strategist|Convergio-Sofia"
    "omri-data-scientist|Convergio-Omri"
)

echo "{"
echo '  "agent_servers": {'

first=true
for agent in "${AGENTS[@]}"; do
    IFS='|' read -r name server_name <<< "$agent"

    if [ "$first" = true ]; then
        first=false
    else
        echo ","
    fi

    echo -n "    \"$server_name\": {"
    echo -n "\"type\": \"custom\", "
    echo -n "\"command\": \"$CONVERGIO_BIN\", "
    echo -n "\"args\": [\"--agent\", \"$name\"], "
    echo -n "\"env\": {}"
    echo -n "}"
done

echo ""
echo "  }"
echo "}"
