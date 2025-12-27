// ============================================================================
// CONVERGIO WEB - VOICE SESSION HOOK
// Uses OpenAI Realtime API with Web Audio API
// ============================================================================

'use client';

import { useCallback, useRef, useEffect, useState } from 'react';
import { useVoiceSessionStore } from '@/lib/stores/app-store';
import type { Maestro, MaestroVoice } from '@/types';

interface UseVoiceSessionOptions {
  onTranscript?: (role: 'user' | 'assistant', text: string) => void;
  onError?: (error: Error) => void;
  onStateChange?: (state: 'idle' | 'connecting' | 'connected' | 'error') => void;
}

export function useVoiceSession(options: UseVoiceSessionOptions = {}) {
  const {
    isConnected,
    isListening,
    isSpeaking,
    isMuted,
    currentMaestro,
    transcript,
    inputLevel,
    outputLevel,
    setConnected,
    setListening,
    setSpeaking,
    setMuted,
    setCurrentMaestro,
    addTranscript,
    clearTranscript,
    setInputLevel,
    setOutputLevel,
    reset,
  } = useVoiceSessionStore();

  const wsRef = useRef<WebSocket | null>(null);
  const audioContextRef = useRef<AudioContext | null>(null);
  const mediaStreamRef = useRef<MediaStream | null>(null);
  const workletNodeRef = useRef<AudioWorkletNode | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);
  const audioQueueRef = useRef<Int16Array[]>([]);
  const isPlayingRef = useRef(false);

  const [connectionState, setConnectionState] = useState<'idle' | 'connecting' | 'connected' | 'error'>('idle');

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      disconnect();
    };
  }, []);

  // Connect to OpenAI Realtime API
  const connect = useCallback(async (maestro: Maestro, ephemeralToken: string) => {
    try {
      setConnectionState('connecting');
      options.onStateChange?.('connecting');

      // Initialize Web Audio
      audioContextRef.current = new AudioContext({ sampleRate: 24000 });

      // Request microphone access
      mediaStreamRef.current = await navigator.mediaDevices.getUserMedia({
        audio: {
          sampleRate: 24000,
          channelCount: 1,
          echoCancellation: true,
          noiseSuppression: true,
          autoGainControl: true,
        },
      });

      // Create analyser for input levels
      analyserRef.current = audioContextRef.current.createAnalyser();
      analyserRef.current.fftSize = 256;

      const source = audioContextRef.current.createMediaStreamSource(mediaStreamRef.current);
      source.connect(analyserRef.current);

      // Connect to OpenAI Realtime WebSocket
      const ws = new WebSocket(
        'wss://api.openai.com/v1/realtime?model=gpt-4o-realtime-preview-2024-12-17',
        ['realtime', `openai-insecure-api-key.${ephemeralToken}`, 'openai-beta.realtime-v1']
      );

      ws.onopen = () => {
        console.log('[Voice] WebSocket connected');

        // Send session configuration
        ws.send(JSON.stringify({
          type: 'session.update',
          session: {
            modalities: ['text', 'audio'],
            voice: maestro.voice,
            input_audio_format: 'pcm16',
            output_audio_format: 'pcm16',
            input_audio_transcription: { model: 'whisper-1' },
            turn_detection: {
              type: 'server_vad',
              threshold: 0.5,
              prefix_padding_ms: 300,
              silence_duration_ms: 500,
              create_response: true,
            },
            instructions: maestro.systemPrompt,
            max_response_output_tokens: 4096,
          },
        }));

        setConnected(true);
        setCurrentMaestro(maestro);
        setConnectionState('connected');
        options.onStateChange?.('connected');

        // Start capturing audio
        startAudioCapture();
      };

      ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        handleServerEvent(data);
      };

      ws.onerror = (error) => {
        console.error('[Voice] WebSocket error:', error);
        setConnectionState('error');
        options.onStateChange?.('error');
        options.onError?.(new Error('WebSocket connection failed'));
      };

      ws.onclose = () => {
        console.log('[Voice] WebSocket closed');
        setConnected(false);
        setConnectionState('idle');
      };

      wsRef.current = ws;
    } catch (error) {
      console.error('[Voice] Connection error:', error);
      setConnectionState('error');
      options.onStateChange?.('error');
      options.onError?.(error as Error);
    }
  }, [options]);

  // Handle server events
  const handleServerEvent = useCallback((event: any) => {
    switch (event.type) {
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

      case 'response.audio.delta':
        if (event.delta) {
          const audioData = base64ToInt16Array(event.delta);
          audioQueueRef.current.push(audioData);
          if (!isPlayingRef.current) {
            playNextAudioChunk();
          }
        }
        break;

      case 'response.audio.done':
        setSpeaking(false);
        break;

      case 'response.audio_transcript.delta':
        // Handle streaming transcript if needed
        break;

      case 'response.audio_transcript.done':
        if (event.transcript) {
          addTranscript('assistant', event.transcript);
          options.onTranscript?.('assistant', event.transcript);
        }
        break;

      case 'error':
        console.error('[Voice] Server error:', event.error);
        options.onError?.(new Error(event.error?.message || 'Server error'));
        break;
    }
  }, [addTranscript, options, setListening, setSpeaking]);

  // Start capturing audio from microphone
  const startAudioCapture = useCallback(async () => {
    if (!audioContextRef.current || !mediaStreamRef.current) return;

    try {
      // Load audio worklet for processing
      await audioContextRef.current.audioWorklet.addModule('/audio-processor.js');

      const source = audioContextRef.current.createMediaStreamSource(mediaStreamRef.current);
      workletNodeRef.current = new AudioWorkletNode(audioContextRef.current, 'audio-processor');

      workletNodeRef.current.port.onmessage = (event) => {
        if (wsRef.current?.readyState === WebSocket.OPEN && !isMuted) {
          const audioData = event.data as Int16Array;
          const base64 = int16ArrayToBase64(audioData);
          wsRef.current.send(JSON.stringify({
            type: 'input_audio_buffer.append',
            audio: base64,
          }));
        }

        // Update input level
        if (analyserRef.current) {
          const dataArray = new Uint8Array(analyserRef.current.frequencyBinCount);
          analyserRef.current.getByteFrequencyData(dataArray);
          const average = dataArray.reduce((a, b) => a + b, 0) / dataArray.length;
          setInputLevel(average / 255);
        }
      };

      source.connect(workletNodeRef.current);
      workletNodeRef.current.connect(audioContextRef.current.destination);
      setListening(true);
    } catch (error) {
      console.error('[Voice] Audio capture error:', error);

      // Fallback: use ScriptProcessorNode (deprecated but widely supported)
      const scriptProcessor = audioContextRef.current.createScriptProcessor(4096, 1, 1);
      const source = audioContextRef.current.createMediaStreamSource(mediaStreamRef.current);

      scriptProcessor.onaudioprocess = (event) => {
        if (wsRef.current?.readyState === WebSocket.OPEN && !isMuted) {
          const inputData = event.inputBuffer.getChannelData(0);
          const int16Data = float32ToInt16(inputData);
          const base64 = int16ArrayToBase64(int16Data);
          wsRef.current.send(JSON.stringify({
            type: 'input_audio_buffer.append',
            audio: base64,
          }));
        }
      };

      source.connect(scriptProcessor);
      scriptProcessor.connect(audioContextRef.current.destination);
      setListening(true);
    }
  }, [isMuted, setInputLevel, setListening]);

  // Play audio from queue
  const playNextAudioChunk = useCallback(() => {
    if (audioQueueRef.current.length === 0 || !audioContextRef.current) {
      isPlayingRef.current = false;
      return;
    }

    isPlayingRef.current = true;
    setSpeaking(true);

    const audioData = audioQueueRef.current.shift()!;
    const float32Data = int16ToFloat32(audioData);

    const buffer = audioContextRef.current.createBuffer(1, float32Data.length, 24000);
    buffer.getChannelData(0).set(float32Data);

    const source = audioContextRef.current.createBufferSource();
    source.buffer = buffer;
    source.connect(audioContextRef.current.destination);
    source.onended = () => playNextAudioChunk();
    source.start();

    // Update output level
    const rms = Math.sqrt(float32Data.reduce((sum, val) => sum + val * val, 0) / float32Data.length);
    setOutputLevel(Math.min(rms * 5, 1));
  }, [setOutputLevel, setSpeaking]);

  // Disconnect
  const disconnect = useCallback(() => {
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
    reset();
    setConnectionState('idle');
  }, [reset]);

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
