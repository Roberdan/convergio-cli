'use client';

import { useEffect, useRef, useState, useCallback } from 'react';
import { motion } from 'framer-motion';
import mermaid from 'mermaid';
import { Printer, Download, ZoomIn, ZoomOut, Accessibility } from 'lucide-react';
import { cn } from '@/lib/utils';
import { logger } from '@/lib/logger';
import { useAccessibilityStore } from '@/lib/accessibility/accessibility-store';

interface MindmapNode {
  id: string;
  label: string;
  children?: MindmapNode[];
  icon?: string;
  color?: string;
}

interface MindmapRendererProps {
  title: string;
  nodes: MindmapNode[];
  className?: string;
}

// Initialize Mermaid with accessibility-focused settings
// CRITICAL: securityLevel 'loose' allows inline styles to prevent text truncation
const initMermaidAccessible = (dyslexiaFont: boolean, highContrast: boolean) => {
  mermaid.initialize({
    startOnLoad: false,
    securityLevel: 'loose', // Required for inline styles and proper text rendering
    theme: highContrast ? 'dark' : 'default',
    themeVariables: highContrast
      ? {
          primaryColor: '#ffff00',
          primaryTextColor: '#000000',
          primaryBorderColor: '#ffffff',
          lineColor: '#ffffff',
          secondaryColor: '#000080',
          background: '#000000',
          mainBkg: '#000000',
          nodeBorder: '#ffffff',
          fontFamily: dyslexiaFont ? 'OpenDyslexic, Comic Sans MS, sans-serif' : 'Arial, sans-serif',
        }
      : {
          primaryColor: '#3b82f6',
          primaryTextColor: '#1e293b',
          primaryBorderColor: '#64748b',
          lineColor: '#64748b',
          secondaryColor: '#e2e8f0',
          background: '#ffffff',
          mainBkg: '#f8fafc',
          nodeBorder: '#94a3b8',
          fontFamily: dyslexiaFont ? 'OpenDyslexic, Comic Sans MS, sans-serif' : 'Arial, sans-serif',
        },
    mindmap: {
      padding: 30,
      useMaxWidth: false, // Don't constrain width - allows full text
    },
  });
};

// Convert nodes to Mermaid mindmap syntax with markdown strings for text wrapping
function nodesToMermaidSyntax(nodes: MindmapNode[], title: string): string {
  // Escape label for Mermaid - use markdown string syntax with backticks for auto text wrapping
  const escapeLabel = (label: string): string => {
    if (!label || typeof label !== 'string') {
      return 'Untitled';
    }
    // Clean problematic characters but preserve the full text
    const cleaned = label
      .replace(/[()[\]{}]/g, '') // Remove brackets/parens (Mermaid syntax characters)
      .replace(/[<>]/g, '') // Remove angle brackets
      .replace(/["]/g, "'") // Replace double quotes with single
      .replace(/[`]/g, "'") // Replace backticks with single quotes (we use backticks for wrapping)
      .replace(/[\n\r]/g, ' ') // Replace newlines with space
      .replace(/\s+/g, ' ') // Normalize whitespace
      .trim();

    return cleaned || 'Node';
  };

  // Format label with line breaks for words longer than maxCharsPerLine
  // Mermaid markdown strings use backticks for multi-line text support
  const formatLabel = (label: string): string => {
    const escaped = escapeLabel(label);
    const maxCharsPerLine = 20;

    // Split text into words and build lines respecting max length
    const words = escaped.split(' ');
    const lines: string[] = [];
    let currentLine = '';

    for (const word of words) {
      if (currentLine.length === 0) {
        currentLine = word;
      } else if ((currentLine + ' ' + word).length <= maxCharsPerLine) {
        currentLine += ' ' + word;
      } else {
        lines.push(currentLine);
        currentLine = word;
      }
    }
    if (currentLine) {
      lines.push(currentLine);
    }

    // Join with <br> for line breaks in Mermaid markdown strings
    const multilineText = lines.join('<br>');
    return `\`${multilineText}\``;
  };

  const buildNode = (node: MindmapNode, depth: number = 1): string => {
    if (!node || !node.label) {
      return '';
    }

    // Mermaid mindmap uses 4-space indentation for each level
    const indent = '    '.repeat(depth);
    const formattedLabel = formatLabel(node.label);

    if (node.children && node.children.length > 0) {
      const validChildren = node.children
        .map((child) => buildNode(child, depth + 1))
        .filter((line) => line.trim() !== '');

      if (validChildren.length > 0) {
        return `${indent}${formattedLabel}\n${validChildren.join('\n')}`;
      }
    }

    return `${indent}${formattedLabel}`;
  };

  // Filter out empty/invalid nodes
  const validNodes = nodes.filter((n) => n && n.label);
  const rootContent = validNodes
    .map((node) => buildNode(node))
    .filter((line) => line.trim() !== '')
    .join('\n');

  // Use proper Mermaid mindmap syntax with markdown string for root
  const rootLabel = escapeLabel(title);
  return `mindmap
  root((${rootLabel}))
${rootContent}`;
}

export function MindmapRenderer({ title, nodes, className }: MindmapRendererProps) {
  const containerRef = useRef<HTMLDivElement>(null);
  const [error, setError] = useState<string | null>(null);
  const [rendered, setRendered] = useState(false);
  const [zoom, setZoom] = useState(1);
  const [accessibilityMode, setAccessibilityMode] = useState(false);

  const { settings } = useAccessibilityStore();

  // Generate text description for screen readers (defined before useEffect that uses it)
  const generateTextDescription = useCallback((rootTitle: string, nodeList: MindmapNode[]): string => {
    const describeNode = (node: MindmapNode, prefix: string = ''): string => {
      let desc = `${prefix}${node.label}`;
      if (node.children && node.children.length > 0) {
        desc += ': ' + node.children.map((c) => describeNode(c, '')).join(', ');
      }
      return desc;
    };

    return `${rootTitle} con i seguenti rami: ${nodeList.map((n) => describeNode(n)).join('; ')}`;
  }, []);

  // Render mindmap
  useEffect(() => {
    const renderMindmap = async () => {
      if (!containerRef.current) return;

      try {
        setError(null);
        setRendered(false);

        // Reinitialize Mermaid with current accessibility settings
        initMermaidAccessible(
          settings.dyslexiaFont || accessibilityMode,
          settings.highContrast || accessibilityMode
        );

        // Clear previous content
        containerRef.current.innerHTML = '';

        // Generate unique ID
        const id = `mindmap-${Date.now()}-${Math.random().toString(36).slice(2)}`;

        // Convert to Mermaid syntax
        const mermaidCode = nodesToMermaidSyntax(nodes, title);

        // Render
        const { svg } = await mermaid.render(id, mermaidCode);

        if (containerRef.current) {
          containerRef.current.innerHTML = svg;

          // Add ARIA attributes for accessibility
          const svgElement = containerRef.current.querySelector('svg');
          if (svgElement) {
            svgElement.setAttribute('role', 'img');
            svgElement.setAttribute('aria-label', `Mappa mentale: ${title}`);

            // CRITICAL FIX: Inject CSS to allow text wrapping and prevent clipping
            // Uses pre-wrap to respect <br> line breaks while allowing wrapping
            const styleEl = document.createElementNS('http://www.w3.org/2000/svg', 'style');
            styleEl.textContent = `
              .mindmap-node text,
              .node text,
              text,
              .nodeLabel,
              .label {
                overflow: visible !important;
                text-overflow: clip !important;
              }
              foreignObject,
              foreignObject div,
              .label-container {
                overflow: visible !important;
                white-space: pre-wrap !important;
                word-wrap: break-word !important;
                max-width: none !important;
              }
              .mindmap-node rect,
              .node rect,
              rect.label-container {
                rx: 8;
                ry: 8;
              }
              /* Ensure node backgrounds expand to fit text */
              .mindmap-node,
              .node {
                overflow: visible !important;
              }
              /* Support HTML line breaks in foreignObject */
              foreignObject br {
                display: block;
                content: "";
              }
            `;
            svgElement.insertBefore(styleEl, svgElement.firstChild);

            // Force SVG to auto-size
            svgElement.setAttribute('width', '100%');
            svgElement.setAttribute('height', 'auto');
            svgElement.style.minWidth = '800px';

            // Add title and desc elements for screen readers
            const titleEl = document.createElementNS('http://www.w3.org/2000/svg', 'title');
            titleEl.textContent = `Mappa mentale: ${title}`;
            svgElement.insertBefore(titleEl, svgElement.firstChild);

            const descEl = document.createElementNS('http://www.w3.org/2000/svg', 'desc');
            descEl.textContent = generateTextDescription(title, nodes);
            svgElement.insertBefore(descEl, svgElement.firstChild?.nextSibling || null);
          }

          setRendered(true);
        }
      } catch (err) {
        const errorMsg = err instanceof Error ? err.message : String(err);
        setError(errorMsg);
        logger.error('Mindmap render error', { error: String(err) });
      }
    };

    renderMindmap();
  }, [nodes, title, settings.dyslexiaFont, settings.highContrast, accessibilityMode, generateTextDescription]);

  // Print functionality
  const handlePrint = useCallback(() => {
    if (!containerRef.current) return;

    const printWindow = window.open('', '_blank');
    if (!printWindow) return;

    const svg = containerRef.current.innerHTML;

    printWindow.document.write(`
      <!DOCTYPE html>
      <html>
        <head>
          <title>Mappa Mentale: ${title}</title>
          <style>
            @import url('https://fonts.cdnfonts.com/css/opendyslexic');

            body {
              margin: 0;
              padding: 20mm;
              font-family: ${settings.dyslexiaFont ? 'OpenDyslexic, ' : ''}Arial, sans-serif;
              background: white;
            }

            h1 {
              text-align: center;
              font-size: ${settings.largeText ? '32pt' : '24pt'};
              margin-bottom: 20mm;
              ${settings.dyslexiaFont ? 'letter-spacing: 0.05em; line-height: 1.5;' : ''}
            }

            .mindmap-container {
              display: flex;
              justify-content: center;
              align-items: center;
            }

            .mindmap-container svg {
              max-width: 100%;
              height: auto;
            }

            /* High contrast print styles */
            ${settings.highContrast ? `
              body { background: white; }
              svg text { fill: black !important; font-weight: bold !important; }
              svg path, svg line { stroke: black !important; stroke-width: 2px !important; }
            ` : ''}

            @media print {
              @page {
                size: A4 landscape;
                margin: 15mm;
              }
              body {
                -webkit-print-color-adjust: exact;
                print-color-adjust: exact;
              }
            }
          </style>
        </head>
        <body>
          <h1>${title}</h1>
          <div class="mindmap-container">${svg}</div>
          <script>
            window.onload = function() {
              setTimeout(function() {
                window.print();
                window.close();
              }, 500);
            };
          </script>
        </body>
      </html>
    `);

    printWindow.document.close();
  }, [title, settings.dyslexiaFont, settings.largeText, settings.highContrast]);

  // Download as PNG (convert SVG to canvas then to PNG - better quality than JPG for diagrams)
  const handleDownload = useCallback(async () => {
    if (!containerRef.current) return;

    const svg = containerRef.current.querySelector('svg');
    if (!svg) return;

    try {
      // Clone SVG to avoid modifying the displayed one
      const svgClone = svg.cloneNode(true) as SVGSVGElement;

      // Get actual dimensions from viewBox or attributes
      const viewBox = svg.getAttribute('viewBox');
      let width = 1600; // Default high resolution
      let height = 1200;

      if (viewBox) {
        const parts = viewBox.split(' ').map(Number);
        if (parts.length === 4) {
          width = Math.max(parts[2] * 2, 1600); // At least 1600px wide
          height = Math.max(parts[3] * 2, 1200);
        }
      } else {
        const bbox = svg.getBBox();
        width = Math.max(bbox.width * 2, 1600);
        height = Math.max(bbox.height * 2, 1200);
      }

      // Set explicit dimensions on clone
      svgClone.setAttribute('width', String(width));
      svgClone.setAttribute('height', String(height));

      // Inline all computed styles to ensure they're preserved in export
      const allElements = svgClone.querySelectorAll('*');
      allElements.forEach((el) => {
        if (el instanceof SVGElement || el instanceof HTMLElement) {
          const computed = window.getComputedStyle(el);
          // Copy key style properties
          const stylesToCopy = ['fill', 'stroke', 'stroke-width', 'font-family', 'font-size', 'font-weight', 'opacity'];
          stylesToCopy.forEach((prop) => {
            const value = computed.getPropertyValue(prop);
            if (value && value !== 'none' && value !== 'initial') {
              (el as HTMLElement).style.setProperty(prop, value);
            }
          });
        }
      });

      // Create canvas
      const canvas = document.createElement('canvas');
      canvas.width = width;
      canvas.height = height;
      const ctx = canvas.getContext('2d');
      if (!ctx) {
        logger.error('Failed to get canvas context');
        return;
      }

      // Fill with white background
      ctx.fillStyle = '#ffffff';
      ctx.fillRect(0, 0, width, height);

      // Serialize SVG with proper XML declaration
      const serializer = new XMLSerializer();
      let svgString = serializer.serializeToString(svgClone);

      // Ensure proper XML namespace
      if (!svgString.includes('xmlns=')) {
        svgString = svgString.replace('<svg', '<svg xmlns="http://www.w3.org/2000/svg"');
      }

      // Encode as base64 data URL (more reliable than blob URL)
      const base64 = btoa(unescape(encodeURIComponent(svgString)));
      const dataUrl = `data:image/svg+xml;base64,${base64}`;

      // Load SVG into image
      const img = new Image();
      img.crossOrigin = 'anonymous';

      img.onload = () => {
        // Draw image on canvas
        ctx.drawImage(img, 0, 0, width, height);

        // Convert canvas to PNG and download
        canvas.toBlob(
          (blob) => {
            if (!blob) {
              logger.error('Failed to create blob from canvas');
              return;
            }
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `mappa-mentale-${title.toLowerCase().replace(/\s+/g, '-')}.png`;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
          },
          'image/png'
        );
      };

      img.onerror = (err) => {
        logger.error('Failed to load SVG for export', { error: String(err) });
        // Fallback: download as SVG directly
        const svgBlob = new Blob([svgString], { type: 'image/svg+xml' });
        const url = URL.createObjectURL(svgBlob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `mappa-mentale-${title.toLowerCase().replace(/\s+/g, '-')}.svg`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
      };

      img.src = dataUrl;
    } catch (err) {
      logger.error('Export error', { error: String(err) });
    }
  }, [title]);

  // Zoom controls
  const handleZoomIn = () => setZoom((z) => Math.min(z + 0.25, 3));
  const handleZoomOut = () => setZoom((z) => Math.max(z - 0.25, 0.5));

  return (
    <motion.div
      initial={{ opacity: 0, scale: 0.95 }}
      animate={{ opacity: 1, scale: 1 }}
      className={cn(
        'rounded-xl border overflow-hidden',
        settings.highContrast
          ? 'border-white bg-black'
          : 'border-slate-200 dark:border-slate-700 bg-white dark:bg-slate-800',
        className
      )}
      role="region"
      aria-label={`Mappa mentale: ${title}`}
    >
      {/* Toolbar */}
      <div
        className={cn(
          'flex items-center justify-between px-4 py-2 border-b',
          settings.highContrast
            ? 'border-white bg-black'
            : 'border-slate-200 dark:border-slate-700 bg-slate-50 dark:bg-slate-800/50'
        )}
      >
        <h3
          className={cn(
            'font-semibold',
            settings.dyslexiaFont && 'tracking-wide',
            settings.highContrast ? 'text-yellow-400' : 'text-slate-700 dark:text-slate-200'
          )}
          style={{
            fontSize: `${14 * (settings.largeText ? 1.2 : 1)}px`,
          }}
        >
          {title}
        </h3>

        <div className="flex items-center gap-2">
          {/* Accessibility toggle */}
          <button
            onClick={() => setAccessibilityMode(!accessibilityMode)}
            className={cn(
              'p-2 rounded-lg transition-colors',
              accessibilityMode
                ? 'bg-blue-500 text-white'
                : settings.highContrast
                  ? 'bg-yellow-400 text-black hover:bg-yellow-300'
                  : 'bg-slate-200 dark:bg-slate-700 hover:bg-slate-300 dark:hover:bg-slate-600'
            )}
            title="Modalità accessibilità"
            aria-label="Attiva/disattiva modalità accessibilità"
          >
            <Accessibility className="w-4 h-4" />
          </button>

          {/* Zoom controls */}
          <button
            onClick={handleZoomOut}
            className={cn(
              'p-2 rounded-lg transition-colors',
              settings.highContrast
                ? 'bg-yellow-400 text-black hover:bg-yellow-300'
                : 'bg-slate-200 dark:bg-slate-700 hover:bg-slate-300 dark:hover:bg-slate-600'
            )}
            title="Riduci zoom"
            aria-label="Riduci zoom"
          >
            <ZoomOut className="w-4 h-4" />
          </button>

          <span
            className={cn(
              'text-sm min-w-[4rem] text-center',
              settings.highContrast ? 'text-white' : 'text-slate-600 dark:text-slate-400'
            )}
          >
            {Math.round(zoom * 100)}%
          </span>

          <button
            onClick={handleZoomIn}
            className={cn(
              'p-2 rounded-lg transition-colors',
              settings.highContrast
                ? 'bg-yellow-400 text-black hover:bg-yellow-300'
                : 'bg-slate-200 dark:bg-slate-700 hover:bg-slate-300 dark:hover:bg-slate-600'
            )}
            title="Aumenta zoom"
            aria-label="Aumenta zoom"
          >
            <ZoomIn className="w-4 h-4" />
          </button>

          {/* Download */}
          <button
            onClick={handleDownload}
            className={cn(
              'p-2 rounded-lg transition-colors',
              settings.highContrast
                ? 'bg-yellow-400 text-black hover:bg-yellow-300'
                : 'bg-slate-200 dark:bg-slate-700 hover:bg-slate-300 dark:hover:bg-slate-600'
            )}
            title="Scarica PNG"
            aria-label="Scarica mappa come PNG"
          >
            <Download className="w-4 h-4" />
          </button>

          {/* Print */}
          <button
            onClick={handlePrint}
            className={cn(
              'p-2 rounded-lg transition-colors',
              settings.highContrast
                ? 'bg-yellow-400 text-black hover:bg-yellow-300'
                : 'bg-blue-500 text-white hover:bg-blue-600'
            )}
            title="Stampa"
            aria-label="Stampa mappa mentale"
          >
            <Printer className="w-4 h-4" />
          </button>
        </div>
      </div>

      {/* Mindmap container - uses wrapper to properly contain scaled content */}
      <div
        className={cn(
          'p-4 overflow-auto',
          settings.highContrast ? 'bg-black' : 'bg-white dark:bg-slate-900'
        )}
        style={{ maxHeight: '600px' }}
      >
        {error ? (
          <div
            className={cn(
              'p-4 rounded-lg text-sm',
              settings.highContrast
                ? 'bg-red-900 border-2 border-red-500 text-white'
                : 'bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800 text-red-600 dark:text-red-400'
            )}
            role="alert"
          >
            <strong>Errore:</strong> {error}
          </div>
        ) : (
          <div
            style={{
              // Wrapper expands to contain scaled content for proper scrolling
              width: `${100 * zoom}%`,
              minHeight: `${300 * zoom}px`,
            }}
          >
            <div
              ref={containerRef}
              className={cn(
                'flex justify-center items-center min-h-[300px] transition-transform',
                !rendered && 'animate-pulse rounded-lg',
                !rendered && (settings.highContrast ? 'bg-gray-800' : 'bg-slate-100 dark:bg-slate-700/50')
              )}
              style={{
                transform: `scale(${zoom})`,
                transformOrigin: 'left top',
              }}
            />
          </div>
        )}
      </div>

      {/* Text alternative for screen readers */}
      <div className="sr-only" aria-live="polite">
        {rendered && `Mappa mentale "${title}" con ${nodes.length} rami principali.`}
      </div>
    </motion.div>
  );
}

// Helper to create mindmap from simple structure
export function createMindmapFromTopics(
  title: string,
  topics: Array<{ name: string; subtopics?: string[] }>
): { title: string; nodes: MindmapNode[] } {
  return {
    title,
    nodes: topics.map((topic, i) => ({
      id: `topic-${i}`,
      label: topic.name,
      children: topic.subtopics?.map((sub, j) => ({
        id: `topic-${i}-sub-${j}`,
        label: sub,
      })),
    })),
  };
}

// Example usage for education
export const exampleMindmaps = {
  matematica: createMindmapFromTopics('Matematica', [
    { name: 'Algebra', subtopics: ['Equazioni', 'Disequazioni', 'Polinomi'] },
    { name: 'Geometria', subtopics: ['Triangoli', 'Cerchi', 'Solidi'] },
    { name: 'Analisi', subtopics: ['Limiti', 'Derivate', 'Integrali'] },
  ]),

  storia: createMindmapFromTopics('Storia', [
    { name: 'Antichità', subtopics: ['Grecia', 'Roma', 'Egitto'] },
    { name: 'Medioevo', subtopics: ['Feudalesimo', 'Crociate', 'Comuni'] },
    { name: 'Età Moderna', subtopics: ['Rinascimento', 'Scoperte', 'Riforma'] },
  ]),
};
