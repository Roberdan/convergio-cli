// ============================================================================
// API ROUTE: Provide Azure OpenAI Realtime connection info
// Azure uses API key directly, no ephemeral tokens needed
// ============================================================================

import { NextResponse } from 'next/server';

export async function GET() {
  // Use Azure OpenAI Realtime
  const azureEndpoint = process.env.AZURE_OPENAI_REALTIME_ENDPOINT;
  const azureApiKey = process.env.AZURE_OPENAI_REALTIME_API_KEY;
  const azureDeployment = process.env.AZURE_OPENAI_REALTIME_DEPLOYMENT;

  if (!azureEndpoint || !azureApiKey || !azureDeployment) {
    // Fallback to direct OpenAI if Azure not configured
    const openaiKey = process.env.OPENAI_API_KEY;
    if (!openaiKey) {
      return NextResponse.json(
        { error: 'No API keys configured' },
        { status: 500 }
      );
    }

    // Try OpenAI ephemeral token
    try {
      const response = await fetch('https://api.openai.com/v1/realtime/sessions', {
        method: 'POST',
        headers: {
          'Authorization': `Bearer ${openaiKey}`,
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          model: 'gpt-4o-realtime-preview-2024-12-17',
          voice: 'sage',
        }),
      });

      if (!response.ok) {
        const error = await response.text();
        console.error('OpenAI token request failed:', error);
        return NextResponse.json(
          { error: 'Failed to get session token', details: error },
          { status: response.status }
        );
      }

      const data = await response.json();
      return NextResponse.json({
        provider: 'openai',
        token: data.client_secret?.value || data.client_secret,
        expiresAt: data.expires_at,
      });
    } catch (error) {
      console.error('OpenAI token error:', error);
      return NextResponse.json(
        { error: 'Internal server error' },
        { status: 500 }
      );
    }
  }

  // Return Azure connection info
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
