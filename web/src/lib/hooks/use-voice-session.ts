// ============================================================================
// CONVERGIO WEB - VOICE SESSION HOOK
// Supports both Azure OpenAI and direct OpenAI Realtime API
// ============================================================================

'use client';

import { useCallback, useRef, useEffect, useState } from 'react';
import { useVoiceSessionStore } from '@/lib/stores/app-store';
import type { Maestro } from '@/types';

interface UseVoiceSessionOptions {
  onTranscript?: (role: 'user' | 'assistant', text: string) => void;
  onError?: (error: Error) => void;
  onStateChange?: (state: 'idle' | 'connecting' | 'connected' | 'error') => void;
}

interface ConnectionInfo {
  provider: 'azure' | 'openai';
  wsUrl?: string;
  apiKey?: string;
  token?: string;
}

export function useVoiceSession(options: UseVoiceSessionOptions = {}) {
  const {
    isConnected,
    isListening,
    isSpeaking,
    isMuted,
    currentMaestro,
    transcript,
    toolCalls,
    inputLevel,
    outputLevel,
    setConnected,
    setListening,
    setSpeaking,
    setMuted,
    setCurrentMaestro,
    addTranscript,
    clearTranscript,
    addToolCall,
    updateToolCall,
    clearToolCalls,
    setInputLevel,
    setOutputLevel,
    reset,
  } = useVoiceSessionStore();

  const wsRef = useRef<WebSocket | null>(null);
  const audioContextRef = useRef<AudioContext | null>(null);
  const mediaStreamRef = useRef<MediaStream | null>(null);
  const sourceNodeRef = useRef<MediaStreamAudioSourceNode | null>(null);
  const processorRef = useRef<ScriptProcessorNode | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);
  const audioQueueRef = useRef<Int16Array[]>([]);
  const isPlayingRef = useRef(false);
  const maestroRef = useRef<Maestro | null>(null);

  const [connectionState, setConnectionState] = useState<'idle' | 'connecting' | 'connected' | 'error'>('idle');

  // Connect to Realtime API (Azure or OpenAI)
  const connect = useCallback(async (maestro: Maestro, connectionInfo: ConnectionInfo) => {
    try {
      setConnectionState('connecting');
      options.onStateChange?.('connecting');
      maestroRef.current = maestro;

      // Initialize Web Audio with Safari compatibility
      // Safari uses webkitAudioContext and requires resume on user gesture
      const AudioContextClass = window.AudioContext || (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
      audioContextRef.current = new AudioContextClass();

      // Safari requires explicit resume after user gesture
      if (audioContextRef.current.state === 'suspended') {
        await audioContextRef.current.resume();
      }

      // Safari-compatible audio constraints
      const isSafari = /^((?!chrome|android).)*safari/i.test(navigator.userAgent);
      const audioConstraints: MediaTrackConstraints = {
        echoCancellation: true,
        noiseSuppression: true,
        autoGainControl: true,
      };

      // Safari may need specific sample rate (some versions)
      if (isSafari) {
        audioConstraints.sampleRate = 48000;
        console.log('[Voice] Safari detected, using 48kHz sample rate');
      }

      // Request microphone access
      mediaStreamRef.current = await navigator.mediaDevices.getUserMedia({
        audio: audioConstraints,
      });

      // Create analyser for input levels
      analyserRef.current = audioContextRef.current.createAnalyser();
      analyserRef.current.fftSize = 256;

      // Build WebSocket URL based on provider
      let wsUrl: string;
      let protocols: string[] | undefined;

      if (connectionInfo.provider === 'azure') {
        // Azure requires api-key as query parameter
        wsUrl = `${connectionInfo.wsUrl!}&api-key=${connectionInfo.apiKey}`;
        protocols = undefined; // Azure uses query params, not subprotocols
      } else {
        wsUrl = 'wss://api.openai.com/v1/realtime?model=gpt-4o-realtime-preview-2024-12-17';
        protocols = ['realtime', `openai-insecure-api-key.${connectionInfo.token}`, 'openai-beta.realtime-v1'];
      }

      console.log('[Voice] Connecting to:', connectionInfo.provider, wsUrl);

      // Connect WebSocket
      const ws = protocols ? new WebSocket(wsUrl, protocols) : new WebSocket(wsUrl);

      ws.onopen = () => {
        console.log('[Voice] WebSocket connected to:', connectionInfo.provider);

        // All available tools for maestros
        const maestroTools = [
          {
            type: 'function',
            name: 'run_code',
            description: 'Execute Python or JavaScript code to solve problems, demonstrate algorithms, or compute values',
            parameters: {
              type: 'object',
              properties: {
                language: { type: 'string', enum: ['python', 'javascript'], description: 'Programming language' },
                code: { type: 'string', description: 'Code to execute' },
              },
              required: ['language', 'code'],
            },
          },
          {
            type: 'function',
            name: 'create_chart',
            description: 'Create charts (line, bar, pie, scatter, area) to visualize data',
            parameters: {
              type: 'object',
              properties: {
                type: { type: 'string', enum: ['line', 'bar', 'pie', 'scatter', 'area'] },
                title: { type: 'string' },
                data: {
                  type: 'object',
                  properties: {
                    labels: { type: 'array', items: { type: 'string' } },
                    datasets: {
                      type: 'array',
                      items: {
                        type: 'object',
                        properties: {
                          label: { type: 'string' },
                          data: { type: 'array', items: { type: 'number' } },
                          color: { type: 'string' },
                        },
                      },
                    },
                  },
                },
              },
              required: ['type', 'title', 'data'],
            },
          },
          {
            type: 'function',
            name: 'create_diagram',
            description: 'Create diagrams (flowchart, sequence, class, state, er, mindmap) using Mermaid syntax',
            parameters: {
              type: 'object',
              properties: {
                type: { type: 'string', enum: ['flowchart', 'sequence', 'class', 'state', 'er', 'mindmap'] },
                code: { type: 'string', description: 'Mermaid diagram code' },
                title: { type: 'string' },
              },
              required: ['type', 'code'],
            },
          },
          {
            type: 'function',
            name: 'show_formula',
            description: 'Display mathematical formulas using LaTeX notation',
            parameters: {
              type: 'object',
              properties: {
                latex: { type: 'string', description: 'LaTeX formula' },
                description: { type: 'string', description: 'Explanation of the formula' },
              },
              required: ['latex'],
            },
          },
          {
            type: 'function',
            name: 'create_quiz',
            description: 'Create an interactive quiz with multiple choice, true/false, or open-ended questions',
            parameters: {
              type: 'object',
              properties: {
                title: { type: 'string' },
                subject: { type: 'string' },
                questions: {
                  type: 'array',
                  items: {
                    type: 'object',
                    properties: {
                      text: { type: 'string' },
                      type: { type: 'string', enum: ['multiple_choice', 'true_false', 'open_ended'] },
                      options: { type: 'array', items: { type: 'string' } },
                      correctAnswer: { type: 'string' },
                      hints: { type: 'array', items: { type: 'string' } },
                      explanation: { type: 'string' },
                      difficulty: { type: 'number', minimum: 1, maximum: 5 },
                      topic: { type: 'string' },
                    },
                  },
                },
              },
              required: ['title', 'subject', 'questions'],
            },
          },
          {
            type: 'function',
            name: 'create_flashcard',
            description: 'Create a deck of flashcards for spaced repetition study',
            parameters: {
              type: 'object',
              properties: {
                name: { type: 'string', description: 'Deck name' },
                subject: { type: 'string' },
                cards: {
                  type: 'array',
                  items: {
                    type: 'object',
                    properties: {
                      front: { type: 'string', description: 'Question or term' },
                      back: { type: 'string', description: 'Answer or definition' },
                    },
                  },
                },
              },
              required: ['name', 'subject', 'cards'],
            },
          },
          {
            type: 'function',
            name: 'create_mindmap',
            description: 'Create an interactive mind map to visualize concepts and their relationships',
            parameters: {
              type: 'object',
              properties: {
                title: { type: 'string', description: 'Central topic of the mind map' },
                nodes: {
                  type: 'array',
                  description: 'Main branches from the central topic',
                  items: {
                    type: 'object',
                    properties: {
                      id: { type: 'string' },
                      label: { type: 'string' },
                      icon: { type: 'string', description: 'Optional emoji icon' },
                      color: { type: 'string', description: 'Optional color hex' },
                      children: {
                        type: 'array',
                        items: {
                          type: 'object',
                          properties: {
                            id: { type: 'string' },
                            label: { type: 'string' },
                            children: {
                              type: 'array',
                              items: {
                                type: 'object',
                                properties: {
                                  id: { type: 'string' },
                                  label: { type: 'string' },
                                },
                              },
                            },
                          },
                        },
                      },
                    },
                    required: ['id', 'label'],
                  },
                },
              },
              required: ['title', 'nodes'],
            },
          },
        ];

        // Send session configuration - Azure GA format (2025-08-28)
        const sessionConfig = {
          type: 'session.update',
          session: {
            type: 'realtime',
            instructions: maestro.systemPrompt,
            output_modalities: ['audio'],
            tools: maestroTools,
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
                voice: maestro.voice,
                format: {
                  type: 'audio/pcm',
                  rate: 24000
                }
              }
            }
          },
        };

        console.log('[Voice] Sending session config:', JSON.stringify(sessionConfig, null, 2));
        ws.send(JSON.stringify(sessionConfig));

        setConnected(true);
        setCurrentMaestro(maestro);
        setConnectionState('connected');
        options.onStateChange?.('connected');

        // Start capturing audio
        startAudioCapture();
      };

      ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          console.log('[Voice] Received:', data.type, data);
          handleServerEvent(data);
        } catch (e) {
          console.error('[Voice] Failed to parse message:', e, event.data);
        }
      };

      ws.onerror = (error) => {
        console.error('[Voice] WebSocket error:', error);
        setConnectionState('error');
        options.onStateChange?.('error');
        options.onError?.(new Error('WebSocket connection failed'));
      };

      ws.onclose = (event) => {
        console.log('[Voice] WebSocket closed:', event.code, event.reason);
        setConnected(false);
        if (connectionState !== 'error') {
          setConnectionState('idle');
        }
      };

      // For Azure, we need to add the API key header
      // Since WebSocket doesn't support custom headers in browser, Azure uses query param
      if (connectionInfo.provider === 'azure' && connectionInfo.apiKey) {
        // Azure OpenAI Realtime accepts api-key as query parameter
        // Already included in wsUrl from server
      }

      wsRef.current = ws;
    } catch (error) {
      console.error('[Voice] Connection error:', error);
      setConnectionState('error');
      options.onStateChange?.('error');
      options.onError?.(error as Error);
    }
  }, [options, setConnected, setCurrentMaestro, setConnectionState]);

  // Handle server events
  const handleServerEvent = useCallback((event: any) => {
    switch (event.type) {
      case 'session.created':
        console.log('[Voice] Session created');
        break;

      case 'session.updated':
        console.log('[Voice] Session updated');
        break;

      case 'input_audio_buffer.speech_started':
        setListening(true);
        break;

      case 'input_audio_buffer.speech_stopped':
        setListening(false);
        break;

      case 'conversation.item.input_audio_transcription.completed':
        if (event.transcript) {
          addTranscript('user', event.transcript);
          options.onTranscript?.('user', event.transcript);
        }
        break;

      // GA API uses response.output_audio.delta (was response.audio.delta in beta)
      case 'response.output_audio.delta':
      case 'response.audio.delta':
        if (event.delta) {
          const audioData = base64ToInt16Array(event.delta);
          audioQueueRef.current.push(audioData);
          if (!isPlayingRef.current) {
            playNextAudioChunk();
          }
        }
        break;

      case 'response.output_audio.done':
      case 'response.audio.done':
        // Audio stream complete, let queue finish playing
        break;

      case 'response.output_audio_transcript.delta':
      case 'response.audio_transcript.delta':
        // Could show streaming text here
        break;

      case 'response.output_audio_transcript.done':
      case 'response.audio_transcript.done':
        if (event.transcript) {
          addTranscript('assistant', event.transcript);
          options.onTranscript?.('assistant', event.transcript);
        }
        break;

      case 'response.done':
        console.log('[Voice] Response complete');
        break;

      case 'error':
        console.error('[Voice] Server error:', event.error);
        options.onError?.(new Error(event.error?.message || 'Server error'));
        break;

      // Handle function/tool calls from the AI
      case 'response.function_call_arguments.done':
        console.log('[Voice] Function call completed:', event.name, event.arguments);
        if (event.name && event.arguments) {
          try {
            const args = JSON.parse(event.arguments);
            const toolCall = {
              id: event.call_id || crypto.randomUUID(),
              type: event.name as import('@/types').ToolType,
              name: event.name,
              arguments: args,
              status: 'completed' as const,
            };
            addToolCall(toolCall);

            // Send tool result back to the AI so it can continue
            if (wsRef.current?.readyState === WebSocket.OPEN) {
              wsRef.current.send(JSON.stringify({
                type: 'conversation.item.create',
                item: {
                  type: 'function_call_output',
                  call_id: event.call_id,
                  output: JSON.stringify({ success: true, displayed: true }),
                },
              }));
              // Trigger response to continue after tool use
              wsRef.current.send(JSON.stringify({ type: 'response.create' }));
            }
          } catch (e) {
            console.error('[Voice] Failed to parse function arguments:', e);
          }
        }
        break;

      default:
        // Log unknown events for debugging
        if (event.type) {
          console.log('[Voice] Event:', event.type);
        }
    }
  }, [addTranscript, addToolCall, options, setListening]);

  // Start capturing audio from microphone
  const startAudioCapture = useCallback(() => {
    if (!audioContextRef.current || !mediaStreamRef.current) return;

    const context = audioContextRef.current;
    const source = context.createMediaStreamSource(mediaStreamRef.current);
    sourceNodeRef.current = source;

    // Connect to analyser for level visualization
    source.connect(analyserRef.current!);

    // Use ScriptProcessorNode for audio capture (more compatible than AudioWorklet)
    const processor = context.createScriptProcessor(4096, 1, 1);
    processorRef.current = processor;

    // Resample from native rate to 24kHz
    const nativeSampleRate = context.sampleRate;
    const targetSampleRate = 24000;

    processor.onaudioprocess = (event) => {
      if (wsRef.current?.readyState !== WebSocket.OPEN) return;
      if (isMuted) return;

      const inputData = event.inputBuffer.getChannelData(0);

      // Resample to 24kHz
      const resampledData = resample(inputData, nativeSampleRate, targetSampleRate);

      // Convert to PCM16
      const int16Data = float32ToInt16(resampledData);
      const base64 = int16ArrayToBase64(int16Data);

      wsRef.current.send(JSON.stringify({
        type: 'input_audio_buffer.append',
        audio: base64,
      }));

      // Update input level
      if (analyserRef.current) {
        const dataArray = new Uint8Array(analyserRef.current.frequencyBinCount);
        analyserRef.current.getByteFrequencyData(dataArray);
        const average = dataArray.reduce((a, b) => a + b, 0) / dataArray.length;
        setInputLevel(average / 255);
      }
    };

    source.connect(processor);
    processor.connect(context.destination);
    setListening(true);
  }, [isMuted, setInputLevel, setListening]);

  // Play audio from queue
  const playNextAudioChunk = useCallback(async () => {
    if (audioQueueRef.current.length === 0 || !audioContextRef.current) {
      isPlayingRef.current = false;
      setSpeaking(false);
      return;
    }

    isPlayingRef.current = true;
    setSpeaking(true);

    const audioData = audioQueueRef.current.shift()!;
    const float32Data = int16ToFloat32(audioData);

    // Ensure AudioContext is running (Safari may suspend it)
    const playbackContext = audioContextRef.current;
    if (playbackContext.state === 'suspended') {
      await playbackContext.resume();
    }

    // Create buffer at 24kHz - Safari handles resampling internally
    const buffer = playbackContext.createBuffer(1, float32Data.length, 24000);
    buffer.getChannelData(0).set(float32Data);

    const source = playbackContext.createBufferSource();
    source.buffer = buffer;
    source.connect(playbackContext.destination);
    source.onended = () => playNextAudioChunk();

    // Safari needs small delay between audio chunks to prevent glitches
    try {
      source.start();
    } catch (e) {
      console.warn('[Voice] Audio playback error, retrying:', e);
      // Safari sometimes throws if start is called too quickly after previous end
      setTimeout(() => {
        try {
          source.start();
        } catch (retryError) {
          console.error('[Voice] Audio playback retry failed:', retryError);
          playNextAudioChunk();
        }
      }, 10);
    }

    // Update output level
    const rms = Math.sqrt(float32Data.reduce((sum, val) => sum + val * val, 0) / float32Data.length);
    setOutputLevel(Math.min(rms * 5, 1));
  }, [setOutputLevel, setSpeaking]);

  // Disconnect
  const disconnect = useCallback(() => {
    if (processorRef.current) {
      processorRef.current.disconnect();
      processorRef.current = null;
    }
    if (sourceNodeRef.current) {
      sourceNodeRef.current.disconnect();
      sourceNodeRef.current = null;
    }
    if (wsRef.current) {
      wsRef.current.close();
      wsRef.current = null;
    }
    if (mediaStreamRef.current) {
      mediaStreamRef.current.getTracks().forEach(track => track.stop());
      mediaStreamRef.current = null;
    }
    if (audioContextRef.current) {
      audioContextRef.current.close();
      audioContextRef.current = null;
    }
    audioQueueRef.current = [];
    isPlayingRef.current = false;
    maestroRef.current = null;
    reset();
    setConnectionState('idle');
  }, [reset]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      disconnect();
    };
  }, [disconnect]);

  // Toggle mute
  const toggleMute = useCallback(() => {
    setMuted(!isMuted);
  }, [isMuted, setMuted]);

  // Send text message (fallback)
  const sendText = useCallback((text: string) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({
        type: 'conversation.item.create',
        item: {
          type: 'message',
          role: 'user',
          content: [{ type: 'input_text', text }],
        },
      }));
      wsRef.current.send(JSON.stringify({ type: 'response.create' }));
      addTranscript('user', text);
    }
  }, [addTranscript]);

  // Cancel current response (barge-in)
  const cancelResponse = useCallback(() => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ type: 'response.cancel' }));
      audioQueueRef.current = [];
      isPlayingRef.current = false;
      setSpeaking(false);
    }
  }, [setSpeaking]);

  return {
    // State
    isConnected,
    isListening,
    isSpeaking,
    isMuted,
    currentMaestro,
    transcript,
    toolCalls,
    inputLevel,
    outputLevel,
    connectionState,
    // Actions
    connect,
    disconnect,
    toggleMute,
    sendText,
    cancelResponse,
    clearTranscript,
    clearToolCalls,
  };
}

// === UTILITY FUNCTIONS ===

function base64ToInt16Array(base64: string): Int16Array {
  const binaryString = atob(base64);
  const bytes = new Uint8Array(binaryString.length);
  for (let i = 0; i < binaryString.length; i++) {
    bytes[i] = binaryString.charCodeAt(i);
  }
  return new Int16Array(bytes.buffer);
}

function int16ArrayToBase64(int16Array: Int16Array): string {
  const bytes = new Uint8Array(int16Array.buffer);
  let binaryString = '';
  for (let i = 0; i < bytes.length; i++) {
    binaryString += String.fromCharCode(bytes[i]);
  }
  return btoa(binaryString);
}

function float32ToInt16(float32Array: Float32Array): Int16Array {
  const int16Array = new Int16Array(float32Array.length);
  for (let i = 0; i < float32Array.length; i++) {
    const sample = Math.max(-1, Math.min(1, float32Array[i]));
    int16Array[i] = sample < 0 ? sample * 0x8000 : sample * 0x7FFF;
  }
  return int16Array;
}

function int16ToFloat32(int16Array: Int16Array): Float32Array {
  const float32Array = new Float32Array(int16Array.length);
  for (let i = 0; i < int16Array.length; i++) {
    float32Array[i] = int16Array[i] / (int16Array[i] < 0 ? 0x8000 : 0x7FFF);
  }
  return float32Array;
}

function resample(inputData: Float32Array, fromRate: number, toRate: number): Float32Array {
  if (fromRate === toRate) return inputData;

  const ratio = fromRate / toRate;
  const outputLength = Math.floor(inputData.length / ratio);
  const output = new Float32Array(outputLength);

  for (let i = 0; i < outputLength; i++) {
    const srcIndex = i * ratio;
    const srcIndexFloor = Math.floor(srcIndex);
    const srcIndexCeil = Math.min(srcIndexFloor + 1, inputData.length - 1);
    const fraction = srcIndex - srcIndexFloor;

    // Linear interpolation
    output[i] = inputData[srcIndexFloor] * (1 - fraction) + inputData[srcIndexCeil] * fraction;
  }

  return output;
}
