# Convergio Backend API

FastAPI backend for Azure Cost Management integration.

## Quick Start

```bash
# Setup (first time only)
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

# Start server
uvicorn api.main:app --reload

# Or use the CLI directly
./azure-costs
```

## Authentication

The API uses your local `az login` credentials by default. No additional configuration needed.

For production, set these environment variables in `.env`:
```
AZURE_TENANT_ID=your-tenant-id
AZURE_CLIENT_ID=your-service-principal-id
AZURE_CLIENT_SECRET=your-secret
AZURE_SUBSCRIPTION_ID=your-subscription-id
```

## API Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /health` | Health check |
| `GET /api/v1/costs?days=30` | Cost summary for N days |
| `GET /api/v1/costs/current-month` | Current month costs |
| `GET /api/v1/costs/forecast` | End-of-month forecast |

## CLI Usage

```bash
./azure-costs              # Costs for last 30 days
./azure-costs --forecast   # End-of-month forecast
./azure-costs --daily      # Daily breakdown chart
./azure-costs --days 7     # Last 7 days
./azure-costs --json       # Raw JSON output
```

## Client SDKs

- **Swift**: `ConvergioApp/ConvergioApp/Services/AzureCostService.swift`
- **TypeScript**: `sdk/typescript/src/index.ts`
- **JavaScript**: `website/js/convergio-costs.js`

## Docker

```bash
docker-compose up -d
```

## API Documentation

When running locally, visit http://127.0.0.1:8000/docs for interactive Swagger UI.
