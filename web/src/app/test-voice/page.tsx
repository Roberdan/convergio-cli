'use client';

import { useState } from 'react';

export default function TestVoicePage() {
  const [logs, setLogs] = useState<string[]>([]);
  const [status, setStatus] = useState('idle');

  const log = (msg: string) => {
    console.log(msg);
    setLogs(prev => [...prev, `${new Date().toISOString().split('T')[1].split('.')[0]} ${msg}`]);
  };

  const testConnection = async () => {
    setLogs([]);
    setStatus('connecting');

    try {
      // 1. Get connection info
      log('Fetching /api/realtime/token...');
      const res = await fetch('/api/realtime/token');
      const data = await res.json();
      log(`Got: provider=${data.provider}, wsUrl=${data.wsUrl?.substring(0, 50)}...`);

      if (data.error) {
        log(`ERROR: ${data.error}`);
        setStatus('error');
        return;
      }

      // 2. Build connection based on provider
      let wsUrl: string;
      let protocols: string[] | undefined;

      if (data.provider === 'azure') {
        wsUrl = `${data.wsUrl}&api-key=${data.apiKey}`;
        protocols = undefined;
      } else {
        // OpenAI direct
        wsUrl = 'wss://api.openai.com/v1/realtime?model=gpt-4o-realtime-preview-2024-12-17';
        protocols = ['realtime', `openai-insecure-api-key.${data.token}`, 'openai-beta.realtime-v1'];
      }

      log(`Connecting WebSocket to ${data.provider}...`);

      // 3. Connect
      const ws = protocols ? new WebSocket(wsUrl, protocols) : new WebSocket(wsUrl);

      ws.onopen = () => {
        log('✓ WebSocket OPEN');
        setStatus('connected');

        // Send session config - Azure GA format (2025-08-28)
        const config = {
          type: 'session.update',
          session: {
            type: 'realtime',
            instructions: 'You are a helpful assistant. Say hello in Italian.',
            output_modalities: ['audio'],
            audio: {
              input: {
                transcription: {
                  model: 'whisper-1'
                },
                format: {
                  type: 'audio/pcm',
                  rate: 24000
                },
                turn_detection: {
                  type: 'server_vad',
                  threshold: 0.5,
                  prefix_padding_ms: 300,
                  silence_duration_ms: 200,
                  create_response: true
                }
              },
              output: {
                voice: 'sage',
                format: {
                  type: 'audio/pcm',
                  rate: 24000
                }
              }
            }
          }
        };

        log(`Sending: ${JSON.stringify(config)}`);
        ws.send(JSON.stringify(config));
      };

      ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          if (msg.type === 'error') {
            log(`<< ERROR: ${JSON.stringify(msg.error)}`);
          } else {
            log(`<< ${msg.type}: ${JSON.stringify(msg).substring(0, 200)}...`);
          }
        } catch {
          log(`<< RAW: ${event.data.substring(0, 200)}`);
        }
      };

      ws.onerror = (err) => {
        log(`✗ WebSocket ERROR: ${err}`);
        setStatus('error');
      };

      ws.onclose = (event) => {
        log(`WebSocket CLOSED: code=${event.code} reason=${event.reason}`);
        setStatus('closed');
      };

      // Auto-close after 10s
      setTimeout(() => {
        if (ws.readyState === WebSocket.OPEN) {
          log('Auto-closing after 10s');
          ws.close();
        }
      }, 10000);

    } catch (err) {
      log(`ERROR: ${err}`);
      setStatus('error');
    }
  };

  return (
    <div className="p-8 max-w-4xl mx-auto">
      <h1 className="text-2xl font-bold mb-4">Voice WebSocket Test</h1>

      <button
        onClick={testConnection}
        disabled={status === 'connecting'}
        className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 disabled:opacity-50"
      >
        {status === 'connecting' ? 'Connecting...' : 'Test Connection'}
      </button>

      <div className="mt-2 text-sm">
        Status: <span className={
          status === 'connected' ? 'text-green-500' :
          status === 'error' ? 'text-red-500' :
          'text-gray-500'
        }>{status}</span>
      </div>

      <div className="mt-4 bg-gray-900 text-green-400 p-4 rounded font-mono text-xs overflow-auto max-h-96">
        {logs.length === 0 ? (
          <span className="text-gray-500">Click &quot;Test Connection&quot; to start...</span>
        ) : (
          logs.map((l, i) => <div key={i}>{l}</div>)
        )}
      </div>
    </div>
  );
}
