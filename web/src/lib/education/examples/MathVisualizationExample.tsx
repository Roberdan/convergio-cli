/**
 * @file MathVisualizationExample.tsx
 * @brief Example of dyscalculia-friendly math visualizations
 */

'use client';

import { useState } from 'react';
import {
  formatNumberColored,
  generatePlaceValueBlocks,
  formatFractionVisual,
  formatMathStep,
} from '../accessibility';

export default function MathVisualizationExample() {
  const [number, setNumber] = useState(345);
  const [numerator, setNumerator] = useState(3);
  const [denominator, setDenominator] = useState(4);
  const [useColors, setUseColors] = useState(true);

  return (
    <div className="max-w-4xl mx-auto p-6 space-y-8">
      <header>
        <h1 className="text-3xl font-bold mb-2">Visualizzazioni Matematiche</h1>
        <p className="text-gray-600 dark:text-gray-400">
          Supporto per studenti con discalculia
        </p>
      </header>

      {/* Settings */}
      <div className="bg-blue-50 dark:bg-blue-900/20 border border-blue-200 dark:border-blue-800 rounded-lg p-4">
        <label className="flex items-center gap-2 cursor-pointer">
          <input
            type="checkbox"
            checked={useColors}
            onChange={(e) => setUseColors(e.target.checked)}
            className="w-4 h-4"
          />
          <span className="font-medium">Usa colori per cifre</span>
        </label>
        <p className="text-sm text-gray-600 dark:text-gray-400 mt-1">
          Unità = blu, Decine = verde, Centinaia = rosso
        </p>
      </div>

      {/* Number Visualization */}
      <section className="bg-white dark:bg-gray-800 rounded-lg shadow-lg p-6">
        <h2 className="text-2xl font-bold mb-4">🔢 Numeri con Colori</h2>

        <div className="mb-6">
          <label className="block text-sm font-medium mb-2">
            Scegli un numero:
          </label>
          <input
            type="number"
            value={number}
            onChange={(e) => setNumber(parseInt(e.target.value) || 0)}
            className="px-4 py-2 rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-900 w-full max-w-xs text-lg"
            min="0"
            max="9999"
          />
        </div>

        <div className="space-y-6">
          {/* Regular number */}
          <div>
            <h3 className="text-sm font-semibold text-gray-600 dark:text-gray-400 mb-2">
              Numero normale:
            </h3>
            <div className="text-5xl font-bold">{number}</div>
          </div>

          {/* Colored number */}
          <div>
            <h3 className="text-sm font-semibold text-gray-600 dark:text-gray-400 mb-2">
              Numero con colori:
            </h3>
            <div
              className="text-5xl font-bold"
              dangerouslySetInnerHTML={{
                __html: formatNumberColored(number, useColors),
              }}
            />
          </div>

          {/* Place value blocks */}
          <div>
            <h3 className="text-sm font-semibold text-gray-600 dark:text-gray-400 mb-2">
              Blocchi dei valori posizionali:
            </h3>
            <div
              className="flex flex-wrap gap-4"
              dangerouslySetInnerHTML={{
                __html: generatePlaceValueBlocks(number),
              }}
            />
          </div>
        </div>
      </section>

      {/* Fraction Visualization */}
      <section className="bg-white dark:bg-gray-800 rounded-lg shadow-lg p-6">
        <h2 className="text-2xl font-bold mb-4">🍰 Frazioni</h2>

        <div className="grid grid-cols-2 gap-4 mb-6">
          <div>
            <label className="block text-sm font-medium mb-2">
              Numeratore:
            </label>
            <input
              type="number"
              value={numerator}
              onChange={(e) => setNumerator(parseInt(e.target.value) || 1)}
              className="px-4 py-2 rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-900 w-full"
              min="1"
              max="10"
            />
          </div>

          <div>
            <label className="block text-sm font-medium mb-2">
              Denominatore:
            </label>
            <input
              type="number"
              value={denominator}
              onChange={(e) => setDenominator(Math.max(1, parseInt(e.target.value) || 1))}
              className="px-4 py-2 rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-900 w-full"
              min="1"
              max="10"
            />
          </div>
        </div>

        <div className="bg-gray-50 dark:bg-gray-900 rounded-lg p-6">
          <div
            dangerouslySetInnerHTML={{
              __html: formatFractionVisual(numerator, denominator),
            }}
          />
        </div>

        <div className="mt-4 text-sm text-gray-600 dark:text-gray-400">
          <p>
            {numerator} su {denominator} ={' '}
            {((numerator / denominator) * 100).toFixed(1)}%
          </p>
        </div>
      </section>

      {/* Math Steps */}
      <section className="bg-white dark:bg-gray-800 rounded-lg shadow-lg p-6">
        <h2 className="text-2xl font-bold mb-4">📝 Passaggi Matematici</h2>

        <div className="space-y-4">
          <div className="bg-gray-50 dark:bg-gray-900 rounded-lg p-4">
            <h3 className="font-semibold mb-2">Problema:</h3>
            <p className="text-2xl font-mono">2 + 3 × 4</p>
          </div>

          <div className="space-y-2">
            <h3 className="font-semibold">Passaggi:</h3>
            {formatMathStep('2 + 3 × 4').map((step, i) => (
              <div
                key={i}
                className="bg-blue-50 dark:bg-blue-900/20 border border-blue-200 dark:border-blue-800 rounded-lg p-3"
              >
                <div className="flex items-start gap-3">
                  <span className="flex-shrink-0 w-6 h-6 bg-blue-600 text-white rounded-full flex items-center justify-center text-sm font-bold">
                    {i + 1}
                  </span>
                  <p>{step}</p>
                </div>
              </div>
            ))}
          </div>
        </div>
      </section>

      {/* Example Problems */}
      <section className="bg-white dark:bg-gray-800 rounded-lg shadow-lg p-6">
        <h2 className="text-2xl font-bold mb-4">✏️ Esempi di Addizioni</h2>

        <div className="space-y-4">
          {[
            { a: 123, b: 45 },
            { a: 234, b: 567 },
            { a: 89, b: 12 },
          ].map(({ a, b }, i) => (
            <div
              key={i}
              className="bg-gray-50 dark:bg-gray-900 rounded-lg p-4"
            >
              <div className="flex items-center gap-4 text-3xl font-bold">
                <span
                  dangerouslySetInnerHTML={{
                    __html: formatNumberColored(a, useColors),
                  }}
                />
                <span className="text-gray-400">+</span>
                <span
                  dangerouslySetInnerHTML={{
                    __html: formatNumberColored(b, useColors),
                  }}
                />
                <span className="text-gray-400">=</span>
                <span
                  dangerouslySetInnerHTML={{
                    __html: formatNumberColored(a + b, useColors),
                  }}
                />
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* Color Legend */}
      <section className="bg-white dark:bg-gray-800 rounded-lg shadow-lg p-6">
        <h2 className="text-2xl font-bold mb-4">🎨 Legenda Colori</h2>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          <div className="bg-blue-50 dark:bg-blue-900/20 border border-blue-200 dark:border-blue-800 rounded-lg p-4">
            <div className="text-4xl font-bold text-blue-600 mb-2">1</div>
            <p className="font-semibold">Unità</p>
            <p className="text-sm text-gray-600 dark:text-gray-400">Blu</p>
          </div>

          <div className="bg-green-50 dark:bg-green-900/20 border border-green-200 dark:border-green-800 rounded-lg p-4">
            <div className="text-4xl font-bold text-green-600 mb-2">2</div>
            <p className="font-semibold">Decine</p>
            <p className="text-sm text-gray-600 dark:text-gray-400">Verde</p>
          </div>

          <div className="bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800 rounded-lg p-4">
            <div className="text-4xl font-bold text-red-600 mb-2">3</div>
            <p className="font-semibold">Centinaia</p>
            <p className="text-sm text-gray-600 dark:text-gray-400">Rosso</p>
          </div>
        </div>
      </section>
    </div>
  );
}
