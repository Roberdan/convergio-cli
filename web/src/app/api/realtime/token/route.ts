// ============================================================================
// API ROUTE: Provide Azure OpenAI Realtime connection info
// Azure-only configuration (no OpenAI fallback)
// ============================================================================

import { NextResponse } from 'next/server';

export async function GET() {
  // Azure OpenAI Realtime configuration (required)
  const azureEndpoint = process.env.AZURE_OPENAI_REALTIME_ENDPOINT;
  const azureApiKey = process.env.AZURE_OPENAI_REALTIME_API_KEY;
  const azureDeployment = process.env.AZURE_OPENAI_REALTIME_DEPLOYMENT;

  // Validate Azure configuration
  const missingConfig: string[] = [];
  if (!azureEndpoint) missingConfig.push('AZURE_OPENAI_REALTIME_ENDPOINT');
  if (!azureApiKey) missingConfig.push('AZURE_OPENAI_REALTIME_API_KEY');
  if (!azureDeployment) missingConfig.push('AZURE_OPENAI_REALTIME_DEPLOYMENT');

  if (missingConfig.length > 0 || !azureEndpoint || !azureApiKey || !azureDeployment) {
    return NextResponse.json(
      {
        error: 'Azure OpenAI not configured',
        missingVariables: missingConfig,
        message: 'Configure Azure OpenAI settings in the app or add environment variables',
      },
      { status: 503 }
    );
  }

  // Build WebSocket URL for Azure OpenAI Realtime GA
  // GA endpoint: /openai/v1/realtime?model=<deployment>
  const wsUrl = azureEndpoint
    .replace('https://', 'wss://')
    .replace(/\/$/, '') +
    `/openai/v1/realtime?model=${azureDeployment}`;

  return NextResponse.json({
    provider: 'azure',
    wsUrl,
    apiKey: azureApiKey,
  });
}

// Check configuration status (for settings page)
export async function HEAD() {
  const azureEndpoint = process.env.AZURE_OPENAI_REALTIME_ENDPOINT;
  const azureApiKey = process.env.AZURE_OPENAI_REALTIME_API_KEY;
  const azureDeployment = process.env.AZURE_OPENAI_REALTIME_DEPLOYMENT;

  if (!azureEndpoint || !azureApiKey || !azureDeployment) {
    return new NextResponse(null, { status: 503 });
  }
  return new NextResponse(null, { status: 200 });
}
