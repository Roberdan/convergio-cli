#!/bin/bash
# =============================================================================
# ENSURE OLLAMA IS RUNNING
# =============================================================================
# Helper script to ensure Ollama is running before tests.
# Used by Makefile targets that need local LLM testing.
# =============================================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check if Ollama is installed
if ! command -v ollama &> /dev/null; then
    echo -e "${RED}ERROR: Ollama is not installed${NC}"
    echo "Install with: brew install ollama"
    exit 1
fi

# Check if Ollama is running
if ! pgrep -x "ollama" > /dev/null && ! curl -s http://localhost:11434/api/tags > /dev/null 2>&1; then
    echo -e "${YELLOW}Ollama is not running. Starting...${NC}"

    # Start Ollama in background
    ollama serve > /dev/null 2>&1 &
    OLLAMA_PID=$!

    # Wait for it to be ready (max 30 seconds)
    for i in {1..30}; do
        if curl -s http://localhost:11434/api/tags > /dev/null 2>&1; then
            echo -e "${GREEN}Ollama started successfully (PID: $OLLAMA_PID)${NC}"
            break
        fi
        sleep 1
    done

    # Verify it's running
    if ! curl -s http://localhost:11434/api/tags > /dev/null 2>&1; then
        echo -e "${RED}ERROR: Failed to start Ollama${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}Ollama is already running${NC}"
fi

# Check if required model is available
MODEL="${OLLAMA_MODEL:-qwen2.5:0.5b}"
if ! ollama list 2>/dev/null | grep -q "$MODEL"; then
    echo -e "${YELLOW}Pulling model $MODEL...${NC}"
    ollama pull "$MODEL"
fi

echo -e "${GREEN}Ollama ready with model: $MODEL${NC}"
