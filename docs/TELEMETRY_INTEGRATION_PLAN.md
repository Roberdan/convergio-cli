# Telemetry Integration Plan - All Providers

**Created**: 2025-12-20  
**Status**: ⏳ In Progress  
**Scope**: Integrate telemetry in ALL providers and orchestrator

---

## Integration Pattern

For each provider, add:

1. **Include**: `#include "nous/telemetry.h"` and `#include <time.h>`
2. **Measure Latency**: Use `clock_gettime(CLOCK_MONOTONIC, ...)` before and after API call
3. **Record Success**: `telemetry_record_api_call(provider, model, tokens_input, tokens_output, latency_ms)`
4. **Record Errors**: `telemetry_record_error("provider_network_error")` or appropriate error type

---

## Provider Integration Status

### ✅ Completed

- `src/providers/anthropic.c` - ✅ Telemetry integrated in `anthropic_chat()` and `anthropic_chat_with_tools()`

### ⏳ Pending

- `src/providers/openai.c` - ⏳ Need telemetry in `openai_chat()` and `openai_chat_with_tools()`
- `src/providers/gemini.c` - ⏳ Need telemetry in `gemini_chat()` and `gemini_chat_with_tools()`
- `src/providers/ollama.c` - ⏳ Need telemetry in `ollama_chat()`
- `src/providers/openrouter.c` - ⏳ Need telemetry in `openrouter_chat()`
- `src/providers/mlx.m` - ⏳ Need telemetry in `mlx_provider_chat()` (Objective-C)

---

## Orchestrator Integration

### ⏳ Pending

- `src/orchestrator/orchestrator.c` - ⏳ Need telemetry for delegation events
- `src/orchestrator/delegation.c` - ⏳ Need telemetry for agent delegation
- `src/orchestrator/planning.c` - ⏳ Need telemetry for planning events
- `src/orchestrator/convergence.c` - ⏳ Need telemetry for convergence events

---

## Implementation Steps

For each provider:

1. Add includes: `#include "nous/telemetry.h"` and `#include <time.h>`
2. Find `curl_easy_perform()` or API call location
3. Add latency measurement before API call
4. Add latency calculation after API call
5. Add `telemetry_record_api_call()` on success
6. Add `telemetry_record_error()` on failure
7. Extract tokens from `usage` structure or response

---

## Example Integration

```c
// Before API call
struct timespec start_time, end_time;
clock_gettime(CLOCK_MONOTONIC, &start_time);

// API call
CURLcode res = curl_easy_perform(curl);

// Calculate latency
clock_gettime(CLOCK_MONOTONIC, &end_time);
double latency_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) +
                    ((end_time.tv_nsec - start_time.tv_nsec) / 1000000.0);

// On success
if (res == CURLE_OK && http_code == 200) {
    telemetry_record_api_call("provider_name", model, tokens_input, tokens_output, latency_ms);
} else {
    telemetry_record_error("provider_network_error");
}
```

---

## Testing

After integration:

- Verify telemetry events are recorded
- Verify latency is measured correctly
- Verify tokens are extracted correctly
- Verify errors are recorded

---

## Related Documents

- [GLOBAL_INTEGRATION.md](GLOBAL_INTEGRATION.md) - Global integration guide
- [OBSERVABILITY_INTEGRATION.md](workflow-orchestration/OBSERVABILITY_INTEGRATION.md) - Workflow observability

