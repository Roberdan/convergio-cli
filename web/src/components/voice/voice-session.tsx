'use client';

import { useState, useEffect, useCallback } from 'react';
import Image from 'next/image';
import { motion, AnimatePresence } from 'framer-motion';
import { Mic, MicOff, PhoneOff, VolumeX, Send, MessageSquare } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Card } from '@/components/ui/card';
import { Waveform, CircularWaveform } from './waveform';
import { useVoiceSession } from '@/lib/hooks/use-voice-session';
import { ToolResultDisplay } from '@/components/tools';
import { WebcamCapture } from '@/components/tools/webcam-capture';
import { cn } from '@/lib/utils';
import type { Maestro } from '@/types';

interface ConnectionInfo {
  provider: 'azure';
  wsUrl: string;
  apiKey: string;
}

interface ConnectionError {
  error: string;
  missingVariables?: string[];
  message?: string;
}

interface VoiceSessionProps {
  maestro: Maestro;
  onClose: () => void;
  onSwitchToChat?: () => void;
}

export function VoiceSession({ maestro, onClose, onSwitchToChat }: VoiceSessionProps) {
  const [connectionInfo, setConnectionInfo] = useState<ConnectionInfo | null>(null);
  const [configError, setConfigError] = useState<ConnectionError | null>(null);
  const [textInput, setTextInput] = useState('');
  const [showTextInput, setShowTextInput] = useState(false);
  const [showWebcam, setShowWebcam] = useState(false);
  const [webcamRequest, setWebcamRequest] = useState<{ purpose: string; instructions?: string; callId: string } | null>(null);

  const {
    isConnected,
    isListening,
    isSpeaking,
    isMuted,
    transcript,
    toolCalls,
    inputLevel,
    outputLevel,
    connectionState,
    connect,
    disconnect,
    toggleMute,
    sendText,
    cancelResponse,
    clearTranscript,
    clearToolCalls,
    sendWebcamResult,
  } = useVoiceSession({
    onError: (error) => console.error('Voice error:', error),
    onTranscript: (role, text) => console.log(`${role}: ${text}`),
    onWebcamRequest: (request) => {
      console.log('Webcam requested:', request);
      setWebcamRequest(request);
      setShowWebcam(true);
    },
  });

  // Handle webcam capture completion
  const handleWebcamCapture = useCallback((imageData: string) => {
    if (webcamRequest) {
      sendWebcamResult(webcamRequest.callId, imageData);
      setShowWebcam(false);
      setWebcamRequest(null);
    }
  }, [webcamRequest, sendWebcamResult]);

  // Handle webcam close/cancel
  const handleWebcamClose = useCallback(() => {
    if (webcamRequest) {
      sendWebcamResult(webcamRequest.callId, null);
    }
    setShowWebcam(false);
    setWebcamRequest(null);
  }, [webcamRequest, sendWebcamResult]);

  // Fetch connection info and connect
  useEffect(() => {
    async function init() {
      try {
        const response = await fetch('/api/realtime/token');
        const data = await response.json();
        if (data.error) {
          console.error('API error:', data.error);
          setConfigError(data as ConnectionError);
          return;
        }
        // Store Azure connection info
        setConnectionInfo(data as ConnectionInfo);
      } catch (error) {
        console.error('Failed to get connection info:', error);
        setConfigError({
          error: 'Connection failed',
          message: 'Unable to connect to the API server',
        });
      }
    }
    init();
  }, []);

  // Connect when connection info is available
  useEffect(() => {
    if (connectionInfo && !isConnected && connectionState === 'idle') {
      connect(maestro, connectionInfo);
    }
  }, [connectionInfo, isConnected, connectionState, maestro, connect]);

  // Handle close
  const handleClose = useCallback(() => {
    disconnect();
    onClose();
  }, [disconnect, onClose]);

  // Handle switch to chat
  const handleSwitchToChat = useCallback(() => {
    disconnect();
    onSwitchToChat?.();
  }, [disconnect, onSwitchToChat]);

  // Handle text submit
  const handleTextSubmit = useCallback(() => {
    if (textInput.trim()) {
      sendText(textInput);
      setTextInput('');
    }
  }, [textInput, sendText]);

  // State indicator
  const stateText = configError
    ? 'Configuration Error'
    : connectionState === 'connecting'
    ? 'Connecting...'
    : isListening
    ? 'Listening...'
    : isSpeaking
    ? `${maestro.name} is speaking...`
    : isConnected
    ? 'Ready - speak now'
    : 'Disconnected';

  // Show configuration error
  if (configError) {
    return (
      <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 backdrop-blur-sm">
        <motion.div
          initial={{ opacity: 0, scale: 0.95 }}
          animate={{ opacity: 1, scale: 1 }}
          className="w-full max-w-md mx-4"
        >
          <Card className="bg-gradient-to-b from-red-900 to-slate-950 border-red-700 text-white">
            <div className="p-6">
              <div className="flex items-center gap-3 mb-4">
                <div className="w-12 h-12 rounded-full bg-red-500/20 flex items-center justify-center">
                  <PhoneOff className="w-6 h-6 text-red-400" />
                </div>
                <div>
                  <h2 className="text-xl font-semibold">Azure OpenAI Non Configurato</h2>
                  <p className="text-sm text-red-300">La voce richiede Azure OpenAI Realtime</p>
                </div>
              </div>

              <div className="bg-red-950/50 rounded-lg p-4 mb-4">
                <p className="text-sm text-red-200 mb-2">{configError.message}</p>
                {configError.missingVariables && (
                  <div className="mt-2">
                    <p className="text-xs text-red-300 mb-1">Variabili mancanti:</p>
                    <ul className="text-xs text-red-400 space-y-1">
                      {configError.missingVariables.map((v) => (
                        <li key={v} className="font-mono">- {v}</li>
                      ))}
                    </ul>
                  </div>
                )}
              </div>

              <div className="space-y-2">
                <p className="text-sm text-slate-300">
                  Configura le variabili di ambiente nel file <code className="text-xs bg-slate-800 px-1 rounded">.env.local</code>:
                </p>
                <pre className="text-xs bg-slate-900 p-3 rounded-lg overflow-x-auto">
{`AZURE_OPENAI_REALTIME_ENDPOINT=https://your-resource.openai.azure.com
AZURE_OPENAI_REALTIME_API_KEY=your-api-key
AZURE_OPENAI_REALTIME_DEPLOYMENT=gpt-4o-realtime-preview`}
                </pre>
              </div>

              <Button
                onClick={onClose}
                className="w-full mt-4 bg-red-600 hover:bg-red-700"
              >
                Chiudi
              </Button>
            </div>
          </Card>
        </motion.div>
      </div>
    );
  }

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 backdrop-blur-sm">
      <motion.div
        initial={{ opacity: 0, scale: 0.95 }}
        animate={{ opacity: 1, scale: 1 }}
        exit={{ opacity: 0, scale: 0.95 }}
        className="w-full max-w-2xl mx-4"
      >
        <Card className="bg-gradient-to-b from-slate-900 to-slate-950 border-slate-700 text-white overflow-hidden">
          {/* Header */}
          <div className="p-6 border-b border-slate-700/50">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-4">
                <div
                  className="w-12 h-12 rounded-full overflow-hidden ring-2 ring-white/20"
                  style={{ backgroundColor: maestro.color }}
                >
                  <Image
                    src={maestro.avatar}
                    alt={maestro.name}
                    width={48}
                    height={48}
                    className="w-full h-full object-cover"
                  />
                </div>
                <div>
                  <h2 className="text-xl font-semibold">{maestro.name}</h2>
                  <p className="text-sm text-slate-400">{maestro.specialty}</p>
                </div>
              </div>
              <Button
                variant="ghost"
                size="icon"
                onClick={handleClose}
                className="text-slate-400 hover:text-white hover:bg-slate-700"
              >
                <PhoneOff className="h-5 w-5" />
              </Button>
            </div>
          </div>

          {/* Main visualization */}
          <div className="p-8 flex flex-col items-center gap-6">
            {/* Avatar with waveform */}
            <CircularWaveform
              level={isSpeaking ? outputLevel : inputLevel}
              isActive={isListening || isSpeaking}
              color={maestro.color}
              size={160}
              image={maestro.avatar}
            />

            {/* State indicator */}
            <motion.div
              key={stateText}
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              className="text-center"
            >
              <p className="text-lg font-medium text-slate-200">{stateText}</p>
              {connectionState === 'connecting' && (
                <div className="mt-2 flex items-center justify-center gap-1">
                  <div className="w-2 h-2 rounded-full bg-blue-500 animate-pulse" />
                  <div className="w-2 h-2 rounded-full bg-blue-500 animate-pulse delay-100" />
                  <div className="w-2 h-2 rounded-full bg-blue-500 animate-pulse delay-200" />
                </div>
              )}
            </motion.div>

            {/* Waveform visualization */}
            <div className="w-full">
              <Waveform
                level={isListening ? inputLevel : isSpeaking ? outputLevel : 0}
                isActive={isListening || isSpeaking}
                color={maestro.color}
                barCount={32}
              />
            </div>
          </div>

          {/* Transcript */}
          <div className="px-6 pb-4">
            <div className="max-h-48 overflow-y-auto space-y-3 p-4 bg-slate-800/50 rounded-xl">
              <AnimatePresence>
                {transcript.length === 0 ? (
                  <p className="text-center text-slate-500 text-sm italic">
                    {maestro.greeting}
                  </p>
                ) : (
                  transcript.map((entry, index) => (
                    <motion.div
                      key={index}
                      initial={{ opacity: 0, y: 10 }}
                      animate={{ opacity: 1, y: 0 }}
                      className={cn(
                        'p-3 rounded-lg max-w-[85%]',
                        entry.role === 'user'
                          ? 'bg-blue-600/30 ml-auto text-right'
                          : 'bg-slate-700/50 mr-auto'
                      )}
                    >
                      <p className="text-sm text-slate-200">{entry.content}</p>
                    </motion.div>
                  ))
                )}
              </AnimatePresence>
            </div>
          </div>

          {/* Tool calls visualization */}
          {toolCalls.length > 0 && (
            <div className="px-6 pb-4">
              <div className="space-y-3 p-4 bg-slate-800/30 rounded-xl border border-slate-700/50">
                <div className="flex items-center justify-between">
                  <h4 className="text-sm font-medium text-slate-400">Strumenti utilizzati</h4>
                  <button
                    onClick={clearToolCalls}
                    className="text-xs text-slate-500 hover:text-slate-300"
                  >
                    Cancella
                  </button>
                </div>
                <AnimatePresence>
                  {toolCalls.map((toolCall) => (
                    <motion.div
                      key={toolCall.id}
                      initial={{ opacity: 0, y: 10 }}
                      animate={{ opacity: 1, y: 0 }}
                      exit={{ opacity: 0, y: -10 }}
                    >
                      <ToolResultDisplay toolCall={toolCall} />
                    </motion.div>
                  ))}
                </AnimatePresence>
              </div>
            </div>
          )}

          {/* Controls */}
          <div className="p-6 border-t border-slate-700/50 bg-slate-800/30">
            <div className="flex items-center justify-center gap-4">
              {/* Mute button */}
              <Button
                variant="ghost"
                size="icon-lg"
                onClick={toggleMute}
                className={cn(
                  'rounded-full transition-colors',
                  isMuted
                    ? 'bg-red-500/20 text-red-400 hover:bg-red-500/30'
                    : 'bg-slate-700 text-white hover:bg-slate-600'
                )}
              >
                {isMuted ? <MicOff className="h-6 w-6" /> : <Mic className="h-6 w-6" />}
              </Button>

              {/* Cancel response (during speaking) */}
              {isSpeaking && (
                <Button
                  variant="ghost"
                  size="icon-lg"
                  onClick={cancelResponse}
                  className="rounded-full bg-amber-500/20 text-amber-400 hover:bg-amber-500/30"
                >
                  <VolumeX className="h-6 w-6" />
                </Button>
              )}

              {/* Toggle text input */}
              <Button
                variant="ghost"
                size="icon-lg"
                onClick={() => setShowTextInput(!showTextInput)}
                className="rounded-full bg-slate-700 text-white hover:bg-slate-600"
              >
                <Send className="h-5 w-5" />
              </Button>

              {/* Switch to chat */}
              {onSwitchToChat && (
                <Button
                  variant="ghost"
                  size="icon-lg"
                  onClick={handleSwitchToChat}
                  className="rounded-full bg-green-600/20 text-green-400 hover:bg-green-600/30"
                >
                  <MessageSquare className="h-5 w-5" />
                </Button>
              )}

              {/* End call */}
              <Button
                variant="destructive"
                size="icon-lg"
                onClick={handleClose}
                className="rounded-full"
              >
                <PhoneOff className="h-6 w-6" />
              </Button>
            </div>

            {/* Text input (fallback) */}
            <AnimatePresence>
              {showTextInput && (
                <motion.div
                  initial={{ height: 0, opacity: 0 }}
                  animate={{ height: 'auto', opacity: 1 }}
                  exit={{ height: 0, opacity: 0 }}
                  className="mt-4 overflow-hidden"
                >
                  <div className="flex gap-2">
                    <input
                      type="text"
                      value={textInput}
                      onChange={(e) => setTextInput(e.target.value)}
                      onKeyDown={(e) => e.key === 'Enter' && handleTextSubmit()}
                      placeholder="Type a message..."
                      className="flex-1 px-4 py-2 rounded-xl bg-slate-700 border border-slate-600 text-white placeholder-slate-400 focus:outline-none focus:ring-2 focus:ring-blue-500"
                    />
                    <Button onClick={handleTextSubmit} disabled={!textInput.trim()}>
                      Send
                    </Button>
                  </div>
                </motion.div>
              )}
            </AnimatePresence>
          </div>
        </Card>
      </motion.div>

      {/* Webcam capture overlay */}
      <AnimatePresence>
        {showWebcam && webcamRequest && (
          <WebcamCapture
            purpose={webcamRequest.purpose}
            instructions={webcamRequest.instructions}
            onCapture={handleWebcamCapture}
            onClose={handleWebcamClose}
          />
        )}
      </AnimatePresence>
    </div>
  );
}
