# Convergio Education - Voice Interaction Setup

**Created**: 2025-12-20
**ADR**: ADR-002-voice-interaction-architecture.md

## Overview

The Education Pack supports real-time voice interaction with all 15 maestri using either:
1. **Azure OpenAI Realtime API** (recommended, GDPR-compliant, free with Azure credits)
2. **OpenAI Realtime API** (direct, simpler setup)

Both provide ChatGPT-level voice quality with natural conversation, interruption handling (barge-in), and voice activity detection.

## Quick Start

### Option 1: Azure OpenAI (Recommended)

The Azure deployment is already configured in the VirtualBPM subscription.

```bash
# Add to your .env file
export AZURE_OPENAI_REALTIME_ENDPOINT="https://your-resource-name.openai.azure.com/"
export AZURE_OPENAI_REALTIME_API_KEY="<get-from-azure-portal>"
export AZURE_OPENAI_REALTIME_DEPLOYMENT="gpt-4o-realtime"
export AZURE_OPENAI_REALTIME_API_VERSION="2025-04-01-preview"
```

To get the API key:
1. Go to [Azure Portal](https://portal.azure.com)
2. Navigate to: `your-resource-name` → Keys and Endpoint
3. Copy KEY 1 or KEY 2

### Option 2: Direct OpenAI API

```bash
# Add to your .env file
export OPENAI_API_KEY="sk-..."
```

Get your API key from [OpenAI Platform](https://platform.openai.com/api-keys).

## Azure Deployment Details

| Setting | Value |
|---------|-------|
| Resource | `your-resource-name` |
| Region | Sweden Central (GDPR-compliant) |
| Deployment | `gpt-4o-realtime` |
| Model | `gpt-realtime` (GA 2025-08-28) |
| Subscription | your-subscription |
| Resource Group | your-resource-group |

### WebSocket URL Format

```
wss://your-resource-name.openai.azure.com/openai/v1/realtime?model=gpt-4o-realtime
```

### Supported Features

- Real-time speech-to-speech
- Voice Activity Detection (VAD)
- Barge-in (interrupt mid-response)
- 8 voice presets per maestro
- Multi-language (IT, EN, ES, FR, DE)

## Voice Profiles per Maestro

| Maestro | Voice | Style |
|---------|-------|-------|
| Euclide | sage | Calm, methodical, Greek accent |
| Feynman | echo | Enthusiastic, American |
| Manzoni | coral | Warm, literary, Milanese |
| Darwin | alloy | Curious, British |
| Erodoto | verse | Dramatic storyteller |
| Humboldt | echo | Passionate explorer |
| Leonardo | coral | Creative, Tuscan |
| Shakespeare | verse | Theatrical, British |
| Mozart | shimmer | Joyful, melodic |
| Cicerone | verse | Authoritative, Roman |
| Smith | sage | Analytical, Scottish |
| Lovelace | shimmer | Precise, British |
| Ippocrate | sage | Caring, soothing |
| Socrate | alloy | Questioning, wise |
| Chris | echo | Dynamic, inspiring |

## Cost Estimation

### Azure OpenAI
- Uses VirtualBPM subscription credits
- ~$0.15/min for realtime audio
- Estimated: ~$4-6/month per active student

### Direct OpenAI
- $32/1M input tokens
- $64/1M output tokens
- ~$0.15/min

## Troubleshooting

### "No API key found"
```bash
# Verify environment variables
echo $AZURE_OPENAI_REALTIME_API_KEY
echo $OPENAI_API_KEY
```

### "Region not supported"
The Realtime API only works in:
- East US 2
- Sweden Central

The `your-resource-name` resource is in Sweden Central and works correctly.

### "Model not available"
Ensure the deployment uses one of:
- `gpt-4o-realtime-preview`
- `gpt-realtime` (recommended, GA)
- `gpt-realtime-mini`

## Technical Reference

- [Azure OpenAI Realtime API Docs](https://learn.microsoft.com/en-us/azure/ai-foundry/openai/how-to/realtime-audio-websockets)
- [OpenAI Realtime API Docs](https://platform.openai.com/docs/guides/realtime)
- [ADR-002](../adr/ADR-002-voice-interaction-architecture.md)
