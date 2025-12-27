// ============================================================================
// API ROUTE: Chat completions with Azure OpenAI
// For text-based chat with maestros
// ============================================================================

import { NextRequest, NextResponse } from 'next/server';

interface ChatMessage {
  role: 'user' | 'assistant' | 'system';
  content: string;
}

interface ChatRequest {
  messages: ChatMessage[];
  systemPrompt: string;
  maestroId: string;
}

export async function POST(request: NextRequest) {
  try {
    const body: ChatRequest = await request.json();
    const { messages, systemPrompt, maestroId } = body;

    if (!messages || !Array.isArray(messages)) {
      return NextResponse.json(
        { error: 'Messages array is required' },
        { status: 400 }
      );
    }

    // Azure OpenAI Chat configuration
    const azureEndpoint = process.env.AZURE_OPENAI_ENDPOINT;
    const azureApiKey = process.env.AZURE_OPENAI_API_KEY;
    const azureDeployment = process.env.AZURE_OPENAI_CHAT_DEPLOYMENT || 'gpt-4o';
    const apiVersion = process.env.AZURE_OPENAI_API_VERSION || '2024-08-01-preview';

    if (!azureEndpoint || !azureApiKey) {
      // Fallback to OpenAI if Azure not configured
      const openaiKey = process.env.OPENAI_API_KEY;
      if (!openaiKey) {
        return NextResponse.json(
          { error: 'No API keys configured' },
          { status: 500 }
        );
      }

      // Use OpenAI API
      const response = await fetch('https://api.openai.com/v1/chat/completions', {
        method: 'POST',
        headers: {
          'Authorization': `Bearer ${openaiKey}`,
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          model: 'gpt-4o',
          messages: [
            { role: 'system', content: systemPrompt },
            ...messages,
          ],
          temperature: 0.7,
          max_tokens: 2048,
        }),
      });

      if (!response.ok) {
        const error = await response.text();
        console.error('OpenAI chat error:', error);
        return NextResponse.json(
          { error: 'Chat request failed', details: error },
          { status: response.status }
        );
      }

      const data = await response.json();
      return NextResponse.json({
        content: data.choices[0]?.message?.content || '',
        usage: data.usage,
        maestroId,
      });
    }

    // Use Azure OpenAI
    const url = `${azureEndpoint.replace(/\/$/, '')}/openai/deployments/${azureDeployment}/chat/completions?api-version=${apiVersion}`;

    const response = await fetch(url, {
      method: 'POST',
      headers: {
        'api-key': azureApiKey,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        messages: [
          { role: 'system', content: systemPrompt },
          ...messages,
        ],
        temperature: 0.7,
        max_tokens: 2048,
      }),
    });

    if (!response.ok) {
      const error = await response.text();
      console.error('Azure OpenAI chat error:', error);
      return NextResponse.json(
        { error: 'Chat request failed', details: error },
        { status: response.status }
      );
    }

    const data = await response.json();

    return NextResponse.json({
      content: data.choices[0]?.message?.content || '',
      usage: data.usage,
      maestroId,
    });
  } catch (error) {
    console.error('Chat API error:', error);
    return NextResponse.json(
      { error: 'Internal server error' },
      { status: 500 }
    );
  }
}
