# ADR-015: Web Search Architecture

**Status**: Accepted
**Date**: 2025-12-19
**Authors**: Roberto D'Angelo & AI Team

## Context

Convergio agents need access to real-time web information for tasks like:
- Financial data analysis (stock prices, quarterly reports)
- Current events and news
- Technical documentation lookups
- Market research

Different LLM providers have varying levels of native web search support, requiring a unified architecture that maximizes quality while ensuring all providers have access to web search capabilities.

## Decision

Implement a **three-tier web search strategy** based on provider capabilities:

### Tier 1: Anthropic Native Web Search
- Use Anthropic's native `web_search_20250305` tool
- Transformed automatically in `claude.c` via `transform_tools_for_anthropic()`
- Highest quality results with source citations
- Response includes `server_tool_use` and `web_search_tool_result` blocks

### Tier 2: OpenAI Native Web Search
- Automatically switch to `gpt-4o-search-preview` model when `web_search` tool is detected
- Add `web_search_options` with `search_context_size: medium` to API request
- Filter `web_search` from tools array (handled natively by model)
- Implementation in `openai.c`:
  - `has_web_search_tool()` - detects web_search in tools
  - `build_tools_json_excluding_web_search()` - filters tool array

### Tier 3: Local Fallback (DuckDuckGo Lite)
- For providers without native search: Gemini, Ollama, MLX
- Uses DuckDuckGo Lite HTML endpoint (no JavaScript required)
- Implementation in `tools.c`:
  - `tool_web_search()` - main implementation
  - `parse_duckduckgo_results()` - HTML parser
- Features:
  - URL encoding via libcurl
  - 15-second timeout
  - Max 3 redirects
  - Extracts: title, URL, snippet per result

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Agent Request                             │
│                 (includes web_search tool)                   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Provider Router                           │
└─────────────────────────────────────────────────────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│  Anthropic  │      │   OpenAI    │      │   Others    │
│   claude.c  │      │  openai.c   │      │  (Gemini,   │
│             │      │             │      │  Ollama,    │
│ Transform   │      │ Switch to   │      │  MLX)       │
│ web_search  │      │ gpt-4o-     │      │             │
│ to native   │      │ search-     │      │ Use local   │
│ tool format │      │ preview     │      │ web_search  │
│             │      │ model       │      │ tool        │
└─────────────┘      └─────────────┘      └─────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│  Anthropic  │      │   OpenAI    │      │  DuckDuckGo │
│  Native     │      │   Native    │      │    Lite     │
│  Search     │      │   Search    │      │   Fallback  │
└─────────────┘      └─────────────┘      └─────────────┘
```

## Tool Definition

The `web_search` tool is defined in `TOOLS_JSON` (tools.c):

```json
{
  "name": "web_search",
  "description": "Search the web for current information. Use for real-time data, news, stock prices, recent events, or anything requiring up-to-date information.",
  "input_schema": {
    "type": "object",
    "properties": {
      "query": {"type": "string", "description": "Search query"}
    },
    "required": ["query"]
  }
}
```

## Security Considerations

1. **URL Encoding**: All queries are URL-encoded via `curl_easy_escape()`
2. **Buffer Limits**: URL (2048), title (500), snippet (1000) characters
3. **Timeouts**: 15-second timeout prevents hanging
4. **Redirect Control**: Max 3 redirects via `CURLOPT_MAXREDIRS`
5. **User Agent**: Identifies as "Convergio/1.0"

## Consequences

### Positive
- All providers have web search capability
- Native search used where available (higher quality)
- Graceful fallback for unsupported providers
- No external dependencies for fallback (uses libcurl already in project)
- DuckDuckGo Lite respects privacy

### Negative
- DuckDuckGo fallback quality lower than native search
- HTML parsing is fragile (may break if DuckDuckGo changes format)
- OpenAI search requires model switch (may affect other capabilities)

### Risks
- DuckDuckGo may change HTML format, breaking parser
- Rate limiting not implemented for fallback
- No caching of search results

## Future Improvements

1. Add result caching with TTL
2. Implement rate limiting for DuckDuckGo
3. Consider alternative fallback sources (SearXNG, Brave Search API)
4. Add search result quality metrics
5. Implement retry logic with exponential backoff

## Files Modified

- `include/nous/tools.h` - Added `TOOL_WEB_SEARCH` enum
- `src/tools/tools.c` - Added `tool_web_search()`, `parse_duckduckgo_results()`
- `src/providers/openai.c` - Added native search detection and model switching
- `src/neural/claude.c` - Web search tool transformation (existing)

## References

- [Anthropic Web Search Tool](https://docs.anthropic.com/en/docs/build-with-claude/tool-use/web-search-tool)
- [OpenAI Web Search](https://platform.openai.com/docs/guides/tools-web-search)
- [DuckDuckGo Lite](https://lite.duckduckgo.com/)
