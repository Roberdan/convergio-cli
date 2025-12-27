// ============================================================================
// API ROUTE: Generate ephemeral token for OpenAI Realtime API
// This keeps the API key secure on the server
// ============================================================================

import { NextResponse } from 'next/server';

export async function GET() {
  const apiKey = process.env.OPENAI_API_KEY;

  if (!apiKey) {
    return NextResponse.json(
      { error: 'OpenAI API key not configured' },
      { status: 500 }
    );
  }

  try {
    // Request ephemeral token from OpenAI
    const response = await fetch('https://api.openai.com/v1/realtime/sessions', {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${apiKey}`,
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
        { error: 'Failed to get session token' },
        { status: response.status }
      );
    }

    const data = await response.json();

    return NextResponse.json({
      token: data.client_secret?.value || data.client_secret,
      expiresAt: data.expires_at,
    });
  } catch (error) {
    console.error('Token generation error:', error);
    return NextResponse.json(
      { error: 'Internal server error' },
      { status: 500 }
    );
  }
}
