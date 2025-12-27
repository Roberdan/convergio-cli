/**
 * @file AccessibleLessonExample.tsx
 * @brief Example component demonstrating all accessibility adaptations
 */

'use client';

import { useState } from 'react';
import {
  useEducationAccessibility,
  useDyslexiaFormatting,
  useMathAccessibility,
  useADHDSession,
  useMotorAccessibility,
  generateProgressBar,
  getCelebrationMessage,
} from '../useEducationAccessibility';

export default function AccessibleLessonExample() {
  const {
    profile,
    getCSS,
    adaptContent,
    getAdaptationsSummary,
  } = useEducationAccessibility();

  const { syllabifyText, shouldSyllabify } = useDyslexiaFormatting();
  const { formatNumber, shouldDisableTimer } = useMathAccessibility();
  const { isActive, timeRemaining, start, stop, sessionDuration } = useADHDSession();
  const { getAdjustedTimeout, shouldUseVoice } = useMotorAccessibility();

  const [sessionStart] = useState(new Date());
  const [currentStep, setCurrentStep] = useState(0);
  const totalSteps = 5;

  const lessonText = `La matematica è fondamentale per comprendere il mondo che ci circonda.
Attraverso i numeri possiamo descrivere quantità, misure e relazioni.`;

  const displayText = shouldSyllabify ? syllabifyText(lessonText) : lessonText;

  return (
    <div style={getCSS()} className="max-w-4xl mx-auto p-6">
      {/* Header */}
      <header className="mb-8">
        <h1 className="text-3xl font-bold mb-4">Lezione di Matematica</h1>

        {/* Active adaptations badge */}
        {getAdaptationsSummary().length > 0 && (
          <div className="bg-blue-50 dark:bg-blue-900/20 border border-blue-200 dark:border-blue-800 rounded-lg p-4 mb-4">
            <h2 className="text-lg font-semibold mb-2 flex items-center gap-2">
              <span>♿</span>
              Adattamenti attivi
            </h2>
            <ul className="list-disc list-inside space-y-1 text-sm">
              {getAdaptationsSummary().map((adaptation, i) => (
                <li key={i}>{adaptation}</li>
              ))}
            </ul>
          </div>
        )}
      </header>

      {/* ADHD Session Timer */}
      {profile.adhd && (
        <div className="bg-purple-50 dark:bg-purple-900/20 border border-purple-200 dark:border-purple-800 rounded-lg p-4 mb-6">
          <h3 className="font-semibold mb-3 flex items-center gap-2">
            <span>⏱️</span>
            Sessione di studio
          </h3>

          {!isActive ? (
            <button
              onClick={start}
              className="bg-purple-600 hover:bg-purple-700 text-white px-4 py-2 rounded-lg transition-colors"
            >
              Inizia sessione ({sessionDuration / 60} minuti)
            </button>
          ) : (
            <div className="space-y-2">
              <div className="text-2xl font-mono">
                Tempo rimanente: {Math.floor(timeRemaining / 60)}:
                {String(timeRemaining % 60).padStart(2, '0')}
              </div>
              <div className="text-sm text-gray-600 dark:text-gray-400">
                {generateProgressBar(
                  sessionDuration - timeRemaining,
                  sessionDuration,
                  30
                )}
              </div>
              <button
                onClick={stop}
                className="bg-red-600 hover:bg-red-700 text-white px-4 py-2 rounded-lg transition-colors"
              >
                Ferma sessione
              </button>
            </div>
          )}
        </div>
      )}

      {/* Main lesson content */}
      <div className="bg-white dark:bg-gray-800 rounded-lg shadow-lg p-6 mb-6">
        <h2 className="text-2xl font-bold mb-4">📖 Introduzione</h2>
        <p className="mb-6 leading-relaxed">{displayText}</p>

        {/* Math problem with dyscalculia support */}
        <div className="border-t pt-6">
          <h3 className="text-xl font-semibold mb-4">💡 Problema</h3>

          <div className="bg-gray-50 dark:bg-gray-900 rounded-lg p-4 mb-4">
            <p className="mb-2">Quanto fa:</p>
            <div className="text-3xl font-bold flex items-center gap-3">
              <span dangerouslySetInnerHTML={{ __html: formatNumber(123, true) }} />
              <span>+</span>
              <span dangerouslySetInnerHTML={{ __html: formatNumber(456, true) }} />
              <span>=</span>
              <span>?</span>
            </div>
          </div>

          {shouldDisableTimer && (
            <div className="bg-green-50 dark:bg-green-900/20 border border-green-200 dark:border-green-800 rounded-lg p-3 mb-4">
              <p className="text-sm flex items-center gap-2">
                <span>✅</span>
                <span>Nessun limite di tempo - prenditi tutto il tempo che ti serve!</span>
              </p>
            </div>
          )}

          {/* Voice input option for cerebral palsy */}
          {shouldUseVoice && (
            <div className="mb-4">
              <button className="bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded-lg transition-colors flex items-center gap-2">
                <span>🎤</span>
                <span>Usa input vocale</span>
              </button>
            </div>
          )}

          {/* Answer input with adjusted timeout */}
          <div className="mb-4">
            <label htmlFor="answer" className="block text-sm font-medium mb-2">
              La tua risposta:
            </label>
            <input
              type="number"
              id="answer"
              className="w-full px-4 py-3 rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-lg"
              placeholder="Scrivi qui la risposta..."
              data-timeout={getAdjustedTimeout(5000)}
            />
            <p className="text-xs text-gray-500 mt-1">
              Timeout: {getAdjustedTimeout(5000)}ms (base: 5000ms)
            </p>
          </div>
        </div>
      </div>

      {/* Progress tracker */}
      <div className="bg-white dark:bg-gray-800 rounded-lg shadow-lg p-6">
        <h3 className="text-xl font-semibold mb-4">📊 Il tuo progresso</h3>

        <div className="space-y-4">
          {/* Step progress */}
          <div>
            <div className="flex justify-between text-sm mb-2">
              <span>Passo {currentStep + 1} di {totalSteps}</span>
              <span>{Math.round(((currentStep + 1) / totalSteps) * 100)}%</span>
            </div>
            <div className="w-full bg-gray-200 dark:bg-gray-700 rounded-full h-3">
              <div
                className="bg-green-600 h-3 rounded-full transition-all duration-300"
                style={{ width: `${((currentStep + 1) / totalSteps) * 100}%` }}
              />
            </div>
            <div className="text-xs text-gray-600 dark:text-gray-400 mt-2 font-mono">
              {generateProgressBar(currentStep + 1, totalSteps, 20)}
            </div>
          </div>

          {/* Celebration message */}
          {currentStep >= 3 && (
            <div className="bg-yellow-50 dark:bg-yellow-900/20 border border-yellow-200 dark:border-yellow-800 rounded-lg p-4">
              <p className="text-center text-lg font-semibold">
                🎉 {getCelebrationMessage(currentStep)}
              </p>
            </div>
          )}

          {/* Navigation buttons */}
          <div className="flex gap-3 justify-between">
            <button
              onClick={() => setCurrentStep(Math.max(0, currentStep - 1))}
              disabled={currentStep === 0}
              className="px-4 py-2 rounded-lg bg-gray-200 dark:bg-gray-700 hover:bg-gray-300 dark:hover:bg-gray-600 disabled:opacity-50 disabled:cursor-not-allowed transition-colors"
            >
              ← Indietro
            </button>

            <button
              onClick={() => setCurrentStep(Math.min(totalSteps - 1, currentStep + 1))}
              disabled={currentStep === totalSteps - 1}
              className="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white disabled:opacity-50 disabled:cursor-not-allowed transition-colors"
            >
              Avanti →
            </button>
          </div>
        </div>
      </div>

      {/* Debug info (collapsible) */}
      <details className="mt-6 bg-gray-50 dark:bg-gray-900 rounded-lg p-4">
        <summary className="cursor-pointer font-semibold text-sm">
          🔧 Informazioni di debug
        </summary>
        <div className="mt-4 space-y-2">
          <div className="text-xs">
            <h4 className="font-semibold mb-2">Profilo accessibilità:</h4>
            <pre className="bg-white dark:bg-gray-800 p-3 rounded overflow-auto text-xs">
              {JSON.stringify(profile, null, 2)}
            </pre>
          </div>
        </div>
      </details>
    </div>
  );
}
